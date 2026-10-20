#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include "Primitives/Biquad.h"
#include <cmath>

class ToneStack
{
public:
    /// Value at a knob position from values measured at knob 0, 2.5, 5, 7.5 and 10 (linear in between).
    static float atKnob (const float (&v)[5], float knob)
    {
        const float x = juce::jlimit (0.0f, 1.0f, knob) * 4.0f;
        const int i = juce::jmin (3, static_cast<int> (x));
        return v[i] + (x - static_cast<float> (i)) * (v[i + 1] - v[i]);
    }

    /// A knob's filter as gain / frequency / Q measured at the five knob positions.
    /// Gain at 5 is 0 dB (the knobs are flat at noon).
    struct KnobCurve
    {
        float gainDb[5], hz[5], q[5];

        void at (float knob, float& g, float& f, float& qOut) const
        {
            g = atKnob (gainDb, knob);
            f = atKnob (hz, knob);
            qOut = atKnob (q, knob);
        }
    };

    /// Value at (bass, treble) from a table over knob positions 0 / 2.5 / 5 / 7.5 / 10 on both (bilinear).
    /// With `logScale` the interpolation runs on log values (for frequencies).
    static float atKnobs (const float (&v)[5][5], float bassKnob, float trebleKnob, bool logScale = false)
    {
        const float x = juce::jlimit (0.0f, 1.0f, bassKnob) * 4.0f, y = juce::jlimit (0.0f, 1.0f, trebleKnob) * 4.0f;
        const int i = juce::jmin (3, static_cast<int> (x)), j = juce::jmin (3, static_cast<int> (y));
        const float fx = x - static_cast<float> (i), fy = y - static_cast<float> (j);
        auto at = [&] (int a, int b) { return logScale ? std::log (v[a][b]) : v[a][b]; };
        const float lo = at (i, j) + fy * (at (i, j + 1) - at (i, j));
        const float hi = at (i + 1, j) + fy * (at (i + 1, j + 1) - at (i + 1, j));
        const float r = lo + fx * (hi - lo);
        return logScale ? std::exp (r) : r;
    }

    void prepare (double sampleRate, int /*maxBlockSize*/)
    {
        sr = sampleRate;
        trebleTopHz = std::min (kTrebleTopHz, 0.45f * static_cast<float> (sampleRate));

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

            trebleTop[ch].prepare (sampleRate);
            trebleTop[ch].setParams (Biquad::Type::Peak, trebleTopHz, kTrebleTopQ, 0.0f);

            interaction[ch].prepare (sampleRate);
            interaction[ch].setParams (Biquad::Type::Peak, 400.0f, 0.7f, 0.0f);
        }

        smoothBass.reset (sampleRate, 0.02);
        smoothMid.reset (sampleRate, 0.02);
        smoothTreble.reset (sampleRate, 0.02);
        smoothMidPos.reset (sampleRate, 0.05);

