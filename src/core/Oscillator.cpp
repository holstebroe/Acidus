#include "Oscillator.hpp"
#include <cmath>

namespace syrebas {

Oscillator::Oscillator() {
    setSampleRate(44100.0);
}

void Oscillator::setSampleRate(double sampleRate) {
    sampleRate_ = sampleRate;
    // 303 slide time is roughly 50ms - 60ms constant time constant
    // Exponential approach coefficient per sample at current sampleRate
    double slideTimeSec = 0.055;
    slideCoeff_ = std::exp(-1.0 / (sampleRate_ * slideTimeSec));
}

void Oscillator::noteOn(int noteNumber, bool slide) {
    double freq = noteToFreq(noteNumber);
    targetFreq_ = freq;

    if (!slide) {
        // Hard jump to new frequency and reset phase for fresh note start
        currentFreq_ = targetFreq_;
        phase_ = 0.0;
        isSliding_ = false;
    } else {
        // Slide / portamento: glide smoothly from currentFreq_ to targetFreq_
        isSliding_ = true;
    }
}

void Oscillator::noteOff() {
    // Note off does not change pitch, release handled by envelope/voice
}

float Oscillator::processNextSample() {
    // Smoothly interpolate current pitch towards target pitch if sliding
    if (std::abs(currentFreq_ - targetFreq_) > 0.01) {
        currentFreq_ = targetFreq_ + (currentFreq_ - targetFreq_) * slideCoeff_;
    } else {
        currentFreq_ = targetFreq_;
        isSliding_ = false;
    }

    double phaseInc = currentFreq_ / sampleRate_;
    phase_ += phaseInc;
    if (phase_ >= 1.0) {
        phase_ -= 1.0;
    }

    float out = 0.0f;
    if (waveform_ == Waveform::Saw) {
        // 303 Sawtooth: Unipolar/Bipolar saw with mild curve
        // Standard saw is 1.0 - 2.0 * phase_
        out = static_cast<float>(1.0 - 2.0 * phase_);
    } else {
        // 303 Square: Derived from saw/integrated pulse waveform
        // Square wave with high harmonic content (~50% duty)
        out = (phase_ < 0.5) ? 1.0f : -1.0f;
    }

    return out;
}

} // namespace syrebas
