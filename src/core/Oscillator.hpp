#ifndef SYREBAS_OSCILLATOR_HPP
#define SYREBAS_OSCILLATOR_HPP

#include <cmath>

namespace syrebas {

enum class Waveform {
    Saw = 0,
    Square = 1
};

class Oscillator {
public:
    Oscillator();
    ~Oscillator() = default;

    void setSampleRate(double sampleRate);
    void setWaveform(Waveform wave) { waveform_ = wave; }
    Waveform getWaveform() const { return waveform_; }

    void noteOn(int noteNumber, bool slide);
    void noteOff();

    float processNextSample();
    bool isSliding() const { return isSliding_; }

    void resetFilterStates();

    // Reference corner (Hz) for the shared saw/square coupling-network HPF
    // below. ESTIMATE, exposed as a CLAP parameter for experimentation
    // ("Experimental/Oscillator" > "Osc Coupling Freq"). Plausible range:
    // 70-120 Hz. See processNextSample()'s implementation comment for the
    // full sourcing/confidence rationale.
    void setCouplingHz(float hz) { couplingBaseHz_ = hz; }

private:
    double sampleRate_{44100.0};
    Waveform waveform_{Waveform::Saw};

    double phase_{0.0};
    double currentFreq_{440.0};
    double targetFreq_{440.0};
    bool isSliding_{false};
    double slideCoeff_{0.0};

    // Saw rounding/saturation. BEST GUESS / UNSOURCED (see TB303_RESEARCH_COMPENDIUM.md
    // Section 12: neither the 14 kHz corner nor the quadratic-bend coefficient below
    // traces to any source consulted). Kept as a cheap, plausible-sounding saturation
    // stand-in; not exposed as a tunable this round since it isn't contradicted by
    // research, just unconfirmed.
    double lpfSawCoeff_{0.0};
    double lpfSawState_{0.0};

    // Shared saw/square coupling-network HPF state (single set of state, since
    // only one waveform is generated at a time per Oscillator instance). See
    // the implementation comment in Oscillator.cpp for the full sourcing.
    double couplingHpfX1_{0.0};
    double couplingHpfY1_{0.0};

    // ESTIMATE - plausible range 70-120 Hz, default 98 Hz. Reference corner for
    // the pitch-tracking coupling-network HPF; see Oscillator.cpp.
    float couplingBaseHz_{98.0f};

    static double noteToFreq(int note) {
        return 440.0 * std::pow(2.0, (note - 69) / 12.0);
    }
};

} // namespace syrebas

#endif // SYREBAS_OSCILLATOR_HPP
