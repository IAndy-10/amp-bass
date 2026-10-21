"""Test cases for the AmpBass vs reference comparison, in faceplate units.

Knobs are 0-10, GEQ/Level in dB, switches True/False, midFreq 1-5.
Each case lists which signals it is rendered with:
  - linear cases (EQ shape) use the quiet sweep, with the preamp kept clean;
  - drive cases (distortion, compression) use tones, ramp, bursts, loud sweep and DI.
"""

from __future__ import annotations

import math
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
DRIVE_SIGNALS = ["sweep_12", "tones_41", "tones_100", "tones_400", "tones_1600", "ramp", "bursts", "silence", "di"]

# Reference FX Rack defaults as normalised values (from AmpBassHostRender --list), minus the cabinet.
REF_FX_DEFAULTS: dict[str, float | str] = {
    "PowerSoak": 0.9,         # -4 dB
    "Tight": 1.0,             # Post
    "TightFrequency": 0.111,  # 50 Hz
    "Smooth": 1.0,            # Post
    "SmoothFrequency": 0.792, # 21 kHz
}

# Cabinet on in both plugins (step 12): the reference's default Rec Chain "20 4x10 DYN600+RIBBON", horn off / on.
CAB_ON: dict[str, dict[str, float | str]] = {"reference": {"RecChain": 19 / 26, "Horn": 0.0},
                                             "ampbass": {"Cabinet": 1.0, "Horn": 0.0}}
CAB_HORN: dict[str, dict[str, float | str]] = {"reference": {"RecChain": 19 / 26, "Horn": 1.0},
                                               "ampbass": {"Cabinet": 1.0, "Horn": 1.0}}

# FX Rack (step 9): Tight frequency 30 Hz-3 kHz on a log scale in both plugins; AmpBass FX values for REF_FX_DEFAULTS.
def tight_norm(hz: float) -> float:
    return math.log(hz / 30.0) / math.log(100.0)


def tight(mode: str, hz: float) -> dict[str, dict[str, float | str]]:
    t = {"pre": 0.5, "post": 1.0}[mode]
    return {"reference": {"Tight": t, "TightFrequency": tight_norm(hz)},
            "ampbass": {"Tight": t, "Tight Frequency": tight_norm(hz)}}


def smooth_norm(hz: float) -> float:
    return math.log(hz / 3000.0) / math.log(35.0 / 3.0)


def smooth(mode: str, hz: float) -> dict[str, dict[str, float | str]]:
    t = {"pre": 0.5, "post": 1.0}[mode]
    return {"reference": {"Smooth": t, "SmoothFrequency": smooth_norm(hz)},
            "ampbass": {"Smooth": t, "Smooth Frequency": smooth_norm(hz)}}


def gate(threshold_db: float, range_db: float) -> dict[str, dict[str, float | str]]:
    """Noise gate on in both plugins: threshold -120 ... 0 dB and Range 0 ... 100 dB, linear in both (100 = muted)."""
    t, r = (threshold_db + 120.0) / 120.0, range_db / 100.0
    return {"reference": {"NgByPass": 1.0, "NgThres": t, "NgRange": r},
            "ampbass": {"Noise Gate": 1.0, "Gate Threshold": t, "Gate Range": r}}


AMP_FX_DEFAULTS: dict[str, float | str] = {"Tight": 1.0, "Tight Frequency": 0.111, "Power Soak": 0.9,
                                           "Smooth": 1.0, "Smooth Frequency": 0.792}

# Both plugins as a fresh instance: cabinet, horn, FX Rack and (reference) gate at their defaults.
FACTORY: dict[str, dict[str, float | str]] = {
    "reference": {**REF_FX_DEFAULTS, "RecChain": 19 / 26, "Horn": 1.0, "NgByPass": 1.0, "NgThres": 0.267},
    "ampbass": {**AMP_FX_DEFAULTS, "Cabinet": 1.0, "Horn": 1.0, "Noise Gate": 1.0},
}

