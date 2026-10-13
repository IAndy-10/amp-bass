#pragma once
#include <vector>
#include <algorithm>

// Fixed-capacity circular delay buffer with linear interpolation.
// Real-time safe: no allocations after prepare().
class DelayLine {
public:
    void prepare(int maxDelaySamples) {
        size = maxDelaySamples + 2;
        buffer.assign(static_cast<size_t>(size), 0.0f);
        writePos = 0;
    }

    void write(float sample) {
        buffer[static_cast<size_t>(writePos)] = sample;
        writePos = (writePos + 1) % size;
    }

    float read(int delaySamples) const {
        int idx = (writePos - delaySamples - 1 + size) % size;
        return buffer[static_cast<size_t>(idx)];
    }

    float readInterpolated(float delaySamples) const {
        int d0 = static_cast<int>(delaySamples);
        float frac = delaySamples - static_cast<float>(d0);
        float a = read(d0);
        float b = read(d0 + 1);
        return a + frac * (b - a);
    }

    void clear() { std::fill(buffer.begin(), buffer.end(), 0.0f); }
    int getMaxDelaySamples() const { return size - 2; }

private:
    std::vector<float> buffer;
    int writePos = 0;
    int size = 1;
};
