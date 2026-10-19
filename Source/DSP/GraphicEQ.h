#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include "Primitives/Biquad.h"
#include <cmath>

/// Stage 5: 9-band graphic EQ fitted to the reference plugin. Like an analog proportional-Q EQ, a band
/// gets narrower as it moves away from 0, and the slider is strongly non-linear: +6 on the slider is only
/// +1.5 dB, +12 is about +9.8 dB. Switching the EQ on also adds a broad +0.46 dB hump around 500 Hz.
class GraphicEQ
{
public:
    static constexpr int kNumBands = 9;

    void prepare (double sampleRate, int /*maxBlockSize*/)
    {
        sr = sampleRate;
        crossfadeSamples = static_cast<int> (sampleRate * 64.0 / 48000.0); // ~64 samples at 48k
        if (crossfadeSamples < 1) crossfadeSamples = 1;
        for (int ch = 0; ch < 2; ++ch)
        {
            onHump[ch].prepare (sampleRate);
            onHump[ch].setParams (Biquad::Type::Peak, kOnHumpHz, kOnHumpQ, kOnHumpDb);
        }

        for (int b = 0; b < kNumBands; ++b)
        {
            for (int ch = 0; ch < 2; ++ch)
            {
                bands[b][ch].prepare (sampleRate);
                bands[b][ch].setParams (Biquad::Type::Peak, kBandHz[b], kBoostQ[0], 0.0f);
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
        for (int ch = 0; ch < 2; ++ch)
            onHump[ch].reset();
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
                {
                    float gainDb, q;
                    bandResponse (b, bandDb[b], gainDb, q);
                    for (int ch = 0; ch < numChannels; ++ch)
                        bands[b][ch].setParams (Biquad::Type::Peak, kBandHz[b], q, gainDb);
                }
            }

            const float levelGain  = smoothLevel.getNextValue();
            const float bypassMix  = smoothBypass.getNextValue();

            for (int ch = 0; ch < numChannels; ++ch)
            {
                float* data = buffer.getWritePointer (ch);
                const float dry = data[s];

                // Process through all 9 bands
                float wet = onHump[ch].processSample (dry);
                for (int b = 0; b < kNumBands; ++b)
                    wet = bands[b][ch].processSample (wet);

                wet *= levelGain;

                // Crossfade between dry and wet based on bypass state
                data[s] = dry + bypassMix * (wet - dry);
            }
        }
    }

private:
    // Measured band centres (labels: 33, 80, 150, 300, 600, 900, 2k, 5k, 9k)
    static constexpr float kBandHz[kNumBands] = { 33.1f, 78.6f, 141.4f, 305.9f, 537.4f, 859.7f, 2240.0f, 5046.0f, 7690.0f };
    // Peak gain (dB) and Q of each band with its slider at +12 / -12
    static constexpr float kBand12BoostDb[kNumBands] = { 14.89f, 9.76f, 9.78f, 9.76f, 9.80f, 9.80f, 9.77f, 9.74f, 11.88f };
    static constexpr float kBand12BoostQ[kNumBands]  = { 0.918f, 1.109f, 1.363f, 1.336f, 0.765f, 0.681f, 1.252f, 1.645f, 1.860f };
    static constexpr float kBand12CutDb[kNumBands]   = { 14.34f, 9.43f, 9.43f, 9.40f, 9.49f, 9.50f, 9.43f, 9.36f, 11.33f };
    static constexpr float kBand12CutQ[kNumBands]    = { 0.927f, 1.106f, 1.354f, 1.328f, 0.763f, 0.679f, 1.247f, 1.639f, 1.861f };

    // Slider law measured on the 300 Hz band, applied to every band scaled to its own +-12 values
    static constexpr int kLawPoints = 7;
    static constexpr float kLawSlider[kLawPoints] = { 0.0f, 3.0f, 6.0f, 9.0f, 10.5f, 11.25f, 12.0f };
    static constexpr float kBoostDb[kLawPoints]   = { 0.0f, 0.64f, 1.50f, 3.15f, 4.96f, 6.63f, 9.76f };
    static constexpr float kBoostQ[kLawPoints]    = { 0.336f, 0.338f, 0.397f, 0.555f, 0.753f, 0.947f, 1.336f };
    static constexpr float kCutDb[kLawPoints]     = { 0.0f, 0.63f, 1.46f, 3.04f, 4.77f, 6.38f, 9.40f };
    static constexpr float kCutQ[kLawPoints]      = { 0.334f, 0.334f, 0.388f, 0.541f, 0.737f, 0.932f, 1.328f };

    // Broad hump added whenever the EQ is on
    static constexpr float kOnHumpHz = 496.5f, kOnHumpQ = 0.1f, kOnHumpDb = 0.463f;

    /// Effective peak gain and Q of band b for a slider value in dB (-12 … +12).
    static void bandResponse (int b, float sliderDb, float& gainDb, float& q)
    {
        const bool boost = sliderDb >= 0.0f;
        const float x = juce::jlimit (0.0f, 12.0f, std::abs (sliderDb));
        int i = 0;
        while (i < kLawPoints - 2 && x > kLawSlider[i + 1])
            ++i;
        const float t = (x - kLawSlider[i]) / (kLawSlider[i + 1] - kLawSlider[i]);
        const float* lawDb = boost ? kBoostDb : kCutDb;
        const float* lawQ  = boost ? kBoostQ : kCutQ;
        const float db = lawDb[i] + t * (lawDb[i + 1] - lawDb[i]);
        const float qq = lawQ[i]  + t * (lawQ[i + 1]  - lawQ[i]);
        const float bandDb = boost ? kBand12BoostDb[b] : kBand12CutDb[b];
        const float bandQ  = boost ? kBand12BoostQ[b]  : kBand12CutQ[b];
        gainDb = (boost ? 1.0f : -1.0f) * db * bandDb / lawDb[kLawPoints - 1];
        q = qq * bandQ / lawQ[kLawPoints - 1];
    }

    double sr = 44100.0;
    int crossfadeSamples = 64;
    Biquad bands[kNumBands][2]; // [band][channel]
    Biquad onHump[2];

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothBandDb[kNumBands];
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothLevel;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothBypass;
};
