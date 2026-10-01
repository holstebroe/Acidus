#ifndef ACIDUS_SEQ_PATTERN_HPP
#define ACIDUS_SEQ_PATTERN_HPP

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>

namespace acidus {
namespace seq {

constexpr int kNumPatterns = 16;
constexpr int kMaxSteps = 16;

// Note cell values: 0 = rest, 1..12 = C..B, 13 = high C (the 303's C'
// key, an octave above C), 14 = tie.
constexpr int kNoteRest = 0;
constexpr int kNoteHighC = 13;
constexpr int kNoteTie = 14;
constexpr int kNoteValueCount = 15;

// MIDI key of the pattern's C with no octave flag: C2 (MIDI 36, C4 = 60).
// With the octave flags that spans C1..C4 (high C with octave up), plus
// transpose -- the 303's range (TB303_REFERENCE.md §4.5).
constexpr int kBaseKey = 36;
// Pattern n (0-based) is triggered by MIDI key kFirstTriggerKey + n: C2..D#3
// (C4 = 60), the lowest keys of a standard 61-key keyboard.
constexpr int kFirstTriggerKey = 36;

constexpr int kMinTranspose = -12;
constexpr int kMaxTranspose = 12;

struct Step {
    int note{kNoteRest};    // kNoteRest, 1..12 (C..B), kNoteHighC or kNoteTie
    int octave{0};          // -1 down, 0, +1 up
    bool accent{false};
    bool slide{false};

    bool isNote() const { return note >= 1 && note <= kNoteHighC; }
    bool operator==(const Step& o) const {
        return note == o.note && octave == o.octave && accent == o.accent && slide == o.slide;
    }

    // One byte: bits 0-3 note, bits 4-5 octave + 1, bit 6 accent, bit 7 slide.
    uint8_t pack() const;
    static Step unpack(uint8_t b);
};

// Pattern names are at most this many characters.
constexpr int kMaxNameLength = 23;

// The 16 patterns, shared by the GUI/main thread (edits, state) and the audio
// thread (playback). Step, length and transpose values are lock-free atomics
// so the audio thread never blocks; names are display-only and take a mutex
// the audio thread never touches.
class PatternBank {
public:
    PatternBank();

    Step step(int pattern, int index) const;
    void setStep(int pattern, int index, const Step& s);
    int length(int pattern) const;
    void setLength(int pattern, int length);
    int transpose(int pattern) const;
    void setTranspose(int pattern, int semitones);
    // Pattern chaining: the pattern played after this one ends, or -1 to
    // loop this one. See SequencerEngine for how chains play.
    int next(int pattern) const;
    void setNext(int pattern, int next);
    std::string name(int pattern) const;
    void setName(int pattern, const std::string& name);

    // MIDI key a note step plays in this pattern (plus `extraTranspose`
    // semitones, clamped to 0..127), or -1 for a rest/tie.
    int keyFor(int pattern, const Step& s, int extraTranspose = 0) const;

    void clearPattern(int pattern);
    // Loads the factory patterns (original demos, one in every slot).
    void loadFactory();

    // Serialised form: see Pattern.cpp. deserialize returns the bytes used,
    // or 0 (and changes nothing) if the data is not a valid pattern bank.
    std::string serialize() const;
    size_t deserialize(const uint8_t* data, size_t size);

private:
    std::atomic<uint8_t> steps_[kNumPatterns][kMaxSteps];
    std::atomic<int8_t> length_[kNumPatterns];
    std::atomic<int8_t> transpose_[kNumPatterns];
    std::atomic<int8_t> next_[kNumPatterns];
    mutable std::mutex nameMutex_;
    std::string names_[kNumPatterns];
};

// "C", "C#", ... for 1..12, "C'" for high C, "T" for a tie, "" for a rest.
const char* noteName(int note);
// MIDI key name with C4 = 60, e.g. "C2" for key 36.
std::string keyName(int key);

} // namespace seq
} // namespace acidus

#endif // ACIDUS_SEQ_PATTERN_HPP
