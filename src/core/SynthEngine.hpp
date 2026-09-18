#ifndef SYREBAS_SYNTH_ENGINE_HPP
#define SYREBAS_SYNTH_ENGINE_HPP

#include "Oscillator.hpp"
#include "Envelope.hpp"
#include "Filter.hpp"

namespace syrebas {

enum class EmulationMode {
    Accurate = 0,  // 4x oversampled coupled diode ladder (RK2)
    Faithful = 1   // 8x oversampled coupled diode ladder (RK4) with pole spreading
                   // and coupling poles, tracking the hardware more closely at
                   // higher CPU cost
};

struct SynthParameters {
    float cutoff{0.5f};        // Knob range 0.0 to 1.0
    float resonance{0.5f};     // Knob range 0.0 to 1.0
    float envMod{0.5f};        // Knob range 0.0 to 1.0
    float decay{0.5f};         // Knob range 0.0 to 1.0
    float accent{0.5f};        // Knob range 0.0 to 1.0
    Waveform waveform{Waveform::Saw};
    float masterVolume{0.8f};
    EmulationMode mode{EmulationMode::Accurate};

    // --- Experimental / calibration parameters ---------------------------
    // Not on the plugin's own GUI (see TB303_PARAMETER_CONFIDENCE.md) -
    // exposed only as CLAP parameters (host generic parameter list) for
    // by-ear retuning against a real TB-303 or reference recording. Each
    // default is this project's current best estimate within the plausible
    // range documented alongside its CLAP paramsInfo() entry in
    // SyrebasClap.cpp and in the class that actually uses it.
    float oscCouplingHz{98.0f};        // Oscillator.hpp - plausible range 70-120 Hz
    float resCouplingHz{9.0f};         // Filter.hpp (Faithful mode) - plausible range 5-15 Hz
    float filterFeedbackGain{36.0f};   // Filter.hpp (Faithful mode) - plausible range 20-40
    float resCutoffBleed{0.15f};       // SynthEngine.cpp - plausible range 0.0-0.30 (0-30%)
    float vegDecaySec{3.5f};           // Envelope.hpp - plausible range 2.5-5.0 s
    float vcaGateOffMs{16.0f};         // Envelope.hpp - plausible range 10-25 ms
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

    int currentNote_{-1};
    bool isNoteActive_{false};
    float accentLevel_{0.0f};

    // Smooth VCA Gate Envelope to prevent Note On / Off clicks
    float vcaGateEnv_{0.0f};
    float vcaAttackCoeff_{0.0f};
    float vcaReleaseCoeff_{0.0f};
};

} // namespace syrebas

#endif // SYREBAS_SYNTH_ENGINE_HPP
