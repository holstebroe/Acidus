#include "SequencerClap.hpp"
#include "SequencerGui.hpp"
#include <clap/ext/state.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace acidus {
namespace seq {

// A C++ exception must never cross the plugin boundary into the host.
template <class R, class F>
static R guarded(R fallback, F&& f) noexcept {
    try { return f(); } catch (...) { return fallback; }
}
template <class F>
static void guardedVoid(F&& f) noexcept {
    try { f(); } catch (...) {}
}

static SequencerClap* self(const clap_plugin_t* plugin) {
    return static_cast<SequencerClap*>(plugin->plugin_data);
}

static constexpr uint32_t kMaxBlockEvents = 1024;

static const clap_plugin_note_ports_t g_notePorts = {
    [](const clap_plugin_t*, bool) -> uint32_t { return 1; },
    [](const clap_plugin_t*, uint32_t index, bool isInput, clap_note_port_info_t* info) -> bool {
        if (index != 0 || !info) return false;
        info->id = 0;
        std::snprintf(info->name, sizeof(info->name), isInput ? "Pattern Triggers" : "Notes Out");
        info->supported_dialects = CLAP_NOTE_DIALECT_CLAP | CLAP_NOTE_DIALECT_MIDI;
        info->preferred_dialect = CLAP_NOTE_DIALECT_CLAP;
        return true;
    }
};

// A silent stereo output: some hosts only load an instrument that has one.
static const clap_plugin_audio_ports_t g_audioPorts = {
    [](const clap_plugin_t*, bool isInput) -> uint32_t { return isInput ? 0 : 1; },
    [](const clap_plugin_t*, uint32_t index, bool isInput, clap_audio_port_info_t* info) -> bool {
        if (isInput || index != 0 || !info) return false;
        info->id = 0;
        std::snprintf(info->name, sizeof(info->name), "Audio Output");
        info->channel_count = 2;
        info->flags = CLAP_AUDIO_PORT_IS_MAIN;
        info->port_type = CLAP_PORT_STEREO;
        info->in_place_pair = CLAP_INVALID_ID;
        return true;
    }
};

static const clap_plugin_params_t g_params = {
    [](const clap_plugin_t*) -> uint32_t { return 1; },
    [](const clap_plugin_t* p, uint32_t i, clap_param_info_t* info) -> bool {
        return guarded(false, [&] { return self(p)->paramsInfo(i, info); });
    },
    [](const clap_plugin_t* p, clap_id id, double* v) -> bool {
        return guarded(false, [&] { return self(p)->paramsValue(id, v); });
    },
    [](const clap_plugin_t* p, clap_id id, double v, char* buf, uint32_t size) -> bool {
        return guarded(false, [&] { return self(p)->paramsValueToText(id, v, buf, size); });
    },
    [](const clap_plugin_t* p, clap_id id, const char* text, double* v) -> bool {
        return guarded(false, [&] { return self(p)->paramsTextToValue(id, text, v); });
    },
    [](const clap_plugin_t* p, const clap_input_events_t* in, const clap_output_events_t* out) {
        guardedVoid([&] { self(p)->paramsFlush(in, out); });
    }
};

static const clap_plugin_state_t g_state = {
    [](const clap_plugin_t* p, const clap_ostream_t* s) -> bool {
        return guarded(false, [&] { return self(p)->stateSave(s); });
    },
    [](const clap_plugin_t* p, const clap_istream_t* s) -> bool {
        return guarded(false, [&] { return self(p)->stateLoad(s); });
    }
};

SequencerClap::SequencerClap(const clap_host_t* host) : host_(host) {
    triggers_.reserve(kMaxBlockEvents);
    notes_.resize(kMaxBlockEvents);

    clapPlugin_.desc = nullptr;
    clapPlugin_.plugin_data = this;
    clapPlugin_.init = [](const clap_plugin_t*) -> bool { return true; };
    clapPlugin_.destroy = [](const clap_plugin_t* p) {
        guardedVoid([&] { delete self(p); });
    };
    clapPlugin_.activate = [](const clap_plugin_t* p, double sr, uint32_t, uint32_t) -> bool {
        return guarded(false, [&] { return self(p)->activate(sr); });
    };
    clapPlugin_.deactivate = [](const clap_plugin_t*) {};
    clapPlugin_.start_processing = [](const clap_plugin_t*) -> bool { return true; };
    clapPlugin_.stop_processing = [](const clap_plugin_t*) {};
    clapPlugin_.reset = [](const clap_plugin_t* p) {
        guardedVoid([&] { self(p)->reset(); });
    };
    clapPlugin_.process = [](const clap_plugin_t* p, const clap_process_t* pr) -> clap_process_status {
        return guarded<clap_process_status>(CLAP_PROCESS_CONTINUE, [&] { return self(p)->process(pr); });
    };
    clapPlugin_.get_extension = [](const clap_plugin_t* p, const char* id) -> const void* {
        return guarded<const void*>(nullptr, [&] { return self(p)->getExtension(id); });
    };
    clapPlugin_.on_main_thread = [](const clap_plugin_t* p) {
        guardedVoid([&] { self(p)->onMainThread(); });
    };
}

SequencerClap::~SequencerClap() {
    destroyGui();
}

bool SequencerClap::activate(double sampleRate) {
    engine_.setSampleRate(sampleRate);
    engine_.reset();
    return true;
}

void SequencerClap::reset() {
    engine_.reset();
}

void SequencerClap::createGui() {
    if (!gui_) gui_ = std::make_unique<SequencerGui>(this);
}

void SequencerClap::destroyGui() {
    gui_.reset();
}

void SequencerClap::markStateDirty() {
    stateDirty_.store(true);
    if (host_ && host_->request_callback) host_->request_callback(host_);
}

void SequencerClap::setEditPattern(int p) {
    // Saved with the state, but only a view setting: following the playing
    // pattern must not mark the project modified.
    if (p >= 0 && p < kNumPatterns) editPattern_.store(p, std::memory_order_relaxed);
}

void SequencerClap::onMainThread() {
    if (stateDirty_.exchange(false) && host_) {
        const auto* hostState = static_cast<const clap_host_state_t*>(host_->get_extension(host_, CLAP_EXT_STATE));
        if (hostState && hostState->mark_dirty) hostState->mark_dirty(host_);
    }
}

// Is this incoming event a pattern trigger? Fills `trig` if so.
static bool asTrigger(const clap_event_header_t* h, TriggerEvent& trig) {
    if (h->type == CLAP_EVENT_NOTE_ON || h->type == CLAP_EVENT_NOTE_OFF || h->type == CLAP_EVENT_NOTE_CHOKE) {
        if (h->size < sizeof(clap_event_note_t)) return false;
        const auto* n = reinterpret_cast<const clap_event_note_t*>(h);
        if (n->key < kFirstTriggerKey || n->key >= kFirstTriggerKey + kNumPatterns) return false;
        trig = { h->time, h->type == CLAP_EVENT_NOTE_ON, n->key };
        return true;
    }
    if (h->type == CLAP_EVENT_MIDI) {
        if (h->size < sizeof(clap_event_midi_t)) return false;
        const auto* m = reinterpret_cast<const clap_event_midi_t*>(h);
        const uint8_t status = m->data[0] & 0xF0;
        const int key = m->data[1];
        if (status != 0x90 && status != 0x80) return false;
        if (key < kFirstTriggerKey || key >= kFirstTriggerKey + kNumPatterns) return false;
        trig = { h->time, status == 0x90 && m->data[2] > 0, key };
        return true;
    }
    return false;
}

static bool isPassThroughType(uint16_t type) {
    switch (type) {
        case CLAP_EVENT_NOTE_ON:
        case CLAP_EVENT_NOTE_OFF:
        case CLAP_EVENT_NOTE_CHOKE:
        case CLAP_EVENT_NOTE_END:
        case CLAP_EVENT_NOTE_EXPRESSION:
        case CLAP_EVENT_MIDI:
        case CLAP_EVENT_MIDI_SYSEX:
        case CLAP_EVENT_MIDI2:
            return true;
        default:
            return false;
    }
}

clap_process_status SequencerClap::process(const clap_process_t* process) {
    if (!process) return CLAP_PROCESS_CONTINUE;
    const uint32_t frames = process->frames_count;

    // Silent audio output.
    for (uint32_t o = 0; process->audio_outputs && o < process->audio_outputs_count; ++o) {
        const clap_audio_buffer_t& b = process->audio_outputs[o];
        for (uint32_t c = 0; c < b.channel_count; ++c) {
            if (b.data32 && b.data32[c]) std::memset(b.data32[c], 0, frames * sizeof(float));
            else if (b.data64 && b.data64[c]) std::memset(b.data64[c], 0, frames * sizeof(double));
        }
    }

    TransportInfo transport;
    if (const clap_event_transport_t* tr = process->transport) {
        transport.playing = (tr->flags & CLAP_TRANSPORT_IS_PLAYING) != 0;
        transport.hasTempo = (tr->flags & CLAP_TRANSPORT_HAS_TEMPO) != 0;
        transport.tempo = tr->tempo;
        transport.hasBeats = (tr->flags & CLAP_TRANSPORT_HAS_BEATS_TIMELINE) != 0;
        transport.songPosBeats = static_cast<double>(tr->song_pos_beats) / static_cast<double>(CLAP_BEATTIME_FACTOR);
    }

    const clap_input_events_t* in = process->in_events;
    const uint32_t inCount = in ? in->size(in) : 0;
    triggers_.clear();
    // A transpose set outside the audio thread (GUI, state load) applies
    // from the start of this block.
    const int pendingTranspose = globalTranspose();
    if (pendingTranspose != engine_.globalTranspose()) {
        triggers_.push_back({ 0, false, -1, true, pendingTranspose });
    }
    for (uint32_t i = 0; i < inCount; ++i) {
        const clap_event_header_t* h = in->get(in, i);
        if (!h || h->space_id != CLAP_CORE_EVENT_SPACE_ID) continue;
        TriggerEvent t;
        int semitones;
        if (transposeEvent(h, semitones)) {
            // Host automation: sample-accurate.
            globalTranspose_.store(semitones, std::memory_order_relaxed);
            t = { h->time, false, -1, true, semitones };
            t.time = std::min(t.time, frames > 0 ? frames - 1 : 0);
            if (!triggers_.empty() && t.time < triggers_.back().time) t.time = triggers_.back().time;
            if (triggers_.size() < kMaxBlockEvents) triggers_.push_back(t);
            continue;
        }
        if (asTrigger(h, t) && triggers_.size() < kMaxBlockEvents) {
            t.time = std::min(t.time, frames > 0 ? frames - 1 : 0);
            // Keep the list sorted even if the host's isn't.
            if (!triggers_.empty() && t.time < triggers_.back().time) t.time = triggers_.back().time;
            triggers_.push_back(t);
        }
    }

    const uint32_t noteCount = engine_.process(frames, transport, triggers_.data(),
                                               static_cast<uint32_t>(triggers_.size()),
                                               notes_.data(), static_cast<uint32_t>(notes_.size()));

    // Merge the sequencer's notes with the passed-through events, in time order.
    const clap_output_events_t* out = process->out_events;
    if (!out || !out->try_push) return CLAP_PROCESS_CONTINUE;
    pushParamOut(out);   // time 0: ahead of everything else
    uint32_t n = 0;
    auto pushNote = [&](const NoteEvent& e) {
        clap_event_note_t ev{};
        ev.header.size = sizeof(ev);
        ev.header.time = e.time;
        ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
        ev.header.type = e.on ? CLAP_EVENT_NOTE_ON : CLAP_EVENT_NOTE_OFF;
        ev.header.flags = 0;
        ev.note_id = -1;
        ev.port_index = 0;
        ev.channel = 0;
        ev.key = static_cast<int16_t>(e.key);
        ev.velocity = e.velocity;
        out->try_push(out, &ev.header);
    };
    for (uint32_t i = 0; i < inCount; ++i) {
        const clap_event_header_t* h = in->get(in, i);
        if (!h || h->space_id != CLAP_CORE_EVENT_SPACE_ID || !isPassThroughType(h->type)) continue;
        TriggerEvent t;
        if (asTrigger(h, t)) continue;
        while (n < noteCount && notes_[n].time <= h->time) pushNote(notes_[n++]);
        out->try_push(out, h);
    }
    while (n < noteCount) pushNote(notes_[n++]);
    return CLAP_PROCESS_CONTINUE;
}

const void* SequencerClap::getExtension(const char* id) {
    if (!id) return nullptr;
    if (std::strcmp(id, CLAP_EXT_NOTE_PORTS) == 0) return &g_notePorts;
    if (std::strcmp(id, CLAP_EXT_AUDIO_PORTS) == 0) return &g_audioPorts;
    if (std::strcmp(id, CLAP_EXT_STATE) == 0) return &g_state;
    if (std::strcmp(id, CLAP_EXT_PARAMS) == 0) return &g_params;
    if (std::strcmp(id, CLAP_EXT_GUI) == 0) return &g_sequencerGuiExtension;
    return nullptr;
}

// State: the pattern bank (Pattern.cpp), then the edited pattern, the
// follow flag and the global transpose (int8).
bool SequencerClap::stateSave(const clap_ostream_t* stream) {
    if (!stream || !stream->write) return false;
    std::string data = bank_.serialize();
    data += static_cast<char>(editPattern());
    data += static_cast<char>(followPlaying() ? 1 : 0);
    data += static_cast<char>(static_cast<int8_t>(globalTranspose()));
    size_t done = 0;
    while (done < data.size()) {
        const int64_t w = stream->write(stream, data.data() + done, data.size() - done);
        if (w <= 0 || static_cast<size_t>(w) > data.size() - done) return false;
        done += static_cast<size_t>(w);
    }
    return true;
}

bool SequencerClap::stateLoad(const clap_istream_t* stream) {
    if (!stream || !stream->read) return false;
    std::vector<uint8_t> data;
    uint8_t chunk[1024];
    while (data.size() < (1u << 16)) {
        int64_t r = stream->read(stream, chunk, sizeof(chunk));
        if (r < 0) return false;
        if (r == 0) break;
        r = std::min<int64_t>(r, sizeof(chunk));
        data.insert(data.end(), chunk, chunk + r);
    }
    const size_t used = bank_.deserialize(data.data(), data.size());
    if (used == 0) return false;
    if (data.size() >= used + 2) {
        editPattern_.store(std::min<int>(data[used], kNumPatterns - 1));
        follow_.store(data[used + 1] != 0);
    }
    int transpose = 0;
    if (data.size() >= used + 3) transpose = static_cast<int8_t>(data[used + 2]);
    globalTranspose_.store(std::min(std::max(transpose, kMinTranspose), kMaxTranspose));
    return true;
}

// --- Global transpose parameter ---------------------------------------------

bool SequencerClap::transposeEvent(const clap_event_header_t* h, int& semitones) {
    if (h->type != CLAP_EVENT_PARAM_VALUE || h->size < sizeof(clap_event_param_value_t)) return false;
    const auto* ev = reinterpret_cast<const clap_event_param_value_t*>(h);
    if (ev->param_id != kParamTranspose || !std::isfinite(ev->value)) return false;
    semitones = static_cast<int>(std::lround(std::min(std::max(ev->value, double(kMinTranspose)), double(kMaxTranspose))));
    return true;
}

bool SequencerClap::paramsInfo(uint32_t index, clap_param_info_t* info) const {
    if (index != 0 || !info) return false;
    std::memset(info, 0, sizeof(*info));
    info->id = kParamTranspose;
    info->flags = CLAP_PARAM_IS_AUTOMATABLE | CLAP_PARAM_IS_STEPPED;
    std::snprintf(info->name, sizeof(info->name), "Key Transpose");
    info->min_value = kMinTranspose;
    info->max_value = kMaxTranspose;
    info->default_value = 0.0;
    return true;
}

bool SequencerClap::paramsValue(clap_id id, double* value) const {
    if (id != kParamTranspose || !value) return false;
    *value = globalTranspose();
    return true;
}

bool SequencerClap::paramsValueToText(clap_id id, double value, char* buf, uint32_t size) const {
    if (id != kParamTranspose || !buf || size == 0 || !std::isfinite(value)) return false;
    std::snprintf(buf, size, "%+d st", static_cast<int>(std::lround(value)));
    return true;
}

bool SequencerClap::paramsTextToValue(clap_id id, const char* text, double* value) const {
    if (id != kParamTranspose || !text || !value) return false;
    char* end = nullptr;
    const double v = std::strtod(text, &end);
    if (end == text || !std::isfinite(v)) return false;
    *value = std::min(std::max(std::round(v), double(kMinTranspose)), double(kMaxTranspose));
    return true;
}

void SequencerClap::paramsFlush(const clap_input_events_t* in, const clap_output_events_t* out) {
    const uint32_t count = in ? in->size(in) : 0;
    for (uint32_t i = 0; i < count; ++i) {
        const clap_event_header_t* h = in->get(in, i);
        int semitones;
        if (h && h->space_id == CLAP_CORE_EVENT_SPACE_ID && transposeEvent(h, semitones)) {
            globalTranspose_.store(semitones, std::memory_order_relaxed);   // the next block applies it
        }
    }
    if (out && out->try_push) pushParamOut(out);
}

void SequencerClap::queueParamOut(uint16_t type, double value) {
    std::lock_guard<std::mutex> lock(paramOutMutex_);
    if (paramOutCount_ < static_cast<int>(sizeof(paramOut_) / sizeof(paramOut_[0]))) {
        paramOut_[paramOutCount_++] = { type, value };
    }
}

void SequencerClap::pushParamOut(const clap_output_events_t* out) {
    // Audio or main thread; never wait for the GUI.
    std::unique_lock<std::mutex> lock(paramOutMutex_, std::try_to_lock);
    if (!lock.owns_lock()) return;
    for (int i = 0; i < paramOutCount_; ++i) {
        if (paramOut_[i].type == CLAP_EVENT_PARAM_VALUE) {
            clap_event_param_value_t ev{};
            ev.header = { sizeof(ev), 0, CLAP_CORE_EVENT_SPACE_ID, CLAP_EVENT_PARAM_VALUE, 0 };
            ev.param_id = kParamTranspose;
            ev.cookie = nullptr;
            ev.note_id = -1; ev.port_index = -1; ev.channel = -1; ev.key = -1;
            ev.value = paramOut_[i].value;
            out->try_push(out, &ev.header);
        } else {
            clap_event_param_gesture_t ev{};
            ev.header = { sizeof(ev), 0, CLAP_CORE_EVENT_SPACE_ID, paramOut_[i].type, 0 };
            ev.param_id = kParamTranspose;
            out->try_push(out, &ev.header);
        }
    }
    paramOutCount_ = 0;
}

void SequencerClap::requestFlush() {
    if (!host_ || !host_->get_extension) return;
    const auto* hp = static_cast<const clap_host_params_t*>(host_->get_extension(host_, CLAP_EXT_PARAMS));
    if (hp && hp->request_flush) hp->request_flush(host_);
    else if (host_->request_process) host_->request_process(host_);
}

void SequencerClap::beginTransposeEdit() {
    queueParamOut(CLAP_EVENT_PARAM_GESTURE_BEGIN, 0.0);
    requestFlush();
}

void SequencerClap::setTransposeFromGui(int semitones) {
    semitones = std::min(std::max(semitones, kMinTranspose), kMaxTranspose);
    if (globalTranspose_.exchange(semitones) == semitones) return;
    queueParamOut(CLAP_EVENT_PARAM_VALUE, semitones);
    requestFlush();
    markStateDirty();
}

void SequencerClap::endTransposeEdit() {
    queueParamOut(CLAP_EVENT_PARAM_GESTURE_END, 0.0);
    requestFlush();
}

static const char* g_features[] = {
    CLAP_PLUGIN_FEATURE_INSTRUMENT,
    CLAP_PLUGIN_FEATURE_NOTE_EFFECT,
    nullptr
};

static const clap_plugin_descriptor_t g_descriptor = {
    CLAP_VERSION,
    "com.acidus.burette",
    "Burette",
    "Acidus",
    "https://github.com/holstebroe/Acidus",
    "",
    "",
    "1.0.0",
    "TB-303 style pattern sequencer: MIDI out for Acidus",
    g_features
};

static uint32_t factoryCount(const clap_plugin_factory_t*) { return 1; }

static const clap_plugin_descriptor_t* factoryDescriptor(const clap_plugin_factory_t*, uint32_t index) {
    return index == 0 ? &g_descriptor : nullptr;
}

static const clap_plugin_t* factoryCreate(const clap_plugin_factory_t*, const clap_host_t* host, const char* id) {
    if (!host || !id || !clap_version_is_compatible(host->clap_version)) return nullptr;
    if (std::strcmp(id, g_descriptor.id) != 0) return nullptr;
    try {
        auto* plugin = new SequencerClap(host);
        return plugin->getClapPlugin();
    } catch (...) {
        return nullptr;
    }
}

static const clap_plugin_factory_t g_factory = { factoryCount, factoryDescriptor, factoryCreate };

static bool entryInit(const char*) { return true; }
static void entryDeinit() {}
static const void* entryGetFactory(const char* id) {
    return (id && std::strcmp(id, CLAP_PLUGIN_FACTORY_ID) == 0) ? &g_factory : nullptr;
}

} // namespace seq
} // namespace acidus

extern "C" CLAP_EXPORT const clap_plugin_entry_t clap_entry = {
    CLAP_VERSION,
    acidus::seq::entryInit,
    acidus::seq::entryDeinit,
    acidus::seq::entryGetFactory
};
