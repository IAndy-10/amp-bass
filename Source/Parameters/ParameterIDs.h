#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace ParameterIDs
{
    // Stage 1 — InputGain
    static constexpr const char* inputPad      = "inputPad";

    // Stage 2 — PreFilters
    static constexpr const char* bright        = "bright";
    static constexpr const char* ultraHi       = "ultraHi";
    static constexpr const char* ultraLo       = "ultraLo";

    // Stage 3 — TubePreamp
    static constexpr const char* gain          = "gain";
    static constexpr const char* tubeGain      = "tubeGain";

    // Stage 4 — ToneStack
    static constexpr const char* bass          = "bass";
    static constexpr const char* midrange      = "midrange";
    static constexpr const char* midFreq       = "midFreq";
    static constexpr const char* treble        = "treble";

    // Stage 5 — GraphicEQ
    static constexpr const char* geqOn         = "geqOn";
    static constexpr const char* geq33         = "geq33";
    static constexpr const char* geq80         = "geq80";
    static constexpr const char* geq150        = "geq150";
    static constexpr const char* geq300        = "geq300";
    static constexpr const char* geq600        = "geq600";
    static constexpr const char* geq900        = "geq900";
    static constexpr const char* geq2k         = "geq2k";
    static constexpr const char* geq5k         = "geq5k";
    static constexpr const char* geq8k         = "geq8k";
    static constexpr const char* geqLevel      = "geqLevel";

    // Stage 6 — MasterVolume
    static constexpr const char* master        = "master";

    // Global dry/wet mix
    static constexpr const char* dryWet        = "dryWet";

    // Preset selection (no function yet)
    static constexpr const char* presetSelect  = "presetSelect";

    // Output switches (faceplate POWER / toolbar power icon = bypass)
    static constexpr const char* bypass        = "bypass";
    static constexpr const char* mute          = "mute";

    // Stage 8 — Cabinet (after the power stage, like the reference's Rec Chain)
    static constexpr const char* cabinet       = "cabinet";
    static constexpr const char* horn          = "horn";

    // FX Rack — Tight (Off / Pre / Post the amp) and Power Soak (output attenuation)
    static constexpr const char* tight         = "tight";
    static constexpr const char* tightFreq     = "tightFreq";
    static constexpr const char* powerSoak     = "powerSoak";

    // Stage 1 — input level knob like the reference's Input (-20 ... +20 dB), appended to keep the order
    static constexpr const char* inputGain     = "inputGain";

    // FX Rack — Smooth (Off / Pre / Post the amp: a gentle low-pass, 3-35 kHz)
    static constexpr const char* smooth        = "smooth";
    static constexpr const char* smoothFreq    = "smoothFreq";

    // FX Rack — Noise Gate (listens to the input, turns the amp's output down below the threshold)
    static constexpr const char* gate          = "gate";
    static constexpr const char* gateThreshold = "gateThreshold";
    static constexpr const char* gateRange     = "gateRange";
}
