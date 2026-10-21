#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include "Primitives/Biquad.h"
#include <cmath>

class ToneStack
{
public:
    /// Value at a knob position (0-1) from values measured at N evenly spaced positions, linear in between
    /// (with `logScale`, on a log scale: for frequencies).
    template <int N>
    static float atKnob (const float (&v)[N], float knob, bool logScale = false)
    {
        const float x = juce::jlimit (0.0f, 1.0f, knob) * static_cast<float> (N - 1);
        const int i = juce::jmin (N - 2, static_cast<int> (x));
        const float t = x - static_cast<float> (i);
        return logScale ? v[i] * std::pow (v[i + 1] / v[i], t) : v[i] + t * (v[i + 1] - v[i]);
    }

    /// A knob's filter as gain / frequency / Q measured at N evenly spaced knob positions.
    /// Gain at noon is 0 dB (the knobs are flat at noon).
    template <int N>
    struct KnobCurve
    {
        float gainDb[N], hz[N], q[N];

        void at (float knob, float& g, float& f, float& qOut) const
        {
            g = atKnob (gainDb, knob);
            f = atKnob (hz, knob, true);
            qOut = atKnob (q, knob);
        }
    };

    /// Value at (bass, treble) from an N x N table over evenly spaced knob positions (bilinear; with `logScale`,
    /// on log values).
    template <int N>
    static float atKnobs (const float (&v)[N][N], float bassKnob, float trebleKnob, bool logScale = false)
    {
        const float x = juce::jlimit (0.0f, 1.0f, bassKnob) * static_cast<float> (N - 1);
        const float y = juce::jlimit (0.0f, 1.0f, trebleKnob) * static_cast<float> (N - 1);
        const int i = juce::jmin (N - 2, static_cast<int> (x)), j = juce::jmin (N - 2, static_cast<int> (y));
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

            // Retune every 32 samples, gliding to the new coefficients over those 32 (no zipper noise)
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
                    constexpr int n = kCoeffUpdateInterval;
                    lowShelf[ch].glideTo (Biquad::Type::LowShelf, bassHz, bassQ, bassDb, n);
                    midPeak[ch].glideTo (Biquad::Type::Peak, midHz, midQ, midDb, n);
                    highShelf[ch].glideTo (Biquad::Type::HighShelf, trebleHz, trebleQ, trebleDb, n);
                    trebleTop[ch].glideTo (Biquad::Type::Peak, trebleTopHz, kTrebleTopQ, topDb, n);
                    interaction[ch].glideTo (Biquad::Type::Peak, interHz, interQ, interDb, n);
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

    // The knobs' tapers are curved, so each is measured densely (step 11.3): the filter is fitted to the reference at
    // every 0.25 of the knob and tabulated every 0.5 (bass, mid) or 0.25 (treble, whose shape changes fastest near 0
    // and 10). Between table points the error stays within ~1 dB (2.3 dB at treble 0.125).

    // Bass: low shelf per knob position 0, 0.5 ... 10 (fit within 1.05 dB, 20 Hz-4 kHz).
    static constexpr KnobCurve<21> kBassCurve {
        { -16.24f, -14.11f, -12.09f, -10.21f, -8.47f, -6.84f, -5.30f, -3.86f, -2.50f, -1.22f, 0.00f,
          1.16f, 2.28f, 3.36f, 4.44f, 5.53f, 6.69f, 7.96f, 9.44f, 11.31f, 14.08f },   // gain dB
        { 77.2f, 80.7f, 83.1f, 84.4f, 84.6f, 84.0f, 82.9f, 81.6f, 80.2f, 78.8f, 82.0f,
          76.9f, 76.6f, 76.8f, 77.8f, 79.7f, 83.0f, 88.5f, 97.6f, 114.3f, 152.4f },   // Hz
        { 0.420f, 0.456f, 0.493f, 0.528f, 0.561f, 0.589f, 0.614f, 0.634f, 0.650f, 0.662f, 0.640f,
          0.678f, 0.683f, 0.686f, 0.688f, 0.688f, 0.687f, 0.685f, 0.679f, 0.662f, 0.589f }    // Q
    };

    // Treble: high shelf plus a peak at 19 kHz whose gain follows the knob (it brings the response back toward 0 dB at
    // the top like the reference), per knob position 0, 0.25 ... 10 (fit within 1.6 dB, 100 Hz-20 kHz).
    static constexpr KnobCurve<41> kTrebleCurve {
        { -29.91f, -23.49f, -19.26f, -16.28f, -13.98f, -12.09f, -10.51f, -9.14f, -7.94f, -6.88f, -5.93f,
          -5.07f, -4.30f, -3.60f, -2.96f, -2.37f, -1.83f, -1.32f, -0.85f, -0.41f, 0.00f, 0.39f,
          0.77f, 1.13f, 1.49f, 1.84f, 2.19f, 2.54f, 2.91f, 3.29f, 3.69f, 4.14f, 4.63f,
          5.18f, 5.83f, 6.60f, 7.56f, 8.78f, 10.44f, 12.81f, 16.48f },   // gain dB
        { 2913.2f, 1971.7f, 1511.7f, 1244.2f, 1063.1f, 929.6f, 825.7f, 742.0f, 672.9f, 614.7f, 565.2f,
          522.6f, 485.6f, 453.4f, 425.2f, 400.5f, 378.8f, 359.7f, 342.9f, 328.2f, 420.0f, 304.5f,
          295.1f, 287.4f, 281.3f, 276.7f, 273.8f, 272.7f, 273.4f, 276.3f, 281.7f, 290.0f, 301.8f,
          318.1f, 339.9f, 369.4f, 409.5f, 465.5f, 547.7f, 678.8f, 911.8f },   // Hz
        { 0.380f, 0.439f, 0.479f, 0.507f, 0.530f, 0.548f, 0.565f, 0.579f, 0.593f, 0.605f, 0.616f,
          0.627f, 0.636f, 0.644f, 0.651f, 0.656f, 0.660f, 0.663f, 0.664f, 0.664f, 0.580f, 0.657f,
          0.652f, 0.645f, 0.636f, 0.626f, 0.615f, 0.603f, 0.590f, 0.577f, 0.563f, 0.550f, 0.537f,
          0.524f, 0.512f, 0.501f, 0.491f, 0.480f, 0.469f, 0.453f, 0.431f }    // Q
    };
    static constexpr float kTrebleTopDb[41] = { 1.86f, 5.25f, 6.00f, 6.24f, 6.25f, 6.11f, 5.88f, 5.57f, 5.21f, 4.80f, 4.37f,
          3.92f, 3.46f, 3.00f, 2.55f, 2.10f, 1.65f, 1.22f, 0.81f, 0.40f, 0.00f, -0.39f,
          -0.77f, -1.14f, -1.51f, -1.88f, -2.25f, -2.62f, -3.00f, -3.40f, -3.81f, -4.24f, -4.69f,
          -5.18f, -5.69f, -6.22f, -6.74f, -7.15f, -7.23f, -6.30f, -2.58f };
    static constexpr float kTrebleTopHz = 19000.0f, kTrebleTopQ = 1.2f;

    // Mid peak, fitted to the reference at every selector position and knob position 0, 0.5 ... 10 (within 0.3 dB;
    // 1.3 dB at position 5 cut). Centres per selector position (manual: 220 / 450 / 800 / 1.6k / 3k).
    static constexpr float kMidHz[5] = { 249.1f, 542.9f, 894.9f, 1772.7f, 2883.9f };
    static constexpr float kMidGainDb[5][21] = {
        { -18.38f, -12.38f, -8.87f, -6.37f, -4.43f, -2.85f, -2.20f, -1.60f, -1.03f, -0.50f, 0.00f,
          0.83f, 1.58f, 2.28f, 2.92f, 3.52f, 6.50f, 8.72f, 10.48f, 11.94f, 13.19f },
        { -18.33f, -12.35f, -8.85f, -6.36f, -4.43f, -2.85f, -2.20f, -1.59f, -1.03f, -0.50f, 0.00f,
          0.73f, 1.41f, 2.03f, 2.62f, 3.16f, 6.06f, 8.23f, 9.96f, 11.40f, 12.63f },
        { -19.92f, -13.43f, -9.76f, -7.19f, -5.21f, -3.59f, -2.75f, -1.97f, -1.26f, -0.61f, 0.00f,
          0.34f, 0.67f, 0.98f, 1.28f, 1.58f, 4.89f, 7.28f, 9.15f, 10.69f, 11.99f },
        { -19.80f, -14.39f, -11.10f, -8.73f, -6.87f, -5.32f, -3.96f, -2.79f, -1.76f, -0.83f, 0.00f,
          0.54f, 1.04f, 1.52f, 1.97f, 2.40f, 4.85f, 6.75f, 8.31f, 9.63f, 10.77f },
        { -18.98f, -14.56f, -11.66f, -9.49f, -7.76f, -6.32f, -4.63f, -3.22f, -2.01f, -0.95f, 0.00f,
          0.24f, 0.47f, 0.69f, 0.91f, 1.13f, 4.34f, 6.67f, 8.51f, 10.01f, 11.29f }
    };
    static constexpr float kMidQ[5][21] = {
        { 0.245f, 0.291f, 0.288f, 0.253f, 0.193f, 0.114f, 0.130f, 0.146f, 0.163f, 0.181f, 0.200f,
          0.207f, 0.213f, 0.220f, 0.226f, 0.232f, 0.220f, 0.214f, 0.213f, 0.213f, 0.214f },
        { 0.689f, 0.796f, 0.756f, 0.619f, 0.408f, 0.134f, 0.146f, 0.159f, 0.172f, 0.185f, 0.200f,
          0.221f, 0.242f, 0.262f, 0.281f, 0.299f, 0.330f, 0.359f, 0.386f, 0.412f, 0.436f },
        { 0.511f, 0.614f, 0.596f, 0.501f, 0.348f, 0.146f, 0.156f, 0.166f, 0.176f, 0.186f, 0.200f,
          0.194f, 0.192f, 0.191f, 0.189f, 0.188f, 0.214f, 0.239f, 0.261f, 0.281f, 0.300f },
        { 0.880f, 1.010f, 0.979f, 0.841f, 0.619f, 0.327f, 0.345f, 0.360f, 0.372f, 0.381f, 0.200f,
          0.412f, 0.435f, 0.456f, 0.477f, 0.497f, 0.501f, 0.511f, 0.524f, 0.539f, 0.554f },
        { 0.429f, 0.488f, 0.496f, 0.467f, 0.410f, 0.329f, 0.363f, 0.393f, 0.421f, 0.446f, 0.200f,
          0.456f, 0.442f, 0.429f, 0.417f, 0.405f, 0.382f, 0.374f, 0.373f, 0.375f, 0.380f }
    };
    // Bass x treble interaction. The reference's tone stack is passive (schematic 07S534): bass and treble share
    // components, so together they reshape the low mids. Measured as the combined response minus the sum of the single
    // knobs on a 5 x 5 x 5 x 5 grid (it does not depend on Mid or Mid Freq, within 0.3 dB) and fitted with one peak per
    // [bass][treble] knob position 0, 1.25 ... 10 (residual <= 0.71 dB; it rises steeply above 7.5). Largest at bass 10 +
    // treble 10: -3.8 dB at 464 Hz, a deeper scoop than the knobs alone give.
    static constexpr float kInteractionDb[9][9] = {
        { 0.36f, 0.30f, 0.21f, 0.00f, 0.00f, 0.00f, 0.32f, 0.78f, 1.60f },
        { 0.32f, 0.27f, 0.19f, 0.00f, 0.00f, 0.00f, 0.25f, 0.58f, 1.21f },
        { 0.21f, 0.18f, 0.00f, 0.00f, 0.00f, 0.00f, 0.00f, 0.37f, 0.75f },
        { 0.00f, 0.00f, 0.00f, 0.00f, 0.00f, 0.00f, 0.00f, 0.18f, 0.35f },
        { 0.00f, 0.00f, 0.00f, 0.00f, 0.00f, 0.00f, 0.00f, 0.00f, 0.00f },
        { 0.00f, 0.00f, 0.00f, 0.00f, 0.00f, 0.00f, 0.00f, -0.17f, -0.34f },
        { 0.00f, 0.00f, 0.00f, 0.00f, 0.00f, 0.00f, -0.19f, -0.39f, -0.76f },
        { 0.00f, 0.00f, 0.00f, 0.00f, 0.00f, 0.00f, -0.38f, -0.79f, -1.55f },
        { 0.41f, 0.37f, 0.30f, 0.19f, 0.00f, -0.31f, -0.82f, -1.77f, -3.83f }
    };
    static constexpr float kInteractionHz[9][9] = {
        { 331.2f, 299.2f, 259.3f, 400.0f, 400.0f, 400.0f, 549.9f, 505.1f, 509.0f },
        { 259.4f, 241.8f, 217.9f, 400.0f, 400.0f, 400.0f, 445.1f, 403.8f, 393.5f },
        { 207.3f, 195.8f, 400.0f, 400.0f, 400.0f, 400.0f, 400.0f, 350.1f, 338.7f },
        { 400.0f, 400.0f, 400.0f, 400.0f, 400.0f, 400.0f, 400.0f, 313.6f, 304.7f },
        { 400.0f, 400.0f, 400.0f, 400.0f, 400.0f, 400.0f, 400.0f, 400.0f, 400.0f },
        { 400.0f, 400.0f, 400.0f, 400.0f, 400.0f, 400.0f, 400.0f, 272.6f, 270.5f },
        { 400.0f, 400.0f, 400.0f, 400.0f, 400.0f, 400.0f, 273.6f, 265.6f, 267.9f },
        { 400.0f, 400.0f, 400.0f, 400.0f, 400.0f, 400.0f, 280.1f, 279.0f, 287.1f },
        { 435.6f, 432.2f, 427.3f, 422.2f, 400.0f, 420.5f, 426.2f, 438.2f, 464.1f }
    };
    static constexpr float kInteractionQ[9][9] = {
        { 0.856f, 0.945f, 1.087f, 0.700f, 0.700f, 0.700f, 0.891f, 0.738f, 0.614f },
        { 1.085f, 1.138f, 1.225f, 0.700f, 0.700f, 0.700f, 1.369f, 1.136f, 0.964f },
        { 1.188f, 1.240f, 0.700f, 0.700f, 0.700f, 0.700f, 0.700f, 1.397f, 1.188f },
        { 0.700f, 0.700f, 0.700f, 0.700f, 0.700f, 0.700f, 0.700f, 1.547f, 1.330f },
        { 0.700f, 0.700f, 0.700f, 0.700f, 0.700f, 0.700f, 0.700f, 0.700f, 0.700f },
        { 0.700f, 0.700f, 0.700f, 0.700f, 0.700f, 0.700f, 0.700f, 1.591f, 1.422f },
        { 0.700f, 0.700f, 0.700f, 0.700f, 0.700f, 0.700f, 1.684f, 1.548f, 1.423f },
        { 0.700f, 0.700f, 0.700f, 0.700f, 0.700f, 0.700f, 1.546f, 1.476f, 1.427f },
        { 0.959f, 0.959f, 0.956f, 0.951f, 0.700f, 0.946f, 0.959f, 0.994f, 1.089f }
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
