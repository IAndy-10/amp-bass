"""Compare AmpBass renders with reference renders and write a report.

Reads <renders>/ampbass/<case>/<signal>.wav and <renders>/reference/<case>/<signal>.wav,
writes <out>/report.md, <out>/summary.json and one PNG per case.

    python3 Tests/compare/analyze.py [--cases bass treble]

Measurements (each plugin's latency is found from its clean_flat sweep and removed):
  sweep    frequency response via exponential-sweep deconvolution (Farina), 1/6-octave smoothed
  tones    output level, THD and harmonics H2-H5 at each input level
  ramp     output level vs input level (static compression curve)
  bursts   level envelope over time (attack / release)
  silence  output noise floor
  di       null test after time and level alignment, 1/3-octave level difference
"""

from __future__ import annotations

import argparse
import json
import logging
from dataclasses import asdict, dataclass, field
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402
import numpy as np  # noqa: E402
from scipy.io import wavfile  # noqa: E402
from scipy.signal import fftconvolve, welch  # noqa: E402

from cases import all_cases  # noqa: E402

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]

# Palette: categorical slots 1-2 of the dataviz reference palette (validated pair).
C_AMP, C_REF = "#2a78d6", "#eb6834"
INK, INK_SOFT, GRID, SURFACE = "#1a1a19", "#6b6a64", "#e4e3de", "#fcfcfb"
FREQS = np.geomspace(20, 20_000, 240)
FR_BAND = (40.0, 8_000.0)  # where FR differences are scored
EPS = 1e-12

log = logging.getLogger("analyze")


# ─── I/O ────────────────────────────────────────────────────────────────

def load(path: Path) -> np.ndarray:
    try:
        sr, data = wavfile.read(path)
    except (OSError, ValueError) as e:
        raise SystemExit(f"Cannot read {path}: {e}")
    scale = {np.dtype(np.int16): 32_768.0, np.dtype(np.int32): 2_147_483_648.0}.get(data.dtype, 1.0)
    x = data.astype(np.float64) / scale
    return x[:, 0] if x.ndim == 2 else x


def db(x: np.ndarray | float) -> np.ndarray | float:
    return 20 * np.log10(np.maximum(np.abs(x), EPS))


def rms(x: np.ndarray) -> float:
    return float(np.sqrt(np.mean(x * x))) if len(x) else 0.0


# ─── Sweep → frequency response ─────────────────────────────────────────

def inverse_sweep(info: dict, sr: int) -> np.ndarray:
    n = int(info["seconds"] * sr)
    t = np.arange(n) / sr
    k = np.log(info["f2"] / info["f1"])
    sweep = np.sin(2 * np.pi * info["f1"] * info["seconds"] / k * (np.exp(t * k / info["seconds"]) - 1))
    return sweep[::-1] * np.exp(-t * k / info["seconds"])


def impulse_response(y: np.ndarray, inv: np.ndarray) -> np.ndarray:
    return fftconvolve(y, inv)


def linear_fr(ir: np.ndarray, peak: int, sr: int) -> np.ndarray:
    """Magnitude (dB) at FREQS of the linear IR around `peak`; harmonic IRs sit >0.5 s earlier."""
    pre, post = int(0.005 * sr), int(0.3 * sr)
    seg = ir[max(0, peak - pre): peak + post].copy()
    taper = int(0.002 * sr)
    seg[:taper] *= np.hanning(2 * taper)[:taper]
    seg[-taper:] *= np.hanning(2 * taper)[taper:]
    spec = np.fft.rfft(seg, n=1 << 18)
    f = np.fft.rfftfreq(1 << 18, 1 / sr)
    mag = np.abs(spec)
    out = np.empty_like(FREQS)
    for i, fc in enumerate(FREQS):  # 1/6-octave power average
        band = (f >= fc * 2 ** (-1 / 12)) & (f < fc * 2 ** (1 / 12))
        out[i] = 10 * np.log10(np.mean(mag[band] ** 2) + EPS) if band.any() else np.nan
    return out


