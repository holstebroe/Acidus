// Burette tests: 303 step timing (gate, slide, tie, accent), host-position
// sync, trigger handling, state round trip, CLAP event routing and GUI editing.
#include "sequencer/Pattern.hpp"
#include "sequencer/SequencerEngine.hpp"
#include "sequencer/SequencerClap.hpp"
#include "sequencer/SequencerGui.hpp"
#include "sequencer/MidiExport.hpp"
#include <clap/clap.h>
#include <clap/ext/state.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

using namespace acidus::seq;

static int g_failures = 0;
static void check(bool ok, const std::string& what) {
    if (!ok) { std::printf("FAIL: %s\n", what.c_str()); ++g_failures; }
}

static constexpr double kRate = 48000.0;
static constexpr double kTempo = 120.0;
static constexpr int64_t kStep = 6000;   // samples per 16th at 120 BPM, 48 kHz
static constexpr int64_t kDelay = 2;     // SequencerEngine::kBoundaryDelaySamples

struct Ev { int64_t time; bool on; int key; float vel; };

static Step note(int n, int oct = 0, bool acc = false, bool sl = false) {
    Step s; s.note = n; s.octave = oct; s.accent = acc; s.slide = sl; return s;
}
static Step tie(bool sl = false) { Step s; s.note = kNoteTie; s.slide = sl; return s; }
static Step rest() { return Step{}; }

static void setPattern(PatternBank& b, int p, const std::vector<Step>& steps, int transpose = 0) {
    b.clearPattern(p);
    for (size_t i = 0; i < steps.size(); ++i) b.setStep(p, static_cast<int>(i), steps[i]);
    b.setLength(p, static_cast<int>(steps.size()));
    b.setTranspose(p, transpose);
}

// Runs the engine from `startSample` for `total` samples in blocks; the
// transport plays from song position startSample (in samples at kTempo) when
// `playing`. Triggers are at absolute sample times.
struct Run {
    PatternBank bank;
    SequencerEngine engine{bank};
    std::vector<Ev> events;
    Run() { engine.setSampleRate(kRate); }

    void go(int64_t startSample, int64_t total, const std::vector<TriggerEvent>& triggers,
            bool playing = true, uint32_t block = 512, double songOffsetSamples = 0.0) {
        std::vector<NoteEvent> out(256);
        for (int64_t s = 0; s < total; s += block) {
            const uint32_t frames = static_cast<uint32_t>(std::min<int64_t>(block, total - s));
            TransportInfo t;
            t.playing = playing;
            t.hasTempo = true;
            t.tempo = kTempo;
            t.hasBeats = true;
            t.songPosBeats = (static_cast<double>(startSample + s) + songOffsetSamples) / kRate * kTempo / 60.0;
            std::vector<TriggerEvent> in;
            for (const auto& tr : triggers) {
                if (static_cast<int64_t>(tr.time) >= s && static_cast<int64_t>(tr.time) < s + frames) {
                    TriggerEvent e = tr;
                    e.time = static_cast<uint32_t>(tr.time - s);
                    in.push_back(e);
                }
            }
            const uint32_t n = engine.process(frames, t, in.data(), static_cast<uint32_t>(in.size()),
                                              out.data(), static_cast<uint32_t>(out.size()));
            for (uint32_t i = 0; i < n; ++i) {
                events.push_back({ s + out[i].time, out[i].on, out[i].key, out[i].velocity });
            }
        }
    }
};

static std::string describe(const std::vector<Ev>& ev) {
    std::string s;
    char buf[64];
    for (const auto& e : ev) {
        std::snprintf(buf, sizeof(buf), " %s%d@%lld", e.on ? "+" : "-", e.key, static_cast<long long>(e.time));
        s += buf;
    }
    return s;
}

static void expectEvents(const std::vector<Ev>& got, const std::vector<Ev>& want, const std::string& what) {
    bool ok = got.size() == want.size();
    for (size_t i = 0; ok && i < got.size(); ++i) {
        ok = got[i].time == want[i].time && got[i].on == want[i].on && got[i].key == want[i].key;
    }
    check(ok, what + ": got" + describe(got) + " want" + describe(want));
}

static void testGateLengthAndRest() {
    Run r;
    setPattern(r.bank, 0, { note(1), rest(), note(3), note(5) });
    r.go(0, 4 * kStep, { { 0, true, kFirstTriggerKey } });
    const int64_t g = kStep / 2;
    expectEvents(r.events, {
        { kDelay, true, 36, 0 }, { kDelay + g, false, 36, 0 },
        { 2 * kStep + kDelay, true, 38, 0 }, { 2 * kStep + kDelay + g, false, 38, 0 },
        { 3 * kStep + kDelay, true, 40, 0 }, { 3 * kStep + kDelay + g, false, 40, 0 },
    }, "normal steps gate for half a step, rests are silent");
}

static void testAccentVelocity() {
    Run r;
    setPattern(r.bank, 0, { note(1, 0, true), note(1) });
    r.go(0, 2 * kStep, { { 0, true, kFirstTriggerKey } });
    check(r.events.size() == 4, "accent: four events");
    if (r.events.size() == 4) {
        check(r.events[0].vel >= 0.8f, "accented step velocity is an Acidus accent");
        check(r.events[2].vel < 0.8f, "normal step velocity is below the accent threshold");
    }
}

static void testSlide() {
    Run r;
    setPattern(r.bank, 0, { note(1, 0, false, true), note(8), rest(), rest() });
    r.go(0, 4 * kStep, { { 0, true, kFirstTriggerKey } });
    expectEvents(r.events, {
        { kDelay, true, 36, 0 },
        { kStep + kDelay, true, 43, 0 }, { kStep + kDelay, false, 36, 0 },   // overlap = slide
        { kStep + kDelay + kStep / 2, false, 43, 0 },
    }, "slide holds the gate and overlaps the next note-on");
}

static void testSlideToSamePitchIsTie() {
    Run r;
    setPattern(r.bank, 0, { note(1, 0, false, true), note(1, 0, true), rest(), rest() });
    r.go(0, 4 * kStep, { { 0, true, kFirstTriggerKey } });
    expectEvents(r.events, { { kDelay, true, 36, 0 }, { kStep + kDelay + kStep / 2, false, 36, 0 } },
                 "slide to the same pitch is a tie");
}

