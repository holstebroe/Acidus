#include "Filter.hpp"
#include <cmath>
#include <algorithm>

namespace syrebas {

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
    fHpFbStateX2_ = fHpFbStateY2_ = 0.0f;
    prevFaithfulInput_ = 0.0f;
    inputCoupling_.reset();
    outputCoupling_.reset();
}

float Filter::processAccurateSample(float input, float cutoffHz, float resonance) {
    // 4x oversampling step for accurate coupled diode ladder
    float dt = 1.0f / static_cast<float>(oversampledRateAccurate_);
    float totalCutoffHz = std::min(std::max(cutoffHz, 20.0f), 18000.0f);

    float wc = 2.0f * 3.14159265358979323846f * totalCutoffHz;

    // High pass in feedback path: cutoff dynamically scales between 150 Hz and 250 Hz.
    // BEST GUESS / UNSOURCED - the 2026 research pass (TB303_RESEARCH_COMPENDIUM.md
    // Section 6) found the real composite coupling-pole corner is closer to
    // 5-15 Hz, not 150-250 Hz, and that it should be resonant rather than a
    // plain attenuating HPF. Faithful mode (processFaithfulSample) implements
    // that correction; Accurate mode intentionally keeps this older,
    // differently-calibrated approximation unchanged, since it's the mode
    // users already know and like the sound of.
    float resNorm = std::min(std::max(resonance, 0.0f), 1.0f);
    float hpfCutoff = 150.0f + 100.0f * resNorm;
    float hpfAlpha = 1.0f / (1.0f + 2.0f * 3.14159265358979323846f * hpfCutoff * dt);

    // Feedback loop gain ceiling. BEST GUESS / calibration knob, tuned so the
    // ladder approaches but does not cleanly self-oscillate (see the matching
    // comment in processFaithfulSample() for the sourcing on that principle).
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
    // 8x oversampling. CONFIRMED as sound general practice for a nonlinear
    // ladder/tanh junction (not 303-specific); "4x minimum, prefer 8x+" is
    // the guidance in TB303_EMULATION_REFERENCE.md Section 55.
    constexpr int kOS = 8;
    float dt = 1.0f / static_cast<float>(oversampledRateFaithful_);
    // BEST GUESS: no documented VCF frequency range; purely a numerical clamp.
    // Note SynthEngine.cpp clamps its own output to 20 Hz-15 kHz before ever
    // calling this, so 18 kHz here is a secondary safety net, not the
    // effective ceiling.
    float totalCutoffHz = std::min(std::max(cutoffHz, 20.0f), 18000.0f);
    float wc = 2.0f * 3.14159265358979323846f * totalCutoffHz;

    float resNorm = std::min(std::max(resonance, 0.0f), 1.0f);

    // --- Resonance-loop coupling-pole network -----------------------------
    // Confidence: ESTIMATE (mechanism/order-of-magnitude CONFIRMED by
    // research, exact corner/pole-count NOT sourced -- see
    // TB303_RESEARCH_COMPENDIUM.md Section 6 and TB303_PARAMETER_CONFIDENCE.md).
    //
    // The real VCF has "approximately six further high-pass/coupling poles"
    // surrounding the core 4-pole ladder (Stinchcombe's analysis, via
    // secondary summaries), whose *combined* effect behaves like an HPF in
    // series with the main low-pass, sitting inside the same resonance
    // feedback loop -- and becomes resonant there, boosting rather than only
    // attenuating sub-100 Hz content as Resonance increases. Secondary
    // sources disagree on the exact corner: ~8 Hz (Learning Modular's
    // summary of Stinchcombe) vs ~10 Hz (Electronic Music Wiki); no source
    // gives a pole count or topology for the surrounding network.
    //
    // This was previously modeled as a single one-pole HPF whose corner swept
    // 150->250 Hz with Resonance -- a figure with no traceable source and
    // roughly 15-25x too high a frequency. At typical TB-303 bassline cutoffs
    // (150-800 Hz), that wrongly stripped most of the ladder's own output out
    // of the feedback path before it could resonate, damping exactly the
    // "squelchy acid bass" cutoff range the instrument is known for.
    //
    // Modeled here as 2 cascaded one-pole highpass stages (12 dB/oct) at a
    // single fixed corner, still inside the same feedback loop as before --
    // a deliberately simplified stand-in for an unknown ~6-pole network, not
    // a literal circuit derivation. The resonant "boost near the pole" effect
    // emerges from the feedback loop dynamics themselves (same mechanism as
    // the main ladder's resonance peak at cutoff) once the corner sits at the
    // right frequency, without needing a separate boost stage.
    //
    // Plausible range for the corner: 5-15 Hz. This is the single
    // highest-leverage "sounds more like a real 303" knob identified by
    // TB303_PARAMETER_CONFIDENCE.md -- try values across this range by ear.
    // Pole count (2 here) is itself a BEST GUESS trade-off between "more than
    // one pole, since the real network is clearly more complex than a single
    // RC" and "don't pretend to model 6 individually-placed, unsourced poles."
    // Increasing to 3-4 cascaded stages is a cheap (mono synth, negligible
    // CPU either way) experiment worth trying if 2 poles doesn't sound sharp
    // enough at the transition into the boosted region.
    const float resCouplingAlpha = 1.0f / (1.0f + 2.0f * 3.14159265358979323846f * resCouplingHz_ * dt);

    // Feedback loop gain. BEST GUESS / calibration knob -- not a circuit
    // value. Chosen so the ladder approaches but does not cleanly
    // self-oscillate (CONFIRMED principle: Wikipedia's spec sheet states the
    // stock filter is non-self-oscillating). Because the coupling-pole fix
    // above lets much more of the ladder's own bass-register output back
    // into the loop than the old 150-250 Hz version did, this constant is
    // more likely to need re-tuning than before -- if self-oscillation
    // happens too easily at moderate cutoff/resonance settings, try lower
    // values first. Plausible range: 20-40.
    float kFb = resNorm * 36.0f;

    // BJT thermal-voltage-referenced tanh steepness. The 26 mV base is
    // CONFIRMED textbook physics for a bipolar junction; the x2 "effective"
    // scale used here is a tuning choice for the ladder's inter-stage
    // nonlinearity steepness, not a documented circuit parameter (ESTIMATE).
    const float Vt = 0.052f;
    const float VtInv = 19.23f;

    // Input coupling cap (Section 10/47): blocks DC and trims sub-bass ahead
    // of the ladder. ESTIMATE: a real input coupling network is CONFIRMED to
    // exist (the 303 does not have flat sub-bass response), but no specific
    // corner is sourced. Plausible range: 10-30 Hz.
    float inCouplingAlpha = 1.0f - std::exp(-2.0f * 3.14159265358979323846f * 20.0f * dt);
    // Output/buffer bandwidth limit representing extra high-frequency
    // coupling poles. ESTIMATE, same status as above. Plausible range:
    // 10-25 kHz.
    float outCouplingAlpha = 1.0f - std::exp(-2.0f * 3.14159265358979323846f * 20000.0f * dt);

    float out = 0.0f;
    float prevIn = prevFaithfulInput_;
    prevFaithfulInput_ = input;

    constexpr int kNewtonIters = 3;
    const float w1 = wc * capScale1_, w2 = wc * capScale2_;
    const float w3 = wc * capScale3_, w4 = wc * capScale4_;

    for (int os = 0; os < kOS; ++os) {
        float alphaOS = static_cast<float>(os + 1) / static_cast<float>(kOS);
        float currIn = prevIn + alphaOS * (input - prevIn);

        // BEST GUESS: internal scale representing an assumed ~50 mV RMS VCO
        // output level ahead of the ladder; not sourced to a measured value.
        // Sets how hard the ladder saturates for a given oscillator output.
        float inSample = currIn * 0.05f;
        inSample = inputCoupling_.highpass(inSample, inCouplingAlpha);

        float h = dt;
        float v1 = fLadderV1_, v2 = fLadderV2_, v3 = fLadderV3_, v4 = fLadderV4_;

        // Commit both coupling-pole stages' one-pole state once per
        // oversample step (from the pre-step v4), then reuse that committed
        // state to estimate the feedback voltage at each Newton iterate's
        // predicted v4 without advancing the filter's history multiple times
        // per sample. This "freeze history, linearize within the step"
        // pattern is numerically safe here because the coupling-pole time
        // constant (tau ~= 1/(2*pi*9Hz) ~= 17.7 ms) is enormous relative to
        // the oversampled step (dt ~= 2.8 us at 8x/44.1kHz, dt/tau ~= 1.6e-4)
        // -- so treating it explicitly instead of folding it into the
        // implicit Newton-Raphson solve costs negligible accuracy. Embedding
        // it into the implicit solve would be a substantial restructuring
        // (the tridiagonal Jacobian below would need 2 more rows/columns)
        // for no audible benefit at this timescale separation.
        float hp1_0 = resCouplingAlpha * (fHpFbStateY1_ + v4 - fHpFbStateX1_);
        fHpFbStateX1_ = v4;
        fHpFbStateY1_ = hp1_0;
        float hp2_0 = resCouplingAlpha * (fHpFbStateY2_ + hp1_0 - fHpFbStateX2_);
        fHpFbStateX2_ = hp1_0;
        fHpFbStateY2_ = hp2_0;
        float u0 = inSample - hp2_0 * kFb;

        auto feedbackFor = [&](float v4pred) {
            float hp1 = resCouplingAlpha * (fHpFbStateY1_ + v4pred - fHpFbStateX1_);
            float hp2 = resCouplingAlpha * (fHpFbStateY2_ + hp1 - fHpFbStateX2_);
            return inSample - hp2 * kFb;
        };

        // f(v) at the start of the step (u0, v_old), used as the fixed half of the
        // implicit trapezoidal rule: v_new = v_old + (h/2)*(f(v_old) + f(v_new)).
        float T1o = std::tanh((u0 - v1) * VtInv);
        float T2o = std::tanh((v1 - v2) * VtInv);
        float T3o = std::tanh((v2 - v3) * VtInv);
        float T4o = std::tanh((v3 - v4) * VtInv);
        float f1o = w1 * Vt * (T1o - T2o);
        float f2o = w2 * Vt * (T2o - T3o);
        float f3o = w3 * Vt * (T3o - T4o);
        float f4o = 2.0f * w4 * Vt * T4o;

        // Newton-Raphson on the trapezoidal residual R(k) = k - v_old - (h/2)*(f_old + f(k)),
        // solved via the ladder's tridiagonal Jacobian (each stage only couples to its
        // immediate neighbours) at each iterate. The feedback voltage u is re-derived from
        // each iterate's k4 (a fixed-point update folded into the same loop) rather than
        // included as a Jacobian term, which keeps the system exactly tridiagonal.
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
            // Guard against a pathological Newton step; hold the ladder at its last
            // good state rather than propagate NaN/Inf into the audio output.
            fLadderV1_ = v1;
            fLadderV2_ = v2;
            fLadderV3_ = v3;
            fLadderV4_ = v4;
        }

        float stageOut = fLadderV4_ / 0.05f;
        stageOut = outputCoupling_.lowpass(stageOut, outCouplingAlpha);
        out += stageOut / static_cast<float>(kOS);
    }

    return out;
}

} // namespace syrebas
