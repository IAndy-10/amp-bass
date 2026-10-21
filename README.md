# AmpBass

Bass amplifier plugin (faceplate name **EPIC-AMP-1**) that aims to replicate the sound of the **Ampeg SVT-3 PRO**
bass amplifier head (hardware): a clean preamp with three voicing switches, a passive-style
3-band tone stack with a 5-position mid frequency, a 9-band graphic EQ, a tube-style power stage with Tube Gain,
a 4x10 cabinet with horn and an FX Rack (Tight, Smooth, Noise Gate, Power Soak). The stage order and controls follow
the amp's owner's manual and schematics; the constants are fitted to measured responses.

Ampeg and SVT are trademarks of their owner. This project is not affiliated with or endorsed by Ampeg.

Version 1.1.0, by Epica Audio. VST3, AU and Standalone on macOS.

## User interface

The editor is a web view (Svelte) embedded in the plugin: a long, thin faceplate under a toolbar. The window resizes
from 960 to 1800 px wide and keeps the faceplate's 2562 × 558 shape.

- **Faceplate**: POWER (bypass) with its LED, Gain, Bass, Midrange with its 5-position FREQUENCY selector, Treble,
  Master and Tube Gain; the BRT, HI, LO, −15 dB, MUTE and EQ buttons with LEDs; the nine graphic-EQ sliders and Level
  (dimmed while EQ is off); the PEAK/MUTE LED.
- **Controls**: drag vertically, use the mouse wheel (Shift = fine), or click and use the arrow keys, Home and End.
  Double-click resets a control to its default. Hovering shows the value.
- **Right-click** on a control: Set to default, Copy value, Paste value (system clipboard; pasted text may carry
  units, e.g. "6.5" or "-3 dB"). In VST3 hosts that offer a parameter menu, the host's items (such as Show
  Automation) appear in the same menu; Ableton Live doesn't offer one.
- **Toolbar**: plugin power (same as POWER), undo and redo (also Cmd/Ctrl+Z and Cmd/Ctrl+Shift+Z; a whole drag is one
  step), the presets (only **R&B** has values so far; Rock, Synth and Upright are placeholders) and **FX Rack**.
  Copy, Paste, Reset and About are placeholders.
- **FX Rack panel** (toolbar button; Esc closes it): Input, Noise Gate (on, Threshold, Range), Tight and Smooth
  (Off / Pre / Post and frequency), Cabinet, Horn, Power Soak and Dry/Wet, as plain sliders and buttons. Double-click
  a slider to reset it. Changes made here aren't in the undo history.

## Stack

| Layer | Technology |
|---|---|
| DSP | C++17, JUCE 8.0.4 |
| UI | Svelte 5, TypeScript, Vite (single inlined `index.html`) |
| Bridge | JUCE 8 native relays (WebView) |
| Build | CMake 3.22+, FetchContent for JUCE |
| Formats | VST3, AU, Standalone (macOS) |

## Building

Prerequisites: CMake 3.22+, a C++17 compiler, Node.js 18+.

```bash
cmake -S . -B build
cmake --build build
```

CMake fetches JUCE, builds the Svelte UI, embeds it in the binary, compiles the plugin and copies the VST3/AU to
the system plugin folders. Pass `-DAMPBASS_COPY_PLUGIN=OFF` to skip the copy.

## Development

```bash
cd WebUI && npm install && npm run dev      # UI in the browser at localhost:5173 (no audio, controls start at 0)
cd WebUI && npm run build && npx svelte-check
```

To check the UI with the real plugin, build and open the Standalone app (`build/AmpBass_artefacts/Standalone/`)
or reload the plugin in a DAW.

## Project structure

```
amp-bass/
├── CMakeLists.txt
├── CHANGELOG.md
├── Source/
│   ├── DSP/                      # header-only stages
│   │   └── Primitives/           # Biquad, DryWetMixer
│   ├── Parameters/               # ParameterIDs.h (frozen), ParameterLayout.h
│   └── Plugin/                   # processor, editor (WebView + PEAK event), NativeBridge
└── WebUI/
    └── src/
        ├── bridge/               # bridge.ts + vendored JUCE frontend module
        ├── state/store.ts        # stores synced with the plugin parameters
        ├── lib/                  # faceplate and FX Rack components, actions, stores, tokens
        └── types/parameters.ts   # TypeScript mirror of ParameterIDs.h
```