static void testTie() {
    Run r;
    setPattern(r.bank, 0, { note(1), tie(), tie(), note(5) });
    r.go(0, 4 * kStep, { { 0, true, kFirstTriggerKey } });
    expectEvents(r.events, {
        { kDelay, true, 36, 0 }, { 2 * kStep + kDelay + kStep / 2, false, 36, 0 },
        { 3 * kStep + kDelay, true, 40, 0 }, { 3 * kStep + kDelay + kStep / 2, false, 40, 0 },
    }, "ties extend the gate");
}

static void testSlideAtEndOfTieChain() {
    Run r;
    // The slide flag on the first note applies when its tie ends.
    setPattern(r.bank, 0, { note(1, 0, false, true), tie(), note(5), rest() });
    r.go(0, 4 * kStep, { { 0, true, kFirstTriggerKey } });
    expectEvents(r.events, {
        { kDelay, true, 36, 0 },
        { 2 * kStep + kDelay, true, 40, 0 }, { 2 * kStep + kDelay, false, 36, 0 },
        { 2 * kStep + kDelay + kStep / 2, false, 40, 0 },
    }, "slide at the end of a tie chain");
}

static void testTieAfterRestIsRest() {
    Run r;
    setPattern(r.bank, 0, { rest(), tie(), note(1), rest() });
    r.go(0, 4 * kStep, { { 0, true, kFirstTriggerKey } });
    expectEvents(r.events, { { 2 * kStep + kDelay, true, 36, 0 }, { 2 * kStep + kDelay + kStep / 2, false, 36, 0 } },
                 "a tie after a rest is silent");
}

static void testOctaveAndTranspose() {
    Run r;
    setPattern(r.bank, 0, { note(1, 1), note(12, -1) }, 5);
    r.go(0, 2 * kStep, { { 0, true, kFirstTriggerKey } });
    check(r.events.size() == 4 && r.events[0].key == 36 + 12 + 5 && r.events[2].key == 36 + 11 - 12 + 5,
          "octave flags and transpose" + describe(r.events));
}

static void testPatternLengthWraps() {
    Run r;
    setPattern(r.bank, 0, { note(1), note(3), note(5) });
    r.go(0, 4 * kStep, { { 0, true, kFirstTriggerKey } });
    check(r.events.size() == 8 && r.events[6].key == 36, "a 3-step pattern wraps to step 1" + describe(r.events));
}

static void testSlideOnLastStepWraps() {
    Run r;
    setPattern(r.bank, 0, { note(1), note(3, 0, false, true) });
    r.go(0, 3 * kStep, { { 0, true, kFirstTriggerKey } });
    // step 2 slides into step 1 of the next cycle
    expectEvents(r.events, {
        { kDelay, true, 36, 0 }, { kDelay + kStep / 2, false, 36, 0 },
        { kStep + kDelay, true, 38, 0 },
        { 2 * kStep + kDelay, true, 36, 0 }, { 2 * kStep + kDelay, false, 38, 0 },
        { 2 * kStep + kDelay + kStep / 2, false, 36, 0 },
    }, "a slide on the last step wraps to the first");
}

static void testHostStartMidStep() {
    Run r;
    setPattern(r.bank, 0, { note(1), note(3), note(5), note(6) });
    // Playback starts 40 % into step 1 (song position 1.4 steps), trigger
    // held from there: the first note is step 2 (index 2) at the next boundary.
    const int64_t start = kStep + kStep * 2 / 5;
    r.go(start, kStep, { { 0, true, kFirstTriggerKey } });
    check(!r.events.empty() && r.events[0].key == 40 && r.events[0].time == (2 * kStep - start) + kDelay,
          "host start mid-step waits for the next step, at its pattern position" + describe(r.events));
}

static void testFollowsSongPosition() {
    Run r;
    setPattern(r.bank, 0, { note(1), note(3), note(5), note(6) });
    // Song position 6 steps: a 4-step pattern is on index 2 there.
    r.go(6 * kStep, kStep, { { 0, true, kFirstTriggerKey } });
    check(!r.events.empty() && r.events[0].key == 40, "pattern position follows the song position" + describe(r.events));
}

static void testTriggerReleaseLetsStepFinish() {
    Run r;
    setPattern(r.bank, 0, { note(1), note(3, 0, false, true), note(5), note(6) });
    // Released 100 samples into step 1 (normal) -- it still gates its half
    // step. Second run: released during the slid step 2 -- held to the step end.
    r.go(0, 4 * kStep, { { 0, true, kFirstTriggerKey }, { 100, false, kFirstTriggerKey } });
    expectEvents(r.events, { { kDelay, true, 36, 0 }, { kDelay + kStep / 2, false, 36, 0 } },
                 "trigger release does not cut the gate");
    Run r2;
    setPattern(r2.bank, 0, { note(1), note(3, 0, false, true), note(5), note(6) });
    r2.go(0, 4 * kStep, { { 0, true, kFirstTriggerKey }, { static_cast<uint32_t>(kStep + 100), false, kFirstTriggerKey } });
    expectEvents(r2.events, {
        { kDelay, true, 36, 0 }, { kDelay + kStep / 2, false, 36, 0 },
        { kStep + kDelay, true, 38, 0 }, { 2 * kStep + kDelay, false, 38, 0 },
    }, "a slid step released mid-step holds its gate to the step end");
}

static void testGridAlignedTriggerNotes() {
    // A trigger note from bar 0 to step 2 exactly, as a host places it, and
    // rounded one sample either way: never a missing first step or an extra
    // step at the end.
    for (int jitter = -1; jitter <= 1; ++jitter) {
        Run r;
        setPattern(r.bank, 0, { note(1), note(3), note(5), note(6) });
        const uint32_t on = static_cast<uint32_t>(std::max(0, jitter));
        const uint32_t off = static_cast<uint32_t>(2 * kStep + jitter);
        r.go(0, 4 * kStep, { { on, true, kFirstTriggerKey }, { off, false, kFirstTriggerKey } });
        check(r.events.size() == 4 && r.events[0].key == 36 && r.events[2].key == 38,
              "grid-aligned trigger, jitter " + std::to_string(jitter) + describe(r.events));
    }
}

static void testLateTrigger() {
    Run r;
    setPattern(r.bank, 0, { note(1), note(3) });
    // Trigger 4 ms late for step 1.
    const uint32_t late = static_cast<uint32_t>(kStep + 192);
    r.go(0, 2 * kStep, { { late, true, kFirstTriggerKey } });
    check(!r.events.empty() && r.events[0].time == late && r.events[0].key == 38,
          "a slightly late trigger still plays its step" + describe(r.events));
}

