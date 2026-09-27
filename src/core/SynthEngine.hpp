#ifndef ACIDUS_SYNTH_ENGINE_HPP
#define ACIDUS_SYNTH_ENGINE_HPP

#include "Oscillator.hpp"
#include "Envelope.hpp"
#include "Filter.hpp"
#include "Distortion.hpp"

namespace acidus {

struct SynthParameters {
    float cutoff{0.5f};        // Knob range 0.0 to 1.0
    float resonance{0.5f};     // Knob range 0.0 to 1.0
    float envMod{0.5f};        // Knob range 0.0 to 1.0
    float decay{0.5f};         // Knob range 0.0 to 1.0
    float accent{0.5f};        // Knob range 0.0 to 1.0
    Waveform waveform{Waveform::Saw};
    float masterVolume{0.8f};
    float drive{0.0f};         // MXR Distortion+ emulation; 0 = pedal bypassed
    float tuningCents{0.0f};   // Master tuning trim, ± cents (hardware range: approx. ±700 cents)

    // --- Experimental / calibration parameters ---------------------------
    // Exposed as CLAP parameters only in a ACIDUS_CALIBRATION_BUILD; a
    // Release build keeps these at their defaults (see AcidusClap.hpp).
    float oscCouplingHz{44.5f};   // Oscillator.hpp - pre-filter HP; Open303 44.486 Hz (TB303_REFERENCE.md §11.3; hardware: none before the VCF)
    float resCouplingHz{100.0f};       // Filter.hpp - in-loop coupling HP; plausible range 70-150 Hz (antto 122, Open303 150)
    float filterFeedbackGain{18.5f};   // Filter.hpp - feedback ceiling at Resonance 1; puts max resonance ~x1.06 below threshold near 1 kHz (TB303_REFERENCE.md §11.2, reference test B8)
    float filterPostHpHz{24.167f};   // Filter.hpp - out-of-loop HP; Open303 24.167 Hz, also stands in for the VCA-input coupling (§15.4)
    float filterNotchHz{7.5164f};   // Filter.hpp - out-of-loop notch; Open303 7.5164 Hz
    float filterNotchBandwidthHz{4.7f};   // Filter.hpp - notch bandwidth; Open303 4.7
    float filterAllpassHz{14.008f};   // Filter.hpp - out-of-loop allpass; Open303 14.008 Hz
    float filterInputCouplingHz{50.0f};   // Filter.hpp - VCF input coupling HP; ~50 Hz keeps the Res-0 low end on Stinchcombe (reference test B6)
    float filterOutputCouplingHz{20000.0f};   // Filter.hpp - plausible range 10-25 kHz
    float filterCapScale1{1.0f};       // Filter.hpp - unsourced; 1.0 = schematic (33/33/33/18 nF), range 0.2-4.0 (ladder pole-frequency spread, stage 1)
    float filterCapScale2{1.0f};       // Filter.hpp - unsourced; 1.0 = schematic (33/33/33/18 nF), range 0.2-4.0 (ladder pole-frequency spread, stage 2)
    float filterCapScale3{1.0f};       // Filter.hpp - unsourced; 1.0 = schematic (33/33/33/18 nF), range 0.2-4.0 (ladder pole-frequency spread, stage 3)
    float filterCapScale4{1.0f};       // Filter.hpp - unsourced; 1.0 = schematic (33/33/33/18 nF), range 0.2-4.0 (ladder pole-frequency spread, stage 4)
    float filterLadderInputScale{0.0551989f};   // Filter.hpp - plausible range 0.02-0.20 (ladder nonlinearity drive)
    float vegDecaySec{3.5f};           // Envelope.hpp - plausible range 2.5-5.0 s
    float vcaGateOffMs{3.44988f};          // Envelope.hpp - plausible range 1-5 ms (measured against hardware 2026-09-20, was 1ms)
    float vcaGateOffAccentMs{6.80945f};    // Envelope.hpp - plausible range 1-80 ms (measured against hardware 2026-09-20: no accent asymmetry found, was 50ms)
    float vcaGainSaturationDrive{0.0f};   // SynthEngine.cpp - control-to-gain tanh ceiling; 0 = linear control law (§15.3). Old default 6.9 left accents no headroom

