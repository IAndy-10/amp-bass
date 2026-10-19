"""Measuring and fitting tools for matching AmpBass to the reference (docs/sound-matching-plan.md).

Works on the renders made by render.py (build-tests/compare/renders) unless a command renders its own.

    python3 Tests/compare/probe.py fr clean_flat bass_10                  # FR rows, reference and AmpBass
    python3 Tests/compare/probe.py fr bass_0 bass_10 --minus clean_flat   # a knob's own curve (case minus noon)
    python3 Tests/compare/probe.py fit-shelf low bass_ 0 2.5 7.5 10       # RBJ shelf per knob position + shared fit
    python3 Tests/compare/probe.py fit-peaks clean_flat --peaks 2 --refit-lp 14602 0.382
    python3 Tests/compare/probe.py harmonics master=10                    # CLEAN + overrides: FR change with level, H2/H3
    python3 Tests/compare/probe.py topo topo_bass0 topo_bass10 tones_41   # is a control before the distortion?
    python3 Tests/compare/probe.py refnull defaults di PowerSoak=0.75 --vs PowerSoak=1.0
"""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
import tempfile
from pathlib import Path

import numpy as np
from scipy.optimize import minimize
from scipy.signal import freqz

import analyze as A
import render as R
from cases import CLEAN, DEFAULTS, all_cases

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
RENDERS = ROOT / "build-tests/compare/renders"
SIGNALS = ROOT / "build-tests/compare/signals"
HOST = ROOT / "build-tests/Tests/AmpBassHostRender_artefacts/Release/AmpBassHostRender"
PLUGINS = ("reference", "ampbass")
POINTS = (20, 25, 30, 40, 50, 60, 80, 100, 150, 200, 300, 400, 600, 800, 1000, 1500, 2000, 3000, 4000,
          5000, 6000, 7000, 8000, 10000, 12000, 16000, 20000)
F = A.FREQS


def manifest() -> dict:
    return json.loads((SIGNALS / "manifest.json").read_text())


def fr_of(y: np.ndarray, sig: str) -> np.ndarray:
    """Linear frequency response (dB rel. input) of output y for sweep signal sig."""
    m = manifest()
    info, sr = m["signals"][sig], m["sample_rate"]
    inv, x = A.inverse_sweep(info, sr), A.load(SIGNALS / f"{sig}.wav")
    ix = int(np.argmax(np.abs(A.impulse_response(x, inv))))
    return A.linear_fr(A.impulse_response(y, inv), ix, sr) - A.linear_fr(A.impulse_response(x, inv), ix, sr)


def fr(plugin: str, case: str, sig: str = "sweep_30") -> np.ndarray:
    return fr_of(A.load(RENDERS / plugin / case / f"{sig}.wav"), sig)


def at(v: np.ndarray, f: float) -> float:
    return float(np.interp(f, F, v))


def header() -> str:
    return "".ljust(22) + " ".join(f"{(str(f) if f < 1000 else f'{f / 1000:g}k'):>6}" for f in POINTS)


def row(label: str, v: np.ndarray) -> str:
    return label.ljust(22) + " ".join(f"{at(v, f):+6.1f}" for f in POINTS)


# ─── RBJ magnitude responses (sample rate from the manifest) ─────────────

def _mag(b: list[float], a: list[float]) -> np.ndarray:
    _, h = freqz(b, a, worN=F, fs=manifest()["sample_rate"])
    return 20 * np.log10(np.abs(h))


def _w(f0: float) -> tuple[float, float]:
    w = 2 * np.pi * f0 / manifest()["sample_rate"]
    return np.cos(w), np.sin(w)


def peak_db(f0: float, q: float, g: float) -> np.ndarray:
    a_, (c, s) = 10 ** (g / 40), _w(f0)
    al = s / (2 * q)
    return _mag([1 + al * a_, -2 * c, 1 - al * a_], [1 + al / a_, -2 * c, 1 - al / a_])


def lowpass_db(f0: float, q: float) -> np.ndarray:
    c, s = _w(f0)
    al = s / (2 * q)
    return _mag([(1 - c) / 2, 1 - c, (1 - c) / 2], [1 + al, -2 * c, 1 - al])


