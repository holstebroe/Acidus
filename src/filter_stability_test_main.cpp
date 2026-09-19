#include "core/Filter.hpp"
#include <cmath>
#include <cstdio>
#include <initializer_list>

using namespace acidus;

static bool isStable(float cutoffHz, float resonance) {
    Filter filter;
    double sr = 44100.0;
    filter.setSampleRate(sr);

    int burstLen = 200;
    int totalLen = static_cast<int>(sr) + burstLen + 200;
    double earlySumSq = 0, lateSumSq = 0;
    int earlyStart = burstLen + 100, earlyLen = 2000;
    int lateStart = totalLen - 2000, lateLen = 2000;

    for (int i = 0; i < totalLen; ++i) {
        float in = (i < burstLen) ? 0.3f * std::sin(2.0 * M_PI * 65.0 * i / sr) : 0.0f;
        float out = filter.processSample(in, cutoffHz, resonance);
        if (i >= earlyStart && i < earlyStart + earlyLen) earlySumSq += double(out) * out;
        if (i >= lateStart && i < lateStart + lateLen) lateSumSq += double(out) * out;
    }
    double rmsEarly = std::sqrt(earlySumSq / earlyLen);
    double rmsLate = std::sqrt(lateSumSq / lateLen);
    return rmsLate <= rmsEarly * 0.5;
}

int main() {
    float cutoffs[] = {100.0f, 250.0f, 500.0f, 1000.0f, 2000.0f, 4000.0f, 8000.0f, 12000.0f, 15000.0f};
    float resSteps[] = {0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f};

    int failures = 0;
    std::printf("\nAcidus Filter -- highest stable Resonance per cutoff (1.0 = never went unstable in this sweep):\n");
    for (float cutoff : cutoffs) {
        float highestStable = -1.0f;
        float firstUnstable = -1.0f;
        for (float res : resSteps) {
            if (isStable(cutoff, res)) {
                highestStable = res;
            } else if (firstUnstable < 0.0f) {
                firstUnstable = res;
            }
        }
        if (firstUnstable >= 0.0f) {
            std::printf("  cutoff=%6.0f Hz: stable up to Resonance=%.1f, self-oscillates from %.1f up\n",
                        cutoff, highestStable, firstUnstable);
            failures++;
        } else {
            std::printf("  cutoff=%6.0f Hz: stable across the whole 0.0-1.0 Resonance range\n", cutoff);
        }
    }

    if (failures > 0) {
        std::printf("\n%d cutoff settings self-oscillate somewhere in the 0.0-1.0 Resonance range.\n", failures);
    } else {
        std::printf("\nNo self-oscillation found across the full 0.0-1.0 Resonance range at any tested cutoff.\n");
    }
    return 0;
}
