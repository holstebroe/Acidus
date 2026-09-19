#include "Oscillator.hpp"
#include <cmath>
#include <algorithm>

namespace acidus {

Oscillator::Oscillator() {
    setSampleRate(44100.0);
}

void Oscillator::setSampleRate(double sampleRate) {
    sampleRate_ = sampleRate;

    double slideTimeSec = 0.060;
    slideCoeff_ = std::exp(-1.0 / (sampleRate_ * slideTimeSec));

    double fcLpf = 14000.0;
    lpfSawCoeff_ = 1.0 - std::exp(-2.0 * 3.14159265358979323846 * fcLpf / sampleRate_);

    recomputeCouplingAlpha();
    resetFilterStates();
}

void Oscillator::resetFilterStates() {
    lpfSawState_ = 0.0;
    couplingHpfX1_ = 0.0;
    couplingHpfY1_ = 0.0;
}

void Oscillator::noteOn(int noteNumber, bool slide) {
    double freq = noteToFreq(noteNumber);
    targetFreq_ = freq;

    if (!slide) {
        currentFreq_ = targetFreq_;
        isSliding_ = false;
    } else {
        isSliding_ = true;
    }
}

void Oscillator::noteOff() {
    isSliding_ = false;
}

float Oscillator::processNextSample() {
    if (isSliding_) {
        currentFreq_ = targetFreq_ + (currentFreq_ - targetFreq_) * slideCoeff_;
        if (std::abs(currentFreq_ - targetFreq_) < 0.001) {
            currentFreq_ = targetFreq_;
            isSliding_ = false;
        }
    } else {
        currentFreq_ = targetFreq_;
    }

    double phaseInc = currentFreq_ / sampleRate_;
    phase_ += phaseInc;
    if (phase_ >= 1.0) {
        phase_ -= 1.0;
    }

    double raw = 0.0;
    if (waveform_ == Waveform::Saw) {
        double rawSaw = 1.0 - 2.0 * phase_;
        lpfSawState_ += lpfSawCoeff_ * (rawSaw - lpfSawState_);
        double x = lpfSawState_;
        raw = x - 0.05 * x * x;
    } else {
        double duty = 0.45 + 0.25 * std::exp(-currentFreq_ / 180.0);
        duty = std::min(0.70, std::max(0.45, duty));
        raw = (phase_ < duty) ? 0.75 : -0.75;
    }

    double hpfOut = couplingAlpha_ * (couplingHpfY1_ + raw - couplingHpfX1_);
    couplingHpfX1_ = raw;
    couplingHpfY1_ = hpfOut;

    return static_cast<float>(hpfOut);
}

} // namespace acidus