def shelf_db(kind: str, f0: float, q: float, g: float) -> np.ndarray:
    a_, (c, s) = 10 ** (g / 40), _w(f0)
    s2 = 2 * np.sqrt(a_) * s / (2 * q)
    if kind == "low":
        b = [a_ * ((a_ + 1) - (a_ - 1) * c + s2), 2 * a_ * ((a_ - 1) - (a_ + 1) * c), a_ * ((a_ + 1) - (a_ - 1) * c - s2)]
        a = [(a_ + 1) + (a_ - 1) * c + s2, -2 * ((a_ - 1) + (a_ + 1) * c), (a_ + 1) + (a_ - 1) * c - s2]
    else:
        b = [a_ * ((a_ + 1) + (a_ - 1) * c + s2), -2 * a_ * ((a_ - 1) + (a_ + 1) * c), a_ * ((a_ + 1) + (a_ - 1) * c - s2)]
        a = [(a_ + 1) - (a_ - 1) * c + s2, 2 * ((a_ - 1) - (a_ + 1) * c), (a_ + 1) - (a_ - 1) * c - s2]
    return _mag(b, a)


def fit(err, x0: list[float], starts: list[list[float]] | None = None):
    best = None
    for s in [x0] + (starts or []):
        r = minimize(err, s, method="Nelder-Mead",
                     options={"maxiter": 40_000, "maxfev": 40_000, "xatol": 1e-4, "fatol": 1e-10})
        if best is None or r.fun < best.fun:
            best = r
    return best


# ─── Rendering with ad-hoc settings ──────────────────────────────────────

def render_settings(plugin: str, settings: dict, sig: str, baseline: dict | None = None) -> np.ndarray:
    """Render one signal through a plugin with faceplate settings (and optional baseline overrides)."""
    cfg = json.loads((HERE / "plugins.json").read_text())[plugin]
    cfg = {**cfg, "baseline": {**cfg.get("baseline", {}), **(baseline or {})}}
    with tempfile.TemporaryDirectory() as tmp:
        dst = Path(tmp) / "out.wav"
        try:
            subprocess.run([str(HOST), "--plugin", str(Path(cfg["path"]).expanduser()),
                            "--in", str(SIGNALS / f"{sig}.wav"), "--out", str(dst), *R.to_args(cfg, settings)],
                           capture_output=True, check=True)
        except subprocess.CalledProcessError as e:
            raise SystemExit(f"render failed ({plugin}, {sig}): {e.stderr.decode(errors='replace')[-400:]}")
        return A.load(dst)


def parse_kv(items: list[str]) -> dict[str, float]:
    try:
        return {k: float(v) for k, v in (i.split("=", 1) for i in items)}
    except ValueError:
        raise SystemExit(f"expected key=number, got {items}")


# ─── Commands ────────────────────────────────────────────────────────────

def cmd_fr(args: argparse.Namespace) -> None:
    print(header())
    for case in args.cases:
        for p in PLUGINS:
            v = fr(p, case, args.signal)
            if args.minus:
                v = v - fr(p, args.minus, args.signal)
            print(row(f"{p[:3]} {case}", v))
        if not args.minus:
            print(row(f"ref-amp {case}", fr("reference", case, args.signal) - fr("ampbass", case, args.signal)))


