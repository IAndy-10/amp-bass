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
        glideLeft = 0;
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
        glideLeft = 0;
        updateCoefficients();
    }

    // Like setParams, but the coefficients move to the new values in a straight line over `samples` samples.
    // For a filter retuned while it runs (a knob being moved): a jump clicks, because the state still belongs
    // to the old filter, and retuning every N samples turns the clicks into a buzz at sampleRate / N (zipper
    // noise). Retuning every N samples with a glide of N gives a continuous path. Same values: no change.
    void glideTo(Type t, float hz, float q, float db, int samples) {
        const double from[5] = { b0, b1, b2, a1, a2 };
        setParams(t, hz, q, db);
        if (samples <= 1) { glideLeft = 0; return; }
        const double to[5] = { b0, b1, b2, a1, a2 };
        for (int i = 0; i < 5; ++i) {
            target[i] = to[i];
            step[i] = (to[i] - from[i]) / samples;
        }
        b0 = from[0]; b1 = from[1]; b2 = from[2]; a1 = from[3]; a2 = from[4];
        glideLeft = samples;
    }

    void reset() { s1 = 0.0; s2 = 0.0; }

    float processSample(float x) {
        const double y = b0 * x + s1;
        s1 = b1 * x - a1 * y + s2;
        s2 = b2 * x - a2 * y;
        // Denormal flush
        s1 += 1e-18; s1 -= 1e-18;
        s2 += 1e-18; s2 -= 1e-18;
        if (glideLeft > 0) advanceGlide();
        return static_cast<float>(y);
    }

private:
    double sr = 44100.0;
    Type type = Type::LowPass;
    float freq = 1000.0f;
    float Q = 0.707f;
    float gainDb = 0.0f;

    // Coefficients (normalized: a0 = 1). Double precision: in float, low-frequency sections fall into
    // limit cycles (a steady ~-90 dB pattern near 14 Hz on a 3 kHz tone).
    double b0 = 1.0, b1 = 0.0, b2 = 0.0;
    double a1 = 0.0, a2 = 0.0;

    // State (Direct Form II Transposed)
    double s1 = 0.0, s2 = 0.0;

    // Coefficient glide (glideTo)
    double target[5] = {}, step[5] = {};
    int glideLeft = 0;

    void advanceGlide() {
        if (--glideLeft == 0) {
            b0 = target[0]; b1 = target[1]; b2 = target[2]; a1 = target[3]; a2 = target[4];
            return;
        }
        b0 += step[0]; b1 += step[1]; b2 += step[2]; a1 += step[3]; a2 += step[4];
    }

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
        b0 = cb0 * invA0;
        b1 = cb1 * invA0;
        b2 = cb2 * invA0;
        a1 = ca1 * invA0;
        a2 = ca2 * invA0;
    }
};
