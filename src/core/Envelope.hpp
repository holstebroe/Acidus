#ifndef SYREBAS_ENVELOPE_HPP
#define SYREBAS_ENVELOPE_HPP

namespace syrebas {

class Envelope {
public:
    Envelope();
    ~Envelope() = default;

    void setSampleRate(double sampleRate);
    void setDecay(float decayParam); // 0.0 to 1.0 (corresponds to ~200ms to 2.5s)

    void noteOn(bool isAccent, bool isSlide);
    void noteOff();

    float processNextSample();

    bool isActive() const { return gate_ || (mainEnv_ > 0.0001f) || (accentEnv_ > 0.0001f); }
    float getMainEnv() const { return mainEnv_; }
    float getAccentEnv() const { return accentEnv_; }
    bool isAccent() const { return isAccent_; }

private:
    double sampleRate_{44100.0};

    bool gate_{false};
    bool isAccent_{false};

    float decayTimeSec_{0.2f};
    float decayCoeff_{0.0f};
    float accentDecayCoeff_{0.0f};

    float mainEnv_{0.0f};
    float accentEnv_{0.0f};

    void updateCoefficients();
};

} // namespace syrebas

#endif // SYREBAS_ENVELOPE_HPP