        smoothBass.setCurrentAndTargetValue (0.5f);
        smoothMid.setCurrentAndTargetValue (0.5f);
        smoothTreble.setCurrentAndTargetValue (0.5f);
        smoothMidPos.setCurrentAndTargetValue (2.0f);
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
            trebleTop[ch].reset();
            interaction[ch].reset();
        }
    }

    void setParams (float bassKnob, float midKnob, int midFreqSelect, float trebleKnob)
    {
        // Bass: shelf shape and depth follow the reference per knob position (kBassCurve)
        smoothBass.setTargetValue (bassKnob);

        // Mid: peak per frequency position and knob position (kMid*). The selector is smoothed as a
        // position (0-4) so a switch glides between neighbouring settings, with Hz moving on a log scale.
        smoothMid.setTargetValue (midKnob);
        smoothMidPos.setTargetValue (static_cast<float> (juce::jlimit (0, 4, midFreqSelect - 1)));

        // Treble: shelf plus a fixed-frequency top-end peak, both per knob position (kTreble*)
        smoothTreble.setTargetValue (trebleKnob);
    }

    void process (juce::AudioBuffer<float>& buffer)
    {
        const int numChannels = buffer.getNumChannels();
        const int numSamples  = buffer.getNumSamples();

        static constexpr int kCoeffUpdateInterval = 32;

        for (int s = 0; s < numSamples; ++s)
        {
            const float bassKnob = smoothBass.getNextValue();
            const float midKnob  = smoothMid.getNextValue();
            const float trebleKnob = smoothTreble.getNextValue();
            const float midPos   = smoothMidPos.getNextValue();

            // Update coefficients at block-rate (every 32 samples)
            if (s % kCoeffUpdateInterval == 0)
            {
                float bassDb, bassHz, bassQ, trebleDb, trebleHz, trebleQ;
                kBassCurve.at (bassKnob, bassDb, bassHz, bassQ);
                kTrebleCurve.at (trebleKnob, trebleDb, trebleHz, trebleQ);
                const float topDb = atKnob (kTrebleTopDb, trebleKnob);

                const int mi = juce::jlimit (0, 3, static_cast<int> (midPos));
                const float mt = juce::jlimit (0.0f, 1.0f, midPos - static_cast<float> (mi));
                const float midDb = atKnob (kMidGainDb[mi], midKnob)
                                  + mt * (atKnob (kMidGainDb[mi + 1], midKnob) - atKnob (kMidGainDb[mi], midKnob));
                const float midQ = atKnob (kMidQ[mi], midKnob)
                                 + mt * (atKnob (kMidQ[mi + 1], midKnob) - atKnob (kMidQ[mi], midKnob));
                const float midHz = kMidHz[mi] * std::pow (kMidHz[mi + 1] / kMidHz[mi], mt);

                // Bass/treble interaction (the reference's passive stack): Hz interpolated on a log scale
                const float interDb = atKnobs (kInteractionDb, bassKnob, trebleKnob);
                const float interHz = atKnobs (kInteractionHz, bassKnob, trebleKnob, true);
                const float interQ  = atKnobs (kInteractionQ, bassKnob, trebleKnob);

                for (int ch = 0; ch < numChannels; ++ch)
                {
                    lowShelf[ch].setParams (Biquad::Type::LowShelf, bassHz, bassQ, bassDb);
                    midPeak[ch].setParams (Biquad::Type::Peak, midHz, midQ, midDb);
                    highShelf[ch].setParams (Biquad::Type::HighShelf, trebleHz, trebleQ, trebleDb);
                    trebleTop[ch].setGainDb (topDb);
                    interaction[ch].setParams (Biquad::Type::Peak, interHz, interQ, interDb);
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
                sample = trebleTop[ch].processSample (sample);
                sample = interaction[ch].processSample (sample);
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

    // Treble: high shelf per knob position plus a peak at 19 kHz whose gain follows the knob, which
    // brings the response back toward 0 dB at the top like the reference (within 1.1 dB, 200 Hz-20 kHz).
    static constexpr KnobCurve kTrebleCurve {
        { -30.02f, -5.96f,  0.0f,  3.70f, 16.47f },   // gain dB
        { 2928.5f, 564.4f, 420.0f, 281.5f, 913.5f },  // Hz
        {  0.376f,  0.594f, 0.58f, 0.557f, 0.434f }   // Q
    };
    static constexpr float kTrebleTopDb[5] = { 1.97f, 4.42f, 0.0f, -3.82f, -2.56f };
    static constexpr float kTrebleTopHz = 19000.0f, kTrebleTopQ = 1.2f;

    // Mid peak fitted to the reference (within 0.5 dB; 1.3 dB at position 5 cut, 0.8 dB at position 5 boost). Centres per
    // selector position (manual: 220 / 450 / 800 / 1.6k / 3k); gain and Q per [selector][knob 0, 2.5, 5, 7.5, 10], all
    // measured (2.5 and 7.5 used to be scaled from position 3, up to 3 dB off).
    static constexpr float kMidHz[5] = { 249.1f, 542.9f, 894.9f, 1772.7f, 2883.9f };
    static constexpr float kMidGainDb[5][5] = {
        { -18.380f, -2.850f, 0.000f, 3.520f, 13.190f },
        { -18.330f, -2.847f, 0.000f, 3.163f, 12.630f },
        { -19.920f, -3.593f, 0.000f, 1.578f, 11.990f },
        { -19.800f, -5.322f, 0.000f, 2.401f, 10.770f },
        { -18.980f, -6.318f, 0.000f, 1.128f, 11.290f }
    };
    static constexpr float kMidQ[5][5] = {
        { 0.245f, 0.114f, 0.102f, 0.232f, 0.214f },
        { 0.689f, 0.134f, 0.234f, 0.299f, 0.436f },
        { 0.511f, 0.146f, 0.166f, 0.188f, 0.300f },
        { 0.880f, 0.327f, 0.298f, 0.497f, 0.554f },
        { 0.429f, 0.329f, 0.179f, 0.405f, 0.379f }
    };
    // Bass x treble interaction. The reference's tone stack is passive (schematic 07S534): bass and treble share
    // components, so together they reshape the low mids. Measured as the combined response minus the sum of the single
    // knobs on a 5 x 5 x 5 x 5 grid (it does not depend on Mid or Mid Freq, within 0.3 dB) and fitted with one peak per
    // [bass][treble] knob position (residual <= 0.35 dB, 0.72 dB at bass 10 / treble 10). Largest at bass 10 + treble 10:
    // -3.8 dB at 464 Hz, a deeper scoop than the knobs alone give.
    static constexpr float kInteractionDb[5][5] = {
        { 0.36f, 0.21f, 0.00f, 0.32f, 1.60f },
        { 0.21f, 0.00f, 0.00f, 0.00f, 0.75f },
        { 0.00f, 0.00f, 0.00f, 0.00f, 0.00f },
        { 0.00f, 0.00f, 0.00f, -0.19f, -0.76f },
        { 0.41f, 0.31f, 0.00f, -0.83f, -3.83f }
    };
    static constexpr float kInteractionHz[5][5] = {
        { 332.7f, 261.9f, 400.0f, 547.8f, 507.5f },
        { 208.6f, 400.0f, 400.0f, 400.0f, 337.9f },
        { 400.0f, 400.0f, 400.0f, 400.0f, 400.0f },
        { 400.0f, 400.0f, 400.0f, 276.0f, 269.0f },
        { 437.9f, 431.8f, 400.0f, 425.4f, 464.1f }
    };
    static constexpr float kInteractionQ[5][5] = {
        { 0.855f, 1.083f, 0.700f, 0.896f, 0.618f },
        { 1.174f, 0.700f, 0.700f, 0.700f, 1.201f },
        { 0.700f, 0.700f, 0.700f, 0.700f, 0.700f },
        { 0.700f, 0.700f, 0.700f, 1.692f, 1.416f },
        { 0.978f, 0.981f, 0.700f, 0.952f, 1.085f }
    };
    float trebleTopHz = kTrebleTopHz;

    double sr = 44100.0;

    Biquad voicingLow[2];
    Biquad voicingHigh[2];
    Biquad lowShelf[2];
    Biquad midPeak[2];
    Biquad highShelf[2];
    Biquad trebleTop[2];
    Biquad interaction[2];

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothBass;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothMid;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothTreble;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothMidPos;
};
