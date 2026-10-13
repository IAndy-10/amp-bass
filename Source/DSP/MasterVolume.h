#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

class MasterVolume
{
public:
    void prepare (double sampleRate, int /*maxBlockSize*/)
    {
        smoothedGain.reset (sampleRate, 0.01); // 10ms ramp
        smoothedGain.setCurrentAndTargetValue (0.25f); // default 0.5^2
    }

    void reset()
    {
        smoothedGain.setCurrentAndTargetValue (smoothedGain.getTargetValue());
    }

    void setParams (float masterKnob)
    {
        // Quadratic taper: knob^2
        smoothedGain.setTargetValue (masterKnob * masterKnob);
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
