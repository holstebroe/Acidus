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
    // 303 decay range is approximately 200ms at minimum to ~2.5s at max
    decayTimeSec_ = 0.2f + 2.3f * (decayParam * decayParam); // non-linear knob response
    updateCoefficients();
}

void Envelope::updateCoefficients() {
    // T1/e decay coeff per sample
    decayCoeff_ = std::exp(-1.0f / static_cast<float>(sampleRate_ * decayTimeSec_));

    // Accent envelope decay is fast, fixed around 150ms-200ms
    float accentDecaySec = 0.16f;
    accentDecayCoeff_ = std::exp(-1.0f / static_cast<float>(sampleRate_ * accentDecaySec));
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
