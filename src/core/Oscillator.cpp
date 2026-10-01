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

    // Slide: C35 0.22 uF charged from the R-2R DAC's 100 k Thevenin
    // resistance, tau = 22 ms, applied to the pitch CV in volts (1 V/oct),
    // i.e. linear in semitones along an exponential approach
    // (TB303_REFERENCE.md §6). Open303's 60 ms lag on frequency in Hz is a
    // simplification: slower, and asymmetric between up and down.
    const double slideTauSec = 0.022;
    slideCoeff_ = std::exp(-1.0 / (sampleRate_ * slideTauSec));

    recomputeSawLpfCoeff();

    recomputeCouplingAlpha();
    resetFilterStates();
}

void Oscillator::resetFilterStates() {
    lpfSawState_ = 0.0;
    couplingHpfX1_ = 0.0;
    couplingHpfY1_ = 0.0;
}

void Oscillator::noteOn(int noteNumber, bool slide) {
    heldNote_ = noteNumber;
    targetPitch_ = pitchForNote(noteNumber);

    if (!slide) {
        // Slide off: C35 is driven straight from the op-amp, effectively
        // instant.
        currentPitch_ = targetPitch_;
        isSliding_ = false;
    } else {
        isSliding_ = true;
    }
}

void Oscillator::noteOff() {
    // The gate has no say over the pitch CV: a slide in progress keeps
    // settling on the RC after note-off (no snap to the target), and the
    // next un-slid note jumps.
}

float Oscillator::processNextSample() {
    if (isSliding_) {
        currentPitch_ = targetPitch_ + (currentPitch_ - targetPitch_) * slideCoeff_;
        if (std::abs(currentPitch_ - targetPitch_) < 1e-5) {
            currentPitch_ = targetPitch_;
            isSliding_ = false;
        }
    } else {
        currentPitch_ = targetPitch_;
    }
    if (currentPitch_ != freqPitch_) {
        freqPitch_ = currentPitch_;
        currentFreq_ = pitchToFreq(currentPitch_);
    }

    // Keep the increment below Nyquist and the phase in [0, 1) even at the
    // top MIDI notes on a low sample rate.
    double phaseInc = std::min(currentFreq_ / sampleRate_, 0.49);
    phase_ += phaseInc;
    if (phase_ >= 1.0) {
        phase_ -= std::floor(phase_);
    }

    double raw = 0.0;
    if (waveform_ == Waveform::Saw) {
        // Falling ramp (1 - 2*phase) wraps upward (-1 -> +1) at phase 0/1;
        // add the PolyBLEP residual there to band-limit that edge.
        double rawSaw = 1.0 - 2.0 * phase_ + polyblep(phase_, phaseInc);
        lpfSawState_ += lpfSawCoeff_ * (rawSaw - lpfSawState_);
        double x = lpfSawState_;
        raw = x - sawShape_ * x * x;
    } else {
        double duty = 0.45 + squareDutyDepth_ * std::exp(-currentFreq_ / 180.0);
        duty = std::min(0.70, std::max(0.45, duty));
        // Two discontinuities: rising edge at phase 0/1 (-1 -> +1), falling
        // edge at phase == duty (+1 -> -1); PolyBLEP-correct both.
        double rawSquare = (phase_ < duty) ? 1.0 : -1.0;
        rawSquare += polyblep(phase_, phaseInc);
        rawSquare -= polyblep(std::fmod(phase_ + 1.0 - duty, 1.0), phaseInc);
        raw = rawSquare * squareLevel_;
    }

    double hpfOut = couplingAlpha_ * (couplingHpfY1_ + raw - couplingHpfX1_);
    couplingHpfX1_ = raw;
    couplingHpfY1_ = hpfOut;

    return static_cast<float>(hpfOut);
}

} // namespace acidus
