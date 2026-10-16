#pragma once

// Shared signal generation and measurement helpers for the DSP tests and the
// offline host. Not real-time code — allocations are fine here.

#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>
#include <functional>
#include <vector>

namespace TestSignals
{
    constexpr double kPi = 3.14159265358979323846;

    using ProcessFn = std::function<void (juce::AudioBuffer<float>&)>;

    inline double toDb (double linear) { return 20.0 * std::log10 (std::max (linear, 1e-12)); }

    // Renders `input` (stereo, identical channels) through `process` in blocks
    // of `blockSize`, mimicking how a host calls processBlock.
    inline juce::AudioBuffer<float> render (const std::vector<float>& input, int blockSize, const ProcessFn& process)
    {
        const int total = static_cast<int> (input.size());
        juce::AudioBuffer<float> out (2, total);
        juce::AudioBuffer<float> block (2, blockSize);

        for (int pos = 0; pos < total; pos += blockSize)
        {
            const int n = std::min (blockSize, total - pos);
            block.setSize (2, n, false, false, true);
            for (int ch = 0; ch < 2; ++ch)
                block.copyFrom (ch, 0, input.data() + pos, n);

            process (block);

            for (int ch = 0; ch < 2; ++ch)
                out.copyFrom (ch, pos, block, ch, 0, n);
        }
        return out;
    }

    inline std::vector<float> sine (double sr, double freq, float amp, double seconds)
    {
        std::vector<float> v (static_cast<size_t> (sr * seconds));
        for (size_t i = 0; i < v.size(); ++i)
            v[i] = amp * static_cast<float> (std::sin (2.0 * kPi * freq * static_cast<double> (i) / sr));
        return v;
    }

    // Sum of equal-amplitude sines; `ampEach` is the per-tone amplitude.
    inline std::vector<float> multitone (double sr, const std::vector<double>& freqs, float ampEach, double seconds)
    {
        std::vector<float> v (static_cast<size_t> (sr * seconds), 0.0f);
        for (auto f : freqs)
            for (size_t i = 0; i < v.size(); ++i)
                v[i] += ampEach * static_cast<float> (std::sin (2.0 * kPi * f * static_cast<double> (i) / sr));
        return v;
    }

    inline double rms (const float* x, int start, int end)
    {
        double acc = 0.0;
        for (int i = start; i < end; ++i)
            acc += static_cast<double> (x[i]) * x[i];
        return std::sqrt (acc / std::max (1, end - start));
    }

    inline double peak (const float* x, int start, int end)
    {
        double p = 0.0;
        for (int i = start; i < end; ++i)
            p = std::max (p, static_cast<double> (std::abs (x[i])));
        return p;
    }

    // Amplitude of a single frequency component (Hann-windowed Goertzel).
    inline double toneAmplitude (const float* x, int start, int end, double sr, double freq)
    {
        const int n = end - start;
        const double w = 2.0 * kPi * freq / sr;
        const double coeff = 2.0 * std::cos (w);
        double s1 = 0.0, s2 = 0.0, winSum = 0.0;

        for (int i = 0; i < n; ++i)
        {
            const double win = 0.5 - 0.5 * std::cos (2.0 * kPi * i / (n - 1));
            winSum += win;
            const double s0 = x[start + i] * win + coeff * s1 - s2;
            s2 = s1;
            s1 = s0;
        }
        const double re = s1 - s2 * std::cos (w);
        const double im = s2 * std::sin (w);
        return 2.0 * std::sqrt (re * re + im * im) / winSum;
    }

    // THD as a ratio (harmonics 2..10 over the fundamental).
    inline double thd (const float* x, int start, int end, double sr, double f0)
    {
        const double fund = toneAmplitude (x, start, end, sr, f0);
        double h = 0.0;
        for (int k = 2; k <= 10 && k * f0 < sr * 0.45; ++k)
        {
            const double a = toneAmplitude (x, start, end, sr, k * f0);
            h += a * a;
        }
        return std::sqrt (h) / std::max (fund, 1e-12);
    }

    inline bool allFinite (const juce::AudioBuffer<float>& b)
    {
        for (int ch = 0; ch < b.getNumChannels(); ++ch)
            for (int i = 0; i < b.getNumSamples(); ++i)
                if (! std::isfinite (b.getReadPointer (ch)[i]))
                    return false;
        return true;
    }
}
