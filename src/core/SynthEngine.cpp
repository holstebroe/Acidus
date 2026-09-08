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

        // 4. VCA Stage: Envelope modulated gain (mainEnvVal for smooth decay, constant note volume)
        // Accent adds dynamic boost punch on accented notes
        float vcaEnv = (isNoteActive_ ? 1.0f : mainEnvVal);
        float vcaGain = vcaEnv * (0.7f + 0.5f * accentSignal * params_.accent);
        float finalSample = filterOut * vcaGain * params_.masterVolume;

        if (outLeft) outLeft[i] = finalSample;
        if (outRight) outRight[i] = finalSample;
    }
}

} // namespace syrebas
