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
    float masterVolume{0.8f};  // Volume knob 0.0 to 1.0 (audio taper, ahead of the pedal)
    float drive{0.0f};         // MXR Distortion+ emulation; 0 = pedal bypassed
    float tuningCents{0.0f};   // Master tuning trim, ± cents (hardware range: approx. ±700 cents)

    // --- Experimental / calibration parameters ---------------------------
    // "fitted 2026-09-29 to the x0x set, sweep-tracked": fitted by tools/calibrate_reference.py
    // --manifest to the dinsync.info recordings in test/resources/x0x-reference
    // (one TB-303, 400 notes). Filter, knob-law, envelope and accent constants
    // without a note were fitted in the same run; docs/X0X_CALIBRATION_2026-09-28.md
    // has the procedure, the match quality and where the fit departs from
    // TB303_REFERENCE.md.
    // Exposed as CLAP parameters only in a ACIDUS_CALIBRATION_BUILD; a
    // Release build keeps these at their defaults (see AcidusClap.hpp).
    float oscCouplingHz{47.2397f};   // Oscillator.hpp - pre-filter HP; Open303 44.486 Hz (TB303_REFERENCE.md §11.3; hardware: none before the VCF)
    float resCouplingHz{104.291f};       // Filter.hpp - in-loop coupling HP; plausible range 70-150 Hz (antto 122, Open303 150)
    float filterFeedbackGain{19.2915f};   // Filter.hpp - feedback ceiling at Resonance 1; puts max resonance ~x1.06 below threshold near 1 kHz (TB303_REFERENCE.md §11.2, reference test B8)
    float filterPostHpHz{198.892f};   // Filter.hpp - out-of-loop HP; Open303 24.167 Hz, also stands in for the VCA-input coupling (§15.4); at the calibrator's 200 Hz bound -- it models the x0x unit's weak C2 fundamental (fitted 2026-09-29 to the x0x set, sweep-tracked)
    float filterNotchHz{7.5164f};   // Filter.hpp - out-of-loop notch; Open303 7.5164 Hz
    float filterNotchBandwidthHz{4.7f};   // Filter.hpp - notch bandwidth; Open303 4.7
    float filterAllpassHz{14.008f};   // Filter.hpp - out-of-loop allpass; Open303 14.008 Hz
    float filterInputCouplingHz{6.01647f};   // Filter.hpp - VCF input coupling HP; low values keep the fundamental's phase (waveform match) and the B5 sub-bass hump, at the cost of B6 (fitted 2026-09-29 to the x0x set, sweep-tracked)
    float filterOutputCouplingHz{20000.0f};   // Filter.hpp - plausible range 10-25 kHz
    float filterCapScale1{1.28949f};       // Filter.hpp - unsourced; 1.0 = schematic (33/33/33/18 nF), range 0.2-4.0 (ladder pole-frequency spread, stage 1)
    float filterCapScale2{0.697635f};       // Filter.hpp - unsourced; 1.0 = schematic (33/33/33/18 nF), range 0.2-4.0 (ladder pole-frequency spread, stage 2)
    float filterCapScale3{0.912745f};       // Filter.hpp - unsourced; 1.0 = schematic (33/33/33/18 nF), range 0.2-4.0 (ladder pole-frequency spread, stage 3)
    float filterCapScale4{1.06305f};       // Filter.hpp - unsourced; 1.0 = schematic (33/33/33/18 nF), range 0.2-4.0 (ladder pole-frequency spread, stage 4)
    float filterLadderInputScale{0.0352243f};   // Filter.hpp - plausible range 0.02-0.20 (ladder nonlinearity drive) (fitted 2026-09-29 to the x0x set, sweep-tracked)
    float filterLadderTopology{0.0f};   // Filter.hpp - 0 = legacy mirrored ladder, 1 = circuit orientation (input pair tanh(x - k*y4), half cap on stage 1, terminal tanh(y4); §10.3, audit S1)
    float filterCouplingNetwork{0.0f};   // Filter.hpp - 0 = Open303 empirical coupling (input HP, one in-loop HP, notch, all-pass; §11.3 option 2), 1 = Stinchcombe's full 10-pole network (§11.1, §11.3 option 1)
    float filterNetworkTimeScale{1.0f};  // Filter.hpp - Stinchcombe network RC time-constant scale (network 1 only); 1.0 = schematic, electrolytics +-20 %
    float vegDecaySec{2.68385f};           // Envelope.hpp - VEG tau; R123 x C42 = 1.5 s (§15.1), the x0x unit's held notes decay slower (fitted 2026-09-29 to the x0x set, sweep-tracked)
    float vcaGateOffMs{0.811348f};          // Envelope.hpp - VCA release tau at gate-off; Open303 1 ms (fitted 2026-09-29 to the x0x set, sweep-tracked)
    float vcaGateOffAccentMs{2.80367f};    // Envelope.hpp - VCA release tau at gate-off on accented notes; Open303 50 ms, neither measured unit shows a long accent tail (fitted 2026-09-29 to the x0x set, sweep-tracked)
    float vcaResTapRatio{1.24556f};       // SynthEngine.cpp - filter->VCA wiper tap relative to the fixed tap (§12 trace: 100k/220k = 0.45, reversed 2.2) (fitted 2026-09-29 to the x0x set, sweep-tracked)
    float vcaGainSaturationDrive{0.0f};   // SynthEngine.cpp - control-to-gain tanh ceiling; 0 = linear control law (§15.3). Old default 6.9 left accents no headroom
    float vcoOctaveScale{1.0f};   // Oscillator.hpp - VCO V/oct scale (TM5 width): 1 = exact 2:1 octaves; measured units 0.99-1.03 (TB303_REFERENCE.md §5.3)

    // --- Offline-calibration constants -----------------------------------
    // Previously hard-coded in the DSP; hoisted here so the reference-sample
    // calibrator (tools/calibrate_reference.py) can fit them. Not exposed as
    // CLAP parameters -- update these defaults with the calibrator's output.
    float cutoffBaseHz{184.076f};          // SynthEngine.cpp - cutoff at knob minimum, no env; this unit's TM3 trim (fitted 2026-09-29 to the x0x set, sweep-tracked)
    float cutoffSpanOct{3.09108f};       // SynthEngine.cpp - octaves swept by the Cutoff knob (fitted 2026-09-29 to the x0x set, sweep-tracked)
    float cutoffMaxHz{23723.3f};         // SynthEngine.cpp - ceiling of the cutoff CV; the x0x unit starts Env Mod 100 % notes at 15-16 kHz (ref: 25-28 kHz, conformance E7)
    float cutoffTaperExp{1.42889f};          // SynthEngine.cpp - knob taper, cv = span * knob^exp; 1 = exponential knob-to-Hz law (TB303_REFERENCE.md §13.2)
    // Env Mod law, octaves per unit MEG: envScaler = (1-c)*(C0 + C0Slope*e) + c*(C1 + C1Slope*e),
    // cutoff shift = envScaler * (MEG - (Offset + OffsetCutSlope*c)). Defaults are Open303's fit of
    // hardware measurements (TB303_REFERENCE.md §13.2); non-zero at Env Mod 0 (residual sweep).
    float envModScaleC0{0.761449f};         // SynthEngine.cpp - envScaler at Cutoff min, Env Mod 0
    float envModScaleC0Slope{3.9723f};    // SynthEngine.cpp - envScaler increase per unit Env Mod, Cutoff min
    float envModScaleC1{0.777939f};         // SynthEngine.cpp - envScaler at Cutoff max, Env Mod 0
    float envModScaleC1Slope{4.53338f};    // SynthEngine.cpp - envScaler increase per unit Env Mod, Cutoff max
    float envModOffset{0.334723f};         // SynthEngine.cpp - MEG level at which the Env Mod bias shift is neutral
    float envModTaperExp{2.0f};          // SynthEngine.cpp - Env Mod pot taper: the knob enters the law as envMod^exp; 1 = linear
    float envModTaperMid{0.686116f};          // SynthEngine.cpp - logistic Env Mod taper mid-point (used when envModTaperWidth > 0)
    float envModTaperWidth{0.121931f};        // SynthEngine.cpp - logistic Env Mod taper width; 0 = use envModTaperExp
    float envModOffsetCutSlope{-0.0238615f}; // SynthEngine.cpp - envOffset increase at Cutoff max
    float accentSweepDepthOct{8.60691f};     // SynthEngine.cpp - accent sweep into the cutoff, octaves per unit of VR4b wiper voltage (MEG units) (fitted 2026-09-29 to the x0x set, sweep-tracked)
    float accentVcaDepth{1.62417f};          // SynthEngine.cpp - accent term in the VCA control sum (x accented MEG, x Accent knob) (fitted 2026-09-29 to the x0x set, sweep-tracked)
    float accentChargeBaseSec{0.0305791f};   // Envelope.cpp - R46 47k x C13 1uF (§16.2) (fitted 2026-09-29 to the x0x set, sweep-tracked)
    float accentChargePotSec{0.0440208f};    // Envelope.cpp - VR4b 50k x C13 1uF (Resonance gang B) (fitted 2026-09-29 to the x0x set, sweep-tracked)
    float accentDiodeDrop{0.300332f};         // Envelope.cpp - D24 forward drop as a fraction of the MEG swing; 0 = ideal diode
    float accentMixSec{0.144977f};          // Envelope.cpp - mixing resistor (Whittle 100k, likely R72) x C13 1uF; C13 discharges through it + the lower pot section (fitted 2026-09-29 to the x0x set, sweep-tracked)
    float oscSawLpfHz{40000.0f};         // Oscillator.cpp - saw-core bandwidth limit; unsourced (TB303_REFERENCE.md §7.2), bypassed above 0.45*fs
    float oscSawShape{0.0f};            // Oscillator.cpp - saw x - s*x^2 bend; unsourced (§7.2: "drop them"), 0 = clean ramp
    float oscSquareDutyDepth{0.12f};    // Oscillator.cpp - square duty = 0.45 + depth * exp(-f / 180 Hz); antto fit 0.25. 0.12 = duty 0.533 at C2 reproduces the x0x set's square harmonics (15th nulled) within ~1-2 dB, measured directly
    float oscSquareLevel{0.598235f};        // Oscillator.cpp - square level relative to the saw's +-1 (§9: saw p-p ~2x the square's)
    float vcfAttackMs{0.1f};             // Envelope.cpp - MEG charge time constant (D37 + R152 100R into C62: ~0.1 ms, §14.1)
    float vcaNormalDelayMs{4.50396f};        // Envelope.cpp - VCA opens this much later on unaccented notes (x0x recordings: ~4.5 ms)
    float vcaAttackMs{1.30112f};             // Envelope.cpp - VEG onset time constant; ref 'a few ms' (§15.2), this unit opens faster (fitted 2026-09-29 to the x0x set, sweep-tracked)
    float vcfDecayMinSec{0.0654089f};        // Envelope.cpp - MEG decay tau at Decay min: R136 68k x C62 1uF (§14.1)
    float vcfDecayMaxSec{0.984139f};        // Envelope.cpp - MEG decay tau at Decay max: (68k + VR6 1M) x 1uF; A-taper law in between
    float vcfDecayTaper{17.9825f};          // Envelope.cpp - Decay pot (VR6) taper a: R = Rtot*(a^x-1)/(a-1); 81 = 10 % at mid-travel (§14.1)
    float accentDecaySec{0.0726633f};        // Envelope.cpp - MEG decay tau on accented notes (VR6 shorted -> R136 alone)
    float filterResonanceSkew{-0.871804f};     // Filter.hpp - Resonance pot curve (exponential skew; < 0 = resonance builds late in the travel) (fitted 2026-09-28 to the x0x set; was 3.0)
    float filterResonanceLimit{0.999842f};   // Filter.hpp - max feedback as a fraction of the loop's critical gain (<1 never self-oscillates)
};

class SynthEngine {
public:
    SynthEngine();
    ~SynthEngine() = default;

    // Note-on velocity at or above this is an accented note.
    static constexpr float kAccentVelocity = 0.8f;

    void setSampleRate(double sampleRate);
    void reset();

    void noteOn(int noteNumber, float velocity);
    void noteOff(int noteNumber);
    // Polyphonic pressure on the held note re-latches its accent (pressure
    // >= 0.5 is accented) with no retrigger: Burette's accent change on an
    // equal-pitch slide (TB303_REFERENCE.md §4.6). Other keys are ignored.
    void notePressure(int noteNumber, float pressure);

    void processAudio(float* outLeft, float* outRight, int numFrames);

    SynthParameters& getParams() { return params_; }
    const SynthParameters& getParams() const { return params_; }

    // Read-only probe for tests: the cutoff (Hz) handed to the filter on the
    // most recent processed sample, after the engine's clamp.
    float getLastCutoffHz() const { return lastCutoffHz_; }
    // Is the held (or last) note accented?
    bool isAccent() const { return env_.isAccent(); }

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
