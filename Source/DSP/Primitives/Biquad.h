#pragma once
#include <cmath>

// Direct Form II Transposed biquad filter.
// Real-time safe: no allocations after coefficient setup.
// Supports LowPass, HighPass, BandPass, Notch, LowShelf, HighShelf, Peak.
class Biquad {
public:
    enum class Type { LowPass, HighPass, BandPass, Notch, LowShelf, HighShelf, Peak };

    void prepare(double sampleRate) {
        sr = sampleRate;
        reset();
        updateCoefficients();
    }

    void setType(Type t) { type = t; updateCoefficients(); }
    void setFrequency(float hz) { freq = hz; updateCoefficients(); }
    void setQ(float q) { Q = q; updateCoefficients(); }
    void setGainDb(float db) { gainDb = db; updateCoefficients(); }

    void setParams(Type t, float hz, float q, float db = 0.0f) {
        type = t;
        freq = hz;
        Q = q;
        gainDb = db;
        updateCoefficients();
    }

    void reset() { s1 = 0.0f; s2 = 0.0f; }

    float processSample(float x) {
        float y = b0 * x + s1;
        s1 = b1 * x - a1 * y + s2;
        s2 = b2 * x - a2 * y;
        // Denormal flush
        s1 += 1e-18f; s1 -= 1e-18f;
        s2 += 1e-18f; s2 -= 1e-18f;
        return y;
    }

private:
    double sr = 44100.0;
    Type type = Type::LowPass;
    float freq = 1000.0f;
    float Q = 0.707f;
    float gainDb = 0.0f;

    // Coefficients (normalized: a0 = 1)
    float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
    float a1 = 0.0f, a2 = 0.0f;

    // State (Direct Form II Transposed)
    float s1 = 0.0f, s2 = 0.0f;

    void updateCoefficients() {
        if (sr <= 0.0 || freq <= 0.0f || Q <= 0.0f) return;

        const double pi = 3.14159265358979323846;
        double w0 = 2.0 * pi * static_cast<double>(freq) / sr;
        double cosW0 = std::cos(w0);
        double sinW0 = std::sin(w0);
        double alpha = sinW0 / (2.0 * static_cast<double>(Q));
        double A = std::pow(10.0, static_cast<double>(gainDb) / 40.0);

        double cb0, cb1, cb2, ca0, ca1, ca2;

        switch (type) {
            case Type::LowPass:
                cb0 = (1.0 - cosW0) / 2.0;
                cb1 = 1.0 - cosW0;
                cb2 = (1.0 - cosW0) / 2.0;
                ca0 = 1.0 + alpha;
                ca1 = -2.0 * cosW0;
                ca2 = 1.0 - alpha;
                break;

            case Type::HighPass:
                cb0 = (1.0 + cosW0) / 2.0;
                cb1 = -(1.0 + cosW0);
                cb2 = (1.0 + cosW0) / 2.0;
                ca0 = 1.0 + alpha;
                ca1 = -2.0 * cosW0;
                ca2 = 1.0 - alpha;
                break;

            case Type::BandPass:
                cb0 = alpha;
                cb1 = 0.0;
                cb2 = -alpha;
                ca0 = 1.0 + alpha;
                ca1 = -2.0 * cosW0;
                ca2 = 1.0 - alpha;
                break;

            case Type::Notch:
                cb0 = 1.0;
                cb1 = -2.0 * cosW0;
                cb2 = 1.0;
                ca0 = 1.0 + alpha;
                ca1 = -2.0 * cosW0;
                ca2 = 1.0 - alpha;
                break;

            case Type::LowShelf: {
                double sqrtA = std::sqrt(A);
                double twoSqrtAAlpha = 2.0 * sqrtA * alpha;
                cb0 = A * ((A + 1.0) - (A - 1.0) * cosW0 + twoSqrtAAlpha);
                cb1 = 2.0 * A * ((A - 1.0) - (A + 1.0) * cosW0);
                cb2 = A * ((A + 1.0) - (A - 1.0) * cosW0 - twoSqrtAAlpha);
                ca0 = (A + 1.0) + (A - 1.0) * cosW0 + twoSqrtAAlpha;
                ca1 = -2.0 * ((A - 1.0) + (A + 1.0) * cosW0);
                ca2 = (A + 1.0) + (A - 1.0) * cosW0 - twoSqrtAAlpha;
                break;
            }

            case Type::HighShelf: {
                double sqrtA = std::sqrt(A);
                double twoSqrtAAlpha = 2.0 * sqrtA * alpha;
                cb0 = A * ((A + 1.0) + (A - 1.0) * cosW0 + twoSqrtAAlpha);
                cb1 = -2.0 * A * ((A - 1.0) + (A + 1.0) * cosW0);
                cb2 = A * ((A + 1.0) + (A - 1.0) * cosW0 - twoSqrtAAlpha);
                ca0 = (A + 1.0) - (A - 1.0) * cosW0 + twoSqrtAAlpha;
                ca1 = 2.0 * ((A - 1.0) - (A + 1.0) * cosW0);
                ca2 = (A + 1.0) - (A - 1.0) * cosW0 - twoSqrtAAlpha;
                break;
            }

            case Type::Peak:
                cb0 = 1.0 + alpha * A;
                cb1 = -2.0 * cosW0;
                cb2 = 1.0 - alpha * A;
                ca0 = 1.0 + alpha / A;
                ca1 = -2.0 * cosW0;
                ca2 = 1.0 - alpha / A;
                break;
        }

        // Normalize by a0
        double invA0 = 1.0 / ca0;
        b0 = static_cast<float>(cb0 * invA0);
        b1 = static_cast<float>(cb1 * invA0);
        b2 = static_cast<float>(cb2 * invA0);
        a1 = static_cast<float>(ca1 * invA0);
        a2 = static_cast<float>(ca2 * invA0);
    }
};