def cmd_fit_shelf(args: argparse.Namespace) -> None:
    """Fit the reference's knob curve (case minus noon) with an RBJ shelf, per position and shared."""
    lo, hi = (20, 2000) if args.kind == "low" else (500, 16000)
    band = (F >= lo) & (F <= hi)
    noon = fr("reference", args.noon)
    curves = [fr("reference", f"{args.prefix}{v}") - noon for v in args.values]
    f0 = 120.0 if args.kind == "low" else 3000.0
    edge = (lambda d: float(d[band][0])) if args.kind == "low" else (lambda d: float(d[band][-1]))
    ok = lambda f, q: 10 <= f <= 20_000 and 0.2 <= q <= 3  # noqa: E731
    for v, d in zip(args.values, curves):
        r = fit(lambda p: np.mean((d[band] - shelf_db(args.kind, *p)[band]) ** 2) if ok(p[0], p[1]) else 1e9,
                [f0, 0.7, edge(d)])
        res = np.max(np.abs(d[band] - shelf_db(args.kind, *r.x)[band]))
        print(f"{args.prefix}{v}: {r.x[0]:8.1f} Hz  Q {r.x[1]:.3f}  {r.x[2]:+7.2f} dB  max error {res:.2f} dB")

    def jerr(p):
        if not ok(p[0], p[1]):
            return 1e9
        return np.mean([np.mean((d[band] - shelf_db(args.kind, p[0], p[1], g)[band]) ** 2) for d, g in zip(curves, p[2:])])
    r = fit(jerr, [f0, 0.6] + [edge(d) for d in curves])
    worst = max(np.max(np.abs(d[band] - shelf_db(args.kind, r.x[0], r.x[1], g)[band])) for d, g in zip(curves, r.x[2:]))
    print(f"shared: {r.x[0]:.1f} Hz  Q {r.x[1]:.3f}  gains {np.round(r.x[2:], 2).tolist()}  max error {worst:.2f} dB")


def cmd_fit_peaks(args: argparse.Namespace) -> None:
    """Fit N peaking filters (+ optional low-pass change) and a level offset to reference minus AmpBass."""
    diff = np.mean([fr("reference", c) - fr("ampbass", c) for c in args.cases], axis=0)
    band = (F >= args.lo) & (F <= args.hi)
    lp_old = lowpass_db(*args.refit_lp) if args.refit_lp else None
    seeds = [[57, 0.5, 6], [800, 0.5, -4], [6000, 0.7, 4], [2000, 0.7, 2]][: args.peaks]
    x0 = [v for s in seeds for v in s] + (list(args.refit_lp) if args.refit_lp else [])
    lims = [(20, 20000), (0.1, 4), (-20, 20)] * args.peaks + ([(4000, 22000), (0.3, 2)] if args.refit_lp else [])

    def model(p):
        m = sum(peak_db(*p[3 * i: 3 * i + 3]) for i in range(args.peaks))
        return m + (lowpass_db(p[-2], p[-1]) - lp_old if args.refit_lp else 0)

    def err(p):
        if any(not (lo <= v <= hi) for v, (lo, hi) in zip(p, lims)):
            return 1e9
        d = diff[band] - model(p)[band]
        return np.mean((d - d.mean()) ** 2)
    r = fit(err, x0)
    m = model(r.x)
    off = float(np.mean((diff - m)[band]))
    for i in range(args.peaks):
        f0, q, g = r.x[3 * i: 3 * i + 3]
        print(f"peak {i + 1}: {f0:8.1f} Hz  Q {q:.3f}  {g:+6.2f} dB")
    if args.refit_lp:
        print(f"low-pass: {r.x[-2]:.0f} Hz  Q {r.x[-1]:.3f}")
    print(f"level offset {off:+.2f} dB, rms {np.sqrt(r.fun):.2f} dB, max {np.max(np.abs((diff - m - off)[band])):.2f} dB")
    print(header())
    print(row("ref-amp", diff))
    print(row("residual", diff - m - off))


def cmd_harmonics(args: argparse.Namespace) -> None:
    """CLEAN settings + overrides: does the FR shape change with level, and H2/H3 vs input level (100 Hz)."""
    m = manifest()
    settings = {**DEFAULTS, **CLEAN, **parse_kv(args.overrides)}
    for p in PLUGINS:
        frs = {s: fr_of(render_settings(p, settings, s), s) for s in ("sweep_30", "sweep_12")}
        band = (F >= 30) & (F <= 12000)
        d = (frs["sweep_12"] - frs["sweep_30"])[band]
        d -= np.nanmedian(d)
        print(f"{p:9s} FR shape change -30 -> -12 dBFS: max {np.nanmax(np.abs(d)):.2f} dB")
        for sig in args.tones:
            t = A.analyze_tones(render_settings(p, settings, sig), m["signals"][sig], m["sample_rate"], m["pre_samples"])
            print(f"  {sig}: " + "  ".join(f"{s['in_db']}: H2 {s['h_db'][0]:.0f} H3 {s['h_db'][1]:.0f} out {s['out_db']:.0f}"
                                         for s in t["steps"] if s["in_db"] <= args.max_level))


