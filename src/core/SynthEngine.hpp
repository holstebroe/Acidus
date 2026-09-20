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
    float oscCouplingHz{44.5f};        // Oscillator.hpp - plausible range 30-60 Hz
    float resCouplingHz{150.0f};       // Filter.hpp - plausible range 100-250 Hz
    float filterFeedbackGain{15.3f};   // Filter.hpp - plausible range 12-17
    float filterPostHpHz{24.167f};        // Filter.hpp - plausible range 15-35 Hz
    float filterNotchHz{7.5164f};         // Filter.hpp - plausible range 4-15 Hz
    float filterNotchBandwidthHz{4.7f};   // Filter.hpp - plausible range 2-10 Hz
    float filterAllpassHz{14.008f};       // Filter.hpp - plausible range 8-25 Hz
    float filterInputCouplingHz{20.0f};       // Filter.hpp - plausible range 10-30 Hz
    float filterOutputCouplingHz{20000.0f};   // Filter.hpp - plausible range 10-25 kHz
    float vegDecaySec{3.5f};           // Envelope.hpp - plausible range 2.5-5.0 s
    float vcaGateOffMs{1.0f};          // Envelope.hpp - plausible range 1-5 ms
    float vcaGateOffAccentMs{50.0f};   // Envelope.hpp - plausible range 30-80 ms
    float vcaGainSaturationDrive{3.0f};   // SynthEngine.cpp - plausible range 1-8 (BA662 transconductance-stage saturation)
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