@dataclass
class Latency:
    samples: int = 0
    found: bool = False


def find_latency(y: np.ndarray, x: np.ndarray, inv: np.ndarray, pre: int) -> Latency:
    """Latency from where the output IR peaks vs where the input IR peaks."""
    ix = int(np.argmax(np.abs(impulse_response(x, inv))))
    iy = int(np.argmax(np.abs(impulse_response(y, inv))))
    return Latency(iy - ix, True)


# ─── Tones / ramp / bursts / DI ─────────────────────────────────────────

def tone_amp(seg: np.ndarray, f: float, sr: int) -> float:
    t = np.arange(len(seg)) / sr
    w = np.hanning(len(seg))
    c = np.sum(seg * w * np.cos(2 * np.pi * f * t))
    s = np.sum(seg * w * np.sin(2 * np.pi * f * t))
    return float(2 * np.hypot(c, s) / np.sum(w))


def analyze_tones(y: np.ndarray, info: dict, sr: int, start: int) -> dict:
    step = int(info["step_seconds"] * sr)
    f = info["freq"]
    rows = []
    for i, level in enumerate(info["levels_db"]):
        a = start + i * step
        seg = y[a + int(0.3 * step): a + int(0.9 * step)]
        fund = tone_amp(seg, f, sr)
        harm = [tone_amp(seg, f * h, sr) for h in range(2, 6) if f * h < sr / 2]
        thd = float(np.sqrt(sum(h * h for h in harm)) / max(fund, EPS))
        rows.append({"in_db": level, "out_db": float(db(fund)), "thd_pct": 100 * thd,
                     "h_db": [float(db(h / max(fund, EPS))) for h in harm]})
    return {"freq": f, "steps": rows}


def envelope_db(y: np.ndarray, sr: int, win_s: float) -> np.ndarray:
    n = max(1, int(win_s * sr))
    frames = len(y) // n
    return np.array([db(rms(y[i * n:(i + 1) * n])) for i in range(frames)])


def analyze_ramp(y: np.ndarray, info: dict, sr: int, start: int) -> dict:
    n = int(info["seconds"] * sr)
    env = envelope_db(y[start: start + n], sr, 0.05)
    in_db = np.linspace(info["from_db"], info["to_db"], len(env)) - 3.01  # sine RMS = peak - 3 dB
    return {"in_db": in_db.tolist(), "out_db": env.tolist()}


def analyze_bursts(y: np.ndarray, info: dict, sr: int, start: int) -> dict:
    n = int(info["step_seconds"] * sr * 2 * info["pairs"])
    env = envelope_db(y[start: start + n], sr, 0.02)  # 2 cycles of 100 Hz
    return {"t": (np.arange(len(env)) * 0.02).tolist(), "env_db": env.tolist()}


def third_octave_db(y: np.ndarray, sr: int) -> tuple[np.ndarray, np.ndarray]:
    f, p = welch(y, sr, nperseg=8192)
    centres = 1000 * 2 ** (np.arange(-17, 14) / 3)  # 20 Hz … 20 kHz
    lv = np.array([10 * np.log10(np.sum(p[(f >= c * 2 ** (-1 / 6)) & (f < c * 2 ** (1 / 6))]) + EPS)
                   for c in centres])
    return centres, lv


def null_db(a: np.ndarray, b: np.ndarray) -> tuple[float, float]:
    """Residual of a vs b after the best gain on a. Returns (null dB rel. b, gain dB applied)."""
    n = min(len(a), len(b))
    a, b = a[:n], b[:n]
    g = float(np.dot(a, b) / max(np.dot(a, a), EPS))
    return float(db(rms(b - g * a) / max(rms(b), EPS))), float(db(g))


# ─── Per-case analysis ──────────────────────────────────────────────────

