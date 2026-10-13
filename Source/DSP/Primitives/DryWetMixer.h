#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>

class DryWetMixer {
public:
    void setMix(float mixAmount) { mix = mixAmount; }

    void prepare(int numChannels, int maxBlockSize) {
        dryBuffer.setSize(numChannels, maxBlockSize, false, true, false);
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

        // Equal-power crossfade
        const float halfPi = 1.5707963267948966f;
        float dryGain = std::cos(mix * halfPi);
        float wetGain = std::sin(mix * halfPi);

        for (int ch = 0; ch < channels; ++ch) {
            auto* wet = wetOutput.getWritePointer(ch);
            auto* dry = dryBuffer.getReadPointer(ch);
            for (int i = 0; i < samples; ++i)
                wet[i] = dry[i] * dryGain + wet[i] * wetGain;
        }
    }

private:
    float mix = 0.5f;
    juce::AudioBuffer<float> dryBuffer;
};
