#ifndef ACIDUS_SEQ_SEQUENCER_ENGINE_HPP
#define ACIDUS_SEQ_SEQUENCER_ENGINE_HPP

#include "Pattern.hpp"
#include <atomic>
#include <cstdint>

namespace acidus {
namespace seq {

// Host transport for one process block (position at the block's first sample).
struct TransportInfo {
    bool playing{false};
    bool hasTempo{false};
    double tempo{120.0};          // BPM
    bool hasBeats{false};
    double songPosBeats{0.0};     // quarter notes
};

// A pattern-trigger key going down or up (key kFirstTriggerKey + pattern).
struct TriggerEvent {
    uint32_t time;
    bool on;
    int key;
};

// A note the sequencer plays, for the synth chained after it.
struct NoteEvent {
    uint32_t time;
    bool on;
    int key;
    float velocity;
};

// A TB-303-style step sequencer turning pattern-trigger keys into the notes
// Acidus needs to reproduce the 303's Gate/Accent/Slide timing
// (TB303_REFERENCE.md §4):
//
// - Steps are 16th notes. A normal step's gate is high for the first half of
//   the step (§4.3: 50 %, the measured value; the VCA release is the synth's).
// - Slide: the slid-from step keeps its gate high to the end of the step, and
//   the next note-on arrives *before* the previous note's note-off, at the
//   same sample. A mono synth (Acidus included) sees overlapping notes and
//   glides without retriggering its envelopes (§22).
// - Tie: the tied-into step sends nothing; the previous step's gate is just
//   held through it. A slide flag on a note applies at the end of its tie
//   chain, as on the 303 where the flag belongs to the pitch, not the step.
//   A slide between equal pitches is a tie (§4.4): no new note, no accent.
// - Accent: velocity 127; normal notes 100 (Acidus accents at >= 0.8).
//
// The step grid is locked to the host's song position: step n plays at 16th
// note n of the song, pattern step (n mod length). Playback started (or
// looped) mid-step waits for the next step boundary. With the transport
// stopped, a trigger key starts the pattern immediately from step 1 on a
// free-running clock at the host tempo.
//
// A trigger key only decides whether a step *starts*: once started, a step
// plays out (its gate, slide or tie) whatever the trigger does afterwards.
//
// Threading: process() on the audio thread; playingPattern()/playingStep()
// from any thread.
class SequencerEngine {
public:
    static constexpr double kGateFraction = 0.5;
    static constexpr float kNormalVelocity = 100.0f / 127.0f;
    static constexpr float kAccentVelocity = 1.0f;
    // Step boundaries are placed this many samples after the exact grid time,
    // so a trigger note placed on the grid (which a host may round to either
    // neighbouring sample) is always seen before the step it starts or ends.
    static constexpr double kBoundaryDelaySamples = 2.0;
    // A trigger arriving at most this late after an unplayed step boundary
    // still plays that step (hosts that deliver grid notes late).
    static constexpr double kLateStartSec = 0.010;

    explicit SequencerEngine(const PatternBank& bank) : bank_(bank) {}

    void setSampleRate(double sampleRate);
    // Forget held triggers and stop; a sounding note is released at the
    // start of the next process() call.
    void reset();

    // Processes one block. `in` must be sorted by time. Writes at most
    // `outCapacity` events to `out` (sorted by time) and returns the count.
    uint32_t process(uint32_t frames, const TransportInfo& transport,
                     const TriggerEvent* in, uint32_t inCount,
                     NoteEvent* out, uint32_t outCapacity);

    // Pattern being played (0-based), or -1; and its current step, or -1.
    int playingPattern() const { return playingPattern_.load(std::memory_order_relaxed); }
    int playingStep() const { return playingStep_.load(std::memory_order_relaxed); }

private:
    const PatternBank& bank_;
    double sampleRate_{44100.0};
    double lastTempo_{120.0};

    // Held trigger patterns, oldest first; the newest one plays.
    int held_[kNumPatterns]{};
    int heldCount_{0};
    int activePattern() const { return heldCount_ > 0 ? held_[heldCount_ - 1] : -1; }

    // Step clock. In grid mode the position comes from the host every block;
    // in free mode it runs from a trigger.
    bool gridMode_{false};
    bool freeRunning_{false};
    double pos_{0.0};               // free mode: step position at freeSamples_ == 0
    int64_t freeSamples_{0};        // free mode: samples since pos_ (exact, no drift)
    double freeStepSamples_{0.0};   // free mode: samples per step pos_ was set at
    double expectedNextPos_{0.0};   // grid position the next block should start at
    int64_t lastStepNum_{0};        // last step boundary crossed
    int64_t samplesSinceBoundary_{1 << 30};
    bool lastBoundaryPlayed_{true};
    double stepsPerSample_{0.0};

    // The note currently gated on.
    bool sounding_{false};
    int soundingKey_{-1};
    bool connected_{false};         // gate held into the next step (slide/tie)
    bool chainSlide_{false};        // the sounding note's slide flag (applies at the end of its ties)
    int64_t offCountdown_{0};
    bool pendingRelease_{false};
    int lastPattern_{-1};
    int lastStep_{-1};

    NoteEvent* out_{nullptr};
    uint32_t outCount_{0};
    uint32_t outCapacity_{0};

    std::atomic<int> playingPattern_{-1};
    std::atomic<int> playingStep_{-1};

    void emit(uint32_t time, bool on, int key, float velocity);
    void release(uint32_t time);
    void handleTrigger(const TriggerEvent& ev, uint32_t time);
    void stepBoundary(int64_t stepNum, uint32_t time);
};

} // namespace seq
} // namespace acidus

#endif // ACIDUS_SEQ_SEQUENCER_ENGINE_HPP
