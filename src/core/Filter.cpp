#include "Filter.hpp"
#include <cmath>
#include <algorithm>

namespace acidus {

namespace {

// Solve a 4x4 tridiagonal system via the Thomas algorithm:
//   row0: a[0]*x0 + bUp[0]*x1                                   = d[0]
//   row1: cLow[0]*x0 + a[1]*x1 + bUp[1]*x2                      = d[1]
//   row2:              cLow[1]*x1 + a[2]*x2 + bUp[2]*x3         = d[2]
//   row3:                           cLow[2]*x2 + a[3]*x3        = d[3]
// `a` and `d` are modified in place (forward elimination); results land in x.
inline void solveTridiagonal4(float a[4], const float bUp[3], const float cLow[3],
                               float d[4], float x[4]) {
    for (int i = 1; i < 4; ++i) {
        float w = cLow[i - 1] / a[i - 1];
        a[i] -= w * bUp[i - 1];
        d[i] -= w * d[i - 1];
    }
    x[3] = d[3] / a[3];
    for (int i = 2; i >= 0; --i) {
        x[i] = (d[i] - bUp[i] * x[i + 1]) / a[i];
    }
}

} // namespace

Filter::Filter() {
    setSampleRate(44100.0);
}

void Filter::setSampleRate(double sampleRate) {
    sampleRate_ = sampleRate;
    oversampledRate_ = sampleRate_ * 8.0;
    reset();
}

void Filter::reset() {
    fLadderV1_ = fLadderV2_ = fLadderV3_ = fLadderV4_ = 0.0f;
    fHpFbStateX1_ = fHpFbStateY1_ = 0.0f;
    prevInput_ = 0.0f;
    inputCoupling_.reset();
    outputCoupling_.reset();
}

float Filter::processSample(float input, float cutoffHz, float resonance) {
    constexpr int kOS = 8;
    float dt = 1.0f / static_cast<float>(oversampledRate_);
    float totalCutoffHz = std::min(std::max(cutoffHz, 20.0f), 18000.0f);
    float wc = 2.0f * 3.14159265358979323846f * totalCutoffHz * kCutoffToOmegaScale_;

    float resNorm = std::min(std::max(resonance, 0.0f), 1.0f);

    const float resCouplingHz = resCouplingHz_ + 100.0f * resNorm;
    const float resCouplingAlpha = 1.0f / (1.0f + 2.0f * 3.14159265358979323846f * resCouplingHz * dt);

    float kFb = skewResonance(resNorm)
              * (feedbackGainCeiling_ + kResonanceGainMargin_ * kCutoffHeadroomNumerator_ / totalCutoffHz);

    const float Vt = 0.052f;
    const float VtInv = 19.23f;

    float inCouplingAlpha = 1.0f - std::exp(-2.0f * 3.14159265358979323846f * 20.0f * dt);
    float outCouplingAlpha = 1.0f - std::exp(-2.0f * 3.14159265358979323846f * 20000.0f * dt);

    float out = 0.0f;
    float prevIn = prevInput_;
    prevInput_ = input;

    constexpr int kNewtonIters = 3;
    const float w1 = wc * capScale1_, w2 = wc * capScale2_;
    const float w3 = wc * capScale3_, w4 = wc * capScale4_;

    for (int os = 0; os < kOS; ++os) {
        float alphaOS = static_cast<float>(os + 1) / static_cast<float>(kOS);
        float currIn = prevIn + alphaOS * (input - prevIn);

        float inSample = currIn * 0.05f;
        inSample = inputCoupling_.highpass(inSample, inCouplingAlpha);

        float h = dt;
        float v1 = fLadderV1_, v2 = fLadderV2_, v3 = fLadderV3_, v4 = fLadderV4_;

        float hpOut0 = resCouplingAlpha * (fHpFbStateY1_ + v4 - fHpFbStateX1_);
        fHpFbStateX1_ = v4;
        fHpFbStateY1_ = hpOut0;
        float u0 = inSample - hpOut0 * kFb;

        auto feedbackFor = [&](float v4pred) {
            float hpOut = resCouplingAlpha * (fHpFbStateY1_ + v4pred - fHpFbStateX1_);
            return inSample - hpOut * kFb;
        };

        float T1o = std::tanh((u0 - v1) * VtInv);
        float T2o = std::tanh((v1 - v2) * VtInv);
        float T3o = std::tanh((v2 - v3) * VtInv);
        float T4o = std::tanh((v3 - v4) * VtInv);
        float f1o = w1 * Vt * (T1o - T2o);
        float f2o = w2 * Vt * (T2o - T3o);
        float f3o = w3 * Vt * (T3o - T4o);
        float f4o = 2.0f * w4 * Vt * T4o;

        float k1 = v1 + h * f1o, k2 = v2 + h * f2o;
        float k3 = v3 + h * f3o, k4 = v4 + h * f4o;

        for (int iter = 0; iter < kNewtonIters; ++iter) {
            float uk = feedbackFor(k4);
            float T1 = std::tanh((uk - k1) * VtInv);
            float T2 = std::tanh((k1 - k2) * VtInv);
            float T3 = std::tanh((k2 - k3) * VtInv);
            float T4 = std::tanh((k3 - k4) * VtInv);
            float S1 = 1.0f - T1 * T1;
            float S2 = 1.0f - T2 * T2;
            float S3 = 1.0f - T3 * T3;
            float S4 = 1.0f - T4 * T4;

            float f1 = w1 * Vt * (T1 - T2);
            float f2 = w2 * Vt * (T2 - T3);
            float f3 = w3 * Vt * (T3 - T4);
            float f4 = 2.0f * w4 * Vt * T4;

            float R1 = k1 - v1 - 0.5f * h * (f1o + f1);
            float R2 = k2 - v2 - 0.5f * h * (f2o + f2);
            float R3 = k3 - v3 - 0.5f * h * (f3o + f3);
            float R4 = k4 - v4 - 0.5f * h * (f4o + f4);

            float hh = 0.5f * h;
            float a[4] = {
                1.0f + hh * w1 * (S1 + S2),
                1.0f + hh * w2 * (S2 + S3),
                1.0f + hh * w3 * (S3 + S4),
                1.0f + hh * 2.0f * w4 * S4
            };
            float bUp[3] = { -hh * w1 * S2, -hh * w2 * S3, -hh * w3 * S4 };
            float cLow[3] = { -hh * w2 * S2, -hh * w3 * S3, -hh * 2.0f * w4 * S4 };
            float d[4] = { -R1, -R2, -R3, -R4 };
            float delta[4];
            solveTridiagonal4(a, bUp, cLow, d, delta);

            k1 += delta[0];
            k2 += delta[1];
            k3 += delta[2];
            k4 += delta[3];
        }

        if (std::isfinite(k1) && std::isfinite(k2) && std::isfinite(k3) && std::isfinite(k4)) {
            fLadderV1_ = k1;
            fLadderV2_ = k2;
            fLadderV3_ = k3;
            fLadderV4_ = k4;
        } else {
            fLadderV1_ = v1;
            fLadderV2_ = v2;
            fLadderV3_ = v3;
            fLadderV4_ = v4;
        }

        float stageOut = fLadderV4_ / 0.05f;
        stageOut = outputCoupling_.lowpass(stageOut, outCouplingAlpha);
        out += stageOut / static_cast<float>(kOS);
    }

    float outputGain = 1.0f + skewResonance(resNorm) * (kMaxResonanceOutputGain_ - 1.0f);
    return out * outputGain;
}

} // namespace acidus
