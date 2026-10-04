// Robustness tests: the plugin must survive hostile input -- non-finite and
// out-of-range parameters, absurd sample rates, malformed events, corrupt state
// and degenerate GUI input -- without crashing, and keep its output finite.
#include "clap/AcidusClap.hpp"
#include "gui/GuiWindow.hpp"
#include "core/SynthEngine.hpp"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <random>
#include <vector>

static int g_failures = 0;
static void check(bool ok, const char* what) {
    if (!ok) { std::printf("FAIL: %s\n", what); ++g_failures; }
}

static bool finiteAndBounded(const std::vector<float>& v, float limit = 16.0f) {
    for (float x : v) if (!std::isfinite(x) || std::abs(x) > limit) return false;
    return true;
}

// Every float field of SynthParameters set to `v`.
static void fillParams(acidus::SynthParameters& p, float v) {
    float* f = reinterpret_cast<float*>(&p);
    const acidus::Waveform wf = p.waveform;
    for (size_t i = 0; i < sizeof(p) / sizeof(float); ++i) {
        if (static_cast<void*>(f + i) == static_cast<void*>(&p.waveform)) continue;
        f[i] = v;
    }
    p.waveform = wf;
}

static void testEngineExtremes() {
    const float nan = std::numeric_limits<float>::quiet_NaN();
    const float inf = std::numeric_limits<float>::infinity();
    const float values[] = { nan, inf, -inf, 0.0f, -1.0f, 1.0e30f, -1.0e30f, 1.0e-30f, 1000.0f };
    for (float v : values) {
        acidus::SynthEngine e;
        fillParams(e.getParams(), v);
        std::vector<float> l(2048), r(2048);
        e.noteOn(36, 1.0f);
        e.processAudio(l.data(), r.data(), 2048);
        e.noteOn(48, 0.5f);   // slide
        e.processAudio(l.data(), r.data(), 2048);
        e.noteOff(48);
        e.processAudio(l.data(), r.data(), 2048);
        check(finiteAndBounded(l) && finiteAndBounded(r), "engine output finite for extreme parameter value");
    }
}

static void testEngineRandomParams() {
    std::mt19937 rng(1234);
    std::uniform_real_distribution<float> uni(-2.0f, 2.0f);
    std::uniform_int_distribution<int> note(-5, 140);
    acidus::SynthEngine e;
    std::vector<float> l(256), r(256);
    bool ok = true;
    for (int iter = 0; iter < 300; ++iter) {
        float* f = reinterpret_cast<float*>(&e.getParams());
        for (size_t i = 0; i < sizeof(acidus::SynthParameters) / sizeof(float); ++i) {
            if (static_cast<void*>(f + i) == static_cast<void*>(&e.getParams().waveform)) continue;
            f[i] = uni(rng) * ((rng() % 4 == 0) ? 1000.0f : 1.0f);
        }
        e.getParams().waveform = (rng() & 1) ? acidus::Waveform::Square : acidus::Waveform::Saw;
        e.noteOn(note(rng), uni(rng));
        e.processAudio(l.data(), r.data(), 256);
        if (rng() % 3 == 0) e.noteOff(note(rng));
        ok = ok && finiteAndBounded(l) && finiteAndBounded(r);
    }
    check(ok, "engine output finite under random parameters/notes");
}

static void testSampleRates() {
    const double rates[] = { 0.0, -44100.0, 1.0, 100.0, 8000.0, 22050.0, 192000.0, 1.0e9,
                             std::numeric_limits<double>::quiet_NaN(),
                             std::numeric_limits<double>::infinity() };
    for (double sr : rates) {
        acidus::SynthEngine e;
        e.setSampleRate(sr);
        std::vector<float> l(1024), r(1024);
        e.noteOn(127, 1.0f);
        e.processAudio(l.data(), r.data(), 1024);
        check(finiteAndBounded(l) && finiteAndBounded(r), "engine output finite at odd sample rate");
    }
    acidus::SynthEngine e;
    e.processAudio(nullptr, nullptr, 128);   // no buffers
    e.processAudio(nullptr, nullptr, -5);
    e.noteOff(-1);
}

