# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What This Is

Bass amplifier plugin modeled after the Ampeg SVT-3 PRO. Hybrid DSP signal chain with tube preamp saturation and MOSFET power section. Built with JUCE 8 + Svelte 5.

## Build Commands

```bash
# Full plugin build (fetches JUCE, builds Svelte UI, compiles C++, copies to system plugin folder)
cd build && cmake .. && cmake --build .

# UI-only development (hot reload at localhost:5173, bridge functions stubbed)
cd WebUI && npm install && npm run dev

# UI production build
cd WebUI && tsc && npm run build
```

Prerequisites: CMake 3.22+, C++17 compiler, Node.js 18+.

No test suite exists yet. No linter is configured for C++. TypeScript checking is via `tsc` (part of the build script).

## Architecture

### Signal Chain (7 stages, processed in order)

```
Input → InputGain → PreFilters → TubePreamp → ToneStack → GraphicEQ → MasterVolume → PowerSection
```

Each DSP stage is a **header-only** class in `Source/DSP/` with `prepare(sampleRate, blockSize)` and `process(buffer, params)`. Stages are wired together in `PluginProcessor.cpp`.

### Three-Layer Architecture

1. **C++ DSP + Plugin** (`Source/`): JUCE 8 audio processor, APVTS parameter tree, DSP stages
2. **Native Bridge** (`Source/Plugin/NativeBridge.*` + `WebUI/src/bridge/`): JUCE 8 native relay integration (`withNativeIntegrationEnabled()`). Never use `juce://` URL intercept.
3. **Svelte UI** (`WebUI/`): Neumorphic component library, Svelte 5 stores backed by relay params

### Parameter System

- 24 parameters across 7 stages, defined in `Source/Parameters/ParameterIDs.h` (frozen IDs — changing breaks saved sessions) and `ParameterLayout.h`
- TypeScript mirror in `WebUI/src/types/parameters.ts`
- Bridge pushes only changed params from C++ to JS (delta > 0.001)
- Every float parameter on the audio thread uses `SmoothedValue` (10ms minimum)

### DSP Primitives (`Source/DSP/Primitives/`)

Shared building blocks: Biquad, DelayLine, OnePoleFilter, SmoothedParam, LFO, DryWetMixer. All header-only.

### UI Components (`WebUI/src/components/`)

Neumorphic component library: NeuKnob, NeuSlider, NeuButton, NeuSelector, NeuPanel, etc. Uses Tailwind CSS + Three.js optional shader layer.

## Key Conventions

- **Real-time safety in processBlock**: No heap allocations, no locks, no I/O, no string ops. Cache all component refs in `Awake()`-equivalent. Use `SmoothedParam` for parameter interpolation.
- **Sample-rate scaling**: All delay/filter calculations must scale by `sampleRate / 44100.0`.
- **Equal-power crossfade**: `cos/sin(mix * pi/2)`, not linear.
- **Filter cutoff interpolation**: Logarithmic, not linear Hz.
- **Bridge threading**: All bridge calls on message thread only.
- **Denormal flushing**: Required in all DSP stages.

## Building Process Toolkit

The `building-process/` directory is a **separate toolkit** used to generate and manage plugins (gitignored). It contains agent definitions, skills, hooks, rules, and workflows. See `building-process/CLAUDE.md` for its own conventions. Do not modify it as part of normal plugin development.
