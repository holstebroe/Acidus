#ifndef ACIDUS_CLAP_HPP
#define ACIDUS_CLAP_HPP

#include <clap/clap.h>
#include "core/SynthEngine.hpp"
#include <memory>
#include <vector>
#include <array>
#include <mutex>
#include <atomic>

namespace acidus {

struct GuiParamEvent {
    uint16_t type; // CLAP_EVENT_PARAM_GESTURE_BEGIN, CLAP_EVENT_PARAM_VALUE, CLAP_EVENT_PARAM_GESTURE_END
    clap_id paramId;
    double value;
    uint32_t flags; // e.g. CLAP_EVENT_IS_LIVE, CLAP_EVENT_DONT_RECORD
};

// Parameter IDs
//
// PARAM_CUTOFF..PARAM_TUNE (0-8) are the front-panel controls the plugin's
// GUI draws knobs/switches for (including the MXR Distortion+ emulation's
// Drive knob and the Tuning trim), and the only ones a Release build
// registers as CLAP params -- matching the real hardware's (plus the added
// pedal's) user-facing surface. PARAM_DRIVE/PARAM_TUNE are numbered
// contiguously with the other front-panel controls, rather than appended
// after the experimental block, so a single PARAM_COUNT threshold can gate
// "front-panel vs. experimental" cleanly for the calibration-build split
// below; this does mean a project saved against a version of this plugin
// with fewer front-panel ids will load with the newly-inserted ones (and
// anything after them) reset to their defaults -- an acceptable one-time
// cost for a knob that had just been introduced (same tradeoff already
// accepted when Drive was added).
//
// PARAM_OSC_COUPLING_HZ..PARAM_FILTER_OUTPUT_COUPLING_HZ (9+) are hidden
// circuit-topology/calibration constants (ladder coupling poles, VCA
// saturation drive, etc.). They are only registered as CLAP params -- under
// "Experimental/..." module paths, via a host's generic parameter list --
// in a build configured with -DACIDUS_CALIBRATION_BUILD=ON, so an external
// optimizer/fitting tool -- or a human A/B-ing against a reference hardware
// recording -- can drive every free constant in the model across its full
// documented plausible range (TB303_PARAMETER_CONFIDENCE.md). In a normal
// (Release) build they simply keep their in-code defaults; nothing in the
// DSP core depends on which build it is.
enum ParamId : clap_id {
    PARAM_CUTOFF = 0,
    PARAM_RESONANCE = 1,
    PARAM_ENV_MOD = 2,
    PARAM_DECAY = 3,
    PARAM_ACCENT = 4,
    PARAM_WAVEFORM = 5,
    PARAM_VOLUME = 6,

    // MXR Distortion+ emulation drive knob (front-panel control).
    // 0 = pedal bypassed (disabled).
    PARAM_DRIVE = 7,

    // Master tuning trim (front-panel control), ± cents. Range matches the
    // real hardware's documented Tuning trim travel (TB303_RESEARCH_
    // COMPENDIUM.md: "Tuning control range: approx. ±700 cents") -- lets the
    // plugin be nudged into tune against a reference hardware recording
    // that's itself slightly off-pitch, the same way the real trimmer would.
    PARAM_TUNE = 8,

    PARAM_FRONT_PANEL_COUNT = 9,

