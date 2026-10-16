#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

/// Final stage: plugin bypass (crossfade to the untouched input) and mute
/// (fade to silence). Both ramp over 10 ms so switching never clicks.
/// Mute wins over bypass: a muted plugin is silent either way.
class OutputSwitch
{
public:
    void prepare (double sampleRate, int /*maxBlockSize*/)
    {
        smoothBypass.reset (sampleRate, 0.01);
        smoothMuteGain.reset (sampleRate, 0.01);
        smoothBypass.setCurrentAndTargetValue (bypassTarget);
        smoothMuteGain.setCurrentAndTargetValue (muteGainTarget);
    }

    void reset()
    {
        smoothBypass.setCurrentAndTargetValue (smoothBypass.getTargetValue());
        smoothMuteGain.setCurrentAndTargetValue (smoothMuteGain.getTargetValue());
    }

    void setParams (bool bypass, bool mute)
    {
        bypassTarget   = bypass ? 1.0f : 0.0f;
        muteGainTarget = mute ? 0.0f : 1.0f;
        smoothBypass.setTargetValue (bypassTarget);
        smoothMuteGain.setTargetValue (muteGainTarget);
    }

    /// True when the processed signal is fully replaced by the dry input.
    bool isFullyBypassed() const { return ! smoothBypass.isSmoothing() && bypassTarget > 0.5f; }

    /// `dry` must hold the untouched input for the same samples as `buffer`.
    void process (juce::AudioBuffer<float>& buffer, const juce::AudioBuffer<float>& dry)
    {
        const int numChannels = juce::jmin (buffer.getNumChannels(), dry.getNumChannels());
        const int numSamples  = juce::jmin (buffer.getNumSamples(), dry.getNumSamples());

        if (! smoothBypass.isSmoothing() && ! smoothMuteGain.isSmoothing()
            && bypassTarget < 0.5f && muteGainTarget > 0.5f)
            return; // active and unmuted: nothing to do

        for (int s = 0; s < numSamples; ++s)
        {
            const float b = smoothBypass.getNextValue();
            const float g = smoothMuteGain.getNextValue();

            for (int ch = 0; ch < numChannels; ++ch)
            {
                float* out = buffer.getWritePointer (ch);
                const float d = dry.getReadPointer (ch)[s];
                out[s] = (out[s] + b * (d - out[s])) * g;
            }
        }
    }

private:
    float bypassTarget   = 0.0f;
    float muteGainTarget = 1.0f;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothBypass;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothMuteGain;
};
