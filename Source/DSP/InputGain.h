#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>

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

    void setParams (bool padOn, bool hiInput)
    {
        const float padGain   = padOn   ? 0.17783f : 1.0f;  // -15dB or unity
        const float inputGain = hiInput ? 1.0f     : 0.50119f; // HI or LO (-6dB)
        smoothedGain.setTargetValue (padGain * inputGain);
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
