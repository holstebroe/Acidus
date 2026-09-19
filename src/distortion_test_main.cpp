#include "core/Distortion.hpp"
#include "core/MathConstants.hpp"
#include <cmath>
#include <cstdio>
#include <vector>

using namespace acidus;

namespace {

// RMS of a Distortion instance driving a fixed-frequency sine through `drive`,
// after letting its internal state settle.
double rmsAtDrive(double sr, float freqHz, float amplitude, float drive) {
    Distortion dist;
    dist.setSampleRate(sr);

    int totalSamples = static_cast<int>(sr); // 1 second
    int settleSamples = totalSamples / 4;
    double sumSq = 0.0;
    int counted = 0;

    for (int i = 0; i < totalSamples; ++i) {
        float in = amplitude * std::sin(2.0 * kPI_F * freqHz * i / sr);
        float out = dist.processSample(in, drive);
        if (i >= settleSamples) {
            sumSq += double(out) * out;
            counted++;
        }
    }
    return std::sqrt(sumSq / counted);
}

} // namespace

int main() {
    int failures = 0;
    double sr = 48000.0;

    // 1. drive = 0 must be an exact, sample-accurate bypass.
    {
        Distortion dist;
        dist.setSampleRate(sr);
        bool bypassOk = true;
        for (int i = 0; i < 2000; ++i) {
            float in = 0.7f * std::sin(2.0 * kPI_F * 220.0 * i / sr) + 0.2f;
            float out = dist.processSample(in, 0.0f);
            if (out != in) {
                bypassOk = false;
                break;
            }
        }
        if (!bypassOk) {
            std::printf("FAIL: drive=0 did not bypass sample-exactly\n");
            failures++;
        } else {
            std::printf("PASS: drive=0 is an exact bypass\n");
        }
    }

    // 2. No NaN/Inf anywhere across the full drive range at a hot input level.
    {
        bool finiteOk = true;
        Distortion dist;
        dist.setSampleRate(sr);
        for (int step = 0; step <= 20 && finiteOk; ++step) {
            float drive = step / 20.0f;
            for (int i = 0; i < static_cast<int>(sr) / 4; ++i) {
                float in = 1.2f * std::sin(2.0 * kPI_F * 110.0 * i / sr);
                float out = dist.processSample(in, drive);
                if (!std::isfinite(out)) {
                    finiteOk = false;
                    std::printf("FAIL: non-finite output at drive=%.2f, sample %d\n", drive, i);
                    break;
                }
            }
        }
        if (finiteOk) {
            std::printf("PASS: finite output across the full drive sweep\n");
        } else {
            failures++;
        }
    }

    // 3. Re-engaging after bypass doesn't carry over stale clipper state.
    {
        Distortion dist;
        dist.setSampleRate(sr);
        for (int i = 0; i < 1000; ++i) {
            dist.processSample(1.0f * std::sin(2.0 * kPI_F * 300.0 * i / sr), 1.0f);
        }
        for (int i = 0; i < 100; ++i) {
            dist.processSample(0.0f, 0.0f); // bypass for a while
        }
        float firstReengaged = dist.processSample(0.01f, 0.5f);
        if (!std::isfinite(firstReengaged) || std::fabs(firstReengaged) > 1.0f) {
            std::printf("FAIL: re-engaging after bypass produced an unreasonable sample: %f\n", firstReengaged);
            failures++;
        } else {
            std::printf("PASS: re-engaging after bypass starts clean (%f)\n", firstReengaged);
        }
    }

    // 4. More drive should mean more level/compression, not less, at a
    // moderate playing level (monotonic RMS growth as gain climbs).
    {
        float testAmp = 0.4f;
        double rmsLow = rmsAtDrive(sr, 220.0f, testAmp, 0.05f);
        double rmsMid = rmsAtDrive(sr, 220.0f, testAmp, 0.5f);
        double rmsHigh = rmsAtDrive(sr, 220.0f, testAmp, 1.0f);
        std::printf("RMS @ drive 0.05/0.5/1.0: %.4f / %.4f / %.4f\n", rmsLow, rmsMid, rmsHigh);
        if (!(rmsLow < rmsMid && rmsMid <= rmsHigh * 1.05)) {
            std::printf("FAIL: RMS did not increase monotonically with drive\n");
            failures++;
        } else {
            std::printf("PASS: RMS increases with drive\n");
        }
    }

    if (failures > 0) {
        std::printf("\n%d distortion test(s) failed.\n", failures);
        return 1;
    }
    std::printf("\nAll Acidus Distortion tests passed.\n");
    return 0;
}
