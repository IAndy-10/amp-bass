#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>

/// FX Rack Tight, fitted to the reference: a first-order high-pass (6 dB/octave, -3 dB at the set frequency,
/// 30 Hz-3 kHz) with a makeup gain that grows with the frequency, 2.47 dB per decade above 30 Hz (+0.55 dB at 50 Hz,
/// +4.9 dB at 3 kHz); within 0.65 dB of the reference. The processor runs one instance before the amp (Pre: it changes
/// what reaches the power stage) and one after the power stage (Post). Switching crossfades linearly over 50 ms;
/// off, the signal passes bit for bit.
class Tight
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

                const float wet = static_cast<float> (y * makeup);
                data[s] = dry + m * (wet - dry);
            }
        }
    }

private:
    static constexpr float kMinHz = 30.0f, kMaxHz = 3000.0f;
    static constexpr double kMakeupDbPerDecade = 2.468;   // above kMinHz
    static constexpr double kRampSeconds = 0.05;

    struct State { double x1 = 0.0, y1 = 0.0; };

    /// Glides to the new values over `glideSamples` (0: at once).
    void updateCoefficients (float hz, int glideSamples = 0)
    {
        // Bilinear first-order high-pass: H(z) = (1 - z^-1) / ((1 + K) + (K - 1) z^-1)
        const double k = std::tan (juce::MathConstants<double>::pi * juce::jmin (static_cast<double> (hz), 0.45 * sr) / sr);
        const double norm = 1.0 / (1.0 + k);
        const double to[4] = { norm, -norm, (k - 1.0) * norm,
                               std::pow (10.0, kMakeupDbPerDecade * std::log10 (static_cast<double> (hz) / kMinHz) / 20.0) };
        double* c[4] = { &b0, &b1, &a1, &makeup };
        for (int i = 0; i < 4; ++i)
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
        double* c[4] = { &b0, &b1, &a1, &makeup };
        for (int i = 0; i < 4; ++i)
            *c[i] = last ? target[i] : *c[i] + step[i];
    }

    double sr = 48000.0;
    double b0 = 1.0, b1 = 0.0, a1 = 0.0, makeup = 1.0;
    double target[4] = {}, step[4] = {};
    int glideLeft = 0;
    State state[2];
    bool on = false;
    float targetHz = 50.0f;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> smoothHz;   // glides on a log scale
    juce::LinearSmoothedValue<float> mix;
};
