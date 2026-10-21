#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <algorithm>
#include <cmath>

/// FX Rack Noise Gate. It listens to the plugin input after the Input knob (the pad and the
/// Hi/Lo input don't move it) and turns the amp's output down by Range dB while that input is below the threshold
/// (Range 100 = muted). The detector band-limits the input (first-order high-pass at 348 Hz and low-pass at 3.9 kHz,
/// so bass fundamentals need more level to open it: +11 dB at 100 Hz, +19 dB at 41 Hz) and follows |x| with a 32 ms
/// one-pole, the same up and down, scaled so a sine reads its peak level. The gate opens and closes at the same level
/// (no hysteresis) and the gain moves with a 1.6 ms one-pole. Each channel has its own detector.
/// Checked on level steps, slow ramps, a two-tone and decaying notes; off, or open, the signal
/// passes bit for bit.
class NoiseGate
{
public:
    void prepare (double sampleRate, int /*maxBlockSize*/)
    {
        const auto hzCoeff  = [sampleRate] (double hz) { return 1.0 - std::exp (-juce::MathConstants<double>::twoPi * hz / sampleRate); };
        const auto tauCoeff = [sampleRate] (double s)  { return 1.0 - std::exp (-1.0 / (s * sampleRate)); };
        aHighPass = hzCoeff (kHighPassHz);
        aLowPass  = hzCoeff (kLowPassHz);
        aEnvelope = tauCoeff (kEnvelopeSeconds);
        aGain     = tauCoeff (kGainSeconds);

        thresholdDb.reset (sampleRate, kRampSeconds);
        thresholdDb.setCurrentAndTargetValue (targetThresholdDb);
        rangeDb.reset (sampleRate, kRampSeconds);
        rangeDb.setCurrentAndTargetValue (targetRangeDb);
        updateLevels();
        reset();
    }

    void reset()
    {
        for (auto& st : state)
            st = { 0.0, 0.0, 0.0, on ? floorGain : 1.0 };
    }

    /// `thresholdDb`: -120 ... 0 dB; `rangeDb`: 0 ... 100 dB (100 = muted); `inputDb`: the Input knob, which the
    /// detector hears (the threshold is compared with the input after it).
    void setParams (bool enabled, float threshold, float range, float inputDb)
    {
        on = enabled;
        targetThresholdDb = juce::jlimit (-120.0f, 0.0f, threshold) - juce::jlimit (-20.0f, 20.0f, inputDb);
        targetRangeDb = juce::jlimit (0.0f, 100.0f, range);
        thresholdDb.setTargetValue (targetThresholdDb);
        rangeDb.setTargetValue (targetRangeDb);
    }

    /// `buffer`: the amp output to turn down. `detect`: this sub-block's plugin input, before any stage.
    void process (juce::AudioBuffer<float>& buffer, const juce::AudioBuffer<float>& detect)
    {
        const int numChannels = juce::jmin (2, buffer.getNumChannels(), detect.getNumChannels());
        const int numSamples  = juce::jmin (buffer.getNumSamples(), detect.getNumSamples());

        static constexpr int kLevelUpdateInterval = 32;

        for (int s = 0; s < numSamples; ++s)
        {
            thresholdDb.getNextValue();
            rangeDb.getNextValue();
            if (s % kLevelUpdateInterval == 0)
                updateLevels();

            for (int ch = 0; ch < numChannels; ++ch)
            {
                auto& st = state[ch];
                const double x = detect.getReadPointer (ch)[s];

                st.highPassLow += aHighPass * (x - st.highPassLow);           // high-pass = x - its low-pass
                st.lowPass     += aLowPass * ((x - st.highPassLow) - st.lowPass);
                st.envelope    += aEnvelope * (std::abs (st.lowPass) - st.envelope);
                st.highPassLow += 1e-18; st.highPassLow -= 1e-18;              // denormal flush
                st.lowPass     += 1e-18; st.lowPass     -= 1e-18;
                st.envelope    += 1e-18; st.envelope    -= 1e-18;

                const double target = on && st.envelope * kDetectorScale < thresholdGain ? floorGain : 1.0;
                st.gain += aGain * (target - st.gain);
                if (std::abs (st.gain - target) < kSettled)
                    st.gain = target;

                if (st.gain != 1.0)
                    buffer.getWritePointer (ch)[s] *= static_cast<float> (st.gain);
            }
        }
    }

private:
    static constexpr double kHighPassHz = 347.6, kLowPassHz = 3884.0;   // fitted (sidechain within 0.33 dB, 25 Hz-20 kHz)
    static constexpr double kEnvelopeSeconds = 0.032;                  // fitted to 12 level steps (rms error 0.4 ms)
    static constexpr double kGainSeconds = 0.0016;
    // pi / 2 turns the mean of |sine| into its peak; the 0.2 dB calibrates the digital filters' passband loss
    static constexpr double kDetectorScale = 1.5707963267948966 * 1.0232929922807541;
    static constexpr double kSettled = 1e-6;                           // -120 dB from the target
    static constexpr double kRampSeconds = 0.05;

    struct State { double highPassLow, lowPass, envelope, gain; };

    void updateLevels()
    {
        thresholdGain = std::pow (10.0, thresholdDb.getCurrentValue() / 20.0);
        const float range = rangeDb.getCurrentValue();
        floorGain = range >= 100.0f ? 0.0 : std::pow (10.0, -range / 20.0);
    }

    double aHighPass = 0.0, aLowPass = 0.0, aEnvelope = 0.0, aGain = 1.0;
    double thresholdGain = 0.0, floorGain = 1.0;
    State state[2] {};
    bool on = false;
    float targetThresholdDb = -88.0f, targetRangeDb = 50.0f;
    juce::LinearSmoothedValue<float> thresholdDb, rangeDb;
};
