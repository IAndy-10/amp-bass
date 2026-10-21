#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <algorithm>
#include <cmath>
#include "Primitives/Biquad.h"

/// Stage 7: power amp with the Tube Gain stage, fitted with the cabinet off.
///
/// The amp's main odd-harmonic distortion sits after Master and depends only on how loud the
/// signal would be there, not on how it got there. It is a soft static curve whose 3rd harmonic grows
/// about 1.7 dB per dB, and it distorts more at higher frequencies, most in the low mids. (The hardware,
/// schematic 07S426: a tube driver, 12AX7 + 12AU7, into a MOSFET output stage with global negative feedback.) Modelled as: wide emphasis peak -> static curve -> inverse
/// peak, then the fixed output low-pass. Below about -30 dBFS it is linear and the two peaks cancel.
///
/// Tube Gain acts inside the same stage (measured: Gain and Master trade 1:1, and the knee follows the
/// emphasis). Turning it down
///   - raises the small-signal level (+3.1 dB at 0),
///   - adds a level-independent 2nd harmonic (a kink: the two half-waves get slightly different gain,
///     strongest in the bass: -26 dBc at 41 Hz, -38 at 100, -48 at 400 at Tube Gain 0). The kink path is
///     DC-blocked so it can't shift the operating point,
///   - and biases the curve, with a bias that grows with the signal level (blocking). That gives the
///     extra compression and 2nd harmonic at high level (rising ~1.7 dB per dB, as an envelope-driven bias
///     does on this curve; a fixed bias would give ~0.8) while the 3rd harmonic stays put.
/// Tube Gain 5 and above adds almost nothing.
class PowerSection
{
public:
    void prepare (double sampleRate, int /*maxBlockSize*/)
    {
        const auto lp = outputLowPassFor (sampleRate);
        for (int ch = 0; ch < 2; ++ch)
        {
            emphasis[ch].prepare (sampleRate);
            emphasis[ch].setParams (Biquad::Type::Peak, kEmphasisHz, kEmphasisQ, kEmphasisDb);
            deEmphasis[ch].prepare (sampleRate);
            deEmphasis[ch].setParams (Biquad::Type::Peak, kEmphasisHz, kEmphasisQ, -kEmphasisDb);
            outputLowPass[ch].prepare (sampleRate);
            outputLowPass[ch].setParams (Biquad::Type::LowPass, lp.hz, lp.q);
        }

        kinkIn  = OnePole::lowPass (kKinkInHz, sampleRate);
        kinkOut = OnePole::lowPass (kKinkOutHz, sampleRate);
        kinkDc  = OnePole::highPass (kDcBlockHz, sampleRate);
        envelope = OnePole::lowPass (kEnvelopeHz, sampleRate);
        dcBlock = OnePole::highPass (kDcBlockHz, sampleRate);

        smoothTube.reset (sampleRate, 0.2);     // 200 ms: the bias moves the operating point; faster glides thump
        smoothTube.setCurrentAndTargetValue (smoothTube.getTargetValue());

        reset();
    }

    void reset()
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            emphasis[ch].reset();
            deEmphasis[ch].reset();
            outputLowPass[ch].reset();
            kinkInState[ch] = kinkOutState[ch] = kinkDcState[ch] = envState[ch] = dcState[ch] = {};
        }
    }

    /// Tube Gain knob, 0-1.
    void setParams (float tubeGainKnob)
    {
        smoothTube.setTargetValue (juce::jlimit (0.0f, 1.0f, tubeGainKnob));
    }

    /// The static curve on its own (also used by the tests).
    static float curve (float u)
    {
        const float r = std::pow (std::abs (u) * kInvCeiling, kHardness);
        return u * std::pow (1.0f + r, -kInvHardness);
    }

    /// Slope of the curve at u.
    static float slope (float u)
    {
        const float r = std::pow (std::abs (u) * kInvCeiling, kHardness);
        return std::pow (1.0f + r, -kInvHardness - 1.0f);
    }

    void process (juce::AudioBuffer<float>& buffer)
    {
        const int numChannels = juce::jmin (2, buffer.getNumChannels());
        const int numSamples  = buffer.getNumSamples();

        for (int s = 0; s < numSamples; ++s)
        {
            const float t = smoothTube.getNextValue();
            const float level = juce::Decibels::decibelsToGain (atKnob (kLevelDb, t));
            const float kink  = juce::Decibels::decibelsToGain (atKnob (kKinkDb, t), -200.0f);
            const float drive = juce::Decibels::decibelsToGain (atKnob (kDriveDb, t));
            const float bias  = atKnob (kBias, t);
            const float block = atKnob (kBlocking, t);
            const float onset = atKnob (kBlockingOnset, t);
            const float norm  = 1.0f / (drive * slope (bias));

            for (int ch = 0; ch < numChannels; ++ch)
            {
                float* data = buffer.getWritePointer (ch);
                const float w = data[s] * level;

                // Kink: rectified copy of the signal through a low shelf (2nd harmonic, falls with frequency)
                const float rect = std::abs (kinkIn.process (w, kinkInState[ch]));
                const float shelf = kinkOut.process (rect, kinkOutState[ch]) + kKinkFlat * rect;
                const float v = w + kink * kinkDc.process (shelf, kinkDcState[ch]);

                const float u = emphasis[ch].processSample (v);
                const float env = envelope.process (std::abs (u), envState[ch]);
                const float op = bias + block * std::max (env - onset, 0.0f);
                const float c = curve (drive * u + op) - curve (op);

                const float y = outputLowPass[ch].processSample (deEmphasis[ch].processSample (c)) * norm;
                data[s] = dcBlock.process (y, dcState[ch]);
            }
        }
    }