GEQ_BANDS = ["geq33", "geq80", "geq150", "geq300", "geq600", "geq900", "geq2k", "geq5k", "geq9k"]


@dataclass(frozen=True)
class Case:
    name: str
    settings: dict[str, float | bool]
    signals: list[str] = field(default_factory=list)
    # Per-plugin baseline overrides, merged over plugins.json's baseline:
    # {"reference": {"Tight": 1.0}} (numbers = normalised values, strings = display text).
    baseline: dict[str, dict[str, float | str]] = field(default_factory=dict)


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
        *[linear(f"{b}_-12", geqOn=True, **{b: -12}) for b in GEQ_BANDS],
        *[linear(f"geq300_{v:+g}", geqOn=True, geq300=v) for v in (-11.25, -10.5, -9, -6, -3, 3, 6, 9, 10.5, 11.25)],
        linear("geqLevel_-10", geqOn=True, geqLevel=-10),
        linear("geqLevel_+4", geqOn=True, geqLevel=4),
        drive("defaults"),
        *[drive(f"gain{g}_tube{t}", gain=g, tubeGain=t) for g in (0, 2.5, 7.5, 10) for t in (0, 5, 10)],
        *[drive(f"master{m}", master=m) for m in (2.5, 7.5, 10)],
        # Knob scales (0, 5 and 10 are covered above)
        *[linear(f"bass_{v}", bass=v) for v in (2.5, 7.5)],
        *[linear(f"treble_{v}", treble=v) for v in (2.5, 7.5)],
        *[linear(f"mid_{v}_f3", midrange=v) for v in (2.5, 7.5)],
        *[linear(f"mid_0_f{f}", midrange=0, midFreq=f) for f in (1, 2, 4, 5)],
        # Tone-stack interaction
        linear("bass10_treble0", bass=10, treble=0),
        linear("bass0_treble10", bass=0, treble=10),
        linear("mid0_bass10_treble10", midrange=0, bass=10, treble=10),
        linear("tone_shot", bass=7, midrange=6.75, treble=7),  # screenshot preset's tone knobs, clean
        linear("tone_shot_mid_f3", bass=7, midrange=6.75),     # same without treble, to split the interaction
        # Knob tapers between the old 0 / 2.5 / 5 / 7.5 / 10 grid (step 11.3)
        *[linear(f"treble_{v}", treble=v) for v in (1.25, 8.75)],
        linear("bass_8.75", bass=8.75),
        linear("mid_1.25_f1", midrange=1.25, midFreq=1),
        linear("bass8.75_treble8.75", bass=8.75, treble=8.75),
        # Topology: if the reference's harmonics change with a control, it sits before the distortion
        *[drive(f"topo_bass{v}", gain=7.5, bass=v) for v in (0, 10)],
        *[drive(f"topo_treble{v}", gain=7.5, treble=v) for v in (0, 10)],
        *[drive(f"topo_geq80_{v:+d}", gain=7.5, geqOn=True, geq80=v) for v in (-12, 12)],
        # Level laws (step 4): small-signal level per knob from the -40 dBFS step of the 100 Hz tones
        *[Case(f"law_gain{g}", {**DEFAULTS, "gain": g, "tubeGain": 10}, ["tones_100"])
          for g in (0, 0.5, 1, 2, 3, 3.4, 4, 5, 6, 7, 8, 9, 10)],
        *[Case(f"law_tube{t}", {**DEFAULTS, **CLEAN, "tubeGain": t}, ["tones_100"]) for t in (0, 2.5, 5, 7.5, 10)],
        *[Case(f"law_master{m}", {**DEFAULTS, **CLEAN, "master": m}, ["tones_100"])
          for m in (0, 0.5, 1, 2, 2.5, 3, 4, 5, 6, 7, 7.5, 8, 9, 10)],
        # Power-stage knee: clean preamp, Master varied
        *[Case(f"clean_master{m}", {**DEFAULTS, **CLEAN, "master": m}, DRIVE_SIGNALS) for m in (2.5, 5, 7.5, 10)],
        # Aliasing and channel handling
        Case("alias_gain10", {**DEFAULTS, "gain": 10}, ["alias_3k"]),
        Case("alias_defaults", dict(DEFAULTS), ["alias_3k"]),
        Case("stereo_defaults", dict(DEFAULTS), ["left_only"]),
        # Read off the reference screenshots in building-process/references (approximate).
        Case("screenshot_preset", {
            **DEFAULTS, "gain": 3.4, "bass": 7, "midrange": 6.75, "midFreq": 3, "treble": 7,
            "master": 6, "tubeGain": 3.5, "bright": True, "geqOn": True,
            "geq80": 5, "geq150": 4, "geq2k": 1.5, "geq5k": 3.5,
        }, LINEAR_SIGNALS + DRIVE_SIGNALS),
    ]
    # Same settings with the reference's FX Rack at its defaults (cabinet still bypassed): what the
    # screenshots sound like if the FX Rack was untouched.
    shot = cases[-1]
    cases.append(Case("screenshot_preset_fxdefault", shot.settings, shot.signals,
                      {"reference": REF_FX_DEFAULTS, "ampbass": AMP_FX_DEFAULTS}))
    cases += [
        Case("cab_clean_flat", {**DEFAULTS, **CLEAN}, LINEAR_SIGNALS, CAB_ON),
        Case("cab_horn_clean_flat", {**DEFAULTS, **CLEAN}, LINEAR_SIGNALS, CAB_HORN),
        Case("cab_horn_tone_shot", {**DEFAULTS, **CLEAN, "bass": 7, "midrange": 6.75, "treble": 7}, LINEAR_SIGNALS, CAB_HORN),
        Case("cab_horn_defaults", dict(DEFAULTS), DRIVE_SIGNALS, CAB_HORN),
        Case("cab_horn_screenshot_preset", shot.settings, shot.signals, CAB_HORN),
        # FX Rack (step 9)
        *[Case(f"tight_post_{hz}", {**DEFAULTS, **CLEAN}, LINEAR_SIGNALS, tight("post", hz)) for hz in (50, 300, 1500)],
        Case("tight_pre_300_gain7.5", {**DEFAULTS, "gain": 7.5}, DRIVE_SIGNALS, tight("pre", 300)),
        Case("tight_post_300_gain7.5", {**DEFAULTS, "gain": 7.5}, DRIVE_SIGNALS, tight("post", 300)),
        *[Case(f"smooth_post_{hz // 1000}k", {**DEFAULTS, **CLEAN}, LINEAR_SIGNALS, smooth("post", hz)) for hz in (3000, 21000)],
        Case("smooth_pre_6k_gain7.5", {**DEFAULTS, "gain": 7.5}, DRIVE_SIGNALS, smooth("pre", 6000)),
        Case("input_+10_gain5", dict(DEFAULTS), DRIVE_SIGNALS, {"reference": {"Input": 0.75}, "ampbass": {"Input Gain": 0.75}}),
        Case("powersoak_-20_gain7.5", {**DEFAULTS, "gain": 7.5}, DRIVE_SIGNALS,
             {"reference": {"PowerSoak": 0.5}, "ampbass": {"Power Soak": 0.5}}),
        Case("gate_-60_r20", dict(DEFAULTS), ["decay"], gate(-60, 20)),
        Case("gate_-50_mute", dict(DEFAULTS), ["decay"], gate(-50, 100)),
        Case("gate_-70_r50_gain7.5", {**DEFAULTS, "gain": 7.5}, ["decay"], gate(-70, 50)),
        Case("gate_defaults", dict(DEFAULTS), ["decay", "silence"], gate(-88, 50)),
        Case("factory_defaults", dict(DEFAULTS), LINEAR_SIGNALS + DRIVE_SIGNALS, FACTORY),
        Case("factory_screenshot_preset", shot.settings, shot.signals, FACTORY),
    ]
    names = [c.name for c in cases]
    assert len(names) == len(set(names)), "duplicate case names"
    return cases
