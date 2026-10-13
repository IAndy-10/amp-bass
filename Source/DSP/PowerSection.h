#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>

class PowerSection
{
public:
    void prepare (double sampleRate, int /*maxBlockSize*/)
    {
        // Envelope follower coefficients
        attackCoeff  = std::exp (-1.0f / static_cast<float> (sampleRate * 0.001));  // 1ms
        releaseCoeff = std::exp (-1.0f / static_cast<float> (sampleRate * 0.050));  // 50ms

        for (int ch = 0; ch < 2; ++ch)
            envelope[ch] = 0.0f;
    }

    void reset()
    {
        for (int ch = 0; ch < 2; ++ch)
            envelope[ch] = 0.0f;
    }

    void process (juce::AudioBuffer<float>& buffer)
    {
        const int numChannels = buffer.getNumChannels();
        const int numSamples  = buffer.getNumSamples();

        for (int s = 0; s < numSamples; ++s)
        {
            for (int ch = 0; ch < numChannels; ++ch)
            {
                float x = buffer.getWritePointer (ch)[s];

                // MOSFET symmetric soft clipper
                float absX = std::abs (x);
                float y;

                if (absX < kThreshold)
                {
                    y = x;
                }
                else
                {
                    float sign  = (x >= 0.0f) ? 1.0f : -1.0f;
                    float over  = (absX - kThreshold) / kOneMinusThreshold;
                    y = sign * (kThreshold + kOneMinusThreshold * tanhApprox (over));
                }

                // Envelope follower
                float absY = std::abs (y);
                float coeff = (absY > envelope[ch]) ? attackCoeff : releaseCoeff;
                envelope[ch] = coeff * envelope[ch] + (1.0f - coeff) * absY;

                // Gentle 2:1 compression above threshold
                if (envelope[ch] > kThreshold)
                {
                    float gainReduction = kThreshold / std::sqrt (envelope[ch] / kThreshold);
                    // Avoid division by zero: envelope is always > threshold here
                    y *= gainReduction / envelope[ch];
                }

                buffer.getWritePointer (ch)[s] = y;
            }
        }
    }

private:
    static constexpr float kThreshold        = 0.8f;
    static constexpr float kOneMinusThreshold = 0.2f;

    float attackCoeff  = 0.0f;
    float releaseCoeff = 0.0f;
    float envelope[2]  = {};

    static float tanhApprox (float x)
    {
        if (x > 5.0f)  return 1.0f;
        if (x < -5.0f) return -1.0f;
        const float x2 = x * x;
        return x * (27.0f + x2) / (27.0f + 9.0f * x2);
    }
};
