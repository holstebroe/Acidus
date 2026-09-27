#ifndef ACIDUS_ENVELOPE_HPP
#define ACIDUS_ENVELOPE_HPP

namespace acidus {

class Envelope {
public:
    Envelope();
    ~Envelope() = default;

    void setSampleRate(double sampleRate);
    void setDecay(float decayParam); // 0.0 to 1.0 -> 200ms to 2.5s

    void setFaithfulAccentDecay(bool faithful) { faithfulAccentDecay_ = faithful; }

    void setVegDecaySec(float seconds) { vegDecaySec_ = seconds; updateCoefficients(); }
    void setVcaGateOffMs(float ms) { vcaGateOffSec_ = ms * 0.001f; updateCoefficients(); }
    void setVcaGateOffAccentMs(float ms) { vcaGateOffAccentSec_ = ms * 0.001f; updateCoefficients(); }
    void setAttackTimesMs(float vcfMs, float vcaMs) { vcfAttackSec_ = vcfMs * 0.001f; vcaAttackSec_ = vcaMs * 0.001f; updateCoefficients(); }
    void setDecayRangeSec(float minSec, float maxSec) { vcfDecayMinSec_ = minSec; vcfDecayMaxSec_ = maxSec; setDecay(decayNorm_); }
    void setAccentDecaySec(float seconds) { accentDecaySec_ = seconds; updateCoefficients(); }

    void noteOn(bool isAccent, bool isSlide, float accentKnob = 1.0f);
    void noteOff();

    void processNextSample();

    float getVcfEnv() const { return vcfEnv_; }
    float getVcaEnv() const { return vcaEnv_; }
    float getAccentCap() const { return accentCap_; }
    float getAccentVca() const { return accentVca_; }
    bool isAccent() const { return isAccent_; }
    bool isActive() const { return gate_ || (vcaEnv_ > 0.0001f) || (vcfEnv_ > 0.0001f); }

private:
    double sampleRate_{44100.0};

    bool gate_{false};
    bool isAccent_{false};
    bool faithfulAccentDecay_{false};

    float accentDecaySec_{0.20f};
    float vcfAttackSec_{0.0035f};
    float vcaAttackSec_{0.003f};
    float vcfDecayMinSec_{0.20f};
    float vcfDecayMaxSec_{2.5f};
    float decayNorm_{0.0f};

    float vcfDecayTimeSec_{0.20f};
    float vcfAttackCoeff_{0.0f};
    float vcfDecayCoeff_{0.0f};

    float vcaAttackCoeff_{0.0f};
    float vcaGateHighDecayCoeff_{0.0f};
    float vcaQuickDrainCoeff_{0.0f};
    float vcaQuickDrainAccentCoeff_{0.0f};

    float vegDecaySec_{3.5f};
    float vcaGateOffSec_{0.001f};
    float vcaGateOffAccentSec_{0.05f};

    float accentChargeCoeff_{0.0f};
    float accentDischargeCoeff_{0.0f};
    float accentVcaCoeff_{0.0f};

    float vcfEnv_{0.0f};
    float vcfTarget_{0.0f};

    float vcaEnv_{0.0f};
    float vcaTarget_{0.0f};

    float accentCap_{0.0f};
    float accentVca_{0.0f};

    void updateCoefficients();
};

} // namespace acidus

#endif // ACIDUS_ENVELOPE_HPP
