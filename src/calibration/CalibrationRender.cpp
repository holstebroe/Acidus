// C-ABI render entry point for the offline reference-sample calibrator
// (tools/calibrate_reference.py). Built as a shared library
// (acidus_calibration_render) that the Python script loads through ctypes,
// so an optimizer can render thousands of candidate notes through the exact
// same SynthEngine signal path the plugin uses, without a process spawn or
// a WAV round-trip per evaluation.
//
// Every float field of SynthParameters is addressable by name, so the
// calibrator's output maps 1:1 onto SynthParameters defaults.

#include "core/SynthEngine.hpp"

#include <cstddef>
#include <vector>

#if defined(_WIN32)
#define ACIDUS_CALIB_API extern "C" __declspec(dllexport)
#else
#define ACIDUS_CALIB_API extern "C" __attribute__((visibility("default")))
#endif

namespace {

using acidus::SynthParameters;

struct ParamEntry {
    const char* name;
    float SynthParameters::*member;
};

#define ACIDUS_PARAM(field) { #field, &SynthParameters::field }

const ParamEntry kParams[] = {
    // Front panel
    ACIDUS_PARAM(cutoff),
    ACIDUS_PARAM(resonance),
    ACIDUS_PARAM(envMod),
    ACIDUS_PARAM(decay),
    ACIDUS_PARAM(accent),
    ACIDUS_PARAM(masterVolume),
    ACIDUS_PARAM(drive),
    ACIDUS_PARAM(tuningCents),
    // Experimental / calibration (CLAP-exposed in a calibration build)
    ACIDUS_PARAM(oscCouplingHz),
    ACIDUS_PARAM(resCouplingHz),
    ACIDUS_PARAM(filterFeedbackGain),
    ACIDUS_PARAM(filterPostHpHz),
    ACIDUS_PARAM(filterNotchHz),
    ACIDUS_PARAM(filterNotchBandwidthHz),
    ACIDUS_PARAM(filterAllpassHz),
    ACIDUS_PARAM(filterInputCouplingHz),
    ACIDUS_PARAM(filterOutputCouplingHz),
    ACIDUS_PARAM(filterCapScale1),
    ACIDUS_PARAM(filterCapScale2),
    ACIDUS_PARAM(filterCapScale3),
    ACIDUS_PARAM(filterCapScale4),
    ACIDUS_PARAM(filterLadderInputScale),
    ACIDUS_PARAM(vegDecaySec),
    ACIDUS_PARAM(vcaGateOffMs),
    ACIDUS_PARAM(vcaGateOffAccentMs),
    ACIDUS_PARAM(vcaGainSaturationDrive),
    // Offline-calibration constants
    ACIDUS_PARAM(cutoffBaseHz),
    ACIDUS_PARAM(cutoffSpanOct),
    ACIDUS_PARAM(cutoffTaperExp),
    ACIDUS_PARAM(envModOffsetOct),
    ACIDUS_PARAM(envModDepthOct),
    ACIDUS_PARAM(accentSweepDepthOct),
    ACIDUS_PARAM(accentVcaDepth),
    ACIDUS_PARAM(oscSawLpfHz),
    ACIDUS_PARAM(oscSawShape),
    ACIDUS_PARAM(vcfAttackMs),
    ACIDUS_PARAM(vcaAttackMs),
    ACIDUS_PARAM(vcfDecayMinSec),
    ACIDUS_PARAM(vcfDecayMaxSec),
    ACIDUS_PARAM(accentDecaySec),
    ACIDUS_PARAM(filterResonanceSkew),
    ACIDUS_PARAM(filterFeedbackHeadroomHz),
    ACIDUS_PARAM(filterResCouplingTrackHz),
};

#undef ACIDUS_PARAM

constexpr int kNumParams = static_cast<int>(sizeof(kParams) / sizeof(kParams[0]));

const SynthParameters kDefaults{};

} // namespace

ACIDUS_CALIB_API int acidus_calib_param_count() { return kNumParams; }

ACIDUS_CALIB_API const char* acidus_calib_param_name(int index) {
    return (index >= 0 && index < kNumParams) ? kParams[index].name : nullptr;
}

ACIDUS_CALIB_API double acidus_calib_param_default(int index) {
    return (index >= 0 && index < kNumParams) ? static_cast<double>(kDefaults.*(kParams[index].member)) : 0.0;
}

// Renders one note from silence: note-on at sample 0, note-off at
// gateSamples, totalSamples of mono output written to `out`.
//   values:   acidus_calib_param_count() parameter values, in table order
//   waveform: 0 = saw, 1 = square
//   accent:   non-zero renders an accented step (velocity 1.0 vs 0.5)
// Returns 0 on success, non-zero if the output contains NaN/Inf.
ACIDUS_CALIB_API int acidus_calib_render(const double* values, int waveform, int midiNote, int accent,
                                         double sampleRate, int gateSamples, int totalSamples, float* out) {
    acidus::SynthEngine engine;
    engine.setSampleRate(sampleRate);
    engine.reset();

    SynthParameters& p = engine.getParams();
    for (int i = 0; i < kNumParams; ++i) p.*(kParams[i].member) = static_cast<float>(values[i]);
    p.waveform = (waveform == 1) ? acidus::Waveform::Square : acidus::Waveform::Saw;

    // Push the parameters into the DSP blocks before the note starts.
    float scratch[1];
    engine.processAudio(scratch, nullptr, 0);

    engine.noteOn(midiNote, accent ? 1.0f : 0.5f);
    constexpr int kBlock = 64;
    int pos = 0;
    bool noteOffSent = false;
    while (pos < totalSamples) {
        if (!noteOffSent && pos >= gateSamples) {
            engine.noteOff(midiNote);
            noteOffSent = true;
        }
        int n = std::min(kBlock, totalSamples - pos);
        if (!noteOffSent && pos + n > gateSamples) n = gateSamples - pos;
        engine.processAudio(out + pos, nullptr, n);
        pos += n;
    }

    for (int i = 0; i < totalSamples; ++i) {
        if (!std::isfinite(out[i])) return 1;
    }
    return 0;
}
