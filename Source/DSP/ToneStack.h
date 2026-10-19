#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include "Primitives/Biquad.h"
#include <cmath>

class ToneStack
{
public:
    /// A knob's filter as gain / frequency / Q measured at knob 0, 2.5, 5, 7.5 and 10,
    /// interpolated linearly in between. Gain at 5 is 0 dB (the knobs are flat at noon).
    struct KnobCurve
    {
        float gainDb[5], hz[5], q[5];

        void at (float knob, float& g, float& f, float& qOut) const
        {
            const float x = juce::jlimit (0.0f, 1.0f, knob) * 4.0f;
            const int i = juce::jmin (3, static_cast<int> (x));
            const float t = x - static_cast<float> (i);
            g    = gainDb[i] + t * (gainDb[i + 1] - gainDb[i]);
            f    = hz[i]     + t * (hz[i + 1]     - hz[i]);
            qOut = q[i]      + t * (q[i + 1]      - q[i]);
        }
    };

    void prepare (double sampleRate, int /*maxBlockSize*/)
    {
        sr = sampleRate;

        for (int ch = 0; ch < 2; ++ch)
        {
            // Fixed voicing: the reference's response with every tone knob at noon
            // (bass bump, mid scoop, presence bump). The knobs themselves are flat at noon.
            voicingLow[ch].prepare (sampleRate);
            voicingLow[ch].setParams (Biquad::Type::Peak, kVoicingLowHz, kVoicingLowQ, kVoicingLowDb);
            voicingHigh[ch].prepare (sampleRate);
            voicingHigh[ch].setParams (Biquad::Type::Peak, kVoicingHighHz, kVoicingHighQ, kVoicingHighDb);

            lowShelf[ch].prepare (sampleRate);
            lowShelf[ch].setParams (Biquad::Type::LowShelf, 100.0f, 0.5f, 0.0f);

            midPeak[ch].prepare (sampleRate);
            midPeak[ch].setParams (Biquad::Type::Peak, 800.0f, 1.5f, 0.0f);

            highShelf[ch].prepare (sampleRate);
            highShelf[ch].setParams (Biquad::Type::HighShelf, 2500.0f, 0.5f, 0.0f);
        }

        smoothBass.reset (sampleRate, 0.02);
        smoothMidDb.reset (sampleRate, 0.02);
        smoothTrebleDb.reset (sampleRate, 0.02);
        smoothMidFreq.reset (sampleRate, 0.05);

        smoothBass.setCurrentAndTargetValue (0.5f);
        smoothMidDb.setCurrentAndTargetValue (0.0f);
        smoothTrebleDb.setCurrentAndTargetValue (0.0f);
        smoothMidFreq.setCurrentAndTargetValue (800.0f);
    }

    void reset()
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            voicingLow[ch].reset();
            voicingHigh[ch].reset();
            lowShelf[ch].reset();
            midPeak[ch].reset();
            highShelf[ch].reset();
        }
    }

    void setParams (float bassKnob, float midKnob, int midFreqSelect, float trebleKnob)
    {
        // Bass: shelf shape and depth follow the reference per knob position (kBassCurve)
        smoothBass.setTargetValue (bassKnob);

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
            const float bassKnob = smoothBass.getNextValue();
            const float midDb    = smoothMidDb.getNextValue();
            const float trebleDb = smoothTrebleDb.getNextValue();
            const float midFreq  = smoothMidFreq.getNextValue();

            // Update coefficients at block-rate (every 32 samples)
            if (s % kCoeffUpdateInterval == 0)
            {
                float bassDb, bassHz, bassQ;
                kBassCurve.at (bassKnob, bassDb, bassHz, bassQ);

                for (int ch = 0; ch < numChannels; ++ch)
                {
                    lowShelf[ch].setParams (Biquad::Type::LowShelf, bassHz, bassQ, bassDb);
                    midPeak[ch].setGainDb (midDb);
                    midPeak[ch].setFrequency (midFreq);
                    highShelf[ch].setGainDb (trebleDb);
                }
            }

            for (int ch = 0; ch < numChannels; ++ch)
            {
                float sample = buffer.getWritePointer (ch)[s] * kVoicingGain;
                sample = voicingLow[ch].processSample (sample);
                sample = voicingHigh[ch].processSample (sample);
                sample = lowShelf[ch].processSample (sample);
                sample = midPeak[ch].processSample (sample);
                sample = highShelf[ch].processSample (sample);
                buffer.getWritePointer (ch)[s] = sample;
            }
        }
    }

private:
    // Fitted to the reference (cabinet bypassed, all tone knobs at noon), 30 Hz-20 kHz within 0.5 dB:
    // +4.6 dB at 57 Hz, -3.2 dB at 1 kHz, +2.3 dB at 5.9 kHz.
    static constexpr float kVoicingLowHz  = 57.5f,  kVoicingLowQ  = 0.44f, kVoicingLowDb  = 8.36f;
    static constexpr float kVoicingHighHz = 5906.0f, kVoicingHighQ = 0.69f, kVoicingHighDb = 6.08f;
    static constexpr float kVoicingGain   = 0.6485f;  // -3.76 dB

    // Bass low shelf fitted to the reference per knob position (within 0.6 dB, 20 Hz-2 kHz).
    static constexpr KnobCurve kBassCurve {
        { -16.25f, -6.84f,  0.0f,  5.53f, 14.08f },   // gain dB
        {  77.1f,  84.0f,  82.0f, 79.7f, 152.3f },    // Hz
        {  0.42f,  0.589f, 0.64f, 0.688f, 0.589f }    // Q
    };

    double sr = 44100.0;

    Biquad voicingLow[2];
    Biquad voicingHigh[2];
    Biquad lowShelf[2];
    Biquad midPeak[2];
    Biquad highShelf[2];

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothBass;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothMidDb;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothTrebleDb;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothMidFreq;
};
