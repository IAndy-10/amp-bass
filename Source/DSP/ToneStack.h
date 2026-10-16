#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include "Primitives/Biquad.h"
#include <cmath>

class ToneStack
{
public:
    void prepare (double sampleRate, int /*maxBlockSize*/)
    {
        sr = sampleRate;

        for (int ch = 0; ch < 2; ++ch)
        {
            lowShelf[ch].prepare (sampleRate);
            lowShelf[ch].setParams (Biquad::Type::LowShelf, 100.0f, 0.5f, 0.0f);

            midPeak[ch].prepare (sampleRate);
            midPeak[ch].setParams (Biquad::Type::Peak, 800.0f, 1.5f, 0.0f);

            highShelf[ch].prepare (sampleRate);
            highShelf[ch].setParams (Biquad::Type::HighShelf, 2500.0f, 0.5f, 0.0f);
        }

        smoothBassDb.reset (sampleRate, 0.02);
        smoothMidDb.reset (sampleRate, 0.02);
        smoothTrebleDb.reset (sampleRate, 0.02);
        smoothMidFreq.reset (sampleRate, 0.05);

        smoothBassDb.setCurrentAndTargetValue (0.0f);
        smoothMidDb.setCurrentAndTargetValue (0.0f);
        smoothTrebleDb.setCurrentAndTargetValue (0.0f);
        smoothMidFreq.setCurrentAndTargetValue (800.0f);
    }

    void reset()
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            lowShelf[ch].reset();
            midPeak[ch].reset();
            highShelf[ch].reset();
        }
    }

    void setParams (float bassKnob, float midKnob, int midFreqSelect, float trebleKnob)
    {
        // Bass: knob 0-1 -> -12 to +12 dB
        smoothBassDb.setTargetValue ((bassKnob - 0.5f) * 24.0f);

        // Mid: knob 0-1 -> -15 to +15 dB
        smoothMidDb.setTargetValue ((midKnob - 0.5f) * 30.0f);

        // Mid freq: 5-position selector
        static constexpr float midFreqs[5] = { 220.0f, 450.0f, 800.0f, 1600.0f, 3000.0f };
        int idx = juce::jlimit (0, 4, midFreqSelect - 1);
        smoothMidFreq.setTargetValue (midFreqs[idx]);

        // Treble: asymmetric mapping
        float trebleDb;
        if (trebleKnob >= 0.5f)
            trebleDb = (trebleKnob - 0.5f) * 2.0f * 14.0f;   // 0 to +14 dB
        else
            trebleDb = (trebleKnob - 0.5f) * 2.0f * 19.0f;   // -19 to 0 dB

        smoothTrebleDb.setTargetValue (trebleDb);
    }

    void process (juce::AudioBuffer<float>& buffer)
    {
        const int numChannels = buffer.getNumChannels();
        const int numSamples  = buffer.getNumSamples();

        static constexpr int kCoeffUpdateInterval = 32;

        for (int s = 0; s < numSamples; ++s)
        {
            const float bassDb   = smoothBassDb.getNextValue();
            const float midDb    = smoothMidDb.getNextValue();
            const float trebleDb = smoothTrebleDb.getNextValue();
            const float midFreq  = smoothMidFreq.getNextValue();

            // Update coefficients at block-rate (every 32 samples)
            if (s % kCoeffUpdateInterval == 0)
            {
                for (int ch = 0; ch < numChannels; ++ch)
                {
                    lowShelf[ch].setGainDb (bassDb);
                    midPeak[ch].setGainDb (midDb);
                    midPeak[ch].setFrequency (midFreq);
                    highShelf[ch].setGainDb (trebleDb);
                }
            }

            for (int ch = 0; ch < numChannels; ++ch)
            {
                float sample = buffer.getWritePointer (ch)[s];
                sample = lowShelf[ch].processSample (sample);
                sample = midPeak[ch].processSample (sample);
                sample = highShelf[ch].processSample (sample);
                buffer.getWritePointer (ch)[s] = sample;
            }
        }
    }

private:
    double sr = 44100.0;

    Biquad lowShelf[2];
    Biquad midPeak[2];
    Biquad highShelf[2];

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothBassDb;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothMidDb;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothTrebleDb;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothMidFreq;
};
