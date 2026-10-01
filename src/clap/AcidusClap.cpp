#include "AcidusClap.hpp"
#include "gui/GuiWindow.hpp"
#include "core/CalibrationPresets.hpp"
#include <clap/ext/state.h>
#include <cstring>
#include <cstdio>
#include <algorithm>
#include <cmath>
#include <chrono>
#include <cstdlib>
#if defined(__SSE__)
#include <xmmintrin.h>
#endif

namespace acidus {

// The plugin's calibration constants come from the selected calibration
// preset (src/core/CalibrationPresets.hpp, generated from calibrations/*.json).
// The first preset is the startup calibration and the calibration
// parameters' CLAP defaults.
[[maybe_unused]] static const SynthParameters& kCalibrationDefaults = calibrationPresets()[0].params;

#ifdef ACIDUS_CALIBRATION_BUILD
// Which SynthParameters field each calibration CLAP parameter controls.
struct CalibrationBinding {
    clap_id id;
    float SynthParameters::*field;
};
static const CalibrationBinding kCalibrationBindings[] = {
    { PARAM_OSC_COUPLING_HZ, &SynthParameters::oscCouplingHz },
    { PARAM_RES_COUPLING_HZ, &SynthParameters::resCouplingHz },
    { PARAM_FILTER_FEEDBACK_GAIN, &SynthParameters::filterFeedbackGain },
    { PARAM_FILTER_POST_HP_HZ, &SynthParameters::filterPostHpHz },
    { PARAM_FILTER_NOTCH_HZ, &SynthParameters::filterNotchHz },
    { PARAM_FILTER_NOTCH_BANDWIDTH_HZ, &SynthParameters::filterNotchBandwidthHz },
    { PARAM_FILTER_ALLPASS_HZ, &SynthParameters::filterAllpassHz },
    { PARAM_VEG_DECAY_SEC, &SynthParameters::vegDecaySec },
    { PARAM_VCA_GATE_OFF_MS, &SynthParameters::vcaGateOffMs },
    { PARAM_VCA_GATE_OFF_ACCENT_MS, &SynthParameters::vcaGateOffAccentMs },
    { PARAM_VCA_GAIN_SATURATION_DRIVE, &SynthParameters::vcaGainSaturationDrive },
    { PARAM_FILTER_INPUT_COUPLING_HZ, &SynthParameters::filterInputCouplingHz },
    { PARAM_FILTER_OUTPUT_COUPLING_HZ, &SynthParameters::filterOutputCouplingHz },
    { PARAM_FILTER_CAP_SCALE_1, &SynthParameters::filterCapScale1 },
    { PARAM_FILTER_CAP_SCALE_2, &SynthParameters::filterCapScale2 },
    { PARAM_FILTER_CAP_SCALE_3, &SynthParameters::filterCapScale3 },
    { PARAM_FILTER_CAP_SCALE_4, &SynthParameters::filterCapScale4 },
    { PARAM_FILTER_LADDER_INPUT_SCALE, &SynthParameters::filterLadderInputScale },
    { PARAM_FILTER_RES_LIMIT, &SynthParameters::filterResonanceLimit },
    { PARAM_ACCENT_VCA_DEPTH, &SynthParameters::accentVcaDepth },
    { PARAM_ACCENT_SWEEP_DEPTH, &SynthParameters::accentSweepDepthOct },
    { PARAM_VCA_RES_TAP_RATIO, &SynthParameters::vcaResTapRatio },
    { PARAM_VCO_OCTAVE_SCALE, &SynthParameters::vcoOctaveScale },
    { PARAM_FILTER_LADDER_TOPOLOGY, &SynthParameters::filterLadderTopology },
    { PARAM_CUTOFF_BASE_HZ, &SynthParameters::cutoffBaseHz },
    { PARAM_CUTOFF_SPAN_OCT, &SynthParameters::cutoffSpanOct },
    { PARAM_CUTOFF_TAPER_EXP, &SynthParameters::cutoffTaperExp },
    { PARAM_FILTER_RES_SKEW, &SynthParameters::filterResonanceSkew },
    { PARAM_ENV_MOD_SCALE_C0, &SynthParameters::envModScaleC0 },
    { PARAM_ENV_MOD_SCALE_C0_SLOPE, &SynthParameters::envModScaleC0Slope },
    { PARAM_ENV_MOD_SCALE_C1, &SynthParameters::envModScaleC1 },
    { PARAM_ENV_MOD_SCALE_C1_SLOPE, &SynthParameters::envModScaleC1Slope },
    { PARAM_ENV_MOD_OFFSET, &SynthParameters::envModOffset },
    { PARAM_ENV_MOD_OFFSET_CUT_SLOPE, &SynthParameters::envModOffsetCutSlope },
    { PARAM_VCF_DECAY_MIN_SEC, &SynthParameters::vcfDecayMinSec },
    { PARAM_VCF_DECAY_MAX_SEC, &SynthParameters::vcfDecayMaxSec },
    { PARAM_ACCENT_DECAY_SEC, &SynthParameters::accentDecaySec },
    { PARAM_ACCENT_CHARGE_BASE_SEC, &SynthParameters::accentChargeBaseSec },
    { PARAM_ACCENT_CHARGE_POT_SEC, &SynthParameters::accentChargePotSec },
    { PARAM_ACCENT_MIX_SEC, &SynthParameters::accentMixSec },
    { PARAM_ENV_MOD_TAPER_EXP, &SynthParameters::envModTaperExp },
    { PARAM_ACCENT_DIODE_DROP, &SynthParameters::accentDiodeDrop },
    { PARAM_VCF_DECAY_TAPER, &SynthParameters::vcfDecayTaper },
    { PARAM_ENV_MOD_TAPER_MID, &SynthParameters::envModTaperMid },
    { PARAM_ENV_MOD_TAPER_WIDTH, &SynthParameters::envModTaperWidth },
    { PARAM_CUTOFF_MAX_HZ, &SynthParameters::cutoffMaxHz },
    { PARAM_VCA_NORMAL_DELAY_MS, &SynthParameters::vcaNormalDelayMs },
    { PARAM_VCA_ATTACK_MS, &SynthParameters::vcaAttackMs },
};
static_assert(sizeof(kCalibrationBindings) / sizeof(kCalibrationBindings[0])
                  == PARAM_EXPERIMENTAL_COUNT - PARAM_FRONT_PANEL_COUNT,
              "every calibration CLAP parameter needs a SynthParameters binding");
#endif

// A C++ exception must never cross the plugin boundary into the host: every
// callback the host can reach runs through one of these.
template <class R, class F>
static R guarded(R fallback, F&& f) noexcept {
    try { return f(); } catch (...) { return fallback; }
}
template <class F>
static void guardedVoid(F&& f) noexcept {
    try { f(); } catch (...) {}
}

#if defined(__SSE__)
// Flush denormals to zero while processing (filter/envelope tails decay into
// the denormal range, which can cost orders of magnitude in CPU time).
struct ScopedFlushDenormals {
    unsigned saved = _mm_getcsr();
    ScopedFlushDenormals() { _mm_setcsr(saved | 0x8040u); }
    ~ScopedFlushDenormals() { _mm_setcsr(saved); }
};
#else
struct ScopedFlushDenormals {};
#endif

// Forward declarations of GUI extension functions
extern const clap_plugin_gui_t g_acidusGuiExtension;

static const clap_plugin_note_ports_t g_notePortsExtension = {
    // count
    [](const clap_plugin_t* plugin, bool is_input) -> uint32_t {
        return is_input ? 1 : 0;
    },
    // get
    [](const clap_plugin_t* plugin, uint32_t index, bool is_input, clap_note_port_info_t* info) -> bool {
        if (!is_input || index != 0 || !info) return false;
        info->id = 0;
        snprintf(info->name, sizeof(info->name), "MIDI Note Input");
        info->supported_dialects = CLAP_NOTE_DIALECT_CLAP | CLAP_NOTE_DIALECT_MIDI;
        info->preferred_dialect = CLAP_NOTE_DIALECT_CLAP;
        return true;
    }
};

static const clap_plugin_audio_ports_t g_audioPortsExtension = {
    // count
    [](const clap_plugin_t* plugin, bool is_input) -> uint32_t {
        return is_input ? 0 : 1;
    },
    // get
    [](const clap_plugin_t* plugin, uint32_t index, bool is_input, clap_audio_port_info_t* info) -> bool {
        if (is_input || index != 0 || !info) return false;
        info->id = 0;
        snprintf(info->name, sizeof(info->name), "Audio Output");
        info->channel_count = 2;
        info->flags = CLAP_AUDIO_PORT_IS_MAIN;
        info->port_type = CLAP_PORT_STEREO;
        info->in_place_pair = CLAP_INVALID_ID;
        return true;
    }
};

static const clap_plugin_params_t g_paramsExtension = {
    // count
    [](const clap_plugin_t* plugin) -> uint32_t {
        auto* self = static_cast<AcidusClap*>(plugin->plugin_data);
        return guarded<uint32_t>(0, [&] { return self->paramsCount(); });
    },
    // get_info
    [](const clap_plugin_t* plugin, uint32_t param_index, clap_param_info_t* param_info) -> bool {
        auto* self = static_cast<AcidusClap*>(plugin->plugin_data);
        if (!param_info) return false;
        return guarded(false, [&] { return self->paramsInfo(param_index, param_info); });
    },
    // get_value
    [](const clap_plugin_t* plugin, clap_id param_id, double* out_value) -> bool {
        auto* self = static_cast<AcidusClap*>(plugin->plugin_data);
        return guarded(false, [&] { return self->paramsValue(param_id, out_value); });
    },
    // value_to_text
    [](const clap_plugin_t* plugin, clap_id param_id, double value, char* out_buffer, uint32_t out_buffer_capacity) -> bool {
        auto* self = static_cast<AcidusClap*>(plugin->plugin_data);
        return guarded(false, [&] { return self->paramsValueToText(param_id, value, out_buffer, out_buffer_capacity); });
    },
    // text_to_value
    [](const clap_plugin_t* plugin, clap_id param_id, const char* param_value_text, double* out_value) -> bool {
        auto* self = static_cast<AcidusClap*>(plugin->plugin_data);
        return guarded(false, [&] { return self->paramsTextToValue(param_id, param_value_text, out_value); });
    },
    // flush
    [](const clap_plugin_t* plugin, const clap_input_events_t* in, const clap_output_events_t* out) {
        auto* self = static_cast<AcidusClap*>(plugin->plugin_data);
        guardedVoid([&] { ScopedFlushDenormals ftz; self->paramsFlush(in, out); });
    }
};

static const clap_plugin_state_t g_stateExtension = {
    // save
    [](const clap_plugin_t* plugin, const clap_ostream_t* stream) -> bool {
        auto* self = static_cast<AcidusClap*>(plugin->plugin_data);
        return guarded(false, [&] { return self->stateSave(stream); });
    },
    // load
    [](const clap_plugin_t* plugin, const clap_istream_t* stream) -> bool {
        auto* self = static_cast<AcidusClap*>(plugin->plugin_data);
        return guarded(false, [&] { return self->stateLoad(stream); });
    }
};

AcidusClap::AcidusClap(const clap_host_t* host) : host_(host) {
    clapPlugin_.desc = nullptr;
    clapPlugin_.plugin_data = this;
    clapPlugin_.init = [](const clap_plugin_t* plugin) -> bool {
        return guarded(false, [&] { return static_cast<AcidusClap*>(plugin->plugin_data)->init(); });
    };
    clapPlugin_.destroy = [](const clap_plugin_t* plugin) {
        guardedVoid([&] { static_cast<AcidusClap*>(plugin->plugin_data)->destroy(); });
    };
    clapPlugin_.activate = [](const clap_plugin_t* plugin, double sample_rate, uint32_t min_frames, uint32_t max_frames) -> bool {
        return guarded(false, [&] { return static_cast<AcidusClap*>(plugin->plugin_data)->activate(sample_rate, min_frames, max_frames); });
    };
    clapPlugin_.deactivate = [](const clap_plugin_t* plugin) {
        guardedVoid([&] { static_cast<AcidusClap*>(plugin->plugin_data)->deactivate(); });
    };
    clapPlugin_.start_processing = [](const clap_plugin_t* plugin) -> bool {
        return guarded(false, [&] { return static_cast<AcidusClap*>(plugin->plugin_data)->startProcessing(); });
    };
    clapPlugin_.stop_processing = [](const clap_plugin_t* plugin) {
        guardedVoid([&] { static_cast<AcidusClap*>(plugin->plugin_data)->stopProcessing(); });
    };
    clapPlugin_.reset = [](const clap_plugin_t* plugin) {
        guardedVoid([&] { static_cast<AcidusClap*>(plugin->plugin_data)->reset(); });
    };
    clapPlugin_.process = [](const clap_plugin_t* plugin, const clap_process_t* process) -> clap_process_status {
        return guarded<clap_process_status>(CLAP_PROCESS_CONTINUE, [&] { return static_cast<AcidusClap*>(plugin->plugin_data)->process(process); });
    };
    clapPlugin_.get_extension = [](const clap_plugin_t* plugin, const char* id) -> const void* {
        return guarded<const void*>(nullptr, [&] { return static_cast<AcidusClap*>(plugin->plugin_data)->getExtension(id); });
    };
    clapPlugin_.on_main_thread = [](const clap_plugin_t* plugin) {
        guardedVoid([&] { static_cast<AcidusClap*>(plugin->plugin_data)->onMainThread(); });
    };

    // Range/default tables (from paramsInfo, the single source of truth),
    // then every parameter at its default.
    for (uint32_t i = 0; i < PARAM_COUNT; ++i) {
        clap_param_info_t info;
        if (paramsInfo(i, &info)) {
            paramMin_[i] = info.min_value;
            paramMax_[i] = info.max_value;
            paramDefault_[i] = info.default_value;
        }
    }
    resetParamsToDefaults();

    syncParamsToEngine();
}

bool AcidusClap::init() {
    return true;
}

void AcidusClap::destroy() {
    destroyGuiWindow();
    delete this;
}

void AcidusClap::createGuiWindow() {
    if (!guiWindow_) {
        guiWindow_ = std::make_unique<GuiWindow>(this);
    }
}

void AcidusClap::destroyGuiWindow() {
    guiWindow_.reset();
}

double AcidusClap::sanitizeParam(clap_id id, double v) const {
    if (id >= PARAM_COUNT) return 0.0;
    if (!std::isfinite(v)) return paramDefault_[id];
    return std::min(std::max(v, paramMin_[id]), paramMax_[id]);
}

void AcidusClap::resetParamsToDefaults() {
    for (uint32_t i = 0; i < PARAM_COUNT; ++i) {
        paramValues_[i].store(paramDefault_[i], std::memory_order_relaxed);
    }
}

bool AcidusClap::activate(double sampleRate, uint32_t minFrames, uint32_t maxFrames) {
    engine_.setSampleRate(sampleRate);   // rejects a zero/negative/non-finite rate
    engine_.reset();
    return true;
}

void AcidusClap::deactivate() {}

bool AcidusClap::startProcessing() {
    return true;
}

void AcidusClap::stopProcessing() {}

void AcidusClap::reset() {
    engine_.reset();
}

SynthParameters AcidusClap::buildParams() const {
    // Calibration constants from the selected preset; the front-panel fields
    // it also carries are overwritten from paramValues_ below. Built in a
    // copy so the engine never sees the preset's default knob values.
    SynthParameters params = calibrationPresets()[calibrationPreset_.load()].params;
    params.cutoff = static_cast<float>(getParam(PARAM_CUTOFF));
    params.resonance = static_cast<float>(getParam(PARAM_RESONANCE));
    params.envMod = static_cast<float>(getParam(PARAM_ENV_MOD));
    params.decay = static_cast<float>(getParam(PARAM_DECAY));
    params.accent = static_cast<float>(getParam(PARAM_ACCENT));
    params.waveform = (getParam(PARAM_WAVEFORM) >= 0.5) ? Waveform::Square : Waveform::Saw;
    params.masterVolume = static_cast<float>(getParam(PARAM_VOLUME));

#ifdef ACIDUS_CALIBRATION_BUILD
    // Calibration build: the host-visible calibration parameters override
    // the preset (they start equal to it; a difference is a user edit).
    for (const auto& b : kCalibrationBindings) {
        params.*(b.field) = static_cast<float>(getParam(b.id));
    }
#endif

    params.drive = static_cast<float>(getParam(PARAM_DRIVE));
    params.tuningCents = static_cast<float>(getParam(PARAM_TUNE));
    return params;
}

void AcidusClap::syncParamsToEngine() {
    // The audio thread works from a sanitized snapshot of these (SynthEngine),
    // so a concurrent write cannot reach the DSP in an unsafe state.
    engine_.getParams() = buildParams();
}

void AcidusClap::handleEvent(const clap_event_header_t* header) {
    if (!header || header->size < sizeof(clap_event_header_t)) return;
    if (header->space_id != CLAP_CORE_EVENT_SPACE_ID) return;

    if (header->type == CLAP_EVENT_NOTE_ON) {
        if (header->size < sizeof(clap_event_note_t)) return;
        const auto* noteEv = reinterpret_cast<const clap_event_note_t*>(header);
        noteOnFromHost(noteEv->key, static_cast<float>(noteEv->velocity));
    } else if (header->type == CLAP_EVENT_NOTE_OFF) {
        if (header->size < sizeof(clap_event_note_t)) return;
        const auto* noteEv = reinterpret_cast<const clap_event_note_t*>(header);
        engine_.noteOff(noteEv->key);
    } else if (header->type == CLAP_EVENT_MIDI) {
        if (header->size < sizeof(clap_event_midi_t)) return;
        const auto* midiEv = reinterpret_cast<const clap_event_midi_t*>(header);
        uint8_t status = midiEv->data[0] & 0xF0;
        uint8_t data1 = midiEv->data[1];
        uint8_t data2 = midiEv->data[2];
        if (data1 > 127 || data2 > 127) return;   // malformed MIDI data byte

        if (status == 0x90 && data2 > 0) {
            noteOnFromHost(data1, static_cast<float>(data2) / 127.0f);
        } else if (status == 0x80 || (status == 0x90 && data2 == 0)) {
            engine_.noteOff(data1);
        } else if (status == 0xB0) {
            // MIDI Control Change
            clap_id paramId = PARAM_COUNT;
            if (data1 == MIDI_PARAM_CUTOFF) paramId = PARAM_CUTOFF;
            else if (data1 == MIDI_PARAM_RESONANCE) paramId = PARAM_RESONANCE;
            else if (data1 == MIDI_PARAM_ENV_MOD) paramId = PARAM_ENV_MOD;
            else if (data1 == MIDI_PARAM_DECAY) paramId = PARAM_DECAY;
            else if (data1 == MIDI_PARAM_ACCENT) paramId = PARAM_ACCENT;
            else if (data1 == MIDI_PARAM_WAVEFORM) paramId = PARAM_WAVEFORM;
            else if (data1 == MIDI_PARAM_VOLUME) paramId = PARAM_VOLUME;
            else if (data1 == MIDI_PARAM_DRIVE) paramId = PARAM_DRIVE;
            else if (data1 == MIDI_PARAM_TUNE) paramId = PARAM_TUNE;

            if (paramId < PARAM_COUNT) {
                double normVal = static_cast<double>(data2) / 127.0;
                if (paramId == PARAM_WAVEFORM) {
                    normVal = (data2 >= 64) ? 1.0 : 0.0;
                } else if (paramId == PARAM_TUNE) {
                    // CC 64 is exactly centre (0 cents): 0-64 spans the
                    // lower half and 64-127 the upper, so the trim can be
                    // returned to in tune from a hardware controller.
                    normVal = (data2 <= 64) ? -700.0 + (data2 / 64.0) * 700.0
                                            : ((data2 - 64) / 63.0) * 700.0;
                }
                setParam(paramId, normVal);
                syncParamsToEngine();
                // Runs on the audio thread: the event goes out at the end of
                // this block (pushPendingOutputEvents), no host flush request.
                queueOutEvent({ CLAP_EVENT_PARAM_VALUE, paramId, getParam(paramId), CLAP_EVENT_DONT_RECORD }, true);
            }
        }
    } else if (header->type == CLAP_EVENT_PARAM_VALUE) {
        if (header->size < sizeof(clap_event_param_value_t)) return;
        const auto* paramEv = reinterpret_cast<const clap_event_param_value_t*>(header);
        if (paramEv->param_id < PARAM_COUNT) {
            setParam(paramEv->param_id, paramEv->value);
            syncParamsToEngine();
        }
    }
}

void AcidusClap::noteOnFromHost(int key, float velocity) {
    if (key < 0 || key > 127) return;   // wildcard/out-of-range keys carry no pitch
    if (!std::isfinite(velocity)) velocity = 1.0f;
    velocity = std::min(std::max(velocity, 0.0f), 1.0f);
    engine_.noteOn(key, velocity);
    if (velocity >= SynthEngine::kAccentVelocity) {
        accentCount_.fetch_add(1, std::memory_order_relaxed);
    }
}

bool AcidusClap::transportBarPhase(double& phase) const {
    if (!transportPlaying_.load()) return false;
    const int64_t now = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    const double elapsed = 1e-9 * static_cast<double>(now - transportStampNs_.load());
    if (elapsed < 0.0 || elapsed > 0.5) return false;   // no blocks lately: not playing
    const double p = transportBarPos_.load() + elapsed * transportBarsPerSec_.load();
    if (!std::isfinite(p)) return false;
    phase = p - std::floor(p);
    return true;
}

clap_process_status AcidusClap::process(const clap_process_t* process) {
    if (!process) return CLAP_PROCESS_CONTINUE;
    ScopedFlushDenormals flushDenormals;

    // Host transport, for the GUI's bar-synced logo pulse.
    const clap_event_transport_t* tr = process->transport;
    constexpr uint32_t kNeeded = CLAP_TRANSPORT_HAS_TEMPO | CLAP_TRANSPORT_HAS_BEATS_TIMELINE
                               | CLAP_TRANSPORT_IS_PLAYING;
    if (tr && (tr->flags & kNeeded) == kNeeded && std::isfinite(tr->tempo) && tr->tempo > 0.0
        && tr->tempo < 10000.0) {
        const double beatsPerBar = tr->tsig_num > 0 ? static_cast<double>(tr->tsig_num) : 4.0;
        const double inBar = static_cast<double>(tr->song_pos_beats - tr->bar_start)
                             / static_cast<double>(CLAP_BEATTIME_FACTOR);
        const double pos = inBar / beatsPerBar;
        transportBarPos_.store(std::isfinite(pos) ? pos - std::floor(pos) : 0.0);
        transportBarsPerSec_.store(tr->tempo / 60.0 / beatsPerBar);
        transportStampNs_.store(std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());
        transportPlaying_.store(true);
    } else {
        transportPlaying_.store(false);
    }

    const uint32_t numFrames = process->frames_count;
    const uint32_t numEvents = process->in_events ? process->in_events->size(process->in_events) : 0;
    uint32_t eventIndex = 0;

    // The host may pass no output, or a 64-bit-only one; events are still
    // handled so state stays consistent.
    const bool haveOut = process->audio_outputs && process->audio_outputs_count > 0
                         && process->audio_outputs[0].data32;
    float* outL = (haveOut && process->audio_outputs[0].channel_count > 0)
                      ? process->audio_outputs[0].data32[0]
                      : nullptr;
    float* outR = (haveOut && process->audio_outputs[0].channel_count > 1)
                      ? process->audio_outputs[0].data32[1]
                      : nullptr;

    for (uint32_t frame = 0; frame < numFrames; ) {
        while (eventIndex < numEvents) {
            const clap_event_header_t* hdr = process->in_events->get(process->in_events, eventIndex);
            if (!hdr) { eventIndex++; continue; }
            if (hdr->time > frame) break;
            handleEvent(hdr);
            eventIndex++;
        }

        uint32_t nextEventFrame = numFrames;
        if (eventIndex < numEvents) {
            const clap_event_header_t* hdr = process->in_events->get(process->in_events, eventIndex);
            if (hdr && hdr->time < nextEventFrame) {
                nextEventFrame = hdr->time;
            }
        }

        uint32_t framesToProcess = nextEventFrame > frame ? nextEventFrame - frame : 0;
        if (framesToProcess > 0) {
            float* chunkL = outL ? (outL + frame) : nullptr;
            float* chunkR = outR ? (outR + frame) : nullptr;
            engine_.processAudio(chunkL, chunkR, static_cast<int>(framesToProcess));
            frame += framesToProcess;
        } else {
            // Cannot happen with well-formed events (the next event is always
            // after `frame`); never spin on a malformed list.
            ++eventIndex;
        }
    }

    pushPendingOutputEvents(process->out_events);

    return CLAP_PROCESS_CONTINUE;
}

const void* AcidusClap::getExtension(const char* id) {
    if (!id) return nullptr;
    if (std::strcmp(id, CLAP_EXT_NOTE_PORTS) == 0) return &g_notePortsExtension;
    if (std::strcmp(id, CLAP_EXT_AUDIO_PORTS) == 0) return &g_audioPortsExtension;
    if (std::strcmp(id, CLAP_EXT_PARAMS) == 0) return &g_paramsExtension;
    if (std::strcmp(id, CLAP_EXT_STATE) == 0) return &g_stateExtension;
    if (std::strcmp(id, CLAP_EXT_GUI) == 0) return &g_acidusGuiExtension;
    return nullptr;
}

void AcidusClap::onMainThread() {
    if (stateDirty_.exchange(false) && host_) {
        const auto* hostState = static_cast<const clap_host_state_t*>(host_->get_extension(host_, CLAP_EXT_STATE));
        if (hostState && hostState->mark_dirty) hostState->mark_dirty(host_);
    }
}

uint32_t AcidusClap::paramsCount() const {
    return PARAM_COUNT;
}

bool AcidusClap::paramsInfo(uint32_t paramIndex, clap_param_info_t* paramInfo) const {
    if (paramIndex >= PARAM_COUNT || !paramInfo) return false;

    std::memset(paramInfo, 0, sizeof(*paramInfo));
    paramInfo->id = paramIndex;
    paramInfo->flags = CLAP_PARAM_IS_AUTOMATABLE;

    switch (paramIndex) {
        case PARAM_CUTOFF:
            snprintf(paramInfo->name, sizeof(paramInfo->name), "Cutoff Freq");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Filter");
            paramInfo->min_value = 0.0;
            paramInfo->max_value = 1.0;
            paramInfo->default_value = 0.5;
            break;
        case PARAM_RESONANCE:
            snprintf(paramInfo->name, sizeof(paramInfo->name), "Resonance");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Filter");
            paramInfo->min_value = 0.0;
            paramInfo->max_value = 1.0;
            paramInfo->default_value = 0.5;
            break;
        case PARAM_ENV_MOD:
            snprintf(paramInfo->name, sizeof(paramInfo->name), "Env Mod");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Filter");
            paramInfo->min_value = 0.0;
            paramInfo->max_value = 1.0;
            paramInfo->default_value = 0.5;
            break;
        case PARAM_DECAY:
            snprintf(paramInfo->name, sizeof(paramInfo->name), "Decay");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Envelope");
            paramInfo->min_value = 0.0;
            paramInfo->max_value = 1.0;
            paramInfo->default_value = 0.5;
            break;
        case PARAM_ACCENT:
            snprintf(paramInfo->name, sizeof(paramInfo->name), "Accent");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Envelope");
            paramInfo->min_value = 0.0;
            paramInfo->max_value = 1.0;
            paramInfo->default_value = 0.5;
            break;
        case PARAM_WAVEFORM:
            snprintf(paramInfo->name, sizeof(paramInfo->name), "Waveform");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Oscillator");
            paramInfo->flags |= CLAP_PARAM_IS_STEPPED;
            paramInfo->min_value = 0.0;
            paramInfo->max_value = 1.0;
            paramInfo->default_value = 0.0; // 0 = Saw, 1 = Square
            break;
        case PARAM_VOLUME:
            snprintf(paramInfo->name, sizeof(paramInfo->name), "Master Volume");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Main");
            paramInfo->min_value = 0.0;
            paramInfo->max_value = 1.0;
            paramInfo->default_value = 0.8;
            break;
        case PARAM_TUNE:
            snprintf(paramInfo->name, sizeof(paramInfo->name), "Tuning");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Oscillator");
            // ±700 cents matches the real hardware's documented Tuning trim
            // travel (TB303_RESEARCH_COMPENDIUM.md, "Tuning control range").
            paramInfo->min_value = -700.0;
            paramInfo->max_value = 700.0;
            paramInfo->default_value = 0.0;
            break;

        case PARAM_OSC_COUPLING_HZ:
            snprintf(paramInfo->name, sizeof(paramInfo->name), "Osc Coupling Freq");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Experimental/Oscillator");
            paramInfo->min_value = 15.0;
            paramInfo->max_value = 120.0;
            paramInfo->default_value = kCalibrationDefaults.oscCouplingHz;
            break;
        case PARAM_RES_COUPLING_HZ:
            snprintf(paramInfo->name, sizeof(paramInfo->name), "Filter Res Coupling Freq");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Experimental/Filter");
            paramInfo->min_value = 40.0;
            paramInfo->max_value = 400.0;
            paramInfo->default_value = kCalibrationDefaults.resCouplingHz;
            break;
        case PARAM_FILTER_FEEDBACK_GAIN:
            snprintf(paramInfo->name, sizeof(paramInfo->name), "Filter Feedback Gain");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Experimental/Filter");
            paramInfo->min_value = 6.0;
            paramInfo->max_value = 22.0;
            paramInfo->default_value = kCalibrationDefaults.filterFeedbackGain;
            break;
        case PARAM_FILTER_POST_HP_HZ:
            snprintf(paramInfo->name, sizeof(paramInfo->name), "Filter Post HP Freq");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Experimental/Filter");
            paramInfo->min_value = 5.0;
            paramInfo->max_value = 250.0;
            paramInfo->default_value = kCalibrationDefaults.filterPostHpHz;
            break;
        case PARAM_FILTER_NOTCH_HZ:
            snprintf(paramInfo->name, sizeof(paramInfo->name), "Filter Notch Freq");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Experimental/Filter");
            paramInfo->min_value = 4.0;
            paramInfo->max_value = 15.0;
            paramInfo->default_value = kCalibrationDefaults.filterNotchHz;
            break;
        case PARAM_FILTER_NOTCH_BANDWIDTH_HZ:
            snprintf(paramInfo->name, sizeof(paramInfo->name), "Filter Notch Bandwidth");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Experimental/Filter");
            paramInfo->min_value = 1.0;
            paramInfo->max_value = 15.0;
            paramInfo->default_value = kCalibrationDefaults.filterNotchBandwidthHz;
            break;
        case PARAM_FILTER_ALLPASS_HZ:
            snprintf(paramInfo->name, sizeof(paramInfo->name), "Filter Allpass Freq");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Experimental/Filter");
            paramInfo->min_value = 8.0;
            paramInfo->max_value = 25.0;
            paramInfo->default_value = kCalibrationDefaults.filterAllpassHz;
            break;
        case PARAM_VCA_GAIN_SATURATION_DRIVE:
            snprintf(paramInfo->name, sizeof(paramInfo->name), "VCA Gain Saturation Drive");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Experimental/Envelope");
            paramInfo->min_value = 0.0; // 0 = linear control law
            paramInfo->max_value = 10.0;
            paramInfo->default_value = kCalibrationDefaults.vcaGainSaturationDrive;
            break;
        case PARAM_VEG_DECAY_SEC:
            snprintf(paramInfo->name, sizeof(paramInfo->name), "VCA Decay Time");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Experimental/Envelope");
            paramInfo->min_value = 1.0;
            paramInfo->max_value = 5.0;
            paramInfo->default_value = kCalibrationDefaults.vegDecaySec;
            break;
        case PARAM_VCA_GATE_OFF_MS:
            snprintf(paramInfo->name, sizeof(paramInfo->name), "VCA Gate-Off Tail");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Experimental/Envelope");
            paramInfo->min_value = 0.3;
            paramInfo->max_value = 5.0;
            paramInfo->default_value = kCalibrationDefaults.vcaGateOffMs;
            break;
        case PARAM_VCA_GATE_OFF_ACCENT_MS:
            snprintf(paramInfo->name, sizeof(paramInfo->name), "VCA Gate-Off Tail (Accent)");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Experimental/Envelope");
            paramInfo->min_value = 0.3;
            paramInfo->max_value = 80.0;
            paramInfo->default_value = kCalibrationDefaults.vcaGateOffAccentMs;
            break;
        case PARAM_FILTER_INPUT_COUPLING_HZ:
            snprintf(paramInfo->name, sizeof(paramInfo->name), "Filter Input Coupling Freq");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Experimental/Filter");
            paramInfo->min_value = 3.0;
            paramInfo->max_value = 60.0;
            paramInfo->default_value = kCalibrationDefaults.filterInputCouplingHz;
            break;
        case PARAM_FILTER_OUTPUT_COUPLING_HZ:
            snprintf(paramInfo->name, sizeof(paramInfo->name), "Filter Output Coupling Freq");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Experimental/Filter");
            paramInfo->min_value = 10000.0;
            paramInfo->max_value = 25000.0;
            paramInfo->default_value = kCalibrationDefaults.filterOutputCouplingHz;
            break;
        case PARAM_FILTER_CAP_SCALE_1:
            snprintf(paramInfo->name, sizeof(paramInfo->name), "Filter Ladder Pole Scale 1");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Experimental/Filter");
            paramInfo->min_value = 0.2;
            paramInfo->max_value = 4.0;
            paramInfo->default_value = kCalibrationDefaults.filterCapScale1;
            break;
        case PARAM_FILTER_CAP_SCALE_2:
            snprintf(paramInfo->name, sizeof(paramInfo->name), "Filter Ladder Pole Scale 2");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Experimental/Filter");
            paramInfo->min_value = 0.2;
            paramInfo->max_value = 4.0;
            paramInfo->default_value = kCalibrationDefaults.filterCapScale2;
            break;
        case PARAM_FILTER_CAP_SCALE_3:
            snprintf(paramInfo->name, sizeof(paramInfo->name), "Filter Ladder Pole Scale 3");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Experimental/Filter");
            paramInfo->min_value = 0.2;
            paramInfo->max_value = 4.0;
            paramInfo->default_value = kCalibrationDefaults.filterCapScale3;
            break;
        case PARAM_FILTER_CAP_SCALE_4:
            snprintf(paramInfo->name, sizeof(paramInfo->name), "Filter Ladder Pole Scale 4");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Experimental/Filter");
            paramInfo->min_value = 0.2;
            paramInfo->max_value = 4.0;
            paramInfo->default_value = kCalibrationDefaults.filterCapScale4;
            break;
        case PARAM_FILTER_LADDER_INPUT_SCALE:
            snprintf(paramInfo->name, sizeof(paramInfo->name), "Filter Ladder Input Drive");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Experimental/Filter");
            paramInfo->min_value = 0.01;
            paramInfo->max_value = 0.4;
            paramInfo->default_value = kCalibrationDefaults.filterLadderInputScale;
            break;
        case PARAM_FILTER_RES_LIMIT:
            // Max feedback at Resonance 1 as a fraction of the loop's critical
            // (self-oscillation) gain at the current cutoff. Only binds at
            // high cutoff: < 1 never self-oscillates (hardware reports), > 1
            // lets the top of the sweep tip into nonlinearity-limited
            // oscillation (Stinchcombe's model); above ~1.08 the fixed feedback
            // ceiling binds everywhere, i.e. the limit is off.
            snprintf(paramInfo->name, sizeof(paramInfo->name), "Filter Resonance Limit");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Experimental/Filter");
            paramInfo->min_value = 0.90;
            paramInfo->max_value = 1.20;
            paramInfo->default_value = kCalibrationDefaults.filterResonanceLimit;
            break;
        case PARAM_ACCENT_VCA_DEPTH:
            // Accent term in the VCA control sum, x accented MEG (tau 68 ms)
            // x Accent knob, relative to the normal VCA envelope.
            snprintf(paramInfo->name, sizeof(paramInfo->name), "Accent VCA Depth");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Experimental/Envelope");
            paramInfo->min_value = 0.0;
            paramInfo->max_value = 6.0;
            paramInfo->default_value = kCalibrationDefaults.accentVcaDepth;
            break;
        case PARAM_ACCENT_SWEEP_DEPTH:
            // Accent sweep into the cutoff, octaves at full Accent.
            snprintf(paramInfo->name, sizeof(paramInfo->name), "Accent Sweep Depth");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Experimental/Filter");
            paramInfo->min_value = 0.0;
            paramInfo->max_value = 9.0;
            paramInfo->default_value = kCalibrationDefaults.accentSweepDepthOct;
            break;
        case PARAM_VCA_RES_TAP_RATIO:
            // Filter -> VCA taps (TB303_REFERENCE.md §12): wiper tap relative
            // to the fixed tap. Higher = Resonance-max notes louder relative
            // to Resonance-min notes.
            snprintf(paramInfo->name, sizeof(paramInfo->name), "VCA Resonance Tap Ratio");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Experimental/Envelope");
            paramInfo->min_value = 0.0;
            paramInfo->max_value = 3.0;
            paramInfo->default_value = kCalibrationDefaults.vcaResTapRatio;
            break;
        case PARAM_VCO_OCTAVE_SCALE:
            // VCO V/oct scale (TM5 width trim, TB303_REFERENCE.md §5.3):
            // 1 = exact 2:1 octaves; measured units 0.99-1.03. Pivots on
            // the A key at 110 Hz.
            snprintf(paramInfo->name, sizeof(paramInfo->name), "VCO Octave Scale");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Experimental/Oscillator");
            paramInfo->min_value = 0.95;
            paramInfo->max_value = 1.05;
            paramInfo->default_value = kCalibrationDefaults.vcoOctaveScale;
            break;
        case PARAM_FILTER_LADDER_TOPOLOGY:
            // 0 = legacy mirrored ladder, 1 = circuit orientation (§10.3).
            snprintf(paramInfo->name, sizeof(paramInfo->name), "Filter Ladder Topology");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Experimental/Filter");
            paramInfo->flags |= CLAP_PARAM_IS_STEPPED;
            paramInfo->min_value = 0.0;
            paramInfo->max_value = 1.0;
            paramInfo->default_value = kCalibrationDefaults.filterLadderTopology;
            break;
        case PARAM_CUTOFF_BASE_HZ:
            // The unit's cutoff trim (TM3): shifts the whole Cutoff range.
            snprintf(paramInfo->name, sizeof(paramInfo->name), "Cutoff Trim (Base Freq)");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Experimental/Filter");
            paramInfo->min_value = 80.0;
            paramInfo->max_value = 500.0;
            paramInfo->default_value = kCalibrationDefaults.cutoffBaseHz;
            break;
        case PARAM_CUTOFF_SPAN_OCT:
            snprintf(paramInfo->name, sizeof(paramInfo->name), "Cutoff Knob Span");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Experimental/Filter");
            paramInfo->min_value = 1.5;
            paramInfo->max_value = 5.0;
            paramInfo->default_value = kCalibrationDefaults.cutoffSpanOct;
            break;
        case PARAM_CUTOFF_TAPER_EXP:
            snprintf(paramInfo->name, sizeof(paramInfo->name), "Cutoff Knob Taper");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Experimental/Filter");
            paramInfo->min_value = 0.5;
            paramInfo->max_value = 3.0;
            paramInfo->default_value = kCalibrationDefaults.cutoffTaperExp;
            break;
        case PARAM_FILTER_RES_SKEW:
            snprintf(paramInfo->name, sizeof(paramInfo->name), "Resonance Knob Curve");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Experimental/Filter");
            paramInfo->min_value = -6.0;
            paramInfo->max_value = 8.0;
            paramInfo->default_value = kCalibrationDefaults.filterResonanceSkew;
            break;
        case PARAM_ENV_MOD_SCALE_C0:
            snprintf(paramInfo->name, sizeof(paramInfo->name), "Env Mod Depth @Cut Min");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Experimental/Env Mod");
            paramInfo->min_value = 0.0;
            paramInfo->max_value = 2.0;
            paramInfo->default_value = kCalibrationDefaults.envModScaleC0;
            break;
        case PARAM_ENV_MOD_SCALE_C0_SLOPE:
            snprintf(paramInfo->name, sizeof(paramInfo->name), "Env Mod Depth Slope @Cut Min");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Experimental/Env Mod");
            paramInfo->min_value = 0.0;
            paramInfo->max_value = 8.0;
            paramInfo->default_value = kCalibrationDefaults.envModScaleC0Slope;
            break;
        case PARAM_ENV_MOD_SCALE_C1:
            snprintf(paramInfo->name, sizeof(paramInfo->name), "Env Mod Depth @Cut Max");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Experimental/Env Mod");
            paramInfo->min_value = 0.0;
            paramInfo->max_value = 2.0;
            paramInfo->default_value = kCalibrationDefaults.envModScaleC1;
            break;
        case PARAM_ENV_MOD_SCALE_C1_SLOPE:
            snprintf(paramInfo->name, sizeof(paramInfo->name), "Env Mod Depth Slope @Cut Max");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Experimental/Env Mod");
            paramInfo->min_value = 0.0;
            paramInfo->max_value = 8.0;
            paramInfo->default_value = kCalibrationDefaults.envModScaleC1Slope;
            break;
        case PARAM_ENV_MOD_OFFSET:
            snprintf(paramInfo->name, sizeof(paramInfo->name), "Env Mod Bias Offset");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Experimental/Env Mod");
            paramInfo->min_value = 0.0;
            paramInfo->max_value = 0.8;
            paramInfo->default_value = kCalibrationDefaults.envModOffset;
            break;
        case PARAM_ENV_MOD_OFFSET_CUT_SLOPE:
            snprintf(paramInfo->name, sizeof(paramInfo->name), "Env Mod Bias Offset Cut Slope");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Experimental/Env Mod");
            paramInfo->min_value = -0.5;
            paramInfo->max_value = 0.5;
            paramInfo->default_value = kCalibrationDefaults.envModOffsetCutSlope;
            break;
        case PARAM_VCF_DECAY_MIN_SEC:
            snprintf(paramInfo->name, sizeof(paramInfo->name), "MEG Decay @Decay Min");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Experimental/Envelope");
            paramInfo->min_value = 0.02;
            paramInfo->max_value = 0.3;
            paramInfo->default_value = kCalibrationDefaults.vcfDecayMinSec;
            break;
        case PARAM_VCF_DECAY_MAX_SEC:
            snprintf(paramInfo->name, sizeof(paramInfo->name), "MEG Decay @Decay Max");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Experimental/Envelope");
            paramInfo->min_value = 0.3;
            paramInfo->max_value = 3.0;
            paramInfo->default_value = kCalibrationDefaults.vcfDecayMaxSec;
            break;
        case PARAM_ACCENT_DECAY_SEC:
            snprintf(paramInfo->name, sizeof(paramInfo->name), "MEG Decay (Accent)");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Experimental/Envelope");
            paramInfo->min_value = 0.02;
            paramInfo->max_value = 0.3;
            paramInfo->default_value = kCalibrationDefaults.accentDecaySec;
            break;
        case PARAM_ACCENT_CHARGE_BASE_SEC:
            snprintf(paramInfo->name, sizeof(paramInfo->name), "Accent Sweep Charge (R46 C13)");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Experimental/Envelope");
            paramInfo->min_value = 0.005;
            paramInfo->max_value = 0.2;
            paramInfo->default_value = kCalibrationDefaults.accentChargeBaseSec;
            break;
        case PARAM_ACCENT_CHARGE_POT_SEC:
            snprintf(paramInfo->name, sizeof(paramInfo->name), "Accent Sweep Charge (VR4b C13)");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Experimental/Envelope");
            paramInfo->min_value = 0.005;
            paramInfo->max_value = 0.2;
            paramInfo->default_value = kCalibrationDefaults.accentChargePotSec;
            break;
        case PARAM_ACCENT_MIX_SEC:
            snprintf(paramInfo->name, sizeof(paramInfo->name), "Accent Sweep Discharge (Rmix C13)");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Experimental/Envelope");
            paramInfo->min_value = 0.02;
            paramInfo->max_value = 0.5;
            paramInfo->default_value = kCalibrationDefaults.accentMixSec;
            break;
        case PARAM_ENV_MOD_TAPER_EXP:
            snprintf(paramInfo->name, sizeof(paramInfo->name), "Env Mod Knob Taper");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Experimental/Env Mod");
            paramInfo->min_value = 0.5;
            paramInfo->max_value = 4.0;
            paramInfo->default_value = kCalibrationDefaults.envModTaperExp;
            break;
        case PARAM_ACCENT_DIODE_DROP:
            snprintf(paramInfo->name, sizeof(paramInfo->name), "Accent Sweep Diode Drop");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Experimental/Envelope");
            paramInfo->min_value = 0.0;
            paramInfo->max_value = 0.5;
            paramInfo->default_value = kCalibrationDefaults.accentDiodeDrop;
            break;
        case PARAM_VCF_DECAY_TAPER:
            snprintf(paramInfo->name, sizeof(paramInfo->name), "Decay Knob Taper");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Experimental/Envelope");
            paramInfo->min_value = 1.0;
            paramInfo->max_value = 200.0;
            paramInfo->default_value = kCalibrationDefaults.vcfDecayTaper;
            break;
        case PARAM_ENV_MOD_TAPER_MID:
            snprintf(paramInfo->name, sizeof(paramInfo->name), "Env Mod Taper Mid (logistic)");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Experimental/Env Mod");
            paramInfo->min_value = 0.2;
            paramInfo->max_value = 1.0;
            paramInfo->default_value = kCalibrationDefaults.envModTaperMid;
            break;
        case PARAM_ENV_MOD_TAPER_WIDTH:
            snprintf(paramInfo->name, sizeof(paramInfo->name), "Env Mod Taper Width (0 = power law)");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Experimental/Env Mod");
            paramInfo->min_value = 0.0;
            paramInfo->max_value = 0.5;
            paramInfo->default_value = kCalibrationDefaults.envModTaperWidth;
            break;
        case PARAM_CUTOFF_MAX_HZ:
            snprintf(paramInfo->name, sizeof(paramInfo->name), "Cutoff Ceiling");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Experimental/Filter");
            paramInfo->min_value = 8000.0;
            paramInfo->max_value = 30000.0;
            paramInfo->default_value = kCalibrationDefaults.cutoffMaxHz;
            break;
        case PARAM_VCA_NORMAL_DELAY_MS:
            snprintf(paramInfo->name, sizeof(paramInfo->name), "VCA Onset Delay (Unaccented)");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Experimental/Envelope");
            paramInfo->min_value = 0.0;
            paramInfo->max_value = 10.0;
            paramInfo->default_value = kCalibrationDefaults.vcaNormalDelayMs;
            break;
        case PARAM_VCA_ATTACK_MS:
            snprintf(paramInfo->name, sizeof(paramInfo->name), "VCA Attack (VEG Onset)");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Experimental/Envelope");
            paramInfo->min_value = 0.3;
            paramInfo->max_value = 30.0;
            paramInfo->default_value = kCalibrationDefaults.vcaAttackMs;
            break;

        case PARAM_DRIVE:
            snprintf(paramInfo->name, sizeof(paramInfo->name), "Drive");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Distortion");
            paramInfo->min_value = 0.0;
            paramInfo->max_value = 1.0;
            paramInfo->default_value = 0.0;
            break;

        default:
            return false;
    }
    return true;
}

bool AcidusClap::paramsValue(clap_id paramId, double* outValue) {
    if (paramId >= PARAM_COUNT || !outValue) return false;
    *outValue = getParam(paramId);
    return true;
}

bool AcidusClap::paramsDefaultValue(clap_id paramId, double* outValue) const {
    if (paramId >= PARAM_COUNT || !outValue) return false;
    *outValue = paramDefault_[paramId];
    return true;
}

void AcidusClap::requestHostFlush() {
    if (host_) {
        const auto* host_params = static_cast<const clap_host_params_t*>(
            host_->get_extension(host_, CLAP_EXT_PARAMS));
        if (host_params && host_params->request_flush) {
            host_params->request_flush(host_);
        } else if (host_->request_process) {
            host_->request_process(host_);
        }
    }
}

void AcidusClap::queueOutEvent(const GuiParamEvent& ev, bool audioThread) {
    std::unique_lock<std::mutex> lock(outEventQueueMutex_, std::defer_lock);
    if (audioThread) {
        if (!lock.try_lock()) return;   // never block the audio thread; the engine already has the value
    } else {
        lock.lock();
    }
    if (outEventCount_ < kOutQueueCapacity) outEventQueue_[outEventCount_++] = ev;
}

void AcidusClap::onBeginEditFromGui(clap_id paramId) {
    if (paramId >= PARAM_COUNT) return;
    queueOutEvent({ CLAP_EVENT_PARAM_GESTURE_BEGIN, paramId, 0.0, CLAP_EVENT_IS_LIVE }, false);
    requestHostFlush();
}

void AcidusClap::onParamValueFromGui(clap_id paramId, double value) {
    if (paramId >= PARAM_COUNT) return;
    setParam(paramId, value);   // clamped; a non-finite value becomes the default
    syncParamsToEngine();
    queueOutEvent({ CLAP_EVENT_PARAM_VALUE, paramId, getParam(paramId), CLAP_EVENT_IS_LIVE }, false);
    requestHostFlush();
}

void AcidusClap::onEndEditFromGui(clap_id paramId) {
    if (paramId >= PARAM_COUNT) return;
    queueOutEvent({ CLAP_EVENT_PARAM_GESTURE_END, paramId, 0.0, CLAP_EVENT_IS_LIVE }, false);
    requestHostFlush();
}

int AcidusClap::calibrationPresetCount() {
    return kCalibrationPresetCount;
}

const char* AcidusClap::calibrationPresetName() const {
    return calibrationPresets()[calibrationPreset_.load()].name;
}

bool AcidusClap::isCalibrationModified() const {
#ifdef ACIDUS_CALIBRATION_BUILD
    const SynthParameters& preset = calibrationPresets()[calibrationPreset_.load()].params;
    for (const auto& b : kCalibrationBindings) {
        const double want = static_cast<double>(preset.*(b.field));
        if (std::abs(getParam(b.id) - want) > 1e-6 * std::max(1.0, std::abs(want))) {
            return true;
        }
    }
#endif
    return false;
}

void AcidusClap::selectCalibrationPreset(int index, bool notifyHost) {
    if (index < 0 || index >= kCalibrationPresetCount) return;
    calibrationPreset_.store(index);
#ifdef ACIDUS_CALIBRATION_BUILD
    // Load the preset into the host-visible calibration parameters; the
    // front-panel knobs are left alone.
    const SynthParameters& preset = calibrationPresets()[index].params;
    for (const auto& b : kCalibrationBindings) {
        setParam(b.id, static_cast<double>(preset.*(b.field)));
        if (notifyHost) {
            const double v = getParam(b.id);
            queueOutEvent({ CLAP_EVENT_PARAM_GESTURE_BEGIN, b.id, 0.0, CLAP_EVENT_IS_LIVE }, false);
            queueOutEvent({ CLAP_EVENT_PARAM_VALUE, b.id, v, CLAP_EVENT_IS_LIVE }, false);
            queueOutEvent({ CLAP_EVENT_PARAM_GESTURE_END, b.id, 0.0, CLAP_EVENT_IS_LIVE }, false);
        }
    }
#endif
    syncParamsToEngine();
    if (notifyHost) {
        // The preset is part of the saved state (in a Release build it is
        // not a parameter, so the host would not otherwise know).
        stateDirty_.store(true);
        if (host_ && host_->request_callback) host_->request_callback(host_);
        requestHostFlush();
    }
}

void AcidusClap::cycleCalibrationPresetFromGui() {
    selectCalibrationPreset((calibrationPreset_.load() + 1) % kCalibrationPresetCount, true);
}

void AcidusClap::setParamValueFromGui(clap_id paramId, double value) {
    onParamValueFromGui(paramId, value);
}

void AcidusClap::pushPendingOutputEvents(const clap_output_events_t* out) {
    if (!out || !out->try_push) return;
    size_t count = 0;
    {
        // Audio thread: skip this block's drain rather than wait for the GUI.
        std::unique_lock<std::mutex> lock(outEventQueueMutex_, std::try_to_lock);
        if (!lock.owns_lock()) return;
        count = outEventCount_;
        std::copy(outEventQueue_.begin(), outEventQueue_.begin() + count, drainBuffer_.begin());
        outEventCount_ = 0;
    }

    for (size_t i = 0; i < count; ++i) {
        const GuiParamEvent& ev = drainBuffer_[i];
        if (ev.type == CLAP_EVENT_PARAM_GESTURE_BEGIN || ev.type == CLAP_EVENT_PARAM_GESTURE_END) {
            clap_event_param_gesture_t gestureEv{};
            gestureEv.header.size = sizeof(gestureEv);
            gestureEv.header.time = 0;
            gestureEv.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
            gestureEv.header.type = ev.type;
            gestureEv.header.flags = ev.flags;
            gestureEv.param_id = ev.paramId;
            out->try_push(out, &gestureEv.header);
        } else if (ev.type == CLAP_EVENT_PARAM_VALUE) {
            clap_event_param_value_t valueEv{};
            valueEv.header.size = sizeof(valueEv);
            valueEv.header.time = 0;
            valueEv.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
            valueEv.header.type = CLAP_EVENT_PARAM_VALUE;
            valueEv.header.flags = ev.flags;
            valueEv.param_id = ev.paramId;
            valueEv.cookie = nullptr;
            valueEv.note_id = -1;
            valueEv.port_index = -1;
            valueEv.channel = -1;
            valueEv.key = -1;
            valueEv.value = ev.value;
            out->try_push(out, &valueEv.header);
        }
    }
}

bool AcidusClap::paramsValueToText(clap_id paramId, double value, char* outBuffer, uint32_t outBufferCapacity) {
    if (paramId >= PARAM_COUNT || !outBuffer || outBufferCapacity == 0) return false;
    value = sanitizeParam(paramId, value);

    if (paramId == PARAM_CUTOFF) {
        // The engine's knob law before the envelope (SynthEngine.cpp).
        const SynthParameters p = buildParams();
        double norm = std::min(std::max(value, 0.0), 1.0);
        double hz = p.cutoffBaseHz * std::pow(2.0, p.cutoffSpanOct * std::pow(norm, p.cutoffTaperExp));
        snprintf(outBuffer, outBufferCapacity, "%.1f Hz", hz);
    } else if (paramId == PARAM_WAVEFORM) {
        snprintf(outBuffer, outBufferCapacity, "%s", (value >= 0.5) ? "Square" : "Saw");
    } else if (paramId == PARAM_TUNE) {
        snprintf(outBuffer, outBufferCapacity, "%+.0f cents", value);
    } else if (paramId == PARAM_OSC_COUPLING_HZ || paramId == PARAM_RES_COUPLING_HZ
               || paramId == PARAM_FILTER_POST_HP_HZ || paramId == PARAM_FILTER_NOTCH_HZ
               || paramId == PARAM_FILTER_NOTCH_BANDWIDTH_HZ || paramId == PARAM_FILTER_ALLPASS_HZ
               || paramId == PARAM_FILTER_INPUT_COUPLING_HZ || paramId == PARAM_FILTER_OUTPUT_COUPLING_HZ
               || paramId == PARAM_CUTOFF_BASE_HZ || paramId == PARAM_CUTOFF_MAX_HZ) {
        snprintf(outBuffer, outBufferCapacity, "%.2f Hz", value);
    } else if (paramId == PARAM_ACCENT_SWEEP_DEPTH || paramId == PARAM_CUTOFF_SPAN_OCT) {
        snprintf(outBuffer, outBufferCapacity, "%.2f oct", value);
    } else if (paramId == PARAM_FILTER_RES_LIMIT) {
        snprintf(outBuffer, outBufferCapacity, "%.3f x critical", value);
    } else if (paramId == PARAM_VCF_DECAY_MIN_SEC || paramId == PARAM_VCF_DECAY_MAX_SEC || paramId == PARAM_ACCENT_DECAY_SEC || paramId == PARAM_ACCENT_CHARGE_BASE_SEC || paramId == PARAM_ACCENT_CHARGE_POT_SEC || paramId == PARAM_ACCENT_MIX_SEC) {
        snprintf(outBuffer, outBufferCapacity, "%.0f ms", value * 1000.0);
    } else if (paramId == PARAM_ENV_MOD_SCALE_C0 || paramId == PARAM_ENV_MOD_SCALE_C0_SLOPE || paramId == PARAM_ENV_MOD_SCALE_C1 || paramId == PARAM_ENV_MOD_SCALE_C1_SLOPE) {
        snprintf(outBuffer, outBufferCapacity, "%.3f oct", value);
    } else if (paramId == PARAM_VEG_DECAY_SEC) {
        snprintf(outBuffer, outBufferCapacity, "%.2f s", value);
    } else if (paramId == PARAM_VCA_GATE_OFF_MS || paramId == PARAM_VCA_GATE_OFF_ACCENT_MS
               || paramId == PARAM_VCA_NORMAL_DELAY_MS || paramId == PARAM_VCA_ATTACK_MS) {
        snprintf(outBuffer, outBufferCapacity, "%.1f ms", value);
    } else {
        snprintf(outBuffer, outBufferCapacity, "%.2f", value);
    }
    return true;
}

bool AcidusClap::paramsTextToValue(clap_id paramId, const char* paramValueText, double* outValue) {
    if (paramId >= PARAM_COUNT || !paramValueText || !outValue) return false;
    if (paramId == PARAM_WAVEFORM) {
        if (std::strstr(paramValueText, "Square") || std::strstr(paramValueText, "square")) {
            *outValue = 1.0;
        } else {
            *outValue = 0.0;
        }
        return true;
    }
    if (paramId == PARAM_CUTOFF) {
        const SynthParameters p = buildParams();
        const double hz = std::strtod(paramValueText, nullptr);
        double oct = (std::isfinite(hz) && hz > 0.0 && p.cutoffBaseHz > 0.0f && p.cutoffSpanOct > 0.0f)
                         ? std::log2(hz / p.cutoffBaseHz) / p.cutoffSpanOct : 0.0;
        oct = std::isfinite(oct) ? std::min(std::max(oct, 0.0), 1.0) : 0.0;
        const double taper = (p.cutoffTaperExp > 0.0f) ? p.cutoffTaperExp : 1.0;
        *outValue = std::pow(oct, 1.0 / taper);
        return true;
    }
    const double v = std::strtod(paramValueText, nullptr);
    if (!std::isfinite(v)) return false;
    *outValue = sanitizeParam(paramId, v);
    return true;
}

void AcidusClap::paramsFlush(const clap_input_events_t* in, const clap_output_events_t* out) {
    if (in && in->size && in->get) {
        uint32_t size = in->size(in);
        for (uint32_t i = 0; i < size; ++i) {
            handleEvent(in->get(in, i));
        }
    }
    pushPendingOutputEvents(out);
}

// State: a small header, then the parameter values as doubles.
//   uint32 magic 'ACS2', uint32 parameter count, int32 calibration preset,
//   uint32 reserved, double values[parameter count]
// The legacy format (before calibration presets) is the bare doubles; it
// loads with the first preset. The count lets a Release build and a
// calibration build (more parameters) read each other's state.
static constexpr uint32_t kStateMagic = 0x32534341u; // "ACS2"

bool AcidusClap::stateSave(const clap_ostream_t* stream) {
    if (!stream || !stream->write) return false;
    std::vector<uint8_t> data(16 + PARAM_COUNT * sizeof(double));
    const uint32_t header[4] = { kStateMagic, static_cast<uint32_t>(PARAM_COUNT),
                                 static_cast<uint32_t>(calibrationPreset_.load()), 0u };
    std::memcpy(data.data(), header, sizeof(header));
    for (uint32_t i = 0; i < PARAM_COUNT; ++i) {
        const double v = getParam(i);
        std::memcpy(data.data() + 16 + i * sizeof(double), &v, sizeof(double));
    }
    size_t done = 0;
    while (done < data.size()) {
        int64_t n = stream->write(stream, data.data() + done, data.size() - done);
        if (n <= 0 || static_cast<size_t>(n) > data.size() - done) return false;
        done += static_cast<size_t>(n);
    }
    return true;
}

bool AcidusClap::stateLoad(const clap_istream_t* stream) {
    if (!stream || !stream->read) return false;
    std::vector<uint8_t> data;
    uint8_t chunk[1024];
    while (data.size() < (1u << 20)) {
        int64_t n = stream->read(stream, chunk, sizeof(chunk));
        if (n < 0) return false;
        if (n == 0) break;
        n = std::min<int64_t>(n, sizeof(chunk));   // a misbehaving stream must not overrun the chunk
        data.insert(data.end(), chunk, chunk + n);
    }
    if (data.empty()) return false;

    int preset = 0;
    size_t offset = 0;
    size_t count = data.size() / sizeof(double);
    if (data.size() >= 16) {
        uint32_t header[4];
        std::memcpy(header, data.data(), sizeof(header));
        if (header[0] == kStateMagic) {
            preset = (header[2] < static_cast<uint32_t>(kCalibrationPresetCount)) ? static_cast<int>(header[2]) : 0;
            offset = 16;
            count = std::min<size_t>(header[1], (data.size() - 16) / sizeof(double));
        }
    }
    // Defaults first, so anything the saved state lacks (a shorter or corrupt
    // blob) is well-defined; then the preset (sets the calibration parameters);
    // then the saved values on top: user edits of calibration parameters
    // survive. Each value is range-checked; NaN/Inf fall back to the default.
    resetParamsToDefaults();
    selectCalibrationPreset(preset, false);
    count = std::min<size_t>(count, PARAM_COUNT);
    for (size_t i = 0; i < count; ++i) {
        double v;
        std::memcpy(&v, data.data() + offset + i * sizeof(double), sizeof(double));
        if (std::isfinite(v)) setParam(static_cast<clap_id>(i), v);
    }
    syncParamsToEngine();
    return true;
}

// CLAP Plugin Entry Point
static const char* g_acidusFeatures[] = {
    CLAP_PLUGIN_FEATURE_INSTRUMENT,
    CLAP_PLUGIN_FEATURE_SYNTHESIZER,
    CLAP_PLUGIN_FEATURE_STEREO,
    nullptr
};

static const clap_plugin_descriptor_t g_acidusDescriptor = {
    CLAP_VERSION,
    "com.acidus.synth",
    "Acidus",
    "Acidus",
    "https://github.com/acidus/acidus",
    "",
    "",
    "1.0.0",
    "Roland TB-303 Bass Synth Emulator",
    g_acidusFeatures
};

static uint32_t clap_factory_get_plugin_count(const clap_plugin_factory_t* factory) {
    return 1;
}

static const clap_plugin_descriptor_t* clap_factory_get_plugin_descriptor(const clap_plugin_factory_t* factory, uint32_t index) {
    return (index == 0) ? &g_acidusDescriptor : nullptr;
}

static const clap_plugin_t* clap_factory_create_plugin(const clap_plugin_factory_t* factory, const clap_host_t* host, const char* plugin_id) {
    if (!host || !plugin_id) return nullptr;
    if (!clap_version_is_compatible(host->clap_version)) return nullptr;
    if (std::strcmp(plugin_id, g_acidusDescriptor.id) != 0) return nullptr;

    try {
        auto* plugin = new AcidusClap(host);
        return plugin->getClapPlugin();
    } catch (...) {
        return nullptr;
    }
}

static const clap_plugin_factory_t g_acidusFactory = {
    clap_factory_get_plugin_count,
    clap_factory_get_plugin_descriptor,
    clap_factory_create_plugin
};

static bool entry_init(const char* plugin_path) {
    return true;
}

static void entry_deinit() {}

static const void* entry_get_factory(const char* factory_id) {
    if (factory_id && std::strcmp(factory_id, CLAP_PLUGIN_FACTORY_ID) == 0) {
        return &g_acidusFactory;
    }
    return nullptr;
}

} // namespace acidus

extern "C" CLAP_EXPORT const clap_plugin_entry_t clap_entry = {
    CLAP_VERSION,
    acidus::entry_init,
    acidus::entry_deinit,
    acidus::entry_get_factory
};
