#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "ParameterIDs.h"

// Display and text entry follow the reference plugin (Ampeg SVT-3 PRO), so the same normalised
// value means the same setting in both: knobs read 0.0-10.0, dB values one decimal.
namespace ParameterText
{
    /// Knob attributes: raw 0-1 shown and typed as 0.0-10.0.
    inline juce::AudioParameterFloatAttributes knob()
    {
        return juce::AudioParameterFloatAttributes()
            .withStringFromValueFunction ([] (float v, int) { return juce::String (v * 10.0f, 1); })
            .withValueFromStringFunction ([] (const juce::String& t) { return juce::jlimit (0.0f, 1.0f, t.getFloatValue() / 10.0f); });
    }

    /// dB attributes: one decimal, typed as a plain number.
    inline juce::AudioParameterFloatAttributes decibels()
    {
        return juce::AudioParameterFloatAttributes()
            .withLabel ("dB")
            .withStringFromValueFunction ([] (float v, int) { return juce::String (v, 1); })
            .withValueFromStringFunction ([] (const juce::String& t) { return t.getFloatValue(); });
    }

    /// GEQ Level like the reference: -10 dB at 0, 0 dB at the centre, +8 dB at 1 (linear on each side).
    inline juce::NormalisableRange<float> geqLevelRange()
    {
        return { -10.0f, 8.0f,
                 [] (float, float, float x) { return x < 0.5f ? -10.0f + 20.0f * x : 16.0f * (x - 0.5f); },
                 [] (float, float, float v) { return v < 0.0f ? (v + 10.0f) / 20.0f : 0.5f + v / 16.0f; } };
    }
}

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
        0.34f, ParameterText::knob()));  // reference default 3.4

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::tubeGain, 1 },
        "Tube Gain",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.0f, 1.0f),
        0.5f, ParameterText::knob()));

    // --- Stage 4: ToneStack ---

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::bass, 1 },
        "Bass",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.0f, 1.0f),
        0.5f, ParameterText::knob()));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::midrange, 1 },
        "Midrange",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.0f, 1.0f),
        0.5f, ParameterText::knob()));

    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { ParameterIDs::midFreq, 1 },
        "Mid Frequency",
        1, 5, 1));  // reference default 1

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::treble, 1 },
        "Treble",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.0f, 1.0f),
        0.5f, ParameterText::knob()));

    // --- Stage 5: GraphicEQ ---

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ParameterIDs::geqOn, 1 },
        "Graphic EQ On", false));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::geq33, 1 },
        "GEQ 33 Hz",
        juce::NormalisableRange<float> (-12.0f, 12.0f, 0.0f, 1.0f),
        0.0f, ParameterText::decibels()));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::geq80, 1 },
        "GEQ 80 Hz",
        juce::NormalisableRange<float> (-12.0f, 12.0f, 0.0f, 1.0f),
        0.0f, ParameterText::decibels()));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::geq150, 1 },
        "GEQ 150 Hz",
        juce::NormalisableRange<float> (-12.0f, 12.0f, 0.0f, 1.0f),
        0.0f, ParameterText::decibels()));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::geq300, 1 },
        "GEQ 300 Hz",
        juce::NormalisableRange<float> (-12.0f, 12.0f, 0.0f, 1.0f),
        0.0f, ParameterText::decibels()));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::geq600, 1 },
        "GEQ 600 Hz",
        juce::NormalisableRange<float> (-12.0f, 12.0f, 0.0f, 1.0f),
        0.0f, ParameterText::decibels()));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::geq900, 1 },
        "GEQ 900 Hz",
        juce::NormalisableRange<float> (-12.0f, 12.0f, 0.0f, 1.0f),
        0.0f, ParameterText::decibels()));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::geq2k, 1 },
        "GEQ 2 kHz",
        juce::NormalisableRange<float> (-12.0f, 12.0f, 0.0f, 1.0f),
        0.0f, ParameterText::decibels()));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::geq5k, 1 },
        "GEQ 5 kHz",
        juce::NormalisableRange<float> (-12.0f, 12.0f, 0.0f, 1.0f),
        0.0f, ParameterText::decibels()));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::geq8k, 1 },
        "GEQ 8 kHz",
        juce::NormalisableRange<float> (-12.0f, 12.0f, 0.0f, 1.0f),
        0.0f, ParameterText::decibels()));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::geqLevel, 1 },
        "GEQ Level",
        ParameterText::geqLevelRange(),
        0.0f, ParameterText::decibels()));

    // --- Stage 6: MasterVolume ---

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::master, 1 },
        "Master Volume",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.0f, 1.0f),
        0.5f, ParameterText::knob()));

    // --- Global dry/wet mix ---

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::dryWet, 1 },
        "Dry/Wet",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.0f, 1.0f),
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

    // --- Stage 8: Cabinet (on by default, like the reference's default Rec Chain and Horn) ---
    // Added last so the earlier parameters keep their indices.

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ParameterIDs::cabinet, 1 },
        "Cabinet", true));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ParameterIDs::horn, 1 },
        "Horn", true));

    return { params.begin(), params.end() };
}
