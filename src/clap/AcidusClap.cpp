#include "AcidusClap.hpp"
#include "gui/GuiWindow.hpp"
#include <cstring>
#include <cstdio>
#include <algorithm>

namespace acidus {

// Forward declarations of GUI extension functions
extern const clap_plugin_gui_t g_acidusGuiExtension;

static const clap_plugin_note_ports_t g_notePortsExtension = {
    // count
    [](const clap_plugin_t* plugin, bool is_input) -> uint32_t {
        return is_input ? 1 : 0;
    },
    // get
    [](const clap_plugin_t* plugin, uint32_t index, bool is_input, clap_note_port_info_t* info) -> bool {
        if (!is_input || index != 0) return false;
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
        if (is_input || index != 0) return false;
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
        return self->paramsCount();
    },
    // get_info
    [](const clap_plugin_t* plugin, uint32_t param_index, clap_param_info_t* param_info) -> bool {
        auto* self = static_cast<AcidusClap*>(plugin->plugin_data);
        return self->paramsInfo(param_index, param_info);
    },
    // get_value
    [](const clap_plugin_t* plugin, clap_id param_id, double* out_value) -> bool {
        auto* self = static_cast<AcidusClap*>(plugin->plugin_data);
        return self->paramsValue(param_id, out_value);
    },
    // value_to_text
    [](const clap_plugin_t* plugin, clap_id param_id, double value, char* out_buffer, uint32_t out_buffer_capacity) -> bool {
        auto* self = static_cast<AcidusClap*>(plugin->plugin_data);
        return self->paramsValueToText(param_id, value, out_buffer, out_buffer_capacity);
    },
    // text_to_value
    [](const clap_plugin_t* plugin, clap_id param_id, const char* param_value_text, double* out_value) -> bool {
        auto* self = static_cast<AcidusClap*>(plugin->plugin_data);
        return self->paramsTextToValue(param_id, param_value_text, out_value);
    },
    // flush
    [](const clap_plugin_t* plugin, const clap_input_events_t* in, const clap_output_events_t* out) {
        auto* self = static_cast<AcidusClap*>(plugin->plugin_data);
        self->paramsFlush(in, out);
    }
};

static const clap_plugin_state_t g_stateExtension = {
    // save
    [](const clap_plugin_t* plugin, const clap_ostream_t* stream) -> bool {
        auto* self = static_cast<AcidusClap*>(plugin->plugin_data);
        return self->stateSave(stream);
    },
    // load
    [](const clap_plugin_t* plugin, const clap_istream_t* stream) -> bool {
        auto* self = static_cast<AcidusClap*>(plugin->plugin_data);
        return self->stateLoad(stream);
    }
};

AcidusClap::AcidusClap(const clap_host_t* host) : host_(host) {
    clapPlugin_.desc = nullptr;
    clapPlugin_.plugin_data = this;
    clapPlugin_.init = [](const clap_plugin_t* plugin) -> bool {
        return static_cast<AcidusClap*>(plugin->plugin_data)->init();
    };
    clapPlugin_.destroy = [](const clap_plugin_t* plugin) {
        static_cast<AcidusClap*>(plugin->plugin_data)->destroy();
    };
    clapPlugin_.activate = [](const clap_plugin_t* plugin, double sample_rate, uint32_t min_frames, uint32_t max_frames) -> bool {
        return static_cast<AcidusClap*>(plugin->plugin_data)->activate(sample_rate, min_frames, max_frames);
    };
    clapPlugin_.deactivate = [](const clap_plugin_t* plugin) {
        static_cast<AcidusClap*>(plugin->plugin_data)->deactivate();
    };
    clapPlugin_.start_processing = [](const clap_plugin_t* plugin) -> bool {
        return static_cast<AcidusClap*>(plugin->plugin_data)->startProcessing();
    };
    clapPlugin_.stop_processing = [](const clap_plugin_t* plugin) {
        static_cast<AcidusClap*>(plugin->plugin_data)->stopProcessing();
    };
    clapPlugin_.reset = [](const clap_plugin_t* plugin) {
        static_cast<AcidusClap*>(plugin->plugin_data)->reset();
    };
    clapPlugin_.process = [](const clap_plugin_t* plugin, const clap_process_t* process) -> clap_process_status {
        return static_cast<AcidusClap*>(plugin->plugin_data)->process(process);
    };
    clapPlugin_.get_extension = [](const clap_plugin_t* plugin, const char* id) -> const void* {
        return static_cast<AcidusClap*>(plugin->plugin_data)->getExtension(id);
    };
    clapPlugin_.on_main_thread = [](const clap_plugin_t* plugin) {
        static_cast<AcidusClap*>(plugin->plugin_data)->onMainThread();
    };

    // Initialize default parameter values
    paramValues_[PARAM_CUTOFF] = 0.5;
    paramValues_[PARAM_RESONANCE] = 0.5;
    paramValues_[PARAM_ENV_MOD] = 0.5;
    paramValues_[PARAM_DECAY] = 0.5;
    paramValues_[PARAM_ACCENT] = 0.5;
    paramValues_[PARAM_WAVEFORM] = 0.0; // 0 = Saw, 1 = Square
    paramValues_[PARAM_VOLUME] = 0.8;

#ifdef ACIDUS_CALIBRATION_BUILD
    paramValues_[PARAM_OSC_COUPLING_HZ] = 44.5;
    paramValues_[PARAM_RES_COUPLING_HZ] = 150.0;
    paramValues_[PARAM_FILTER_FEEDBACK_GAIN] = 15.3;
    paramValues_[PARAM_FILTER_POST_HP_HZ] = 24.167;
    paramValues_[PARAM_FILTER_NOTCH_HZ] = 7.5164;
    paramValues_[PARAM_FILTER_NOTCH_BANDWIDTH_HZ] = 4.7;
    paramValues_[PARAM_FILTER_ALLPASS_HZ] = 14.008;
    paramValues_[PARAM_VEG_DECAY_SEC] = 3.5;
    paramValues_[PARAM_VCA_GATE_OFF_MS] = 3.0;
    paramValues_[PARAM_VCA_GATE_OFF_ACCENT_MS] = 3.0;
    paramValues_[PARAM_VCA_GAIN_SATURATION_DRIVE] = 3.0;
    paramValues_[PARAM_FILTER_INPUT_COUPLING_HZ] = 20.0;
    paramValues_[PARAM_FILTER_OUTPUT_COUPLING_HZ] = 20000.0;
    paramValues_[PARAM_FILTER_CAP_SCALE_1] = 1.0;
    paramValues_[PARAM_FILTER_CAP_SCALE_2] = 1.0;
    paramValues_[PARAM_FILTER_CAP_SCALE_3] = 1.0;
    paramValues_[PARAM_FILTER_CAP_SCALE_4] = 1.0;
    paramValues_[PARAM_FILTER_LADDER_INPUT_SCALE] = 0.05;
#endif

    paramValues_[PARAM_DRIVE] = 0.0; // pedal bypassed by default
    paramValues_[PARAM_TUNE] = 0.0;  // in tune by default

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

bool AcidusClap::activate(double sampleRate, uint32_t minFrames, uint32_t maxFrames) {
    engine_.setSampleRate(sampleRate);
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

void AcidusClap::syncParamsToEngine() {
    auto& params = engine_.getParams();
    params.cutoff = static_cast<float>(paramValues_[PARAM_CUTOFF]);
    params.resonance = static_cast<float>(paramValues_[PARAM_RESONANCE]);
    params.envMod = static_cast<float>(paramValues_[PARAM_ENV_MOD]);
    params.decay = static_cast<float>(paramValues_[PARAM_DECAY]);
    params.accent = static_cast<float>(paramValues_[PARAM_ACCENT]);
    params.waveform = (paramValues_[PARAM_WAVEFORM] >= 0.5) ? Waveform::Square : Waveform::Saw;
    params.masterVolume = static_cast<float>(paramValues_[PARAM_VOLUME]);

#ifdef ACIDUS_CALIBRATION_BUILD
    params.oscCouplingHz = static_cast<float>(paramValues_[PARAM_OSC_COUPLING_HZ]);
    params.resCouplingHz = static_cast<float>(paramValues_[PARAM_RES_COUPLING_HZ]);
    params.filterFeedbackGain = static_cast<float>(paramValues_[PARAM_FILTER_FEEDBACK_GAIN]);
    params.filterPostHpHz = static_cast<float>(paramValues_[PARAM_FILTER_POST_HP_HZ]);
    params.filterNotchHz = static_cast<float>(paramValues_[PARAM_FILTER_NOTCH_HZ]);
    params.filterNotchBandwidthHz = static_cast<float>(paramValues_[PARAM_FILTER_NOTCH_BANDWIDTH_HZ]);
    params.filterAllpassHz = static_cast<float>(paramValues_[PARAM_FILTER_ALLPASS_HZ]);
    params.vegDecaySec = static_cast<float>(paramValues_[PARAM_VEG_DECAY_SEC]);
    params.vcaGateOffMs = static_cast<float>(paramValues_[PARAM_VCA_GATE_OFF_MS]);
    params.vcaGateOffAccentMs = static_cast<float>(paramValues_[PARAM_VCA_GATE_OFF_ACCENT_MS]);
    params.vcaGainSaturationDrive = static_cast<float>(paramValues_[PARAM_VCA_GAIN_SATURATION_DRIVE]);
    params.filterInputCouplingHz = static_cast<float>(paramValues_[PARAM_FILTER_INPUT_COUPLING_HZ]);
    params.filterOutputCouplingHz = static_cast<float>(paramValues_[PARAM_FILTER_OUTPUT_COUPLING_HZ]);
    params.filterCapScale1 = static_cast<float>(paramValues_[PARAM_FILTER_CAP_SCALE_1]);
    params.filterCapScale2 = static_cast<float>(paramValues_[PARAM_FILTER_CAP_SCALE_2]);
    params.filterCapScale3 = static_cast<float>(paramValues_[PARAM_FILTER_CAP_SCALE_3]);
    params.filterCapScale4 = static_cast<float>(paramValues_[PARAM_FILTER_CAP_SCALE_4]);
    params.filterLadderInputScale = static_cast<float>(paramValues_[PARAM_FILTER_LADDER_INPUT_SCALE]);
#endif

    params.drive = static_cast<float>(paramValues_[PARAM_DRIVE]);
    params.tuningCents = static_cast<float>(paramValues_[PARAM_TUNE]);
}

void AcidusClap::handleEvent(const clap_event_header_t* header) {
    if (header->space_id != CLAP_CORE_EVENT_SPACE_ID) return;

    if (header->type == CLAP_EVENT_NOTE_ON) {
        const auto* noteEv = reinterpret_cast<const clap_event_note_t*>(header);
        engine_.noteOn(noteEv->key, static_cast<float>(noteEv->velocity));
    } else if (header->type == CLAP_EVENT_NOTE_OFF) {
        const auto* noteEv = reinterpret_cast<const clap_event_note_t*>(header);
        engine_.noteOff(noteEv->key);
    } else if (header->type == CLAP_EVENT_MIDI) {
        const auto* midiEv = reinterpret_cast<const clap_event_midi_t*>(header);
        uint8_t status = midiEv->data[0] & 0xF0;
        uint8_t data1 = midiEv->data[1];
        uint8_t data2 = midiEv->data[2];

        if (status == 0x90 && data2 > 0) {
            engine_.noteOn(data1, static_cast<float>(data2) / 127.0f);
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
                    normVal = -700.0 + normVal * 1400.0;
                }
                paramValues_[paramId] = normVal;
                syncParamsToEngine();
                {
                    std::lock_guard<std::mutex> lock(outEventQueueMutex_);
                    outEventQueue_.push_back({ CLAP_EVENT_PARAM_VALUE, paramId, normVal, CLAP_EVENT_DONT_RECORD });
                }
                requestHostFlush();
            }
        }
    } else if (header->type == CLAP_EVENT_PARAM_VALUE) {
        const auto* paramEv = reinterpret_cast<const clap_event_param_value_t*>(header);
        if (paramEv->param_id < PARAM_COUNT) {
            paramValues_[paramEv->param_id] = paramEv->value;
            syncParamsToEngine();
        }
    }
}

clap_process_status AcidusClap::process(const clap_process_t* process) {
    const uint32_t numFrames = process->frames_count;
    const uint32_t numEvents = process->in_events ? process->in_events->size(process->in_events) : 0;
    uint32_t eventIndex = 0;

    float* outL = (process->audio_outputs_count > 0 && process->audio_outputs[0].channel_count > 0)
                      ? process->audio_outputs[0].data32[0]
                      : nullptr;
    float* outR = (process->audio_outputs_count > 0 && process->audio_outputs[0].channel_count > 1)
                      ? process->audio_outputs[0].data32[1]
                      : nullptr;

    for (uint32_t frame = 0; frame < numFrames; ) {
        while (eventIndex < numEvents) {
            const clap_event_header_t* hdr = process->in_events->get(process->in_events, eventIndex);
            if (hdr->time > frame) break;
            handleEvent(hdr);
            eventIndex++;
        }

        uint32_t nextEventFrame = numFrames;
        if (eventIndex < numEvents) {
            const clap_event_header_t* hdr = process->in_events->get(process->in_events, eventIndex);
            if (hdr->time < nextEventFrame) {
                nextEventFrame = hdr->time;
            }
        }

        uint32_t framesToProcess = nextEventFrame - frame;
        if (framesToProcess > 0) {
            float* chunkL = outL ? (outL + frame) : nullptr;
            float* chunkR = outR ? (outR + frame) : nullptr;
            engine_.processAudio(chunkL, chunkR, framesToProcess);
            frame += framesToProcess;
        }
    }

    pushPendingOutputEvents(process->out_events);

    return CLAP_PROCESS_CONTINUE;
}

const void* AcidusClap::getExtension(const char* id) {
    if (std::strcmp(id, CLAP_EXT_NOTE_PORTS) == 0) return &g_notePortsExtension;
    if (std::strcmp(id, CLAP_EXT_AUDIO_PORTS) == 0) return &g_audioPortsExtension;
    if (std::strcmp(id, CLAP_EXT_PARAMS) == 0) return &g_paramsExtension;
    if (std::strcmp(id, CLAP_EXT_STATE) == 0) return &g_stateExtension;
    if (std::strcmp(id, CLAP_EXT_GUI) == 0) return &g_acidusGuiExtension;
    return nullptr;
}

void AcidusClap::onMainThread() {}

uint32_t AcidusClap::paramsCount() const {
    return PARAM_COUNT;
}

bool AcidusClap::paramsInfo(uint32_t paramIndex, clap_param_info_t* paramInfo) const {
    if (paramIndex >= PARAM_COUNT) return false;

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
            paramInfo->min_value = 30.0;
            paramInfo->max_value = 60.0;
            paramInfo->default_value = 44.5;
            break;
        case PARAM_RES_COUPLING_HZ:
            snprintf(paramInfo->name, sizeof(paramInfo->name), "Filter Res Coupling Freq");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Experimental/Filter");
            paramInfo->min_value = 100.0;
            paramInfo->max_value = 250.0;
            paramInfo->default_value = 150.0;
            break;
        case PARAM_FILTER_FEEDBACK_GAIN:
            snprintf(paramInfo->name, sizeof(paramInfo->name), "Filter Feedback Gain");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Experimental/Filter");
            paramInfo->min_value = 12.0;
            paramInfo->max_value = 17.0;
            paramInfo->default_value = 15.3;
            break;
        case PARAM_FILTER_POST_HP_HZ:
            snprintf(paramInfo->name, sizeof(paramInfo->name), "Filter Post HP Freq");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Experimental/Filter");
            paramInfo->min_value = 15.0;
            paramInfo->max_value = 35.0;
            paramInfo->default_value = 24.167;
            break;
        case PARAM_FILTER_NOTCH_HZ:
            snprintf(paramInfo->name, sizeof(paramInfo->name), "Filter Notch Freq");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Experimental/Filter");
            paramInfo->min_value = 4.0;
            paramInfo->max_value = 15.0;
            paramInfo->default_value = 7.5164;
            break;
        case PARAM_FILTER_NOTCH_BANDWIDTH_HZ:
            snprintf(paramInfo->name, sizeof(paramInfo->name), "Filter Notch Bandwidth");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Experimental/Filter");
            paramInfo->min_value = 2.0;
            paramInfo->max_value = 10.0;
            paramInfo->default_value = 4.7;
            break;
        case PARAM_FILTER_ALLPASS_HZ:
            snprintf(paramInfo->name, sizeof(paramInfo->name), "Filter Allpass Freq");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Experimental/Filter");
            paramInfo->min_value = 8.0;
            paramInfo->max_value = 25.0;
            paramInfo->default_value = 14.008;
            break;
        case PARAM_VCA_GAIN_SATURATION_DRIVE:
            snprintf(paramInfo->name, sizeof(paramInfo->name), "VCA Gain Saturation Drive");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Experimental/Envelope");
            paramInfo->min_value = 1.0;
            paramInfo->max_value = 8.0;
            paramInfo->default_value = 3.0;
            break;
        case PARAM_VEG_DECAY_SEC:
            snprintf(paramInfo->name, sizeof(paramInfo->name), "VCA Decay Time");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Experimental/Envelope");
            paramInfo->min_value = 2.5;
            paramInfo->max_value = 5.0;
            paramInfo->default_value = 3.5;
            break;
        case PARAM_VCA_GATE_OFF_MS:
            snprintf(paramInfo->name, sizeof(paramInfo->name), "VCA Gate-Off Tail");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Experimental/Envelope");
            paramInfo->min_value = 1.0;
            paramInfo->max_value = 5.0;
            paramInfo->default_value = 3.0;
            break;
        case PARAM_VCA_GATE_OFF_ACCENT_MS:
            snprintf(paramInfo->name, sizeof(paramInfo->name), "VCA Gate-Off Tail (Accent)");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Experimental/Envelope");
            paramInfo->min_value = 1.0;
            paramInfo->max_value = 80.0;
            paramInfo->default_value = 3.0;
            break;
        case PARAM_FILTER_INPUT_COUPLING_HZ:
            snprintf(paramInfo->name, sizeof(paramInfo->name), "Filter Input Coupling Freq");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Experimental/Filter");
            paramInfo->min_value = 10.0;
            paramInfo->max_value = 30.0;
            paramInfo->default_value = 20.0;
            break;
        case PARAM_FILTER_OUTPUT_COUPLING_HZ:
            snprintf(paramInfo->name, sizeof(paramInfo->name), "Filter Output Coupling Freq");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Experimental/Filter");
            paramInfo->min_value = 10000.0;
            paramInfo->max_value = 25000.0;
            paramInfo->default_value = 20000.0;
            break;
        case PARAM_FILTER_CAP_SCALE_1:
            snprintf(paramInfo->name, sizeof(paramInfo->name), "Filter Ladder Pole Scale 1");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Experimental/Filter");
            paramInfo->min_value = 0.2;
            paramInfo->max_value = 4.0;
            paramInfo->default_value = 1.0;
            break;
        case PARAM_FILTER_CAP_SCALE_2:
            snprintf(paramInfo->name, sizeof(paramInfo->name), "Filter Ladder Pole Scale 2");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Experimental/Filter");
            paramInfo->min_value = 0.2;
            paramInfo->max_value = 4.0;
            paramInfo->default_value = 1.0;
            break;
        case PARAM_FILTER_CAP_SCALE_3:
            snprintf(paramInfo->name, sizeof(paramInfo->name), "Filter Ladder Pole Scale 3");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Experimental/Filter");
            paramInfo->min_value = 0.2;
            paramInfo->max_value = 4.0;
            paramInfo->default_value = 1.0;
            break;
        case PARAM_FILTER_CAP_SCALE_4:
            snprintf(paramInfo->name, sizeof(paramInfo->name), "Filter Ladder Pole Scale 4");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Experimental/Filter");
            paramInfo->min_value = 0.2;
            paramInfo->max_value = 4.0;
            paramInfo->default_value = 1.0;
            break;
        case PARAM_FILTER_LADDER_INPUT_SCALE:
            snprintf(paramInfo->name, sizeof(paramInfo->name), "Filter Ladder Input Drive");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Experimental/Filter");
            paramInfo->min_value = 0.02;
            paramInfo->max_value = 0.20;
            paramInfo->default_value = 0.05;
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
    *outValue = paramValues_[paramId];
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

void AcidusClap::onBeginEditFromGui(clap_id paramId) {
    if (paramId >= PARAM_COUNT) return;
    {
        std::lock_guard<std::mutex> lock(outEventQueueMutex_);
        outEventQueue_.push_back({ CLAP_EVENT_PARAM_GESTURE_BEGIN, paramId, 0.0, CLAP_EVENT_IS_LIVE });
    }
    requestHostFlush();
}

void AcidusClap::onParamValueFromGui(clap_id paramId, double value) {
    if (paramId >= PARAM_COUNT) return;
    paramValues_[paramId] = value;
    syncParamsToEngine();
    {
        std::lock_guard<std::mutex> lock(outEventQueueMutex_);
        outEventQueue_.push_back({ CLAP_EVENT_PARAM_VALUE, paramId, value, CLAP_EVENT_IS_LIVE });
    }
    requestHostFlush();
}

void AcidusClap::onEndEditFromGui(clap_id paramId) {
    if (paramId >= PARAM_COUNT) return;
    {
        std::lock_guard<std::mutex> lock(outEventQueueMutex_);
        outEventQueue_.push_back({ CLAP_EVENT_PARAM_GESTURE_END, paramId, 0.0, CLAP_EVENT_IS_LIVE });
    }
    requestHostFlush();
}

void AcidusClap::setParamValueFromGui(clap_id paramId, double value) {
    onParamValueFromGui(paramId, value);
}

void AcidusClap::pushPendingOutputEvents(const clap_output_events_t* out) {
    if (!out) return;
    std::vector<GuiParamEvent> pending;
    {
        std::lock_guard<std::mutex> lock(outEventQueueMutex_);
        pending.swap(outEventQueue_);
    }

    for (const auto& ev : pending) {
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

    if (paramId == PARAM_CUTOFF) {
        double norm = std::min(std::max(value, 0.0), 1.0);
        double hz = 200.0 * std::pow(12.5, norm);
        snprintf(outBuffer, outBufferCapacity, "%.1f Hz", hz);
    } else if (paramId == PARAM_WAVEFORM) {
        snprintf(outBuffer, outBufferCapacity, "%s", (value >= 0.5) ? "Square" : "Saw");
    } else if (paramId == PARAM_TUNE) {
        snprintf(outBuffer, outBufferCapacity, "%+.0f cents", value);
    } else if (paramId == PARAM_OSC_COUPLING_HZ || paramId == PARAM_RES_COUPLING_HZ
               || paramId == PARAM_FILTER_POST_HP_HZ || paramId == PARAM_FILTER_NOTCH_HZ
               || paramId == PARAM_FILTER_NOTCH_BANDWIDTH_HZ || paramId == PARAM_FILTER_ALLPASS_HZ
               || paramId == PARAM_FILTER_INPUT_COUPLING_HZ || paramId == PARAM_FILTER_OUTPUT_COUPLING_HZ) {
        snprintf(outBuffer, outBufferCapacity, "%.2f Hz", value);
    } else if (paramId == PARAM_VEG_DECAY_SEC) {
        snprintf(outBuffer, outBufferCapacity, "%.2f s", value);
    } else if (paramId == PARAM_VCA_GATE_OFF_MS || paramId == PARAM_VCA_GATE_OFF_ACCENT_MS) {
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
        double hz = std::atof(paramValueText);
        if (hz <= 200.0) *outValue = 0.0;
        else if (hz >= 2500.0) *outValue = 1.0;
        else *outValue = std::log(hz / 200.0) / std::log(12.5);
        return true;
    }
    *outValue = std::atof(paramValueText);
    return true;
}

void AcidusClap::paramsFlush(const clap_input_events_t* in, const clap_output_events_t* out) {
    if (in) {
        uint32_t size = in->size(in);
        for (uint32_t i = 0; i < size; ++i) {
            const clap_event_header_t* hdr = in->get(in, i);
            handleEvent(hdr);
        }
    }
    pushPendingOutputEvents(out);
}

bool AcidusClap::stateSave(const clap_ostream_t* stream) {
    if (!stream) return false;
    int64_t written = stream->write(stream, paramValues_, sizeof(paramValues_));
    return written == sizeof(paramValues_);
}

bool AcidusClap::stateLoad(const clap_istream_t* stream) {
    if (!stream) return false;
    double buffer[PARAM_COUNT];
    std::memcpy(buffer, paramValues_, sizeof(buffer));
    int64_t readBytes = stream->read(stream, buffer, sizeof(buffer));
    if (readBytes <= 0) return false;
    uint32_t numDoublesRead = static_cast<uint32_t>(readBytes) / sizeof(double);
    if (numDoublesRead > PARAM_COUNT) numDoublesRead = PARAM_COUNT;
    std::memcpy(paramValues_, buffer, numDoublesRead * sizeof(double));
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
    if (!clap_version_is_compatible(host->clap_version)) return nullptr;
    if (std::strcmp(plugin_id, g_acidusDescriptor.id) != 0) return nullptr;

    auto* plugin = new AcidusClap(host);
    return plugin->getClapPlugin();
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
    if (std::strcmp(factory_id, CLAP_PLUGIN_FACTORY_ID) == 0) {
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