@dataclass
class CaseResult:
    name: str
    level_diff_db: float | None = None     # reference minus AmpBass, main signal RMS
    fr_max_diff_db: float | None = None    # after removing level offset, within FR_BAND
    fr_worst_hz: float | None = None
    thd_amp_pct: float | None = None       # 100 Hz tone at -16 dBFS
    thd_ref_pct: float | None = None
    di_null_db: float | None = None
    noise_amp_db: float | None = None
    noise_ref_db: float | None = None
    plots: dict = field(default_factory=dict)


def shifted(y: np.ndarray, lat: Latency) -> np.ndarray:
    return y[lat.samples:] if lat.samples > 0 else y


def analyze_case(name: str, renders: Path, signals: Path, manifest: dict,
                 lat: dict[str, Latency], out: Path) -> CaseResult | None:
    sr, pre = manifest["sample_rate"], manifest["pre_samples"]
    res = CaseResult(name)
    data: dict[str, dict[str, dict]] = {}

    for sig, info in manifest["signals"].items():
        files = {p: renders / p / name / f"{sig}.wav" for p in ("ampbass", "reference")}
        if not all(f.exists() for f in files.values()):
            continue
        ys = {p: shifted(load(f), lat[p]) for p, f in files.items()}
        kind = info["kind"]
        d: dict[str, dict] = {}
        if kind == "sweep":
            inv = inverse_sweep(info, sr)
            x = load(signals / f"{sig}.wav")
            ix = int(np.argmax(np.abs(impulse_response(x, inv))))
            fr_in = linear_fr(impulse_response(x, inv), ix, sr)
            for p, y in ys.items():
                d[p] = {"fr_db": (linear_fr(impulse_response(y, inv), ix, sr) - fr_in).tolist()}
        elif kind == "tones":
            d = {p: analyze_tones(y, info, sr, pre) for p, y in ys.items()}
        elif kind == "ramp":
            d = {p: analyze_ramp(y, info, sr, pre) for p, y in ys.items()}
        elif kind == "bursts":
            d = {p: analyze_bursts(y, info, sr, pre) for p, y in ys.items()}
        elif kind == "silence":
            # skip the lead-in: start-up transients are not noise
            d = {p: {"noise_db": float(db(rms(y[pre:])))} for p, y in ys.items()}
        elif kind == "di":
            centres, la = third_octave_db(ys["ampbass"], sr)
            _, lr = third_octave_db(ys["reference"], sr)
            nd, _ = null_db(ys["ampbass"], ys["reference"])
            d = {"ampbass": {"bands_db": la.tolist()}, "reference": {"bands_db": lr.tolist()},
                 "both": {"centres": centres.tolist(), "null_db": nd}}
        data[sig] = d

        if res.level_diff_db is None and kind in ("sweep", "di"):
            res.level_diff_db = float(db(rms(ys["reference"]) / max(rms(ys["ampbass"]), EPS)))

    if not data:
        return None

    for sig in ("sweep_30", "sweep_12"):
        if sig in data:
            a, r = np.array(data[sig]["ampbass"]["fr_db"]), np.array(data[sig]["reference"]["fr_db"])
            band = (FREQS >= FR_BAND[0]) & (FREQS <= FR_BAND[1])
            diff = (r - a)[band]
            diff -= np.nanmedian(diff)  # shape only; level is reported separately
            i = int(np.nanargmax(np.abs(diff)))
            res.fr_max_diff_db, res.fr_worst_hz = float(diff[i]), float(FREQS[band][i])
            break
    if "tones_100" in data:
        pick = lambda p: next(s["thd_pct"] for s in data["tones_100"][p]["steps"] if s["in_db"] == -16)  # noqa: E731
        res.thd_amp_pct, res.thd_ref_pct = pick("ampbass"), pick("reference")
    if "di" in data:
        res.di_null_db = data["di"]["both"]["null_db"]
    if "silence" in data:
        res.noise_amp_db = data["silence"]["ampbass"]["noise_db"]
        res.noise_ref_db = data["silence"]["reference"]["noise_db"]

    plot_case(name, data, out / f"{name}.png")
    res.plots["png"] = f"{name}.png"
    return res


