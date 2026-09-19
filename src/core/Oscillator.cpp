#include "Oscillator.hpp"
#include <cmath>
#include <algorithm>

namespace acidus {

namespace {

// PolyBLEP (polynomial band-limited step) residual: a cheap analytic
// correction subtracted/added at a waveform discontinuity to suppress the
// aliasing a naive digital step/ramp would otherwise produce. The real
// oscillator is a continuous analogue ramp/reset circuit with no such
// aliasing at all (EMULATION_GUIDE §55: "Simply running a nonlinear
// oscillator at 44.1 kHz with no oversampling/band-limiting will generate
// aliasing that was not present in the original analogue hardware") -- this
// is a numerical-correctness fix, not a hardware-sourced constant.
inline double polyblep(double t, double dt) {
    if (dt <= 0.0) return 0.0;
    if (t < dt) {
        double x = t / dt;
        return x + x - x * x - 1.0;
    } else if (t > 1.0 - dt) {
        double x = (t - 1.0) / dt;
        return x * x + x + x + 1.0;
    }
    return 0.0;
}

} // namespace

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
        // Falling ramp (1 - 2*phase) wraps upward (-1 -> +1) at phase 0/1;
        // add the PolyBLEP residual there to band-limit that edge.
        double rawSaw = 1.0 - 2.0 * phase_ + polyblep(phase_, phaseInc);
        lpfSawState_ += lpfSawCoeff_ * (rawSaw - lpfSawState_);
        double x = lpfSawState_;
        raw = x - 0.05 * x * x;
    } else {
        double duty = 0.45 + 0.25 * std::exp(-currentFreq_ / 180.0);
        duty = std::min(0.70, std::max(0.45, duty));
        // Two discontinuities: rising edge at phase 0/1 (-1 -> +1), falling
        // edge at phase == duty (+1 -> -1); PolyBLEP-correct both.
        double rawSquare = (phase_ < duty) ? 1.0 : -1.0;
        rawSquare += polyblep(phase_, phaseInc);
        rawSquare -= polyblep(std::fmod(phase_ + 1.0 - duty, 1.0), phaseInc);
        raw = rawSquare * 0.75;
    }

    double hpfOut = couplingAlpha_ * (couplingHpfY1_ + raw - couplingHpfX1_);
    couplingHpfX1_ = raw;
    couplingHpfY1_ = hpfOut;

    return static_cast<float>(hpfOut);
}

} // namespace acidus
