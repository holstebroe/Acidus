// Standalone offline optimizer: fits Filter.hpp's free calibration constants
// (capScale1..4, feedbackGainCeiling, ladderInputScale) against a small set
// of target (frequency, relative-dB) landmark points read off a reference
// hardware spectrum, using Nelder-Mead simplex search directly against the
// real, nonlinear Filter class (not a linearized model) -- so the fit
// accounts for the same saturation/instability effects a human ear would
// run into while tuning these by hand.
//
// Usage:
//   acidus_filter_resonance_fit [targets.txt] [cutoffHz] [resonance]
//
// targets.txt format: one "freq_hz target_db" pair per line (# comments
// allowed), dB expressed relative to the modeled/target level at the first
// listed frequency (i.e. the first row should be your fundamental/reference
// point, conventionally 0 dB). If no file is given, a small placeholder
// target set is used -- see kDefaultTargets below -- built from landmark
// values reported by ear/eye against a reference hardware track in this
// project's session log (TB303_PARAMETER_CONFIDENCE.md, 2026-09-20 entries).
// Replace it with an FFT-derived target from an actual hardware recording
// for a real fit; the placeholder is only enough to get the search moving
// in roughly the right direction.
//
// Output: the best-found parameter set and the CLAP calibration values to
// plug into Experimental/Filter (Filter Ladder Pole Scale 1..4, Filter
// Feedback Gain, Filter Ladder Input Drive).

#include "core/Filter.hpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace acidus;

struct TargetPoint {
    double freqHz;
    double targetDb; // relative to the first target point
};

// Rough placeholder target, from this session's reported hardware readings
// at cutoff~2500Hz, Resonance=1.0: fundamental as reference, a ~1.7kHz
// plateau at -16dB, and the resonant peak (~3000-3250Hz) at -2.5dB.
static std::vector<TargetPoint> kDefaultTargets = {
    {110.0, 0.0},
    {1700.0, -16.0},
    {3100.0, -2.5},
};

namespace Params { constexpr int kDim = 6; } // [capScale1..4, feedbackGainCeiling, ladderInputScale]

static void applyParams(Filter& f, const double* x) {
    f.setCapScale1((float)x[0]);
    f.setCapScale2((float)x[1]);
    f.setCapScale3((float)x[2]);
    f.setCapScale4((float)x[3]);
    f.setFeedbackGainCeiling((float)x[4]);
    f.setLadderInputScale((float)x[5]);
}

static bool isStable(const double* x, float cutoffHz, float resonance, double totalSec = 1.2) {
    Filter filter;
    double sr = 44100.0;
    filter.setSampleRate(sr);
    applyParams(filter, x);
    int burstLen = 200;
    int totalLen = (int)(totalSec * sr) + burstLen;
    double earlySumSq = 0, lateSumSq = 0;
    int earlyStart = burstLen + 100, earlyLen = 1500;
    int lateStart = totalLen - 1500, lateLen = 1500;
    for (int i = 0; i < totalLen; ++i) {
        float in = (i < burstLen) ? 0.5f * std::sin(2.0 * M_PI * 65.0 * i / sr) : 0.0f;
        float out = filter.processSample(in, cutoffHz, resonance);
        if (!std::isfinite(out)) return false;
        if (i >= earlyStart && i < earlyStart + earlyLen) earlySumSq += double(out) * out;
        if (i >= lateStart && i < lateStart + lateLen) lateSumSq += double(out) * out;
    }
    double rmsEarly = std::sqrt(earlySumSq / earlyLen);
    double rmsLate = std::sqrt(lateSumSq / lateLen);
    return rmsLate <= rmsEarly * 0.1;
}

static double steadyPeak(const double* x, float cutoffHz, float resonance, double freqHz, double settleSec) {
    Filter filter;
    double sr = 44100.0;
    filter.setSampleRate(sr);
    applyParams(filter, x);
    int settleLen = (int)(settleSec * sr);
    int measureLen = (int)(0.08 * sr);
    double peak = 0;
    for (int i = 0; i < settleLen + measureLen; ++i) {
        float in = 1.0f * std::sin(2.0 * M_PI * freqHz * i / sr); // full-scale probe, matches real oscillator level
        float out = filter.processSample(in, cutoffHz, resonance);
        if (!std::isfinite(out)) return -1.0;
        if (i >= settleLen) peak = std::max(peak, (double)std::abs(out));
    }
    return peak;
}

