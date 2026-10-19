#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <algorithm>
#include <cmath>

class TubePreamp
{
public:
    void prepare (double sampleRate, int /*maxBlockSize*/)
    {
        sr = sampleRate;

        // Subsonic HPF at 25Hz before the asymmetric shaper: a = 1 / (1 + 2*pi*25/sr)
        // (low enough to keep the low-E fundamental at 41Hz within ~1.3dB)
        preEmphCoeff = static_cast<float> (1.0 / (1.0 + 2.0 * 3.14159265358979 * 25.0 / sampleRate));

        // DC blocker HPF at 10Hz
        dcBlockCoeff = static_cast<float> (1.0 / (1.0 + 2.0 * 3.14159265358979 * 10.0 / sampleRate));

        smoothDrive.reset (sampleRate, 0.01);    // 10ms
        smoothHeadroom.reset (sampleRate, 0.05);  // 50ms
        smoothLevel.reset (sampleRate, 0.01);    // 10ms

        smoothDrive.setCurrentAndTargetValue (1.0f);
        smoothHeadroom.setCurrentAndTargetValue (1.0f);
        smoothLevel.setCurrentAndTargetValue (0.0f);

        for (int ch = 0; ch < 2; ++ch)
        {
            xPrevPre[ch]  = 0.0f;
            yPrevPre[ch]  = 0.0f;
            xPrevDc[ch]   = 0.0f;
            yPrevDc[ch]   = 0.0f;
        }
    }

    void reset()
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            xPrevPre[ch]  = 0.0f;
            yPrevPre[ch]  = 0.0f;
            xPrevDc[ch]   = 0.0f;
            yPrevDc[ch]   = 0.0f;
        }
    }

    void setParams (float gainKnob, float tubeGainKnob)
    {
        // gain_knob 0-1 -> drive 1-10
        smoothDrive.setTargetValue (1.0f + gainKnob * 9.0f);

        // tube_gain_knob 0-1 -> headroom 0.3-1.0
        smoothHeadroom.setTargetValue (0.3f + tubeGainKnob * 0.7f);

        // Small-signal level follows GAIN linearly like the reference (+6 dB per doubling, silent at 0)
        smoothLevel.setTargetValue (kLevelAtFullGain * gainKnob);
    }

    void process (juce::AudioBuffer<float>& buffer)
    {
        const int numChannels = buffer.getNumChannels();
        const int numSamples  = buffer.getNumSamples();

        for (int s = 0; s < numSamples; ++s)
        {
            const float drive    = smoothDrive.getNextValue();
            const float headroom = smoothHeadroom.getNextValue();
            const float level    = smoothLevel.getNextValue();
            // Output scaled so the small-signal gain is `level`, whatever the drive: GAIN sets
            // loudness like the reference's linear pot, and the drive sets how hard the shaper works.
            const float normGain = level * headroom / drive;

            for (int ch = 0; ch < numChannels; ++ch)
            {
                float x = buffer.getWritePointer (ch)[s];

                // Subsonic HPF at 25Hz
                float yPre = preEmphCoeff * (yPrevPre[ch] + x - xPrevPre[ch]);
                xPrevPre[ch] = x;
                yPrevPre[ch] = yPre;
                x = yPre;

                // DC bias (more asymmetry when starved)
                float xBiased = x + 0.1f * (1.0f - headroom);

                // Drive scaling
                float xDriven = xBiased * drive / headroom;
                peakDrive = std::max (peakDrive, std::abs (xDriven));

                // Asymmetric waveshaper with Pade tanh approximation
                float y;
                float t = tanhApprox (xDriven);

                if (xDriven >= 0.0f)
                {
                    y = t;
                }
                else
                {
                    float absXd = -xDriven;
                    y = t * (1.0f - 0.25f * absXd / (1.0f + absXd));
                }

                // Normalize output
                y *= normGain;

                // DC blocker HPF at 10Hz
                float yDc = dcBlockCoeff * (yPrevDc[ch] + y - xPrevDc[ch]);
                xPrevDc[ch] = y;
                yPrevDc[ch] = yDc;

                buffer.getWritePointer (ch)[s] = yDc;
            }
        }
    }

    /// Largest drive level reached since the last call (1 = clipping onset).
    float takePeakDrive()
    {
        const float p = peakDrive;
        peakDrive = 0.0f;
        return p;
    }

private:
    // Small-signal gain at full GAIN (x1.9 at GAIN 1, where AmpBass already matched the reference)
    static constexpr float kLevelAtFullGain = 19.0f;

    float peakDrive = 0.0f;
    double sr = 44100.0;
    float preEmphCoeff = 0.0f;
    float dcBlockCoeff = 0.0f;

    // Per-channel state
    float xPrevPre[2] = {};
    float yPrevPre[2] = {};
    float xPrevDc[2]  = {};
    float yPrevDc[2]  = {};

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothDrive;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothHeadroom;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothLevel;

    // Pade approximant: tanh(x) ~ x*(27+x*x)/(27+9*x*x), <0.1% error for |x|<3
    static float tanhApprox (float x)
    {
        // Clamp for stability at extreme drive
        if (x > 5.0f)  return 1.0f;
        if (x < -5.0f) return -1.0f;
        const float x2 = x * x;
        return x * (27.0f + x2) / (27.0f + 9.0f * x2);
    }
};
