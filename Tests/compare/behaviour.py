"""Behaviour checks for the sound-matching plan, step 8 (docs/sound-matching-plan.md).

Renders its own files with AmpBassHostRender into build-tests/compare/behaviour (not the main renders).

    python3 Tests/compare/behaviour.py rates      # 8.1: FR, level and compression at 44.1 / 96 kHz vs 48 kHz, both plugins
    python3 Tests/compare/behaviour.py blocks     # 8.2: AmpBass output must null across host block sizes (< -100 dB)
    python3 Tests/compare/behaviour.py stereo     # 8.3: crosstalk and L/R match, both plugins (from the main renders)
    python3 Tests/compare/behaviour.py switches   # 8.4: Bypass / Mute output and how the switch changes, both plugins
"""

from __future__ import annotations

import argparse
import json
import subprocess
import tempfile
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

import numpy as np
from scipy.io import wavfile
from scipy.signal import resample_poly

import analyze as A
import render as R
from cases import all_cases

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
SIGNALS = ROOT / "build-tests/compare/signals"
RENDERS = ROOT / "build-tests/compare/renders"
OUT = ROOT / "build-tests/compare/behaviour"
HOST = ROOT / "build-tests/Tests/AmpBassHostRender_artefacts/Release/AmpBassHostRender"
PLUGINS = ("reference", "ampbass")
CONFIG = json.loads((HERE / "plugins.json").read_text())
MANIFEST = json.loads((SIGNALS / "manifest.json").read_text())
SR = MANIFEST["sample_rate"]
PRE_S = MANIFEST["pre_samples"] / SR


def case_args(plugin: str, case_name: str) -> list[str]:
    case = next(c for c in all_cases() if c.name == case_name)
    cfg = CONFIG[plugin]
    baseline = {**cfg.get("baseline", {}), **case.baseline.get(plugin, {})}
    return R.to_args({**cfg, "baseline": baseline}, case.settings)


def render(plugin: str, case_name: str, src: Path, extra: list[str] | None = None) -> tuple[np.ndarray, int]:
    """Stereo render of `src` through a plugin with a case's settings; returns (samples x 2, sample rate)."""
    with tempfile.TemporaryDirectory() as tmp:
        dst = Path(tmp) / "out.wav"
        cmd = [str(HOST), "--plugin", str(Path(CONFIG[plugin]["path"]).expanduser()), "--in", str(src),
               "--out", str(dst), *case_args(plugin, case_name), *(extra or [])]
        res = subprocess.run(cmd, capture_output=True, text=True, check=False)
        if res.returncode != 0 or not dst.exists():
            raise SystemExit(f"render failed ({plugin}, {case_name}, {src.name}): {res.stderr[-400:]}")
        sr, y = wavfile.read(dst)
        return y.astype(np.float64), sr


