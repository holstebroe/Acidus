#include "SynthEngine.hpp"
#include <algorithm>

namespace syrebas {

SynthEngine::SynthEngine() {
    setSampleRate(44100.0);
}

void SynthEngine::setSampleRate(double sampleRate) {
    sampleRate_ = sampleRate;
    osc_.setSampleRate(sampleRate_);
    env_.setSampleRate(sampleRate_);
    filter_.setSampleRate(sampleRate_);
}

void SynthEngine::reset() {
    filter_.reset();
    currentNote_ = -1;
    isNoteActive_ = false;
    accentLevel_ = 0.0f;
}

void SynthEngine::noteOn(int noteNumber, float velocity) {
    // Check if slide condition (a note is currently active and not finished)
    bool isSlide = isNoteActive_;

    // Accent is triggered by velocity >= 0.8
    bool isAccent = (velocity >= 0.8f);
    accentLevel_ = isAccent ? 1.0f : 0.0f;

    currentNote_ = noteNumber;
    isNoteActive_ = true;

    osc_.setWaveform(params_.waveform);
    osc_.noteOn(noteNumber, isSlide);
    env_.setDecay(params_.decay);
    env_.noteOn(isAccent, isSlide);
}

void SynthEngine::noteOff(int noteNumber) {
    if (noteNumber == currentNote_) {
        isNoteActive_ = false;
        osc_.noteOff();
        env_.noteOff();
    }
}

void SynthEngine::processAudio(float* outLeft, float* outRight, int numFrames) {
    osc_.setWaveform(params_.waveform);
    env_.setDecay(params_.decay);

    for (int i = 0; i < numFrames; ++i) {
        if (!env_.isActive() && !isNoteActive_) {
            if (outLeft) outLeft[i] = 0.0f;
            if (outRight) outRight[i] = 0.0f;
            continue;
        }

        // 1. Generate oscillator signal
        float rawOsc = osc_.processNextSample();

        // 2. Process envelope sample
        env_.processNextSample();
        float vcfEnvVal = env_.getVcfEnv();
        float vcaEnvVal = env_.getVcaEnv();
        bool noteAccent = env_.isAccent();

        // 3. Calculate Cutoff Frequency & Interactive Knob Scaling
        // Cutoff Pot: Maps exponentially from 200 Hz (0.0) to 2.5 kHz (1.0) under zero envelope modulation
        float cNorm = std::min(std::max(params_.cutoff, 0.0f), 1.0f);
        if (params_.cutoff > 1.0f) {
            // Backward compatibility if cutoff parameter passed in Hz (e.g. 200Hz - 2500Hz)
            cNorm = std::min(std::max((params_.cutoff - 200.0f) / 2300.0f, 0.0f), 1.0f);
        }
        float baseCutoff = 200.0f * std::pow(12.5f, cNorm); // 200 * (2500 / 200)^cNorm = 200 * (12.5)^cNorm

        // Resonance / Cutoff Interaction: Negative control-voltage bleed
        // Effective_Cutoff = Base_Cutoff * (1.0 - (0.15 * Resonance))
        float resNorm = std::min(std::max(params_.resonance, 0.0f), 1.0f);
        float effectiveCutoff = baseCutoff * (1.0f - (0.15f * resNorm));

        // Env Mod Pot: Scales peak envelope depth up to 7.5 kHz
        float envModNorm = std::min(std::max(params_.envMod, 0.0f), 1.0f);
        // Accent Logic: Force Env Mod depth to 100% for accent notes
        float effectiveEnvMod = noteAccent ? 1.0f : envModNorm;

        // VCF Envelope sweep up to 7.5 kHz
        float totalCutoff = effectiveCutoff + vcfEnvVal * (effectiveEnvMod * 7500.0f);

        // 4. Diode Ladder Filter Stage
        float filterOut = filter_.processSample(rawOsc, totalCutoff, resNorm);

        // 5. VCA Stage & Accent Saturation Boost
        // Base VCA signal driven by VCA envelope (3ms attack, 4s decay)
        float vcaSignal = filterOut * vcaEnvVal;

        // Accent Logic: Apply +6dB gain boost (+2.0x) into VCA stage driving asymmetric / heavy tanh clipping
        if (noteAccent) {
            float accentAmount = std::min(std::max(params_.accent, 0.0f), 1.0f);
            // +6dB boost scaled by Accent knob amount: gain factor 1.0 + (2.0 - 1.0)*accentAmount = 1.0 + accentAmount
            float boostFactor = 1.0f + accentAmount;
            float boosted = vcaSignal * boostFactor;
            // Asymmetric VCA clipping / heavy saturation
            if (boosted > 0.0f) {
                vcaSignal = std::tanh(boosted * 1.2f);
            } else {
                vcaSignal = std::tanh(boosted * 0.9f);
            }
        }

        float finalSample = vcaSignal * params_.masterVolume;

        if (outLeft) outLeft[i] = finalSample;
        if (outRight) outRight[i] = finalSample;
    }
}

} // namespace syrebas
