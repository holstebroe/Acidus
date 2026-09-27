// Reference-conformance tests: measure the DSP core against the claims in
// docs/TB-303 Reference/TB303_REFERENCE.md that carry a hard source tag
// ([S] service notes, [A] published circuit analysis, [D] derived from those,
// [M] measured on hardware). Deliberately does NOT compare against the
// hardware sample recordings in test/resources -- the knob positions used
// for those are uncertain, and fitting against them is what the calibrator
// does. These tests instead check model structure: frequency response,
// loop stability margin, time constants and control-law shape.
//
// Every test prints what it measured next to the reference target and the
// section of TB303_REFERENCE.md it comes from. Rows tagged INFO report a
// number whose reference value is only an estimate ([E]/[I]) and never fail.
//
// Usage: acidus_reference_test [--fast] [--ladder-topology 0|1]
//   --fast skips the slowest filter sweeps (oscillation-margin bisection).
// Exit code: number of failed (non-INFO) checks.

#include "core/SynthEngine.hpp"
#include "core/Filter.hpp"
#include "core/Oscillator.hpp"
#include "core/Envelope.hpp"

#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <complex>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <functional>
#include <string>
#include <vector>

using namespace acidus;

namespace {

constexpr double kPi = 3.14159265358979323846;

// ---------------------------------------------------------------------------
// Reporting
// ---------------------------------------------------------------------------

struct Row {
    std::string id;
    std::string section;
    std::string name;
    std::string measured;
    std::string target;
    bool pass;
    bool info;
};

std::vector<Row> gRows;

std::string fmt(const char* f, ...) {
    char buf[512];
    va_list ap;
    va_start(ap, f);
    std::vsnprintf(buf, sizeof(buf), f, ap);
    va_end(ap);
    return buf;
}

void check(const std::string& id, const std::string& section, const std::string& name,
           const std::string& measured, const std::string& target, bool pass, bool info = false) {
    gRows.push_back({id, section, name, measured, target, pass, info});
    std::printf("[%s] %-5s %-7s %s\n        measured: %s\n        target:   %s\n",
                info ? "INFO" : (pass ? "PASS" : "FAIL"), id.c_str(), section.c_str(), name.c_str(),
                measured.c_str(), target.c_str());
    std::fflush(stdout);
}

bool inRange(double v, double lo, double hi) { return v >= lo && v <= hi; }

// ---------------------------------------------------------------------------
// Mirrors the per-block parameter push in SynthEngine::processAudio, so the
// filter/oscillator/envelope tests run with exactly the shipped defaults.
// ---------------------------------------------------------------------------

// --ladder-topology N: run every check with that ladder orientation instead
// of the shipped default (Filter.hpp setLadderTopology).
int gLadderTopologyOverride = -1;

void applyFilterParams(Filter& f, const SynthParameters& p) {
    f.setResCouplingHz(p.resCouplingHz);
    f.setFeedbackGainCeiling(p.filterFeedbackGain);
    f.setPostFilterHpHz(p.filterPostHpHz);
    f.setNotchFreqHz(p.filterNotchHz);
    f.setNotchBandwidthHz(p.filterNotchBandwidthHz);
    f.setAllpassFreqHz(p.filterAllpassHz);
    f.setInputCouplingHz(p.filterInputCouplingHz);
    f.setOutputCouplingHz(p.filterOutputCouplingHz);
    f.setCapScale1(p.filterCapScale1);
    f.setCapScale2(p.filterCapScale2);
    f.setCapScale3(p.filterCapScale3);
    f.setCapScale4(p.filterCapScale4);
    f.setLadderInputScale(p.filterLadderInputScale);
    f.setLadderTopology(gLadderTopologyOverride >= 0 ? gLadderTopologyOverride : (p.filterLadderTopology >= 0.5f ? 1 : 0));
    f.setResonanceSkew(p.filterResonanceSkew);
    f.setResonanceLimit(p.filterResonanceLimit);
}

void applyOscParams(Oscillator& o, const SynthParameters& p) {
    o.setTuningCents(p.tuningCents);
    o.setCouplingHz(p.oscCouplingHz);
    o.setSawShaping(p.oscSawLpfHz, p.oscSawShape);
}

void applyEnvParams(Envelope& e, const SynthParameters& p, float decayKnob) {
    e.setFaithfulAccentDecay(true);
    e.setVegDecaySec(p.vegDecaySec);
    e.setVcaGateOffMs(p.vcaGateOffMs);
    e.setVcaGateOffAccentMs(p.vcaGateOffAccentMs);
    e.setAttackTimesMs(p.vcfAttackMs, p.vcaAttackMs);
    e.setDecayRangeSec(p.vcfDecayMinSec, p.vcfDecayMaxSec);
    e.setAccentDecaySec(p.accentDecaySec);
    e.setDecay(decayKnob);
}

// Removes every coupling network around the ladder so only the 4-pole core
// is left (used to compare against Stinchcombe's core polynomial).
void neutralizeCouplings(Filter& f) {
    f.setInputCouplingHz(0.001f);
    f.setPostFilterHpHz(0.001f);
    f.setNotchFreqHz(20000.0f);   // park the notch far above the band...
    f.setNotchBandwidthHz(1.0f);  // ...and make it vanishingly narrow
    f.setOutputCouplingHz(1.0e7f);
}

// ---------------------------------------------------------------------------
// Stinchcombe models (TB303_REFERENCE.md §10.2, §11.1)
// ---------------------------------------------------------------------------

using cd = std::complex<double>;

cd stinchCoreL(cd sn) { // s normalised to wc
    return sn * sn * sn * sn + std::pow(2.0, 11.0 / 4.0) * sn * sn * sn + 10.0 * std::sqrt(2.0) * sn * sn
           + std::pow(2.0, 13.0 / 4.0) * sn + 1.0;
}

double stinchCoreDb(double w_over_wc) {
    return 20.0 * std::log10(1.0 / std::abs(stinchCoreL(cd(0.0, w_over_wc))));
}

// Full 10-pole / 6-zero model, wc = 2*pi*820, k = 0..1.
double stinchFullDb(double fHz, double k) {
    const double wc = 2.0 * kPi * 820.0;
    cd s(0.0, 2.0 * kPi * fHz);
    cd L = stinchCoreL(s / wc);
    cd num = 1.06 * s * s * s * (s + 109.9) * (s + 34.0) * (s + 7.41);
    cd den = L * (s + 97.5) * (s + 38.5) * (s + 4.45) * (s + 578.1) * (s + 20.0) * (s + 7.41)
             + 18.7 * k * s * s * s * s * (s + 46.5) * (s + 4.40);
    return 20.0 * std::log10(std::abs(num / den));
}

// ---------------------------------------------------------------------------
// Filter measurement helpers
// ---------------------------------------------------------------------------

using FilterCfg = std::function<void(Filter&)>;

// Hann-windowed single-bin DFT amplitude of x[start, start+len) at f.
double toneAmplitude(const std::vector<float>& x, size_t start, size_t len, double sr, double f) {
    double re = 0, im = 0, wsum = 0;
    for (size_t n = 0; n < len; ++n) {
        double w = 0.5 - 0.5 * std::cos(2.0 * kPi * (n + 0.5) / len);
        double ph = 2.0 * kPi * f * static_cast<double>(start + n) / sr;
        re += w * x[start + n] * std::cos(ph);
        im += w * x[start + n] * std::sin(ph);
        wsum += w;
    }
    return 2.0 * std::sqrt(re * re + im * im) / wsum;
}

// Small-signal steady-state gain (dB) of a freshly-configured filter.
double filterGainDb(const FilterCfg& cfg, double sr, float cutoffHz, float res, double f, double amp = 0.003) {
    Filter flt;
    flt.setSampleRate(sr);
    cfg(flt);
    double settleSec = f < 60.0 ? 2.5 : 0.6;
    double measSec = std::max(0.3, 16.0 / f);
    size_t nSettle = static_cast<size_t>(settleSec * sr);
    size_t nMeas = static_cast<size_t>(measSec * sr);
    size_t fade = static_cast<size_t>(0.05 * sr);
    std::vector<float> y(nSettle + nMeas);
    for (size_t n = 0; n < y.size(); ++n) {
        double env = n < fade ? 0.5 - 0.5 * std::cos(kPi * n / fade) : 1.0;
        float in = static_cast<float>(amp * env * std::sin(2.0 * kPi * f * n / sr));
        y[n] = flt.processSample(in, cutoffHz, res);
    }
    return 20.0 * std::log10(std::max(1e-12, toneAmplitude(y, nSettle, nMeas, sr, f) / amp));
}

// Resonant peak search: coarse log grid, then golden-section refinement.
double findPeakHz(const FilterCfg& cfg, double sr, float cutoffHz, float res, double fLo, double fHi,
                  double* peakDb) {
    const int kGrid = 36;
    std::vector<double> fs(kGrid), gs(kGrid);
    int best = 0;
    for (int i = 0; i < kGrid; ++i) {
        fs[i] = fLo * std::pow(fHi / fLo, static_cast<double>(i) / (kGrid - 1));
        gs[i] = filterGainDb(cfg, sr, cutoffHz, res, fs[i]);
        if (gs[i] > gs[best]) best = i;
    }
    double a = std::log(fs[std::max(0, best - 1)]);
    double b = std::log(fs[std::min(kGrid - 1, best + 1)]);
    const double gr = (std::sqrt(5.0) - 1.0) / 2.0;
    double c = b - gr * (b - a), d = a + gr * (b - a);
    double gc = filterGainDb(cfg, sr, cutoffHz, res, std::exp(c));
    double gd = filterGainDb(cfg, sr, cutoffHz, res, std::exp(d));
    for (int it = 0; it < 14; ++it) {
        if (gc > gd) { b = d; d = c; gd = gc; c = b - gr * (b - a); gc = filterGainDb(cfg, sr, cutoffHz, res, std::exp(c)); }
        else         { a = c; c = d; gc = gd; d = a + gr * (b - a); gd = filterGainDb(cfg, sr, cutoffHz, res, std::exp(d)); }
    }
    double fPk = std::exp(0.5 * (a + b));
    double gPk = std::max({gc, gd, gs[best]});
    if (peakDb) *peakDb = gPk;
    return fPk;
}

// Small click in, 1.6 s out: stable if the late tail is well below the early
// response (a self-oscillating loop grows or sustains).
bool filterIsStable(const FilterCfg& cfg, double sr, float cutoffHz, float res) {
    Filter flt;
    flt.setSampleRate(sr);
    cfg(flt);
    size_t n = static_cast<size_t>(1.6 * sr);
    double early = 0, late = 0;
    for (size_t i = 0; i < n; ++i) {
        float in = (i < 4) ? 0.002f : 0.0f;
        float y = flt.processSample(in, cutoffHz, res);
        double t = i / sr;
        if (t >= 0.005 && t < 0.105) early += double(y) * y;
        if (t >= 1.4) late += double(y) * y;
    }
    double eR = std::sqrt(early / (0.1 * sr)), lR = std::sqrt(late / (0.2 * sr));
    return std::isfinite(lR) && lR < 0.05 * eR;
}

// ---------------------------------------------------------------------------
// Engine rendering helpers
// ---------------------------------------------------------------------------

struct Ev {
    double t;
    int note;
    float vel;
    bool on;
};

struct Render {
    std::vector<float> audio;
    std::vector<float> cutoff;
    double sr;
};

Render renderEngine(const SynthParameters& p, double sr, std::vector<Ev> evs, double dur) {
    SynthEngine e;
    e.setSampleRate(sr);
    e.getParams() = p;
    if (gLadderTopologyOverride >= 0) e.getParams().filterLadderTopology = static_cast<float>(gLadderTopologyOverride);
    std::sort(evs.begin(), evs.end(), [](const Ev& a, const Ev& b) { return a.t < b.t; });
    size_t n = static_cast<size_t>(dur * sr);
    Render r{std::vector<float>(n), std::vector<float>(n), sr};
    size_t ei = 0;
    for (size_t i = 0; i < n; ++i) {
        while (ei < evs.size() && evs[ei].t * sr <= static_cast<double>(i)) {
            if (evs[ei].on) e.noteOn(evs[ei].note, evs[ei].vel);
            else e.noteOff(evs[ei].note);
            ++ei;
        }
        float l, rr;
        e.processAudio(&l, &rr, 1);
        r.audio[i] = l;
        r.cutoff[i] = e.getLastCutoffHz();
    }
    return r;
}

double rmsOf(const std::vector<float>& x, double sr, double t0, double t1) {
    size_t a = static_cast<size_t>(t0 * sr), b = std::min(x.size(), static_cast<size_t>(t1 * sr));
    double s = 0;
    for (size_t i = a; i < b; ++i) s += double(x[i]) * x[i];
    return std::sqrt(s / std::max<size_t>(1, b - a));
}

double maxOf(const std::vector<float>& x, double sr, double t0, double t1, double* tAt = nullptr) {
    size_t a = static_cast<size_t>(t0 * sr), b = std::min(x.size(), static_cast<size_t>(t1 * sr));
    double m = -1e30;
    for (size_t i = a; i < b; ++i)
        if (x[i] > m) { m = x[i]; if (tAt) *tAt = i / sr; }
    return m;
}

double midiHz(int note) { return 440.0 * std::pow(2.0, (note - 69) / 12.0); }

// Resonant peak of a rendered saw note: the harmonic with the largest
// amplitude*n (whitening the saw's 1/n spectrum, so this estimates the
// filter's gain at each harmonic), refined by parabolic interpolation.
double harmonicPeakHz(const std::vector<float>& x, double sr, double t0, double t1, double f0, double fMin) {
    size_t a = static_cast<size_t>(t0 * sr);
    size_t len = static_cast<size_t>((t1 - t0) * sr);
    int nLo = static_cast<int>(std::ceil(fMin / f0));
    int nHi = static_cast<int>(std::floor(std::min(0.45 * sr, 20000.0) / f0));
    std::vector<double> m(nHi + 2, 0.0);
    int best = nLo;
    for (int n = nLo; n <= nHi; ++n) {
        m[n] = std::log(std::max(1e-15, toneAmplitude(x, a, len, sr, n * f0) * n));
        if (m[n] > m[best]) best = n;
    }
    double delta = 0.0;
    if (best > nLo && best < nHi) {
        double y0 = m[best - 1], y1 = m[best], y2 = m[best + 1];
        double den = y0 - 2 * y1 + y2;
        if (std::abs(den) > 1e-12) delta = 0.5 * (y0 - y2) / den;
    }
    return f0 * (best + delta);
}

// Plain spectral peak of a short window (for fast-moving ring frequencies).
double spectralPeakHz(const std::vector<float>& x, double sr, double t0, double len, double fLo, double fHi) {
    size_t a = static_cast<size_t>(t0 * sr), n = static_cast<size_t>(len * sr);
    double bestF = fLo, bestA = -1;
    for (int i = 0; i < 600; ++i) {
        double f = fLo * std::pow(fHi / fLo, i / 599.0);
        double amp = toneAmplitude(x, a, n, sr, f);
        if (amp > bestA) { bestA = amp; bestF = f; }
    }
    return bestF;
}

SynthParameters panel(float cutoff, float res, float envMod, float decay, float accent) {
    SynthParameters p;
    p.cutoff = cutoff;
    p.resonance = res;
    p.envMod = envMod;
    p.decay = decay;
    p.accent = accent;
    p.waveform = Waveform::Saw;
    p.masterVolume = 1.0f;
    p.drive = 0.0f;
    return p;
}

// ===========================================================================
// A. Ladder core, small signal (§10.2)
// ===========================================================================

void testLadderCore() {
    std::printf("\n== A. Ladder core small-signal response (TB303_REFERENCE.md §10.2) ==\n");
    const double sr = 44100.0;
    const float cutoffHz = 2000.0f;
    const double rel[] = {0.02, 0.04, 0.07, 0.1, 0.15, 0.22, 0.33, 0.5, 0.75, 1.1, 1.6, 2.4, 3.3};

    auto run = [&](const std::string& id, const std::string& label, const FilterCfg& cfg, bool expectPass) {
        std::vector<double> fr, g;
        for (double r : rel) {
            double f = r * cutoffHz;
            fr.push_back(f);
            g.push_back(filterGainDb(cfg, sr, cutoffHz, 0.0f, f));
        }
        // Fit wc: minimise the worst-case deviation from Stinchcombe's core.
        double bestRatio = 1, bestDev = 1e9;
        for (int i = 0; i < 3000; ++i) {
            double ratio = 0.05 * std::pow(100.0, i / 2999.0); // wc / (2*pi*cutoffHz)
            double dev = 0;
            for (size_t j = 0; j < fr.size(); ++j)
                dev = std::max(dev, std::abs(g[j] - stinchCoreDb(fr[j] / (ratio * cutoffHz))));
            if (dev < bestDev) { bestDev = dev; bestRatio = ratio; }
        }
        (void)expectPass;
        check(id, "§10.2", label,
              fmt("best-fit wc = %.3f x 2pi*cutoffHz, worst deviation from H_tb(s) = %.2f dB", bestRatio, bestDev),
              "worst deviation <= 1.0 dB (poles -0.128/-1.038/-2.325/-3.236 wc, 24 dB/oct asymptote)",
              bestDev <= 1.0);
    };

    run("A1", "Solver control: equal cap scales (1,1,1,1), couplings removed",
        [](Filter& f) { neutralizeCouplings(f); }, true);
    SynthParameters p;
    run("A2", "Shipped defaults: capScale1..4 as shipped, couplings removed",
        [p](Filter& f) { applyFilterParams(f, p); neutralizeCouplings(f); }, false);
}

// ===========================================================================
// B. Resonance loop (§11.2, §12)
// ===========================================================================

void testResonanceLoop(bool fast) {
    std::printf("\n== B. Resonance loop with the shipped coupling network (§11.2, §12) ==\n");
    const double sr = 44100.0;
    const SynthParameters p;
    FilterCfg shipped = [p](Filter& f) { applyFilterParams(f, p); };
    // Stinchcombe's model ends at the VCF output, so the frequency-response
    // comparisons (B2..B7) park the out-of-loop Open303-style stages (post
    // HP and notch; the allpass is magnitude-neutral). Everything inside the
    // loop and ahead of the ladder stays exactly as shipped.
    FilterCfg shippedVcf = [p](Filter& f) {
        applyFilterParams(f, p);
        f.setPostFilterHpHz(0.001f);
        f.setNotchFreqHz(20000.0f);
        f.setNotchBandwidthHz(1.0f);
    };

    // B1: no self-oscillation anywhere on the Resonance knob.
    {
        const float cutoffs[] = {100, 200, 400, 800, 1600, 3200, 6400, 12800};
        std::string bad;
        for (float c : cutoffs)
            if (!filterIsStable(shipped, sr, c, 1.0f)) bad += fmt("%.0f ", c);
        check("B1", "§12", "No self-oscillation at Resonance = 1 (shipped defaults), cutoffHz 100..12800",
              bad.empty() ? "stable at every tested cutoff" : "self-oscillates at cutoffHz = " + bad,
              "stable everywhere (stock unit sits just below the threshold)", bad.empty());
    }

    // Locate cutoffHz whose Resonance=1 peak lands at Stinchcombe's 1034 Hz.
    float cut = 1034.0f;
    double pkDb = 0, pkHz = 0;
    for (int it = 0; it < 4; ++it) {
        pkHz = findPeakHz(shippedVcf, sr, cut, 1.0f, 150.0, 6000.0, &pkDb);
        cut = static_cast<float>(cut * 1034.0 / pkHz);
    }
    pkHz = findPeakHz(shippedVcf, sr, cut, 1.0f, 150.0, 6000.0, &pkDb);
    std::printf("        (cutoffHz = %.1f puts the Resonance=1 peak at %.1f Hz)\n", cut, pkHz);
    bool stableHere = filterIsStable(shippedVcf, sr, cut, 1.0f);

    auto g = [&](float res, double f) { return filterGainDb(shippedVcf, sr, cut, res, f); };
    const double ref0 = g(0.0f, 100.0);
    const double s0 = stinchFullDb(100.0, 0.0);

    double drop100 = ref0 - g(1.0f, 100.0);
    double drop30 = g(0.0f, 30.0) - g(1.0f, 30.0);
    double peakRel = pkDb - ref0;
    double hump = g(1.0f, 8.7) - g(1.0f, 100.0);
    double lf0 = g(0.0f, 30.0) - ref0;
    double sDrop100 = s0 - stinchFullDb(100.0, 1.0);
    double sDrop30 = stinchFullDb(30.0, 0.0) - stinchFullDb(30.0, 1.0);
    double sPeakRel = 4.1 - s0;
    double sHump = stinchFullDb(8.7, 1.0) - stinchFullDb(100.0, 1.0);
    double sLf0 = stinchFullDb(30.0, 0.0) - s0;

    check("B2", "§11.2", "Pass-band loss at 100 Hz, Resonance 0 -> 1 (peak matched to 1034 Hz)",
          fmt("%.1f dB", drop100), fmt("%.1f dB +/- 3 (Stinchcombe full model; core-only 1/(1+k) = 25 dB)", sDrop100),
          std::abs(drop100 - sDrop100) <= 3.0);
    check("B3", "§11.2", "Pass-band loss at 30 Hz, Resonance 0 -> 1",
          fmt("%.1f dB", drop30), fmt("%.1f dB +/- 3", sDrop30), std::abs(drop30 - sDrop30) <= 3.0);
    check("B4", "§11.2", "Resonant peak height at Resonance 1, relative to the Resonance-0 level at 100 Hz",
          fmt("%+.1f dB (peak at %.0f Hz)", peakRel, pkHz), fmt("%+.1f dB +/- 3", sPeakRel),
          std::abs(peakRel - sPeakRel) <= 3.0);
    check("B5", "§11.2", "Sub-bass hump at Resonance 1 (VCF output): gain(8.7 Hz) - gain(100 Hz)",
          fmt("%+.1f dB", hump), fmt("%+.1f dB +/- 4 (\"stays ~15 dB above the 100 Hz level\")", sHump),
          std::abs(hump - sHump) <= 4.0);
    check("B6", "§11.2", "Low-frequency shape at Resonance 0: gain(30 Hz) - gain(100 Hz)",
          fmt("%+.1f dB", lf0), fmt("%+.1f dB +/- 3", sLf0), std::abs(lf0 - sLf0) <= 3.0);

    // B7: resonant peak moves up with resonance at fixed control current.
    {
        double dbHalf = 0;
        double pkHalf = findPeakHz(shippedVcf, sr, cut, 0.5f, 150.0, 6000.0, &dbHalf);
        double ratio = pkHz / pkHalf;
        check("B7", "§11.2", "Peak frequency rises with Resonance at fixed cutoff current: f(1.0)/f(0.5)",
              fmt("%.2f (%.0f Hz -> %.0f Hz)", ratio, pkHalf, pkHz),
              "> 1.05 (Stinchcombe: 854 -> 1034 Hz between k=0.6 and k=1; knob law uncertain)", ratio > 1.05);
    }

    // B8: how far max resonance sits below the oscillation threshold.
    if (!fast) {
        if (!stableHere) {
            check("B8", "§11.2", "Oscillation margin: feedback-gain multiplier that tips Resonance=1 into oscillation",
                  "already self-oscillating at x1.00", "x1.02 .. x1.15 (Stinchcombe: unstable at k ~ 1.065)", false);
        } else {
            double lo = 1.0, hi = 3.0;
            auto stableAt = [&](double m) {
                SynthParameters q = p;
                q.filterFeedbackGain = static_cast<float>(p.filterFeedbackGain * m);
                q.filterResonanceLimit = static_cast<float>(p.filterResonanceLimit * m);
                return filterIsStable([q](Filter& f) { applyFilterParams(f, q); }, sr, cut, 1.0f);
            };
            if (stableAt(hi)) {
                check("B8", "§11.2", "Oscillation margin: feedback-gain multiplier that tips Resonance=1 into oscillation",
                      "> x3.0", "x1.02 .. x1.15 (Stinchcombe: unstable at k ~ 1.065)", false);
            } else {
                for (int i = 0; i < 9; ++i) {
                    double m = 0.5 * (lo + hi);
                    (stableAt(m) ? lo : hi) = m;
                }
                double m = 0.5 * (lo + hi);
                check("B8", "§11.2", "Oscillation margin: feedback-gain multiplier that tips Resonance=1 into oscillation",
                      fmt("x%.3f", m), "x1.02 .. x1.15 (Stinchcombe: unstable at k ~ 1.065)", inRange(m, 1.02, 1.15));
            }
        }
    }

    // B9: the same margin across the cutoff range. Stinchcombe's full model
    // (§11.1) puts the threshold at k = 2.42 / 1.58 / 1.22 / 1.06 / 0.98 /
    // 0.95 for wc = 820 Hz x 1/8 .. 4 (roots of the 10th-order denominator,
    // precomputed): resonance fades at low cutoff because the coupling
    // network eats the loop gain, and the loop is closest to oscillation at
    // high cutoff. Where his model dips below 1 the hardware is still
    // reported not to self-oscillate cleanly (§12), so the target there is
    // "just above 1". cutoffHz is scaled with the same factor that put the
    // Resonance=1 peak at 1034 Hz above (wc = 820 Hz <-> that cutoffHz).
    if (!fast && stableHere) {
        const double mult[] = {0.125, 0.25, 0.5, 1.0, 2.0, 4.0};
        const double stinch[] = {2.424, 1.579, 1.222, 1.060, 0.983, 0.946};
        std::string meas, targ;
        bool ok = true;
        for (int i = 0; i < 6; ++i) {
            float c = static_cast<float>(cut * mult[i]);
            auto stableAt = [&](double m) {
                SynthParameters q = p;
                q.filterFeedbackGain = static_cast<float>(p.filterFeedbackGain * m);
                q.filterResonanceLimit = static_cast<float>(p.filterResonanceLimit * m);
                return filterIsStable([q](Filter& f) { applyFilterParams(f, q); }, sr, c, 1.0f);
            };
            double lo = 0.25, hi = 4.0, m;
            if (!stableAt(1.0)) { hi = 1.0; } else { lo = 1.0; }
            if (stableAt(hi)) { m = hi; }
            else {
                for (int it = 0; it < 9; ++it) { double mid = std::sqrt(lo * hi); (stableAt(mid) ? lo : hi) = mid; }
                m = std::sqrt(lo * hi);
            }
            double t = std::max(stinch[i], 1.02);
            bool rowOk = (stinch[i] < 1.0) ? inRange(m, 1.0, 1.1) : std::abs(std::log(m / t)) <= std::log(1.15);
            ok = ok && rowOk;
            meas += fmt("%s%.2f", i ? " / " : "", m);
            targ += fmt("%s%.2f", i ? " / " : "", t);
        }
        check("B9", "§11.1/§12", "Oscillation margin across cutoff (wc = 820 Hz x 1/8, 1/4, 1/2, 1, 2, 4)",
              "x" + meas, "x" + targ + " (+/-15 %; 1.00..1.10 where Stinchcombe < 1)", ok);
    }
}

// ===========================================================================
// C/D. Oscillator and pulse shaper (§6, §7, §8, §9)
// ===========================================================================

std::vector<float> renderOsc(const SynthParameters& p, Waveform w, int note, double sr, double dur,
                             float couplingOverrideHz = -1.0f) {
    Oscillator o;
    o.setSampleRate(sr);
    applyOscParams(o, p);
    if (couplingOverrideHz > 0) o.setCouplingHz(couplingOverrideHz);
    o.setWaveform(w);
    o.noteOn(note, false);
    std::vector<float> y(static_cast<size_t>(dur * sr));
    for (auto& v : y) v = o.processNextSample();
    return y;
}

void testOscillator() {
    std::printf("\n== C. Oscillator, pulse shaper, slide (§6, §7, §8, §9) ==\n");
    const SynthParameters p;
    const double sr = 44100.0;

    // C1: pre-filter high-pass. The saw goes straight into the VCF input
    // coupling [M]; the empirical lumped value is Open303's 44.5 Hz [I].
    {
        auto y = renderOsc(p, Waveform::Saw, 33, sr, 1.5); // A1 = 55 Hz
        double f0 = midiHz(33);
        double h1 = toneAmplitude(y, static_cast<size_t>(0.5 * sr), static_cast<size_t>(1.0 * sr), sr, f0);
        double h8 = toneAmplitude(y, static_cast<size_t>(0.5 * sr), static_cast<size_t>(1.0 * sr), sr, 8 * f0) * 8;
        double loss = 20 * std::log10(h8 / h1);
        check("C1", "§7.2/§11.3", "Oscillator-side high-pass: fundamental loss of a 55 Hz saw (vs 1/n law)",
              fmt("%.1f dB (oscCouplingHz = %.1f Hz; filter adds its own %.1f Hz input HP on top)", loss,
                  p.oscCouplingHz, p.filterInputCouplingHz),
              "<= 2.5 dB (44.5 Hz one-pole = 2.1 dB; hardware: none before the VCF's own coupling)", loss <= 2.5);
    }

    // C2: saw is a clean linear ramp; no x - s*x^2 bend, no 14 kHz LPF.
    {
        auto y = renderOsc(p, Waveform::Saw, 45, sr, 0.5, 0.01f); // 110 Hz, HP removed
        double period = sr / midiHz(45);
        size_t start = static_cast<size_t>(0.3 * sr);
        // find a reset (largest positive step) after start
        size_t rst = start;
        float bestStep = 0;
        for (size_t i = start; i < start + static_cast<size_t>(period) + 2; ++i)
            if (y[i + 1] - y[i] > bestStep) { bestStep = y[i + 1] - y[i]; rst = i + 1; }
        size_t a = rst + static_cast<size_t>(0.1 * period), b = rst + static_cast<size_t>(0.9 * period);
        double sx = 0, sy = 0, sxx = 0, sxy = 0, n = double(b - a);
        for (size_t i = a; i < b; ++i) { double x = double(i); sx += x; sy += y[i]; sxx += x * x; sxy += x * y[i]; }
        double slope = (n * sxy - sx * sy) / (n * sxx - sx * sx), icpt = (sy - slope * sx) / n;
        double maxRes = 0, mn = 1e9, mx = -1e9;
        for (size_t i = a; i < b; ++i) {
            maxRes = std::max(maxRes, std::abs(y[i] - (slope * i + icpt)));
            mn = std::min<double>(mn, y[i]); mx = std::max<double>(mx, y[i]);
        }
        double rel = 100.0 * maxRes / (mx - mn);
        check("C2", "§7.2", "Saw ramp linearity (residual from a straight line, middle 80% of a cycle)",
              fmt("%.2f %% of p-p (oscSawShape = %.3f, oscSawLpfHz = %.0f)", rel, p.oscSawShape, p.oscSawLpfHz),
              "<= 1 % (\"14 kHz saw LPF / x - 0.05x^2 bend not supported by any source -- drop them\")", rel <= 1.0);
    }

    // C3: square duty near 50 % at 100-120 Hz.
    {
        auto y = renderOsc(p, Waveform::Square, 45, sr, 1.0, 0.01f);
        size_t a = static_cast<size_t>(0.2 * sr), b = y.size();
        float mn = *std::min_element(y.begin() + a, y.end()), mx = *std::max_element(y.begin() + a, y.end());
        float mid = 0.5f * (mn + mx);
        size_t hi = 0;
        for (size_t i = a; i < b; ++i) hi += y[i] > mid;
        double duty = 100.0 * hi / double(b - a);
        check("C3", "§8.2", "Square pulse width at 110 Hz",
              fmt("%.1f %% high", duty), "47..53 % (\"close to symmetric around 100-120 Hz\", antto fit)",
              inRange(duty, 47.0, 53.0));
    }

    // C4: saw/square level ratio into the VCF.
    {
        auto ys = renderOsc(p, Waveform::Saw, 45, sr, 0.5, 0.01f);
        auto yq = renderOsc(p, Waveform::Square, 45, sr, 0.5, 0.01f);
        auto pp = [&](const std::vector<float>& y) {
            auto a = y.begin() + static_cast<long>(0.2 * sr);
            return *std::max_element(a, y.end()) - *std::min_element(a, y.end());
        };
        double r = pp(ys) / pp(yq);
        check("C4", "§9", "Saw / square peak-to-peak level ratio at the VCF input",
              fmt("%.2f", r), "~2 (schematic sketches: saw ~5.5-12 V, square ~5-8 V); accept 1.6..2.6",
              inRange(r, 1.6, 2.6));
    }

    // C5: one-edge excitation of the resonance by the square.
    {
        auto osc = renderOsc(p, Waveform::Square, 33, sr, 1.0);
        Filter flt;
        flt.setSampleRate(sr);
        applyFilterParams(flt, p);
        std::vector<float> y(osc.size());
        for (size_t i = 0; i < osc.size(); ++i) y[i] = flt.processSample(osc[i], 2500.0f, 1.0f);
        size_t a = static_cast<size_t>(0.4 * sr);
        float mn = *std::min_element(osc.begin() + a, osc.end()), mx = *std::max_element(osc.begin() + a, osc.end());
        float mid = 0.5f * (mn + mx);
        double eUp = 0, eDn = 0;
        size_t w0 = static_cast<size_t>(0.0003 * sr), w1 = static_cast<size_t>(0.004 * sr);
        for (size_t i = a; i + w1 + 1 < osc.size(); ++i) {
            bool up = osc[i] <= mid && osc[i + 1] > mid, dn = osc[i] >= mid && osc[i + 1] < mid;
            if (!up && !dn) continue;
            double e = 0;
            for (size_t k = i + w0; k < i + w1; ++k) { double d = y[k + 1] - y[k]; e += d * d; }
            (up ? eUp : eDn) += e;
        }
        double ratioDb = 10 * std::log10(std::min(eUp, eDn) / std::max(eUp, eDn));
        check("C5", "§8.2", "Square: resonance ringing after the soft edge vs the hard edge (Res 1, high cutoff)",
              fmt("%+.1f dB", ratioDb), "<= -6 dB (\"rings after the hard edge but hardly at all after the soft edge\")",
              ratioDb <= -6.0);
    }

    // C6/C7: slide RC, tau = 22 ms, applied in volts (semitones).
    {
        auto slide = [&](int from, int to, double* t63, double* t90) {
            Oscillator o;
            o.setSampleRate(sr);
            applyOscParams(o, p);
            o.noteOn(from, false);
            for (int i = 0; i < static_cast<int>(0.05 * sr); ++i) o.processNextSample();
            o.noteOn(to, true);
            double st0 = from, st1 = to;
            *t63 = *t90 = -1;
            for (int i = 0; i < static_cast<int>(0.5 * sr); ++i) {
                o.processNextSample();
                double st = 69 + 12 * std::log2(o.getCurrentFreqHz() / 440.0);
                double frac = (st - st0) / (st1 - st0);
                if (*t63 < 0 && frac >= 1 - std::exp(-1.0)) *t63 = (i + 1) / sr;
                if (*t90 < 0 && frac >= 0.9) *t90 = (i + 1) / sr;
            }
        };
        double u63, u90, d63, d90;
        slide(45, 57, &u63, &u90);
        slide(57, 45, &d63, &d90);
        check("C6", "§6", "Slide time constant in the pitch (semitone) domain, octave up / octave down",
              fmt("up: 63%% at %.1f ms, 90%% at %.1f ms; down: 63%% at %.1f ms, 90%% at %.1f ms",
                  u63 * 1e3, u90 * 1e3, d63 * 1e3, d90 * 1e3),
              "tau = 22 ms (63% at 18..26 ms), 90% at ~51 ms, identical up and down (RC on the CV in volts)",
              inRange(u63, 0.018, 0.026) && inRange(d63, 0.018, 0.026) && std::abs(u63 - d63) < 0.002);
    }

    // C8: final output polarity -- DC-coupled recordings show the saw rising
    // then dropping (§2.3, §7.2).
    {
        auto r = renderEngine(panel(1.0f, 0.0f, 0.0f, 0.0f, 0.0f), sr, {{0.0, 45, 0.5f, true}}, 0.4);
        double up = 0, dn = 0;
        for (size_t i = static_cast<size_t>(0.1 * sr); i + 1 < r.audio.size(); ++i) {
            double d = r.audio[i + 1] - r.audio[i];
            up = std::max(up, d); dn = std::min(dn, d);
        }
        check("C7", "§2.3/§7.2", "Output polarity of the saw (direction of the sharp edge)",
              fmt("sharp edge %s (max step up %.3f, down %.3f)", (-dn > up) ? "falls" : "rises", up, dn),
              "sharp edge falls (\"in most DC-coupled recordings the saw rises from -1 to +1 and then drops\")",
              -dn > up);
    }
}

// ===========================================================================
// D. Envelopes (§14, §15)
// ===========================================================================

void testEnvelopes() {
    std::printf("\n== D. MEG / VEG timing (§14, §15) ==\n");
    const SynthParameters p;
    const double sr = 44100.0;

    struct Trace { std::vector<float> meg, veg; };
    auto run = [&](float decay, bool accent, double dur) {
        Envelope e;
        e.setSampleRate(sr);
        applyEnvParams(e, p, decay);
        e.noteOn(accent, false, 1.0f);
        Trace t;
        for (int i = 0; i < static_cast<int>(dur * sr); ++i) {
            e.processNextSample();
            t.meg.push_back(e.getVcfEnv());
            t.veg.push_back(e.getVcaEnv());
        }
        return t;
    };
    auto tau = [&](const std::vector<float>& x, double* tPeak) {
        size_t ip = std::max_element(x.begin(), x.end()) - x.begin();
        if (tPeak) *tPeak = ip / sr;
        float thr = x[ip] * std::exp(-1.0f);
        for (size_t i = ip; i < x.size(); ++i)
            if (x[i] <= thr) return (i - ip) / sr;
        return -1.0;
    };
    auto riseTo = [&](const std::vector<float>& x, double frac) {
        float pk = *std::max_element(x.begin(), x.end());
        for (size_t i = 0; i < x.size(); ++i)
            if (x[i] >= frac * pk) return (i + 1) / sr;
        return -1.0;
    };

    auto t0 = run(0.0f, false, 2.0), t1 = run(1.0f, false, 8.0), tm = run(0.5f, false, 3.0), ta = run(0.7f, true, 2.0);
    double tMin = tau(t0.meg, nullptr), tMax = tau(t1.meg, nullptr), tMid = tau(tm.meg, nullptr), tAcc = tau(ta.meg, nullptr);
    double att = riseTo(t0.meg, 0.95);

    check("D1", "§14.1", "MEG charge time (to 95 % of peak)",
          fmt("%.2f ms (vcfAttackMs = %.2f)", att * 1e3, p.vcfAttackMs),
          "<= 1 ms (C62 recharges through R152 100 R, tau ~0.1 ms; \"3 ms attack\" is busted)", att <= 0.001);
    check("D2", "§14.1", "MEG decay time constant at Decay = min",
          fmt("tau = %.0f ms", tMin * 1e3), "68..87 ms (R136 68k x C62 1uF; printed T90 = 200 ms)", inRange(tMin, 0.060, 0.095));
    check("D3", "§14.1", "MEG decay time constant at Decay = max",
          fmt("tau = %.2f s", tMax), "1.07..1.09 s (68k + 1M into 1 uF; printed T90 = 2.5 s); accept 0.95..1.2",
          inRange(tMax, 0.95, 1.2));
    check("D4", "§14.1", "MEG decay time constant at Decay = 12 o'clock (A-taper pot law)",
          fmt("tau = %.0f ms", tMid * 1e3), "~168 ms if VR6 follows R = Rtot(81^x - 1)/80 [E]", true, true);
    check("D5", "§14.1/§16.1", "MEG decay time constant on accented notes (Decay pot shorted)",
          fmt("tau = %.0f ms (accentDecaySec = %.3f)", tAcc * 1e3, p.accentDecaySec),
          "68..87 ms, same as Decay = min", inRange(tAcc, 0.060, 0.095));

    auto tv = run(0.0f, false, 8.0);
    double tVeg = tau(tv.veg, nullptr);
    check("D6", "§15.1", "VEG decay time constant, gate held",
          fmt("tau = %.2f s (vegDecaySec = %.2f)", tVeg, p.vegDecaySec),
          "1.2..1.5 s (R123 1.5M x C42 1uF; Open303 fit 1.23 s); \"3-4 s\" is T90", inRange(tVeg, 1.1, 1.6));
    double vOn = riseTo(tv.veg, 0.9);
    check("D7", "§15.2", "VEG onset: time for the VCA control to reach 90 %",
          fmt("%.1f ms", vOn * 1e3), "a few ms, up to ~5 ms (R134 22k / C41 0.1uF, tau 2.2 ms); accept <= 6 ms",
          vOn <= 0.006);
}

// ===========================================================================
// E. Control-current summing: cutoff, Env Mod, accent, VCA (§13, §15, §16)
// ===========================================================================

void testEngine() {
    std::printf("\n== E. Cutoff / Env Mod / Accent / VCA through the whole engine (§13, §15, §16) ==\n");
    const double sr = 44100.0;

    // E1: service calibration anchor.
    {
        auto r = renderEngine(panel(0.5f, 1.0f, 0.0f, 0.0f, 0.0f), sr, {{0.0, 36, 0.5f, true}}, 0.8);
        double f = harmonicPeakHz(r.audio, sr, 0.3, 0.3 + 12 / midiHz(36), midiHz(36), 130.0);
        check("E1", "§13.3", "Service VCF calibration: Cutoff centre, Res max, EnvMod/Decay/Accent min, saw, key C",
              fmt("resonant peak %.0f Hz (period %.2f ms)", f, 1e3 / f),
              "ring period 2 ms +/- 0.5 ms -> 400..670 Hz [S]", inRange(f, 400.0, 670.0));
    }

    // E2/E3: Cutoff knob span, EnvMod min, Res max.
    {
        double f0 = midiHz(33);
        auto rLo = renderEngine(panel(0.0f, 1.0f, 0.0f, 0.0f, 0.0f), sr, {{0.0, 33, 0.5f, true}}, 0.8);
        auto rHi = renderEngine(panel(1.0f, 1.0f, 0.0f, 0.0f, 0.0f), sr, {{0.0, 33, 0.5f, true}}, 0.8);
        double lo = harmonicPeakHz(rLo.audio, sr, 0.4, 0.4 + 16 / f0, f0, 130.0);
        double hi = harmonicPeakHz(rHi.audio, sr, 0.4, 0.4 + 16 / f0, f0, 130.0);
        check("E2", "§13.3", "Cutoff knob minimum (EnvMod min, Res max), settled",
              fmt("resonant peak %.0f Hz", lo), "~314 Hz (Open303 fit of hardware); accept +/-25 % TM3 tolerance: 235..390",
              inRange(lo, 235.0, 390.0));
        check("E3", "§13.3", "Cutoff knob maximum (EnvMod min, Res max), settled",
              fmt("resonant peak %.0f Hz", hi), "~2394 Hz settled, 3.2-3.5 kHz at note start; accept 1800..3000",
              inRange(hi, 1800.0, 3000.0));
    }

    // Env Mod law, read off the engine's cutoff CV probe.
    auto cvRun = [&](float cutoff, float envMod) {
        auto r = renderEngine(panel(cutoff, 0.5f, envMod, 0.0f, 0.0f), sr, {{0.0, 45, 0.5f, true}}, 2.0);
        double peak = maxOf(r.cutoff, sr, 0.0, 0.1);
        double rest = r.cutoff[static_cast<size_t>(1.9 * sr)];
        return std::make_pair(peak, rest);
    };
    {
        auto e0 = cvRun(0.0f, 0.0f);
        double resid = std::log2(e0.first / e0.second);
        check("E4", "§13.1/§13.2", "Residual MEG sweep at Env Mod = 0 (Cutoff min)",
              fmt("%.2f oct", resid), ">= 0.5 oct (Open303 envScaler 0.737 oct; kunn 0.73-0.88)", resid >= 0.5);

        auto e1 = cvRun(0.0f, 1.0f);
        double bias = std::log2(e1.second / e0.second);
        check("E5", "§13.1", "Env Mod bias shift: settled cutoff at Env Mod 1 vs Env Mod 0 (Cutoff min)",
              fmt("%+.2f oct", bias),
              "<= -0.8 oct (\"raising Env Mod ... shifts the bias so the cutoff drops\"; Open303 ~ -1.3 oct)",
              bias <= -0.8);
        double depth = std::log2(e1.first / e1.second);
        check("E6", "§13.2", "MEG sweep depth at Env Mod = 1 (Cutoff min): peak vs settled",
              fmt("%.2f oct", depth), "4.5..5.7 oct (Open303 4.51 at Cutoff min; kunn 5.3-5.65)", inRange(depth, 4.5, 5.7));
    }

    // E7: full-modulation resonant peak.
    {
        const double sr2 = 192000.0;
        auto r = renderEngine(panel(1.0f, 1.0f, 1.0f, 1.0f, 0.0f), sr2, {{0.0, 33, 0.5f, true}}, 0.1);
        double tAt = 0;
        double cvMax = maxOf(r.cutoff, sr2, 0.0, 0.08, &tAt);
        double f = spectralPeakHz(r.audio, sr2, std::max(0.0, tAt - 0.002), 0.004, 1500.0, 60000.0);
        check("E7", "§13.3", "Resonant peak at Cutoff max + Env Mod max (MEG peak), Res max",
              fmt("~%.1f kHz (engine cutoff CV peaks at %.0f Hz; clamped at 15 kHz / 18 kHz)", f / 1e3, cvMax),
              "~25-28 kHz (rv0 units: 27.5-28 and 23.5 kHz); accept >= 20 kHz", f >= 20000.0);
    }

    // E8: VCA follows the VEG decay (held note, filter static).
    {
        auto r = renderEngine(panel(1.0f, 0.0f, 0.0f, 0.0f, 0.0f), sr, {{0.0, 45, 0.5f, true}}, 1.3);
        double per = 1.0 / midiHz(45);
        double a = rmsOf(r.audio, sr, 0.2, 0.2 + 4 * per), b = rmsOf(r.audio, sr, 1.0, 1.0 + 4 * per);
        double drop = 20 * std::log10(a / b);
        check("E8", "§15.1/§15.3", "Audible VCA decay of a held note, 0.2 s -> 1.0 s (Res 0, Cutoff max, EnvMod 0)",
              fmt("%.1f dB", drop),
              "3.5..8 dB (VEG tau 1.2-1.5 s: 4.6-5.8 dB; Robin's fit e^(-t/1.23)+0.76e^(-t/58ms): 5.9 dB)",
              inRange(drop, 3.5, 8.0));
    }

    // E9: MEG leaks into the VCA on every note.
    {
        auto r = renderEngine(panel(1.0f, 0.0f, 0.0f, 0.0f, 0.0f), sr, {{0.0, 45, 0.5f, true}}, 0.4);
        double per = 1.0 / midiHz(45);
        double a = rmsOf(r.audio, sr, 0.002, 0.002 + 2 * per), b = rmsOf(r.audio, sr, 0.150, 0.150 + 2 * per);
        double ratio = a / b;
        check("E9", "§15.3", "Fast MEG term in the VCA on a normal note: level(2-20 ms) / level(150-168 ms), Decay min",
              fmt("%.2f", ratio), ">= 1.35 (Robin's fit gives ~1.75; VEG alone gives ~1.13)", ratio >= 1.35);
    }

    // E10: accent is much louder at max Accent, min Resonance.
    {
        double per = 1.0 / midiHz(45);
        auto rn = renderEngine(panel(1.0f, 0.0f, 0.0f, 0.0f, 1.0f), sr, {{0.0, 45, 0.5f, true}}, 0.1);
        auto ra = renderEngine(panel(1.0f, 0.0f, 0.0f, 0.0f, 1.0f), sr, {{0.0, 45, 1.0f, true}}, 0.1);
        double gain = 20 * std::log10(rmsOf(ra.audio, sr, 0.003, 0.003 + 3 * per) / rmsOf(rn.audio, sr, 0.003, 0.003 + 3 * per));
        check("E10", "§15.3", "Accent loudness at Accent max, Resonance min (first ~30 ms)",
              fmt("%+.1f dB", gain), ">= +3 dB (\"accented notes are much louder\"; control-current sum, not +6 dB)",
              gain >= 3.0);
    }

    // E11/E12: accent-sweep capacitor C13 memory.
    {
        const float res = 0.75f;
        auto peakOfNote = [&](const Render& r, double t0) {
            return maxOf(r.cutoff, sr, t0, t0 + 0.03);
        };
        std::vector<Ev> accThenNormal = {{0.0, 45, 1.0f, true}, {0.0625, 45, 0, false},
                                         {0.125, 45, 0.5f, true}, {0.1875, 45, 0, false}};
        std::vector<Ev> normalThenNormal = {{0.0, 45, 0.5f, true}, {0.0625, 45, 0, false},
                                            {0.125, 45, 0.5f, true}, {0.1875, 45, 0, false}};
        auto rA = renderEngine(panel(0.3f, res, 0.5f, 0.0f, 1.0f), sr, accThenNormal, 0.3);
        auto rN = renderEngine(panel(0.3f, res, 0.5f, 0.0f, 1.0f), sr, normalThenNormal, 0.3);
        double lift = std::log2(peakOfNote(rA, 0.125) / peakOfNote(rN, 0.125));
        check("E11", "§16.2", "Normal note right after an accent starts higher (C13 still discharging), 120 BPM 16ths",
              fmt("%+.2f oct vs the same note after a normal note", lift),
              ">= +0.1 oct (\"at fast tempos the notes after an accent also start higher\")", lift >= 0.1);

        std::vector<Ev> chain;
        for (int i = 0; i < 4; ++i) {
            chain.push_back({0.125 * i, 45, 1.0f, true});
            chain.push_back({0.125 * i + 0.0625, 45, 0, false});
        }
        auto rC = renderEngine(panel(0.3f, 1.0f, 0.5f, 0.0f, 1.0f), sr, chain, 0.6);
        double p1 = peakOfNote(rC, 0.0), p4 = peakOfNote(rC, 0.375);
        double rise = std::log2(p4 / p1);
        check("E12", "§16.2", "Accent chain A-A-A-A at 120 BPM 16ths, Res max: 4th peak vs 1st",
              fmt("%+.2f oct", rise), "> +0.1 oct (\"each peak is higher than the last\")", rise > 0.1);
    }
}

} // namespace

