#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <algorithm>
#include <cmath>

/// Stage 3: preamp. Measured on the reference plugin, the preamp is clean at every Gain setting with
/// Tube Gain at 10: its odd-harmonic distortion comes from the power stage (PowerSection). So this
/// stage is linear: a 25 Hz high-pass, the Gain level (a linear pot like the reference: small-signal
/// gain 19 x Gain, +6 dB per doubling, silent at 0). Being linear it adds no DC, so the chain's DC blocker
/// sits after the nonlinearity in PowerSection. Tube Gain lives in PowerSection, where the reference has it.
class TubePreamp
{
public:
    void prepare (double sampleRate, int /*maxBlockSize*/)
    {
        // Subsonic HPF at 25Hz: a = 1 / (1 + 2*pi*25/sr)
        // (low enough to keep the low-E fundamental at 41Hz within ~1.3dB)
        preEmphCoeff = static_cast<float> (1.0 / (1.0 + 2.0 * 3.14159265358979 * 25.0 / sampleRate));

        smoothLevel.reset (sampleRate, 0.01);    // 10ms
        smoothLevel.setCurrentAndTargetValue (0.0f);

        reset();
    }

    void reset()
    {
        for (int ch = 0; ch < 2; ++ch)
            xPrevPre[ch] = yPrevPre[ch] = 0.0f;
    }

    void setParams (float gainKnob)
    {
        smoothLevel.setTargetValue (kLevelAtFullGain * gainKnob);
    }

    void process (juce::AudioBuffer<float>& buffer)
    {
        const int numChannels = buffer.getNumChannels();
        const int numSamples  = buffer.getNumSamples();

        for (int s = 0; s < numSamples; ++s)
        {
            const float level = smoothLevel.getNextValue();

            for (int ch = 0; ch < numChannels; ++ch)
            {
                float x = buffer.getWritePointer (ch)[s];

                // Subsonic HPF at 25Hz
                const float yPre = preEmphCoeff * (yPrevPre[ch] + x - xPrevPre[ch]);
                xPrevPre[ch] = x;
                yPrevPre[ch] = yPre;

                const float y = yPre * level;
                peakOut = std::max (peakOut, std::abs (y));

                buffer.getWritePointer (ch)[s] = y;
            }
        }
    }

    /// Largest preamp output level since the last call (1 = full scale inside the amp: Gain too high).
    float takePeakDrive()
    {
        const float p = peakOut;
        peakOut = 0.0f;
        return p;
    }

private:
    // Small-signal gain at full GAIN (x1.9 at GAIN 1, where AmpBass matches the reference)
    static constexpr float kLevelAtFullGain = 19.0f;

    float peakOut = 0.0f;
    float preEmphCoeff = 0.0f;

    // Per-channel state
    float xPrevPre[2] = {};
    float yPrevPre[2] = {};

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothLevel;
};
