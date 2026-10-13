#pragma once
#include <cmath>

template <typename T = float>
class ExponentialSmoother {
public:
    void prepare(double sampleRate, double smoothingMs) {
        coeff = static_cast<T>(std::exp(-1.0 / (sampleRate * smoothingMs * 0.001)));
        current = target;
    }
    void setTarget(T val) { target = val; }
    void setCurrentAndTarget(T val) { current = target = val; }
    T getNext() {
        current = coeff * current + (T(1) - coeff) * target;
        return current;
    }
    T get() const { return current; }
    bool isSmoothing() const { return std::abs(current - target) > T(0.00001); }

private:
    T current = T(0), target = T(0), coeff = T(0);
};
