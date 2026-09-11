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

float Filter::processOversampledSample(float input, float cutoffHz, float resonance) {
    float totalCutoffHz = std::min(std::max(cutoffHz, 20.0f), 18000.0f);

    // Resonance Bass Drop: Dynamic HPF in feedback loop scaling between 150 Hz and 250 Hz as Resonance increases
    float resNorm = std::min(std::max(resonance, 0.0f), 1.0f);
    float hpfCutoff = 150.0f + 100.0f * resNorm;
    hpfFeedback_.setCutoff(hpfCutoff);

    // Non-linear feedback gain scaling (does not self oscillate to clean sine whistle)
    // Max resonance gain is capped around 3.5 - 3.8 so that feedback saturates passband amplitude
    float resGain = resNorm * 3.6f;

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

    // Feedback path with non-linear saturation Feedback(x) = tanh(x * Resonance_Gain)
    float hpFb = hpfFeedback_.process(0.0f);
    float satFb = std::tanh(hpFb * resGain);
    float x1 = input - satFb;

    // Fixed point iteration loop to resolve non-linear ZDF feedback
    for (int iter = 0; iter < 3; ++iter) {
        // Restore state prior to trial processing
        stage1_.setState(savedS1);
        stage2_.setState(savedS2);
        stage3_.setState(savedS3);
        stage4_.setState(savedS4);
        hpfFeedback_.setState(savedHpfState);

        float y1 = stage1_.process(x1, g1);
        float y2 = stage2_.process(y1, g2);
        float y3 = stage3_.process(y2, g3);
        float y4 = stage4_.process(y3, g4);

        hpFb = hpfFeedback_.process(y4);
        satFb = std::tanh(hpFb * resGain);
        x1 = input - satFb;
    }

    // Final state restoration before true state update step
    stage1_.setState(savedS1);
    stage2_.setState(savedS2);
    stage3_.setState(savedS3);
    stage4_.setState(savedS4);
    hpfFeedback_.setState(savedHpfState);

    // Final forward pass updating capacitor memory
    float y1 = stage1_.process(x1, g1);
    float y2 = stage2_.process(y1, g2);
    float y3 = stage3_.process(y2, g3);
    float y4 = stage4_.process(y3, g4);

    hpfFeedback_.process(y4);

    return y4;
}

float Filter::processSample(float input, float cutoffHz, float resonance) {
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
        filterOut[k] = processOversampledSample(oversampledSamples[k], cutoffHz, resonance);
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
