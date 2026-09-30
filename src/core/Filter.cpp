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

// Solve a dense 4x4 system with partial pivoting; A and b are overwritten.
inline void solveDense4(float A[4][4], float b[4], float x[4]) {
    for (int c = 0; c < 4; ++c) {
        int piv = c;
        for (int r = c + 1; r < 4; ++r)
            if (std::abs(A[r][c]) > std::abs(A[piv][c])) piv = r;
        if (piv != c) {
            for (int k = 0; k < 4; ++k) std::swap(A[c][k], A[piv][k]);
            std::swap(b[c], b[piv]);
        }
        for (int r = c + 1; r < 4; ++r) {
            float w = A[r][c] / A[c][c];
            for (int k = c; k < 4; ++k) A[r][k] -= w * A[c][k];
            b[r] -= w * b[c];
        }
    }
    for (int r = 3; r >= 0; --r) {
        float acc = b[r];
        for (int k = r + 1; k < 4; ++k) acc -= A[r][k] * x[k];
        x[r] = acc / A[r][r];
    }
}

} // namespace

void Filter::updatePoles() {
    const float sc[4] = {capScale1_, capScale2_, capScale3_, capScale4_};
    if (sc[0] == polesForScales_[0] && sc[1] == polesForScales_[1]
        && sc[2] == polesForScales_[2] && sc[3] == polesForScales_[3]) return;
    for (int i = 0; i < 4; ++i) polesForScales_[i] = sc[i];

    // Characteristic polynomial of the linearised ladder (wc = 1) via the
    // tridiagonal continuant. Diagonal -2*s_i; the off-diagonal products are
    // s1*s2, s2*s3 and 2*s3*s4 (the x2 is the stage-4 term in processSample).
    const double s1 = sc[0], s2 = sc[1], s3 = sc[2], s4 = sc[3];
    auto charPoly = [&](double l) {
        double f0 = 1.0;
        double f1 = l + 2.0 * s1;
        double f2 = (l + 2.0 * s2) * f1 - s1 * s2 * f0;
        double f3 = (l + 2.0 * s3) * f2 - s2 * s3 * f1;
        return (l + 2.0 * s4) * f3 - 2.0 * s3 * s4 * f2;
    };
    // All four roots are real and negative; Gershgorin bounds them to
    // [-5*max(s), 0]. Bracket by scanning, then bisect.
    const double lo = -5.0 * std::max({s1, s2, s3, s4}) - 1e-6;
    const int kScan = 4000;
    int found = 0;
    double prevL = lo, prevV = charPoly(lo);
    for (int i = 1; i <= kScan && found < 4; ++i) {
        double l = lo * (1.0 - static_cast<double>(i) / kScan);
        double v = charPoly(l);
        if (v == 0.0 || (v > 0.0) != (prevV > 0.0)) {
            double a = prevL, b = l, fa = prevV;
            for (int it = 0; it < 60; ++it) {
                double m = 0.5 * (a + b), fm = charPoly(m);
                if ((fm > 0.0) == (fa > 0.0)) { a = m; fa = fm; } else { b = m; }
            }
            poles_[found++] = -0.5 * (a + b);
        }
        prevL = l;
        prevV = v;
    }
    kcTableValid_ = false; // the k_crit table depends on the poles
}

double Filter::criticalFeedbackGainExact(double cutoffHz) {
    updatePoles();
    const double wc = 2.0 * 3.14159265358979323846 * cutoffHz * kCutoffToOmegaScale_;
    const double h = 2.0 * 3.14159265358979323846 * resCouplingHz_ / wc;
    // Loop gain G(jx) = H_ladder(jx) * HP(jx), x = w / wc. Its phase falls
    // monotonically from +90 deg to -360 deg; find the -180 deg crossing.
    auto phase = [&](double x) {
        double ph = std::atan2(h, x);
        for (double p : poles_) ph -= std::atan(x / p);
        return ph;
    };
    double a = std::log(1e-4), b = std::log(1e3);
    for (int it = 0; it < 48; ++it) {
        double m = 0.5 * (a + b);
        if (phase(std::exp(m)) > -3.14159265358979323846) a = m; else b = m;
    }
    const double x = std::exp(0.5 * (a + b));
    double mag = x / std::sqrt(x * x + h * h);
    for (double p : poles_) mag *= p / std::sqrt(x * x + p * p);
    return 1.0 / mag;
}

