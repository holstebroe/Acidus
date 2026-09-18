#include "Oscillator.hpp"
#include <cmath>
#include <algorithm>

namespace syrebas {

Oscillator::Oscillator() {
    setSampleRate(44100.0);
}

void Oscillator::setSampleRate(double sampleRate) {
    sampleRate_ = sampleRate;

    // Nominal analogue slide lag time constant ~60 ms. CONFIRMED (Whittle's
    // Devil Fish manual states this explicitly as the *stock* value, before
    // the mod extends it) - see TB303_RESEARCH_COMPENDIUM.md Section 3.
    double slideTimeSec = 0.060;
    slideCoeff_ = std::exp(-1.0 / (sampleRate_ * slideTimeSec));

    // 1-pole LPF at 14 kHz for sawtooth peak rounding. BEST GUESS / UNSOURCED -
    // see the member comment in Oscillator.hpp.
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
        // Hard jump to new frequency without phase reset (continuous analogue VCO)
        currentFreq_ = targetFreq_;
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
    // Pitch Glide via 1-pole lag filter when sliding
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
        // Negative-going sawtooth: 1.0 - 2.0 * phase_
        double rawSaw = 1.0 - 2.0 * phase_;

        // 1-pole LPF at 14 kHz to round sharp peaks
        lpfSawState_ += lpfSawCoeff_ * (rawSaw - lpfSawState_);

        // Mild quadratic distortion: f(x) = x - 0.05 * x^2. BEST GUESS / UNSOURCED,
        // see Oscillator.hpp.
        double x = lpfSawState_;
        raw = x - 0.05 * x * x;
    } else {
        // Pitch-dependent duty cycle. CONFIRMED direction/endpoints (Whittle, via
        // Olney): ~45% at high pitch widening to ~70-71% at low pitch. The
        // exponential-in-frequency shape between those endpoints (180 Hz time
        // constant below) is this project's own curve-fit, not itself sourced.
        double duty = 0.45 + 0.25 * std::exp(-currentFreq_ / 180.0);
        duty = std::min(0.70, std::max(0.45, duty));

        raw = (phase_ < duty) ? 0.75 : -0.75;
    }

    // --- Shared saw/square coupling-network HPF ---------------------------
    // Confidence: ESTIMATE (mechanism/region CONFIRMED by research, exact
    // corner NOT sourced - see TB303_RESEARCH_COMPENDIUM.md Section 5 and
    // TB303_PARAMETER_CONFIDENCE.md).
    //
    // Previously this HPF was applied to the square wave only, fixed at
    // 150 Hz - wrong on both counts: Olney's analysis of reference
    // recordings found that a high-passed square resembles the reference
    // saw and vice versa, i.e. this is a property of the shared oscillator/
    // VCF coupling-capacitor network acting on *both* waveforms after the
    // waveform selector, not a square-specific filter, and Olney's
    // reconstruction used corners around 80-115 Hz, not 150 Hz.
    //
    // 2026-09 correction: this project tried making the corner track pitch
    // (swinging +-15% around a ~98 Hz reference, per the 80-115 Hz figure
    // above), which caused a large, audible regression - at ordinary TB-303
    // bass pitches (e.g. a 65 Hz C2, a 49 Hz G1) that corner sat AT OR ABOVE
    // the fundamental, so the "coupling network" was instead cutting into or
    // removing the played note's own fundamental. Cross-checked against
    // RobinSchmidt/Open303, a well-regarded, independently ear/measurement-
    // tuned open-source TB-303 emulation: its equivalent stage
    // (`highpass1.setCutoff(44.486)`, its "pre-filter highpass" applied to
    // the oscillator signal ahead of the main VCF) uses a FIXED corner, not
    // pitch-tracking, at roughly half of even the low end of the 80-115 Hz
    // figure. Reverted to a single, fixed one-pole HPF (both saw and square
    // pass through it; the saw's own 14 kHz LPF/quadratic-bend stage above
    // is unaffected) at a corner well below normal TB-303 playing range.
    // Plausible range 30-60 Hz, default 44.5 Hz (Open303's exact value),
    // exposed as the CLAP parameter "Osc Coupling Freq".
    double hpfOut = couplingAlpha_ * (couplingHpfY1_ + raw - couplingHpfX1_);
    couplingHpfX1_ = raw;
    couplingHpfY1_ = hpfOut;

    return static_cast<float>(hpfOut);
}

} // namespace syrebas
