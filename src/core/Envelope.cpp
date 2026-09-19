#include "Envelope.hpp"
#include <cmath>
#include <algorithm>

namespace syrebas {

Envelope::Envelope() {
    setSampleRate(44100.0);
    setDecay(0.5f);
}

void Envelope::setSampleRate(double sampleRate) {
    sampleRate_ = sampleRate;
    updateCoefficients();
}

void Envelope::setDecay(float decayParam) {
    // Exponential scaling from 200ms (0.2s) fully CCW to 2.5s fully CW (Section 19.2)
    float d = std::min(std::max(decayParam, 0.0f), 1.0f);
    vcfDecayTimeSec_ = 0.20f * std::pow(12.5f, d);
    updateCoefficients();
}

void Envelope::updateCoefficients() {
    // VCF Attack: 3.5ms RC curve
    vcfAttackCoeff_ = 1.0f - std::exp(-1.0f / static_cast<float>(sampleRate_ * 0.0035));

    // VCF Decay: exponential decay time constant. In faithful mode, an active accent
    // overrides this unconditionally (Section 25: "the MEG decay control is
    // bypassed/switched... independent of the front-panel Decay setting... should not
    // merely multiply the decay coefficient"), and must be re-derived from the fixed
    // accent decay time - not from vcfDecayTimeSec_ - every time this runs, since
    // setDecay() calls this once per audio block for the lifetime of the note.
    //
    // CORRECTED 2026-09-19: this used to divide by 6.907755 to treat
    // vcfDecayTimeSec_/kAccentDecayTimeSec as a t60 (time to -60dB) and derive tau from
    // it. Cross-checking RobinSchmidt/Open303's actual source
    // (rosic_Open303.cpp/rosic_DecayEnvelope.cpp) shows its Decay-knob range
    // (200-2000ms, quoted directly from real 303 hardware in its own setDecay() doc
    // comment) and its accentDecay=200.0 constant are both fed *directly* as tau
    // (DecayEnvelope::setDecayTimeConstant() -> c = exp(-1/(0.001*tau*fs)), no t60
    // conversion anywhere in Open303's codebase). The stray /6.907755f here made every
    // MEG decay ~6.9x faster than intended across the whole Decay knob range. Removed;
    // vcfDecayTimeSec_ and kAccentDecayTimeSec are now used directly as tau, matching
    // Open303 exactly (its accentDecay=200.0ms literal value now equals
    // kAccentDecayTimeSec exactly).
    if (faithfulAccentDecay_ && isAccent_) {
        vcfDecayCoeff_ = std::exp(-1.0f / static_cast<float>(sampleRate_ * kAccentDecayTimeSec));
    } else {
        vcfDecayCoeff_ = std::exp(-1.0f / static_cast<float>(sampleRate_ * vcfDecayTimeSec_));
    }

    // VCA Attack: 3.0ms RC curve. ESTIMATE (order-of-magnitude "very fast",
    // not independently sourced to this exact figure).
    vcaAttackCoeff_ = 1.0f - std::exp(-1.0f / static_cast<float>(sampleRate_ * 0.003));
    // VCA Gate HIGH Phase 1 Decay: slow discharge time constant.
    // CORRECTED 2026-09-19: same t60-vs-tau fix as the VCF Decay above --
    // vegDecaySec_ is now used directly as tau, matching Open303's
    // AnalogEnvelope::setDecay() convention (also a direct-tau RC
    // coefficient, no t60 division anywhere in Open303). This lands our
    // existing 3.5s default almost exactly on Open303's own doc comment for
    // this parameter ("on the normal 303, this parameter was fixed to
    // approximately 3-4 seconds"), stronger corroboration than the vague
    // "rather long" (Whittle) language it was originally estimated from.
    // Tunable via setVegDecaySec() / SynthParameters::vegDecaySec (CLAP
    // parameter), see Envelope.hpp.
    vcaGateHighDecayCoeff_ = std::exp(-1.0f / static_cast<float>(sampleRate_ * vegDecaySec_));
    // VCA Gate LOW Phase 2 Quick Drain (non-accented note-off): discharge to
    // silence. CORRECTED 2026-09-19: same t60-vs-tau fix, PLUS cross-checked
    // against Open303's normalAmpRelease=1.0ms (also used directly as tau,
    // see rosic_Open303.cpp triggerNote()/rosic_AnalogEnvelope.cpp's release
    // phase) -- default lowered from an unsourced 16ms estimate to 1ms.
    // Tunable via setVcaGateOffMs() / SynthParameters::vcaGateOffMs (CLAP
    // parameter).
    vcaQuickDrainCoeff_ = std::exp(-1.0f / static_cast<float>(sampleRate_ * vcaGateOffSec_));
    // VCA Gate LOW Phase 2 Quick Drain (ACCENTED note-off): NEW 2026-09-19.
    // Open303 gives accented notes a distinctly longer release
    // (accentAmpRelease=50.0ms, same direct-tau convention) instead of the
    // same hard cutoff as normal notes -- this project previously had no
    // such distinction. Tunable via setVcaGateOffAccentMs() /
    // SynthParameters::vcaGateOffAccentMs (CLAP parameter).
    vcaQuickDrainAccentCoeff_ = std::exp(-1.0f / static_cast<float>(sampleRate_ * vcaGateOffAccentSec_));

    // Accent Sweep RC (47 kOhm + 1 uF -> tau ~ 47ms)
    accentChargeCoeff_ = 1.0f - std::exp(-1.0f / static_cast<float>(sampleRate_ * 0.047));
    accentDischargeCoeff_ = std::exp(-1.0f / static_cast<float>(sampleRate_ * 0.047));

    // Accent VCA RC smoothing (47 kOhm + 0.033 uF -> tau ~ 1.55ms)
    accentVcaCoeff_ = 1.0f - std::exp(-1.0f / static_cast<float>(sampleRate_ * 0.00155));
}

void Envelope::noteOn(bool isAccent, bool isSlide, float accentKnob) {
    gate_ = true;
    isAccent_ = isAccent;

    updateCoefficients();

    // Legacy (Accurate mode) accent decay behavior: scale toward minDecay by accentKnob.
    // When accentKnob == 0.0, VCF decay stays at the normal Decay-knob setting. Left as-is
    // (including being re-derived from vcfDecayTimeSec_ on every subsequent audio block,
    // which effectively drops the override after the first block) so Accurate mode's sound
    // is unchanged; faithfulAccentDecay_ handles the corrected behavior in updateCoefficients().
    if (!faithfulAccentDecay_ && isAccent_) {
        float actualDecayTimeSec = vcfDecayTimeSec_ + (kAccentDecayTimeSec - vcfDecayTimeSec_) * std::min(std::max(accentKnob, 0.0f), 1.0f);
        vcfDecayCoeff_ = std::exp(-1.0f / static_cast<float>(sampleRate_ * (actualDecayTimeSec / 6.907755f)));
    }

    if (!isSlide) {
        // Re-Trigger Logic (Legato / Staccato when Slide == False):
        // Start attack phase from current voltage level (do not hard-reset to 0.0, catch existing tail)
        vcfTarget_ = 1.0f;
        vcaTarget_ = 1.0f;
    } else {
        // Re-Trigger Logic (Slide == True):
        // Do not trigger attack phase of either envelope.
        // Let VCF envelope continue uninterrupted decay toward 0, hold VCA target at current/decay state.
        vcfTarget_ = 0.0f;
        vcaTarget_ = 0.0f;
    }
}

void Envelope::noteOff() {
    gate_ = false;
    // Phase 2 (Gate LOW):
    // VCA: Instantly switch decay target to 0.0 and override time constant (quick drain to 0 in 15-20ms)
    vcaTarget_ = 0.0f;
    // VCF: Continues tracking along its current exponential decay path toward 0.0. Ignores Note-Off entirely.
    vcfTarget_ = 0.0f;
}

void Envelope::processNextSample() {
    // 1. VCF Filter Envelope Processing
    if (vcfTarget_ > vcfEnv_) {
        // Attack Phase: 3.5ms exponential RC rise
        vcfEnv_ += vcfAttackCoeff_ * (vcfTarget_ - vcfEnv_);
        if (vcfEnv_ >= 0.99f) {
            vcfTarget_ = 0.0f; // Transition smoothly to decay phase
        }
    } else {
        // Decay Phase: Uninterrupted exponential decay toward 0.0
        vcfEnv_ *= vcfDecayCoeff_;
    }

    // 2. VCA Amplitude Envelope Processing
    if (gate_) {
        // PHASE 1: GATE = HIGH
        if (vcaTarget_ > vcaEnv_) {
            // Attack Phase: 3.0ms exponential RC rise up to 1.0 peak
            vcaEnv_ += vcaAttackCoeff_ * (vcaTarget_ - vcaEnv_);
            if (vcaEnv_ >= 0.99f) {
                vcaTarget_ = 0.0f; // Transition to slow decay
            }
        } else {
            // Gate HIGH Decay Phase: slow exponential discharge
            vcaEnv_ *= vcaGateHighDecayCoeff_;
        }
    } else {
        // PHASE 2: GATE = LOW (The Quick Drain Correction). Accented notes use a
        // distinctly longer release tail than non-accented notes (2026-09-19, see
        // setVcaGateOffAccentMs() doc comment) -- isAccent_ still reflects whichever
        // note just ended, since noteOff() does not touch it.
        vcaEnv_ *= isAccent_ ? vcaQuickDrainAccentCoeff_ : vcaQuickDrainCoeff_;
    }

    // 3. Accent Sweep Capacitor Processing (1uF capacitor charge memory)
    // Continuous capacitor state across notes (never reset)
    if (isAccent_ && gate_) {
        accentCap_ += accentChargeCoeff_ * (vcfEnv_ - accentCap_);
    } else {
        accentCap_ *= accentDischargeCoeff_;
    }

    // 4. Accent VCA control path smoothing (47 kOhm + 0.033 uF RC network)
    float accentVcaTarget = (isAccent_ && gate_) ? vcfEnv_ : 0.0f;
    accentVca_ += accentVcaCoeff_ * (accentVcaTarget - accentVca_);
}

} // namespace syrebas
