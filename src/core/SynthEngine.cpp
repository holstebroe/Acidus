#include "SynthEngine.hpp"
#include <algorithm>
#include <cmath>

namespace acidus {

namespace {

// Hard limit on the plugin's output: only reached by a numerical blow-up, well
// above anything the model produces in normal use (the bypassed path tops out
// at the VCA's unity tanh ceiling times kOutputStageGain = 8).
constexpr float kOutputSafetyLimit = 16.0f;

// Output stage: VCA -> VR8 Volume (50 kOhm "A" taper) -> output amplifier
// (the Q33/Q34 mixer) -> OUTPUT jack -> pedal (TB303_REFERENCE.md §17). The
// pot is a passive divider, so it only attenuates; all of the stage's gain
// is in this one fixed amplifier after it. The VCA's tanh ceiling is unity
// and a typical note reaches about a quarter of it, so full Volume puts
// typical peaks near +6 dBFS: a hot line level that drives the Distortion+
// hard. Volume 0.8 (the default) sits about 8 dB down the taper, near 0 dBFS.
// Linear, so the clean timbre (and the calibration fits, which solve one
// global gain) are unchanged.
constexpr float kOutputStageGain = 8.0f;

// Audio ("A") pot law, the same one TB303_REFERENCE.md §14.1 gives for every
// A pot on the panel: 10 % of the travel's resistance at mid-rotation
// (-20 dB), 0 at fully CCW, 1 at fully CW.
constexpr float kAudioTaperBase = 81.0f;
inline float audioTaper(float knob) {
    return (std::pow(kAudioTaperBase, knob) - 1.0f) / (kAudioTaperBase - 1.0f);
}

inline float clampParam(float v, float lo, float hi, float fallback) {
    if (!std::isfinite(v)) return fallback;
    return std::min(std::max(v, lo), hi);
}

// A copy of `in` with every field forced finite and into a range the DSP
// handles without dividing by zero, going unstable or overflowing. The ranges
// are deliberately wider than the CLAP parameter ranges: this is a last line of
// defence against corrupt state, hostile host automation and bad callers, not
// the parameters' user-facing limits.
// Valid range per float field. A table rather than one clamp call per field:
// the DSP core builds at -O3, where 61 inlined clamps cost ~6 KB of code.
struct ParamRange {
    float SynthParameters::* field;
    float lo, hi;
};

const ParamRange kParamRanges[] = {
    {&SynthParameters::cutoff, 0.0f, 1.0f},
    {&SynthParameters::resonance, 0.0f, 1.0f},
    {&SynthParameters::envMod, 0.0f, 1.0f},
    {&SynthParameters::decay, 0.0f, 1.0f},
    {&SynthParameters::accent, 0.0f, 1.0f},
    {&SynthParameters::masterVolume, 0.0f, 1.0f},
    {&SynthParameters::drive, 0.0f, 1.0f},
    {&SynthParameters::tuningCents, -1200.0f, 1200.0f},

    {&SynthParameters::oscCouplingHz, 1.0f, 500.0f},
    {&SynthParameters::resCouplingHz, 1.0f, 2000.0f},
    {&SynthParameters::filterFeedbackGain, 0.0f, 40.0f},
    {&SynthParameters::filterPostHpHz, 1.0f, 1000.0f},
    {&SynthParameters::filterNotchHz, 0.5f, 100.0f},
    {&SynthParameters::filterNotchBandwidthHz, 0.1f, 100.0f},
    {&SynthParameters::filterAllpassHz, 0.5f, 200.0f},
    {&SynthParameters::filterInputCouplingHz, 0.5f, 500.0f},
    {&SynthParameters::filterOutputCouplingHz, 1000.0f, 40000.0f},
    {&SynthParameters::filterCapScale1, 0.2f, 4.0f},
    {&SynthParameters::filterCapScale2, 0.2f, 4.0f},
    {&SynthParameters::filterCapScale3, 0.2f, 4.0f},
    {&SynthParameters::filterCapScale4, 0.2f, 4.0f},
    {&SynthParameters::filterLadderInputScale, 0.005f, 1.0f},
    {&SynthParameters::filterLadderTopology, 0.0f, 1.0f},
    {&SynthParameters::vegDecaySec, 0.05f, 30.0f},
    {&SynthParameters::vcaGateOffMs, 0.05f, 1000.0f},
    {&SynthParameters::vcaGateOffAccentMs, 0.05f, 1000.0f},
    {&SynthParameters::vcaResTapRatio, 0.0f, 10.0f},
    {&SynthParameters::vcaGainSaturationDrive, 0.0f, 50.0f},
    {&SynthParameters::vcoOctaveScale, 0.5f, 2.0f},

    {&SynthParameters::cutoffBaseHz, 10.0f, 2000.0f},
    {&SynthParameters::cutoffSpanOct, 0.0f, 8.0f},
    {&SynthParameters::cutoffMaxHz, 100.0f, 40000.0f},
    {&SynthParameters::cutoffTaperExp, 0.1f, 5.0f},
    {&SynthParameters::envModScaleC0, 0.0f, 20.0f},
    {&SynthParameters::envModScaleC0Slope, 0.0f, 20.0f},
    {&SynthParameters::envModScaleC1, 0.0f, 20.0f},
    {&SynthParameters::envModScaleC1Slope, 0.0f, 20.0f},
    {&SynthParameters::envModOffset, -2.0f, 2.0f},
    {&SynthParameters::envModTaperExp, 0.1f, 8.0f},
    {&SynthParameters::envModTaperMid, 0.05f, 1.0f},
    {&SynthParameters::envModTaperWidth, 0.0f, 2.0f},
    {&SynthParameters::envModOffsetCutSlope, -2.0f, 2.0f},
    {&SynthParameters::accentSweepDepthOct, 0.0f, 20.0f},
    {&SynthParameters::accentVcaDepth, 0.0f, 20.0f},
    {&SynthParameters::accentChargeBaseSec, 0.0005f, 2.0f},
    {&SynthParameters::accentChargePotSec, 0.0005f, 2.0f},
    {&SynthParameters::accentDiodeDrop, 0.0f, 0.9f},
    {&SynthParameters::accentMixSec, 0.001f, 2.0f},
    {&SynthParameters::oscSawLpfHz, 100.0f, 1.0e6f},
    {&SynthParameters::oscSawShape, -1.0f, 1.0f},
    {&SynthParameters::oscSquareDutyDepth, 0.0f, 0.25f},
    {&SynthParameters::oscSquareLevel, 0.0f, 2.0f},
    {&SynthParameters::vcfAttackMs, 0.01f, 100.0f},
    {&SynthParameters::vcaNormalDelayMs, 0.0f, 100.0f},
    {&SynthParameters::vcaAttackMs, 0.05f, 500.0f},
    {&SynthParameters::vcfDecayMinSec, 0.005f, 10.0f},
    {&SynthParameters::vcfDecayMaxSec, 0.005f, 20.0f},
    {&SynthParameters::vcfDecayTaper, 1.0f, 500.0f},
    {&SynthParameters::accentDecaySec, 0.005f, 5.0f},
    {&SynthParameters::filterResonanceSkew, -20.0f, 20.0f},
    {&SynthParameters::filterResonanceLimit, 0.1f, 3.0f},
};

SynthParameters sanitizeParams(const SynthParameters& in) {
    const SynthParameters d;   // defaults, used for non-finite values
    SynthParameters p = in;
    for (const ParamRange& r : kParamRanges)
        p.*r.field = clampParam(p.*r.field, r.lo, r.hi, d.*r.field);
    if (p.waveform != Waveform::Saw && p.waveform != Waveform::Square) p.waveform = Waveform::Saw;
    p.vcfDecayMaxSec = std::max(p.vcfDecayMaxSec, p.vcfDecayMinSec);
    return p;
}

} // namespace

SynthEngine::SynthEngine() {
    setSampleRate(44100.0);
}

void SynthEngine::setSampleRate(double sampleRate) {
    // A host can hand over zero, a negative or a non-finite rate; every
    // coefficient below divides by it.
    if (!std::isfinite(sampleRate) || sampleRate <= 0.0) sampleRate = 44100.0;
    sampleRate = std::min(std::max(sampleRate, 8000.0), 768000.0);
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
    if (noteNumber < 0 || noteNumber > 127) return;   // not a MIDI note (e.g. the CLAP wildcard -1)
    if (!std::isfinite(velocity)) velocity = 1.0f;
    const SynthParameters p = sanitizeParams(params_);
    bool isSlide = isNoteActive_;
    bool isAccent = (velocity >= kAccentVelocity);
    accentLevel_ = isAccent ? 1.0f : 0.0f;

    currentNote_ = noteNumber;
    isNoteActive_ = true;

    osc_.setWaveform(p.waveform);
    osc_.noteOn(noteNumber, isSlide);
    env_.setFaithfulAccentDecay(true);
    env_.setDecay(p.decay);
    env_.noteOn(isAccent, isSlide, p.accent);
}

void SynthEngine::notePressure(int noteNumber, float pressure) {
    if (!isNoteActive_ || noteNumber != currentNote_ || !std::isfinite(pressure)) return;
    const bool isAccent = pressure >= 0.5f;
    if (isAccent == env_.isAccent()) return;
    accentLevel_ = isAccent ? 1.0f : 0.0f;
    env_.setAccent(isAccent);
}

void SynthEngine::noteOff(int noteNumber) {
    if (noteNumber == currentNote_ || noteNumber < 0) {
        isNoteActive_ = false;
        osc_.noteOff();
        env_.noteOff();
    }
}

void SynthEngine::processAudio(float* outLeft, float* outRight, int numFrames) {
    if (numFrames <= 0) return;
    // params_ is written by the GUI/host threads while this runs; work from a
    // sanitized snapshot so a torn or corrupt update can never reach the DSP.
    const SynthParameters p = sanitizeParams(params_);
    osc_.setWaveform(p.waveform);
    env_.setFaithfulAccentDecay(true);
    env_.setDecay(p.decay);

    osc_.setTuningCents(p.tuningCents);
    osc_.setOctaveScale(p.vcoOctaveScale);
    osc_.setCouplingHz(p.oscCouplingHz);
    filter_.setResCouplingHz(p.resCouplingHz);
    filter_.setFeedbackGainCeiling(p.filterFeedbackGain);
    filter_.setPostFilterHpHz(p.filterPostHpHz);
    filter_.setNotchFreqHz(p.filterNotchHz);
    filter_.setNotchBandwidthHz(p.filterNotchBandwidthHz);
    filter_.setAllpassFreqHz(p.filterAllpassHz);
    filter_.setInputCouplingHz(p.filterInputCouplingHz);
    filter_.setOutputCouplingHz(p.filterOutputCouplingHz);
    filter_.setCapScale1(p.filterCapScale1);
    filter_.setCapScale2(p.filterCapScale2);
    filter_.setCapScale3(p.filterCapScale3);
    filter_.setCapScale4(p.filterCapScale4);
    filter_.setLadderInputScale(p.filterLadderInputScale);
    filter_.setLadderTopology(p.filterLadderTopology >= 0.5f ? 1 : 0);
    env_.setVegDecaySec(p.vegDecaySec);
    env_.setVcaGateOffMs(p.vcaGateOffMs);
    env_.setVcaGateOffAccentMs(p.vcaGateOffAccentMs);
    env_.setAttackTimesMs(p.vcfAttackMs, p.vcaAttackMs);
    env_.setDecayTaper(p.vcfDecayTaper);
    env_.setDecayRangeSec(p.vcfDecayMinSec, p.vcfDecayMaxSec);
    env_.setAccentDecaySec(p.accentDecaySec);
    env_.setAccentSweepResonance(p.resonance);
    env_.setAccentSweepTimes(p.accentChargeBaseSec, p.accentChargePotSec, p.accentMixSec);
    env_.setAccentKnob(std::min(std::max(p.accent, 0.0f), 1.0f));
    env_.setAccentDiodeDrop(p.accentDiodeDrop);
    env_.setVcaNormalDelaySec(p.vcaNormalDelayMs * 0.001f);
    osc_.setSawShaping(p.oscSawLpfHz, p.oscSawShape);
    osc_.setSquareShaping(p.oscSquareDutyDepth, p.oscSquareLevel);
    filter_.setResonanceSkew(p.filterResonanceSkew);
    filter_.setResonanceLimit(p.filterResonanceLimit);
    const float volumeGain = audioTaper(p.masterVolume);

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

        float cNorm = std::min(std::max(p.cutoff, 0.0f), 1.0f);
        float resNorm = std::min(std::max(p.resonance, 0.0f), 1.0f);
        float envModNorm = std::min(std::max(p.envMod, 0.0f), 1.0f);
        float accentNorm = std::min(std::max(p.accent, 0.0f), 1.0f);

        float cTaper = std::pow(cNorm, p.cutoffTaperExp);

        float cv_base = p.cutoffSpanOct * cTaper;

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
        if (p.envModTaperWidth > 0.0f) {
            auto logistic = [&](float x) {
                return 1.0f / (1.0f + std::exp(-(x - p.envModTaperMid) / p.envModTaperWidth));
            };
            float l0 = logistic(0.0f), l1 = logistic(1.0f);
            envModTapered = (logistic(envModNorm) - l0) / std::max(l1 - l0, 1e-6f);
        } else {
            envModTapered = std::pow(envModNorm, p.envModTaperExp);
        }
        float envScaler = (1.0f - cNorm) * (p.envModScaleC0 + p.envModScaleC0Slope * envModTapered)
                        + cNorm * (p.envModScaleC1 + p.envModScaleC1Slope * envModTapered);
        float envOffset = p.envModOffset + p.envModOffsetCutSlope * cNorm;
        float cv_envmod = envScaler * (vcfEnvVal - envOffset);

        // Accent sweep: the VR4b wiper voltage of the R46 / VR4b / C13 network
        // (Envelope.hpp, TB303_REFERENCE.md §16.2), already scaled by the
        // Accent knob. Low Resonance: a sharp kick of ~0.42 x MEG_acc; high
        // Resonance: the delayed, rounded C13 bump ("wow"). C13's charge is
        // seen on every note, so notes after an accent start higher.
        float cv_accent = accentSweepVal * p.accentSweepDepthOct;

        float cv_total = cv_base + cv_envmod + cv_accent;

        float effectiveCutoff = p.cutoffBaseHz * std::pow(2.0f, cv_total);
        float totalCutoff = std::min(std::max(effectiveCutoff, 20.0f), p.cutoffMaxHz);
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
            vcaControl += accentVcaVal * accentNorm * p.accentVcaDepth;
        }

