#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "ParameterIDs.h"

inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // --- Stage 1: InputGain ---

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ParameterIDs::inputPad, 1 },
        "Input Pad", false));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ParameterIDs::inputHiLo, 1 },
        "Input Select", true));

    // --- Stage 2: PreFilters ---

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ParameterIDs::bright, 1 },
        "Bright", false));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ParameterIDs::ultraHi, 1 },
        "Ultra Hi", false));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ParameterIDs::ultraLo, 1 },
        "Ultra Lo", false));

    // --- Stage 3: TubePreamp ---

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::gain, 1 },
        "Gain",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.0f, 1.0f),
        0.5f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::tubeGain, 1 },
        "Tube Gain",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.0f, 1.0f),
        0.5f));

    // --- Stage 4: ToneStack ---

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::bass, 1 },
        "Bass",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.0f, 1.0f),
        0.5f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::midrange, 1 },
        "Midrange",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.0f, 1.0f),
        0.5f));

    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { ParameterIDs::midFreq, 1 },
        "Mid Frequency",
        1, 5, 3));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::treble, 1 },
        "Treble",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.0f, 1.0f),
        0.5f));

    // --- Stage 5: GraphicEQ ---

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ParameterIDs::geqOn, 1 },
        "Graphic EQ On", false));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::geq33, 1 },
        "GEQ 33 Hz",
        juce::NormalisableRange<float> (-12.0f, 12.0f, 0.0f, 1.0f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::geq80, 1 },
        "GEQ 80 Hz",
        juce::NormalisableRange<float> (-12.0f, 12.0f, 0.0f, 1.0f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::geq150, 1 },
        "GEQ 150 Hz",
        juce::NormalisableRange<float> (-12.0f, 12.0f, 0.0f, 1.0f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::geq300, 1 },
        "GEQ 300 Hz",
        juce::NormalisableRange<float> (-12.0f, 12.0f, 0.0f, 1.0f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::geq600, 1 },
        "GEQ 600 Hz",
        juce::NormalisableRange<float> (-12.0f, 12.0f, 0.0f, 1.0f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::geq900, 1 },
        "GEQ 900 Hz",
        juce::NormalisableRange<float> (-12.0f, 12.0f, 0.0f, 1.0f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::geq2k, 1 },
        "GEQ 2 kHz",
        juce::NormalisableRange<float> (-12.0f, 12.0f, 0.0f, 1.0f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::geq5k, 1 },
        "GEQ 5 kHz",
        juce::NormalisableRange<float> (-12.0f, 12.0f, 0.0f, 1.0f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::geq8k, 1 },
        "GEQ 8 kHz",
        juce::NormalisableRange<float> (-12.0f, 12.0f, 0.0f, 1.0f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::geqLevel, 1 },
        "GEQ Level",
        juce::NormalisableRange<float> (-10.0f, 8.0f, 0.0f, 1.0f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    // --- Stage 6: MasterVolume ---

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::master, 1 },
        "Master Volume",
        juce::NormalisableRange<float> (-10.0f, 8.0f, 0.0f, 1.0f),
        0.5f));

    // --- Global dry/wet mix ---

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::dryWet, 1 },
        "Dry/Wet",
        juce::NormalisableRange<float> (-10.0f, 8.0f, 0.0f, 1.0f),
        1.0f));  // default 100% wet (fully processed)

    // --- Phase 2: Preset selection ---

    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { ParameterIDs::presetSelect, 1 },
        "Preset",
        1, 4, 1));

    // --- Output switches ---

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ParameterIDs::bypass, 1 },
        "Bypass", false));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ParameterIDs::mute, 1 },
        "Mute", false));

    return { params.begin(), params.end() };
}
