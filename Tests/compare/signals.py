"""Generate the test-signal pack used to compare AmpBass with the reference plugin.

Every file is 48 kHz, stereo (identical channels, except left_only), 32-bit float, with 0.5 s of
silence before the signal (lets parameter smoothing settle) and 1 s after it
(keeps the tail of a plugin with latency). manifest.json records how each file
was built so analyze.py can find the steps, tones and levels again.

    python3 Tests/compare/signals.py [--out build-tests/compare/signals] [--di my_di.wav]
"""

from __future__ import annotations

import argparse
import json
import logging
from pathlib import Path

import numpy as np
from scipy.io import wavfile

SR = 48_000
PRE_S = 0.5
POST_S = 1.0

SWEEP_F1, SWEEP_F2, SWEEP_S = 20.0, 20_000.0, 6.0
TONE_FREQS = (41.0, 100.0, 400.0, 1600.0)
TONE_LEVELS_DB = tuple(range(-40, 1, 4))  # -40 … 0 dBFS
TONE_STEP_S = 0.5
RAMP_FREQ, RAMP_FROM_DB, RAMP_TO_DB, RAMP_S = 100.0, -50.0, 0.0, 10.0
BURST_FREQ, BURST_LOW_DB, BURST_HIGH_DB, BURST_S, BURST_PAIRS = 100.0, -30.0, -6.0, 0.5, 4
SILENCE_S = 3.0
ALIAS_FREQ, ALIAS_DB, ALIAS_S = 3_000.0, -6.0, 1.0  # loud HF tone: harmonics above Nyquist fold back
LEFT_FREQ, LEFT_DB, LEFT_S = 100.0, -20.0, 1.0      # left channel only: does a plugin mix channels?
DECAY_NOTES = (41.2, 55.0, 73.4, 98.0)               # E1 A1 D2 G2: decaying notes for the noise gate
DECAY_FROM_DB, DECAY_DB_PER_S, DECAY_NOTE_S, DECAY_GAP_S, DECAY_NOISE_DB = -12.0, 15.0, 4.5, 0.5, -80.0
FADE_S = 0.005

log = logging.getLogger("signals")


def db_to_amp(db: float) -> float:
    return float(10.0 ** (db / 20.0))


def pad(x: np.ndarray) -> np.ndarray:
    return np.concatenate([np.zeros(int(PRE_S * SR)), x, np.zeros(int(POST_S * SR))])