## How it works

```
Input ─┬─► InputGain ► [Tight Pre] ► [Smooth Pre] ► PreFilters ► TubePreamp ► ToneStack ► GraphicEQ ► MasterVolume
       │      ► PowerSection ► [Tight Post] ► [Smooth Post] ► NoiseGate ► Cabinet ► PowerSoak ─┬─► Dry/Wet ► Bypass / Mute ► Output
       └──────────────── dry copy (also the gate's detector) ─────────────────────────────────────┘
```

- Each stage is a header-only class in `Source/DSP/` with `prepare()`, `setParams()` and `process()`.
  `PluginProcessor::processBlock` reads the parameters once per host block, then runs the chain in sub-blocks no
  larger than the size announced in `prepareToPlay`. The output doesn't depend on the host's block size.
- Every parameter change glides (10–50 ms; Dry/Wet 20 ms; Tube Gain 200 ms). Filter coefficients are recomputed every 32 samples
  and glide across them (`Biquad::glideTo`), so moving a knob doesn't buzz.
- Everything runs at the host sample rate with no oversampling and no latency; filters run in double precision.
  Left and right are processed independently.
- Only **PowerSection** is non-linear. Everything before it is gain or EQ, so the preamp, tone stack and EQ shape
  the level and spectrum that reach the power stage, and **Gain and Master trade 1:1**.

| Stage | What it does |
|---|---|
| InputGain | −15 dB pad and Input (−20…+20 dB), a pure gain before everything |
| PreFilters | Bright (+5.3 dB at 3 kHz), Ultra Hi (+5.3 dB at 5.6 kHz), Ultra Lo (−12.4 dB at 530 Hz, the scooped voicing) |
| TubePreamp | Linear: 25 Hz high-pass, then level 19 × Gain (+20 dB from Gain 1 to 10, silent at 0) |
| ToneStack | Fixed voicing (+4.6 dB at 57 Hz, −3.2 dB at 1 kHz, +2.3 dB at 5.9 kHz), then Bass shelf, Mid peak (249 / 543 / 895 / 1773 / 2884 Hz), Treble shelf and a Bass × Treble correction around 200–550 Hz, tabulated per knob position |
| GraphicEQ | Nine proportional-Q peaks (33 Hz … 7.7 kHz, labelled 9k), Level −10…+8 dB; off = crossfade to the unprocessed signal |
| MasterVolume | 0.5 × knob (−12 dB at the default 5) |
| PowerSection | Tube Gain level and a 2nd-harmonic "kink", an 872 Hz emphasis around a soft limiter (ceiling 1.64), a level-dependent bias, then a fixed output low-pass (14.6 kHz at 48 kHz) and a 10 Hz DC blocker. The character is at Tube Gain 0–2.5 |
| Tight / Smooth | First-order high-pass (30 Hz–3 kHz, with makeup gain) / low-pass (3–35 kHz) before (Pre) or after (Post) the amp |
| NoiseGate | Hears the input through a 348 Hz–3.9 kHz band, 32 ms envelope; below the threshold turns the output down by Range dB (100 = mute) |
| Cabinet | 4x10 cabinet (24 biquads) and horn (16), fitted per sample-rate family; switching crossfades over 50 ms |
| PowerSoak | Output attenuation, −40…0 dB |
| Dry/Wet | Equal-power crossfade with the untouched input |
| Bypass / Mute | 50 ms fades; bypassed = the input bit for bit, muted = silence |

The **PEAK/MUTE LED** lights while Mute is on, and for 300 ms after an overload: raw input ≥ 0.98, preamp output
≥ 1.0 (Gain too high for the input), or signal ≥ 1.0 after the tone stack and EQ.

**Levels at the defaults** (Gain 3.4, Tube Gain 5, Master 5, tone centred, EQ off, cabinet off): a quiet note
comes out about 9.5 dB louder and a −6 dBFS note is squashed by about 3 dB, mostly 3rd harmonic. Tube Gain 0 adds
a strong 2nd harmonic and clamps loud notes hard; Tube Gain 10 is 3 dB quieter and the cleanest.

