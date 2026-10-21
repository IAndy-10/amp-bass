# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What This Is

Bass amplifier plugin (shown as **EPIC-AMP-1**) whose sound and layout follow the Brainworx / Plugin Alliance
**Ampeg SVT-3 PRO** plugin. A linear preamp; the distortion is a fitted tube-style power stage that also holds
Tube Gain (where the reference has it). JUCE 8 + Svelte 5.

## Build Commands

```bash
# Full plugin build (fetches JUCE, builds the Svelte UI, compiles C++, copies VST3/AU to the system folders)
cmake -S . -B build && cmake --build build
#   -DAMPBASS_COPY_PLUGIN=OFF   don't overwrite the installed plugin (secondary checkouts)

# UI-only development (hot reload at localhost:5173; no plugin backend, controls start at 0)
cd WebUI && npm install && npm run dev

# UI checks (run before committing UI changes)
cd WebUI && npm run build && npx svelte-check

```

No tests in the repository since the clean-up (user's choice, 2026-10-21). The DSP tests, the offline VST3 host and the
reference comparison (`Tests/`) are in the git tag `sound-matching-archive`; fit scripts and renders' sources are in
`../amp-bass-archive/`. To measure a sound change, check out `Tests/` from that tag into a scratch worktree.

Prerequisites: CMake 3.22+, C++17 compiler, Node.js 18+.
No C++ linter is configured.

## Architecture

### Signal chain (`PluginProcessor::processBlock`)

```
Input → InputGain → [Tight Pre] → [Smooth Pre] → PreFilters → TubePreamp → ToneStack → GraphicEQ → [PEAK check]
      → MasterVolume → PowerSection → [Tight Post] → [Smooth Post] → NoiseGate (hears the input) → Cabinet → PowerSoak
      → Dry/Wet → OutputSwitch (bypass, mute)
      → Output
```

Each stage is a **header-only** class in `Source/DSP/` with `prepare (sampleRate, blockSize)`,
`setParams (...)` and `process (buffer)`. The host block is processed in sub-blocks no larger than the
size given to `prepareToPlay`. `docs/how-ampbass-works.md` explains every stage with numbers.

### Three layers

1. **C++ DSP + plugin** (`Source/`): JUCE 8 processor, APVTS parameter tree, DSP stages.
2. **Native bridge** (`Source/Plugin/NativeBridge.*` + `WebUI/src/bridge/`): JUCE 8 relays
   (`withNativeIntegrationEnabled()`), using JUCE's frontend module vendored in `WebUI/src/bridge/juce/`.
   Int parameters (`midFreq`, `presetSelect`) use slider relays. The PEAK LED is a `peak` event sent by
   `PluginEditor::timerCallback`. Never use `juce://` URL intercept.
3. **Svelte UI** (`WebUI/src/lib/`): faceplate components positioned in reference pixels (2562 × 558 canvas).
   `src/state/store.ts` holds the stores synced with C++; `src/lib/stores/amp.ts` is a display-unit view
   that writes through the bridge (never keep a second copy of a value). The production build is a single
   inlined `index.html` embedded via BinaryData.

### Parameters

- 37 parameters in `Source/Parameters/ParameterIDs.h` (**frozen IDs** — changing one breaks saved sessions)
  and `ParameterLayout.h`; TypeScript mirror in `WebUI/src/types/parameters.ts`.
- `geq8k` is labelled 9k on the faceplate. `dryWet`, `cabinet`, `horn`, `tight`, `tightFreq`, `powerSoak`, `smooth`,
  `smoothFreq`, `inputGain`, `gate`, `gateThreshold` and `gateRange` are in the FX Rack panel (toolbar, `FxRack.svelte`),
  not on the faceplate. `presetSelect` has no control. `bypass` is also the host's bypass parameter.
- Every float parameter is smoothed on the audio thread with `juce::SmoothedValue` (10 ms minimum).
- Scales, display text and defaults match the reference plugin (same 0–1 value = same setting): knobs show 0.0–10.0,
  GEQ Level is piecewise (0 dB at the centre, mirrored in `WebUI/src/lib/stores/amp.ts`), defaults Gain 3.4 / Mid Freq 1.
  The UI's reset values in `amp.ts` must match `ParameterLayout.h`.
- Saved state carries `stateVersion` (`PluginProcessor.cpp`). Bump it when a range or meaning changes and convert older
  states in `setStateInformation`.

### DSP primitives (`Source/DSP/Primitives/`)

`Biquad` (RBJ cookbook, DF-II transposed) and `DryWetMixer`. Header-only.

## Key Conventions

- **Real-time safety in processBlock**: no heap allocations, no locks, no I/O, no string ops.
- **Sample rate**: filter coefficients are computed from the actual sample rate.
- **Equal-power crossfade**: `cos/sin(mix * pi/2)`, not linear.
- **Filter cutoff interpolation**: logarithmic, not linear Hz.
- **Bridge threading**: all bridge calls on the message thread.
- **Denormals**: `ScopedNoDenormals` in `processBlock`; filters also flush their state.
- **Sound changes are measured**: with the archived tests (`git worktree add ../amp-bass-tests sound-matching-archive`).

## Project files

- `BACKLOG.md` — open bugs, cleanup and planned work (P1–P3). Add findings here.
- `CHANGELOG.md` — one line per merge to `dev`.
- `docs/sound-matching-plan.md` — the matching plan: steps, the user's decisions, log. Read it before any DSP change.
- `docs/` is gitignored except `how-ampbass-works.md` and `sound-matching-plan.md`; the UI spec and reference material are local only.

## Building Process Toolkit

The `building-process/` directory is a **separate toolkit** used to generate and manage plugins (gitignored).
It contains agent definitions, skills, hooks, rules, workflows and reference material. See
`building-process/CLAUDE.md` for its own conventions. Do not modify or commit it as part of plugin development.
