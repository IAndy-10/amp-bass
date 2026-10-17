"""Test cases for the AmpBass vs reference comparison, in faceplate units.

Knobs are 0-10, GEQ/Level in dB, switches True/False, midFreq 1-5.
Each case lists which signals it is rendered with:
  - linear cases (EQ shape) use the quiet sweep, with the preamp kept clean;
  - drive cases (distortion, compression) use tones, ramp, bursts, loud sweep and DI.
"""

from __future__ import annotations

from dataclasses import dataclass, field

DEFAULTS: dict[str, float | bool] = {
    "gain": 5, "tubeGain": 5, "bass": 5, "midrange": 5, "midFreq": 3, "treble": 5, "master": 5,
    "bright": False, "inputPad": False, "ultraHi": False, "ultraLo": False, "geqOn": False,
    "geq33": 0, "geq80": 0, "geq150": 0, "geq300": 0, "geq600": 0, "geq900": 0,
    "geq2k": 0, "geq5k": 0, "geq9k": 0, "geqLevel": 0,
}

# Low Gain + full Tube Gain keeps the preamp out of saturation at -30 dBFS,
# so the sweep measures the EQ alone.
CLEAN = {"gain": 1, "tubeGain": 10}

LINEAR_SIGNALS = ["sweep_30"]
DRIVE_SIGNALS = ["sweep_12", "tones_41", "tones_100", "tones_400", "ramp", "bursts", "silence", "di"]

GEQ_BANDS = ["geq33", "geq80", "geq150", "geq300", "geq600", "geq900", "geq2k", "geq5k", "geq9k"]


@dataclass(frozen=True)
class Case:
    name: str
    settings: dict[str, float | bool]
    signals: list[str] = field(default_factory=list)


def linear(name: str, **overrides: float | bool) -> Case:
    return Case(name, {**DEFAULTS, **CLEAN, **overrides}, LINEAR_SIGNALS)


def drive(name: str, **overrides: float | bool) -> Case:
    return Case(name, {**DEFAULTS, **overrides}, DRIVE_SIGNALS)


def all_cases() -> list[Case]:
    cases = [
        linear("clean_flat"),
        linear("bass_0", bass=0), linear("bass_10", bass=10),
        linear("treble_0", treble=0), linear("treble_10", treble=10),
        linear("mid_0_f3", midrange=0),
        *[linear(f"mid_10_f{f}", midrange=10, midFreq=f) for f in range(1, 6)],
        linear("bright", bright=True), linear("ultraHi", ultraHi=True), linear("ultraLo", ultraLo=True),
        linear("pad", inputPad=True),
        linear("geq_on_flat", geqOn=True),
        *[linear(f"{b}_+12", geqOn=True, **{b: 12}) for b in GEQ_BANDS],
        linear("geqLevel_+8", geqOn=True, geqLevel=8),
        drive("defaults"),
        *[drive(f"gain{g}_tube{t}", gain=g, tubeGain=t) for g in (0, 2.5, 7.5, 10) for t in (0, 5, 10)],
        *[drive(f"master{m}", master=m) for m in (2.5, 7.5, 10)],
    ]
    names = [c.name for c in cases]
    assert len(names) == len(set(names)), "duplicate case names"
    return cases