### Where to tune

These constants shape the sound and aren't parameters, so changing them doesn't break saved sessions.

| Character | Constant | Where |
|---|---|---|
| Gain law | `kLevelAtFullGain` (19) | `TubePreamp.h` |
| Low-end tightness into the power stage | 25 Hz high-pass | `TubePreamp.h` `prepare` |
| Fixed voicing | `kVoicing*` | `ToneStack.h` |
| Bass / Mid / Treble shapes | `kBassCurve`, `kMidHz` / `kMidGainDb` / `kMidQ`, `kTrebleCurve`, `kTrebleTop*` | `ToneStack.h` |
| Voicing switches | `kBright*`, `kUltraHi*`, `kUltraLo*` | `PreFilters.h` |
| GEQ bands and slider law | `kBandHz`, `kBand12*`, `kLawSlider` / `kBoost*` / `kCut*`, `kOnHump*` | `GraphicEQ.h` |
| Master taper | `kGainAtFull` | `MasterVolume.h` |
| Power-stage softness and ceiling | `kHardness` (1.778), `kCeiling` (1.6435) | `PowerSection.h` |
| Which frequencies distort first | `kEmphasis*` (872 Hz, Q 0.33, +7.9 dB) | `PowerSection.h` |
| Tube Gain | `kLevelDb`, `kKinkDb`, `kDriveDb`, `kBias`, `kBlocking`, `kBlockingOnset`, `kKink*`, `kEnvelopeHz` | `PowerSection.h` |
| Top end | `kOutputLowPass*` (per sample-rate family) | `PowerSection.h` |
| Cabinet and horn | `k441` / `k48` / `k96` | `Cabinet.h` |
| Tight makeup gain | `kMakeupDbPerDecade` | `Tight.h` |
| Smooth cutoff | `kCutoffScale`, `kMatchFraction` | `Smooth.h` |
| Noise gate detector | `kHighPassHz`, `kLowPassHz`, `kEnvelopeSeconds`, `kGainSeconds` | `NoiseGate.h` |
| PEAK thresholds | `kPeakInput`, `kPeakDrive`, `kPeakPreMaster` | `PluginProcessor.h` |

There are no automated tests in the repository; the DSP tests and an offline VST3 host are kept in the git tag
`sound-matching-archive`.

Known limitations: after a very loud note at Tube Gain 0 the amp briefly chokes, which the 5 Hz envelope
doesn't reproduce; at extreme input levels there is 1–3 dB too little compression; one cabinet only, matched in
magnitude but not in phase; at 88.2 kHz and up about 1 dB of top-end difference.

## Parameters

37 parameters. IDs are frozen: changing one breaks saved sessions. Saved state carries a `stateVersion`, and
`setStateInformation` converts older sessions so they keep sounding the same.

| Group | Parameters |
|---|---|
| Input | −15 dB Pad, Input Gain (−20…+20 dB; FX Rack panel) |
| Pre-filters | Bright, Ultra Hi, Ultra Lo |
| Gain | Gain (preamp level), Tube Gain (in the power stage) |
| Tone | Bass, Midrange, Mid Frequency (220/450/800/1.6k/3k), Treble |
| Graphic EQ | On, 9 bands (ID `geq8k` = 9k band), Level |
| Output | Master Volume, Dry/Wet (FX Rack panel), Bypass, Mute |
| Cabinet | Cabinet, Horn (both on by default; FX Rack panel) |
| FX Rack | Tight (Off/Pre/Post, default Post), Tight Frequency (30 Hz–3 kHz, default 50 Hz), Power Soak (−40…0 dB, default −4), Smooth (Off/Pre/Post, default Post), Smooth Frequency (3–35 kHz, default 21 kHz), Noise Gate (default on), Gate Threshold (−120…0 dB, default −88), Gate Range (0…100 dB, default 50; 100 = muted) |
| Later | Preset (no function yet) |

Knobs read 0.0–10.0 and dB values one decimal. The toolbar's **FX Rack** button opens the panel with the
parameters that have no faceplate control.

## License

All rights reserved.
