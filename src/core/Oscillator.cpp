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
    // corner/pitch-tracking law NOT sourced - see
    // TB303_RESEARCH_COMPENDIUM.md Section 5 and TB303_PARAMETER_CONFIDENCE.md).
    //
    // Previously this HPF was applied to the square wave only, fixed at
    // 150 Hz. Research found neither claim holds up: Olney's analysis of
    // reference recordings found that a high-passed square resembles the
    // *reference saw*, and a high-passed saw resembles the *reference
    // square*, with the effective corner sitting around 80-115 Hz and
    // tracking pitch - i.e. this is a property of the shared oscillator/VCF
    // coupling-capacitor network acting on *both* waveforms after the
    // waveform selector, not a square-specific filter, and not at 150 Hz.
    //
    // Implemented here as a single one-pole HPF applied to whichever
    // waveform is currently selected (both saw and square pass through it -
    // the saw's own 14 kHz LPF/quadratic-bend stage above is unaffected and
    // still saw-only, since that part of the model isn't in question here).
    // The corner tracks pitch using the same exponential-in-frequency shape
    // already used for the duty-cycle curve above (reused for consistency,
    // not because it's independently sourced for this stage): it swings
    // +-15% around the tunable `couplingBaseHz_` reference, low at low
    // pitch and high at high pitch, which lands close to the observed
    // ~80-115 Hz region when `couplingBaseHz_` is left at its ~98 Hz
    // default. Exposed as a CLAP parameter ("Osc Coupling Freq") so the
    // reference corner (and therefore the whole tracked range) can be
    // retuned by ear; plausible range 70-120 Hz.
    double pitchFactor = 1.0 - std::exp(-currentFreq_ / 180.0); // 0 (low pitch) .. ~1 (high pitch)
    double couplingHz = static_cast<double>(couplingBaseHz_) * (0.85 + 0.30 * pitchFactor);
    couplingHz = std::min(std::max(couplingHz, 40.0), 200.0);
    double couplingAlpha = std::exp(-2.0 * 3.14159265358979323846 * couplingHz / sampleRate_);

    double hpfOut = couplingAlpha * (couplingHpfY1_ + raw - couplingHpfX1_);
    couplingHpfX1_ = raw;
    couplingHpfY1_ = hpfOut;

    return static_cast<float>(hpfOut);
}

} // namespace syrebas
