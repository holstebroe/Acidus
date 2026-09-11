#include "Filter.hpp"
#include <cmath>
#include <algorithm>

namespace syrebas {

static const float FIR_COEFFS[16] = {
    -0.0031f, 0.0f, 0.0156f, 0.0f, -0.0528f, 0.0f, 0.3134f, 0.5f,
     0.3134f, 0.0f, -0.0528f, 0.0f, 0.0156f, 0.0f, -0.0031f, 0.0f
};

Filter::Filter() {
    setSampleRate(44100.0);
}

void Filter::setSampleRate(double sampleRate) {
    sampleRate_ = sampleRate;
    oversampledRate_ = sampleRate_ * 4.0;
    hpfFeedback_.setSampleRate(oversampledRate_);
    reset();
}

void Filter::reset() {
    stage1_.reset();
    stage2_.reset();
    stage3_.reset();
    stage4_.reset();
    hpfFeedback_.reset();
    upBuffer1_.fill(0.0f);
    upBuffer2_.fill(0.0f);
    downBuffer1_.fill(0.0f);
    downBuffer2_.fill(0.0f);
    upIdx1_ = upIdx2_ = downIdx1_ = downIdx2_ = 0;
}

float Filter::processOversampledSample(float input, float cutoffHz, float resonance, float envModVal, float accentVal) {
    // 303 VCF cutoff modulation range
    float totalCutoffHz = cutoffHz + envModVal * 4800.0f + accentVal * 5500.0f;
    totalCutoffHz = (std::min)((std::max)(totalCutoffHz, 30.0f), 18000.0f);

    // 4-stage ladder feedback resonance threshold for self-oscillation/squelch is K >= 4.0
    // Resonance knob 0.0 -> 1.0 scales K from 0.0 to 4.95 with non-linear curve
    float resGainCorr = 1.0f - (std::min)((std::max)((totalCutoffHz - 5000.0f) / 15000.0f, 0.0f), 0.35f);
    float kRes = (resonance * resonance * 4.95f) * resGainCorr;

    float wc = 2.0f * 3.14159265358979323846f * totalCutoffHz;
    float gBase = std::tan(wc / (2.0f * static_cast<float>(oversampledRate_)));

    float g1 = gBase * capScale1_;
    float g2 = gBase * capScale2_;
    float g3 = gBase * capScale3_;
    float g4 = gBase * capScale4_;

    // Save initial state memory prior to ZDF iteration loop
    float savedS1 = stage1_.getState();
    float savedS2 = stage2_.getState();
    float savedS3 = stage3_.getState();
    float savedS4 = stage4_.getState();
    HPFFeedback::State savedHpfState = hpfFeedback_.getState();

    // Initial feedback estimate using previous state memory
    float hpFb = hpfFeedback_.process(0.0f);
    float x1 = input - kRes * hpFb;

    // Fixed point iteration loop to resolve non-linear ZDF feedback
    for (int iter = 0; iter < 3; ++iter) {
        // Restore state prior to trial processing
        stage1_.setState(savedS1);
        stage2_.setState(savedS2);
        stage3_.setState(savedS3);
        stage4_.setState(savedS4);
        hpfFeedback_.setState(savedHpfState);

        float y0 = std::tanh(x1);
        float y1 = stage1_.process(y0, g1);
        float y2 = stage2_.process(std::tanh(y1), g2);
        float y3 = stage3_.process(std::tanh(y2), g3);
        float y4 = stage4_.process(std::tanh(y3), g4);

        hpFb = hpfFeedback_.process(std::tanh(y4));
        x1 = input - kRes * hpFb;
    }

    // Final state restoration before true state update step
    stage1_.setState(savedS1);
    stage2_.setState(savedS2);
    stage3_.setState(savedS3);
    stage4_.setState(savedS4);
    hpfFeedback_.setState(savedHpfState);

    // Final forward pass updating capacitor memory
    float y0 = std::tanh(x1);
    float y1 = stage1_.process(y0, g1);
    float y2 = stage2_.process(std::tanh(y1), g2);
    float y3 = stage3_.process(std::tanh(y2), g3);
    float y4 = stage4_.process(std::tanh(y3), g4);

    hpfFeedback_.process(std::tanh(y4));

    return y4;
}

float Filter::processSample(float input, float cutoffHz, float resonance, float envModVal, float accentVal) {
    float oversampledSamples[4];

    for (int i = 0; i < 2; ++i) {
        float inVal = (i == 0) ? input * 2.0f : 0.0f;
        upBuffer1_[upIdx1_] = inVal;

        float stage1Out = 0.0f;
        for (int tap = 0; tap < FIR_TAPS; ++tap) {
            int idx = (upIdx1_ - tap + FIR_TAPS) % FIR_TAPS;
            stage1Out += upBuffer1_[idx] * FIR_COEFFS[tap];
        }
        upIdx1_ = (upIdx1_ + 1) % FIR_TAPS;

        for (int j = 0; j < 2; ++j) {
            float inVal2 = (j == 0) ? stage1Out * 2.0f : 0.0f;
            upBuffer2_[upIdx2_] = inVal2;

            float stage2Out = 0.0f;
            for (int tap = 0; tap < FIR_TAPS; ++tap) {
                int idx = (upIdx2_ - tap + FIR_TAPS) % FIR_TAPS;
                stage2Out += upBuffer2_[idx] * FIR_COEFFS[tap];
            }
            upIdx2_ = (upIdx2_ + 1) % FIR_TAPS;

            oversampledSamples[i * 2 + j] = stage2Out;
        }
    }

    float filterOut[4];
    for (int k = 0; k < 4; ++k) {
        filterOut[k] = processOversampledSample(oversampledSamples[k], cutoffHz, resonance, envModVal, accentVal);
    }

    float downStage1[2];
    for (int k = 0; k < 2; ++k) {
        downBuffer1_[downIdx1_] = filterOut[k * 2];
        downIdx1_ = (downIdx1_ + 1) % FIR_TAPS;
        downBuffer1_[downIdx1_] = filterOut[k * 2 + 1];
        downIdx1_ = (downIdx1_ + 1) % FIR_TAPS;

        float outVal = 0.0f;
        for (int tap = 0; tap < FIR_TAPS; ++tap) {
            int idx = (downIdx1_ - 1 - tap + FIR_TAPS) % FIR_TAPS;
            outVal += downBuffer1_[idx] * FIR_COEFFS[tap];
        }
        downStage1[k] = outVal;
    }

    downBuffer2_[downIdx2_] = downStage1[0];
    downIdx2_ = (downIdx2_ + 1) % FIR_TAPS;
    downBuffer2_[downIdx2_] = downStage1[1];
    downIdx2_ = (downIdx2_ + 1) % FIR_TAPS;

    float finalOut = 0.0f;
    for (int tap = 0; tap < FIR_TAPS; ++tap) {
        int idx = (downIdx2_ - 1 - tap + FIR_TAPS) % FIR_TAPS;
        finalOut += downBuffer2_[idx] * FIR_COEFFS[tap];
    }

    return finalOut;
}

} // namespace syrebas