    // Experimental / calibration-only parameters (not on the plugin GUI).
    PARAM_OSC_COUPLING_HZ = 9,                  // Oscillator.hpp - plausible range 30-60 Hz
    PARAM_RES_COUPLING_HZ = 10,                 // Filter.hpp - plausible range 100-250 Hz
    PARAM_FILTER_FEEDBACK_GAIN = 11,            // Filter.hpp - plausible range 12-17
    PARAM_FILTER_POST_HP_HZ = 12,               // Filter.hpp - Open303 24 Hz; fitted 153 Hz (acidvoice) / 199 Hz (x0x)
    PARAM_FILTER_NOTCH_HZ = 13,                 // Filter.hpp - plausible range 4-15 Hz
    PARAM_FILTER_NOTCH_BANDWIDTH_HZ = 14,       // Filter.hpp - plausible range 2-10 Hz
    PARAM_FILTER_ALLPASS_HZ = 15,               // Filter.hpp - plausible range 8-25 Hz
    PARAM_VEG_DECAY_SEC = 16,                   // Envelope.hpp - R123 x C42 = 1.5 s; fitted 2.2-2.4 s
    PARAM_VCA_GATE_OFF_MS = 17,                 // Envelope.hpp - Open303 1 ms; fitted 0.7-1.1 ms
    PARAM_VCA_GATE_OFF_ACCENT_MS = 18,          // Envelope.hpp - plausible range 1-80 ms (widened 2026-09-20)
    PARAM_VCA_GAIN_SATURATION_DRIVE = 19,       // SynthEngine.cpp - plausible range 1-8
    PARAM_FILTER_INPUT_COUPLING_HZ = 20,        // Filter.hpp - plausible range 10-30 Hz
    PARAM_FILTER_OUTPUT_COUPLING_HZ = 21,       // Filter.hpp - plausible range 10-25 kHz
    PARAM_FILTER_CAP_SCALE_1 = 22,              // Filter.hpp - plausible range 0.2-4.0
    PARAM_FILTER_CAP_SCALE_2 = 23,              // Filter.hpp - plausible range 0.2-4.0
    PARAM_FILTER_CAP_SCALE_3 = 24,              // Filter.hpp - plausible range 0.2-4.0
    PARAM_FILTER_CAP_SCALE_4 = 25,              // Filter.hpp - plausible range 0.2-4.0
    PARAM_FILTER_LADDER_INPUT_SCALE = 26,       // Filter.hpp - plausible range 0.02-0.20
    PARAM_FILTER_RES_LIMIT = 27,                // Filter.hpp - max feedback as a fraction of the loop's critical gain
    PARAM_ACCENT_VCA_DEPTH = 28,                // SynthEngine.cpp - accent term in the VCA control sum
    PARAM_ACCENT_SWEEP_DEPTH = 29,              // SynthEngine.cpp - accent sweep depth into the cutoff, octaves
    PARAM_VCA_RES_TAP_RATIO = 30,               // SynthEngine.cpp - filter->VCA wiper tap vs fixed tap (Resonance level balance)
    PARAM_FILTER_LADDER_TOPOLOGY = 31,          // Filter.hpp - 0 = legacy mirrored ladder, 1 = circuit orientation
    // Cutoff knob law (knob -> Hz before the envelope): base * 2^(span * knob^taper).
    // The base is the unit's TM3 cutoff trim -- the main difference between
    // calibrations/acidvoice.json (248 Hz) and calibrations/x0x.json (159 Hz).
    PARAM_CUTOFF_BASE_HZ = 32,                  // SynthEngine.cpp - cutoff at knob minimum
    PARAM_CUTOFF_SPAN_OCT = 33,                 // SynthEngine.cpp - octaves swept by the Cutoff knob
    PARAM_CUTOFF_TAPER_EXP = 34,                // SynthEngine.cpp - knob taper, 1 = exponential knob-to-Hz
    PARAM_FILTER_RES_SKEW = 35,                 // Filter.hpp - Resonance pot curve; < 0 builds late in the travel
    // Env Mod law (SynthEngine.cpp): depth = (1-c)*(C0 + C0Slope*e) + c*(C1 + C1Slope*e)
    // octaves per unit MEG, cutoff shift = depth * (MEG - (Offset + OffsetCutSlope*c)),
    // and the MEG / accent-sweep time constants (Envelope.cpp).
    PARAM_ENV_MOD_SCALE_C0 = 36,           // sweep depth (oct per unit MEG) at Env Mod 0, Cutoff min
    PARAM_ENV_MOD_SCALE_C0_SLOPE = 37,     // added depth per unit Env Mod, Cutoff min
    PARAM_ENV_MOD_SCALE_C1 = 38,           // sweep depth at Env Mod 0, Cutoff max
    PARAM_ENV_MOD_SCALE_C1_SLOPE = 39,     // added depth per unit Env Mod, Cutoff max
    PARAM_ENV_MOD_OFFSET = 40,             // MEG level where the Env Mod bias shift is neutral (floor drop = depth x offset)
    PARAM_ENV_MOD_OFFSET_CUT_SLOPE = 41,   // offset change at Cutoff max
    PARAM_VCF_DECAY_MIN_SEC = 42,          // MEG decay tau at Decay min
    PARAM_VCF_DECAY_MAX_SEC = 43,          // MEG decay tau at Decay max
    PARAM_ACCENT_DECAY_SEC = 44,           // MEG decay tau on accented notes
    PARAM_ACCENT_CHARGE_BASE_SEC = 45,     // accent sweep: R46 x C13
    PARAM_ACCENT_CHARGE_POT_SEC = 46,      // accent sweep: VR4b x C13, scaled by Resonance
    PARAM_ACCENT_MIX_SEC = 47,             // accent sweep: mixing resistor x C13

