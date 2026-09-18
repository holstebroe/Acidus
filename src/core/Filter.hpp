#ifndef SYREBAS_FILTER_HPP
#define SYREBAS_FILTER_HPP

#include <cmath>

namespace syrebas {

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

    // "Accurate" mode: 4x oversampled coupled diode-ladder solver (RK2/midpoint).
    // Kept exactly as-is: this is the emulation mode users already know and like.
    float processAccurateSample(float input, float cutoffHz, float resonance);

    // "Faithful" mode: implicit trapezoidal solve (Newton-Raphson, tridiagonal
    // Jacobian) of the coupled diode-ladder at 8x oversampling, with per-stage
    // capacitor pole spreading and the extra input/output coupling poles the
    // real VCF's surrounding network adds.
    float processFaithfulSample(float input, float cutoffHz, float resonance);

private:
    double sampleRate_{44100.0};
    double oversampledRateAccurate_{176400.0};
    double oversampledRateFaithful_{352800.0};

    // Coupled ladder node voltage states, one set per mode so switching modes
    // mid-performance (or A/B-ing them) never cross-contaminates state.
    float ladderV1_{0.0f};
    float ladderV2_{0.0f};
    float ladderV3_{0.0f};
    float ladderV4_{0.0f};
    float hpFbStateX1_{0.0f};
    float hpFbStateY1_{0.0f};
    float prevAccurateInput_{0.0f};

    float fLadderV1_{0.0f};
    float fLadderV2_{0.0f};
    float fLadderV3_{0.0f};
    float fLadderV4_{0.0f};
    float fHpFbStateX1_{0.0f};
    float fHpFbStateY1_{0.0f};
    float prevFaithfulInput_{0.0f};
    OnePole inputCoupling_;   // ~20 Hz HPF ahead of the ladder (input DC-block cap)
    OnePole outputCoupling_;  // ~20 kHz LPF after the ladder (stray/buffer bandwidth)

    // Diode ladder capacitor values / pole spreading for ~18dB/oct slope
    // C1 = 10nF, C2 = 15nF, C3 = 33nF, C4 = 10nF -> conductance scale = 1/C
    const float capScale1_{1.0000f};
    const float capScale2_{0.6667f};
    const float capScale3_{0.3030f};
    const float capScale4_{1.0000f};
};

} // namespace syrebas

#endif // SYREBAS_FILTER_HPP
