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

    // VCA (VEG) gate-high decay time constant (tau, seconds -- see the
    // 2026-09-19 correction note in Envelope.cpp's updateCoefficients()).
    // Cross-checked against RobinSchmidt/Open303's own doc comment for the
    // equivalent parameter ("on the normal 303, this parameter was fixed to
    // approximately 3-4 seconds"); plausible range 2.5-5.0 s, default 3.5 s.
    // Exposed as a CLAP parameter for experimentation.
    void setVegDecaySec(float seconds) { vegDecaySec_ = seconds; updateCoefficients(); }

    // VCA gate-off "quick drain" time for a NON-accented note-off (tau,
    // milliseconds). CORRECTED 2026-09-19: cross-checked against Open303's
    // normalAmpRelease (1.0 ms, used directly as a tau -- see rosic_Open303.cpp
    // triggerNote()/rosic_AnalogEnvelope.cpp's release-phase coefficient,
    // which is the same "always tau, never t60" RC convention used
    // throughout Open303). Default lowered from an unsourced 16 ms estimate
    // to 1 ms to match; plausible range 1-5 ms. See also
    // setVcaGateOffAccentMs() for the (much longer) accented-note-off case.
    void setVcaGateOffMs(float ms) { vcaGateOffSec_ = ms * 0.001f; updateCoefficients(); }

    // VCA gate-off "quick drain" time for an ACCENTED note-off (tau,
    // milliseconds). NEW 2026-09-19: this project previously used a single
    // fixed gate-off time regardless of accent. Open303's source
    // (rosic_Open303.cpp triggerNote()) sets a distinctly longer
    // accentAmpRelease (50.0 ms, also used directly as tau) whenever a note
    // is accented -- giving accented notes a longer, more "hanging" release
    // tail instead of a hard cutoff. Default 50 ms to match; plausible range
    // 30-80 ms.
    void setVcaGateOffAccentMs(float ms) { vcaGateOffAccentSec_ = ms * 0.001f; updateCoefficients(); }

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
    float vcaQuickDrainAccentCoeff_{0.0f};

    // Tunable time constants backing vcaGateHighDecayCoeff_/vcaQuickDrainCoeff_/
    // vcaQuickDrainAccentCoeff_ above; see the setVegDecaySec()/
    // setVcaGateOffMs()/setVcaGateOffAccentMs() doc comments.
    float vegDecaySec_{3.5f};
    float vcaGateOffSec_{0.001f};
    float vcaGateOffAccentSec_{0.05f};

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
