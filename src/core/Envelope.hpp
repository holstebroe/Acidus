#ifndef ACIDUS_ENVELOPE_HPP
#define ACIDUS_ENVELOPE_HPP

namespace acidus {

class Envelope {
public:
    Envelope();
    ~Envelope() = default;

    void setSampleRate(double sampleRate);
    void setDecay(float decayParam); // 0.0 to 1.0 -> MEG tau 68 ms .. 1.07 s (A-taper pot)

    void setFaithfulAccentDecay(bool faithful) { faithfulAccentDecay_ = faithful; }

    void setVegDecaySec(float seconds) { vegDecaySec_ = seconds; updateCoefficients(); }
    void setVcaGateOffMs(float ms) { vcaGateOffSec_ = ms * 0.001f; updateCoefficients(); }
    void setVcaGateOffAccentMs(float ms) { vcaGateOffAccentSec_ = ms * 0.001f; updateCoefficients(); }
    void setAttackTimesMs(float vcfMs, float vcaMs) { vcfAttackSec_ = vcfMs * 0.001f; vcaAttackSec_ = vcaMs * 0.001f; updateCoefficients(); }
    void setDecayRangeSec(float minSec, float maxSec) { vcfDecayMinSec_ = minSec; vcfDecayMaxSec_ = maxSec; setDecay(decayNorm_); }
    void setAccentDecaySec(float seconds) { accentDecaySec_ = seconds; updateCoefficients(); }

    // Accent-sweep network (TB303_REFERENCE.md §16.2), solved as the circuit:
    //   MEG_acc -> D24 -> R46 47k -> VR4b (50k, Resonance gang B) -> C13 1uF -> gnd
    //                                   wiper -> R_mix 100k -> cutoff summing node
    // Resonance moves the wiper from the R46 end (0) to the C13 end (1).
    // All resistances are given as time constants with C13 (R x 1 uF).
    // getAccentSweep() is the wiper voltage, in MEG units: at Resonance 0 it
    // jumps to ~0.42 x MEG_acc (the divider R46 / pot / R_mix) and follows
    // C13 from there; at Resonance 1 it is C13 alone, charging towards
    // ~0.51 x MEG_acc with tau ~49 ms. C13 discharges through the pot and
    // R_mix (0.10-0.15 s) on every note, accented or not.
    void setAccentSweepResonance(float res) { accentSweepRes_ = res; }
    void setAccentSweepTimes(float r46Sec, float potSec, float mixSec) {
        accentR46Sec_ = r46Sec; accentPotSec_ = potSec; accentMixSec_ = mixSec;
    }
    // Accent knob (VR7) level of MEG_acc feeding the sweep network.
    void setAccentKnob(float k) { accentKnob_ = k; }
    // D24 forward drop, as a fraction of the full MEG swing (0 = ideal diode).
    void setAccentDiodeDrop(float d) { accentDiodeDrop_ = d; }

    void noteOn(bool isAccent, bool isSlide, float accentKnob = 1.0f);
    void noteOff();

    void processNextSample();

    float getVcfEnv() const { return vcfEnv_; }
    float getVcaEnv() const { return vcaEnv_; }
    float getAccentSweep() const { return accentWiper_; }
    float getAccentVca() const { return accentVca_; }
    bool isAccent() const { return isAccent_; }
    bool isActive() const { return gate_ || (vcaEnv_ > 0.0001f) || (vcfEnv_ > 0.0001f); }

private:
    double sampleRate_{44100.0};

    bool gate_{false};
    bool isAccent_{false};
    bool faithfulAccentDecay_{false};

    float accentDecaySec_{0.068f};
    float vcfAttackSec_{0.0001f};
    float vcaAttackSec_{0.003f};
    float vcfDecayMinSec_{0.068f};
    float vcfDecayMaxSec_{1.068f};
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

    float accentSweepRes_{0.0f};
    float accentKnob_{1.0f};
    float accentR46Sec_{0.047f};
    float accentPotSec_{0.050f};
    float accentMixSec_{0.100f};
    float accentDiodeDrop_{0.0f};
    float accentVcaCoeff_{0.0f};

    float vcfEnv_{0.0f};
    float vcfTarget_{0.0f};

    float vcaEnv_{0.0f};
    float vcaTarget_{0.0f};

    float accentCap_{0.0f};     // C13 voltage (MEG units)
    float accentWiper_{0.0f};   // VR4b wiper voltage into the summing node
    float accentVca_{0.0f};

    void updateCoefficients();
};

} // namespace acidus

#endif // ACIDUS_ENVELOPE_HPP
