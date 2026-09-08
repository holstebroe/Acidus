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

    // VCA gate attack time ~3ms, release time ~35ms
    vcaAttackCoeff_ = std::exp(-1.0f / static_cast<float>(sampleRate_ * 0.003));
    vcaReleaseCoeff_ = std::exp(-1.0f / static_cast<float>(sampleRate_ * 0.035));
}

void SynthEngine::reset() {
    filter_.reset();
    currentNote_ = -1;
    isNoteActive_ = false;
    accentLevel_ = 0.0f;
    vcaGateEnv_ = 0.0f;
}

void SynthEngine::noteOn(int noteNumber, float velocity) {
    // Check if slide condition (a note is currently active and not finished)
    bool isSlide = isNoteActive_;

    // Accent is triggered by high velocity (max velocity or >= 0.8 / 100+ MIDI velocity)
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
        float mainEnvVal = env_.getMainEnv();
        float accentEnvVal = env_.getAccentEnv();

        // Envelope mod contribution
        float envModSignal = mainEnvVal * params_.envMod;

        // Accent contribution (drives envelope bump and VCA accent gain)
        float accentSignal = accentEnvVal * params_.accent;

        // 3. Process physical 303 filter engine
        float filterOut = filter_.processSample(rawOsc, params_.cutoff, params_.resonance, envModSignal, accentSignal);

        // 4. VCA Stage: Smooth gate envelope follower to eliminate Note On / Off clicks
        float targetGate = isNoteActive_ ? 1.0f : 0.0f;
        if (targetGate > vcaGateEnv_) {
            vcaGateEnv_ = targetGate + (vcaGateEnv_ - targetGate) * vcaAttackCoeff_;
        } else {
            vcaGateEnv_ = targetGate + (vcaGateEnv_ - targetGate) * vcaReleaseCoeff_;
        }

        // Constant note volume modulated by VCA Gate follower + accent dynamic punch
        float vcaGain = vcaGateEnv_ * (0.7f + 0.5f * accentSignal * params_.accent);
        float finalSample = filterOut * vcaGain * params_.masterVolume;

        if (outLeft) outLeft[i] = finalSample;
        if (outRight) outRight[i] = finalSample;
    }
}

} // namespace syrebas
