#ifndef ACIDUS_FILTER_HPP
#define ACIDUS_FILTER_HPP

#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace acidus {

// Single one-pole state, used for the small coupling networks around the ladder
// (input DC-block, output bandwidth limit, resonance feedback tilt).
class OnePole {
public:
    void reset() { state_ = 0.0f; }

    // One-pole lowpass: y[n] = y[n-1] + a*(x[n]-y[n-1])
    inline float lowpass(float x, float a) {
        state_ += a * (x - state_);
        return state_;
    }

    // One-pole highpass built from the same lowpass state (x - LP(x))
    inline float highpass(float x, float a) {
        state_ += a * (x - state_);
        return x - state_;
    }

    float getState() const { return state_; }
    void setState(float s) { state_ = s; }

private:
    float state_{0.0f};
};

class Filter {
public:
    Filter();
    ~Filter() = default;

    void setSampleRate(double sampleRate);
    void reset();

    // 8x oversampled coupled diode ladder filter (implicit trapezoidal solve via Newton-Raphson)
    float processSample(float input, float cutoffHz, float resonance);

    // Tunables exposed as CLAP parameters for experimentation.
    void setResCouplingHz(float hz) { resCouplingHz_ = hz; }           // plausible range 100-250 Hz
    void setFeedbackGainCeiling(float k) { feedbackGainCeiling_ = k; } // plausible range 12-17

private:
    double sampleRate_{44100.0};
    double oversampledRate_{352800.0};

    float fLadderV1_{0.0f};
    float fLadderV2_{0.0f};
    float fLadderV3_{0.0f};
    float fLadderV4_{0.0f};

    float fHpFbStateX1_{0.0f};
    float fHpFbStateY1_{0.0f};
    float prevInput_{0.0f};

    OnePole inputCoupling_;   // ~20 Hz HPF ahead of the ladder (input DC-block cap)
    OnePole outputCoupling_;  // ~20 kHz LPF after the ladder (stray/buffer bandwidth)

    const float capScale1_{1.0000f};
    const float capScale2_{1.0000f};
    const float capScale3_{1.0000f};
    const float capScale4_{1.0000f};

    static constexpr float kLadderCriticalGain_ = 17.0f;
    static constexpr float kResonanceGainMargin_ = 0.90f;
    static constexpr float kCutoffHeadroomNumerator_ = 6600.0f;

    static inline float skewResonance(float resNorm) {
        return (1.0f - std::exp(-3.0f * resNorm)) / (1.0f - std::exp(-3.0f));
    }

    static constexpr float kMaxResonanceOutputGain_ = 2.3f;
    static constexpr float kCutoffToOmegaScale_ = 0.70710678f; // 1/sqrt(2)

    float resCouplingHz_{150.0f};
    float feedbackGainCeiling_{kLadderCriticalGain_ * kResonanceGainMargin_};
};

} // namespace acidus

#endif // ACIDUS_FILTER_HPP
