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
    // 2026-09-19: the feedback gain, resonance mapping, output-level
    // compensation and cutoff calibration were corrected after an audit
    // proved the ladder self-oscillated within the front panel's reachable
    // Resonance range -- see the shared confidence-tagged constants below
    // and TB303_FILTER_AUDIT_2026-09-19.md / TB303_PARAMETER_CONFIDENCE.md
    // for the full derivation. The ladder ODE structure itself is untouched.
    float processAccurateSample(float input, float cutoffHz, float resonance);

    // "Faithful" mode: implicit trapezoidal solve (Newton-Raphson, tridiagonal
    // Jacobian) of the coupled diode-ladder at 8x oversampling, with per-stage
    // capacitor pole spreading, input/output coupling poles, and a one-pole
    // resonance-loop coupling-pole network (base ~150 Hz + up to +100 Hz with
    // Resonance, ESTIMATE) standing in for the several further high-pass/
    // coupling poles the real VCF's surrounding network adds inside the
    // resonance feedback path -- see the implementation comment in Filter.cpp
    // for full sourcing (2026-09: corrected back to this region after an
    // earlier ~9 Hz attempt caused a large, audible regression).
    float processFaithfulSample(float input, float cutoffHz, float resonance);

    // Faithful-mode-only tunables, exposed as CLAP parameters for
    // experimentation (not on the plugin's own GUI). See
    // TB303_PARAMETER_CONFIDENCE.md for the full rationale on each.
    void setResCouplingHz(float hz) { resCouplingHz_ = hz; }           // plausible range 100-250 Hz
    void setFeedbackGainCeiling(float k) { feedbackGainCeiling_ = k; } // plausible range 12-17 (see kLadderCriticalGain_ below -- do not set at or above 17 without also re-verifying stability with syrebas_filter_stability_test)

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
    // Resonance-loop coupling-pole network state (single one-pole HPF sitting
    // *inside* the resonance feedback path -- see the extensive comment above
    // processFaithfulSample() in Filter.cpp for the confidence rationale).
    float fHpFbStateX1_{0.0f};
    float fHpFbStateY1_{0.0f};
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

    // Diode ladder capacitor pole-spreading ratios. NEUTRALIZED 2026-09-19
    // (all four equal): the previous 1.0/0.6667/0.303/1.0 spread was BEST
    // GUESS / unsourced, and the 2026-09-19 audit found it distorts the
    // ladder's pole positions away from even this project's own earlier
    // (also unverified) reference-doc numbers, and raises the closed-loop
    // critical feedback gain to ~30-42 (cutoff-dependent) -- meaning
    // feedbackGainCeiling_ below had effectively been re-tuned to compensate
    // for this distortion rather than for anything measured. The *idea* of
    // unequal stage capacitors -- giving unevenly-spaced poles, why the
    // filter is often called "18 dB/octave" despite being physically 4-pole
    // -- is still CONFIRMED (Stinchcombe) as a real hardware property, but
    // with all four capacitor scales equal the ladder's own coupled
    // (non-cascaded) structure *already* produces widely-spaced poles on its
    // own (verified analytically 2026-09-19: the open-loop/no-feedback
    // characteristic polynomial s^4+8s^3+20s^2+16s+2=0 has roots at
    // approximately s/wc = -0.152, -1.235, -2.765, -3.848 -- a ~25:1 spread
    // between the fastest and slowest pole, with no capacitor spreading
    // needed at all). Re-introducing a *sourced* capacitor spread later is
    // fine, but it must come with a re-derivation of the critical gain below
    // to match, not a kFb value re-tuned by feel to compensate.
    const float capScale1_{1.0000f};
    const float capScale2_{1.0000f};
    const float capScale3_{1.0000f};
    const float capScale4_{1.0000f};

    // --- Feedback-loop stability calibration (2026-09-19) -----------------
    // CONFIRMED by direct re-derivation (not just cited from the audit): for
    // the equal-capacitor coupled ladder above, closing the loop with
    // u = input - k*v4 gives closed-loop characteristic polynomial
    // s^4+8s^3+20s^2+16s+(2+2k)=0. Substituting s=j*omega and solving for
    // marginal stability (a root exactly on the imaginary axis) gives
    // omega^2=2 and k=17 *exactly*, independent of omega/cutoff -- i.e. the
    // continuous-time critical feedback gain of this ladder topology is a
    // pure number, 17, matching RobinSchmidt/Open303's own `k=17` anchor for
    // the same topology (its TB_303-mode ladder ODE is this one transposed;
    // transposition preserves eigenvalues/characteristic polynomial, which
    // resolved an initial 2026-09-19 suspicion that the two ladders were
    // structurally different -- they are not). The resonance feedback
    // high-pass (resCouplingHz_ above) sits inside this loop too and, being
    // a real pole, can only *raise* the true critical gain above this bare
    // value (measured 2026-09-19: up to ~19-38 depending on cutoff) -- so
    // capping at the bare value with a margin is a safe (if slightly
    // conservative) bound regardless of the exact resCouplingHz_ setting.
    static constexpr float kLadderCriticalGain_ = 17.0f;
    // 10% safety margin below critical, per the audit's explicit
    // recommendation to use "the continuous-time limit with about 10%
    // margin" rather than importing Open303's own k(fx) polynomial (which
    // compensates for *its* explicit/Euler discretization, not this
    // project's implicit trapezoidal + Newton-Raphson solve, and was found
    // to self-oscillate again at high cutoff when transplanted here).
    static constexpr float kResonanceGainMargin_ = 0.90f;

    // Resonance-knob skew, matching Open303's own mapping
    // (r = (1-exp(-3x))/(1-exp(-3))) so the front-panel Resonance knob's
    // *feel* is comparable: without this, feedbackGainCeiling_ is reached at
    // knob position 1.0 exactly at the linear midpoint of the knob's
    // perceptual range instead of clustering the "hot"/squelchy character
    // toward the top of the knob's travel, where the real pot's audio taper
    // and the ear's own log-ish sensitivity to resonance both put it.
    static inline float skewResonance(float resNorm) {
        return (1.0f - std::exp(-3.0f * resNorm)) / (1.0f - std::exp(-3.0f));
    }

    // Output-level compensation: without this, output level measurably
    // drops as Resonance increases (the loop's DC/passband gain falls as
    // feedback approaches critical) -- Open303 compensates with an output
    // gain that rises to roughly 2.3x at maximum resonance; matched here so
    // increasing Resonance doesn't also quietly turn down the volume.
    static constexpr float kMaxResonanceOutputGain_ = 2.3f;

    // Cutoff-to-angular-frequency calibration: the closed-loop resonant
    // peak of this ladder sits at angular frequency omega = sqrt(2)*wc (see
    // kLadderCriticalGain_ derivation above -- the same omega^2=2 condition
    // that sets the critical gain also sets where the resonance peak sits).
    // Without correction, the front-panel "Cutoff" label would therefore
    // undersell itself by a factor of sqrt(2) (~1.41x) versus where the
    // resonance peak actually is. Scaling wc by 1/sqrt(2) here makes the
    // labeled cutoff Hz equal to the actual resonance-peak frequency,
    // matching Open303's own convention (whose "Cutoff" parameter is
    // documented/measured to equal its resonance-peak frequency to within
    // 1-10%).
    static constexpr float kCutoffToOmegaScale_ = 0.70710678f; // 1/sqrt(2)

    // Resonance-loop coupling-pole base corner (Hz), Faithful mode only
    // (actual corner used is this plus up to +100 Hz scaled by Resonance --
    // see processFaithfulSample()). ESTIMATE - plausible range 100-250 Hz,
    // default 150 Hz. Cross-checked against RobinSchmidt/Open303's
    // `TeeBeeFilter::setFeedbackHighpassCutoff(150.0)`, a well-regarded,
    // independently ear/measurement-tuned open-source TB-303 emulation,
    // which uses a fixed 150 Hz here -- far better-supported than the ~9 Hz
    // this project tried in 2026-09 based on two thin secondary-source
    // summaries, which caused a large, audible regression (excessive
    // low-frequency feedback energy driving the per-stage tanh saturation
    // much harder than intended). Not const: exposed as a CLAP parameter
    // (SynthParameters::resCouplingHz).
    float resCouplingHz_{150.0f};

    // Feedback loop gain ceiling, Faithful mode only (kFb = skewResonance(resNorm) *
    // feedbackGainCeiling_). CORRECTED 2026-09-19: was 36 (~2.1x the analytically
    // confirmed critical gain of 17, i.e. self-oscillating within the reachable
    // Resonance range -- see kLadderCriticalGain_ above). Default is now
    // kLadderCriticalGain_ * kResonanceGainMargin_ = 15.3. Plausible range
    // 12-17 -- do not raise at or above 17 without re-verifying stability
    // with syrebas_filter_stability_test. Exposed as a CLAP parameter
    // (SynthParameters::filterFeedbackGain).
    float feedbackGainCeiling_{kLadderCriticalGain_ * kResonanceGainMargin_};
};

} // namespace syrebas

#endif // SYREBAS_FILTER_HPP