struct Events {
    std::vector<std::vector<uint8_t>> raw;
    clap_input_events_t list{};
    static uint32_t size(const clap_input_events_t* l) { return static_cast<uint32_t>(static_cast<Events*>(l->ctx)->raw.size()); }
    static const clap_event_header_t* get(const clap_input_events_t* l, uint32_t i) {
        auto* self = static_cast<Events*>(l->ctx);
        return i < self->raw.size() ? reinterpret_cast<const clap_event_header_t*>(self->raw[i].data()) : nullptr;
    }
    Events() { list.ctx = this; list.size = size; list.get = get; }
    template <class T> T& add(uint16_t type, uint32_t time) {
        raw.emplace_back(sizeof(T), 0);
        T* ev = reinterpret_cast<T*>(raw.back().data());
        ev->header.size = sizeof(T);
        ev->header.time = time;
        ev->header.space_id = CLAP_CORE_EVENT_SPACE_ID;
        ev->header.type = type;
        return *ev;
    }
};

static void testPluginEvents() {
    acidus::AcidusClap plugin(nullptr);
    plugin.activate(0.0, 1, 512);   // bad rate
    plugin.activate(std::numeric_limits<double>::quiet_NaN(), 1, 512);
    plugin.activate(44100.0, 1, 512);

    const double bad[] = { std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::infinity(),
                           -1.0e300, 1.0e300, -5.0, 5.0 };
    Events ev;
    for (uint32_t id = 0; id < acidus::PARAM_COUNT; ++id) {
        for (double v : bad) {
            auto& p = ev.add<clap_event_param_value_t>(CLAP_EVENT_PARAM_VALUE, 0);
            p.param_id = id;
            p.value = v;
        }
    }
    auto& outOfRange = ev.add<clap_event_param_value_t>(CLAP_EVENT_PARAM_VALUE, 3);
    outOfRange.param_id = 9999;
    auto& wildcard = ev.add<clap_event_note_t>(CLAP_EVENT_NOTE_ON, 5);
    wildcard.key = -1; wildcard.velocity = 1.0;
    auto& hugeKey = ev.add<clap_event_note_t>(CLAP_EVENT_NOTE_ON, 6);
    hugeKey.key = 30000; hugeKey.velocity = std::numeric_limits<double>::quiet_NaN();
    auto& note = ev.add<clap_event_note_t>(CLAP_EVENT_NOTE_ON, 7);
    note.key = 40; note.velocity = 7.0;
    auto& offAll = ev.add<clap_event_note_t>(CLAP_EVENT_NOTE_OFF, 300);
    offAll.key = -1;
    auto& midi = ev.add<clap_event_midi_t>(CLAP_EVENT_MIDI, 9);
    midi.data[0] = 0xB0; midi.data[1] = 71; midi.data[2] = 200;   // malformed data byte
    auto& midi2 = ev.add<clap_event_midi_t>(CLAP_EVENT_MIDI, 10);
    midi2.data[0] = 0x90; midi2.data[1] = 250; midi2.data[2] = 100;
    // A truncated event claiming to be a note.
    ev.raw.emplace_back(sizeof(clap_event_header_t), 0);
    auto* trunc = reinterpret_cast<clap_event_header_t*>(ev.raw.back().data());
    trunc->size = sizeof(clap_event_header_t);
    trunc->type = CLAP_EVENT_NOTE_ON;
    trunc->time = 11;
    trunc->space_id = CLAP_CORE_EVENT_SPACE_ID;
    // Time beyond the block, and out of order.
    auto& late = ev.add<clap_event_note_t>(CLAP_EVENT_NOTE_ON, 0xFFFFFFF0u);
    late.key = 50; late.velocity = 0.5;
    auto& early = ev.add<clap_event_note_t>(CLAP_EVENT_NOTE_ON, 0);
    early.key = 45; early.velocity = 0.9;
    // Pressure with junk values, keys and sizes.
    auto& nanPressure = ev.add<clap_event_note_expression_t>(CLAP_EVENT_NOTE_EXPRESSION, 12);
    nanPressure.expression_id = CLAP_NOTE_EXPRESSION_PRESSURE;
    nanPressure.key = 40; nanPressure.value = std::numeric_limits<double>::quiet_NaN();
    auto& hugePressure = ev.add<clap_event_note_expression_t>(CLAP_EVENT_NOTE_EXPRESSION, 13);
    hugePressure.expression_id = CLAP_NOTE_EXPRESSION_PRESSURE;
    hugePressure.key = 30000; hugePressure.value = 1.0e300;
    auto& midiPressure = ev.add<clap_event_midi_t>(CLAP_EVENT_MIDI, 14);
    midiPressure.data[0] = 0xA0; midiPressure.data[1] = 200; midiPressure.data[2] = 127;
    ev.raw.emplace_back(sizeof(clap_event_note_t), 0);   // too short for an expression
    auto* shortExpr = reinterpret_cast<clap_event_header_t*>(ev.raw.back().data());
    *shortExpr = { sizeof(clap_event_note_t), 15, CLAP_CORE_EVENT_SPACE_ID, CLAP_EVENT_NOTE_EXPRESSION, 0 };

    std::vector<float> l(512, 1.0f), r(512, 1.0f);
    float* chans[2] = { l.data(), r.data() };
    clap_audio_buffer_t out{};
    out.channel_count = 2;
    out.data32 = chans;
    clap_process_t proc{};
    proc.frames_count = 512;
    proc.in_events = &ev.list;
    proc.audio_outputs = &out;
    proc.audio_outputs_count = 1;
    plugin.process(&proc);
    check(finiteAndBounded(l) && finiteAndBounded(r), "plugin output finite after hostile events");

    for (uint32_t id = 0; id < acidus::PARAM_COUNT; ++id) {
        double v = 0.0;
        check(plugin.paramsValue(id, &v) && std::isfinite(v), "parameter value finite after hostile events");
    }

    // Degenerate process() arguments.
    plugin.process(nullptr);
    clap_process_t empty{};
    plugin.process(&empty);
    clap_audio_buffer_t noData{};
    noData.channel_count = 2;
    clap_process_t p2{};
    p2.frames_count = 64;
    p2.audio_outputs = &noData;
    p2.audio_outputs_count = 1;
    plugin.process(&p2);
    clap_process_t zero{};
    zero.audio_outputs = &out;
    zero.audio_outputs_count = 1;
    plugin.process(&zero);
    clap_event_transport_t tr{};
    tr.flags = CLAP_TRANSPORT_HAS_TEMPO | CLAP_TRANSPORT_HAS_BEATS_TIMELINE | CLAP_TRANSPORT_IS_PLAYING;
    tr.tempo = std::numeric_limits<double>::infinity();
    tr.tsig_num = 0;
    clap_process_t p3{};
    p3.frames_count = 64;
    p3.transport = &tr;
    p3.audio_outputs = &out;
    p3.audio_outputs_count = 1;
    plugin.process(&p3);
    double phase = 0.0;
    if (plugin.transportBarPhase(phase)) check(std::isfinite(phase), "transport phase finite");
    tr.tempo = std::numeric_limits<double>::quiet_NaN();
    plugin.process(&p3);

    // Param flush with null lists.
    plugin.paramsFlush(nullptr, nullptr);
}

