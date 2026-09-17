#include "Filter.hpp"
#include <cmath>
#include <algorithm>

namespace syrebas {

namespace {

// Coupled diode-ladder derivatives (Section 7/84 of the emulation reference):
//   dv1/dt = w1 * Vt * [ tanh((u - v1)/Vt)  - tanh((v1 - v2)/Vt) ]
//   dv2/dt = w2 * Vt * [ tanh((v1 - v2)/Vt) - tanh((v2 - v3)/Vt) ]
//   dv3/dt = w3 * Vt * [ tanh((v2 - v3)/Vt) - tanh((v3 - v4)/Vt) ]
//   dv4/dt = 2*w4 * Vt * tanh((v3 - v4)/Vt)
// Each stage gets its own rate w_n = wc * capScale_n so the ladder's poles
// spread apart instead of coinciding, matching the 10/15/33/10 nF capacitors.
inline void ladderDerivatives(float u, float v1, float v2, float v3, float v4,
                               float wc, float Vt, float VtInv,
                               float c1, float c2, float c3, float c4,
                               float& dv1, float& dv2, float& dv3, float& dv4) {
    float t1 = std::tanh((u - v1) * VtInv);
    float t2 = std::tanh((v1 - v2) * VtInv);
    float t3 = std::tanh((v2 - v3) * VtInv);
    float t4 = std::tanh((v3 - v4) * VtInv);
    dv1 = wc * c1 * Vt * (t1 - t2);
    dv2 = wc * c2 * Vt * (t2 - t3);
    dv3 = wc * c3 * Vt * (t3 - t4);
    dv4 = 2.0f * wc * c4 * Vt * t4;
}

} // namespace

Filter::Filter() {
    setSampleRate(44100.0);
}

void Filter::setSampleRate(double sampleRate) {
    sampleRate_ = sampleRate;
    oversampledRateAccurate_ = sampleRate_ * 4.0;
    oversampledRateFaithful_ = sampleRate_ * 8.0;
    reset();
}

void Filter::reset() {
    ladderV1_ = ladderV2_ = ladderV3_ = ladderV4_ = 0.0f;
    hpFbStateX1_ = hpFbStateY1_ = 0.0f;
    prevAccurateInput_ = 0.0f;

    fLadderV1_ = fLadderV2_ = fLadderV3_ = fLadderV4_ = 0.0f;
    fHpFbStateX1_ = fHpFbStateY1_ = 0.0f;
    prevFaithfulInput_ = 0.0f;
    inputCoupling_.reset();
    outputCoupling_.reset();
}

float Filter::processAccurateSample(float input, float cutoffHz, float resonance) {
    // 4x oversampling step for accurate coupled diode ladder
    float dt = 1.0f / static_cast<float>(oversampledRateAccurate_);
    float totalCutoffHz = std::min(std::max(cutoffHz, 20.0f), 18000.0f);

    float wc = 2.0f * 3.14159265358979323846f * totalCutoffHz;

    // High pass in feedback path: cutoff dynamically scales between 150 Hz and 250 Hz (Section 13)
    float resNorm = std::min(std::max(resonance, 0.0f), 1.0f);
    float hpfCutoff = 150.0f + 100.0f * resNorm;
    float hpfAlpha = 1.0f / (1.0f + 2.0f * 3.14159265358979323846f * hpfCutoff * dt);

    // Coupled 4-stage diode ladder oscillation threshold k = 33.0 for self-oscillation & intense squelch
    float kFb = resNorm * 33.0f;

    // Physical BJT thermal voltage V_T = 26mV. Effective scale factor Vt = 2*V_T = 0.052V (Vt_inv = 1 / 0.052 = 19.23)
    const float Vt = 0.052f;
    const float Vt_inv = 19.23f;

    float accOut = 0.0f;

    float prevIn = prevAccurateInput_;
    prevAccurateInput_ = input;

    for (int os = 0; os < 4; ++os) {
        // Linear interpolation across 4x oversampling sub-steps
        float alphaOS = static_cast<float>(os + 1) / 4.0f;
        float currIn = prevIn + alphaOS * (input - prevIn);

        // Physical input signal voltage entering the ladder buffer (~0.05V RMS)
        float inSample = currIn * 0.05f;

        // Feedback calculation (hpOut is in volts matching ladderV4_)
        float hpOut = hpfAlpha * (hpFbStateY1_ + ladderV4_ - hpFbStateX1_);
        hpFbStateX1_ = ladderV4_;
        hpFbStateY1_ = hpOut;

        float u = inSample - hpOut * kFb;

        // Coupled Diode Ladder Differential Equations with physical BJT differential pair scaling (Section 7, 84)
        // dv1/dt = w * Vt * [ tanh((u - v1)/Vt) - tanh((v1 - v2)/Vt) ]
        // dv2/dt = w * Vt * [ tanh((v1 - v2)/Vt) - tanh((v2 - v3)/Vt) ]
        // dv3/dt = w * Vt * [ tanh((v2 - v3)/Vt) - tanh((v3 - v4)/Vt) ]
        // dv4/dt = 2w * Vt * tanh((v3 - v4)/Vt)
        float h = dt;
        float v1 = ladderV1_;
        float v2 = ladderV2_;
        float v3 = ladderV3_;
        float v4 = ladderV4_;

        // K1
        float dv1_1 = wc * Vt * (std::tanh((u - v1) * Vt_inv) - std::tanh((v1 - v2) * Vt_inv));
        float dv2_1 = wc * Vt * (std::tanh((v1 - v2) * Vt_inv) - std::tanh((v2 - v3) * Vt_inv));
        float dv3_1 = wc * Vt * (std::tanh((v2 - v3) * Vt_inv) - std::tanh((v3 - v4) * Vt_inv));
        float dv4_1 = 2.0f * wc * Vt * std::tanh((v3 - v4) * Vt_inv);

        // K2
        float v1_mid = v1 + 0.5f * h * dv1_1;
        float v2_mid = v2 + 0.5f * h * dv2_1;
        float v3_mid = v3 + 0.5f * h * dv3_1;
        float v4_mid = v4 + 0.5f * h * dv4_1;

        float hpOut_mid = hpfAlpha * (hpFbStateY1_ + v4_mid - hpFbStateX1_);
        float u_mid = inSample - hpOut_mid * kFb;

        float dv1_2 = wc * Vt * (std::tanh((u_mid - v1_mid) * Vt_inv) - std::tanh((v1_mid - v2_mid) * Vt_inv));
        float dv2_2 = wc * Vt * (std::tanh((v1_mid - v2_mid) * Vt_inv) - std::tanh((v2_mid - v3_mid) * Vt_inv));
        float dv3_2 = wc * Vt * (std::tanh((v2_mid - v3_mid) * Vt_inv) - std::tanh((v3_mid - v4_mid) * Vt_inv));
        float dv4_2 = 2.0f * wc * Vt * std::tanh((v3_mid - v4_mid) * Vt_inv);

        ladderV1_ += h * dv1_2;
        ladderV2_ += h * dv2_2;
        ladderV3_ += h * dv3_2;
        ladderV4_ += h * dv4_2;

        accOut += (ladderV4_ / 0.05f) * 0.25f; // Normalize voltage and average 4x decimation
    }

    return accOut;
}

float Filter::processFaithfulSample(float input, float cutoffHz, float resonance) {
    // 8x oversampling (Section 55 recommends 4x minimum, "preferably 8x") for the
    // nonlinear ladder, since aliasing from the tanh junctions is otherwise audible.
    constexpr int kOS = 8;
    float dt = 1.0f / static_cast<float>(oversampledRateFaithful_);
    float totalCutoffHz = std::min(std::max(cutoffHz, 20.0f), 18000.0f);
    float wc = 2.0f * 3.14159265358979323846f * totalCutoffHz;

    float resNorm = std::min(std::max(resonance, 0.0f), 1.0f);
    float hpfCutoff = 150.0f + 100.0f * resNorm;
    float hpfAlpha = 1.0f / (1.0f + 2.0f * 3.14159265358979323846f * hpfCutoff * dt);

    // Real ladders sit below clean self-oscillation (Section 12); push the threshold
    // a little further out than the accurate mode's so resonance keeps "squelching"
    // rather than ringing cleanly even near maximum.
    float kFb = resNorm * 36.0f;

    const float Vt = 0.052f;
    const float VtInv = 19.23f;

    // Input coupling cap (Section 10/47): blocks DC and trims sub-bass ahead of the ladder.
    float inCouplingAlpha = 1.0f - std::exp(-2.0f * 3.14159265358979323846f * 20.0f * dt);
    // Output/buffer bandwidth limit representing the extra high-frequency coupling poles.
    float outCouplingAlpha = 1.0f - std::exp(-2.0f * 3.14159265358979323846f * 20000.0f * dt);

    float out = 0.0f;
    float prevIn = prevFaithfulInput_;
    prevFaithfulInput_ = input;

    for (int os = 0; os < kOS; ++os) {
        float alphaOS = static_cast<float>(os + 1) / static_cast<float>(kOS);
        float currIn = prevIn + alphaOS * (input - prevIn);

        float inSample = currIn * 0.05f;
        inSample = inputCoupling_.highpass(inSample, inCouplingAlpha);

        float h = dt;
        float v1 = fLadderV1_, v2 = fLadderV2_, v3 = fLadderV3_, v4 = fLadderV4_;

        // Commit the feedback HPF's one-pole state once per oversample step (from
        // the pre-step v4), then reuse that committed state to estimate the
        // feedback voltage at each RK stage's predicted v4 without advancing the
        // filter's history multiple times per sample.
        float hpOut0 = hpfAlpha * (fHpFbStateY1_ + v4 - fHpFbStateX1_);
        fHpFbStateX1_ = v4;
        fHpFbStateY1_ = hpOut0;
        float u0 = inSample - hpOut0 * kFb;

        auto feedbackFor = [&](float v4pred) {
            float hpOut = hpfAlpha * (fHpFbStateY1_ + v4pred - fHpFbStateX1_);
            return inSample - hpOut * kFb;
        };

        // Classical RK4 on the coupled ladder, re-deriving the nonlinear feedback
        // voltage at each stage from that stage's predicted v4 (closer to an
        // implicit solve than the accurate mode's 2-stage predictor/corrector,
        // without the cost of a full Newton iteration on a 4x4 Jacobian).
        float dv1_1, dv2_1, dv3_1, dv4_1;
        ladderDerivatives(u0, v1, v2, v3, v4, wc, Vt, VtInv,
                           capScale1_, capScale2_, capScale3_, capScale4_,
                           dv1_1, dv2_1, dv3_1, dv4_1);

        float v1a = v1 + 0.5f * h * dv1_1, v2a = v2 + 0.5f * h * dv2_1;
        float v3a = v3 + 0.5f * h * dv3_1, v4a = v4 + 0.5f * h * dv4_1;
        float ua = feedbackFor(v4a);
        float dv1_2, dv2_2, dv3_2, dv4_2;
        ladderDerivatives(ua, v1a, v2a, v3a, v4a, wc, Vt, VtInv,
                           capScale1_, capScale2_, capScale3_, capScale4_,
                           dv1_2, dv2_2, dv3_2, dv4_2);

        float v1b = v1 + 0.5f * h * dv1_2, v2b = v2 + 0.5f * h * dv2_2;
        float v3b = v3 + 0.5f * h * dv3_2, v4b = v4 + 0.5f * h * dv4_2;
        float ub = feedbackFor(v4b);
        float dv1_3, dv2_3, dv3_3, dv4_3;
        ladderDerivatives(ub, v1b, v2b, v3b, v4b, wc, Vt, VtInv,
                           capScale1_, capScale2_, capScale3_, capScale4_,
                           dv1_3, dv2_3, dv3_3, dv4_3);

        float v1c = v1 + h * dv1_3, v2c = v2 + h * dv2_3;
        float v3c = v3 + h * dv3_3, v4c = v4 + h * dv4_3;
        float uc = feedbackFor(v4c);
        float dv1_4, dv2_4, dv3_4, dv4_4;
        ladderDerivatives(uc, v1c, v2c, v3c, v4c, wc, Vt, VtInv,
                           capScale1_, capScale2_, capScale3_, capScale4_,
                           dv1_4, dv2_4, dv3_4, dv4_4);

        fLadderV1_ = v1 + (h / 6.0f) * (dv1_1 + 2.0f * dv1_2 + 2.0f * dv1_3 + dv1_4);
        fLadderV2_ = v2 + (h / 6.0f) * (dv2_1 + 2.0f * dv2_2 + 2.0f * dv2_3 + dv2_4);
        fLadderV3_ = v3 + (h / 6.0f) * (dv3_1 + 2.0f * dv3_2 + 2.0f * dv3_3 + dv3_4);
        fLadderV4_ = v4 + (h / 6.0f) * (dv4_1 + 2.0f * dv4_2 + 2.0f * dv4_3 + dv4_4);

        float stageOut = fLadderV4_ / 0.05f;
        stageOut = outputCoupling_.lowpass(stageOut, outCouplingAlpha);
        out += stageOut / static_cast<float>(kOS);
    }

    return out;
}

} // namespace syrebas
