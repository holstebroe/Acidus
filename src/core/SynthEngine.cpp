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
    if (noteNumber == currentNote_ || noteNumber < 0) {
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
        float accentCapVal = env_.getAccentCap();
        float accentVcaVal = env_.getAccentVca();
        bool noteAccent = env_.isAccent();

        // 3. Control-Current Domain Summing for Cutoff
        float cNorm = std::min(std::max(params_.cutoff, 0.0f), 1.0f);
        if (params_.cutoff > 1.0f) {
            cNorm = std::min(std::max((params_.cutoff - 200.0f) / 2300.0f, 0.0f), 1.0f);
        }
        float resNorm = std::min(std::max(params_.resonance, 0.0f), 1.0f);
        float envModNorm = std::min(std::max(params_.envMod, 0.0f), 1.0f);
        float accentNorm = std::min(std::max(params_.accent, 0.0f), 1.0f);

        // Base cutoff frequency calculation with Env Mod baseline offset
        float baseCutoff = 200.0f * std::pow(12.5f, cNorm);
        float cutoffOffset = envModNorm * 350.0f;
        float effectiveCutoff = (baseCutoff + cutoffOffset) * (1.0f - (0.15f * resNorm));

        // Dual-gang Resonance pot interaction with Accent Sweep:
        // As Resonance increases, more of the accent signal charges and sweeps through the 1uF cap
        float directAccentPortion = (1.0f - resNorm * 0.7f) * vcfEnvVal;
        float sweepCapPortion = (resNorm * 0.7f) * accentCapVal;
        float accentSweepSignal = directAccentPortion + sweepCapPortion;

        float effectiveEnvMod = noteAccent ? 1.0f : envModNorm;
        float totalEnvContribution = vcfEnvVal * (effectiveEnvMod * 7500.0f);
        float totalAccentContribution = noteAccent ? (accentSweepSignal * accentNorm * 4000.0f) : (sweepCapPortion * accentNorm * 2000.0f);

        float totalCutoff = std::min(effectiveCutoff + totalEnvContribution + totalAccentContribution, 16000.0f);

        // 4. Diode Ladder Filter Stage
        float filterOut = filter_.processSample(rawOsc, totalCutoff, resNorm);

        // 5. VCA Stage & Smoothed Accent Saturation Boost
        // Base VCA signal driven by VEG
        float vcaGain = vcaEnvVal;

        // Accent contribution to VCA control path through 47k + 0.033uF smoothing
        if (accentVcaVal > 0.0001f) {
            vcaGain += accentVcaVal * accentNorm * 0.8f;
        }

        float vcaSignal = filterOut * vcaGain;

        if (noteAccent || accentVcaVal > 0.001f) {
            if (vcaSignal > 0.0f) {
                vcaSignal = std::tanh(vcaSignal * 1.2f);
            } else {
                vcaSignal = std::tanh(vcaSignal * 0.9f);
            }
        }

        float finalSample = vcaSignal * params_.masterVolume;

        if (outLeft) outLeft[i] = finalSample;
        if (outRight) outRight[i] = finalSample;
    }
}

} // namespace syrebas
