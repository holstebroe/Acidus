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
    // VCF Attack: 3.5ms
    vcfAttackCoeff_ = 1.0f - std::exp(-1.0f / static_cast<float>(sampleRate_ * 0.0035));
    // VCF Decay: RC time constant for vcfDecayTimeSec_
    vcfDecayCoeff_ = std::exp(-1.0f / static_cast<float>(sampleRate_ * (vcfDecayTimeSec_ / 3.0f)));

    // VCA Attack: 3.0ms
    vcaAttackCoeff_ = 1.0f - std::exp(-1.0f / static_cast<float>(sampleRate_ * 0.003));
    // VCA Decay: 4.0s
    vcaDecayCoeff_ = std::exp(-1.0f / static_cast<float>(sampleRate_ * (4.0f / 3.0f)));
}

void Envelope::noteOn(bool isAccent, bool isSlide) {
    vcaGate_ = true;
    isAccent_ = isAccent;

    updateCoefficients();

    // Accent Logic: If Note_Accent == True, force VCF decay envelope time directly to its absolute minimum (~200ms)
    if (isAccent_) {
        float minDecayTimeSec = 0.20f;
        vcfDecayCoeff_ = std::exp(-1.0f / static_cast<float>(sampleRate_ * (minDecayTimeSec / 3.0f)));
    }

    if (!isSlide) {
        // On non-slide note, VCF envelope triggers / retriggers
        vcfState_ = 1.0f;
        vcaState_ = 1.0f;
    } else {
        // During Slide: do NOT send note-off or note-on trigger to envelopes.
        // VCF envelope continues natural decay toward zero uninterrupted.
        // VCA envelope gate remains high.
        vcaState_ = 1.0f;
    }
}

void Envelope::noteOff() {
    vcaGate_ = false;
    // On 303, VCF envelope has strictly 0 sustain/release; it decays exponentially to 0.
    // VCA gate goes low, but long decay tail (4.0s) continues.
}

void Envelope::processNextSample() {
    // VCF Envelope: Attack (3.5ms exponential RC) + Decay (200ms to 2.5s exponential RC)
    vcfEnv_ += vcfAttackCoeff_ * (vcfState_ - vcfEnv_);
    vcfState_ *= vcfDecayCoeff_;

    // VCA Envelope: Attack (3.0ms) + Decay (4.0s continuous gating tail)
    float targetVcaState = vcaGate_ ? 1.0f : 0.0f;
    if (vcaGate_) {
        vcaState_ = 1.0f;
    } else {
        vcaState_ *= vcaDecayCoeff_;
    }
    vcaEnv_ += vcaAttackCoeff_ * (vcaState_ - vcaEnv_);
}

} // namespace syrebas
