#include "SyrebasClap.hpp"
#include "gui/GuiWindow.hpp"
#include <cstring>
#include <cstdio>
#include <algorithm>

namespace syrebas {

// Forward declarations of GUI extension functions
extern const clap_plugin_gui_t g_syrebasGuiExtension;

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
        auto* self = static_cast<SyrebasClap*>(plugin->plugin_data);
        return self->paramsCount();
    },
    // get_info
    [](const clap_plugin_t* plugin, uint32_t param_index, clap_param_info_t* param_info) -> bool {
        auto* self = static_cast<SyrebasClap*>(plugin->plugin_data);
        return self->paramsInfo(param_index, param_info);
    },
    // get_value
    [](const clap_plugin_t* plugin, clap_id param_id, double* out_value) -> bool {
        auto* self = static_cast<SyrebasClap*>(plugin->plugin_data);
        return self->paramsValue(param_id, out_value);
    },
    // value_to_text
    [](const clap_plugin_t* plugin, clap_id param_id, double value, char* out_buffer, uint32_t out_buffer_capacity) -> bool {
        auto* self = static_cast<SyrebasClap*>(plugin->plugin_data);
        return self->paramsValueToText(param_id, value, out_buffer, out_buffer_capacity);
    },
    // text_to_value
    [](const clap_plugin_t* plugin, clap_id param_id, const char* param_value_text, double* out_value) -> bool {
        auto* self = static_cast<SyrebasClap*>(plugin->plugin_data);
        return self->paramsTextToValue(param_id, param_value_text, out_value);
    },
    // flush
    [](const clap_plugin_t* plugin, const clap_input_events_t* in, const clap_output_events_t* out) {
        auto* self = static_cast<SyrebasClap*>(plugin->plugin_data);
        self->paramsFlush(in, out);
    }
};

static const clap_plugin_state_t g_stateExtension = {
    // save
    [](const clap_plugin_t* plugin, const clap_ostream_t* stream) -> bool {
        auto* self = static_cast<SyrebasClap*>(plugin->plugin_data);
        return self->stateSave(stream);
    },
    // load
    [](const clap_plugin_t* plugin, const clap_istream_t* stream) -> bool {
        auto* self = static_cast<SyrebasClap*>(plugin->plugin_data);
        return self->stateLoad(stream);
    }
};

SyrebasClap::SyrebasClap(const clap_host_t* host) : host_(host) {
    clapPlugin_.desc = nullptr;
    clapPlugin_.plugin_data = this;
    clapPlugin_.init = [](const clap_plugin_t* plugin) -> bool {
        return static_cast<SyrebasClap*>(plugin->plugin_data)->init();
    };
    clapPlugin_.destroy = [](const clap_plugin_t* plugin) {
        static_cast<SyrebasClap*>(plugin->plugin_data)->destroy();
    };
    clapPlugin_.activate = [](const clap_plugin_t* plugin, double sample_rate, uint32_t min_frames, uint32_t max_frames) -> bool {
        return static_cast<SyrebasClap*>(plugin->plugin_data)->activate(sample_rate, min_frames, max_frames);
    };
    clapPlugin_.deactivate = [](const clap_plugin_t* plugin) {
        static_cast<SyrebasClap*>(plugin->plugin_data)->deactivate();
    };
    clapPlugin_.start_processing = [](const clap_plugin_t* plugin) -> bool {
        return static_cast<SyrebasClap*>(plugin->plugin_data)->startProcessing();
    };
    clapPlugin_.stop_processing = [](const clap_plugin_t* plugin) {
        static_cast<SyrebasClap*>(plugin->plugin_data)->stopProcessing();
    };
    clapPlugin_.reset = [](const clap_plugin_t* plugin) {
        static_cast<SyrebasClap*>(plugin->plugin_data)->reset();
    };
    clapPlugin_.process = [](const clap_plugin_t* plugin, const clap_process_t* process) -> clap_process_status {
        return static_cast<SyrebasClap*>(plugin->plugin_data)->process(process);
    };
    clapPlugin_.get_extension = [](const clap_plugin_t* plugin, const char* id) -> const void* {
        return static_cast<SyrebasClap*>(plugin->plugin_data)->getExtension(id);
    };
    clapPlugin_.on_main_thread = [](const clap_plugin_t* plugin) {
        static_cast<SyrebasClap*>(plugin->plugin_data)->onMainThread();
    };

    // Initialize default parameter values
    paramValues_[PARAM_CUTOFF] = 800.0;
    paramValues_[PARAM_RESONANCE] = 0.5;
    paramValues_[PARAM_ENV_MOD] = 0.5;
    paramValues_[PARAM_DECAY] = 0.5;
    paramValues_[PARAM_ACCENT] = 0.5;
    paramValues_[PARAM_WAVEFORM] = 0.0; // 0 = Saw, 1 = Square
    paramValues_[PARAM_VOLUME] = 0.8;

    syncParamsToEngine();
}