        // Control-current sum -> gain. vcaGainSaturationDrive > 0 bends the
        // control-to-gain law into a tanh ceiling normalised to 1 at full
        // VEG; at the old default (6.9) that ceiling sat on every normal
        // note, so the accent term had no headroom left (+0.6 dB, where the
        // hardware pair D2 c1r1 a0/a1 shows +7.8 dB). Default 0 = linear
        // control law (TB303_REFERENCE.md §15.3: accent is a control-current
        // sum); the saturation lives on the signal below.
        float driveNorm = vcaControl * p.vcaGainSaturationDrive;
        float vcaGain = (p.vcaGainSaturationDrive > 0.0f)
                            ? std::tanh(driveNorm) / std::tanh(std::max(p.vcaGainSaturationDrive, 1e-6f))
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
        float tapGain = (1.0f + p.vcaResTapRatio * resNorm) / (1.0f + p.vcaResTapRatio);
        float xVal = filterOut * tapGain * vcaGain;
        float vcaSignal = std::tanh(xVal);

        // Volume comes before the pedal, as with a real 303 plugged into
        // one: turning it up drives the Distortion+ harder.
        float lineOut = vcaSignal * volumeGain * kOutputStageGain;
        float finalSample = distortion_.processSample(lineOut, p.drive);

        // A numerical blow-up (NaN/Inf in a filter or pedal state) would
        // otherwise latch forever and poison the host's mix bus: silence this
        // sample and restart the stateful stages from rest.
        if (!std::isfinite(finalSample)) {
            filter_.reset();
            distortion_.reset();
            osc_.resetFilterStates();
            finalSample = 0.0f;
        }
        finalSample = std::min(std::max(finalSample, -kOutputSafetyLimit), kOutputSafetyLimit);

        if (outLeft) outLeft[i] = finalSample;
        if (outRight) outRight[i] = finalSample;
    }
}

} // namespace acidus
