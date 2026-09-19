#include "Envelope.hpp"
#include <cmath>
#include <algorithm>

namespace acidus {

Envelope::Envelope() {
    setSampleRate(44100.0);
}

void Envelope::setSampleRate(double sampleRate) {
    sampleRate_ = sampleRate;
    updateCoefficients();
}

void Envelope::setDecay(float decayParam) {
    float norm = std::min(std::max(decayParam, 0.0f), 1.0f);
    vcfDecayTimeSec_ = 0.20f * std::pow(12.5f, norm);
    updateCoefficients();
}

void Envelope::updateCoefficients() {
    float dt = 1.0f / static_cast<float>(sampleRate_);

    // VCF envelope attack/decay time constants
    vcfAttackCoeff_ = 1.0f - std::exp(-dt / 0.0035f);
    if (faithfulAccentDecay_ && isAccent_) {
        vcfDecayCoeff_ = std::exp(-1.0f / static_cast<float>(sampleRate_ * kAccentDecayTimeSec));
    } else {
        vcfDecayCoeff_ = std::exp(-1.0f / static_cast<float>(sampleRate_ * vcfDecayTimeSec_));
    }

    // VCA envelope attack/decay time constants
    vcaAttackCoeff_           = 1.0f - std::exp(-dt / 0.003f);
    vcaGateHighDecayCoeff_    = std::exp(-1.0f / static_cast<float>(sampleRate_ * vegDecaySec_));
    vcaQuickDrainCoeff_       = std::exp(-1.0f / static_cast<float>(sampleRate_ * vcaGateOffSec_));
    vcaQuickDrainAccentCoeff_ = std::exp(-1.0f / static_cast<float>(sampleRate_ * vcaGateOffAccentSec_));

    // Accent Sweep RC (47 kOhm + 1 uF -> tau ~ 47ms)
    accentChargeCoeff_    = 1.0f - std::exp(-1.0f / static_cast<float>(sampleRate_ * 0.047));
    accentDischargeCoeff_ = std::exp(-1.0f / static_cast<float>(sampleRate_ * 0.047));

    // Accent VCA RC smoothing (47 kOhm + 0.033 uF -> tau ~ 1.55ms)
    accentVcaCoeff_ = 1.0f - std::exp(-1.0f / static_cast<float>(sampleRate_ * 0.00155));
}

void Envelope::noteOn(bool isAccent, bool isSlide, float accentKnob) {
    gate_ = true;
    isAccent_ = isAccent;

    updateCoefficients();

    if (!isSlide) {
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
    vcaTarget_ = 0.0f;
    vcfTarget_ = 0.0f;
}

void Envelope::processNextSample() {
    // 1. VCF Filter Envelope Processing
    if (vcfTarget_ > vcfEnv_) {
        vcfEnv_ += vcfAttackCoeff_ * (vcfTarget_ - vcfEnv_);
        if (vcfEnv_ >= 0.99f) {
            vcfTarget_ = 0.0f; // Transition smoothly to decay phase
        }
    } else {
        vcfEnv_ *= vcfDecayCoeff_;
    }

    // 2. VCA Amplitude Envelope Processing
    if (gate_) {
        if (vcaTarget_ > vcaEnv_) {
            vcaEnv_ += vcaAttackCoeff_ * (vcaTarget_ - vcaEnv_);
            if (vcaEnv_ >= 0.99f) {
                vcaTarget_ = 0.0f; // Transition to slow decay
            }
        } else {
            vcaEnv_ *= vcaGateHighDecayCoeff_;
        }
    } else {
        vcaEnv_ *= isAccent_ ? vcaQuickDrainAccentCoeff_ : vcaQuickDrainCoeff_;
    }

    // 3. Accent Sweep Capacitor Processing (1uF capacitor charge memory)
    if (isAccent_ && gate_) {
        accentCap_ += accentChargeCoeff_ * (vcfEnv_ - accentCap_);
    } else {
        accentCap_ *= accentDischargeCoeff_;
    }

    // 4. Accent VCA control path smoothing
    float accentVcaTarget = (isAccent_ && gate_) ? vcfEnv_ : 0.0f;
    accentVca_ += accentVcaCoeff_ * (accentVcaTarget - accentVca_);
}

} // namespace acidus
