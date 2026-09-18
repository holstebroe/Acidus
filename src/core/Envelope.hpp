#ifndef SYREBAS_ENVELOPE_HPP
#define SYREBAS_ENVELOPE_HPP

namespace syrebas {

class Envelope {
public:
    Envelope();
    ~Envelope() = default;

    void setSampleRate(double sampleRate);
    void setDecay(float decayParam); // 0.0 to 1.0 -> 200ms to 2.5s

    // "Faithful" mode treats the accent MEG-decay override as the unconditional
    // hardware switch the reference documents it as (independent of the Accent
    // knob, and stable across every audio block rather than being re-derived
    // from the Decay knob each block). "Accurate" mode keeps its existing,
    // Accent-knob-scaled behavior unchanged.
    void setFaithfulAccentDecay(bool faithful) { faithfulAccentDecay_ = faithful; }

    // VCA (VEG) gate-high decay time constant (t60, seconds). UNSOURCED
    // ESTIMATE - no primary source gives a specific stock figure (Whittle
    // only describes it as "rather long"); plausible range 2.5-5.0 s,
    // default 3.5 s. Exposed as a CLAP parameter for experimentation.
    void setVegDecaySec(float seconds) { vegDecaySec_ = seconds; updateCoefficients(); }

    // VCA gate-off "quick drain" time (t60, milliseconds). UNSOURCED ESTIMATE
    // - plausible range 10-25 ms, default 16 ms. Exposed as a CLAP parameter
    // for experimentation.
    void setVcaGateOffMs(float ms) { vcaGateOffSec_ = ms * 0.001f; updateCoefficients(); }

    void noteOn(bool isAccent, bool isSlide, float accentKnob = 1.0f);
    void noteOff();

    void processNextSample();

    float getVcfEnv() const { return vcfEnv_; }
    float getVcaEnv() const { return vcaEnv_; }
    float getAccentCap() const { return accentCap_; }
    float getAccentVca() const { return accentVca_; }
    bool isAccent() const { return isAccent_; }
    bool isActive() const { return gate_ || (vcaEnv_ > 0.0001f) || (vcfEnv_ > 0.0001f); }

private:
    double sampleRate_{44100.0};

    bool gate_{false};
    bool isAccent_{false};
    bool faithfulAccentDecay_{false};

    // Accent MEG decay override target (~200ms, Section 25), fixed regardless of the
    // Decay knob and, in faithful mode, regardless of the Accent knob too.
    static constexpr float kAccentDecayTimeSec = 0.20f;

    float vcfDecayTimeSec_{0.20f};
    float vcfAttackCoeff_{0.0f};
    float vcfDecayCoeff_{0.0f};

    float vcaAttackCoeff_{0.0f};
    float vcaGateHighDecayCoeff_{0.0f};
    float vcaQuickDrainCoeff_{0.0f};

    // Tunable time constants backing vcaGateHighDecayCoeff_/vcaQuickDrainCoeff_
    // above; see the setVegDecaySec()/setVcaGateOffMs() doc comments.
    float vegDecaySec_{3.5f};
    float vcaGateOffSec_{0.016f};

    float accentChargeCoeff_{0.0f};
    float accentDischargeCoeff_{0.0f};
    float accentVcaCoeff_{0.0f};

    float vcfEnv_{0.0f};
    float vcfTarget_{0.0f};

    float vcaEnv_{0.0f};
    float vcaTarget_{0.0f};

    float accentCap_{0.0f};
    float accentVca_{0.0f};

    void updateCoefficients();
};

} // namespace syrebas

#endif // SYREBAS_ENVELOPE_HPP