int main(int argc, char** argv) {
    bool fast = false;
    for (int i = 1; i < argc; ++i)
        if (std::strcmp(argv[i], "--fast") == 0) fast = true;
        else if (std::strcmp(argv[i], "--ladder-topology") == 0 && i + 1 < argc) gLadderTopologyOverride = std::atoi(argv[++i]);

    std::printf("Acidus reference-conformance tests (docs/TB-303 Reference/TB303_REFERENCE.md)\n");
    testLadderCore();
    testResonanceLoop(fast);
    testOscillator();
    testEnvelopes();
    testEngine();

    int fails = 0, passes = 0, infos = 0;
    for (const auto& r : gRows) {
        if (r.info) ++infos;
        else if (r.pass) ++passes;
        else ++fails;
    }
    std::printf("\nSummary: %d passed, %d failed, %d informational\n", passes, fails, infos);
    std::printf("\n| ID | Ref | Check | Measured | Target | Result |\n|---|---|---|---|---|---|\n");
    for (const auto& r : gRows)
        std::printf("| %s | %s | %s | %s | %s | %s |\n", r.id.c_str(), r.section.c_str(), r.name.c_str(),
                    r.measured.c_str(), r.target.c_str(), r.info ? "INFO" : (r.pass ? "PASS" : "**FAIL**"));
    return fails;
}
