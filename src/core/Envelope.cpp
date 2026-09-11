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
    // Exponential scaling from 200ms (0.2s) fully CCW to 2.5s fully CW
    float d = std::min(std::max(decayParam, 0.0f), 1.0f);
    // Exponential mapping: 0.2 * (2.5 / 0.2)^d = 0.2 * (12.5)^d
    vcfDecayTimeSec_ = 0.20f * std::pow(12.5f, d);
    updateCoefficients();
}

void Envelope::updateCoefficients() {
    // VCF Attack: 3.5ms RC curve
    vcfAttackCoeff_ = 1.0f - std::exp(-1.0f / static_cast<float>(sampleRate_ * 0.0035));
    // VCF Decay: exponential decay time constant for vcfDecayTimeSec_ (tau = t_60 / 6.9078)
    vcfDecayCoeff_ = std::exp(-1.0f / static_cast<float>(sampleRate_ * (vcfDecayTimeSec_ / 6.907755f)));

    // VCA Attack: 3.0ms RC curve
    vcaAttackCoeff_ = 1.0f - std::exp(-1.0f / static_cast<float>(sampleRate_ * 0.003));
    // VCA Gate HIGH Phase 1 Decay: 4.0s slow discharge time constant
    vcaGateHighDecayCoeff_ = std::exp(-1.0f / static_cast<float>(sampleRate_ * (4.0f / 6.907755f)));
    // VCA Gate LOW Phase 2 Quick Drain: discharge to silence (-60dB / 0.001) in 18ms (tau = 18ms / 6.9078 = 2.6ms)
    float quickDrainTimeSec = 0.018f;
    vcaQuickDrainCoeff_ = std::exp(-1.0f / static_cast<float>(sampleRate_ * (quickDrainTimeSec / 6.907755f)));
}

void Envelope::noteOn(bool isAccent, bool isSlide) {
    gate_ = true;
    isAccent_ = isAccent;

    updateCoefficients();

    // Accent Logic: If Note_Accent == True, force VCF decay envelope time directly to its absolute minimum (~200ms)
    if (isAccent_) {
        float minDecayTimeSec = 0.20f;
        vcfDecayCoeff_ = std::exp(-1.0f / static_cast<float>(sampleRate_ * (minDecayTimeSec / 6.907755f)));
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
                vcaTarget_ = 0.0f; // Transition to 4.0s slow decay
            }
        } else {
            // Gate HIGH Decay Phase: 4.0-second slow exponential discharge
            vcaEnv_ *= vcaGateHighDecayCoeff_;
        }
    } else {
        // PHASE 2: GATE = LOW (The Quick Drain Correction)
        // Discharges to absolute silence (-60dB) within 15ms to 20ms
        vcaEnv_ *= vcaQuickDrainCoeff_;
    }
}

} // namespace syrebas
