# AmpBass

Bass amplifier plugin modeled after the Ampeg SVT-3 PRO. Hybrid DSP signal chain with tube preamp saturation and MOSFET power section character.

## Stack

| Layer | Technology |
|---|---|
| DSP | C++17, JUCE 8.0.4 |
| UI | Svelte 5, TypeScript, Vite |
| Build | CMake 3.22+, FetchContent for JUCE |
| Formats | VST3, AU, Standalone |
| Bridge | JUCE 8 native relay integration |

## Signal Chain

```
Input → InputGain (-15dB pad, HI/LO)
      → PreFilters (Bright, Ultra HI, Ultra LO)
      → TubePreamp (12AX7 waveshaper + starved plate)
      → ToneStack (Bass/Mid+Freq/Treble)
      → GraphicEQ (9-band, bypassable)
      → MasterVolume
      → PowerSection (MOSFET soft clip + compression)
```

## Building

```bash
# Prerequisites: CMake 3.22+, C++17 compiler, Node.js 18+

# Clone and build
cd amp-bass
mkdir build && cd build
cmake ..
cmake --build .
```

CMake will:
1. Fetch JUCE 8.0.4 via FetchContent
2. Build the Svelte UI (`npm install && npm run build` in `WebUI/`)
3. Embed the UI into the plugin binary via `juce_add_binary_data`
4. Compile DSP + plugin code
5. Copy plugin to system plugin folder (`COPY_PLUGIN_AFTER_BUILD`)

## Development

```bash
# Run the UI standalone for development
cd WebUI
npm install
npm run dev
```

The UI runs in the browser at `localhost:5173` with default parameter values. JUCE relay functions are stubbed when running outside the plugin.

## Project Structure

```
amp-bass/
├── CMakeLists.txt
├── Source/
│   ├── DSP/                    # Signal chain stages (header-only)
│   │   ├── InputGain.h
│   │   ├── PreFilters.h
│   │   ├── TubePreamp.h
│   │   ├── ToneStack.h
│   │   ├── GraphicEQ.h
│   │   ├── MasterVolume.h
│   │   ├── PowerSection.h
│   │   └── Primitives/         # Shared DSP building blocks
│   ├── Parameters/
│   │   ├── ParameterIDs.h      # Frozen parameter identifiers
│   │   └── ParameterLayout.h   # APVTS parameter definitions
│   └── Plugin/
│       ├── PluginProcessor.*   # Audio processing + stage wiring
│       ├── PluginEditor.*      # WebView host + resource provider
│       └── NativeBridge.*      # JUCE 8 relay parameter sync
└── WebUI/
    ├── src/
    │   ├── App.svelte          # Main UI layout
    │   ├── bridge/bridge.ts    # JUCE 8 relay frontend bridge
    │   ├── state/store.ts      # Svelte stores backed by relays
    │   ├── components/         # Neumorphic UI components
    │   └── types/parameters.ts # TypeScript parameter ID types
    ├── vite.config.ts
    └── package.json
```

## Parameters

24 parameters across 7 DSP stages. All parameter IDs are frozen — changing them breaks saved sessions.

| Group | Parameters |
|---|---|
| Input | -15dB Pad, HI/LO |
| Pre-filters | Bright, Ultra HI, Ultra LO |
| Preamp | Gain, Tube Gain |
| Tone | Bass, Midrange, Mid Freq (5-pos), Treble |
| Graphic EQ | 9 bands (33Hz-8kHz), Level, On/Off |
| Output | Master |
| Meta | Preset Select (Phase 2) |

## License

All rights reserved.
