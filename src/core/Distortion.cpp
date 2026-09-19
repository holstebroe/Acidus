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

// Calibration from the plugin's -1..1 float domain into the circuit's volts:
// a full-scale sample is treated as a hot ~50 mV pedal input, matching a
// guitar peak level scaled for a punchy bass synth; the output scale maps
// the diode clipper's self-limited ceiling (~0.2-0.3 V, Section 6.1) back to
// roughly unity so max-drive square waves don't blow past full scale.
constexpr float kInputVoltScale = 0.05f;
constexpr float kOutputVoltScale = 0.25f;

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
            float sh = std::sinh(x);
            float ch = std::cosh(x);
            float f = alphaNode * v + 2.0f * kDiodeIs * sh - K;
            float fp = alphaNode + (2.0f * kDiodeIs / a) * ch;
            v -= f / fp;
        }
        if (std::isfinite(v)) {
            diodeV_ = v;
        }
        diodeGPrev_ = (vth - diodeV_) / kRth - 2.0f * kDiodeIs * std::sinh(diodeV_ / a);

        float stageOut = diodeV_ / kOutputVoltScale;
        out += stageOut / static_cast<float>(kOversample);
    }

    return out;
}

} // namespace acidus
