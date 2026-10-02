#include "MidiExport.hpp"
#include "SequencerEngine.hpp"
#include <algorithm>
#include <cmath>

namespace acidus {
namespace seq {

// Offline render: 120 BPM at 48 kHz is 6000 samples per step, 250 per tick.
static constexpr double kRenderRate = 48000.0;
static constexpr double kRenderTempo = 120.0;
static constexpr int64_t kStepSamples = 6000;
static constexpr int64_t kTickSamples = kStepSamples / kMidiTicksPerStep;
static constexpr uint32_t kRenderBlock = 4096;

std::vector<MidiNote> renderPatternNotes(const PatternBank& bank, int pattern, int globalTranspose) {
    std::vector<MidiNote> notes;
    if (pattern < 0 || pattern >= kNumPatterns) return notes;
    int chain[kNumPatterns];
    const int count = SequencerEngine::chainOf(bank, pattern, chain);
    int steps = 0;
    for (int i = 0; i < count; ++i) steps += bank.length(chain[i]);
    const int endTick = steps * kMidiTicksPerStep;

    SequencerEngine engine(bank);
    engine.setSampleRate(kRenderRate);
    engine.reset();
    // Step boundaries come kBoundaryDelaySamples after the grid; times are
    // taken back to the grid before rounding to ticks.
    const int64_t delay = static_cast<int64_t>(SequencerEngine::kBoundaryDelaySamples);
    const int64_t loopEnd = static_cast<int64_t>(steps) * kStepSamples;
    // One extra step: a gate held over the end (a tie or slide wrapping to
    // the start) is released at the next boundary.
    const int64_t total = loopEnd + kStepSamples + delay + 1;

    struct Open { bool on{false}; int start{0}; int velocity{0}; };
    Open open[128];
    auto toTick = [&](int64_t t) {
        const int64_t tick = (t - delay + kTickSamples / 2) / kTickSamples;
        return static_cast<int>(std::min<int64_t>(std::max<int64_t>(tick, 0), endTick));
    };
    auto close = [&](int key, int end) {
        if (!open[key].on) return;
        open[key].on = false;
        if (end > open[key].start) notes.push_back({ open[key].start, end, key, open[key].velocity });
    };

    std::vector<NoteEvent> out(256);
    int64_t lastOnTime = -1;
    for (int64_t s = 0; s < total; s += kRenderBlock) {
        const uint32_t frames = static_cast<uint32_t>(std::min<int64_t>(kRenderBlock, total - s));
        TransportInfo t;
        t.playing = true;
        t.hasTempo = true;
        t.tempo = kRenderTempo;
        t.hasBeats = true;
        t.songPosBeats = static_cast<double>(s) / kRenderRate * kRenderTempo / 60.0;
        TriggerEvent in[2];
        uint32_t inCount = 0;
        if (s == 0) {
            in[inCount++] = { 0, false, -1, true, globalTranspose };
            in[inCount++] = { 0, true, kFirstTriggerKey + pattern };
        }
        const uint32_t n = engine.process(frames, t, in, inCount, out.data(), static_cast<uint32_t>(out.size()));
        for (uint32_t i = 0; i < n; ++i) {
            const NoteEvent& e = out[i];
            const int64_t time = s + e.time;
            if (e.key < 0 || e.key > 127) continue;
            if (e.on) {
                if (time >= loopEnd + delay) continue;   // the next time round
                close(e.key, toTick(time));
                open[e.key] = { true, toTick(time),
                                std::min(127, std::max(1, static_cast<int>(std::lround(e.velocity * 127.0f)))) };
                lastOnTime = time;
            } else {
                // A note-off at the same sample as another note's on is a slide:
                // overlap the notes in the clip.
                const bool slide = time == lastOnTime;
                close(e.key, std::min(endTick, toTick(time) + (slide ? kMidiSlideOverlapTicks : 0)));
            }
        }
    }
    for (int k = 0; k < 128; ++k) close(k, endTick);
    std::stable_sort(notes.begin(), notes.end(),
                     [](const MidiNote& a, const MidiNote& b) { return a.start < b.start; });
    return notes;
}

static void putVarLen(std::string& out, uint32_t v) {
    uint8_t bytes[5];
    int n = 0;
    bytes[n++] = v & 0x7F;
    while ((v >>= 7) != 0) bytes[n++] = static_cast<uint8_t>(0x80 | (v & 0x7F));
    while (n > 0) out += static_cast<char>(bytes[--n]);
}

static void putBE(std::string& out, uint32_t v, int bytes) {
    for (int i = bytes - 1; i >= 0; --i) out += static_cast<char>((v >> (8 * i)) & 0xFF);
}

std::string patternMidiFile(const PatternBank& bank, int pattern, int globalTranspose) {
    const std::vector<MidiNote> notes = renderPatternNotes(bank, pattern, globalTranspose);
    int chain[kNumPatterns];
    const int count = SequencerEngine::chainOf(bank, pattern, chain);
    int steps = 0;
    for (int i = 0; i < count; ++i) steps += bank.length(chain[i]);

    // Events in time order; at the same tick note-offs go first.
    struct Ev { int tick; bool on; int key; int velocity; };
    std::vector<Ev> events;
    for (const MidiNote& n : notes) {
        events.push_back({ n.start, true, n.key, n.velocity });
        events.push_back({ n.end, false, n.key, 64 });
    }
    std::stable_sort(events.begin(), events.end(), [](const Ev& a, const Ev& b) {
        return a.tick != b.tick ? a.tick < b.tick : (!a.on && b.on);
    });

    std::string track;
    const std::string name = bank.name(pattern);
    putVarLen(track, 0);
    track += "\xFF\x03";                    // track name
    putVarLen(track, static_cast<uint32_t>(name.size()));
    track += name;
    putVarLen(track, 0);
    track += std::string("\xFF\x58\x04\x04\x02\x18\x08", 7);   // 4/4
    int tick = 0;
    for (const Ev& e : events) {
        putVarLen(track, static_cast<uint32_t>(e.tick - tick));
        tick = e.tick;
        track += static_cast<char>(e.on ? 0x90 : 0x80);
        track += static_cast<char>(e.key);
        track += static_cast<char>(e.velocity);
    }
    // End of track at the pattern's end, so the clip is the pattern's length.
    putVarLen(track, static_cast<uint32_t>(std::max(0, steps * kMidiTicksPerStep - tick)));
    track += std::string("\xFF\x2F\x00", 3);

    std::string file("MThd", 4);
    putBE(file, 6, 4);
    putBE(file, 0, 2);          // format 0
    putBE(file, 1, 2);          // one track
    putBE(file, kMidiPpq, 2);
    file += "MTrk";
    putBE(file, static_cast<uint32_t>(track.size()), 4);
    file += track;
    return file;
}

std::string midiFileName(const std::string& patternName) {
    std::string out;
    for (char c : patternName) {
        const bool safe = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')
                          || c == ' ' || c == '-' || c == '_' || c == '#' || c == '\'' || c == '+';
        out += safe ? c : '_';
    }
    while (!out.empty() && (out.back() == ' ' || out.back() == '.')) out.pop_back();
    if (out.empty()) out = "PATTERN";
    return out + ".mid";
}

} // namespace seq
} // namespace acidus
