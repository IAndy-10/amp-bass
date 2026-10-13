#pragma once
#include <cmath>

class OnePoleFilter {
public:
    enum class Type { LowPass, HighPass };

    void prepare(double sampleRate) { sr = sampleRate; updateCoefficient(); }
    void setType(Type t) { type = t; updateCoefficient(); }
    void setCutoff(float hz) { cutoffHz = hz; updateCoefficient(); }
    void reset() { z1 = 0.0f; }

    float processSample(float x) {
        float y;
        if (type == Type::LowPass) {
            y = x * b0 + z1 * a1;
            z1 = y;
        } else {
            float lp = x * b0 + z1 * a1;
            z1 = lp;
            y = x - lp;
        }
        // Denormal flush
        z1 += 1e-18f;
        z1 -= 1e-18f;
        return y;
    }

private:
    double sr = 44100.0;
    float cutoffHz = 1000.0f;
    Type type = Type::LowPass;
    float b0 = 1.0f, a1 = 0.0f;
    float z1 = 0.0f;

    void updateCoefficient() {
        if (sr <= 0.0) return;
        float w = static_cast<float>(2.0 * 3.14159265358979 * cutoffHz / sr);
        float g = w / (1.0f + w);
        b0 = g;
        a1 = 1.0f - g;
    }
};
