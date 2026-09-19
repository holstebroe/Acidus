// Filter self-oscillation / stability sweep.
//
// The stock TB-303 filter is documented (TB303_RESEARCH_COMPENDIUM.md
// Section 6, citing Wikipedia's spec sheet) as NOT self-oscillating: turning
// Resonance up should make it squelchy/resonant, not lock into an
// undamped/sustained tone independent of the input. This tool feeds each
// filter mode a short burst followed by silence, across a grid of
// cutoff/resonance settings, and reports the highest resonance at which the
// filter still decays to silence (stable) for each tested cutoff -- i.e.
// the self-oscillation threshold as actually implemented, which should stay
// safely above whatever resonance values the front-panel knob can reach in
// normal use.
//
// This was written during the 2026-09 investigation into why a specific
// render sounded nothing like a TB-303 (a strong, fixed ~2.6 kHz tone
// dominating the whole recording, independent of the note being played --
// the classic signature of filter self-oscillation). It's kept as a
// permanent regression check: run it after any change to Filter.cpp's
// feedback gain, coupling-pole, or capacitor-scale constants.
#include "core/Filter.hpp"
#include <cmath>
#include <cstdio>
#include <initializer_list>

using namespace syrebas;

// Feeds a short sine burst then 1s of silence; returns true if the filter's
// output has decayed to silence by the end of that second (stable), false if
// it's still ringing at a level comparable to the burst response (unstable).
static bool isStable(bool faithful, float cutoffHz, float resonance) {
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
        float out = faithful ? filter.processFaithfulSample(in, cutoffHz, resonance)
                              : filter.processAccurateSample(in, cutoffHz, resonance);
        if (i >= earlyStart && i < earlyStart + earlyLen) earlySumSq += double(out) * out;
        if (i >= lateStart && i < lateStart + lateLen) lateSumSq += double(out) * out;
    }
    double rmsEarly = std::sqrt(earlySumSq / earlyLen);
    double rmsLate = std::sqrt(lateSumSq / lateLen);
    // Unstable/self-oscillating: barely decayed at all after ~1s of silence.
    return rmsLate <= rmsEarly * 0.5;
}

int main() {
    float cutoffs[] = {100.0f, 250.0f, 500.0f, 1000.0f, 2000.0f, 4000.0f, 8000.0f, 12000.0f, 15000.0f};
    float resSteps[] = {0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f};

    int failures = 0;
    for (bool faithful : {false, true}) {
        std::printf("\n%s mode -- highest stable Resonance per cutoff (1.0 = never went unstable in this sweep):\n",
                    faithful ? "Faithful" : "Accurate");
        for (float cutoff : cutoffs) {
            float highestStable = -1.0f;
            float firstUnstable = -1.0f;
            for (float res : resSteps) {
                if (isStable(faithful, cutoff, res)) {
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
    }

    if (failures > 0) {
        std::printf("\n%d cutoff/mode combination(s) self-oscillate somewhere in the 0.0-1.0 Resonance range.\n"
                    "This does not necessarily mean the constants are wrong for every setting -- the stock 303's\n"
                    "own resonance range may legitimately approach instability at high Resonance (see\n"
                    "TB303_RESEARCH_COMPENDIUM.md Section 6: 'can bring the filter closer to oscillation') --\n"
                    "but any self-oscillation reachable within the *low-to-moderate* Resonance range used in\n"
                    "ordinary patches is a strong candidate bug, not a hardware-accurate feature.\n", failures);
    } else {
        std::printf("\nNo self-oscillation found across the full 0.0-1.0 Resonance range at any tested cutoff.\n");
    }
    return 0;
}
