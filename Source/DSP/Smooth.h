#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <algorithm>
#include <cmath>

/// FX Rack Smooth: a first-order low-pass (6 dB/octave) at 3-35 kHz that keeps its analog
/// shape at every sample rate (the same curve at 44.1, 48 and 96 kHz). A bilinear one-pole would put
/// a zero at Nyquist, so the filter is instead matched to the analog magnitude at DC, at Nyquist and at a mid frequency
/// (closed form, below). The cutoff is 1.02 x the displayed frequency; within 0.17 dB of the target at 3-35 kHz and
/// 44.1-96 kHz. The processor runs one instance before the amp (Pre) and one after the power
/// stage (Post). Switching crossfades linearly over 50 ms; off, the signal passes bit for bit.
class Smooth
{
public:
    void prepare (double sampleRate, int /*maxBlockSize*/)
    {
        sr = sampleRate;
        smoothHz.reset (sampleRate, kRampSeconds);
        smoothHz.setCurrentAndTargetValue (targetHz);
        mix.reset (sampleRate, kRampSeconds);
        mix.setCurrentAndTargetValue (on ? 1.0f : 0.0f);
        reset();
        updateCoefficients (targetHz);
    }

    void reset()
    {
        for (auto& s : state)
            s = {};
    }

    /// `hz`: the displayed frequency, 3-35 kHz.
    void setParams (bool enabled, float hz)
    {
        on = enabled;
        targetHz = juce::jlimit (kMinHz, kMaxHz, hz);
        mix.setTargetValue (on ? 1.0f : 0.0f);
        smoothHz.setTargetValue (targetHz);
    }

    void process (juce::AudioBuffer<float>& buffer)
    {
        const int numChannels = juce::jmin (2, buffer.getNumChannels());
        const int numSamples  = buffer.getNumSamples();

        static constexpr int kCoeffUpdateInterval = 32;

        for (int s = 0; s < numSamples; ++s)
        {
            // Retune every 32 samples, gliding over those 32 (no zipper noise while the frequency moves)
            const float hz = smoothHz.getNextValue();
            if (s % kCoeffUpdateInterval == 0)
                updateCoefficients (hz, kCoeffUpdateInterval);
            if (glideLeft > 0)
                advanceGlide();

            const float m = mix.getNextValue();

            for (int ch = 0; ch < numChannels; ++ch)
            {
                float* data = buffer.getWritePointer (ch);
                const float dry = data[s];
                auto& st = state[ch];

                double y = b0 * dry + b1 * st.x1 - a1 * st.y1;
                y += 1e-18; y -= 1e-18;   // denormal flush
                st.x1 = dry;
                st.y1 = y;

                data[s] = dry + m * (static_cast<float> (y) - dry);
            }
        }
    }

private:
    static constexpr float kMinHz = 3000.0f, kMaxHz = 35000.0f;
    static constexpr double kCutoffScale = 1.02;     // fitted
    static constexpr double kMatchFraction = 0.25;   // mid match point: the cutoff, at most sampleRate / 4
    static constexpr double kRampSeconds = 0.05;

    struct State { double x1 = 0.0, y1 = 0.0; };

    /// First-order low-pass H(z) = (b0 + b1 z^-1) / (1 + a1 z^-1) whose magnitude equals the analog
    /// 1 / (1 + s / wc) at DC, Nyquist and fm. With c = cos w, |H|^2 = (P + Q c) / (1 + S c): three linear equations
    /// in P, Q, S, then factored back into coefficients. Glides to the new values over `glideSamples` (0: at once).
    void updateCoefficients (float hz, int glideSamples = 0)
    {
        const double fc = kCutoffScale * static_cast<double> (hz);
        const auto analog2 = [fc] (double f) { return 1.0 / (1.0 + (f / fc) * (f / fc)); };
        const double fm = std::min (fc, kMatchFraction * sr);
        const double c0 = std::cos (juce::MathConstants<double>::twoPi * fm / sr);
        const double gm = analog2 (fm), gn = analog2 (0.5 * sr);

        // P + Q - S = 1;  P - Q + gn S = gn;  P + c0 Q - gm c0 S = gm   (Cramer's rule)
        const double m[3][3] = { { 1.0, 1.0, -1.0 }, { 1.0, -1.0, gn }, { 1.0, c0, -gm * c0 } };
        const double r[3] = { 1.0, gn, gm };
        const auto det = [] (const double (&a)[3][3])
        {
            return a[0][0] * (a[1][1] * a[2][2] - a[1][2] * a[2][1])
                 - a[0][1] * (a[1][0] * a[2][2] - a[1][2] * a[2][0])
                 + a[0][2] * (a[1][0] * a[2][1] - a[1][1] * a[2][0]);
        };
        const double d = det (m);
        double x[3];
        for (int k = 0; k < 3; ++k)
        {
            double mk[3][3];
            for (int i = 0; i < 3; ++i)
                for (int j = 0; j < 3; ++j)
                    mk[i][j] = j == k ? r[i] : m[i][j];
            x[k] = det (mk) / d;
        }
        const double P = x[0], Q = x[1], S = x[2];

        const double a = std::abs (S) > 1e-12 ? (1.0 - std::sqrt (std::max (0.0, 1.0 - S * S))) / S : 0.0;
        const double u = (1.0 + a * a) * P, v = (1.0 + a * a) * Q;
        const double sum = std::sqrt (std::max (0.0, u + v)), diff = std::sqrt (std::max (0.0, u - v));
        const double to[3] = { 0.5 * (sum + diff), 0.5 * (sum - diff), a };

        double* c[3] = { &b0, &b1, &a1 };
        for (int i = 0; i < 3; ++i)
        {
            target[i] = to[i];
            step[i] = glideSamples > 1 ? (to[i] - *c[i]) / glideSamples : 0.0;
            if (glideSamples <= 1) *c[i] = to[i];
        }
        glideLeft = glideSamples > 1 ? glideSamples : 0;
    }

    void advanceGlide()
    {
        const bool last = --glideLeft == 0;
        double* c[3] = { &b0, &b1, &a1 };
        for (int i = 0; i < 3; ++i)
            *c[i] = last ? target[i] : *c[i] + step[i];
    }

    double sr = 48000.0;
    double b0 = 1.0, b1 = 0.0, a1 = 0.0;
    double target[3] = {}, step[3] = {};
    int glideLeft = 0;
    State state[2];
    bool on = false;
    float targetHz = 21000.0f;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> smoothHz;   // glides on a log scale
    juce::LinearSmoothedValue<float> mix;
};
