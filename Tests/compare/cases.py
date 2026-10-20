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
DRIVE_SIGNALS = ["sweep_12", "tones_41", "tones_100", "tones_400", "tones_1600", "ramp", "bursts", "silence", "di"]

# Reference FX Rack defaults as normalised values (from AmpBassHostRender --list), minus the cabinet.
REF_FX_DEFAULTS: dict[str, float | str] = {
    "PowerSoak": 0.9,         # -4 dB
    "Tight": 1.0,             # Post
    "TightFrequency": 0.111,  # 50 Hz
    "Smooth": 1.0,            # Post
    "SmoothFrequency": 0.792, # 21 kHz
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
    # screenshots sound like if the FX Rack was untouched. AmpBass has no Tight / Power Soak yet (step 9).
    shot = cases[-1]
    cases.append(Case("screenshot_preset_fxdefault", shot.settings, shot.signals,
                      {"reference": REF_FX_DEFAULTS}))
    names = [c.name for c in cases]
    assert len(names) == len(set(names)), "duplicate case names"
    return cases