def resampled(sig: str, sr: int) -> Path:
    """The 48 kHz test signal resampled to `sr` (cached in OUT/signals)."""
    if sr == SR:
        return SIGNALS / f"{sig}.wav"
    dst = OUT / "signals" / f"{sig}_{sr}.wav"
    if not dst.exists():
        dst.parent.mkdir(parents=True, exist_ok=True)
        _, x = wavfile.read(SIGNALS / f"{sig}.wav")
        g = np.gcd(sr, SR)
        y = resample_poly(x.astype(np.float64), sr // g, SR // g, axis=0)
        wavfile.write(dst, sr, y.astype(np.float32))
    return dst


def null_db(a: np.ndarray, b: np.ndarray) -> float:
    n = min(len(a), len(b))
    return float(A.db(A.rms(a[:n] - b[:n]) / max(A.rms(b[:n]), A.EPS)))


# ─── 8.1 Sample rate ─────────────────────────────────────────────────────

def measure_rate(plugin: str, case_name: str, sr: int) -> dict:
    pre = int(round(PRE_S * sr))
    sweep_info = MANIFEST["signals"]["sweep_30"]
    y, _ = render(plugin, case_name, resampled("sweep_30", sr))
    ir = A.impulse_response(y[:, 0], A.inverse_sweep(sweep_info, sr))
    fr = A.linear_fr(ir, int(np.argmax(np.abs(ir))), sr)
    tones, _ = render(plugin, case_name, resampled("tones_100", sr))
    steps = A.analyze_tones(tones[:, 0], MANIFEST["signals"]["tones_100"], sr, pre)["steps"]
    di, _ = render(plugin, case_name, resampled("di", sr))
    return {"fr": fr, "out": np.array([s["out_db"] for s in steps]),
            "h3": np.array([s["h_db"][1] for s in steps]), "di_db": float(A.db(A.rms(di[pre:, 0])))}


def cmd_rates(_: argparse.Namespace) -> None:
    rates = (44100, 48000, 96000)
    jobs = [(p, c, sr) for p in PLUGINS for c in ("clean_flat", "screenshot_preset") for sr in rates]
    with ThreadPoolExecutor(6) as ex:
        res = dict(zip(jobs, ex.map(lambda j: measure_rate(*j), jobs)))
    band = (A.FREQS >= A.FR_FULL[0]) & (A.FREQS <= A.FR_FULL[1])
    print("Difference from 48 kHz. FR (shape, 0 dB at 1 kHz): worst over 25 Hz-16 kHz (and at 20 kHz); tones_100: worst step level; "
          "H3 at the -16 dBFS step; DI level.")
    for p, c, sr in jobs:
        if sr == SR:
            continue
        r, b = res[(p, c, sr)], res[(p, c, SR)]
        d = r["fr"] - b["fr"]
        d -= d[int(np.argmin(np.abs(A.FREQS - 1000.0)))]   # the sweep IR's scale depends on the rate: compare shape
        worst = int(np.nanargmax(np.where(band, np.abs(d), -1)))
        i16 = MANIFEST["signals"]["tones_100"]["levels_db"].index(-16)
        print(f"{p:9s} {c:17s} {sr / 1000:5.1f} kHz: FR {d[worst]:+5.2f} dB @{A.FREQS[worst]:.0f} Hz "
              f"({d[-1]:+5.1f} @20k) | tones {np.max(np.abs(r['out'] - b['out'])):4.2f} dB | "
              f"H3 {r['h3'][i16] - b['h3'][i16]:+5.1f} dB | DI {r['di_db'] - b['di_db']:+5.2f} dB")


# ─── 8.2 Block size ──────────────────────────────────────────────────────

def cmd_blocks(_: argparse.Namespace) -> None:
    blocks = (32, 100, 512, 4096)
    cases = ("screenshot_preset", "gain7.5_tube0", "defaults")
    sigs = ("di", "bursts", "sweep_12")
    jobs = [(c, s, b) for c in cases for s in sigs for b in blocks]
    with ThreadPoolExecutor(6) as ex:
        out = dict(zip(jobs, ex.map(lambda j: render("ampbass", j[0], SIGNALS / f"{j[1]}.wav",
                                                     ["--block", str(j[2])])[0], jobs)))
    print("AmpBass null vs --block 512 (dB rel. the 512 render; target < -100):")
    for c in cases:
        for s in sigs:
            ref = out[(c, s, 512)]
            print(f"  {c:17s} {s:9s} " + "  ".join(f"{b:>4d}: {null_db(out[(c, s, b)], ref):7.1f}"
                                                  for b in blocks if b != 512))


# ─── 8.3 Stereo ──────────────────────────────────────────────────────────

def cmd_stereo(_: argparse.Namespace) -> None:
    info = MANIFEST["signals"]["left_only"]
    pre = MANIFEST["pre_samples"]
    for p in PLUGINS:
        x = A.crosstalk_db(RENDERS / p / "stereo_defaults" / "left_only.wav", info, SR, pre, A.Latency())
        lr = [A.lr_diff_db(RENDERS / p / c / "di.wav") for c in ("defaults", "screenshot_preset", "gain10_tube0")]
        print(f"{p:9s} left-only input, right channel rel. left: {x:7.1f} dB | identical L/R input, "
              f"L-R difference (defaults / screenshot / gain10_tube0): " + " / ".join(f"{v:.1f}" for v in lr) + " dB")


# ─── 8.4 Bypass and Mute ─────────────────────────────────────────────────

def switch_curve(y: np.ndarray, off: np.ndarray, on: np.ndarray, at: int, sr: int) -> tuple[float, float]:
    """Per 1 ms frame around a switch at sample `at`, the weight w of the 'on' render in y = (1 - w) off + w on.
    Returns (time from w = 0.1 to 0.9 in ms, overshoot of w beyond [0, 1])."""
    fr = sr // 1000
    ws = []
    for k in range(-20, 200):
        a = at + k * fr
        d = on[a:a + fr] - off[a:a + fr]
        den = float(np.dot(d, d))
        ws.append(float(np.dot(y[a:a + fr] - off[a:a + fr], d) / den) if den > 1e-12 else np.nan)
    w = np.array(ws)
    t10 = np.argmax(w >= 0.1) if np.any(w >= 0.1) else -1
    t90 = np.argmax(w >= 0.9) if np.any(w >= 0.9) else -1
    ramp = float(t90 - t10) if t10 >= 0 and t90 >= 0 else float("nan")
    return ramp, float(np.nanmax(np.abs(w - np.clip(w, 0, 1))))


def cmd_switches(_: argparse.Namespace) -> None:
    case, sig = "screenshot_preset", "di"
    src = SIGNALS / f"{sig}.wav"
    _, x = wavfile.read(src)
    x = x.astype(np.float64)
    names = {"reference": {"bypass": "#0", "mute": "Mute"}, "ampbass": {"bypass": "Bypass", "mute": "Mute"}}
    t_on, t_off = PRE_S + 2.0, PRE_S + 4.0
    for p in PLUGINS:
        wet, _ = render(p, case, src)
        for sw in ("bypass", "mute"):
            nm = names[p][sw]
            on, _ = render(p, case, src, ["--set", f"{nm}=1"])
            moved, _ = render(p, case, src, ["--set-at", f"{t_on}", f"{nm}=1", "--set-at", f"{t_off}", f"{nm}=0"])
            target = x if sw == "bypass" else np.zeros_like(x)
            static = (f"null vs input {null_db(on[:, 0], x[:, 0]):7.1f} dB" if sw == "bypass"
                      else f"peak {float(A.db(np.max(np.abs(on)))):7.1f} dBFS")
            a_on, a_off = int(round(t_on * SR)), int(round(t_off * SR))
            r_on, o_on = switch_curve(moved[:, 0], wet[:, 0], on[:, 0], a_on, SR)
            r_off, o_off = switch_curve(moved[:, 0], on[:, 0], wet[:, 0], a_off, SR)
            settled = null_db(moved[a_on + SR // 2:a_off, 0], target[a_on + SR // 2:a_off, 0]) if sw == "bypass" else \
                float(A.db(np.max(np.abs(moved[a_on + SR // 10:a_off, 0]))))
            print(f"{p:9s} {sw:6s} on: {static} | switching on: 10-90 % in {r_on:4.0f} ms (overshoot {o_on:.2f}), "
                  f"off: {r_off:4.0f} ms (overshoot {o_off:.2f}) | while on after switching: "
                  + (f"null vs input {settled:6.1f} dB" if sw == "bypass" else f"peak {settled:6.1f} dBFS"))


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    for name, fn in (("rates", cmd_rates), ("blocks", cmd_blocks), ("stereo", cmd_stereo), ("switches", cmd_switches)):
        sub.add_parser(name).set_defaults(fn=fn)
    args = ap.parse_args()
    if not HOST.exists():
        raise SystemExit(f"HostRender not found at {HOST}. Build it with: cmake --build build-tests")
    args.fn(args)


if __name__ == "__main__":
    main()
