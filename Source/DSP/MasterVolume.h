#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

class MasterVolume
{
public:
    void prepare (double sampleRate, int /*maxBlockSize*/)
    {
        smoothedGain.reset (sampleRate, 0.01); // 10ms ramp
        smoothedGain.setCurrentAndTargetValue (0.5f * kGainAtFull); // default knob 0.5
    }

    void reset()
    {
        smoothedGain.setCurrentAndTargetValue (smoothedGain.getTargetValue());
    }

    void setParams (float masterKnob)
    {
        // Linear taper like the reference: level follows the knob (+6 dB per doubling), silent at 0
        smoothedGain.setTargetValue (masterKnob * kGainAtFull);
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
    // Gain at full Master. Keeps Master 5 where it was (-12 dB), which matches the reference there.
    static constexpr float kGainAtFull = 0.5f;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedGain;
};
