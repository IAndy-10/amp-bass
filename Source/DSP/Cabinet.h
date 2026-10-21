#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include "Primitives/Biquad.h"
#include <cmath>

/// Stage 8: speaker cabinet, a 4x10 miked with a dynamic and a ribbon microphone, and its horn.
/// The cabinet is a fixed filter (its effect is the same at every level, drive and setting, within
/// 0.3 dB), so it is a cascade of biquads fitted to its magnitude response: a 4x10 with a broad dip around 400 Hz,
/// a lift at 3-5 kHz and a steep roll-off above 10 kHz. The horn adds 3-7 dB between 3 and 9 kHz and takes ~1.5 dB
/// off below 1 kHz. Like the amp's top end, the cabinet changes with the sample rate above ~5 kHz, so
/// each rate family has its own fit.
/// Switching the cabinet or the horn crossfades linearly over 50 ms (the paths are correlated, so no level dip).
class Cabinet
{
public:
    void prepare (double sampleRate, int /*maxBlockSize*/)
    {
        const Voicing& v = voicingFor (sampleRate);
        const double maxHz = 0.45 * sampleRate;
        for (int ch = 0; ch < 2; ++ch)
        {
            for (int i = 0; i < kCabStages; ++i)
            {
                cab[i][ch].prepare (sampleRate);
                cab[i][ch].setParams (v.cab[i].type, static_cast<float> (juce::jmin<double> (v.cab[i].hz, maxHz)), v.cab[i].q, v.cab[i].db);
            }
            for (int i = 0; i < kHornStages; ++i)
            {
                horn[i][ch].prepare (sampleRate);
                horn[i][ch].setParams (v.horn[i].type, static_cast<float> (juce::jmin<double> (v.horn[i].hz, maxHz)), v.horn[i].q, v.horn[i].db);
            }
        }
        cabGain  = juce::Decibels::decibelsToGain (v.cabGainDb);
        hornGain = juce::Decibels::decibelsToGain (v.hornGainDb);

        cabMix.reset (sampleRate, kRampSeconds);
        hornMix.reset (sampleRate, kRampSeconds);
        cabMix.setCurrentAndTargetValue (cabOn ? 1.0f : 0.0f);
        hornMix.setCurrentAndTargetValue (hornOn ? 1.0f : 0.0f);
    }

    void reset()
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            for (auto& f : cab)  f[ch].reset();
            for (auto& f : horn) f[ch].reset();
        }
    }

    void setParams (bool cabinetOn, bool hornEnabled)
    {
        cabOn = cabinetOn;
        hornOn = hornEnabled;
        cabMix.setTargetValue (cabOn ? 1.0f : 0.0f);
        hornMix.setTargetValue (hornOn ? 1.0f : 0.0f);
    }

    void process (juce::AudioBuffer<float>& buffer)
    {
        const int numChannels = juce::jmin (2, buffer.getNumChannels());
        const int numSamples  = buffer.getNumSamples();

        // The filters always run so switching never starts them from silence; fully off costs nothing else.
        for (int s = 0; s < numSamples; ++s)
        {
            const float c = cabMix.getNextValue();
            const float h = hornMix.getNextValue();

            for (int ch = 0; ch < numChannels; ++ch)
            {
                float* data = buffer.getWritePointer (ch);
                const float dry = data[s];

                float speaker = dry;
                for (auto& f : cab)
                    speaker = f[ch].processSample (speaker);
                speaker *= cabGain;

                float withHorn = speaker;
                for (auto& f : horn)
                    withHorn = f[ch].processSample (withHorn);
                withHorn *= hornGain;

                const float wet = speaker + h * (withHorn - speaker);
                data[s] = dry + c * (wet - dry);
            }
        }
    }