static double objective(const double* x, float cutoffHz, float resonance,
                         const std::vector<TargetPoint>& targets) {
    // Bounds (soft, via penalty) -- keep the search in physically sane territory.
    double penalty = 0;
    auto clampPenalty = [&](double v, double lo, double hi) {
        if (v < lo) penalty += (lo - v) * (lo - v) * 1000.0;
        if (v > hi) penalty += (v - hi) * (v - hi) * 1000.0;
    };
    clampPenalty(x[0], 0.15, 5.0);
    clampPenalty(x[1], 0.15, 5.0);
    clampPenalty(x[2], 0.15, 5.0);
    clampPenalty(x[3], 0.15, 5.0);
    clampPenalty(x[4], 1.0, 40.0);
    clampPenalty(x[5], 0.005, 0.5);

    if (!isStable(x, cutoffHz, resonance)) {
        return 1.0e6 + penalty; // heavily reject self-oscillating configurations
    }

    // Fast settle for the search itself; can under-estimate very-high-Q peaks
    // slightly, but that's an acceptable tradeoff for search speed -- the
    // final reported result is re-verified with a much longer settle time.
    double refPeak = steadyPeak(x, cutoffHz, resonance, targets[0].freqHz, 0.35);
    if (refPeak <= 0) return 1.0e6 + penalty;

    double sumSq = 0;
    for (size_t i = 0; i < targets.size(); ++i) {
        double p = (i == 0) ? refPeak : steadyPeak(x, cutoffHz, resonance, targets[i].freqHz, 0.35);
        if (p <= 0) return 1.0e6 + penalty;
        double modeledDb = 20.0 * std::log10(p / refPeak);
        double err = modeledDb - targets[i].targetDb;
        sumSq += err * err;
    }
    return sumSq + penalty;
}

// Self-contained Nelder-Mead simplex minimizer.
static void nelderMead(double x0[Params::kDim], float cutoffHz, float resonance,
                        const std::vector<TargetPoint>& targets, int maxIters) {
    const int n = Params::kDim;
    const int m = n + 1;
    double simplex[m][Params::kDim];
    double fval[m];

    for (int i = 0; i < n; ++i) simplex[0][i] = x0[i];
    double step[Params::kDim] = {0.3, 0.3, 0.3, 0.3, 1.0, 0.01};
    for (int k = 1; k < m; ++k) {
        for (int i = 0; i < n; ++i) simplex[k][i] = x0[i];
        simplex[k][k - 1] += step[k - 1];
    }
    for (int k = 0; k < m; ++k) fval[k] = objective(simplex[k], cutoffHz, resonance, targets);

    for (int iter = 0; iter < maxIters; ++iter) {
        // sort by fval
        for (int a = 0; a < m; ++a)
            for (int b = a + 1; b < m; ++b)
                if (fval[b] < fval[a]) {
                    std::swap(fval[a], fval[b]);
                    for (int i = 0; i < n; ++i) std::swap(simplex[a][i], simplex[b][i]);
                }

        if (iter % 5 == 0) {
            printf("iter %4d  best=%.4f  capScale=(%.3f,%.3f,%.3f,%.3f)  feedbackCeiling=%.3f  inputScale=%.4f\n",
                   iter, fval[0], simplex[0][0], simplex[0][1], simplex[0][2], simplex[0][3],
                   simplex[0][4], simplex[0][5]);
            fflush(stdout);
        }
        if (fval[m - 1] - fval[0] < 1e-5) break;

        double centroid[Params::kDim] = {0};
        for (int k = 0; k < m - 1; ++k)
            for (int i = 0; i < n; ++i) centroid[i] += simplex[k][i] / (m - 1);

        double reflected[Params::kDim];
        for (int i = 0; i < n; ++i) reflected[i] = centroid[i] + 1.0 * (centroid[i] - simplex[m - 1][i]);
        double fr = objective(reflected, cutoffHz, resonance, targets);

        if (fr < fval[0]) {
            double expanded[Params::kDim];
            for (int i = 0; i < n; ++i) expanded[i] = centroid[i] + 2.0 * (reflected[i] - centroid[i]);
            double fe = objective(expanded, cutoffHz, resonance, targets);
            if (fe < fr) { for (int i=0;i<n;++i) simplex[m-1][i]=expanded[i]; fval[m-1]=fe; }
            else { for (int i=0;i<n;++i) simplex[m-1][i]=reflected[i]; fval[m-1]=fr; }
        } else if (fr < fval[m - 2]) {
            for (int i=0;i<n;++i) simplex[m-1][i]=reflected[i];
            fval[m-1]=fr;
        } else {
            double contracted[Params::kDim];
            for (int i = 0; i < n; ++i) contracted[i] = centroid[i] + 0.5 * (simplex[m - 1][i] - centroid[i]);
            double fc = objective(contracted, cutoffHz, resonance, targets);
            if (fc < fval[m - 1]) {
                for (int i=0;i<n;++i) simplex[m-1][i]=contracted[i];
                fval[m-1]=fc;
            } else {
                for (int k = 1; k < m; ++k) {
                    for (int i = 0; i < n; ++i) simplex[k][i] = simplex[0][i] + 0.5 * (simplex[k][i] - simplex[0][i]);
                    fval[k] = objective(simplex[k], cutoffHz, resonance, targets);
                }
            }
        }
    }

    for (int i = 0; i < n; ++i) x0[i] = simplex[0][i];
    printf("\nFinal best cost: %.5f\n", fval[0]);
    fflush(stdout);
}

