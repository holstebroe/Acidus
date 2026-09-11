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
    env_.noteOn(isAccent, isSlide, params_.accent);
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
            cNorm = std::min(std::max((params_.cutoff - 300.0f) / 9700.0f, 0.0f), 1.0f);
        }
        float resNorm = std::min(std::max(params_.resonance, 0.0f), 1.0f);
        float envModNorm = std::min(std::max(params_.envMod, 0.0f), 1.0f);
        float accentNorm = std::min(std::max(params_.accent, 0.0f), 1.0f);

        // Audio pot tapers (50 kOhm Audio / A taper) for Cutoff and Env Mod knobs
        float cTaper = cNorm * cNorm;
        float envModTaper = envModNorm * envModNorm;

        // Base cutoff knob CV range: 200 Hz to 2500 Hz (~3.64385 octaves)
        float cv_base = 3.64385f * cTaper;

        // Env Mod baseline offset (+350 Hz / +0.80735 octaves at max Env Mod)
        float cv_offset = envModTaper * 0.80735f;

        // Effective Env Mod depth for this sample (boosted on accent, scaled by accentNorm)
        float effectiveEnvMod = noteAccent ? (envModNorm + (1.0f - envModNorm) * accentNorm) : envModNorm;
        float effectiveEnvModTaper = effectiveEnvMod * effectiveEnvMod;
        float cv_envmod = effectiveEnvModTaper * vcfEnvVal * 3.5f;

        // Dual-gang Resonance pot interaction with Accent Sweep:
        float directAccentPortion = (1.0f - resNorm * 0.7f) * vcfEnvVal;
        float sweepCapPortion = (resNorm * 0.7f) * accentCapVal;
        float accentSweepSignal = directAccentPortion + sweepCapPortion;

        // Accent Sweep CV contribution to cutoff
        float cv_accent = noteAccent ? (accentNorm * accentSweepSignal * 1.5f) : (accentNorm * sweepCapPortion * 0.75f);

        // Control Voltage Summing in control-current (exponential octave) domain
        float cv_total = cv_base + cv_offset + cv_envmod + cv_accent;

        // Convert CV to frequency with Resonance CV Bleed (up to 15% reduction)
        float effectiveCutoff = 200.0f * std::pow(2.0f, cv_total) * (1.0f - (0.15f * resNorm));
        float totalCutoff = std::min(std::max(effectiveCutoff, 20.0f), 14000.0f);

        // 4. Diode Ladder Filter Stage
        float filterOut = filter_.processSample(rawOsc, totalCutoff, resNorm);

        // 5. VCA Stage & Smoothed Accent Saturation Boost
        float vcaGain = vcaEnvVal;

        if (noteAccent && accentVcaVal > 0.0001f) {
            vcaGain += accentVcaVal * accentNorm * 0.8f;
        }

        float vcaSignal = filterOut * vcaGain;

        if (noteAccent && accentNorm > 0.01f) {
            float satDrive = 1.0f + accentNorm * 0.25f;
            if (vcaSignal > 0.0f) {
                vcaSignal = std::tanh(vcaSignal * satDrive);
            } else {
                vcaSignal = std::tanh(vcaSignal * (satDrive * 0.85f));
            }
        }

        float finalSample = vcaSignal * params_.masterVolume;

        if (outLeft) outLeft[i] = finalSample;
        if (outRight) outRight[i] = finalSample;
    }
}

} // namespace syrebas
