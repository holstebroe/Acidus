#include "Envelope.hpp"
#include <cmath>

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
    // 303 VCF decay time constant (tau):
    // T60 = 6.91 * tau
    // Decay = 0 -> T60 = 200ms (0.2s) -> tau = 0.2 / 6.91 = 0.0289s (~29ms)
    // Decay = 1 -> T60 = 2500ms (2.5s) -> tau = 2.5 / 6.91 = 0.3618s (~362ms)
    float t60Sec = 0.20f + 2.30f * (decayParam * decayParam);
    float tauSec = t60Sec / 6.907755f;
    decayTimeSec_ = tauSec;
    updateCoefficients();
}

void Envelope::updateCoefficients() {
    // Decay factor per sample: e^(-1 / (fs * tau))
    decayCoeff_ = std::exp(-1.0f / static_cast<float>(sampleRate_ * decayTimeSec_));

    // Accent envelope T60 = ~160ms -> tau = 0.16 / 6.91 = ~23ms
    float accentT60Sec = 0.16f;
    float accentTauSec = accentT60Sec / 6.907755f;
    accentDecayCoeff_ = std::exp(-1.0f / static_cast<float>(sampleRate_ * accentTauSec));
}

void Envelope::noteOn(bool isAccent, bool isSlide) {
    gate_ = true;
    isAccent_ = isAccent;

    if (!isSlide) {
        // Main envelope retriggers on non-slide notes
        mainEnv_ = 1.0f;

        if (isAccent_) {
            // Accent triggers short pulse pop
            accentEnv_ = 1.0f;
        } else {
            accentEnv_ = 0.0f;
        }
    } else {
        // On slide notes, the envelope is NOT retriggered, but if accent is turned on on slide,
        // accent pulse can be added
        if (isAccent_) {
            accentEnv_ = 1.0f;
        }
    }
}

void Envelope::noteOff() {
    gate_ = false;
    // On 303, VCF decay continues decaying at the same rate, VCA has simple fast decay on release
}

float Envelope::processNextSample() {
    // Decay envelopes exponentially
    mainEnv_ *= decayCoeff_;
    accentEnv_ *= accentDecayCoeff_;

    return mainEnv_;
}

} // namespace syrebas
