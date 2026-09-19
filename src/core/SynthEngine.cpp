#include "SynthEngine.hpp"
#include <algorithm>

namespace acidus {

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
    bool isSlide = isNoteActive_;
    bool isAccent = (velocity >= 0.8f);
    accentLevel_ = isAccent ? 1.0f : 0.0f;

    currentNote_ = noteNumber;
    isNoteActive_ = true;

    osc_.setWaveform(params_.waveform);
    osc_.noteOn(noteNumber, isSlide);
    env_.setFaithfulAccentDecay(true);
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
    env_.setFaithfulAccentDecay(true);
    env_.setDecay(params_.decay);

    osc_.setCouplingHz(params_.oscCouplingHz);
    filter_.setResCouplingHz(params_.resCouplingHz);
    filter_.setFeedbackGainCeiling(params_.filterFeedbackGain);
    env_.setVegDecaySec(params_.vegDecaySec);
    env_.setVcaGateOffMs(params_.vcaGateOffMs);
    env_.setVcaGateOffAccentMs(params_.vcaGateOffAccentMs);

    for (int i = 0; i < numFrames; ++i) {
        if (!env_.isActive() && !isNoteActive_) {
            if (outLeft) outLeft[i] = 0.0f;
            if (outRight) outRight[i] = 0.0f;
            continue;
        }

        float rawOsc = osc_.processNextSample();

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

        float cTaper = cNorm * cNorm;
        float envModTaper = envModNorm;

        float cv_base = 3.64385f * cTaper;
        float cv_offset = envModTaper * 0.80735f;

        float effectiveEnvMod = noteAccent ? 1.0f : envModNorm;
        float effectiveEnvModTaper = effectiveEnvMod;
        float cv_envmod = effectiveEnvModTaper * vcfEnvVal * 3.5f;

        float directAccentPortion = (1.0f - resNorm * 0.7f) * vcfEnvVal;
        float sweepCapPortion = (resNorm * 0.7f) * accentCapVal;
        float accentSweepSignal = directAccentPortion + sweepCapPortion;

        float cv_accent = noteAccent ? (accentNorm * accentSweepSignal * 1.5f) : (accentNorm * sweepCapPortion * 0.75f);

        float cv_total = cv_base + cv_offset + cv_envmod + cv_accent;

        float resBleed = std::min(std::max(params_.resCutoffBleed, 0.0f), 1.0f);
        float effectiveCutoff = 200.0f * std::pow(2.0f, cv_total) * (1.0f - (resBleed * resNorm));
        float totalCutoff = std::min(std::max(effectiveCutoff, 20.0f), 15000.0f);

        float filterOut = filter_.processSample(rawOsc, totalCutoff, resNorm);

        float vcaGain = vcaEnvVal + 0.45f * vcfEnvVal;
        if (noteAccent) {
            vcaGain += accentVcaVal * accentNorm * 0.8f;
        }

        float xVal = filterOut * vcaGain;
        float vcaSignal = (xVal > 0.0f) ? std::tanh(xVal * 1.1f) : std::tanh(xVal * 0.9f);

        float finalSample = vcaSignal * params_.masterVolume;

        if (outLeft) outLeft[i] = finalSample;
        if (outRight) outRight[i] = finalSample;
    }
}

} // namespace acidus
