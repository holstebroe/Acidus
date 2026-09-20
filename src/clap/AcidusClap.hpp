#ifndef ACIDUS_CLAP_HPP
#define ACIDUS_CLAP_HPP

#include <clap/clap.h>
#include "core/SynthEngine.hpp"
#include <memory>
#include <vector>
#include <mutex>

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
    PARAM_FILTER_POST_HP_HZ = 12,               // Filter.hpp - plausible range 15-35 Hz
    PARAM_FILTER_NOTCH_HZ = 13,                 // Filter.hpp - plausible range 4-15 Hz
    PARAM_FILTER_NOTCH_BANDWIDTH_HZ = 14,       // Filter.hpp - plausible range 2-10 Hz
    PARAM_FILTER_ALLPASS_HZ = 15,               // Filter.hpp - plausible range 8-25 Hz
    PARAM_VEG_DECAY_SEC = 16,                   // Envelope.hpp - plausible range 2.5-5.0 s
    PARAM_VCA_GATE_OFF_MS = 17,                 // Envelope.hpp - plausible range 1-5 ms
    PARAM_VCA_GATE_OFF_ACCENT_MS = 18,          // Envelope.hpp - plausible range 1-80 ms (widened 2026-09-20)
    PARAM_VCA_GAIN_SATURATION_DRIVE = 19,       // SynthEngine.cpp - plausible range 1-8
    PARAM_FILTER_INPUT_COUPLING_HZ = 20,        // Filter.hpp - plausible range 10-30 Hz
    PARAM_FILTER_OUTPUT_COUPLING_HZ = 21,       // Filter.hpp - plausible range 10-25 kHz
    PARAM_FILTER_CAP_SCALE_1 = 22,              // Filter.hpp - plausible range 0.2-4.0
    PARAM_FILTER_CAP_SCALE_2 = 23,              // Filter.hpp - plausible range 0.2-4.0
    PARAM_FILTER_CAP_SCALE_3 = 24,              // Filter.hpp - plausible range 0.2-4.0
    PARAM_FILTER_CAP_SCALE_4 = 25,              // Filter.hpp - plausible range 0.2-4.0

    PARAM_EXPERIMENTAL_COUNT = 26,

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
    void setParamValueFromGui(clap_id paramId, double value);
    void onBeginEditFromGui(clap_id paramId);
    void onParamValueFromGui(clap_id paramId, double value);
    void onEndEditFromGui(clap_id paramId);
    bool paramsValueToText(clap_id paramId, double value, char* outBuffer, uint32_t outBufferCapacity);
    bool paramsTextToValue(clap_id paramId, const char* paramValueText, double* outValue);
    void paramsFlush(const clap_input_events_t* in, const clap_output_events_t* out);
    void pushPendingOutputEvents(const clap_output_events_t* out);
    void requestHostFlush();

    // CLAP State Extension
    bool stateSave(const clap_ostream_t* stream);
    bool stateLoad(const clap_istream_t* stream);

    SynthEngine& getEngine() { return engine_; }

    const clap_host_t* getHost() const { return host_; }

    class GuiWindow* getGuiWindow() { return guiWindow_.get(); }
    void createGuiWindow();
    void destroyGuiWindow();

private:
    const clap_host_t* host_{nullptr};
    clap_plugin_t clapPlugin_{};
    SynthEngine engine_;
    std::unique_ptr<class GuiWindow> guiWindow_;

    double paramValues_[PARAM_COUNT]{};

    std::mutex outEventQueueMutex_;
    std::vector<GuiParamEvent> outEventQueue_;

    void handleEvent(const clap_event_header_t* header);
    void syncParamsToEngine();
};

} // namespace acidus

#endif // ACIDUS_CLAP_HPP
