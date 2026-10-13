#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>

class TubePreamp
{
public:
    void prepare (double sampleRate, int /*maxBlockSize*/)
    {
        sr = sampleRate;

        // Pre-emphasis HPF at 80Hz: a = 1 / (1 + 2*pi*80/sr)
        preEmphCoeff = static_cast<float> (1.0 / (1.0 + 2.0 * 3.14159265358979 * 80.0 / sampleRate));

        // DC blocker HPF at 10Hz
        dcBlockCoeff = static_cast<float> (1.0 / (1.0 + 2.0 * 3.14159265358979 * 10.0 / sampleRate));

        smoothDrive.reset (sampleRate, 0.01);    // 10ms
        smoothHeadroom.reset (sampleRate, 0.05);  // 50ms

        smoothDrive.setCurrentAndTargetValue (1.0f);
        smoothHeadroom.setCurrentAndTargetValue (1.0f);

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
    }

    void process (juce::AudioBuffer<float>& buffer)
    {
        const int numChannels = buffer.getNumChannels();
        const int numSamples  = buffer.getNumSamples();

        for (int s = 0; s < numSamples; ++s)
        {
            const float drive    = smoothDrive.getNextValue();
            const float headroom = smoothHeadroom.getNextValue();
            const float normGain = headroom / std::max (1.0f, drive * 0.3f);

            for (int ch = 0; ch < numChannels; ++ch)
            {
                float x = buffer.getWritePointer (ch)[s];

                // Pre-emphasis HPF at 80Hz
                float yPre = preEmphCoeff * (yPrevPre[ch] + x - xPrevPre[ch]);
                xPrevPre[ch] = x;
                yPrevPre[ch] = yPre;
                x = yPre;

                // DC bias (more asymmetry when starved)
                float xBiased = x + 0.1f * (1.0f - headroom);

                // Drive scaling
                float xDriven = xBiased * drive / headroom;

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

private:
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
