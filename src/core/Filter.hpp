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
    // capacitor pole spreading, input/output coupling poles, and a 2-pole
    // resonance-loop coupling-pole network (~9 Hz, ESTIMATE, plausible range
    // 5-15 Hz) standing in for the several further high-pass/coupling poles
    // the real VCF's surrounding network adds inside the resonance feedback
    // path -- see the implementation comment in Filter.cpp for full sourcing.
    float processFaithfulSample(float input, float cutoffHz, float resonance);

    // Faithful-mode-only tunables, exposed as CLAP parameters for
    // experimentation (not on the plugin's own GUI). See
    // TB303_PARAMETER_CONFIDENCE.md for the full rationale on each.
    void setResCouplingHz(float hz) { resCouplingHz_ = hz; }           // plausible range 5-15 Hz
    void setFeedbackGainCeiling(float k) { feedbackGainCeiling_ = k; } // plausible range 20-40

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
    // Resonance-loop coupling-pole network state (2 cascaded one-pole HPF
    // stages sitting *inside* the resonance feedback path -- see the
    // extensive comment above processFaithfulSample() in Filter.cpp for the
    // confidence rationale). Stage 1: fHpFbStateX1_/Y1_. Stage 2 (cascaded
    // on stage 1's output): fHpFbStateX2_/Y2_.
    float fHpFbStateX1_{0.0f};
    float fHpFbStateY1_{0.0f};
    float fHpFbStateX2_{0.0f};
    float fHpFbStateY2_{0.0f};
    float prevFaithfulInput_{0.0f};
    OnePole inputCoupling_;   // ~20 Hz HPF ahead of the ladder (input DC-block cap)
    OnePole outputCoupling_;  // ~20 kHz LPF after the ladder (stray/buffer bandwidth)

    // --- Confidence-tagged constants (Faithful mode) ---------------------
    // See TB303_PARAMETER_CONFIDENCE.md for the full cross-reference against
    // TB303_RESEARCH_COMPENDIUM.md. Three tiers used throughout this class:
    //   CONFIRMED    - matches a factory service-notes value or primary
    //                  circuit analysis directly.
    //   ESTIMATE     - mechanism/order-of-magnitude is sourced, exact number
    //                  is not; a plausible range is given to experiment with.
    //   BEST GUESS   - no source at all; a free calibration knob.

    // Diode ladder capacitor pole-spreading ratios. BEST GUESS: no source
    // consulted gives component-level VCF capacitor values (the factory
    // service notes' own VCF trim target, TM3, wasn't legibly recoverable
    // from the scan used for the 2026 research pass). The *idea* of unequal
    // stage capacitors -- giving unevenly-spaced poles, which is why the
    // filter is often called "18 dB/octave" despite being physically 4-pole
    // -- is CONFIRMED (Stinchcombe); these specific ratios (as if C1=10nF,
    // C2=15nF, C3=33nF, C4=10nF) are not. Plausible range for each ratio:
    // 0.3-1.5x relative to stage 1; try spreading them further apart (e.g.
    // capScale2_ 0.4-0.8, capScale3_ 0.15-0.5) for a more pronounced
    // "18 dB-ish" transition-region slope.
    const float capScale1_{1.0000f};
    const float capScale2_{0.6667f};
    const float capScale3_{0.3030f};
    const float capScale4_{1.0000f};

    // Resonance-loop coupling-pole corner (Hz), Faithful mode only. ESTIMATE
    // - plausible range 5-15 Hz, default 9 Hz. Full rationale and sourcing in
    // processFaithfulSample()'s implementation comment in Filter.cpp. Not
    // const: exposed as a CLAP parameter (SynthParameters::resCouplingHz).
    float resCouplingHz_{9.0f};

    // Feedback loop gain ceiling, Faithful mode only (kFb = resNorm *
    // feedbackGainCeiling_). BEST GUESS / calibration knob, not a circuit
    // value - plausible range 20-40, default 36. Exposed as a CLAP parameter
    // (SynthParameters::filterFeedbackGain).
    float feedbackGainCeiling_{36.0f};
};

} // namespace syrebas

#endif // SYREBAS_FILTER_HPP
