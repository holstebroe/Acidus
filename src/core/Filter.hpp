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

    // Per-stage ladder pole-frequency scale, relative to the nominal cutoff
    // (w_n = wc * capScaleN_). All default to 1.0 (coincident poles, the
    // idealized equal-component approximation). The real ladder's poles are
    // documented as unevenly spread (TB303_EMULATION_GUIDE.md: "4-pole
    // diode ladder, spread pole frequencies"; TB303_RESEARCH_COMPENDIUM.md
    // Sec6 gives illustrative normalized pole values -0.13/-1.04/-2.33/-3.24,
    // explicitly flagged as unsourced, not a spec) -- these four let that
    // spread be fit against a reference recording instead of guessed.
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
    void setFeedbackHeadroomHz(float hz) { feedbackHeadroomHz_ = hz; }
    void setResCouplingTrackHz(float hz) { resCouplingTrackHz_ = hz; }

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

    static constexpr float kLadderCriticalGain_ = 17.0f;
    static constexpr float kResonanceGainMargin_ = 0.90f;
    float feedbackHeadroomHz_{6600.0f};
    float resonanceSkew_{3.0f};
    float resCouplingTrackHz_{100.0f};

    inline float skewResonance(float resNorm) const {
        if (std::abs(resonanceSkew_) < 1e-4f) return resNorm;
        return (1.0f - std::exp(-resonanceSkew_ * resNorm)) / (1.0f - std::exp(-resonanceSkew_));
    }
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
