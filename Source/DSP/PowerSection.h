#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <algorithm>
#include <cmath>
#include "Primitives/Biquad.h"

/// Stage 7: power amp, fitted to the reference plugin with its cabinet bypassed.
///
/// The reference's main odd-harmonic distortion sits after Master and depends only on how loud the
/// signal would be there, not on how it got there. It is a soft static curve whose 3rd harmonic grows
/// about 1.7 dB per dB, and it distorts more at higher frequencies (as a tube power amp with negative
/// feedback does), most in the low mids. Modelled as: wide emphasis peak -> static curve -> inverse
/// peak, then the fixed output low-pass. Below about -30 dBFS it is linear and the two peaks cancel.
class PowerSection
{
public:
    void prepare (double sampleRate, int /*maxBlockSize*/)
    {
        // Kept below Nyquist at low rates.
        const float lpHz = std::min (kOutputLowPassHz, 0.45f * static_cast<float> (sampleRate));
        for (int ch = 0; ch < 2; ++ch)
        {
            emphasis[ch].prepare (sampleRate);
            emphasis[ch].setParams (Biquad::Type::Peak, kEmphasisHz, kEmphasisQ, kEmphasisDb);
            deEmphasis[ch].prepare (sampleRate);
            deEmphasis[ch].setParams (Biquad::Type::Peak, kEmphasisHz, kEmphasisQ, -kEmphasisDb);
            outputLowPass[ch].prepare (sampleRate);
            outputLowPass[ch].setParams (Biquad::Type::LowPass, lpHz, kOutputLowPassQ);
        }
        reset();
    }

    void reset()
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            emphasis[ch].reset();
            deEmphasis[ch].reset();
            outputLowPass[ch].reset();
        }
    }

    /// The static curve on its own (also used by the tests).
    static float curve (float u)
    {
        const float r = std::pow (std::abs (u) * kInvCeiling, kHardness);
        return u * std::pow (1.0f + r, -kInvHardness);
    }

    void process (juce::AudioBuffer<float>& buffer)
    {
        const int numChannels = juce::jmin (2, buffer.getNumChannels());
        const int numSamples  = buffer.getNumSamples();

        for (int ch = 0; ch < numChannels; ++ch)
        {
            float* data = buffer.getWritePointer (ch);
            for (int s = 0; s < numSamples; ++s)
            {
                const float c = curve (emphasis[ch].processSample (data[s]));
                data[s] = outputLowPass[ch].processSample (deEmphasis[ch].processSample (c));
            }
        }
    }

private:
    // Static curve u / (1 + |u / ceiling|^hardness)^(1 / hardness)
    static constexpr float kCeiling      = 1.6435f;
    static constexpr float kHardness     = 1.7781f;
    static constexpr float kInvCeiling   = 1.0f / kCeiling;
    static constexpr float kInvHardness  = 1.0f / kHardness;
    // Emphasis around the curve: wide peak before, the inverse after
    static constexpr float kEmphasisHz   = 872.07f;
    static constexpr float kEmphasisQ    = 0.3254f;
    static constexpr float kEmphasisDb   = 7.9087f;
    // Fixed output low-pass, fitted together with the tone-stack voicing (-4 dB at 10 kHz, -19 dB at 20 kHz)
    static constexpr float kOutputLowPassHz = 14602.0f;
    static constexpr float kOutputLowPassQ  = 0.382f;

    Biquad emphasis[2];
    Biquad deEmphasis[2];
    Biquad outputLowPass[2];
};
