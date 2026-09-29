#include "Envelope.hpp"
#include <cmath>
#include <algorithm>

namespace acidus {

Envelope::Envelope() {
    setSampleRate(44100.0);
}

void Envelope::setSampleRate(double sampleRate) {
    sampleRate_ = sampleRate;
    updateCoefficients();
}

void Envelope::setDecay(float decayParam) {
    float norm = std::min(std::max(decayParam, 0.0f), 1.0f);
    decayNorm_ = norm;
    // tau = C62 * (R136 + VR6(theta)), VR6 a 1M audio-taper pot:
    // R(theta) = Rtot * (a^theta - 1) / (a - 1); a = 81 is 10 % at
    // mid-rotation (TB303_REFERENCE.md §14.1). a is calibratable: a real
    // pot's taper, and a worn one, can differ.
    const float a = decayTaper_;
    float potFrac = (std::abs(a - 1.0f) < 1e-3f) ? norm : (std::pow(a, norm) - 1.0f) / (a - 1.0f);
    vcfDecayTimeSec_ = vcfDecayMinSec_ + (vcfDecayMaxSec_ - vcfDecayMinSec_) * potFrac;
    updateCoefficients();
}

void Envelope::updateCoefficients() {
    float dt = 1.0f / static_cast<float>(sampleRate_);

    // VCF envelope attack/decay time constants
    vcfAttackCoeff_ = 1.0f - std::exp(-dt / vcfAttackSec_);
    if (faithfulAccentDecay_ && isAccent_) {
        vcfDecayCoeff_ = std::exp(-1.0f / static_cast<float>(sampleRate_ * accentDecaySec_));
    } else {
        vcfDecayCoeff_ = std::exp(-1.0f / static_cast<float>(sampleRate_ * vcfDecayTimeSec_));
    }

    // VCA envelope attack/decay time constants
    vcaAttackCoeff_           = 1.0f - std::exp(-dt / vcaAttackSec_);
    vcaGateHighDecayCoeff_    = std::exp(-1.0f / static_cast<float>(sampleRate_ * vegDecaySec_));
    vcaQuickDrainCoeff_       = std::exp(-1.0f / static_cast<float>(sampleRate_ * vcaGateOffSec_));
    vcaQuickDrainAccentCoeff_ = std::exp(-1.0f / static_cast<float>(sampleRate_ * vcaGateOffAccentSec_));

    // Accent VCA RC smoothing (47 kOhm + 0.033 uF -> tau ~ 1.55ms)
    accentVcaCoeff_ = 1.0f - std::exp(-1.0f / static_cast<float>(sampleRate_ * 0.00155));
}

void Envelope::noteOn(bool isAccent, bool isSlide, float accentKnob) {
    gate_ = true;
    isAccent_ = isAccent;

    updateCoefficients();

    if (!isSlide) {
        // Start attack phase from current voltage level (do not hard-reset to 0.0, catch existing tail)
        vcfTarget_ = 1.0f;
        vcaTarget_ = 1.0f;
        // The x0x recordings: an unaccented note starts sounding ~4.5 ms
        // after an accented one (only a faint click at the gate), so its VCA
        // attack is held back.
        vcaDelaySamples_ = isAccent ? 0 : static_cast<int>(vcaNormalDelaySec_ * sampleRate_ + 0.5);
    } else {
        // Re-Trigger Logic (Slide == True):
        // Do not trigger attack phase of either envelope.
        // Let VCF envelope continue uninterrupted decay toward 0, hold VCA target at current/decay state.
        vcfTarget_ = 0.0f;
        vcaTarget_ = 0.0f;
    }
}

void Envelope::noteOff() {
    gate_ = false;
    vcaTarget_ = 0.0f;
    vcfTarget_ = 0.0f;
}

void Envelope::processNextSample() {
    // 1. VCF Filter Envelope Processing
    if (vcfTarget_ > vcfEnv_) {
        vcfEnv_ += vcfAttackCoeff_ * (vcfTarget_ - vcfEnv_);
        // C62 charges fully (tau ~0.1 ms) before the one-shot releases it.
        if (vcfEnv_ >= 0.999f) {
            vcfEnv_ = 1.0f;
            vcfTarget_ = 0.0f; // Transition smoothly to decay phase
        }
    } else {
        vcfEnv_ *= vcfDecayCoeff_;
    }

    // 2. VCA Amplitude Envelope Processing
    if (gate_ && vcaDelaySamples_ > 0) {
        --vcaDelaySamples_;
    } else if (gate_) {
        if (vcaTarget_ > vcaEnv_) {
            vcaEnv_ += vcaAttackCoeff_ * (vcaTarget_ - vcaEnv_);
            if (vcaEnv_ >= 0.99f) {
                vcaTarget_ = 0.0f; // Transition to slow decay
            }
        } else {
            vcaEnv_ *= vcaGateHighDecayCoeff_;
        }
    } else {
        vcaEnv_ *= isAccent_ ? vcaQuickDrainAccentCoeff_ : vcaQuickDrainCoeff_;
    }

    // 3. Accent-sweep network (see Envelope.hpp). MEG_acc only exists on
    // accented steps while the gate is high. Ideal diode: D24 conducts when
    // MEG_acc is above the voltage the network would sit at without it.
    {
        const double res = std::min(std::max(static_cast<double>(accentSweepRes_), 0.0), 1.0);
        const double rS = accentR46Sec_ + res * accentPotSec_;           // R46 + upper pot section
        const double rBot = (1.0 - res) * accentPotSec_;                 // lower pot section to C13
        const double rMix = accentMixSec_;
        // D24 only conducts while the (Accent-pot-scaled) MEG exceeds the
        // network by its forward drop, so the charging source is MEG - drop.
        const double vMeg = (isAccent_ && gate_) ? accentKnob_ * vcfEnv_ - accentDiodeDrop_ : 0.0;
        const double vc = accentCap_;
        const double vOff = vc * rMix / (rMix + rBot);                   // wiper with D24 off
        double a, b;                                                     // dVc/dt = a - b*Vc
        double vw;
        if (vMeg > vOff) {
            const double den = 1.0 + rBot / rS + rBot / rMix;
            a = vMeg / rS / den;
            b = (1.0 / rS + 1.0 / rMix) / den;
            vw = (rBot > 0.0) ? (vMeg / rS + vc / rBot) / (1.0 / rS + 1.0 / rMix + 1.0 / rBot) : vc;
        } else {
            a = 0.0;
            b = 1.0 / (rMix + rBot);
            vw = vOff;
        }
        const double k = 1.0 - std::exp(-b / sampleRate_);
        accentCap_ = static_cast<float>(vc + k * (a / b - vc));
        accentWiper_ = static_cast<float>(vw);
    }

    // 4. Accent VCA control path smoothing
    float accentVcaTarget = (isAccent_ && gate_) ? vcfEnv_ : 0.0f;
    accentVca_ += accentVcaCoeff_ * (accentVcaTarget - accentVca_);
}

} // namespace acidus
