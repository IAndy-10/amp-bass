#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include "Primitives/Biquad.h"
#include <cmath>

class GraphicEQ
{
public:
    static constexpr int kNumBands = 9;

    void prepare (double sampleRate, int /*maxBlockSize*/)
    {
        sr = sampleRate;
        crossfadeSamples = static_cast<int> (sampleRate * 64.0 / 48000.0); // ~64 samples at 48k
        if (crossfadeSamples < 1) crossfadeSamples = 1;
        static constexpr float freqs[kNumBands] = {
            33.0f, 80.0f, 150.0f, 300.0f, 600.0f, 900.0f, 2000.0f, 5000.0f, 8000.0f
        };

        for (int b = 0; b < kNumBands; ++b)
        {
            for (int ch = 0; ch < 2; ++ch)
            {
                bands[b][ch].prepare (sampleRate);
                bands[b][ch].setParams (Biquad::Type::Peak, freqs[b], 2.0f, 0.0f);
            }

            smoothBandDb[b].reset (sampleRate, 0.02); // 20ms
            smoothBandDb[b].setCurrentAndTargetValue (0.0f);
        }

        smoothLevel.reset (sampleRate, 0.01); // 10ms
        smoothLevel.setCurrentAndTargetValue (1.0f);

        smoothBypass.reset (sampleRate, static_cast<double> (crossfadeSamples) / sampleRate);
        smoothBypass.setCurrentAndTargetValue (0.0f); // 0 = bypassed, 1 = active
    }

    void reset()
    {
        for (int b = 0; b < kNumBands; ++b)
            for (int ch = 0; ch < 2; ++ch)
                bands[b][ch].reset();
    }

    void setParams (bool eqOn, const float bandDbValues[kNumBands], float levelDb)
    {
        smoothBypass.setTargetValue (eqOn ? 1.0f : 0.0f);

        for (int b = 0; b < kNumBands; ++b)
            smoothBandDb[b].setTargetValue (bandDbValues[b]);

        smoothLevel.setTargetValue (std::pow (10.0f, levelDb / 20.0f));
    }

    void process (juce::AudioBuffer<float>& buffer)
    {
        const int numChannels = buffer.getNumChannels();
        const int numSamples  = buffer.getNumSamples();

        static constexpr int kCoeffUpdateInterval = 32;

        for (int s = 0; s < numSamples; ++s)
        {
            // Advance smoothed values every sample, update coefficients every 32
            float bandDb[kNumBands];
            for (int b = 0; b < kNumBands; ++b)
                bandDb[b] = smoothBandDb[b].getNextValue();

            if (s % kCoeffUpdateInterval == 0)
            {
                for (int b = 0; b < kNumBands; ++b)
                    for (int ch = 0; ch < numChannels; ++ch)
                        bands[b][ch].setGainDb (bandDb[b]);
            }

            const float levelGain  = smoothLevel.getNextValue();
            const float bypassMix  = smoothBypass.getNextValue();

            for (int ch = 0; ch < numChannels; ++ch)
            {
                float* data = buffer.getWritePointer (ch);
                const float dry = data[s];

                // Process through all 9 bands
                float wet = dry;
                for (int b = 0; b < kNumBands; ++b)
                    wet = bands[b][ch].processSample (wet);

                wet *= levelGain;

                // Crossfade between dry and wet based on bypass state
                data[s] = dry + bypassMix * (wet - dry);
            }
        }
    }

private:
    double sr = 44100.0;
    int crossfadeSamples = 64;
    Biquad bands[kNumBands][2]; // [band][channel]

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothBandDb[kNumBands];
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothLevel;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothBypass;
};