bool SyrebasClap::init() {
    return true;
}

void SyrebasClap::destroy() {
    destroyGuiWindow();
    delete this;
}

void SyrebasClap::createGuiWindow() {
    if (!guiWindow_) {
        guiWindow_ = std::make_unique<GuiWindow>(this);
    }
}

void SyrebasClap::destroyGuiWindow() {
    guiWindow_.reset();
}

bool SyrebasClap::activate(double sampleRate, uint32_t minFrames, uint32_t maxFrames) {
    engine_.setSampleRate(sampleRate);
    engine_.reset();
    return true;
}

void SyrebasClap::deactivate() {}

bool SyrebasClap::startProcessing() {
    return true;
}

void SyrebasClap::stopProcessing() {}

void SyrebasClap::reset() {
    engine_.reset();
}

void SyrebasClap::syncParamsToEngine() {
    auto& params = engine_.getParams();
    params.cutoff = static_cast<float>(paramValues_[PARAM_CUTOFF]);
    params.resonance = static_cast<float>(paramValues_[PARAM_RESONANCE]);
    params.envMod = static_cast<float>(paramValues_[PARAM_ENV_MOD]);
    params.decay = static_cast<float>(paramValues_[PARAM_DECAY]);
    params.accent = static_cast<float>(paramValues_[PARAM_ACCENT]);
    params.waveform = (paramValues_[PARAM_WAVEFORM] >= 0.5) ? Waveform::Square : Waveform::Saw;
    params.masterVolume = static_cast<float>(paramValues_[PARAM_VOLUME]);
}

void SyrebasClap::handleEvent(const clap_event_header_t* header) {
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
        uint8_t note = midiEv->data[1];
        uint8_t vel = midiEv->data[2];

        if (status == 0x90 && vel > 0) {
            engine_.noteOn(note, static_cast<float>(vel) / 127.0f);
        } else if (status == 0x80 || (status == 0x90 && vel == 0)) {
            engine_.noteOff(note);
        }
    } else if (header->type == CLAP_EVENT_PARAM_VALUE) {
        const auto* paramEv = reinterpret_cast<const clap_event_param_value_t*>(header);
        if (paramEv->param_id < PARAM_COUNT) {
            paramValues_[paramEv->param_id] = paramEv->value;
            syncParamsToEngine();
        }
    }
}

clap_process_status SyrebasClap::process(const clap_process_t* process) {
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
        // Handle events scheduled at or before current frame
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

    return CLAP_PROCESS_CONTINUE;
}

const void* SyrebasClap::getExtension(const char* id) {
    if (std::strcmp(id, CLAP_EXT_NOTE_PORTS) == 0) return &g_notePortsExtension;
    if (std::strcmp(id, CLAP_EXT_AUDIO_PORTS) == 0) return &g_audioPortsExtension;
    if (std::strcmp(id, CLAP_EXT_PARAMS) == 0) return &g_paramsExtension;
    if (std::strcmp(id, CLAP_EXT_STATE) == 0) return &g_stateExtension;
    if (std::strcmp(id, CLAP_EXT_GUI) == 0) return &g_syrebasGuiExtension;
    return nullptr;
}

void SyrebasClap::onMainThread() {}

uint32_t SyrebasClap::paramsCount() const {
    return PARAM_COUNT;
}

bool SyrebasClap::paramsInfo(uint32_t paramIndex, clap_param_info_t* paramInfo) const {
    if (paramIndex >= PARAM_COUNT) return false;

    std::memset(paramInfo, 0, sizeof(*paramInfo));
    paramInfo->id = paramIndex;
    paramInfo->flags = CLAP_PARAM_IS_AUTOMATABLE;

    switch (paramIndex) {
        case PARAM_CUTOFF:
            snprintf(paramInfo->name, sizeof(paramInfo->name), "Cutoff Freq");
            snprintf(paramInfo->module, sizeof(paramInfo->module), "Filter");
            paramInfo->min_value = 300.0;
            paramInfo->max_value = 10000.0;
            paramInfo->default_value = 800.0;
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
        default:
            return false;
    }
    return true;
}