static void testFreeRunning() {
    Run r;
    setPattern(r.bank, 0, { note(1), note(3) });
    r.go(0, 2 * kStep, { { 1000, true, kFirstTriggerKey } }, false);
    expectEvents(r.events, {
        { 1000, true, 36, 0 }, { 1000 + kStep / 2, false, 36, 0 },
        { 1000 + kStep, true, 38, 0 }, { 1000 + kStep + kStep / 2, false, 38, 0 },
    }, "transport stopped: the trigger starts the pattern at once");
}

static void testPatternSwitch() {
    Run r;
    setPattern(r.bank, 0, { note(1), note(1), note(1), note(1) });
    setPattern(r.bank, 3, { note(8), note(8), note(8), note(8) });
    // Pattern 4 pressed during step 1 while pattern 1 is held.
    r.go(0, 3 * kStep, { { 0, true, kFirstTriggerKey }, { static_cast<uint32_t>(kStep + 100), true, kFirstTriggerKey + 3 } });
    check(r.events.size() == 6 && r.events[2].key == 36 && r.events[4].key == 43,
          "pattern change takes effect at the next step" + describe(r.events));
}

static void testLoopJump() {
    Run r;
    setPattern(r.bank, 0, { note(1), note(3), note(5), note(6) });
    r.go(0, 3 * kStep, { { 0, true, kFirstTriggerKey } });
    const size_t before = r.events.size();
    // The host loops back to the start: step 1 plays again, on time.
    r.go(0, kStep, {}, true, 512, 0.0);
    check(r.events.size() > before && r.events[before].key == 36 && r.events[before].time == kDelay,
          "loop back to the start replays step 1" + describe(r.events));
}

static void testNoStuckNotes() {
    // Random patterns and triggers: every note-on gets exactly one note-off.
    uint32_t seed = 12345;
    auto rnd = [&](int n) { seed = seed * 1664525u + 1013904223u; return static_cast<int>((seed >> 8) % n); };
    for (int trial = 0; trial < 50; ++trial) {
        Run r;
        for (int p = 0; p < 3; ++p) {
            std::vector<Step> steps;
            const int len = 1 + rnd(16);
            for (int i = 0; i < len; ++i) steps.push_back(note(rnd(14), rnd(3) - 1, rnd(2), rnd(2)));
            setPattern(r.bank, p, steps, rnd(25) - 12);
        }
        std::vector<TriggerEvent> trig;
        uint32_t t = 0;
        for (int k = 0; k < 10; ++k) {
            t += static_cast<uint32_t>(rnd(30000));
            const int key = rnd(3);
            trig.push_back({ t, true, kFirstTriggerKey + key });
            trig.push_back({ t + static_cast<uint32_t>(rnd(40000)), false, kFirstTriggerKey + key });
        }
        std::sort(trig.begin(), trig.end(), [](const TriggerEvent& a, const TriggerEvent& b) { return a.time < b.time; });
        r.go(0, 700000, trig, trial % 2 == 0, 64 + static_cast<uint32_t>(rnd(1000)));
        int sounding = -1;
        bool ok = true;
        for (const auto& e : r.events) {
            if (e.on) {
                sounding = e.key;
            } else {
                ok = ok && e.key >= 0;
                if (e.key == sounding) sounding = -1;
            }
        }
        check(ok && sounding == -1, "random run " + std::to_string(trial) + " leaves no note hanging");
    }
}

static void testFactoryPatterns() {
    PatternBank b;
    check(b.name(0) == "OCTAVE JUMPER" && b.length(0) == 16 && b.transpose(0) == 5, "factory 1 header");
    const Step s0 = b.step(0, 0);
    check(s0.note == 1 && s0.octave == -1 && !s0.accent && !s0.slide, "factory 1 step 1 = C down");
    const Step s5 = b.step(0, 5);
    check(s5.note == 11 && s5.octave == -1 && s5.accent && s5.slide, "factory 1 step 6 = A# down accent slide");
    check(b.keyFor(0, s0) == 36 - 12 + 5, "factory 1 step 1 key");
    check(b.length(1) == 8 && b.step(1, 0).slide && b.step(1, 0).octave == 1, "factory 2");
    check(b.step(3, 5).note == kNoteTie && b.step(3, 3).slide, "factory 4 tie/slide");
    check(b.step(4, 1).note == kNoteTie && b.step(4, 15).note == kNoteTie, "factory 5 ties");
    check(b.next(5) == 6 && b.next(6) == 5 && b.next(0) == -1, "factory chain 6 > 7 > 6");
    check(b.step(6, 0).note == kNoteHighC && b.step(6, 0).octave == 1
          && b.keyFor(6, b.step(6, 0)) == 60, "high C with octave up is C4 (MIDI 60), the 303's top note");
    // Every slot holds a demo; 15 and 16 are a chain.
    for (int p = 0; p < kNumPatterns; ++p) {
        bool hasNote = false;
        for (int i = 0; i < b.length(p); ++i) hasNote = hasNote || b.step(p, i).isNote();
        check(hasNote && b.name(p).rfind("PATTERN", 0) != 0, "factory slot " + std::to_string(p + 1) + " holds a demo");
    }
    check(b.length(11) == 7 && b.next(14) == 15 && b.next(15) == 14, "factory 7-step pattern and 15 > 16 chain");
    check(b.step(13, 0).note == kNoteHighC && b.step(13, 0).octave == 1 && b.step(13, 0).slide, "factory 14 high C'");
    check(b.step(8, 1).note == kNoteRest && b.step(8, 15).note == 1, "factory 9 rests");
    b.clearPattern(7);
    check(b.name(7) == "PATTERN 8" && b.step(7, 0).note == kNoteRest && b.length(7) == 16, "cleared slot");
}