def fade(x: np.ndarray, seconds: float = FADE_S) -> np.ndarray:
    n = min(int(seconds * SR), len(x) // 2)
    if n > 0:
        ramp = np.linspace(0.0, 1.0, n)
        x[:n] *= ramp
        x[-n:] *= ramp[::-1]
    return x


def ess(level_db: float) -> np.ndarray:
    """Exponential sine sweep (Farina). The inverse filter is built in analyze.py."""
    t = np.arange(int(SWEEP_S * SR)) / SR
    k = np.log(SWEEP_F2 / SWEEP_F1)
    phase = 2 * np.pi * SWEEP_F1 * SWEEP_S / k * (np.exp(t * k / SWEEP_S) - 1)
    return fade(db_to_amp(level_db) * np.sin(phase), 0.02)


def stepped_tone(freq: float) -> np.ndarray:
    n = int(TONE_STEP_S * SR)
    t = np.arange(n) / SR
    steps = [fade(db_to_amp(db) * np.sin(2 * np.pi * freq * t)) for db in TONE_LEVELS_DB]
    return np.concatenate(steps)


def ramp() -> np.ndarray:
    n = int(RAMP_S * SR)
    t = np.arange(n) / SR
    level_db = np.linspace(RAMP_FROM_DB, RAMP_TO_DB, n)
    return fade(10.0 ** (level_db / 20.0) * np.sin(2 * np.pi * RAMP_FREQ * t))


def bursts() -> np.ndarray:
    n = int(BURST_S * SR)
    t = np.arange(n) / SR
    tone = np.sin(2 * np.pi * BURST_FREQ * t)
    # Level jumps are abrupt on purpose: they expose attack/release behaviour.
    parts = [tone * db_to_amp(db) for _ in range(BURST_PAIRS) for db in (BURST_LOW_DB, BURST_HIGH_DB)]
    return fade(np.concatenate(parts))


def decay() -> np.ndarray:
    """Plucked-like notes (harmonics falling 1.3/k) that fade 15 dB/s from -12 dBFS, over a -80 dBFS noise floor.
    A gate closes somewhere along each tail; the noise shows how far it turns the floor down."""
    n = int(DECAY_NOTE_S * SR)
    t = np.arange(n) / SR
    parts = []
    for f0 in DECAY_NOTES:
        x = sum(np.sin(2 * np.pi * f0 * k * t) / k ** 1.3 for k in range(1, 30) if f0 * k < 15_000)
        x = x / np.max(np.abs(x)) * db_to_amp(DECAY_FROM_DB) * 10.0 ** (-DECAY_DB_PER_S * t / 20.0)
        parts += [fade(x, 0.003), np.zeros(int(DECAY_GAP_S * SR))]
    x = np.concatenate(parts)
    noise = np.random.default_rng(1).standard_normal(len(x))   # fixed seed: the file is the same every run
    return x + db_to_amp(DECAY_NOISE_DB) * noise / np.sqrt(np.mean(noise ** 2))


def tone(freq: float, level_db: float, seconds: float) -> np.ndarray:
    t = np.arange(int(seconds * SR)) / SR
    return fade(db_to_amp(level_db) * np.sin(2 * np.pi * freq * t))


def write(path: Path, mono: np.ndarray, right: np.ndarray | None = None) -> None:
    stereo = np.stack([mono, mono if right is None else right], axis=1).astype(np.float32)
    wavfile.write(path, SR, stereo)
    log.info("wrote %s (%.1f s)", path.name, len(mono) / SR)


def load_di(path: Path) -> np.ndarray:
    try:
        sr, data = wavfile.read(path)
    except (OSError, ValueError) as e:
        raise SystemExit(f"Cannot read DI file {path}: {e}")
    if sr != SR:
        raise SystemExit(f"DI file must be {SR} Hz (got {sr}). Export it at 48 kHz.")
    # scipy returns 24-bit files as left-aligned int32, so int32 scaling covers both.
    scale = {np.dtype(np.int16): 32_768.0, np.dtype(np.int32): 2_147_483_648.0}.get(data.dtype, 1.0)
    mono = data.astype(np.float64) / scale
    return mono.mean(axis=1) if mono.ndim == 2 else mono


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--out", type=Path, default=Path("build-tests/compare/signals"))
    ap.add_argument("--di", type=Path, help="dry bass DI recording (48 kHz WAV) to include as di.wav")
    args = ap.parse_args()
    logging.basicConfig(level=logging.INFO, format="%(message)s")

    args.out.mkdir(parents=True, exist_ok=True)
    old_manifest = args.out / "manifest.json"
    old_di = json.loads(old_manifest.read_text())["signals"].get("di") if old_manifest.exists() else None
    pre = int(PRE_S * SR)
    manifest: dict[str, dict] = {"sample_rate": SR, "pre_samples": pre, "signals": {}}

    for level in (-30.0, -12.0):
        name = f"sweep_{int(-level)}"
        write(args.out / f"{name}.wav", pad(ess(level)))
        manifest["signals"][name] = {"kind": "sweep", "level_db": level, "f1": SWEEP_F1, "f2": SWEEP_F2,
                                     "seconds": SWEEP_S}

    for f in TONE_FREQS:
        name = f"tones_{int(f)}"
        write(args.out / f"{name}.wav", pad(stepped_tone(f)))
        manifest["signals"][name] = {"kind": "tones", "freq": f, "levels_db": list(TONE_LEVELS_DB),
                                     "step_seconds": TONE_STEP_S}

    write(args.out / "ramp.wav", pad(ramp()))
    manifest["signals"]["ramp"] = {"kind": "ramp", "freq": RAMP_FREQ, "from_db": RAMP_FROM_DB,
                                   "to_db": RAMP_TO_DB, "seconds": RAMP_S}

    write(args.out / "bursts.wav", pad(bursts()))
    manifest["signals"]["bursts"] = {"kind": "bursts", "freq": BURST_FREQ, "low_db": BURST_LOW_DB,
                                     "high_db": BURST_HIGH_DB, "step_seconds": BURST_S, "pairs": BURST_PAIRS}

    write(args.out / "silence.wav", np.zeros(int(SILENCE_S * SR)))
    manifest["signals"]["silence"] = {"kind": "silence"}

    write(args.out / "alias_3k.wav", pad(tone(ALIAS_FREQ, ALIAS_DB, ALIAS_S)))
    manifest["signals"]["alias_3k"] = {"kind": "alias", "freq": ALIAS_FREQ, "level_db": ALIAS_DB,
                                       "seconds": ALIAS_S}

    left = pad(tone(LEFT_FREQ, LEFT_DB, LEFT_S))
    write(args.out / "left_only.wav", left, np.zeros_like(left))
    manifest["signals"]["left_only"] = {"kind": "stereo", "freq": LEFT_FREQ, "level_db": LEFT_DB,
                                        "seconds": LEFT_S}

    write(args.out / "decay.wav", pad(decay()))
    manifest["signals"]["decay"] = {"kind": "decay", "notes": list(DECAY_NOTES), "from_db": DECAY_FROM_DB,
                                     "db_per_s": DECAY_DB_PER_S, "note_seconds": DECAY_NOTE_S,
                                     "gap_seconds": DECAY_GAP_S, "noise_db": DECAY_NOISE_DB}

    if args.di:
        write(args.out / "di.wav", pad(load_di(args.di)))
        manifest["signals"]["di"] = {"kind": "di", "source": args.di.name}
    elif old_di and (args.out / "di.wav").exists():
        manifest["signals"]["di"] = old_di  # keep the DI from an earlier run
        log.info("kept di.wav (%s)", old_di.get("source"))

    (args.out / "manifest.json").write_text(json.dumps(manifest, indent=2))
    log.info("manifest: %s", args.out / "manifest.json")


if __name__ == "__main__":
    main()