private:
    static constexpr int kCabStages = 24, kHornStages = 16;
    static constexpr double kRampSeconds = 0.05;

    using T = Biquad::Type;
    struct Stage { T type; float hz, q, db; };
    struct Voicing
    {
        float cabGainDb;
        Stage cab[kCabStages];
        float hornGainDb;   // horn: applied on top of the cabinet
        Stage horn[kHornStages];
    };

    // Fitted per rate at 1/24-octave resolution (cabinet effect = cabinet on minus off;
    // horn effect = horn on minus off), 25 Hz-20 kHz. The narrow dips of its two-microphone mix (-8 dB at 2.4 kHz, -14 dB
    // at 4 kHz with the horn) need the narrow peaks. After 1/6-octave averaging, within 0.4 dB (cabinet) and 1.1 dB
    // (horn) to 12 kHz. Individual stages trade large opposite gains; only the
    // whole cascade is meaningful.
    static constexpr Voicing k441 {
        -13.38f,
        { { T::HighPass,     17.4f, 1.208f, -27.97f },
          { T::LowPass,    9565.6f, 5.027f,  29.82f },
          { T::Peak,       8237.6f, 2.882f, -25.30f },
          { T::Peak,       2390.7f, 2.270f, -29.55f },
          { T::Peak,        138.6f, 0.289f,  10.15f },
          { T::Peak,      10412.9f, 0.210f,  28.10f },
          { T::Peak,       2211.7f, 4.519f,  18.85f },
          { T::Peak,       3332.5f, 4.671f, -10.99f },
          { T::Peak,       1277.1f, 1.403f,   9.63f },
          { T::Peak,       4627.5f, 4.360f,  30.00f },
          { T::Peak,       5721.4f, 12.000f,  15.00f },
          { T::Peak,       5090.9f, 9.707f,  -9.00f },
          { T::Peak,      16539.4f, 9.161f,  30.00f },
          { T::Peak,       3822.4f, 2.398f,  29.43f },
          { T::Peak,       9606.5f, 9.112f, -10.99f },
          { T::Peak,      15998.6f, 0.466f, -29.01f },
          { T::Peak,       5682.3f, 6.247f, -16.05f },
          { T::Peak,       7304.3f, 12.000f,  -2.67f },
          { T::Peak,       2958.3f, 5.257f,   8.18f },
          { T::Peak,       7577.0f, 12.000f,   5.47f },
          { T::Peak,       4197.0f, 8.323f, -12.27f },
          { T::Peak,       3839.3f, 12.000f, -19.39f },
          { T::Peak,        105.0f, 0.783f,  10.74f },
          { T::Peak,       4616.1f, 9.924f, -23.12f } },
        -1.57f,
        { { T::Peak,       6628.2f, 2.090f,  12.67f },
          { T::Peak,       4105.4f, 12.000f, -16.48f },
          { T::Peak,       4957.0f, 3.276f,   6.60f },
          { T::Peak,       5486.5f, 11.999f, -20.34f },
          { T::Peak,       6648.4f, 12.000f, -24.53f },
          { T::Peak,       3328.0f, 9.866f,   7.59f },
          { T::Peak,      10936.9f, 8.151f, -16.84f },
          { T::Peak,      16765.9f, 1.070f,  -6.74f },
          { T::Peak,      13718.7f, 11.992f,  -8.61f },
          { T::Peak,      11506.6f, 1.260f,  15.52f },
          { T::Peak,      12246.5f, 11.996f, -19.92f },
          { T::Peak,       9897.3f, 12.000f, -11.77f },
          { T::Peak,       1973.8f, 8.855f,   2.10f },
          { T::Peak,       2316.2f, 12.000f,   2.50f },
          { T::Peak,      18543.4f, 8.629f,  28.46f },
          { T::Peak,       1209.4f, 7.009f,   1.92f } }
    };
    static constexpr Voicing k48 {
        -21.94f,
        { { T::HighPass,     15.0f, 1.223f,  29.97f },
          { T::LowPass,   10720.4f, 11.451f, -28.43f },
          { T::Peak,       8373.2f, 3.186f, -15.16f },
          { T::Peak,       2393.3f, 2.239f, -30.00f },
          { T::Peak,        110.8f, 0.291f,  29.12f },
          { T::Peak,       4531.9f, 0.383f,  27.94f },
          { T::Peak,       2213.1f, 4.392f,  18.93f },
          { T::Peak,       3348.1f, 3.886f, -12.91f },
          { T::Peak,       1278.8f, 1.497f,   9.59f },
          { T::Peak,       4849.2f, 12.000f,   7.85f },
          { T::Peak,       5806.5f, 12.000f,   3.88f },
          { T::Peak,      17533.3f, 12.000f,  19.16f },
          { T::Peak,       5001.4f, 3.772f, -14.36f },
          { T::Peak,      10399.0f, 2.931f, -24.62f },
          { T::Peak,       3806.8f, 1.848f,  29.24f },
          { T::Peak,      16207.6f, 7.757f,  -6.87f },
          { T::Peak,       7735.7f, 12.000f,   7.54f },
          { T::Peak,       9154.2f, 6.133f,  10.66f },
          { T::Peak,       6905.6f, 12.000f,   2.40f },
          { T::Peak,      10292.5f, 12.000f,   9.92f },
          { T::Peak,       2966.9f, 5.236f,   8.78f },
          { T::Peak,       3849.8f, 12.000f, -15.00f },
          { T::Peak,       4193.5f, 12.000f,  -8.36f },
          { T::Peak,      11527.4f, 2.212f,   6.13f } },
        -1.54f,
        { { T::Peak,      10129.5f, 1.750f,  20.08f },
          { T::Peak,       4120.4f, 11.181f, -23.49f },
          { T::Peak,       5644.3f, 1.425f,  14.11f },
          { T::Peak,       5546.9f, 12.000f, -28.52f },
          { T::Peak,       6707.8f, 12.000f, -19.48f },
          { T::Peak,       3481.4f, 2.570f,   9.12f },
          { T::Peak,      10129.8f, 8.678f, -26.38f },
          { T::Peak,      19754.4f, 2.950f,  30.00f },
          { T::Peak,       2926.6f, 7.258f,  -5.90f },
          { T::Peak,      11186.4f, 11.914f, -13.79f },
          { T::Peak,      18843.7f, 0.955f, -14.54f },
          { T::Peak,       3684.0f, 10.802f,  -8.34f },
          { T::Peak,      12664.8f, 12.000f, -11.19f },
          { T::Peak,       2407.2f, 11.130f, -17.95f },
          { T::Peak,       2379.7f, 12.000f,  17.03f },
          { T::Peak,       9009.4f, 12.000f,  -4.28f } }
    };
    static constexpr Voicing k96 {
        -10.41f,
        { { T::HighPass,     15.0f, 0.923f, -28.42f },
          { T::LowPass,    3006.2f, 8.308f, -29.93f },
          { T::Peak,      11606.4f, 0.558f,  30.00f },
          { T::Peak,       2402.6f, 2.220f, -30.00f },
          { T::Peak,       9322.9f, 3.010f, -18.83f },
          { T::Peak,        158.4f, 0.341f,   6.02f },
          { T::Peak,       3356.0f, 1.562f, -26.99f },
          { T::Peak,       2217.1f, 3.964f,  20.84f },
          { T::Peak,       9866.3f, 1.298f,  30.00f },
          { T::Peak,       1289.0f, 1.126f,  13.25f },
          { T::Peak,       4946.9f, 12.000f,  15.18f },
          { T::Peak,       4980.2f, 5.088f, -15.65f },
          { T::Peak,      10879.6f, 7.489f,  -8.85f },
          { T::Peak,       3788.9f, 3.045f,  16.41f },
          { T::Peak,       8243.7f, 9.479f,  11.41f },
          { T::Peak,       5997.1f, 12.000f,   5.96f },
          { T::Peak,       8627.2f, 2.395f, -17.21f },
          { T::Peak,       7271.1f, 11.993f,   4.14f },
          { T::Peak,       4297.1f, 12.000f, -18.78f },
          { T::Peak,       3919.3f, 12.000f, -10.39f },
          { T::Peak,        104.4f, 0.728f,  12.27f },
          { T::Peak,       4334.7f, 2.394f,  29.85f },
          { T::Peak,      15728.1f, 10.936f,  -1.31f },
          { T::Peak,       6611.0f, 11.972f,   2.66f } },
        -1.58f,
        { { T::Peak,      10317.4f, 0.997f,  19.41f },
          { T::Peak,       4168.4f, 9.237f, -24.52f },
          { T::Peak,       5761.6f, 1.381f,  11.63f },
          { T::Peak,       5719.4f, 12.000f, -30.00f },
          { T::Peak,       7044.7f, 12.000f, -23.62f },
          { T::Peak,      11331.4f, 11.373f, -20.85f },
          { T::Peak,       3604.9f, 3.160f,   9.51f },
          { T::Peak,      15270.6f, 12.000f, -16.96f },
          { T::Peak,       2937.3f, 8.588f,  -5.14f },
          { T::Peak,      12906.3f, 10.495f, -16.09f },
          { T::Peak,      18196.6f, 12.000f, -11.17f },
          { T::Peak,       9805.2f, 6.029f, -10.81f },
          { T::Peak,       8310.5f, 12.000f,  -5.78f },
          { T::Peak,       3715.7f, 12.000f,  -9.32f },
          { T::Peak,       2401.2f, 9.720f, -18.26f },
          { T::Peak,       2374.8f, 12.000f,  18.38f } }
    };   // 88.2 kHz and up

    static const Voicing& voicingFor (double sampleRate)
    {
        if (sampleRate < 46000.0) return k441;
        if (sampleRate < 70000.0) return k48;
        return k96;
    }

    Biquad cab[kCabStages][2], horn[kHornStages][2];
    float cabGain = 1.0f, hornGain = 1.0f;
    bool cabOn = false, hornOn = false;   // the processor sets them before prepare(), so playback starts without a fade
    juce::LinearSmoothedValue<float> cabMix, hornMix;
};