static void testChaining() {
    Run r;
    setPattern(r.bank, 0, { note(1), note(3) });
    setPattern(r.bank, 2, { note(5), note(6), note(8) });
    r.bank.setNext(0, 2);
    r.bank.setNext(2, 0);   // back to the start: chain 1 > 3, then loops
    r.go(0, 6 * kStep, { { 0, true, kFirstTriggerKey } });
    std::vector<int> keys;
    for (const auto& e : r.events) if (e.on) keys.push_back(e.key);
    check(keys == std::vector<int>({ 36, 38, 40, 41, 43, 36 }), "chain 1 > 3 then loops" + describe(r.events));

    // A link back into the middle of the chain still loops from the start;
    // a slide (applied at the end of its tie) and a tie cross pattern
    // boundaries.
    Run r2;
    setPattern(r2.bank, 0, { note(1, 0, false, true) });
    setPattern(r2.bank, 1, { tie(), note(8) });
    r2.bank.setNext(0, 1);
    r2.bank.setNext(1, 1);
    r2.go(0, 3 * kStep, { { 0, true, kFirstTriggerKey } });
    expectEvents(r2.events, {
        { kDelay, true, 36, 0 },
        { 2 * kStep + kDelay, true, 43, 0 }, { 2 * kStep + kDelay, false, 36, 0 },
        { 2 * kStep + kDelay + kStep / 2, false, 43, 0 },
    }, "slide and tie across a chain boundary");

    // The chain follows the song position too: song step 4 of a 2 + 3 chain
    // is the second pattern's last step.
    Run r3;
    setPattern(r3.bank, 0, { note(1), note(3) });
    setPattern(r3.bank, 2, { note(5), note(6), note(8) });
    r3.bank.setNext(0, 2);
    r3.go(4 * kStep, kStep, { { 0, true, kFirstTriggerKey } });
    check(!r3.events.empty() && r3.events[0].key == 43, "chain position follows the song" + describe(r3.events));
    check(r3.engine.playingPattern() == 2, "the playing chain member is reported");
}

static void testGlobalTranspose() {
    Run r;
    setPattern(r.bank, 0, { note(1, 0, false, true), note(1), note(1), note(1) });
    // Transpose +7 during step 1 (a slid note): it holds; step 2 is an
    // equal-pitch slide in the pattern but sounds a new pitch, so it glides.
    TriggerEvent tr{ static_cast<uint32_t>(kStep / 4), false, -1, true, 7 };
    TriggerEvent tr2{ static_cast<uint32_t>(3 * kStep), false, -1, true, -30 };   // clamped to -12
    r.go(0, 4 * kStep, { { 0, true, kFirstTriggerKey }, tr, tr2 });
    std::vector<int> keys;
    for (const auto& e : r.events) if (e.on) keys.push_back(e.key);
    check(keys == std::vector<int>({ 36, 43, 43, 24 }), "global transpose applies from the next note" + describe(r.events));
    check(r.events.size() > 2 && r.events[1].on && r.events[2].key == 36 && !r.events[2].on,
          "a transposed slide still overlaps" + describe(r.events));
}

static void testStepPacking() {
    for (int n = 0; n < kNoteValueCount; ++n)
        for (int o = -1; o <= 1; ++o)
            for (int a = 0; a < 2; ++a)
                for (int s = 0; s < 2; ++s) {
                    const Step st = note(n, o, a, s);
                    check(Step::unpack(st.pack()) == st, "step pack round trip");
                }
}

// --- CLAP-level tests ----------------------------------------------------------

struct Buf { std::vector<uint8_t> data; size_t pos{0}; };

static const clap_host_t* testHost() {
    static clap_host_t host{};
    host.clap_version = CLAP_VERSION;
    host.name = "test";
    host.get_extension = [](const clap_host_t*, const char*) -> const void* { return nullptr; };
    host.request_restart = [](const clap_host_t*) {};
    host.request_process = [](const clap_host_t*) {};
    host.request_callback = [](const clap_host_t*) {};
    return &host;
}

static void testStateRoundTrip() {
    SequencerClap a(testHost());
    a.bank().setStep(7, 3, note(9, 1, true, true));
    a.bank().setLength(7, 11);
    a.bank().setTranspose(7, -7);
    a.bank().setName(7, "MY RIFF");
    a.setEditPattern(7);
    a.setFollowPlaying(false);
    a.bank().setNext(7, 2);
    a.setTransposeFromGui(-5);

    Buf buf;
    clap_ostream_t os{ &buf, [](const clap_ostream_t* s, const void* d, uint64_t n) -> int64_t {
        auto* b = static_cast<Buf*>(s->ctx);
        b->data.insert(b->data.end(), static_cast<const uint8_t*>(d), static_cast<const uint8_t*>(d) + n);
        return static_cast<int64_t>(n);
    } };
    check(a.stateSave(&os), "state save");

    SequencerClap b(testHost());
    clap_istream_t is{ &buf, [](const clap_istream_t* s, void* d, uint64_t n) -> int64_t {
        auto* b = static_cast<Buf*>(s->ctx);
        const size_t k = std::min<size_t>(n, b->data.size() - b->pos);
        std::memcpy(d, b->data.data() + b->pos, k);
        b->pos += k;
        return static_cast<int64_t>(k);
    } };
    check(b.stateLoad(&is), "state load");
    check(b.bank().step(7, 3) == note(9, 1, true, true) && b.bank().length(7) == 11
          && b.bank().transpose(7) == -7 && b.bank().name(7) == "MY RIFF"
          && b.editPattern() == 7 && !b.followPlaying() && b.bank().next(7) == 2
          && b.globalTranspose() == -5, "state round trip");

    // Truncated state: rejected, nothing changed.
    SequencerClap c(testHost());
    buf.pos = 0;
    buf.data.resize(buf.data.size() / 2);
    check(!c.stateLoad(&is), "truncated state is rejected");
    check(c.bank().name(0) == "OCTAVE JUMPER", "rejected state leaves the patterns alone");

    // Version 1 state (no next byte; the tie was note 13).
    std::string v1("A3SQ\x01", 5);
    for (int p = 0; p < kNumPatterns; ++p) {
        v1 += static_cast<char>(4); v1 += static_cast<char>(2); v1 += static_cast<char>(1); v1 += 'X';
        for (int i = 0; i < kMaxSteps; ++i) v1 += static_cast<char>(i == 1 ? 13 : 0x11);
    }
    PatternBank old;
    check(old.deserialize(reinterpret_cast<const uint8_t*>(v1.data()), v1.size()) == v1.size()
          && old.step(3, 1).note == kNoteTie && old.step(3, 0).note == 1 && old.next(3) == -1
          && old.length(3) == 4 && old.transpose(3) == 2 && old.name(3) == "X", "version 1 state loads");
}