# ─── Plotting ───────────────────────────────────────────────────────────

def style(ax: plt.Axes, title: str, xlabel: str, ylabel: str) -> None:
    ax.set_title(title, loc="left", fontsize=10, color=INK)
    ax.set_xlabel(xlabel, fontsize=8, color=INK_SOFT)
    ax.set_ylabel(ylabel, fontsize=8, color=INK_SOFT)
    ax.tick_params(colors=INK_SOFT, labelsize=7, length=0)
    ax.grid(True, color=GRID, linewidth=0.6)
    ax.set_facecolor(SURFACE)
    for s in ax.spines.values():
        s.set_visible(False)


def pair(ax: plt.Axes, x_a, y_a, x_r, y_r) -> None:
    ax.plot(x_a, y_a, color=C_AMP, linewidth=2, label="AmpBass")
    ax.plot(x_r, y_r, color=C_REF, linewidth=2, label="Reference")
    ax.legend(fontsize=7, frameon=False, labelcolor=INK)


def plot_case(name: str, data: dict, path: Path) -> None:
    panels = []
    for sig in ("sweep_30", "sweep_12"):
        if sig in data:
            panels.append(("fr", sig))
            panels.append(("frdiff", sig))
            break
    panels += [("tones", s) for s in ("tones_41", "tones_100", "tones_400") if s in data]
    panels += [(k, k) for k in ("ramp", "bursts", "di") if k in data]

    cols = 2
    rows = (len(panels) + cols - 1) // cols
    fig, axes = plt.subplots(rows, cols, figsize=(11, 3.2 * rows), squeeze=False, facecolor=SURFACE)
    for ax in axes.flat[len(panels):]:
        ax.set_visible(False)

    for ax, (kind, sig) in zip(axes.flat, panels):
        d = data[sig]
        if kind == "fr":
            pair(ax, FREQS, d["ampbass"]["fr_db"], FREQS, d["reference"]["fr_db"])
            ax.set_xscale("log")
            style(ax, f"Frequency response ({sig})", "Hz", "dB rel. input")
        elif kind == "frdiff":
            diff = np.array(d["reference"]["fr_db"]) - np.array(d["ampbass"]["fr_db"])
            ax.plot(FREQS, diff, color=INK, linewidth=2)
            ax.axhline(0, color=INK_SOFT, linewidth=0.8)
            ax.set_xscale("log")
            style(ax, "Reference minus AmpBass", "Hz", "dB")
        elif kind == "tones":
            ia = [s["in_db"] for s in d["ampbass"]["steps"]]
            pair(ax, ia, [s["thd_pct"] for s in d["ampbass"]["steps"]],
                 [s["in_db"] for s in d["reference"]["steps"]], [s["thd_pct"] for s in d["reference"]["steps"]])
            style(ax, f"THD vs input level, {int(d['ampbass']['freq'])} Hz", "input dBFS (peak)", "THD %")
        elif kind == "ramp":
            pair(ax, d["ampbass"]["in_db"], d["ampbass"]["out_db"], d["reference"]["in_db"], d["reference"]["out_db"])
            style(ax, "Output vs input level (100 Hz ramp)", "input dBFS (RMS)", "output dBFS (RMS)")
        elif kind == "bursts":
            pair(ax, d["ampbass"]["t"], d["ampbass"]["env_db"], d["reference"]["t"], d["reference"]["env_db"])
            style(ax, "Level envelope, -30/-6 dBFS bursts", "s", "dBFS (20 ms RMS)")
        elif kind == "di":
            c = d["both"]["centres"]
            pair(ax, c, d["ampbass"]["bands_db"], c, d["reference"]["bands_db"])
            ax.set_xscale("log")
            style(ax, f"DI, 1/3-octave levels (null {d['both']['null_db']:.1f} dB)", "Hz", "dB")

    fig.suptitle(name, x=0.01, ha="left", fontsize=12, color=INK)
    fig.tight_layout()
    fig.savefig(path, dpi=110, facecolor=SURFACE)
    plt.close(fig)


