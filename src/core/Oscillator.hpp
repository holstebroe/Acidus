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

    // Saw-core bandwidth limit and waveshaper curvature (x - shape*x^2).
    void setSawShaping(float lpfHz, float shape) {
        if (lpfHz != sawLpfHz_) { sawLpfHz_ = lpfHz; recomputeSawLpfCoeff(); }
        sawShape_ = shape;
    }

    // Front-panel Tuning control: shifts VCO pitch by up to the real
    // hardware's documented trim range (TB303_RESEARCH_COMPENDIUM.md:
    // "Tuning control range: approx. ±700 cents"). Re-targets the currently
    // held note immediately so it tracks live, the same way a hardware
    // trimmer would while a note rings out.
    void setTuningCents(float cents) {
        if (cents == tuningCents_) return;
        tuningCents_ = cents;
        if (heldNote_ >= 0) {
            targetFreq_ = noteToFreq(heldNote_) * tuningRatio();
            if (!isSliding_) currentFreq_ = targetFreq_;
        }
    }

private:
    double sampleRate_{44100.0};
    Waveform waveform_{Waveform::Saw};

    double phase_{0.0};
    double currentFreq_{440.0};
    double targetFreq_{440.0};
    bool isSliding_{false};
    double slideCoeff_{0.0};

    int heldNote_{-1};
    float tuningCents_{0.0f};

    double tuningRatio() const { return std::pow(2.0, static_cast<double>(tuningCents_) / 1200.0); }

    double lpfSawCoeff_{0.0};
    double lpfSawState_{0.0};

    double couplingHpfX1_{0.0};
    double couplingHpfY1_{0.0};
    double couplingAlpha_{0.0};

    float couplingHz_{44.5f};
    float sawLpfHz_{14000.0f};
    float sawShape_{0.05f};

    void recomputeSawLpfCoeff() {
        lpfSawCoeff_ = 1.0 - std::exp(-2.0 * 3.14159265358979323846 * static_cast<double>(sawLpfHz_) / sampleRate_);
    }

    void recomputeCouplingAlpha() {
        couplingAlpha_ = std::exp(-2.0 * 3.14159265358979323846 * static_cast<double>(couplingHz_) / sampleRate_);
    }

    static double noteToFreq(int note) {
        return 440.0 * std::pow(2.0, (note - 69) / 12.0);
    }
};

} // namespace acidus

#endif // ACIDUS_OSCILLATOR_HPP