struct EventList {
    std::vector<std::vector<uint8_t>> events;
    void addNote(uint16_t type, uint32_t time, int key) {
        clap_event_note_t e{};
        e.header = { sizeof(e), time, CLAP_CORE_EVENT_SPACE_ID, type, 0 };
        e.note_id = -1; e.key = static_cast<int16_t>(key); e.velocity = 0.9;
        add(&e.header);
    }
    void addMidi(uint32_t time, uint8_t s, uint8_t d1, uint8_t d2) {
        clap_event_midi_t e{};
        e.header = { sizeof(e), time, CLAP_CORE_EVENT_SPACE_ID, CLAP_EVENT_MIDI, 0 };
        e.data[0] = s; e.data[1] = d1; e.data[2] = d2;
        add(&e.header);
    }
    void add(const clap_event_header_t* h) {
        const auto* p = reinterpret_cast<const uint8_t*>(h);
        events.emplace_back(p, p + h->size);
    }
};

static void testClapRouting() {
    SequencerClap plugin(testHost());
    const clap_plugin_t* p = plugin.getClapPlugin();
    p->activate(p, kRate, 1, 4096);
    setPattern(plugin.bank(), 0, { note(1) });

    EventList in;
    in.addNote(CLAP_EVENT_NOTE_ON, 0, kFirstTriggerKey);   // trigger pattern 1: consumed
    in.addNote(CLAP_EVENT_NOTE_ON, 5, 0);          // below the trigger keys: passes through
    in.addNote(CLAP_EVENT_NOTE_ON, 10, 60);        // ordinary note: passes through
    in.addMidi(20, 0xB0, 74, 100);                 // CC: passes through
    EventList out;

    clap_input_events_t inEv{ &in,
        [](const clap_input_events_t* l) -> uint32_t { return static_cast<uint32_t>(static_cast<EventList*>(l->ctx)->events.size()); },
        [](const clap_input_events_t* l, uint32_t i) -> const clap_event_header_t* {
            return reinterpret_cast<const clap_event_header_t*>(static_cast<EventList*>(l->ctx)->events[i].data());
        } };
    clap_output_events_t outEv{ &out, [](const clap_output_events_t* l, const clap_event_header_t* h) -> bool {
        static_cast<EventList*>(l->ctx)->add(h);
        return true;
    } };

    std::vector<float> l(512), r(512, 1.0f);
    float* chans[2] = { l.data(), r.data() };
    clap_audio_buffer_t audio{};
    audio.data32 = chans;
    audio.channel_count = 2;
    clap_event_transport_t tr{};
    tr.header = { sizeof(tr), 0, CLAP_CORE_EVENT_SPACE_ID, CLAP_EVENT_TRANSPORT, 0 };
    tr.flags = CLAP_TRANSPORT_HAS_TEMPO | CLAP_TRANSPORT_HAS_BEATS_TIMELINE | CLAP_TRANSPORT_IS_PLAYING;
    tr.tempo = kTempo;
    tr.song_pos_beats = 0;
    clap_process_t proc{};
    proc.frames_count = 512;
    proc.transport = &tr;
    proc.audio_outputs = &audio;
    proc.audio_outputs_count = 1;
    proc.in_events = &inEv;
    proc.out_events = &outEv;
    p->process(p, &proc);

    std::vector<std::pair<uint16_t, uint32_t>> got;
    int seqKey = -1;
    for (const auto& e : out.events) {
        const auto* h = reinterpret_cast<const clap_event_header_t*>(e.data());
        got.push_back({ h->type, h->time });
        if (h->type == CLAP_EVENT_NOTE_ON && h->time == kDelay) {
            seqKey = reinterpret_cast<const clap_event_note_t*>(h)->key;
        }
    }
    check(got.size() == 4, "routing: sequencer note + 3 passed-through events");
    check(seqKey == 36, "routing: the sequencer's note");
    bool sorted = true;
    for (size_t i = 1; i < got.size(); ++i) sorted = sorted && got[i].second >= got[i - 1].second;
    check(sorted, "routing: output sorted by time");
    check(r[100] == 0.0f, "audio output is silent");
}

static void testTransposeParam() {
    SequencerClap plugin(testHost());
    const clap_plugin_t* p = plugin.getClapPlugin();
    p->activate(p, kRate, 1, 4096);
    setPattern(plugin.bank(), 0, { note(1), note(1) });
    const auto* params = static_cast<const clap_plugin_params_t*>(p->get_extension(p, CLAP_EXT_PARAMS));
    check(params && params->count(p) == 1, "one parameter");
    clap_param_info_t info;
    check(params->get_info(p, 0, &info) && (info.flags & CLAP_PARAM_IS_AUTOMATABLE)
          && info.min_value == -12 && info.max_value == 12, "key transpose param info");

    // Host automation to +3 in mid-block, before the second step's note.
    EventList in;
    in.addNote(CLAP_EVENT_NOTE_ON, 0, kFirstTriggerKey);
    clap_event_param_value_t pv{};
    pv.header = { sizeof(pv), 3000, CLAP_CORE_EVENT_SPACE_ID, CLAP_EVENT_PARAM_VALUE, 0 };
    pv.param_id = SequencerClap::kParamTranspose;
    pv.value = 3.0;
    in.add(&pv.header);
    EventList out;
    clap_input_events_t inEv{ &in,
        [](const clap_input_events_t* l) -> uint32_t { return static_cast<uint32_t>(static_cast<EventList*>(l->ctx)->events.size()); },
        [](const clap_input_events_t* l, uint32_t i) -> const clap_event_header_t* {
            return reinterpret_cast<const clap_event_header_t*>(static_cast<EventList*>(l->ctx)->events[i].data());
        } };
    clap_output_events_t outEv{ &out, [](const clap_output_events_t* l, const clap_event_header_t* h) -> bool {
        static_cast<EventList*>(l->ctx)->add(h);
        return true;
    } };
    clap_event_transport_t tr{};
    tr.flags = CLAP_TRANSPORT_HAS_TEMPO | CLAP_TRANSPORT_HAS_BEATS_TIMELINE | CLAP_TRANSPORT_IS_PLAYING;
    tr.tempo = kTempo;
    clap_process_t proc{};
    proc.frames_count = 2 * kStep;
    proc.transport = &tr;
    proc.in_events = &inEv;
    proc.out_events = &outEv;
    p->process(p, &proc);
    std::vector<int> keys;
    for (const auto& e : out.events) {
        const auto* h = reinterpret_cast<const clap_event_header_t*>(e.data());
        if (h->type == CLAP_EVENT_NOTE_ON) keys.push_back(reinterpret_cast<const clap_event_note_t*>(h)->key);
    }
    check(keys == std::vector<int>({ 36, 39 }) && plugin.globalTranspose() == 3,
          "host automation transposes from the next note");

    // A GUI edit goes to the host as begin / value / end.
    plugin.beginTransposeEdit();
    plugin.setTransposeFromGui(5);
    plugin.endTransposeEdit();
    EventList flushed;
    clap_output_events_t flushOut{ &flushed, outEv.try_push };
    EventList none;
    clap_input_events_t noIn{ &none, inEv.size, inEv.get };
    params->flush(p, &noIn, &flushOut);
    std::vector<uint16_t> types;
    for (const auto& e : flushed.events) types.push_back(reinterpret_cast<const clap_event_header_t*>(e.data())->type);
    check(types == std::vector<uint16_t>({ CLAP_EVENT_PARAM_GESTURE_BEGIN, CLAP_EVENT_PARAM_VALUE, CLAP_EVENT_PARAM_GESTURE_END }),
          "GUI transpose edit is one host gesture");
    double v = 0;
    check(params->get_value(p, SequencerClap::kParamTranspose, &v) && v == 5.0, "param value follows the GUI");
}

