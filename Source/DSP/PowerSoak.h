#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

/// FX Rack Power Soak: output attenuation after the power stage and cabinet, -40 ... 0 dB. In the reference it is a
/// pure gain: renders at different settings null to -149 dB after level matching (sound-matching plan 0.4), so it
/// lowers the level without changing the drive. Changes glide over 50 ms.
class PowerSoak
{
public:
    void prepare (double sampleRate, int /*maxBlockSize*/)
    {
        gain.reset (sampleRate, kRampSeconds);
        gain.setCurrentAndTargetValue (targetGain);
    }

    void reset() {}

    void setParams (float soakDb)
    {
        targetGain = juce::Decibels::decibelsToGain (juce::jlimit (-40.0f, 0.0f, soakDb), -100.0f);
        gain.setTargetValue (targetGain);
    }

    void process (juce::AudioBuffer<float>& buffer)
    {
        const int numChannels = buffer.getNumChannels();
        const int numSamples  = buffer.getNumSamples();

        if (! gain.isSmoothing())
        {
            if (targetGain < 1.0f)
                buffer.applyGain (targetGain);
            return;
        }

        for (int s = 0; s < numSamples; ++s)
        {
            const float g = gain.getNextValue();
            for (int ch = 0; ch < numChannels; ++ch)
                buffer.getWritePointer (ch)[s] *= g;
        }
    }

private:
    static constexpr double kRampSeconds = 0.05;

    float targetGain = 1.0f;
    juce::LinearSmoothedValue<float> gain;
};