    PARAM_ENV_MOD_TAPER_EXP = 48,              // Env Mod pot taper, knob^exp (1 = linear)
    PARAM_ACCENT_DIODE_DROP = 49,              // D24 forward drop, fraction of the MEG swing (0 = ideal)
    PARAM_VCF_DECAY_TAPER = 50,                // Decay pot taper a, R = Rtot*(a^x-1)/(a-1) (81 = 10 % at mid-travel)
    PARAM_ENV_MOD_TAPER_MID = 51,              // logistic Env Mod taper mid-point
    PARAM_ENV_MOD_TAPER_WIDTH = 52,            // logistic Env Mod taper width (0 = power law)
    PARAM_CUTOFF_MAX_HZ = 53,                  // ceiling of the cutoff CV
    PARAM_VCA_NORMAL_DELAY_MS = 54,            // VCA onset delay on unaccented notes
    PARAM_VCA_ATTACK_MS = 55,                  // VEG onset time constant (Devil Fish: Soft Attack)

    PARAM_VCO_OCTAVE_SCALE = 56,               // VCO V/oct scale (TM5 width), 1 = exact 2:1 octaves

    PARAM_FILTER_COUPLING_NETWORK = 57,        // Filter.hpp - 0 = Open303 empirical coupling, 1 = Stinchcombe's full network
    PARAM_FILTER_NETWORK_TIME_SCALE = 58,      // Filter.hpp - Stinchcombe network RC time-constant scale

    PARAM_EXPERIMENTAL_COUNT = 59,

#ifdef ACIDUS_CALIBRATION_BUILD
    PARAM_COUNT = PARAM_EXPERIMENTAL_COUNT
#else
    PARAM_COUNT = PARAM_FRONT_PANEL_COUNT
#endif
};

enum MidiParamId : clap_id {
    MIDI_PARAM_CUTOFF = 71,
    MIDI_PARAM_RESONANCE = 72,
    MIDI_PARAM_ENV_MOD = 73,
    MIDI_PARAM_DECAY = 74,
    MIDI_PARAM_ACCENT = 22,
    MIDI_PARAM_WAVEFORM = 23,
    MIDI_PARAM_VOLUME = 20,
    MIDI_PARAM_DRIVE = 21,
    MIDI_PARAM_TUNE = 24,
    MIDI_PARAM_COUNT = 9
};

class AcidusClap {
public:
    explicit AcidusClap(const clap_host_t* host);
    ~AcidusClap() = default;

    const clap_plugin_t* getClapPlugin() const { return &clapPlugin_; }

    // CLAP Core Callbacks
    bool init();
    void destroy();
    bool activate(double sampleRate, uint32_t minFrames, uint32_t maxFrames);
    void deactivate();
    bool startProcessing();
    void stopProcessing();
    void reset();
    clap_process_status process(const clap_process_t* process);
    const void* getExtension(const char* id);
    void onMainThread();