static void testTextConversion() {
    acidus::AcidusClap plugin(nullptr);
    char buf[16];
    double v = 0.0;
    for (uint32_t id = 0; id < acidus::PARAM_COUNT; ++id) {
        check(plugin.paramsValueToText(id, std::numeric_limits<double>::quiet_NaN(), buf, sizeof(buf)), "value_to_text NaN");
        check(!plugin.paramsValueToText(id, 0.5, buf, 0), "value_to_text zero capacity");
        check(!plugin.paramsValueToText(id, 0.5, nullptr, 16), "value_to_text null buffer");
        const char* texts[] = { "", "abc", "nan", "inf", "1e999", "-1e999", "99999999999", "-0", "0x10" };
        for (const char* t : texts) {
            if (plugin.paramsTextToValue(id, t, &v)) check(std::isfinite(v), "text_to_value finite");
        }
        check(!plugin.paramsTextToValue(id, nullptr, &v), "text_to_value null text");
    }
    check(!plugin.paramsValueToText(acidus::PARAM_COUNT + 5, 0.5, buf, sizeof(buf)), "value_to_text bad id");
    clap_param_info_t info;
    check(!plugin.paramsInfo(acidus::PARAM_COUNT, &info), "param info out of range");
    check(!plugin.paramsInfo(0, nullptr), "param info null");
}