static void testGuiEditing() {
    SequencerClap plugin(testHost());
    plugin.createGui();
    SequencerGui* gui = plugin.gui();
    plugin.bank().clearPattern(9);
    plugin.setEditPattern(9);
    int x, y, w, h;

    // Note cell: click up, right-click down, wraps.
    SequencerGui::cellRect(SequencerGui::Row::Note, 2, x, y, w, h);
    gui->mouseDown(x + 5, y + 5, false); gui->mouseUp(x + 5, y + 5);
    check(plugin.bank().step(9, 2).note == 1, "click: rest -> C");
    gui->mouseDown(x + 5, y + 5, true); gui->mouseUp(x + 5, y + 5);
    gui->mouseDown(x + 5, y + 5, true); gui->mouseUp(x + 5, y + 5);
    check(plugin.bank().step(9, 2).note == kNoteTie, "right-click: C -> rest -> tie");

    // Drag up three values.
    gui->mouseDown(x + 5, y + 20, false);
    gui->mouseDrag(x + 5, y + 20 - 36);
    gui->mouseUp(x + 5, y + 20 - 36);
    check(plugin.bank().step(9, 2).note == 2, "drag up 3: tie -> rest -> C -> C#");

    SequencerGui::cellRect(SequencerGui::Row::Octave, 2, x, y, w, h);
    gui->mouseDown(x + 5, y + 5, false); gui->mouseUp(x + 5, y + 5);
    check(plugin.bank().step(9, 2).octave == 1, "octave up");
    SequencerGui::cellRect(SequencerGui::Row::Slide, 2, x, y, w, h);
    gui->mouseDown(x + 5, y + 5, false); gui->mouseUp(x + 5, y + 5);
    check(plugin.bank().step(9, 2).slide, "slide on");
    SequencerGui::cellRect(SequencerGui::Row::Accent, 2, x, y, w, h);
    gui->mouseDown(x + 5, y + 5, true); gui->mouseUp(x + 5, y + 5);
    check(plugin.bank().step(9, 2).accent, "accent toggles with either button");

    SequencerGui::boxRect(SequencerGui::Box::Length, x, y, w, h);
    gui->mouseDown(x + 5, y + 5, true);
    check(plugin.bank().length(9) == 15, "length down");
    SequencerGui::boxRect(SequencerGui::Box::Transpose, x, y, w, h);
    gui->mouseDown(x + 5, y + 5, false); gui->mouseUp(x + 5, y + 5);
    check(plugin.bank().transpose(9) == 1, "transpose up");
    SequencerGui::boxRect(SequencerGui::Box::Next, x, y, w, h);
    gui->mouseDown(x + 5, y + 20, false);
    gui->mouseDrag(x + 5, y + 20 - 24);   // - > 1 > 2
    gui->mouseUp(x + 5, y + 20 - 24);
    check(plugin.bank().next(9) == 1, "next: drag up from - to 2");
    SequencerGui::boxRect(SequencerGui::Box::Key, x, y, w, h);
    gui->mouseDown(x + 5, y + 5, true);
    check(plugin.globalTranspose() == -1, "key transpose down");

    SequencerGui::patternButtonRect(11, x, y, w, h);
    gui->mouseDown(x + 5, y + 5, false); gui->mouseUp(x + 5, y + 5);
    check(plugin.editPattern() == 11, "pattern button selects");

    // Rendering: the inactive area past the length is darker than the active grid.
    plugin.setEditPattern(1);   // 8 steps
    gui->renderFrame();
    SequencerGui::cellRect(SequencerGui::Row::Accent, 12, x, y, w, h);
    const uint32_t inactive = gui->pixels()[(y + 5) * SequencerGui::kWidth + x + 5];
    SequencerGui::cellRect(SequencerGui::Row::Accent, 0, x, y, w, h);
    const uint32_t active = gui->pixels()[(y + 5) * SequencerGui::kWidth + x + 5];
    check(((inactive >> 8) & 0xFF) < ((active >> 8) & 0xFF), "steps past the length are shaded");
    plugin.destroyGui();
}

// --- MIDI export, bank files, play button, new GUI controls --------------------

// Notes and the end-of-track tick read back from a format 0 MIDI file.
struct ParsedMidi { bool ok{false}; int division{0}; int endTick{0}; std::vector<MidiNote> notes; };

static ParsedMidi parseMidi(const std::string& f) {
    ParsedMidi m;
    auto u = [&](size_t i) { return static_cast<uint8_t>(f[i]); };
    if (f.size() < 22 || f.compare(0, 4, "MThd") != 0 || f.compare(14, 4, "MTrk") != 0) return m;
    if (u(8) != 0 || u(9) != 0 || u(11) != 1) return m;   // format 0, one track
    m.division = (u(12) << 8) | u(13);
    const size_t len = (static_cast<size_t>(u(18)) << 24) | (u(19) << 16) | (u(20) << 8) | u(21);
    if (22 + len != f.size()) return m;
    size_t i = 22;
    int tick = 0;
    MidiNote open[128];
    bool isOpen[128] = {};
    while (i < f.size()) {
        uint32_t delta = 0;
        do { delta = (delta << 7) | (u(i) & 0x7F); } while (u(i++) & 0x80);
        tick += static_cast<int>(delta);
        const uint8_t st = u(i++);
        if (st == 0xFF) {
            const uint8_t type = u(i++);
            const uint8_t n = u(i++);
            i += n;
            if (type == 0x2F) { m.endTick = tick; m.ok = i == f.size(); return m; }
        } else if ((st & 0xF0) == 0x90 || (st & 0xF0) == 0x80) {
            const int key = u(i), vel = u(i + 1);
            i += 2;
            if ((st & 0xF0) == 0x90 && vel > 0) { open[key] = { tick, 0, key, vel }; isOpen[key] = true; }
            else if (isOpen[key]) { open[key].end = tick; m.notes.push_back(open[key]); isOpen[key] = false; }
        } else {
            return m;
        }
    }
    return m;
}