void Filter::buildKcTable() {
    const double lo = std::log(20.0);
    const double hi = std::log(std::max(40.0, 0.1 * oversampledRate_));
    const double step = (hi - lo) / (kKcTableSize - 1);
    for (int i = 0; i < kKcTableSize; ++i) {
        kcTable_[i] = static_cast<float>(criticalFeedbackGainExact(std::exp(lo + step * i)));
    }
    kcTableLogLo_ = static_cast<float>(lo);
    kcTableInvStep_ = static_cast<float>(1.0 / step);
    kcTableCouplingHz_ = resCouplingHz_;
    kcTableOsRate_ = oversampledRate_;
    kcTableValid_ = true;
}

float Filter::criticalFeedbackGain(float cutoffHz) {
    updatePoles();
    if (!kcTableValid_ || resCouplingHz_ != kcTableCouplingHz_ || oversampledRate_ != kcTableOsRate_) {
        buildKcTable();
    }
    float u = (std::log(std::max(cutoffHz, 1.0f)) - kcTableLogLo_) * kcTableInvStep_;
    u = std::min(std::max(u, 0.0f), static_cast<float>(kKcTableSize - 1));
    const int i = std::min(static_cast<int>(u), kKcTableSize - 2);
    const float f = u - static_cast<float>(i);
    return kcTable_[i] + f * (kcTable_[i + 1] - kcTable_[i]);
}

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
    postFilterHp_.reset();
    notch_.reset();
    allpass_.reset();
}

