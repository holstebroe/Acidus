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

        float cNorm = std::min(std::max(params_.cutoff, 0.0f), 1.0f);
        float resNorm = std::min(std::max(params_.resonance, 0.0f), 1.0f);
        float envModNorm = std::min(std::max(params_.envMod, 0.0f), 1.0f);
        float accentNorm = std::min(std::max(params_.accent, 0.0f), 1.0f);

        float filterOut = 0.0f;
        float vcaSignal = 0.0f;

        if (params_.mode == EmulationMode::Accurate) {
            // --- ACCURATE MODE: Current-Domain Control Summing & Coupled Diode Ladder ---
            // Cutoff audio taper (50 kOhm Log/A pot)
            float cTaper = cNorm * cNorm;
            float envModTaper = envModNorm * envModNorm;

            // Control Currents summing (Section 16, 83)
            // I_cutoff = I_static + I_env + I_accent + I_bias
            float i_static = 0.05f + cTaper * 0.80f; // Static cutoff bias
            float i_env = envModTaper * vcfEnvVal * 1.2f;

            // Dual-gang Resonance section 2 controls Accent Sweep signal delivery (Section 14, 27)
            float directPortion = (1.0f - resNorm * 0.6f) * vcfEnvVal;
            float capPortion = (resNorm * 0.6f) * accentCapVal;
            float accentSweepSig = directPortion + capPortion;

            float i_accent = noteAccent ? (accentNorm * accentSweepSig * 1.0f) : (accentNorm * capPortion * 0.4f);

            float i_total = i_static + i_env + i_accent;

            // Exponential voltage-to-current conversion for filter cutoff frequency
            float cutoffHz = 200.0f * std::exp(i_total * 2.80f);
            cutoffHz = std::min(std::max(cutoffHz, 20.0f), 16000.0f);

            // Coupled diode ladder filter simulation
            filterOut = filter_.processAccurateSample(rawOsc, cutoffHz, resNorm);

            // BA662 VCA Model with control current summing (Section 23, 26)
            float vcaGain = vcaEnvVal;
            if (noteAccent) {
                vcaGain += accentVcaVal * accentNorm * 0.7f;
            }

            // Asymmetric BA662 VCA saturation (Section 44)
            float xVal = filterOut * vcaGain;
            if (xVal > 0.0f) {
                vcaSignal = std::tanh(xVal * 1.2f) / 1.2f;
            } else {
                vcaSignal = std::tanh(xVal * 1.0f);
            }
        } else {
            // --- SIMPLIFIED MODE ---
            float cTaper = cNorm * cNorm;
            float envModTaper = envModNorm * envModNorm;

            float cv_base = 3.64385f * cTaper;
            float cv_offset = envModTaper * 0.80735f;

            float effectiveEnvMod = noteAccent ? (envModNorm + (1.0f - envModNorm) * accentNorm) : envModNorm;
            float effectiveEnvModTaper = effectiveEnvMod * effectiveEnvMod;
            float cv_envmod = effectiveEnvModTaper * vcfEnvVal * 3.5f;

            float directAccentPortion = (1.0f - resNorm * 0.7f) * vcfEnvVal;
            float sweepCapPortion = (resNorm * 0.7f) * accentCapVal;
            float accentSweepSignal = directAccentPortion + sweepCapPortion;

            float cv_accent = noteAccent ? (accentNorm * accentSweepSignal * 1.5f) : (accentNorm * sweepCapPortion * 0.75f);
            float cv_total = cv_base + cv_offset + cv_envmod + cv_accent;

            float effectiveCutoff = 200.0f * std::pow(2.0f, cv_total) * (1.0f - (0.15f * resNorm));
            float totalCutoff = std::min(std::max(effectiveCutoff, 20.0f), 14000.0f);

            filterOut = filter_.processSample(rawOsc, totalCutoff, resNorm);

            float vcaGain = vcaEnvVal;
            if (noteAccent && accentVcaVal > 0.0001f) {
                vcaGain += accentVcaVal * accentNorm * 0.8f;
            }

            vcaSignal = filterOut * vcaGain;
            if (noteAccent && accentNorm > 0.01f) {
                float satDrive = 1.0f + accentNorm * 0.25f;
                if (vcaSignal > 0.0f) {
                    vcaSignal = std::tanh(vcaSignal * satDrive);
                } else {
                    vcaSignal = std::tanh(vcaSignal * (satDrive * 0.85f));
                }
            }
        }

        float finalSample = vcaSignal * params_.masterVolume;

        if (outLeft) outLeft[i] = finalSample;
        if (outRight) outRight[i] = finalSample;
    }
}

} // namespace syrebas
