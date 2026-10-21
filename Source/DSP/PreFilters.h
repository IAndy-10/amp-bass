#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include "Primitives/Biquad.h"
#include <cmath>

/// Stage 2: the Bright, Ultra Hi and Ultra Lo switches. Each filter is fitted to the reference
/// plugin's response with the switch on minus off (within 0.1 dB, 30 Hz-20 kHz). Switching glides
/// over 20 ms by scaling each filter's gain in dB.
class PreFilters
{
public:
    void prepare (double sampleRate, int /*maxBlockSize*/)
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            brightPeak[ch].prepare (sampleRate);
            brightShelf[ch].prepare (sampleRate);
            ultraHiPeak[ch].prepare (sampleRate);
            ultraHiShelf[ch].prepare (sampleRate);
            ultraLoCut[ch].prepare (sampleRate);
        }
        setGains (0.0f, 0.0f, 0.0f);

        smoothBright.reset (sampleRate, 0.02);
        smoothUltraHi.reset (sampleRate, 0.02);
        smoothUltraLo.reset (sampleRate, 0.02);

        smoothBright.setCurrentAndTargetValue (0.0f);
        smoothUltraHi.setCurrentAndTargetValue (0.0f);
        smoothUltraLo.setCurrentAndTargetValue (0.0f);
    }

    void reset()
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            brightPeak[ch].reset();
            brightShelf[ch].reset();
            ultraHiPeak[ch].reset();
            ultraHiShelf[ch].reset();
            ultraLoCut[ch].reset();
        }
    }

    void setParams (bool bright, bool ultraHi, bool ultraLo)
    {
        smoothBright.setTargetValue (bright ? 1.0f : 0.0f);
        smoothUltraHi.setTargetValue (ultraHi ? 1.0f : 0.0f);
        smoothUltraLo.setTargetValue (ultraLo ? 1.0f : 0.0f);
    }

    void process (juce::AudioBuffer<float>& buffer)
    {
        const int numChannels = buffer.getNumChannels();
        const int numSamples  = buffer.getNumSamples();

        static constexpr int kCoeffUpdateInterval = 32;

        for (int s = 0; s < numSamples; ++s)
        {
            const float bright  = smoothBright.getNextValue();
            const float ultraHi = smoothUltraHi.getNextValue();
            const float ultraLo = smoothUltraLo.getNextValue();

            // Retune every 32 samples, gliding over those 32 (no zipper noise)
            if (s % kCoeffUpdateInterval == 0)
                setGains (bright, ultraHi, ultraLo, kCoeffUpdateInterval);

            for (int ch = 0; ch < numChannels; ++ch)
            {
                float sample = buffer.getWritePointer (ch)[s];
                sample = brightPeak[ch].processSample (sample);
                sample = brightShelf[ch].processSample (sample);
                sample = ultraHiPeak[ch].processSample (sample);
                sample = ultraHiShelf[ch].processSample (sample);
                sample = ultraLoCut[ch].processSample (sample);
                buffer.getWritePointer (ch)[s] = sample;
            }
        }
    }

private:
    // Bright: +5.3 dB around 3 kHz, easing to +1.2 dB at the top.
    static constexpr float kBrightPeakHz  = 2991.0f,  kBrightPeakQ  = 0.269f, kBrightPeakDb  = 5.312f;
    static constexpr float kBrightShelfHz = 10867.0f, kBrightShelfQ = 0.475f, kBrightShelfDb = 1.174f;
    // Ultra Hi: +6 dB around 6 kHz, +2.4 dB at the top.
    static constexpr float kUltraHiPeakHz  = 5602.0f, kUltraHiPeakQ  = 0.369f, kUltraHiPeakDb  = 5.323f;
    static constexpr float kUltraHiShelfHz = 9026.0f, kUltraHiShelfQ = 0.359f, kUltraHiShelfDb = 2.359f;
    // Ultra Lo: -12.4 dB at 530 Hz. The manual's +2 dB at 40 Hz isn't in the reference plugin.
    static constexpr float kUltraLoHz = 530.2f, kUltraLoQ = 0.713f, kUltraLoDb = -12.36f;

    void setGains (float bright, float ultraHi, float ultraLo, int glideSamples = 0)
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            brightPeak[ch].glideTo (Biquad::Type::Peak, kBrightPeakHz, kBrightPeakQ, kBrightPeakDb * bright, glideSamples);
            brightShelf[ch].glideTo (Biquad::Type::HighShelf, kBrightShelfHz, kBrightShelfQ, kBrightShelfDb * bright, glideSamples);
            ultraHiPeak[ch].glideTo (Biquad::Type::Peak, kUltraHiPeakHz, kUltraHiPeakQ, kUltraHiPeakDb * ultraHi, glideSamples);
            ultraHiShelf[ch].glideTo (Biquad::Type::HighShelf, kUltraHiShelfHz, kUltraHiShelfQ, kUltraHiShelfDb * ultraHi, glideSamples);
            ultraLoCut[ch].glideTo (Biquad::Type::Peak, kUltraLoHz, kUltraLoQ, kUltraLoDb * ultraLo, glideSamples);
        }
    }

    Biquad brightPeak[2], brightShelf[2];
    Biquad ultraHiPeak[2], ultraHiShelf[2];
    Biquad ultraLoCut[2];

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothBright;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothUltraHi;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothUltraLo;
};
