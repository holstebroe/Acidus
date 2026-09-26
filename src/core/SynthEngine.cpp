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
    distortion_.setSampleRate(sampleRate_);
}

void SynthEngine::reset() {
    filter_.reset();
    distortion_.reset();
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

    osc_.setTuningCents(params_.tuningCents);
    osc_.setCouplingHz(params_.oscCouplingHz);
    filter_.setResCouplingHz(params_.resCouplingHz);
    filter_.setFeedbackGainCeiling(params_.filterFeedbackGain);
    filter_.setPostFilterHpHz(params_.filterPostHpHz);
    filter_.setNotchFreqHz(params_.filterNotchHz);
    filter_.setNotchBandwidthHz(params_.filterNotchBandwidthHz);
    filter_.setAllpassFreqHz(params_.filterAllpassHz);
    filter_.setInputCouplingHz(params_.filterInputCouplingHz);
    filter_.setOutputCouplingHz(params_.filterOutputCouplingHz);
    filter_.setCapScale1(params_.filterCapScale1);
    filter_.setCapScale2(params_.filterCapScale2);
    filter_.setCapScale3(params_.filterCapScale3);
    filter_.setCapScale4(params_.filterCapScale4);
    filter_.setLadderInputScale(params_.filterLadderInputScale);
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

        // Env Mod's depth is set purely by the Env Mod pot, on every note --
        // Accent never bypasses it. The real circuit's accent contribution
        // to the filter is a separate, parallel Accent Sweep current
        // injected at the same summing node (cv_accent, below), not a
        // forced 100% Env Mod depth (TB303_RESEARCH_COMPENDIUM.md §9:
        // Open303 "never overrides its envelope scaler on accent -- it only
        // adds a separate, smaller, purely-accent-driven term on top").
        float cv_envmod = envModTaper * vcfEnvVal * 3.5f;

        // Accent Sweep circuit: the Resonance pot's second gang blends
        // between the MEG reaching the filter almost directly (low
        // Resonance -> sharp, fast filter "kick") and through the smoothed
        // 1uF-capacitor path (high Resonance -> curved, delayed "wow"/
        // "wapp"); the wiper should be able to reach (near-)all-direct or
        // (near-)all-capacitor at its travel extremes (compendium §9).
        float directAccentPortion = (1.0f - resNorm) * vcfEnvVal;
        float sweepCapPortion = resNorm * accentCapVal;
        float accentSweepSignal = directAccentPortion + sweepCapPortion;

        // The whole Accent Sweep path is gated by the same per-step accent
        // switch as the MEG-decay override and the accent-VCA path (§9:
        // "all sourced from the MEG through a switch that is only closed
        // during accented steps") -- zero contribution to the filter on a
        // non-accented note. The capacitor's own charge persists and decays
        // between notes regardless (Envelope::accentCap_), ready for the
        // next accented step -- that's what produces the documented rising
        // peaks across consecutive accents (§9/§30).
        float cv_accent = noteAccent ? (accentNorm * accentSweepSignal * 1.5f) : 0.0f;

        float cv_total = cv_base + cv_offset + cv_envmod + cv_accent;

        float effectiveCutoff = 200.0f * std::pow(2.0f, cv_total);
        float totalCutoff = std::min(std::max(effectiveCutoff, 20.0f), 15000.0f);

        float filterOut = filter_.processSample(rawOsc, totalCutoff, resNorm);

        // MEG->VCA bleed on accent goes through the 47kOhm/0.033uF softening
        // network (EMULATION_REFERENCE Sec26) -- that's accentVcaVal, an RC
        // lowpass of the MEG whose target snaps to 0 on gate-off (Envelope.cpp),
        // so it already decays fast (~1.55ms) and scales with the Accent knob.
        // There is no second, undamped MEG->VCA term in the docs: adding raw
        // vcfEnvVal here duplicated that path but skipped both the RC
        // softening and the gate-off snap, since the MEG free-runs its own
        // Decay-controlled decay independent of note-off -- that's what left
        // accented notes with no audible release at all.
        float vcaControl = vcaEnvVal;
        if (noteAccent) {
            vcaControl += accentVcaVal * accentNorm * 0.8f;
        }

        // BA662-style transconductance VCA: model the amplifier's own gain
        // as a saturating function of its control current, not a hard
        // linear multiply into the output stage (EMULATION_REFERENCE §23:
        // "should not simply be output = input*envelope ... nonlinear
        // current-to-gain behavior, saturation at high control levels,
        // especially for high-level accented material"). The gain stage
        // and the following signal-path buffer stage are two distinct
        // nonlinearities, not one shared tanh.
        float driveNorm = vcaControl * params_.vcaGainSaturationDrive;
        float vcaGain = (params_.vcaGainSaturationDrive > 0.0f)
                            ? std::tanh(driveNorm) / std::tanh(std::max(params_.vcaGainSaturationDrive, 1e-6f))
                            : vcaControl;

        float xVal = filterOut * vcaGain;
        float vcaSignal = (xVal > 0.0f) ? std::tanh(xVal * 1.1f) : std::tanh(xVal * 0.9f);

        float drivenSignal = distortion_.processSample(vcaSignal, params_.drive);
        float finalSample = drivenSignal * params_.masterVolume;

        if (outLeft) outLeft[i] = finalSample;
        if (outRight) outRight[i] = finalSample;
    }
}

} // namespace acidus