    // CLAP Params Extension
    uint32_t paramsCount() const;
    bool paramsInfo(uint32_t paramIndex, clap_param_info_t* paramInfo) const;
    bool paramsValue(clap_id paramId, double* outValue);
    bool paramsDefaultValue(clap_id paramId, double* outValue) const;
    void setParamValueFromGui(clap_id paramId, double value);
    void onBeginEditFromGui(clap_id paramId);
    void onParamValueFromGui(clap_id paramId, double value);
    void onEndEditFromGui(clap_id paramId);
    // Calibration presets (src/core/CalibrationPresets.hpp): selecting one
    // loads its calibration constants; the front-panel knobs are untouched.
    // In a calibration build the constants are CLAP parameters, the host is
    // told about each one, and editing any of them afterwards makes
    // isCalibrationModified() true.
    static int calibrationPresetCount();
    int calibrationPresetIndex() const { return calibrationPreset_.load(); }
    const char* calibrationPresetName() const;
    bool isCalibrationModified() const;
    void selectCalibrationPreset(int index, bool notifyHost);
    void cycleCalibrationPresetFromGui();
    bool paramsValueToText(clap_id paramId, double value, char* outBuffer, uint32_t outBufferCapacity);
    bool paramsTextToValue(clap_id paramId, const char* paramValueText, double* outValue);
    void paramsFlush(const clap_input_events_t* in, const clap_output_events_t* out);
    void pushPendingOutputEvents(const clap_output_events_t* out);
    void requestHostFlush();

    // CLAP State Extension
    bool stateSave(const clap_ostream_t* stream);
    bool stateLoad(const clap_istream_t* stream);

    SynthEngine& getEngine() { return engine_; }

    // For the GUI: number of accented note-ons so far (wraps), and the
    // position in the host's current bar (0..1), extrapolated from the last
    // processed block. transportBarPhase() returns false when the host is
    // stopped or sends no tempo/beat timeline.
    uint32_t accentCount() const { return accentCount_.load(std::memory_order_relaxed); }
    bool transportBarPhase(double& phase) const;

    const clap_host_t* getHost() const { return host_; }

    class GuiWindow* getGuiWindow() { return guiWindow_.get(); }
    void createGuiWindow();
    void destroyGuiWindow();

private:
    const clap_host_t* host_{nullptr};
    clap_plugin_t clapPlugin_{};
    SynthEngine engine_;
    std::unique_ptr<class GuiWindow> guiWindow_;

    // Written by the audio thread (host automation, MIDI CC) and the GUI
    // thread, read by both: atomic so a read is never torn.
    std::atomic<double> paramValues_[PARAM_COUNT];
    double paramMin_[PARAM_COUNT]{};
    double paramMax_[PARAM_COUNT]{};
    double paramDefault_[PARAM_COUNT]{};
    double getParam(clap_id id) const { return paramValues_[id].load(std::memory_order_relaxed); }
    // Clamps to the parameter's range; NaN/Inf fall back to the default.
    double sanitizeParam(clap_id id, double v) const;
    void setParam(clap_id id, double v) { paramValues_[id].store(sanitizeParam(id, v), std::memory_order_relaxed); }
    void resetParamsToDefaults();
    SynthParameters buildParams() const;
    std::atomic<int> calibrationPreset_{0};
    std::atomic<bool> stateDirty_{false};
    std::atomic<uint32_t> accentCount_{0};
    // Host transport at the start of the last block (see transportBarPhase).
    std::atomic<bool> transportPlaying_{false};
    std::atomic<double> transportBarPos_{0.0};      // fraction of the bar
    std::atomic<double> transportBarsPerSec_{0.0};
    std::atomic<int64_t> transportStampNs_{0};      // steady_clock time of that block
    void noteOnFromHost(int key, float velocity);
    void notePressureFromHost(int key, float pressure);

    // GUI/preset -> host events, drained by the audio thread. Fixed capacity
    // (no allocation on either side, bounded when the host is not processing);
    // events beyond it are dropped.
    static constexpr size_t kOutQueueCapacity = 1024;
    std::mutex outEventQueueMutex_;
    std::array<GuiParamEvent, kOutQueueCapacity> outEventQueue_;
    size_t outEventCount_{0};
    std::array<GuiParamEvent, kOutQueueCapacity> drainBuffer_;   // audio-thread scratch
    void queueOutEvent(const GuiParamEvent& ev, bool audioThread);

    void handleEvent(const clap_event_header_t* header);
    void syncParamsToEngine();
};

} // namespace acidus

#endif // ACIDUS_CLAP_HPP