struct MemStream {
    std::vector<uint8_t> data;
    size_t pos = 0;
    bool overrun = false;   // read() returning more than asked
    static int64_t read(const clap_istream_t* s, void* buf, uint64_t size) {
        auto* self = static_cast<MemStream*>(s->ctx);
        if (self->overrun) return static_cast<int64_t>(size) * 4;   // lies
        size_t n = std::min<size_t>(size, self->data.size() - self->pos);
        std::memcpy(buf, self->data.data() + self->pos, n);
        self->pos += n;
        return static_cast<int64_t>(n);
    }
};

static void testState() {
    acidus::AcidusClap plugin(nullptr);
    std::mt19937 rng(99);
    for (int iter = 0; iter < 200; ++iter) {
        MemStream ms;
        size_t len = rng() % 700;
        ms.data.resize(len);
        for (auto& b : ms.data) b = static_cast<uint8_t>(rng());
        if (len >= 16 && (rng() & 1)) {   // valid magic, garbage after
            const uint32_t magic = 0x32534341u;
            std::memcpy(ms.data.data(), &magic, 4);
            uint32_t cnt = rng() % 100;
            std::memcpy(ms.data.data() + 4, &cnt, 4);
        }
        clap_istream_t is{};
        is.ctx = &ms;
        is.read = MemStream::read;
        plugin.stateLoad(&is);
        for (uint32_t id = 0; id < acidus::PARAM_COUNT; ++id) {
            double v = 0.0;
            plugin.paramsValue(id, &v);
            char t[64];
            check(std::isfinite(v), "loaded parameter finite");
            (void)t;
        }
        std::vector<float> l(128), r(128);
        plugin.getEngine().noteOn(40, 1.0f);
        plugin.getEngine().processAudio(l.data(), r.data(), 128);
        check(finiteAndBounded(l) && finiteAndBounded(r), "audio finite after corrupt state load");
    }
    // NaN-filled blob.
    MemStream nanState;
    const uint32_t hdr[4] = { 0x32534341u, acidus::PARAM_COUNT, 7u, 0u };
    nanState.data.resize(16 + acidus::PARAM_COUNT * 8);
    std::memcpy(nanState.data.data(), hdr, 16);
    for (uint32_t i = 0; i < acidus::PARAM_COUNT; ++i) {
        double v = std::numeric_limits<double>::quiet_NaN();
        std::memcpy(nanState.data.data() + 16 + i * 8, &v, 8);
    }
    clap_istream_t is{};
    is.ctx = &nanState;
    is.read = MemStream::read;
    check(plugin.stateLoad(&is), "NaN state loads");
    // A stream that reports more bytes than requested.
    MemStream liar;
    liar.overrun = true;
    liar.data.assign(64, 1);
    clap_istream_t lis{};
    lis.ctx = &liar;
    lis.read = MemStream::read;
    plugin.stateLoad(&lis);   // must not overrun (may fail)
    check(!plugin.stateLoad(nullptr), "null stream rejected");
}