private:
    /// Linear interpolation over the five knob positions 0 / 2.5 / 5 / 7.5 / 10.
    static float atKnob (const float (&v)[5], float knob)
    {
        const float x = juce::jlimit (0.0f, 1.0f, knob) * 4.0f;
        const int i = juce::jmin (3, static_cast<int> (x));
        return v[i] + (x - static_cast<float> (i)) * (v[i + 1] - v[i]);
    }

    /// First-order bilinear filter; the state is kept per channel by the caller. Double precision: with poles this
    /// close to 1 (5-20 Hz at 48 kHz) float rounding noise is amplified ~60 dB near DC.
    struct OnePole
    {
        double b0 = 1.0, b1 = 0.0, a1 = 0.0;
        struct State { double x1 = 0.0, y1 = 0.0; };

        static OnePole lowPass (float hz, double sr)
        {
            const double k = std::tan (juce::MathConstants<double>::pi * hz / sr);
            return { k / (1.0 + k), k / (1.0 + k), (k - 1.0) / (1.0 + k) };
        }

        static OnePole highPass (float hz, double sr)
        {
            const double k = std::tan (juce::MathConstants<double>::pi * hz / sr);
            return { 1.0 / (1.0 + k), -1.0 / (1.0 + k), (k - 1.0) / (1.0 + k) };
        }

        float process (float x, State& st) const
        {
            const double y = b0 * x + b1 * st.x1 - a1 * st.y1;
            st.x1 = x;
            st.y1 = y;
            return static_cast<float> (y);
        }
    };

    // Static curve u / (1 + |u / ceiling|^hardness)^(1 / hardness)
    static constexpr float kCeiling      = 1.6435f;
    static constexpr float kHardness     = 1.7781f;
    static constexpr float kInvCeiling   = 1.0f / kCeiling;
    static constexpr float kInvHardness  = 1.0f / kHardness;
    // Emphasis around the curve: wide peak before, the inverse after
    static constexpr float kEmphasisHz   = 872.07f;
    static constexpr float kEmphasisQ    = 0.3254f;
    static constexpr float kEmphasisDb   = 7.9087f;
    // Fixed output low-pass, fitted together with the tone-stack voicing (-4 dB at 10 kHz, -19 dB at 20 kHz at 48 kHz).
    // The top octave depends on the sample rate (at 88.2 kHz and up it is ~5 dB darker at 14-16 kHz),
    // so each rate family has its own fit to the change from 48 kHz.
    struct LowPass { float hz, q; };
    static constexpr LowPass kOutputLowPass48  { 14602.0f, 0.382f };
    static constexpr LowPass kOutputLowPass441 { 21098.2f, 0.05f };    // f near Nyquist: reads as the 48 kHz design
    static constexpr LowPass kOutputLowPass96  { 7691.6f, 0.5945f };   // 88.2 / 96 kHz
    static constexpr LowPass kOutputLowPass192 { 7129.9f, 0.6372f };   // 176.4 / 192 kHz

    static LowPass outputLowPassFor (double sampleRate)
    {
        if (sampleRate < 46000.0)  return kOutputLowPass441;
        if (sampleRate < 70000.0)  return kOutputLowPass48;
        if (sampleRate < 130000.0) return kOutputLowPass96;
        return kOutputLowPass192;
    }
    static constexpr float kDcBlockHz       = 10.0f;

    // Tube Gain, per knob position 0 / 2.5 / 5 / 7.5 / 10 (fitted on stepped tones)
    static constexpr float kLevelDb[5]       = { 3.1f, 3.1f, 2.9f, 1.5f, 0.0f };        // small-signal level
    static constexpr float kKinkDb[5]        = { -8.71f, -18.60f, -49.78f, -49.77f, -120.0f };
    static constexpr float kDriveDb[5]       = { 3.167f, 2.638f, 0.0f, 0.0f, 0.086f };
    static constexpr float kBias[5]          = { 0.0f, 0.0f, 0.00084f, 0.00080f, 0.00014f };   // static part: none at 0 / 2.5
    static constexpr float kBlocking[5]      = { 2.700f, 0.6683f, -0.01221f, -0.01204f, -0.00144f };
    static constexpr float kBlockingOnset[5] = { 0.01299f, 0.01131f, 0.00627f, 0.00536f, 0.0669f };
    // Kink shape (shared by all positions, fitted on the low-level H2 at 41 / 100 / 400 / 1600 Hz, within 2 dB):
    // low-pass before the rectifier, low shelf (pole kKinkOutHz, zero kKinkOutHz / kKinkFlat) after it
    static constexpr float kKinkInHz   = 703.4f;
    static constexpr float kKinkOutHz  = 20.0f;
    static constexpr float kKinkFlat   = 0.02011f;
    static constexpr float kEnvelopeHz = 5.0f;

    Biquad emphasis[2];
    Biquad deEmphasis[2];
    Biquad outputLowPass[2];
    OnePole kinkIn, kinkOut, kinkDc, envelope, dcBlock;
    OnePole::State kinkInState[2], kinkOutState[2], kinkDcState[2], envState[2], dcState[2];

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothTube { 1.0f };
};
