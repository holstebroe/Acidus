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
    void setResCouplingHz(float hz) { resCouplingHz_ = hz; }           // plausible range 70-150 Hz (antto 122, Open303 150)
    void setFeedbackGainCeiling(float k) { feedbackGainCeiling_ = k; } // plausible range 16-21 (see resonanceLimit below)
    void setPostFilterHpHz(float hz) { postFilterHpHz_ = hz; }         // plausible range 15-35 Hz
    void setNotchFreqHz(float hz) { notchFreqHz_ = hz; }               // plausible range 4-15 Hz
    void setNotchBandwidthHz(float hz) { notchBandwidthHz_ = hz; }     // plausible range 2-10 Hz
    void setAllpassFreqHz(float hz) { allpassFreqHz_ = hz; }           // plausible range 8-25 Hz
    void setInputCouplingHz(float hz) { inputCouplingHz_ = hz; }       // plausible range 10-30 Hz
    void setOutputCouplingHz(float hz) { outputCouplingHz_ = hz; }     // plausible range 10-25 kHz

    // Per-stage ladder pole-frequency scale, relative to the nominal cutoff
    // (w_n = wc * capScaleN_). All default to 1.0, which is NOT "coincident
    // poles": the coupled ladder equations below already contain the TB-303's
    // half-size bottom capacitor (C18 = C/2), and with every scale at 1.0 the
    // linear core is exactly Stinchcombe's H_tb(s), poles -0.128 / -1.038 /
    // -2.325 / -3.236 wc (TB303_REFERENCE.md §10.2; reference test A1).
    // Anything other than 1.0 is an unsourced departure from the schematic's
    // 33/33/33/18 nF; kept only as experimental knobs.
    void setCapScale1(float s) { capScale1_ = s; } // plausible range 0.2-4.0
    void setCapScale2(float s) { capScale2_ = s; } // plausible range 0.2-4.0
    void setCapScale3(float s) { capScale3_ = s; } // plausible range 0.2-4.0
    void setCapScale4(float s) { capScale4_ = s; } // plausible range 0.2-4.0

    // How hard the input signal drives the ladder's per-stage tanh
    // nonlinearity, relative to its thermal-voltage-like scale (Vt = 0.052).
    // A full-scale (~1.0) audio signal maps to ladderInputScale_ "Vt units";
    // since ladderInputScale_ starts almost equal to Vt, realistic playing
    // levels already sit at the nonlinearity's saturation onset, which
    // measurably crushes the resonant peak's height at those levels (small-
    // signal probing shows a much taller peak than a full-scale oscillator
    // does) -- lower this to give the peak more headroom before saturation.
    // Output is rescaled by the reciprocal to keep the overall passband
    // gain unchanged as this is swept.
    void setLadderInputScale(float s) { ladderInputScale_ = s; } // plausible range 0.02-0.20 (default 0.05)

    // Ladder orientation (audit S1, TB303_REFERENCE.md §10.3).
    //   0: legacy mirrored form -- input stage tanh(u - v1), half capacitor
    //      on the output stage, no terminal tanh.
    //   1: circuit form -- the Q12 input pair saturates on tanh(u) with
    //      u = input - feedback, half capacitor (C18) on stage 1, and the
    //      top stage terminates in tanh(y4).
    // Both are linearly identical (same poles, DC gain 1); only the large-
    // signal behaviour differs.
    void setLadderTopology(int t) { ladderTopology_ = t; }

    // Resonance-pot law and the resonance-dependent feedback/coupling terms.
    // Deliberately NOT a resonance-dependent output gain: TB303_EMULATION_
    // REFERENCE.md Sec60 documents passband/bass gain *falling* as
    // Resonance increases (a real, load-bearing loading effect of the
    // feedback path) and explicitly says "do not add a separate 'bass
    // compensation' stage" -- so this model has none. Whatever level the
    // resonant peak reaches has to come from kFb (below) approaching the
    // ladder's self-oscillation ceiling, the same way the real feedback
    // loop does it, not from a post-hoc broadband multiply.
    void setResonanceSkew(float k) { resonanceSkew_ = k; }

    // Resonance limit, as a fraction of the loop's critical (self-oscillation)
    // feedback gain at the current cutoff. The feedback applied at Resonance
    // = 1 is min(feedbackGainCeiling_, resonanceLimit_ * k_crit(cutoff)),
    // where k_crit comes from the linearised loop (ladder core + in-loop
    // coupling high-pass). Around ~1 kHz and below, the ceiling is what
    // binds and the loop's own coupling network sets how close it gets to
    // oscillation, as in the hardware. At high cutoff, Stinchcombe's full
    // model (§11.1) goes past threshold while hardware reports "no clean
    // self-oscillation" (§12) -- this knob picks between the two:
    //   < 1.0  never self-oscillates (0.98 = stays ~2 % below threshold)
    //   > 1.0  allows nonlinearity-limited self-oscillation at the top
    //   > ~1.08 off with the default 18.5 ceiling (the ceiling binds everywhere)
    void setResonanceLimit(float r) { resonanceLimit_ = r; }

    // Critical feedback gain of the linearised loop at this cutoff (exposed
    // for tests/diagnostics). Interpolated from a table over log cutoff that
    // is rebuilt only when the poles, the feedback coupling or the rate
    // change; criticalFeedbackGainExact() is the direct search.
    float criticalFeedbackGain(float cutoffHz);
    double criticalFeedbackGainExact(double cutoffHz);

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

    float capScale1_{1.0000f};
    float capScale2_{1.0000f};
    float capScale3_{1.0000f};
    float capScale4_{1.0000f};

    float ladderInputScale_{0.05f};
    int ladderTopology_{0};

    float resonanceSkew_{3.0f};
    float resonanceLimit_{0.98f};

    // Normalised ladder pole magnitudes (units of wc) for the current cap
    // scales; recomputed only when a scale changes.
    float polesForScales_[4]{-1.0f, -1.0f, -1.0f, -1.0f};
    double poles_[4]{0.152241, 1.234633, 2.765367, 3.847759};
    void updatePoles();

    // k_crit table over log(cutoff), 20 Hz .. the solver's 10 % clamp. The
    // search behind it costs ~50 atan/exp calls per point, far too much to
    // run per sample while the envelope sweeps the cutoff.
    static constexpr int kKcTableSize = 1024;
    float kcTable_[kKcTableSize]{};
    float kcTableLogLo_{0.0f};
    float kcTableInvStep_{0.0f};
    float kcTableCouplingHz_{-1.0f};
    double kcTableOsRate_{-1.0};
    bool kcTableValid_{false};
    void buildKcTable();

    // Cached input/output coupling coefficients (see processSample).
    float inCouplingAlpha_{0.0f}, outCouplingAlpha_{0.0f};
    float couplingForInHz_{-1.0f}, couplingForOutHz_{-1.0f}, couplingForDt_{-1.0f};

    inline float skewResonance(float resNorm) const {
        if (std::abs(resonanceSkew_) < 1e-4f) return resNorm;
        return (1.0f - std::exp(-resonanceSkew_ * resNorm)) / (1.0f - std::exp(-resonanceSkew_));
    }
    static constexpr float kCutoffToOmegaScale_ = 0.70710678f; // 1/sqrt(2)

    float resCouplingHz_{100.0f};
    float feedbackGainCeiling_{18.5f};

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