static void testOutEventQueueBounded() {
    acidus::AcidusClap plugin(nullptr);
    // GUI edits with the host never processing: the queue must stay bounded.
    for (int i = 0; i < 200000; ++i) plugin.onParamValueFromGui(acidus::PARAM_CUTOFF, (i % 100) / 100.0);
    plugin.onParamValueFromGui(acidus::PARAM_CUTOFF, std::numeric_limits<double>::quiet_NaN());
    plugin.onParamValueFromGui(9999, 0.5);
    double v = 0.0;
    plugin.paramsValue(acidus::PARAM_CUTOFF, &v);
    check(v >= 0.0 && v <= 1.0, "GUI NaN clamped");
}

static void testGui() {
    acidus::AcidusClap plugin(nullptr);
    acidus::GuiWindow gui(&plugin);
    gui.renderFrame();
    check(!gui.setSize(0, 0), "zero-size window rejected");
    check(!gui.setSize(1000000, 1000000), "huge window rejected");
    check(gui.setSize(50, 40), "small window accepted");
    gui.renderFrame();
    gui.advanceAnimation(std::numeric_limits<double>::quiet_NaN());
    gui.advanceAnimation(1.0e300);
    gui.setSize(1070, 180);
    gui.renderFrame();
    const int coords[] = { -1000000, -1, 0, 5000, 1000000, 2147483647, -2147483647 };
    for (int x : coords) for (int y : coords) {
        gui.handleMouseDown(x, y, false);
        gui.handleMouseDrag(x, y, true);
        gui.handleMouseUp();
    }
    gui.handleMouseDrag(10, 10, false);   // drag without a press
    gui.handleMouseUp();
}

// Pressure on the held note re-latches its accent (Burette's equal-pitch
// slide, TB303_REFERENCE.md §4.6), as a CLAP note expression or MIDI poly
// pressure; channel pressure and other keys do nothing.
static void testPluginPressure() {
    acidus::AcidusClap plugin(nullptr);
    plugin.activate(44100.0, 1, 512);
    Events ev;
    auto& on = ev.add<clap_event_note_t>(CLAP_EVENT_NOTE_ON, 0);
    on.key = 36; on.velocity = 100.0 / 127.0;
    auto& other = ev.add<clap_event_note_expression_t>(CLAP_EVENT_NOTE_EXPRESSION, 50);
    other.expression_id = CLAP_NOTE_EXPRESSION_PRESSURE; other.key = 48; other.value = 1.0;
    auto& channel = ev.add<clap_event_midi_t>(CLAP_EVENT_MIDI, 60);
    channel.data[0] = 0xD0; channel.data[1] = 127;
    auto& accentOn = ev.add<clap_event_note_expression_t>(CLAP_EVENT_NOTE_EXPRESSION, 100);
    accentOn.expression_id = CLAP_NOTE_EXPRESSION_PRESSURE; accentOn.key = 36; accentOn.value = 1.0;
    auto& accentOff = ev.add<clap_event_midi_t>(CLAP_EVENT_MIDI, 200);
    accentOff.data[0] = 0xA0; accentOff.data[1] = 36; accentOff.data[2] = 0;
    auto& accentAgain = ev.add<clap_event_midi_t>(CLAP_EVENT_MIDI, 300);
    accentAgain.data[0] = 0xA0; accentAgain.data[1] = 36; accentAgain.data[2] = 127;
    std::vector<float> l(512), r(512);
    float* chans[2] = { l.data(), r.data() };
    clap_audio_buffer_t out{};
    out.channel_count = 2;
    out.data32 = chans;
    clap_process_t proc{};
    proc.frames_count = 512;
    proc.in_events = &ev.list;
    proc.audio_outputs = &out;
    proc.audio_outputs_count = 1;
    plugin.process(&proc);
    check(plugin.accentCount() == 2, "pressure: accent on via note expression and MIDI poly pressure only");
    check(finiteAndBounded(l) && finiteAndBounded(r), "pressure: output finite");
}

int main() {
    testEngineExtremes();
    testEngineRandomParams();
    testSampleRates();
    testPluginEvents();
    testPluginPressure();
    testTextConversion();
    testState();
    testOutEventQueueBounded();
    testGui();
    if (g_failures == 0) std::printf("All Acidus robustness tests passed.\n");
    return g_failures == 0 ? 0 : 1;
}
