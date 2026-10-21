# AmpBass

Bass amplifier plugin (faceplate name **EPIC-AMP-1**) whose sound and layout follow the
Ampeg SVT-3 PRO plugin by Brainworx / Plugin Alliance: a clean preamp, a 3-band
semi-parametric tone stack, a 9-band graphic EQ and a tube-style power stage with Tube Gain,
all fitted to measurements of the reference.

## Stack

| Layer | Technology |
|---|---|
| DSP | C++17, JUCE 8.0.4 |
| UI | Svelte 5, TypeScript, Vite (single inlined `index.html`) |
| Bridge | JUCE 8 native relays (WebView) |
| Build | CMake 3.22+, FetchContent for JUCE |
| Formats | VST3, AU, Standalone (macOS) |

## Signal chain

```
Input → InputGain (−15 dB pad)
      → Tight, Pre (FX Rack high-pass, when set to Pre)
      → Smooth, Pre (FX Rack low-pass, when set to Pre)
      → PreFilters (Bright, Hi, Lo)
      → TubePreamp (linear: 25 Hz high-pass + Gain level)
      → ToneStack (Bass, Midrange + 5-position Frequency, Treble)
      → GraphicEQ (9 bands 33 Hz–9k, ±12 dB, Level −10…+8 dB, switchable)
      → MasterVolume
      → PowerSection (Tube Gain stage + soft power-amp curve, output low-pass)
      → Tight, Post (when set to Post: the default, 50 Hz)
      → Smooth, Post (when set to Post: the default, 21 kHz)
      → Noise Gate (turns the amp down while the input is below the threshold: on, −88 dB, Range 50 dB by default)
      → Cabinet (4x10 + horn, fitted to the reference's default Rec Chain; switchable)
      → Power Soak (output attenuation, −4 dB by default)
      → Dry/Wet → Bypass / Mute
```

A detailed, stage-by-stage explanation with the numbers that matter for tuning is in
[`docs/how-ampbass-works.md`](docs/how-ampbass-works.md).

## Building

```bash
# Prerequisites: CMake 3.22+, C++17 compiler, Node.js 18+
cmake -S . -B build
cmake --build build
```

CMake fetches JUCE, builds the Svelte UI, embeds it in the binary, compiles the plugin and
copies the VST3/AU to the system plugin folders. Pass `-DAMPBASS_COPY_PLUGIN=OFF` to skip the copy.

## Development

```bash
cd WebUI && npm install && npm run dev      # UI in the browser at localhost:5173 (no audio)
cd WebUI && npm run build && npx svelte-check
```

### Tests and measurement

```bash
cmake -S . -B build-tests -G Ninja -DAMPBASS_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release \
      -DFETCHCONTENT_SOURCE_DIR_JUCE=$PWD/build/_deps/juce-src
cmake --build build-tests
```

- `AmpBassDSPTests` — per-stage checks (gain, frequency response, THD, bypass/mute, block sizes).
- `AmpBassHostRender` — loads any VST3 offline: list/probe parameters, render WAV files.
- `Tests/compare/` — renders the same signals through AmpBass and the reference plugin and
  writes a comparison report with a regression gate; `behaviour.py` checks sample rates, block
  sizes, stereo and Bypass/Mute ([README](Tests/compare/README.md)).
- `docs/sound-matching-plan.md` — how AmpBass is matched to the reference, step by step, with a log.

## Project structure

```
amp-bass/
├── CMakeLists.txt
├── BACKLOG.md                    # open work, P1–P3
├── CHANGELOG.md
├── Source/
│   ├── DSP/                      # header-only stages + OutputSwitch (bypass/mute)
│   │   └── Primitives/           # Biquad, DryWetMixer
│   ├── Parameters/               # ParameterIDs.h (frozen), ParameterLayout.h
│   └── Plugin/                   # processor, editor (WebView + PEAK event), NativeBridge
├── Tests/                        # DSPTests, HostRender, compare/ (Python)
├── WebUI/
│   └── src/
│       ├── bridge/               # bridge.ts + vendored JUCE frontend module
│       ├── state/store.ts        # stores synced with the plugin parameters
│       ├── lib/                  # faceplate components, actions, amp/history stores, tokens
│       └── types/parameters.ts   # TypeScript mirror of ParameterIDs.h
└── docs/how-ampbass-works.md
```

## Parameters

38 parameters. IDs are frozen — changing one breaks saved sessions.

| Group | Parameters |
|---|---|
| Input | −15 dB Pad, Input Select (HI/LO, not on faceplate), Input Gain (−20…+20 dB like the reference's Input, not on faceplate) |
| Pre-filters | Bright, Ultra Hi, Ultra Lo |
| Gain | Gain (preamp level), Tube Gain (in the power stage) |
| Tone | Bass, Midrange, Mid Frequency (220/450/800/1.6k/3k), Treble |
| Graphic EQ | On, 9 bands (ID `geq8k` = 9k band), Level |
| Output | Master Volume, Dry/Wet (FX Rack panel), Bypass, Mute |
| Cabinet | Cabinet, Horn (both on by default; FX Rack panel) |
| FX Rack | Tight (Off/Pre/Post, default Post), Tight Frequency (30 Hz–3 kHz, default 50 Hz), Power Soak (−40…0 dB, default −4), Smooth (Off/Pre/Post, default Post), Smooth Frequency (3–35 kHz, default 21 kHz), Noise Gate (default on), Gate Threshold (−120…0 dB, default −88), Gate Range (0…100 dB, default 50; 100 = muted), Input Gain; in the FX Rack panel (toolbar) |
| Later | Preset (no function yet) |

## License

All rights reserved.