def cmd_topo(args: argparse.Namespace) -> None:
    """Fundamental output (hi case minus lo case) vs input level. Constant = control after the distortion."""
    m = manifest()
    info = m["signals"][args.signal]
    for p in PLUGINS:
        t = {c: A.analyze_tones(A.load(RENDERS / p / c / f"{args.signal}.wav"), info, m["sample_rate"],
                                m["pre_samples"])["steps"] for c in (args.lo, args.hi)}
        print(f"{p:9s} " + "  ".join(f"{a['in_db']}: {b['out_db'] - a['out_db']:+.1f} (H3 {a['h_db'][1]:.0f}/{b['h_db'][1]:.0f})"
                                     for a, b in zip(t[args.lo], t[args.hi]) if a["in_db"] % 8 == 0))


def cmd_refnull(args: argparse.Namespace) -> None:
    """Render the reference with baseline overrides (normalised values) and null against another set."""
    case = next((c for c in all_cases() if c.name == args.case), None)
    if case is None:
        raise SystemExit(f"unknown case {args.case}")
    a = render_settings("reference", case.settings, args.signal, parse_kv(args.overrides))
    b = render_settings("reference", case.settings, args.signal, parse_kv(args.vs))
    n = min(len(a), len(b))
    a, b = a[:n], b[:n]
    g = float(np.dot(a, b) / max(np.dot(b, b), A.EPS))
    print(f"level {A.db(A.rms(a) / max(A.rms(b), A.EPS)):+.2f} dB, raw null {A.db(A.rms(a - b) / max(A.rms(b), A.EPS)):.1f} dB, "
          f"level-matched null {A.db(A.rms(a - g * b) / max(A.rms(a), A.EPS)):.1f} dB")


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)

    s = sub.add_parser("fr", help="print frequency-response rows")
    s.add_argument("cases", nargs="+")
    s.add_argument("--minus", help="subtract this case (per plugin), e.g. clean_flat to get a knob's curve")
    s.add_argument("--signal", default="sweep_30")
    s.set_defaults(func=cmd_fr)

    s = sub.add_parser("fit-shelf", help="fit a knob's curve with an RBJ shelf")
    s.add_argument("kind", choices=("low", "high"))
    s.add_argument("prefix", help="case name prefix, e.g. bass_")
    s.add_argument("values", nargs="+", help="knob positions in the case names, e.g. 0 2.5 7.5 10")
    s.add_argument("--noon", default="clean_flat")
    s.set_defaults(func=cmd_fit_shelf)

    s = sub.add_parser("fit-peaks", help="fit peaking filters to reference minus AmpBass")
    s.add_argument("cases", nargs="+")
    s.add_argument("--peaks", type=int, default=2, choices=range(1, 5))
    s.add_argument("--refit-lp", type=float, nargs=2, metavar=("HZ", "Q"),
                   help="also refit the output low-pass, starting from AmpBass's current one")
    s.add_argument("--lo", type=float, default=30.0)
    s.add_argument("--hi", type=float, default=20_000.0)
    s.set_defaults(func=cmd_fit_peaks)

    s = sub.add_parser("harmonics", help="render CLEAN + overrides and print H2/H3 vs level")
    s.add_argument("overrides", nargs="*", help="faceplate key=value, e.g. master=10")
    s.add_argument("--tones", nargs="+", default=["tones_100"])
    s.add_argument("--max-level", type=float, default=-12.0)
    s.set_defaults(func=cmd_harmonics)

    s = sub.add_parser("topo", help="does a control sit before the distortion?")
    s.add_argument("lo")
    s.add_argument("hi")
    s.add_argument("signal")
    s.set_defaults(func=cmd_topo)

    s = sub.add_parser("refnull", help="null the reference against itself with different baseline settings")
    s.add_argument("case")
    s.add_argument("signal")
    s.add_argument("overrides", nargs="*", help="reference parameter=normalised value")
    s.add_argument("--vs", nargs="*", default=[])
    s.set_defaults(func=cmd_refnull)

    args = ap.parse_args()
    args.func(args)


if __name__ == "__main__":
    sys.exit(main())
