#ifndef ACIDUS_OSCILLATOR_HPP
#define ACIDUS_OSCILLATOR_HPP

#include <cmath>

namespace acidus {

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

    void setCouplingHz(float hz) { couplingHz_ = hz; recomputeCouplingAlpha(); }

private:
    double sampleRate_{44100.0};
    Waveform waveform_{Waveform::Saw};

    double phase_{0.0};
    double currentFreq_{440.0};
    double targetFreq_{440.0};
    bool isSliding_{false};
    double slideCoeff_{0.0};

    double lpfSawCoeff_{0.0};
    double lpfSawState_{0.0};

    double couplingHpfX1_{0.0};
    double couplingHpfY1_{0.0};
    double couplingAlpha_{0.0};

    float couplingHz_{44.5f};

    void recomputeCouplingAlpha() {
        couplingAlpha_ = std::exp(-2.0 * 3.14159265358979323846 * static_cast<double>(couplingHz_) / sampleRate_);
    }

    static double noteToFreq(int note) {
        return 440.0 * std::pow(2.0, (note - 69) / 12.0);
    }
};

} // namespace acidus

#endif // ACIDUS_OSCILLATOR_HPP