int main(int argc, char** argv) {
    std::vector<TargetPoint> targets = kDefaultTargets;
    float cutoffHz = 2500.0f;
    float resonance = 1.0f;

    if (argc > 1 && std::strlen(argv[1]) > 0) {
        std::ifstream f(argv[1]);
        if (!f) { fprintf(stderr, "Could not open targets file %s\n", argv[1]); return 1; }
        targets.clear();
        std::string line;
        while (std::getline(f, line)) {
            if (line.empty() || line[0] == '#') continue;
            std::istringstream ss(line);
            TargetPoint p;
            if (ss >> p.freqHz >> p.targetDb) targets.push_back(p);
        }
        if (targets.empty()) { fprintf(stderr, "No target points parsed from %s\n", argv[1]); return 1; }
    }
    if (argc > 2) cutoffHz = (float)std::atof(argv[2]);
    if (argc > 3) resonance = (float)std::atof(argv[3]);

    printf("Fitting against %zu target points at cutoff=%.0fHz, resonance=%.2f:\n", targets.size(), cutoffHz, resonance);
    for (auto& t : targets) printf("  %.1f Hz -> %.2f dB\n", t.freqHz, t.targetDb);
    printf("\n");
    fflush(stdout);

    // Start from the project's current defaults.
    double x[Params::kDim] = {1.0, 1.0, 1.0, 1.0, 15.3, 0.05};
    int maxIters = 150;
    if (argc > 4) maxIters = std::atoi(argv[4]);
    nelderMead(x, cutoffHz, resonance, targets, maxIters);

    printf("\n=== Result ===\n");
    printf("Filter Ladder Pole Scale 1: %.3f\n", x[0]);
    printf("Filter Ladder Pole Scale 2: %.3f\n", x[1]);
    printf("Filter Ladder Pole Scale 3: %.3f\n", x[2]);
    printf("Filter Ladder Pole Scale 4: %.3f\n", x[3]);
    printf("Filter Feedback Gain:       %.3f\n", x[4]);
    printf("Filter Ladder Input Drive:  %.4f\n", x[5]);

    bool stableFinal = isStable(x, cutoffHz, resonance, 2.5);
    printf("\nStable (2.5s decay check): %s\n", stableFinal ? "yes" : "NO -- do not use this result");

    if (stableFinal) {
        printf("\nHigh-precision re-check (2.0s settle, matches the search's own reference convention):\n");
        double refPeak = steadyPeak(x, cutoffHz, resonance, targets[0].freqHz, 2.0);
        for (auto& t : targets) {
            double p = steadyPeak(x, cutoffHz, resonance, t.freqHz, 2.0);
            double db = (p > 0 && refPeak > 0) ? 20.0 * std::log10(p / refPeak) : -999;
            printf("  %7.1f Hz  target=%6.2f dB  modeled=%6.2f dB  (err %+.2f dB)\n",
                   t.freqHz, t.targetDb, db, db - t.targetDb);
        }
    }
    return 0;
}
