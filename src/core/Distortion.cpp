#include "Distortion.hpp"
#include <cmath>
#include <algorithm>

namespace acidus {

namespace {
constexpr float kPi = 3.14159265358979323846f;

// Reissue (Dunlop) component values, per the compendium's Section 4.2/11.4
// cheat sheet. R3/R4/C3 set the gain-stage ground leg; the Distortion pot
// (0-500k reverse-log) is the only value the knob moves.
constexpr float kR3 = 4700.0f;
constexpr float kR4 = 1.0e6f;
constexpr float kC3 = 47.0e-9f;
constexpr float kRpotMax = 500000.0f;

// Ideal HF-plateau gain at the two ends of the Distortion pot's travel
// (Section 5.2 table), used to map the drive knob onto the pot's range.
constexpr float kGainMinDb = 9.5f;   // Rpot = 500k
constexpr float kGainMaxDb = 46.6f;  // Rpot = 0

// LM741 gain-bandwidth product (customary figure, not a datasheet spec for
// the plain 741 - Section 3/7.1 priority #1: the treble corner tracks it).
constexpr float kGbwHz = 1.0e6f;

// Post-op-amp network (coupling cap, series resistor, volume pot, C5).
constexpr float kR5 = 10000.0f;
constexpr float kRvol = 10000.0f;   // stock 10k volume pot, held at full (unity)
constexpr float kC5 = 1.0e-9f;
constexpr float kCouplingHz = 8.0f; // C4 high-pass corner (Section 5.4)

// Germanium diode fit ("Ge fit A", Section 6.1: Vf ~= 0.28 V @ 1 mA).
constexpr float kDiodeIs = 240.0e-9f;
constexpr float kDiodeN = 1.3f;
constexpr float kThermalVoltage = 0.02585f;

// Asymmetric 9V-rail op-amp output swing about the +4.5V bias (Section 6.1,
// "assumed, not directly sourced" - the same caveat the compendium raises).
constexpr float kVHigh = 3.0f;
constexpr float kVLow = 2.6f;

// Calibration from the plugin's float domain into the circuit's volts. The
// pedal sits after the 303's Volume knob and output stage (SynthEngine.cpp,
// kOutputStageGain), which put a typical note near 0 dBFS peak at the default
// Volume and ~+6 dBFS at full Volume; a full-scale sample is a ~50 mV pedal
// input. At the default Volume the diodes start clipping about a quarter of
// the way up the Distortion travel and the 741 hits its rails from about two
// thirds (Section 6.1); full Volume reaches both sooner, while low Volume and
// low Distortion stay a crunch.
//
// The output scale is set so that, below clipping, the pedal at minimum
// Distortion has the same level as bypass: its small-signal gain there is
// the gain stage's 9.5 dB plateau times the R5/volume-pot divider (about
// 1.5x, i.e. +3.5 dB on the real pedal with Output at full), so dividing by
// that brings engaging the pedal at 1 % in at the bypass level. Turning
// Distortion up then only adds level until the diodes take over and hold
// the peaks at their ~0.2-0.3 V ceiling (Section 6.1).
constexpr float kInputVoltScale = 0.05f;
constexpr float kGainMinLinear = 2.985f;   // 10^(kGainMinDb / 20)
constexpr float kOutputVoltScale = kInputVoltScale * kGainMinLinear * (kRvol / (kR5 + kRvol));

// --- Auto output (not part of the circuit) ----------------------------------
// The real pedal's Output knob is left at full above, which makes full
// Distortion ~20 dB louder (RMS) than bypass. Auto output stands in for a
// player riding that knob: a static trim, set by the Distortion knob alone,
// that leaves the bottom of the travel at the bypass level and scales the
// extra loudness down so that full Distortion ends up kAutoOutputMaxBoostDb
// louder than bypass. Static, so the pedal's own compression and dynamics
// are untouched (no pumping).
//
// TUNE BY EAR: how much louder (RMS, dB) full Distortion is than bypass.
// 0 = level-matched across the whole travel; ~20 = auto output off.
constexpr float kAutoOutputMaxBoostDb = 6.0f;

// Model of the pedal's extra loudness over bypass at a given Distortion
// setting (Output at full): it rises with the gain stage's dB-linear sweep,
// then levels off as the diodes take over. Fitted to renders of a typical
// saw patch at the default 303 Volume (within ~0.6 dB across the travel).
constexpr float kExtraLoudnessCeilingDb = 22.0f;
inline float extraLoudnessDb(float drive) {
    return kExtraLoudnessCeilingDb *
           std::tanh(drive * (kGainMaxDb - kGainMinDb) / kExtraLoudnessCeilingDb);
}

constexpr int kOversample = 8;
constexpr int kDiodeNewtonIters = 5;
} // namespace

Distortion::Distortion() {
    setSampleRate(44100.0);
}

void Distortion::setSampleRate(double sampleRate) {
    sampleRate_ = sampleRate;
    oversampledRate_ = sampleRate_ * static_cast<double>(kOversample);
    reset();
}

void Distortion::reset() {
    prevInput_ = 0.0f;
    shelfX1_ = shelfY1_ = 0.0f;
    gbwState_ = 0.0f;
    couplingState_ = 0.0f;
    diodeV_ = 0.0f;
    diodeGPrev_ = 0.0f;
}

float Distortion::processSample(float input, float drive) {
    drive = std::min(std::max(drive, 0.0f), 1.0f);
    if (drive <= 0.0f) {
        // Footswitch up: true bypass, and leave the stage's state settled so
        // re-engaging the pedal starts clean instead of from a stale clip.
        reset();
        return input;
    }

    const float dt = 1.0f / static_cast<float>(oversampledRate_);

    if (drive != autoOutputDrive_) {
        // Scale the modelled extra loudness so full Distortion lands at
        // kAutoOutputMaxBoostDb; trim off the rest.
        const float keep = std::min(kAutoOutputMaxBoostDb / extraLoudnessDb(1.0f), 1.0f);
        const float trimDb = -(1.0f - keep) * extraLoudnessDb(drive);
        autoOutputGain_ = std::pow(10.0f, trimDb / 20.0f);
        autoOutputDrive_ = drive;
    }

    // --- Distortion-knob-dependent gain stage (Section 5.2) -----------------
    // Map drive linearly in dB across the reissue's gain range, then solve
    // for the pot resistance that produces it (G = 1 + R4/(R3+Rpot)).
    float gainDb = kGainMinDb + drive * (kGainMaxDb - kGainMinDb);
    float gainLinear = std::pow(10.0f, gainDb / 20.0f);
    float rPot = kR4 / (gainLinear - 1.0f) - kR3;
    rPot = std::min(std::max(rPot, 0.0f), kRpotMax);

    float tauPole = kC3 * (kR3 + rPot);
    float tauZero = kC3 * (kR3 + rPot + kR4);

    // Tustin-discretized first-order shelf: H(s) = (1 + s*tauZero)/(1 + s*tauPole)
    float twoOverT = 2.0f / dt;
    float aNum = tauZero * twoOverT;
    float bDen = tauPole * twoOverT;
    float shelfB0 = (1.0f + aNum) / (1.0f + bDen);
    float shelfB1 = (1.0f - aNum) / (1.0f + bDen);
    float shelfA1 = (1.0f - bDen) / (1.0f + bDen);

    // --- 741 gain-bandwidth-limited treble rolloff (Section 5.3) -----------
    // The corner falls as gain rises, reproducing the "mid hump" that
    // narrows and drops as the Distortion knob is turned up.
    float gbwCornerHz = kGbwHz / gainLinear;
    float gbwAlpha = 1.0f - std::exp(-2.0f * kPi * gbwCornerHz * dt);

    // --- Post-op-amp network / diode-clipper constants (Section 5.4/5.5) ---
    constexpr float kRth = (kR5 * kRvol) / (kR5 + kRvol);
    constexpr float kTheveninDivider = kRvol / (kR5 + kRvol);
    float couplingAlpha = 1.0f - std::exp(-2.0f * kPi * kCouplingHz * dt);

    constexpr float a = kDiodeN * kThermalVoltage;
    float alphaNode = 2.0f * kC5 / dt + 1.0f / kRth;

    float out = 0.0f;
    float prevIn = prevInput_;
    prevInput_ = input;

    for (int os = 0; os < kOversample; ++os) {
        float frac = static_cast<float>(os + 1) / static_cast<float>(kOversample);
        float xIn = prevIn + frac * (input - prevIn);
        float xVolt = xIn * kInputVoltScale;

        // Gain-stage shelf (direct form 1).
        float shelfOut = shelfB0 * xVolt + shelfB1 * shelfX1_ - shelfA1 * shelfY1_;
        shelfX1_ = xVolt;
        shelfY1_ = shelfOut;

        // GBW-limited lowpass.
        gbwState_ += gbwAlpha * (shelfOut - gbwState_);

        // Asymmetric op-amp rail saturation.
        float sat = (gbwState_ >= 0.0f)
            ? kVHigh * std::tanh(gbwState_ / kVHigh)
            : kVLow * std::tanh(gbwState_ / kVLow);

        // C4 coupling high-pass: blocks the DC the asymmetric clip creates.
        couplingState_ += couplingAlpha * (sat - couplingState_);
        float hpOut = sat - couplingState_;

        float vth = hpOut * kTheveninDivider;

        // Dynamic antiparallel-diode shunt clipper: trapezoidal rule, solved
        // by Newton-Raphson from the previous node voltage (Section 7.4/11.1).
        float K = (2.0f * kC5 / dt) * diodeV_ + vth / kRth + diodeGPrev_;
        float v = diodeV_;
        for (int iter = 0; iter < kDiodeNewtonIters; ++iter) {
            float x = std::min(std::max(v / a, -60.0f), 60.0f);
            // sinh and cosh from one expm1 (no cancellation near 0).
            float em = std::expm1(x);
            float e = em + 1.0f;
            float sh = 0.5f * (em + em / e);
            float ch = 0.5f * (e + 1.0f / e);
            float f = alphaNode * v + 2.0f * kDiodeIs * sh - K;
            float fp = alphaNode + (2.0f * kDiodeIs / a) * ch;
            const float dv = f / fp;
            v -= dv;
            if (std::abs(dv) <= 1e-7f * std::abs(v)) break;   // converged to float precision
        }
        if (std::isfinite(v)) {
            diodeV_ = v;
        }
        diodeGPrev_ = (vth - diodeV_) / kRth - 2.0f * kDiodeIs * std::sinh(diodeV_ / a);

        float stageOut = diodeV_ / kOutputVoltScale;
        out += stageOut / static_cast<float>(kOversample);
    }

    return out * autoOutputGain_;
}

} // namespace acidus