float Filter::processSample(float input, float cutoffHz, float resonance) {
    constexpr int kOS = 8;
    if (!std::isfinite(input)) input = 0.0f;
    float dt = 1.0f / static_cast<float>(oversampledRate_);
    // Safety clamp only: the engine limits the cutoff CV (cutoffMaxHz); the
    // 8x oversampled solver stays accurate to ~10 % of its own rate.
    float totalCutoffHz = std::min(std::max(cutoffHz, 20.0f), 0.1f * static_cast<float>(oversampledRate_));
    float wc = 2.0f * 3.14159265358979323846f * totalCutoffHz * kCutoffToOmegaScale_;

    float resNorm = std::min(std::max(resonance, 0.0f), 1.0f);

    const float resCouplingAlpha = 1.0f / (1.0f + 2.0f * 3.14159265358979323846f * resCouplingHz_ * dt);

    // Feedback at full Resonance: the fixed ceiling, capped at a fraction of
    // the loop's critical gain at this cutoff (see setResonanceLimit).
    const float kMax = std::min(feedbackGainCeiling_, resonanceLimit_ * criticalFeedbackGain(totalCutoffHz));
    float kFb = skewResonance(resNorm) * kMax;
    if (!std::isfinite(kFb)) kFb = 0.0f;

    const float Vt = 0.052f;
    const float VtInv = 19.23f;

    // Coupling coefficients: recomputed only when a corner or the rate changes.
    if (inputCouplingHz_ != couplingForInHz_ || outputCouplingHz_ != couplingForOutHz_ || dt != couplingForDt_) {
        inCouplingAlpha_ = 1.0f - std::exp(-2.0f * 3.14159265358979323846f * inputCouplingHz_ * dt);
        outCouplingAlpha_ = 1.0f - std::exp(-2.0f * 3.14159265358979323846f * outputCouplingHz_ * dt);
        couplingForInHz_ = inputCouplingHz_;
        couplingForOutHz_ = outputCouplingHz_;
        couplingForDt_ = dt;
    }
    const float inCouplingAlpha = inCouplingAlpha_;
    const float outCouplingAlpha = outCouplingAlpha_;

    float out = 0.0f;
    float prevIn = prevInput_;
    prevInput_ = input;

    constexpr int kNewtonIters = 3;
    const float w1 = wc * capScale1_, w2 = wc * capScale2_;
    const float w3 = wc * capScale3_, w4 = wc * capScale4_;

    for (int os = 0; os < kOS; ++os) {
        float alphaOS = static_cast<float>(os + 1) / static_cast<float>(kOS);
        float currIn = prevIn + alphaOS * (input - prevIn);

        float inSample = currIn * ladderInputScale_;
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

        float k1, k2, k3, k4;
        if (ladderTopology_ == 1) {
            // Circuit orientation (§10.3): input pair tanh(u), half capacitor
            // on stage 1, terminal tanh(y4). Full Newton, including the
            // feedback path's dependence on the new y4 (dense 4x4 solve).
            auto rhs = [&](float u, float y1, float y2, float y3, float y4, float f[4]) {
                float Tu = std::tanh(u * VtInv), T12 = std::tanh((y1 - y2) * VtInv);
                float T23 = std::tanh((y2 - y3) * VtInv), T34 = std::tanh((y3 - y4) * VtInv);
                float T4 = std::tanh(y4 * VtInv);
                f[0] = 2.0f * w1 * Vt * (Tu - T12);
                f[1] = w2 * Vt * (T12 - T23);
                f[2] = w3 * Vt * (T23 - T34);
                f[3] = w4 * Vt * (T34 - T4);
            };
            float fo[4];
            rhs(u0, v1, v2, v3, v4, fo);
            k1 = v1 + h * fo[0]; k2 = v2 + h * fo[1];
            k3 = v3 + h * fo[2]; k4 = v4 + h * fo[3];
            const float dUdY4 = -kFb * resCouplingAlpha;
            for (int iter = 0; iter < kNewtonIters; ++iter) {
                float uk = feedbackFor(k4);
                float Tu = std::tanh(uk * VtInv), T12 = std::tanh((k1 - k2) * VtInv);
                float T23 = std::tanh((k2 - k3) * VtInv), T34 = std::tanh((k3 - k4) * VtInv);
                float T4 = std::tanh(k4 * VtInv);
                float Su = 1.0f - Tu * Tu, S12 = 1.0f - T12 * T12, S23 = 1.0f - T23 * T23;
                float S34 = 1.0f - T34 * T34, S4 = 1.0f - T4 * T4;
                float f[4] = {2.0f * w1 * Vt * (Tu - T12), w2 * Vt * (T12 - T23),
                              w3 * Vt * (T23 - T34), w4 * Vt * (T34 - T4)};
                float hh = 0.5f * h;
                float b[4] = {-(k1 - v1 - hh * (fo[0] + f[0])), -(k2 - v2 - hh * (fo[1] + f[1])),
                              -(k3 - v3 - hh * (fo[2] + f[2])), -(k4 - v4 - hh * (fo[3] + f[3]))};
                float A[4][4] = {
                    {1.0f + hh * 2.0f * w1 * S12, -hh * 2.0f * w1 * S12, 0.0f, -hh * 2.0f * w1 * Su * dUdY4},
                    {-hh * w2 * S12, 1.0f + hh * w2 * (S12 + S23), -hh * w2 * S23, 0.0f},
                    {0.0f, -hh * w3 * S23, 1.0f + hh * w3 * (S23 + S34), -hh * w3 * S34},
                    {0.0f, 0.0f, -hh * w4 * S34, 1.0f + hh * w4 * (S34 + S4)}};
                float delta[4];
                solveDense4(A, b, delta);
                k1 += delta[0]; k2 += delta[1]; k3 += delta[2]; k4 += delta[3];
            }
        } else {
            float T1o = std::tanh((u0 - v1) * VtInv);
            float T2o = std::tanh((v1 - v2) * VtInv);
            float T3o = std::tanh((v2 - v3) * VtInv);
            float T4o = std::tanh((v3 - v4) * VtInv);
            float f1o = w1 * Vt * (T1o - T2o);
            float f2o = w2 * Vt * (T2o - T3o);
            float f3o = w3 * Vt * (T3o - T4o);
            float f4o = 2.0f * w4 * Vt * T4o;

            k1 = v1 + h * f1o; k2 = v2 + h * f2o;
            k3 = v3 + h * f3o; k4 = v4 + h * f4o;

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

        float stageOut = fLadderV4_ / ladderInputScale_;
        stageOut = outputCoupling_.lowpass(stageOut, outCouplingAlpha);
        out += stageOut / static_cast<float>(kOS);
    }

    // Further coupling-network poles, outside the resonance feedback loop
    // (see the OnePoleAllpass/NotchFilter comments in Filter.hpp). Host-rate,
    // not oversampled -- their corners are tens of Hz, far below Nyquist/2
    // even at 44.1 kHz.
    float postHpAlpha = 1.0f - std::exp(-2.0f * 3.14159265358979323846f
                                         * postFilterHpHz_ / static_cast<float>(sampleRate_));
    out = postFilterHp_.highpass(out, postHpAlpha);
    out = notch_.process(out, notchFreqHz_, notchBandwidthHz_, sampleRate_);
    float tanAp = std::tan(3.14159265358979323846 * allpassFreqHz_ / sampleRate_);
    float apCoeff = (tanAp - 1.0f) / (tanAp + 1.0f);
    out = allpass_.process(out, apCoeff);

    // No resonance-dependent output gain here -- see the comment on
    // setResonanceSkew in Filter.hpp. Whatever level
    // the resonant peak reaches is whatever kFb (above) actually produced.
    return out;
}

} // namespace acidus
