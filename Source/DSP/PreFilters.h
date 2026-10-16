#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include "Primitives/Biquad.h"
#include <cmath>

class PreFilters
{
public:
    void prepare (double sampleRate, int /*maxBlockSize*/)
    {
        sr = sampleRate;

        // Bright: high-shelf at 2kHz
        for (auto& f : brightFilter)
        {
            f.prepare (sampleRate);
            f.setParams (Biquad::Type::HighShelf, 2000.0f, 0.707f, 0.0f);
        }

        // Ultra HI: high-shelf at 5kHz
        for (auto& f : ultraHiFilter)
        {
            f.prepare (sampleRate);
            f.setParams (Biquad::Type::HighShelf, 5000.0f, 0.707f, 0.0f);
        }

        // Ultra LO (manual): +2 dB at 40 Hz, -10 dB at 500 Hz
        for (int ch = 0; ch < 2; ++ch)
        {
            ultraLoLowShelf[ch].prepare (sampleRate);
            ultraLoLowShelf[ch].setParams (Biquad::Type::LowShelf, kUltraLoShelfHz, 0.707f, 0.0f);

            ultraLoMidCut[ch].prepare (sampleRate);
            ultraLoMidCut[ch].setParams (Biquad::Type::Peak, 500.0f, 1.5f, 0.0f);
        }

        smoothBrightGain.reset (sampleRate, 0.02);
        smoothUltraHiGain.reset (sampleRate, 0.02);
        smoothUltraLoGain.reset (sampleRate, 0.02);

        smoothBrightGain.setCurrentAndTargetValue (0.0f);
        smoothUltraHiGain.setCurrentAndTargetValue (0.0f);
        smoothUltraLoGain.setCurrentAndTargetValue (0.0f);
    }

    void reset()
    {
        for (auto& f : brightFilter)    f.reset();
        for (auto& f : ultraHiFilter)   f.reset();
        for (int ch = 0; ch < 2; ++ch)
        {
            ultraLoLowShelf[ch].reset();
            ultraLoMidCut[ch].reset();
        }
    }

    void setParams (bool bright, bool ultraHi, bool ultraLo)
    {
        smoothBrightGain.setTargetValue (bright  ? 6.0f : 0.0f);
        smoothUltraHiGain.setTargetValue (ultraHi ? 6.0f : 0.0f);
        smoothUltraLoGain.setTargetValue (ultraLo ? 1.0f : 0.0f);
    }

    void process (juce::AudioBuffer<float>& buffer)
    {
        const int numChannels = buffer.getNumChannels();
        const int numSamples  = buffer.getNumSamples();

        static constexpr int kCoeffUpdateInterval = 32;

        for (int s = 0; s < numSamples; ++s)
        {
            const float brightDb   = smoothBrightGain.getNextValue();
            const float ultraHiDb  = smoothUltraHiGain.getNextValue();
            const float ultraLoAmt = smoothUltraLoGain.getNextValue();

            // Update filter coefficients at block-rate (every 32 samples)
            if (s % kCoeffUpdateInterval == 0)
            {
                for (auto& f : brightFilter)
                    f.setGainDb (brightDb);

                for (auto& f : ultraHiFilter)
                    f.setGainDb (ultraHiDb);

                for (int ch = 0; ch < 2; ++ch)
                {
                    ultraLoLowShelf[ch].setGainDb (kUltraLoShelfDb * ultraLoAmt);
                    ultraLoMidCut[ch].setGainDb (-10.0f * ultraLoAmt);
                }
            }

            for (int ch = 0; ch < numChannels; ++ch)
            {
                float sample = buffer.getWritePointer (ch)[s];
                sample = brightFilter[ch].processSample (sample);
                sample = ultraHiFilter[ch].processSample (sample);
                sample = ultraLoLowShelf[ch].processSample (sample);
                sample = ultraLoMidCut[ch].processSample (sample);
                buffer.getWritePointer (ch)[s] = sample;
            }
        }
    }

private:
    double sr = 44100.0;

    // Shelf corner above 40 Hz so the boost measured at 40 Hz is ~2 dB
    static constexpr float kUltraLoShelfHz = 80.0f;
    static constexpr float kUltraLoShelfDb = 2.15f;

    Biquad brightFilter[2];
    Biquad ultraHiFilter[2];
    Biquad ultraLoLowShelf[2];
    Biquad ultraLoMidCut[2];

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothBrightGain;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothUltraHiGain;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothUltraLoGain;
};