# ─── Report ─────────────────────────────────────────────────────────────

def fmt(v: float | None, spec: str) -> str:
    return "—" if v is None else format(v, spec)


def write_report(results: list[CaseResult], lat: dict[str, Latency], out: Path, self_test: bool) -> None:
    lines = [
        "# AmpBass vs reference",
        "",
        f"Latency removed: AmpBass {lat['ampbass'].samples} samples, reference {lat['reference'].samples} samples.",
        "",
        "Level = reference minus AmpBass (RMS). FR shape = worst difference "
        f"{FR_BAND[0]:.0f} Hz–{FR_BAND[1] / 1000:.0f} kHz after removing the level offset. "
        "THD at 100 Hz, −16 dBFS input. Null = DI residual after level match (lower is closer).",
        "",
        "| Case | Level dB | FR shape dB | at Hz | THD AmpBass % | THD ref % | DI null dB | Noise AmpBass / ref dBFS |",
        "|---|---:|---:|---:|---:|---:|---:|---|",
    ]
    for r in results:
        noise = "—" if r.noise_amp_db is None else f"{r.noise_amp_db:.0f} / {r.noise_ref_db:.0f}"
        lines.append(f"| [{r.name}]({r.plots.get('png', '')}) | {fmt(r.level_diff_db, '+.1f')} | "
                     f"{fmt(r.fr_max_diff_db, '+.1f')} | {fmt(r.fr_worst_hz, '.0f')} | "
                     f"{fmt(r.thd_amp_pct, '.1f')} | {fmt(r.thd_ref_pct, '.1f')} | {fmt(r.di_null_db, '.1f')} | {noise} |")
    if self_test:
        worst = max((abs(r.fr_max_diff_db or 0) for r in results), default=0)
        lines += ["", f"**Self-test:** worst FR difference {worst:.3f} dB (should be ~0)."]
    (out / "report.md").write_text("\n".join(lines) + "\n")
    (out / "summary.json").write_text(json.dumps([asdict(r) for r in results], indent=2))


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--renders", type=Path, default=ROOT / "build-tests/compare/renders")
    ap.add_argument("--signals", type=Path, default=ROOT / "build-tests/compare/signals")
    ap.add_argument("--out", type=Path, default=ROOT / "build-tests/compare/report")
    ap.add_argument("--cases", nargs="*", default=[])
    ap.add_argument("--self-test", action="store_true", help="note in the report that both sides are AmpBass")
    args = ap.parse_args()
    logging.basicConfig(level=logging.INFO, format="%(message)s")

    manifest = json.loads((args.signals / "manifest.json").read_text())
    sr, pre = manifest["sample_rate"], manifest["pre_samples"]
    args.out.mkdir(parents=True, exist_ok=True)

    # Latency per plugin from the clean, linear sweep.
    info = manifest["signals"]["sweep_30"]
    inv, x = inverse_sweep(info, sr), load(args.signals / "sweep_30.wav")
    lat: dict[str, Latency] = {}
    for p in ("ampbass", "reference"):
        f = args.renders / p / "clean_flat" / "sweep_30.wav"
        lat[p] = find_latency(load(f), x, inv, pre) if f.exists() else Latency()
        log.info("%s latency: %d samples%s", p, lat[p].samples, "" if lat[p].found else " (clean_flat missing, assumed 0)")

    results = []
    for case in all_cases():
        if args.cases and not any(f in case.name for f in args.cases):
            continue
        r = analyze_case(case.name, args.renders, args.signals, manifest, lat, args.out)
        if r:
            results.append(r)
            log.info("%-18s level %s  FR %s dB @ %s Hz", r.name, fmt(r.level_diff_db, "+.2f"),
                     fmt(r.fr_max_diff_db, "+.2f"), fmt(r.fr_worst_hz, ".0f"))
    write_report(results, lat, args.out, args.self_test)
    log.info("report: %s", args.out / "report.md")


if __name__ == "__main__":
    main()