static bool sameNotes(std::vector<MidiNote> a, std::vector<MidiNote> b) {
    auto key = [](const MidiNote& n) { return std::make_pair(n.start, n.key); };
    std::sort(a.begin(), a.end(), [&](const MidiNote& x, const MidiNote& y) { return key(x) < key(y); });
    std::sort(b.begin(), b.end(), [&](const MidiNote& x, const MidiNote& y) { return key(x) < key(y); });
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (a[i].start != b[i].start || a[i].end != b[i].end || a[i].key != b[i].key || a[i].velocity != b[i].velocity) return false;
    }
    return true;
}

static std::string describeNotes(const std::vector<MidiNote>& notes) {
    std::string s = ":";
    for (const auto& n : notes) {
        s += " " + std::to_string(n.key) + "@" + std::to_string(n.start) + "-" + std::to_string(n.end)
             + "v" + std::to_string(n.velocity);
    }
    return s;
}

static void testMidiExport() {
    PatternBank b;
    setPattern(b, 0, { note(1, 0, true), note(3, 0, false, true), note(5), tie(), rest(), note(8) });
    const int S = kMidiTicksPerStep, G = S / 2;
    // With KEY +2: accented C, a slide D -> E (overlapping), E tied over a
    // step then gated for half the tie step, a rest, G.
    const std::vector<MidiNote> want = {
        { 0, G, 38, 127 },
        { S, 2 * S + kMidiSlideOverlapTicks, 40, 100 },
        { 2 * S, 3 * S + G, 42, 100 },
        { 5 * S, 5 * S + G, 45, 100 },
    };
    const auto notes = renderPatternNotes(b, 0, 2);
    check(sameNotes(notes, want), "MIDI export: gate, accent, slide overlap, tie" + describeNotes(notes));
    const ParsedMidi m = parseMidi(patternMidiFile(b, 0, 2));
    check(m.ok && m.division == kMidiPpq && m.endTick == 6 * S && sameNotes(m.notes, want),
          "MIDI file round trip" + describeNotes(m.notes));

    // A slide on the last step wraps: the note is cut at the clip's end, and
    // the next time round is not in the clip.
    setPattern(b, 1, { note(1), note(3, 0, false, true) });
    check(sameNotes(renderPatternNotes(b, 1, 0), { { 0, G, 36, 100 }, { S, 2 * S, 38, 100 } }),
          "MIDI export: a wrapping slide ends at the clip end" + describeNotes(renderPatternNotes(b, 1, 0)));

    // A chained pattern exports its whole chain once.
    PatternBank factory;
    const ParsedMidi chain = parseMidi(patternMidiFile(factory, 5, 0));
    check(chain.ok && chain.endTick == (factory.length(5) + factory.length(6)) * S && chain.notes.size() > 8,
          "MIDI export of a chain (6 > 7)");
    for (int p = 0; p < kNumPatterns; ++p) {
        const ParsedMidi f = parseMidi(patternMidiFile(factory, p, 0));
        bool inRange = f.ok && !f.notes.empty();
        for (const auto& n : f.notes) inRange = inRange && n.start < n.end && n.end <= f.endTick;
        check(inRange, "factory pattern " + std::to_string(p + 1) + " exports");
    }
    check(midiFileName("A/B: C?") == "A_B_ C_.mid" && midiFileName("") == "PATTERN.mid", "MIDI file names");
}

static std::filesystem::path tempPath(const char* name) {
    return std::filesystem::temp_directory_path() / name;
}

static void testBankFile() {
    SequencerClap a(testHost());
    a.bank().setName(4, "SAVED");
    a.bank().setStep(4, 2, note(7, -1, true, false));
    const auto path = tempPath("burette_test.burette");
    check(a.saveBankFile(path), "bank file save");
    a.bank().clearPattern(4);
    check(a.loadBankFile(path) && a.bank().name(4) == "SAVED" && a.bank().step(4, 2) == note(7, -1, true, false),
          "bank file load");
    const auto junk = tempPath("burette_junk.burette");
    { std::ofstream f(junk, std::ios::binary); f << "not a bank"; }
    check(!a.loadBankFile(junk) && a.bank().name(4) == "SAVED", "a file that is no bank is refused");
    check(!a.loadBankFile(tempPath("burette_missing.burette")), "a missing file is refused");
    std::filesystem::remove(path);
    std::filesystem::remove(junk);

    const auto midi = a.writePatternMidi(4);
    check(!midi.empty() && midi.filename() == "SAVED.mid" && std::filesystem::file_size(midi) > 22,
          "pattern MIDI written for dragging");
    std::filesystem::remove(midi);
}

static void testPreview() {
    SequencerClap plugin(testHost());
    const clap_plugin_t* p = plugin.getClapPlugin();
    p->activate(p, kRate, 1, 4096);
    setPattern(plugin.bank(), 2, { note(1), note(3) });
    EventList none;
    clap_input_events_t noIn{ &none,
        [](const clap_input_events_t* l) -> uint32_t { return static_cast<uint32_t>(static_cast<EventList*>(l->ctx)->events.size()); },
        [](const clap_input_events_t* l, uint32_t i) -> const clap_event_header_t* {
            return reinterpret_cast<const clap_event_header_t*>(static_cast<EventList*>(l->ctx)->events[i].data());
        } };
    EventList out;
    clap_output_events_t outEv{ &out, [](const clap_output_events_t* l, const clap_event_header_t* h) -> bool {
        static_cast<EventList*>(l->ctx)->add(h);
        return true;
    } };
    clap_process_t proc{};
    proc.frames_count = 2 * kStep;   // no transport: the host is stopped
    proc.in_events = &noIn;
    proc.out_events = &outEv;
    auto noteOns = [&]() {
        std::vector<int> keys;
        for (const auto& e : out.events) {
            const auto* h = reinterpret_cast<const clap_event_header_t*>(e.data());
            if (h->type == CLAP_EVENT_NOTE_ON) keys.push_back(reinterpret_cast<const clap_event_note_t*>(h)->key);
        }
        return keys;
    };
    p->process(p, &proc);
    check(noteOns().empty(), "nothing plays before the play button");
    plugin.setPreviewPattern(2);
    p->process(p, &proc);
    check(noteOns() == std::vector<int>({ 36, 38 }) && plugin.engine().playingPattern() == 2,
          "the play button plays the pattern with the host stopped");
    plugin.setPreviewPattern(-1);
    out.events.clear();
    p->process(p, &proc);
    p->process(p, &proc);
    check(noteOns().empty() && plugin.engine().playingPattern() == -1, "pause stops it");
}

