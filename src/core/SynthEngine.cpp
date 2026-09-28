#include "SynthEngine.hpp"
#include <algorithm>
#include <cmath>

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
    filter_.setLadderTopology(params_.filterLadderTopology >= 0.5f ? 1 : 0);
    env_.setVegDecaySec(params_.vegDecaySec);
    env_.setVcaGateOffMs(params_.vcaGateOffMs);
    env_.setVcaGateOffAccentMs(params_.vcaGateOffAccentMs);
    env_.setAttackTimesMs(params_.vcfAttackMs, params_.vcaAttackMs);
    env_.setDecayTaper(params_.vcfDecayTaper);
    env_.setDecayRangeSec(params_.vcfDecayMinSec, params_.vcfDecayMaxSec);
    env_.setAccentDecaySec(params_.accentDecaySec);
    env_.setAccentSweepResonance(params_.resonance);
    env_.setAccentSweepTimes(params_.accentChargeBaseSec, params_.accentChargePotSec, params_.accentMixSec);
    env_.setAccentKnob(std::min(std::max(params_.accent, 0.0f), 1.0f));
    env_.setAccentDiodeDrop(params_.accentDiodeDrop);
    env_.setVcaNormalDelaySec(params_.vcaNormalDelayMs * 0.001f);
    osc_.setSawShaping(params_.oscSawLpfHz, params_.oscSawShape);
    osc_.setSquareShaping(params_.oscSquareDutyDepth, params_.oscSquareLevel);
    filter_.setResonanceSkew(params_.filterResonanceSkew);
    filter_.setResonanceLimit(params_.filterResonanceLimit);

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
        float accentSweepVal = env_.getAccentSweep();
        float accentVcaVal = env_.getAccentVca();
        bool noteAccent = env_.isAccent();

        float cNorm = std::min(std::max(params_.cutoff, 0.0f), 1.0f);
        float resNorm = std::min(std::max(params_.resonance, 0.0f), 1.0f);
        float envModNorm = std::min(std::max(params_.envMod, 0.0f), 1.0f);
        float accentNorm = std::min(std::max(params_.accent, 0.0f), 1.0f);

        float cTaper = std::pow(cNorm, params_.cutoffTaperExp);

        float cv_base = params_.cutoffSpanOct * cTaper;

        // Env Mod law (TB303_REFERENCE.md §13.1-13.2, Open303's fit of
        // hardware measurements): the MEG enters the antilog converter in
        // the octave domain, scaled by an Env-Mod- and Cutoff-dependent
        // factor and offset so that raising Env Mod *lowers* the settled
        // cutoff (Q9 bias shift) while the MEG peak lifts it. envScaler is
        // non-zero at Env Mod = 0: the real 303 keeps a residual MEG sweep
        // there (visible in the hardware samples as the resonant peak
        // gliding down during a Env Mod = 0 note). Accent never overrides
        // this; its sweep is the separate cv_accent term below.
        // Env Mod pot taper: the x0x recordings sweep barely more at Env Mod
        // 25 % than at 0 % and most of the depth arrives in the top half of
        // the travel (1.0 / 1.5 / 3.6 / 5.1 oct at 25..100 %, SET-E3), the
        // shape of an audio-taper pot. 1 = linear.
        // envModTaperWidth > 0 selects a normalised logistic (S-shaped) law
        // instead: little change up to ~50 %, the steepest rise around the
        // mid-point, flattening toward 100 % -- the x0x unit measures
        // 0.86 / 1.48 / 3.58 / 5.05 oct at 25..100 % in both E3 and D3.
        float envModTapered;
        if (params_.envModTaperWidth > 0.0f) {
            auto logistic = [&](float x) {
                return 1.0f / (1.0f + std::exp(-(x - params_.envModTaperMid) / params_.envModTaperWidth));
            };
            float l0 = logistic(0.0f), l1 = logistic(1.0f);
            envModTapered = (logistic(envModNorm) - l0) / std::max(l1 - l0, 1e-6f);
        } else {
            envModTapered = std::pow(envModNorm, params_.envModTaperExp);
        }
        float envScaler = (1.0f - cNorm) * (params_.envModScaleC0 + params_.envModScaleC0Slope * envModTapered)
                        + cNorm * (params_.envModScaleC1 + params_.envModScaleC1Slope * envModTapered);
        float envOffset = params_.envModOffset + params_.envModOffsetCutSlope * cNorm;
        float cv_envmod = envScaler * (vcfEnvVal - envOffset);

        // Accent sweep: the VR4b wiper voltage of the R46 / VR4b / C13 network
        // (Envelope.hpp, TB303_REFERENCE.md §16.2), already scaled by the
        // Accent knob. Low Resonance: a sharp kick of ~0.42 x MEG_acc; high
        // Resonance: the delayed, rounded C13 bump ("wow"). C13's charge is
        // seen on every note, so notes after an accent start higher.
        float cv_accent = accentSweepVal * params_.accentSweepDepthOct;

        float cv_total = cv_base + cv_envmod + cv_accent;

        float effectiveCutoff = params_.cutoffBaseHz * std::pow(2.0f, cv_total);
        float totalCutoff = std::min(std::max(effectiveCutoff, 20.0f), params_.cutoffMaxHz);
        lastCutoffHz_ = totalCutoff;

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
            vcaControl += accentVcaVal * accentNorm * params_.accentVcaDepth;
        }

        // Control-current sum -> gain. vcaGainSaturationDrive > 0 bends the
        // control-to-gain law into a tanh ceiling normalised to 1 at full
        // VEG; at the old default (6.9) that ceiling sat on every normal
        // note, so the accent term had no headroom left (+0.6 dB, where the
        // hardware pair D2 c1r1 a0/a1 shows +7.8 dB). Default 0 = linear
        // control law (TB303_REFERENCE.md §15.3: accent is a control-current
        // sum); the saturation lives on the signal below.
        float driveNorm = vcaControl * params_.vcaGainSaturationDrive;
        float vcaGain = (params_.vcaGainSaturationDrive > 0.0f)
                            ? std::tanh(driveNorm) / std::tanh(std::max(params_.vcaGainSaturationDrive, 1e-6f))
                            : vcaControl;

        // Smooth OTA-style soft ceiling on the signal. Must stay smooth
        // through zero: the previous (x > 0 ? tanh(1.1x) : tanh(0.9x)) had a
        // slope kink at every zero crossing, i.e. x + 0.1|x| at normal
        // levels, which sprayed a -12 dB/oct comb of harmonics up to
        // Nyquist -- audible as "harmonic noise" above a closed filter,
        // where the hardware rolls off cleanly (test/resources c0r1).
        // Filter -> VCA through two AC-coupled taps (TB303_REFERENCE.md §12,
        // audit S7): one from the top of the Resonance pot VR4a, one from its
        // wiper. Resonance (a linear pot) therefore sends more signal to the
        // VCA as it rises, partly offsetting the ladder's ~20 dB pass-band
        // loss. This is the circuit's own level path, not a compensation
        // stage: the hardware samples show only 1.4 dB between A2 c0r0 and
        // c0r1, where the ladder alone gives ~7 dB.
        float tapGain = (1.0f + params_.vcaResTapRatio * resNorm) / (1.0f + params_.vcaResTapRatio);
        float xVal = filterOut * tapGain * vcaGain;
        float vcaSignal = std::tanh(xVal);

        float drivenSignal = distortion_.processSample(vcaSignal, params_.drive);
        float finalSample = drivenSignal * params_.masterVolume;

        if (outLeft) outLeft[i] = finalSample;
        if (outRight) outRight[i] = finalSample;
    }
}

} // namespace acidus
