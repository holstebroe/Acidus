#ifndef ACIDUS_FILTER_HPP
#define ACIDUS_FILTER_HPP

#include <algorithm>
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

// First-order allpass, used for the fixed low-frequency phase-shaping
// coupling pole that sits outside the resonance feedback loop (Open303's
// ~14 Hz "allpass" stage; TB303_RESEARCH_COMPENDIUM.md §16).
class OnePoleAllpass {
public:
    void reset() { x1_ = y1_ = 0.0f; }

    // a = (tan(pi*fc/fs) - 1) / (tan(pi*fc/fs) + 1)
    inline float process(float x, float a) {
        float y = a * (x - y1_) + x1_;
        x1_ = x;
        y1_ = y;
        return y;
    }

private:
    float x1_{0.0f};
    float y1_{0.0f};
};

// RBJ-cookbook biquad notch, used for the fixed low-frequency coupling-
// network notch that sits outside the resonance feedback loop (Open303's
// ~7.5 Hz, bandwidth ~4.7 "notch" stage; TB303_RESEARCH_COMPENDIUM.md §16).
// Recomputes its own coefficients each call -- freq/bandwidth are runtime
// (host-automatable) parameters, and this runs once per host-rate sample,
// not per oversampled tick, so the extra trig cost is negligible.
class NotchFilter {
public:
    void reset() { x1_ = x2_ = y1_ = y2_ = 0.0f; }

    float process(float x, double freqHz, double bandwidthHz, double sampleRate) {
        double w0 = 2.0 * M_PI * freqHz / sampleRate;
        double q = freqHz / std::max(0.1, bandwidthHz);
        double alpha = std::sin(w0) / (2.0 * q);
        double cosw0 = std::cos(w0);
        double a0 = 1.0 + alpha;
        double b0 = 1.0 / a0, b1 = -2.0 * cosw0 / a0, b2 = 1.0 / a0;
        double a1 = -2.0 * cosw0 / a0, a2 = (1.0 - alpha) / a0;

        double y = b0 * x + b1 * x1_ + b2 * x2_ - a1 * y1_ - a2 * y2_;
        x2_ = x1_;
        x1_ = x;
        y2_ = y1_;
        y1_ = static_cast<float>(y);
        return y1_;
    }

private:
    float x1_{0.0f};
    float x2_{0.0f};
    float y1_{0.0f};
    float y2_{0.0f};
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
    void setPostFilterHpHz(float hz) { postFilterHpHz_ = hz; }         // plausible range 15-35 Hz
    void setNotchFreqHz(float hz) { notchFreqHz_ = hz; }               // plausible range 4-15 Hz
    void setNotchBandwidthHz(float hz) { notchBandwidthHz_ = hz; }     // plausible range 2-10 Hz
    void setAllpassFreqHz(float hz) { allpassFreqHz_ = hz; }           // plausible range 8-25 Hz
    void setInputCouplingHz(float hz) { inputCouplingHz_ = hz; }       // plausible range 10-30 Hz
    void setOutputCouplingHz(float hz) { outputCouplingHz_ = hz; }     // plausible range 10-25 kHz

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

    // Further coupling-network poles the research compendium documents as
    // sitting outside the resonance feedback loop, in series with the
    // signal (TB303_RESEARCH_COMPENDIUM.md §6/§16: "six further poles",
    // templated on Open303's fixed post-filter highpass/notch/allpass).
    // Run once per host-rate sample after the oversampled ladder solve,
    // since their time constants (tens of ms) are slow relative to a
    // sample either way.
    OnePole postFilterHp_;
    NotchFilter notch_;
    OnePoleAllpass allpass_;

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

    // Defaults cross-checked against Open303's shipped, working values for
    // its equivalent (fixed, non-resonance-swept) stages -- see
    // TB303_RESEARCH_COMPENDIUM.md §16.
    float postFilterHpHz_{24.167f};
    float notchFreqHz_{7.5164f};
    float notchBandwidthHz_{4.7f};
    float allpassFreqHz_{14.008f};

    // Input DC-block / output bandwidth-limit coupling poles around the
    // ladder (TB303_PARAMETER_CONFIDENCE.md: unsourced but plausible;
    // "Plausible range: 10-30 Hz" / "10-25 kHz" respectively).
    float inputCouplingHz_{20.0f};
    float outputCouplingHz_{20000.0f};
};

} // namespace acidus

#endif // ACIDUS_FILTER_HPP