static void click(SequencerGui* gui, SequencerGui::Button b) {
    int x, y, w, h;
    SequencerGui::buttonRect(b, x, y, w, h);
    gui->mouseDown(x + w / 2, y + h / 2, false);
    gui->mouseUp(x + w / 2, y + h / 2);
}

static void testGuiControls() {
    SequencerClap plugin(testHost());
    plugin.createGui();
    SequencerGui* gui = plugin.gui();
    using B = SequencerGui::Button;
    plugin.setEditPattern(2);
    gui->takeActions();

    // Name: type, Enter keeps it.
    click(gui, B::Name);
    check(gui->editingName() && (gui->takeActions() & SequencerGui::kActionTakeFocus), "name click starts editing");
    for (int i = 0; i < 30; ++i) gui->keyPress(SequencerGui::Key::Backspace);
    gui->keyText("my acid~line 1234567890123456");
    gui->keyPress(SequencerGui::Key::Enter);
    check(!gui->editingName() && plugin.bank().name(2) == "MY ACIDLINE 12345678901"
          && (gui->takeActions() & SequencerGui::kActionReleaseFocus),
          "typed name is upper case, panel-font characters only, at most 23 long: " + plugin.bank().name(2));
    // Escape cancels; a click elsewhere keeps.
    click(gui, B::Name);
    gui->keyText("X");
    gui->keyPress(SequencerGui::Key::Escape);
    check(plugin.bank().name(2) == "MY ACIDLINE 12345678901", "escape cancels the name edit");
    click(gui, B::Name);
    gui->keyPress(SequencerGui::Key::Backspace);
    click(gui, B::Follow);
    check(!gui->editingName() && plugin.bank().name(2) == "MY ACIDLINE 1234567890", "a click elsewhere keeps the name");
    check(!plugin.followPlaying(), "follow toggle");
    click(gui, B::Follow);
    check(plugin.followPlaying(), "follow toggles back");

    // INIT needs a second click; a click elsewhere disarms it.
    click(gui, B::Init);
    check(plugin.bank().length(2) == 8 && plugin.bank().step(2, 0).isNote(), "one INIT click only arms");
    click(gui, B::Follow);
    click(gui, B::Follow);
    click(gui, B::Init);
    click(gui, B::Init);
    bool empty = true;
    for (int i = 0; i < kMaxSteps; ++i) empty = empty && plugin.bank().step(2, i).note == kNoteRest;
    check(empty && plugin.bank().length(2) == 16 && plugin.bank().name(2) == "PATTERN 3", "INIT twice clears the pattern");

    // Play / pause; the pattern buttons move a playing preview along.
    click(gui, B::Play);
    check(plugin.previewPattern() == 2, "play holds the edited pattern");
    int x, y, w, h;
    SequencerGui::patternButtonRect(5, x, y, w, h);
    gui->mouseDown(x + 5, y + 5, false); gui->mouseUp(x + 5, y + 5);
    check(plugin.previewPattern() == 5, "selecting a pattern while playing plays it");
    click(gui, B::Play);
    check(plugin.previewPattern() == -1, "pause");

    // Load, save and MIDI drag go to the platform layer.
    gui->takeActions();
    click(gui, B::Load);
    check(gui->takeActions() == SequencerGui::kActionLoadBank, "load button");
    click(gui, B::Save);
    check(gui->takeActions() == SequencerGui::kActionSaveBank, "save button");
    SequencerGui::buttonRect(B::Midi, x, y, w, h);
    gui->mouseDown(x + 10, y + 10, false);
    gui->mouseUp(x + 10, y + 10);
    check(gui->takeActions() == 0, "a click on MIDI is no drag");
    gui->mouseDown(x + 10, y + 10, false);
    gui->mouseDrag(x + 10, y + 20);
    gui->mouseDrag(x + 10, y + 30);
    check(gui->takeActions() == SequencerGui::kActionDragMidi, "dragging MIDI starts one drag");
    gui->mouseUp(x + 10, y + 30);

    // Bank files from the dialogs; saving adds the extension.
    const auto base = tempPath("burette_gui_bank");
    plugin.bank().setName(0, "BANKED");
    gui->saveBankTo(base.u8string());
    plugin.bank().setName(0, "CHANGED");
    gui->loadBankFrom(base.u8string() + ".burette");
    check(plugin.bank().name(0) == "BANKED", "save adds .burette, load reads it back");
    std::filesystem::remove(base.u8string() + ".burette");
    plugin.destroyGui();
}

int main() {
    testGateLengthAndRest();
    testAccentVelocity();
    testSlide();
    testSlideToSamePitchIsTie();
    testTie();
    testSlideAtEndOfTieChain();
    testTieAfterRestIsRest();
    testOctaveAndTranspose();
    testPatternLengthWraps();
    testSlideOnLastStepWraps();
    testHostStartMidStep();
    testFollowsSongPosition();
    testTriggerReleaseLetsStepFinish();
    testGridAlignedTriggerNotes();
    testLateTrigger();
    testFreeRunning();
    testPatternSwitch();
    testLoopJump();
    testNoStuckNotes();
    testFactoryPatterns();
    testChaining();
    testGlobalTranspose();
    testTransposeParam();
    testStepPacking();
    testStateRoundTrip();
    testClapRouting();
    testGuiEditing();
    testMidiExport();
    testBankFile();
    testPreview();
    testGuiControls();
    if (g_failures == 0) std::printf("All sequencer tests passed.\n");
    return g_failures == 0 ? 0 : 1;
}