    // --- Offline-calibration constants -----------------------------------
    // Previously hard-coded in the DSP; hoisted here so the reference-sample
    // calibrator (tools/calibrate_reference.py) can fit them. Not exposed as
    // CLAP parameters -- update these defaults with the calibrator's output.
    float cutoffBaseHz{341.5f};          // SynthEngine.cpp - cutoff at knob minimum, no env (rescaled x0.43 when capScales returned to 1.0)
    float cutoffSpanOct{2.93f};       // SynthEngine.cpp - octaves swept by the Cutoff knob (settled Res-max peak ~370 Hz -> ~2.6 kHz; TB303_REFERENCE.md §13.3)
    float cutoffTaperExp{2.0f};          // SynthEngine.cpp - knob taper: cv = span * knob^exp
    // Env Mod law, octaves per unit MEG: envScaler = (1-c)*(C0 + C0Slope*e) + c*(C1 + C1Slope*e),
    // cutoff shift = envScaler * (MEG - (Offset + OffsetCutSlope*c)). Defaults are Open303's fit of
    // hardware measurements (TB303_REFERENCE.md §13.2); non-zero at Env Mod 0 (residual sweep).
    float envModScaleC0{0.737f};         // SynthEngine.cpp - envScaler at Cutoff min, Env Mod 0
    float envModScaleC0Slope{3.774f};    // SynthEngine.cpp - envScaler increase per unit Env Mod, Cutoff min
    float envModScaleC1{0.864f};         // SynthEngine.cpp - envScaler at Cutoff max, Env Mod 0
    float envModScaleC1Slope{4.195f};    // SynthEngine.cpp - envScaler increase per unit Env Mod, Cutoff max
    float envModOffset{0.2944f};         // SynthEngine.cpp - MEG level at which the Env Mod bias shift is neutral
    float envModOffsetCutSlope{0.0483f}; // SynthEngine.cpp - envOffset increase at Cutoff max
    float accentSweepDepthOct{4.7f};     // SynthEngine.cpp - accent sweep into the cutoff (octaves x C13/MEG_acc x Accent knob); fitted to D2 c1r1 a0/a1
    float accentVcaDepth{2.05f};          // SynthEngine.cpp - accent term in the VCA control sum (x accented MEG, x Accent knob); fitted to D2 c1r1 a0/a1 (+7.8 dB at note start)
    float accentChargeBaseSec{0.047f};   // Envelope.cpp - C13 charge tau at Resonance min: R46 47k x C13 1uF (§16.2)
    float accentChargePotSec{0.050f};    // Envelope.cpp - added C13 charge tau at Resonance max: VR4b 50k x 1uF
    float accentDischargeSec{0.14f};     // Envelope.cpp - C13 discharge tau through wiper + mixing resistor (0.1-0.15 s; fitted to D2 c1r1 a0/a1)
    float oscSawLpfHz{40000.0f};         // Oscillator.cpp - saw-core bandwidth limit; unsourced (TB303_REFERENCE.md §7.2), bypassed above 0.45*fs
    float oscSawShape{0.0f};            // Oscillator.cpp - saw x - s*x^2 bend; unsourced (§7.2: "drop them"), 0 = clean ramp
    float vcfAttackMs{0.1f};             // Envelope.cpp - MEG charge time constant (D37 + R152 100R into C62: ~0.1 ms, §14.1)
    float vcaAttackMs{3.0f};             // Envelope.cpp - VEG attack time constant
    float vcfDecayMinSec{0.068f};        // Envelope.cpp - MEG decay tau at Decay min: R136 68k x C62 1uF (§14.1)
    float vcfDecayMaxSec{1.068f};        // Envelope.cpp - MEG decay tau at Decay max: (68k + VR6 1M) x 1uF; A-taper law in between
    float accentDecaySec{0.068f};        // Envelope.cpp - MEG decay tau on accented notes (VR6 shorted -> R136 alone)
    float filterResonanceSkew{3.0f};     // Filter.hpp - Resonance pot curve (exponential skew)
    float filterResonanceLimit{0.98f};   // Filter.hpp - max feedback as a fraction of the loop's critical gain (<1 never self-oscillates)
};

class SynthEngine {
public:
    SynthEngine();
    ~SynthEngine() = default;

    void setSampleRate(double sampleRate);
    void reset();

    void noteOn(int noteNumber, float velocity);
    void noteOff(int noteNumber);

    void processAudio(float* outLeft, float* outRight, int numFrames);

    SynthParameters& getParams() { return params_; }
    const SynthParameters& getParams() const { return params_; }

    // Read-only probe for tests: the cutoff (Hz) handed to the filter on the
    // most recent processed sample, after the engine's clamp.
    float getLastCutoffHz() const { return lastCutoffHz_; }

private:
    double sampleRate_{44100.0};
    SynthParameters params_;

    Oscillator osc_;
    Envelope env_;
    Filter filter_;
    Distortion distortion_;

    int currentNote_{-1};
    bool isNoteActive_{false};
    float accentLevel_{0.0f};
    float lastCutoffHz_{0.0f};

    // Smooth VCA Gate Envelope to prevent Note On / Off clicks
    float vcaGateEnv_{0.0f};
    float vcaAttackCoeff_{0.0f};
    float vcaReleaseCoeff_{0.0f};
};

} // namespace acidus

#endif // ACIDUS_SYNTH_ENGINE_HPP
