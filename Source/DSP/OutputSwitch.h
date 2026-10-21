#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

/// Final stage: plugin bypass (crossfade to the untouched input) and mute
/// (fade to silence). Both ramp linearly over 50 ms, so switching never clicks.
/// Fully bypassed, the output is the input bit for bit. Mute wins over bypass: a muted plugin is silent either way.
class OutputSwitch
{
public:
    void prepare (double sampleRate, int /*maxBlockSize*/)
    {
        smoothBypass.reset (sampleRate, kRampSeconds);
        smoothMuteGain.reset (sampleRate, kRampSeconds);
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

        if (isFullyBypassed() && ! smoothMuteGain.isSmoothing() && muteGainTarget > 0.5f)
        {
            for (int ch = 0; ch < numChannels; ++ch)   // exact input; the crossfade formula rounds
                buffer.copyFrom (ch, 0, dry, ch, 0, numSamples);
            return;
        }

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
    static constexpr double kRampSeconds = 0.05;   // linear, 10-90 % in 40 ms

    float bypassTarget   = 0.0f;
    float muteGainTarget = 1.0f;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothBypass;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothMuteGain;
};
