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
    float oscCouplingHz{21.0619f};        // Oscillator.hpp - plausible range 30-60 Hz
    float resCouplingHz{53.1459f};       // Filter.hpp - plausible range 100-250 Hz
    float filterFeedbackGain{21.9857f};   // Filter.hpp - plausible range 12-17
    float filterPostHpHz{59.6591f};        // Filter.hpp - plausible range 15-35 Hz
    float filterNotchHz{11.6954f};         // Filter.hpp - plausible range 4-15 Hz
    float filterNotchBandwidthHz{1.01559f};   // Filter.hpp - plausible range 2-10 Hz
    float filterAllpassHz{8.2056f};       // Filter.hpp - plausible range 8-25 Hz
    float filterInputCouplingHz{57.7366f};       // Filter.hpp - plausible range 10-30 Hz
    float filterOutputCouplingHz{19227.7f};   // Filter.hpp - plausible range 10-25 kHz
    float filterCapScale1{2.51136f};       // Filter.hpp - plausible range 0.2-4.0 (ladder pole-frequency spread, stage 1)
    float filterCapScale2{0.303818f};       // Filter.hpp - plausible range 0.2-4.0 (ladder pole-frequency spread, stage 2)
    float filterCapScale3{0.816061f};       // Filter.hpp - plausible range 0.2-4.0 (ladder pole-frequency spread, stage 3)
    float filterCapScale4{0.855279f};       // Filter.hpp - plausible range 0.2-4.0 (ladder pole-frequency spread, stage 4)
    float filterLadderInputScale{0.010036f};   // Filter.hpp - plausible range 0.02-0.20 (ladder nonlinearity drive)
    float vegDecaySec{3.5f};           // Envelope.hpp - plausible range 2.5-5.0 s
    float vcaGateOffMs{2.23397f};          // Envelope.hpp - plausible range 1-5 ms (measured against hardware 2026-09-20, was 1ms)
    float vcaGateOffAccentMs{3.0f};    // Envelope.hpp - plausible range 1-80 ms (measured against hardware 2026-09-20: no accent asymmetry found, was 50ms)
    float vcaGainSaturationDrive{0.17573f};   // SynthEngine.cpp - plausible range 1-8 (BA662 transconductance-stage saturation)

    // --- Offline-calibration constants -----------------------------------
    // Previously hard-coded in the DSP; hoisted here so the reference-sample
    // calibrator (tools/calibrate_reference.py) can fit them. Not exposed as
    // CLAP parameters -- update these defaults with the calibrator's output.
    float cutoffBaseHz{364.251f};          // SynthEngine.cpp - cutoff at knob minimum, no env
    float cutoffSpanOct{3.39414f};       // SynthEngine.cpp - octaves swept by the Cutoff knob
    float cutoffTaperExp{2.0f};          // SynthEngine.cpp - knob taper: cv = span * knob^exp
    float envModOffsetOct{0.80735f};     // SynthEngine.cpp - static cutoff offset from the Env Mod pot
    float envModDepthOct{3.5f};          // SynthEngine.cpp - MEG sweep depth at full Env Mod
    float accentSweepDepthOct{2.8365f};     // SynthEngine.cpp - Accent Sweep circuit depth at full Accent
    float accentVcaDepth{1.73187f};          // SynthEngine.cpp - MEG->VCA bleed on accented notes
    float oscSawLpfHz{19093.6f};         // Oscillator.cpp - saw-core bandwidth limit
    float oscSawShape{0.372375f};            // Oscillator.cpp - saw waveshaper 2nd-order curvature
    float vcfAttackMs{7.01795f};             // Envelope.cpp - MEG attack time constant
    float vcaAttackMs{2.34743f};             // Envelope.cpp - VEG attack time constant
    float vcfDecayMinSec{0.2f};          // Envelope.cpp - MEG decay at Decay knob minimum
    float vcfDecayMaxSec{2.5f};          // Envelope.cpp - MEG decay at Decay knob maximum
    float accentDecaySec{0.279491f};          // Envelope.cpp - MEG decay forced on accented notes
    float filterResonanceSkew{1.78832f};     // Filter.hpp - Resonance pot curve (exponential skew)
    float filterFeedbackHeadroomHz{7053.17f}; // Filter.hpp - low-cutoff feedback-gain compensation numerator
    float filterResCouplingTrackHz{11.7827f};  // Filter.cpp - resonance-dependent shift of the feedback coupling pole
    float filterMaxResonanceOutputGain{3.20153f}; // Filter.hpp - makeup gain at full Resonance
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

    // Smooth VCA Gate Envelope to prevent Note On / Off clicks
    float vcaGateEnv_{0.0f};
    float vcaAttackCoeff_{0.0f};
    float vcaReleaseCoeff_{0.0f};
};

} // namespace acidus

#endif // ACIDUS_SYNTH_ENGINE_HPP