bool SyrebasClap::paramsValue(clap_id paramId, double* outValue) {
    if (paramId >= PARAM_COUNT || !outValue) return false;
    *outValue = paramValues_[paramId];
    return true;
}

bool SyrebasClap::paramsValueToText(clap_id paramId, double value, char* outBuffer, uint32_t outBufferCapacity) {
    if (paramId >= PARAM_COUNT || !outBuffer || outBufferCapacity == 0) return false;

    if (paramId == PARAM_CUTOFF) {
        snprintf(outBuffer, outBufferCapacity, "%.1f Hz", value);
    } else if (paramId == PARAM_WAVEFORM) {
        snprintf(outBuffer, outBufferCapacity, "%s", (value >= 0.5) ? "Square" : "Saw");
    } else {
        snprintf(outBuffer, outBufferCapacity, "%.2f", value);
    }
    return true;
}

bool SyrebasClap::paramsTextToValue(clap_id paramId, const char* paramValueText, double* outValue) {
    if (paramId >= PARAM_COUNT || !paramValueText || !outValue) return false;
    if (paramId == PARAM_WAVEFORM) {
        if (std::strstr(paramValueText, "Square") || std::strstr(paramValueText, "square")) {
            *outValue = 1.0;
        } else {
            *outValue = 0.0;
        }
        return true;
    }
    *outValue = std::atof(paramValueText);
    return true;
}

void SyrebasClap::paramsFlush(const clap_input_events_t* in, const clap_output_events_t* out) {
    if (!in) return;
    uint32_t size = in->size(in);
    for (uint32_t i = 0; i < size; ++i) {
        const clap_event_header_t* hdr = in->get(in, i);
        handleEvent(hdr);
    }
}

bool SyrebasClap::stateSave(const clap_ostream_t* stream) {
    if (!stream) return false;
    int64_t written = stream->write(stream, paramValues_, sizeof(paramValues_));
    return written == sizeof(paramValues_);
}

bool SyrebasClap::stateLoad(const clap_istream_t* stream) {
    if (!stream) return false;
    int64_t readBytes = stream->read(stream, paramValues_, sizeof(paramValues_));
    if (readBytes == sizeof(paramValues_)) {
        syncParamsToEngine();
        return true;
    }
    return false;
}

// CLAP Plugin Entry Point
static const char* g_syrebasFeatures[] = {
    CLAP_PLUGIN_FEATURE_INSTRUMENT,
    CLAP_PLUGIN_FEATURE_SYNTHESIZER,
    CLAP_PLUGIN_FEATURE_STEREO,
    nullptr
};

static const clap_plugin_descriptor_t g_syrebasDescriptor = {
    CLAP_VERSION,
    "com.syrebas.synth",
    "Syrebas",
    "Syrebas Synth",
    "https://github.com/syrebas/syrebas",
    "",
    "",
    "1.0.0",
    "Roland TB-303 Bass Synth Emulator",
    g_syrebasFeatures
};

static uint32_t clap_factory_get_plugin_count(const clap_plugin_factory_t* factory) {
    return 1;
}

static const clap_plugin_descriptor_t* clap_factory_get_plugin_descriptor(const clap_plugin_factory_t* factory, uint32_t index) {
    return (index == 0) ? &g_syrebasDescriptor : nullptr;
}

static const clap_plugin_t* clap_factory_create_plugin(const clap_plugin_factory_t* factory, const clap_host_t* host, const char* plugin_id) {
    if (!clap_version_is_compatible(host->clap_version)) return nullptr;
    if (std::strcmp(plugin_id, g_syrebasDescriptor.id) != 0) return nullptr;

    auto* plugin = new SyrebasClap(host);
    return plugin->getClapPlugin();
}

static const clap_plugin_factory_t g_syrebasFactory = {
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
        return &g_syrebasFactory;
    }
    return nullptr;
}

} // namespace syrebas

extern "C" CLAP_EXPORT const clap_plugin_entry_t clap_entry = {
    CLAP_VERSION,
    syrebas::entry_init,
    syrebas::entry_deinit,
    syrebas::entry_get_factory
};
