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
    double getCurrentFreqHz() const { return currentFreq_; } // read-only probe for tests

    void resetFilterStates();

    void setCouplingHz(float hz) { couplingHz_ = hz; recomputeCouplingAlpha(); }

    // Saw-core bandwidth limit and waveshaper curvature (x - shape*x^2).
    void setSawShaping(float lpfHz, float shape) {
        if (lpfHz != sawLpfHz_) { sawLpfHz_ = lpfHz; recomputeSawLpfCoeff(); }
        sawShape_ = shape;
    }

    // Square pulse: duty = 0.45 + dutyDepth * exp(-f / 180 Hz) (the pulse
    // narrows toward symmetric as the pitch rises), output level = level.
    void setSquareShaping(float dutyDepth, float level) {
        squareDutyDepth_ = dutyDepth;
        squareLevel_ = level;
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
            targetPitch_ = heldNote_ + tuningCents_ / 100.0;
            if (!isSliding_) currentPitch_ = targetPitch_;
        }
    }

private:
    double sampleRate_{44100.0};
    Waveform waveform_{Waveform::Saw};

    double phase_{0.0};
    // Pitch CV in semitones (MIDI note number + tuning). Slide is an RC on
    // this CV, before the exponential converter (TB303_REFERENCE.md §6).
    double currentPitch_{69.0};
    double targetPitch_{69.0};
    double currentFreq_{440.0};
    double freqPitch_{69.0};   // pitch currentFreq_ was computed for
    bool isSliding_{false};
    double slideCoeff_{0.0};

    int heldNote_{-1};
    float tuningCents_{0.0f};


    double lpfSawCoeff_{0.0};
    double lpfSawState_{0.0};

    double couplingHpfX1_{0.0};
    double couplingHpfY1_{0.0};
    double couplingAlpha_{0.0};

    float couplingHz_{44.5f};
    float sawLpfHz_{14000.0f};
    float sawShape_{0.05f};
    float squareDutyDepth_{0.25f};
    float squareLevel_{0.75f};

    void recomputeSawLpfCoeff() {
        // At or above ~0.45*fs the (unsourced) saw LPF is bypassed entirely.
        if (sawLpfHz_ >= 0.45 * sampleRate_) { lpfSawCoeff_ = 1.0; return; }
        lpfSawCoeff_ = 1.0 - std::exp(-2.0 * 3.14159265358979323846 * static_cast<double>(sawLpfHz_) / sampleRate_);
    }

    void recomputeCouplingAlpha() {
        couplingAlpha_ = std::exp(-2.0 * 3.14159265358979323846 * static_cast<double>(couplingHz_) / sampleRate_);
    }

    static double pitchToFreq(double pitch) {
        return 440.0 * std::pow(2.0, (pitch - 69.0) / 12.0);
    }
};

} // namespace acidus

#endif // ACIDUS_OSCILLATOR_HPP
