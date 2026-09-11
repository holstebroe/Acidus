#ifndef SYREBAS_ENVELOPE_HPP
#define SYREBAS_ENVELOPE_HPP

namespace syrebas {

class Envelope {
public:
    Envelope();
    ~Envelope() = default;

    void setSampleRate(double sampleRate);
    void setDecay(float decayParam); // 0.0 to 1.0 -> 200ms to 2.5s

    void noteOn(bool isAccent, bool isSlide);
    void noteOff();

    void processNextSample();

    float getVcfEnv() const { return vcfEnv_; }
    float getVcaEnv() const { return vcaEnv_; }
    bool isAccent() const { return isAccent_; }
    bool isActive() const { return vcaGate_ || (vcaEnv_ > 0.0001f) || (vcfEnv_ > 0.0001f); }

private:
    double sampleRate_{44100.0};

    bool vcaGate_{false};
    bool isAccent_{false};

    float vcfDecayTimeSec_{0.20f};
    float vcfAttackCoeff_{0.0f};
    float vcfDecayCoeff_{0.0f};

    float vcaAttackCoeff_{0.0f};
    float vcaDecayCoeff_{0.0f};

    float vcfEnv_{0.0f};
    float vcfState_{0.0f};

    float vcaEnv_{0.0f};
    float vcaState_{0.0f};

    void updateCoefficients();
};

} // namespace syrebas

#endif // SYREBAS_ENVELOPE_HPP
