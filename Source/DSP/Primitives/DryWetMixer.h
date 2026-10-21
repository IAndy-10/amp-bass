#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>

class DryWetMixer {
public:
    void setMix(float mixAmount) { mix.setTargetValue(juce::jlimit(0.0f, 1.0f, mixAmount)); }

    void prepare(double sampleRate, int numChannels, int maxBlockSize) {
        dryBuffer.setSize(numChannels, maxBlockSize, false, true, false);
        mix.reset(sampleRate, kRampSeconds);
        mix.setCurrentAndTargetValue(mix.getTargetValue());
    }

    void saveDry(const juce::AudioBuffer<float>& input) {
        int channels = input.getNumChannels();
        int samples = input.getNumSamples();
        dryBuffer.setSize(channels, samples, false, false, true);
        for (int ch = 0; ch < channels; ++ch)
            dryBuffer.copyFrom(ch, 0, input, ch, 0, samples);
    }

    void mixToOutput(juce::AudioBuffer<float>& wetOutput) {
        int channels = juce::jmin(wetOutput.getNumChannels(), dryBuffer.getNumChannels());
        int samples = juce::jmin(wetOutput.getNumSamples(), dryBuffer.getNumSamples());

        // Fully wet and not moving: the output is the wet signal as it is.
        if (!mix.isSmoothing() && mix.getCurrentValue() >= 1.0f)
            return;

        // Equal-power crossfade; the mix glides over kRampSeconds so moving it doesn't click.
        for (int i = 0; i < samples; ++i) {
            const float m = mix.getNextValue();
            const float dryGain = std::cos(m * kHalfPi);
            const float wetGain = std::sin(m * kHalfPi);
            for (int ch = 0; ch < channels; ++ch) {
                auto* wet = wetOutput.getWritePointer(ch);
                wet[i] = dryBuffer.getSample(ch, i) * dryGain + wet[i] * wetGain;
            }
        }
    }

    /// The input saved by the last saveDry() call.
    const juce::AudioBuffer<float>& getDry() const { return dryBuffer; }

private:
    static constexpr float kHalfPi = 1.5707963267948966f;
    static constexpr double kRampSeconds = 0.02;

    juce::SmoothedValue<float> mix { 1.0f };
    juce::AudioBuffer<float> dryBuffer;
};
