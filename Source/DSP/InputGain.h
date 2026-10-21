#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>

/// Stage 1: input level. The reference's Input knob (-20 ... +20 dB, a pure gain before everything: renders null to
/// -110 dB against the same input scaled) and the faceplate's -15 dB pad.
class InputGain
{
public:
    void prepare (double sampleRate, int /*maxBlockSize*/)
    {
        smoothedGain.reset (sampleRate, 0.01); // 10ms ramp
        smoothedGain.setCurrentAndTargetValue (1.0f);
    }

    void reset()
    {
        smoothedGain.setCurrentAndTargetValue (smoothedGain.getTargetValue());
    }

    void setParams (bool padOn, float inputDb = 0.0f)
    {
        const float padGain = padOn ? 0.17783f : 1.0f;  // -15dB or unity
        smoothedGain.setTargetValue (padGain * juce::Decibels::decibelsToGain (juce::jlimit (-20.0f, 20.0f, inputDb)));
    }

    void process (juce::AudioBuffer<float>& buffer)
    {
        const int numChannels = buffer.getNumChannels();
        const int numSamples  = buffer.getNumSamples();

        if (smoothedGain.isSmoothing())
        {
            for (int s = 0; s < numSamples; ++s)
            {
                const float g = smoothedGain.getNextValue();
                for (int ch = 0; ch < numChannels; ++ch)
                    buffer.getWritePointer (ch)[s] *= g;
            }
        }
        else
        {
            const float g = smoothedGain.getTargetValue();
            for (int ch = 0; ch < numChannels; ++ch)
                juce::FloatVectorOperations::multiply (buffer.getWritePointer (ch), g, numSamples);
        }
    }

private:
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedGain;
};
