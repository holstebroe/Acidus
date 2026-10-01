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

// Note cell values: 0 = rest, 1..12 = C..B, 13 = tie.
constexpr int kNoteRest = 0;
constexpr int kNoteTie = 13;
constexpr int kNoteValueCount = 14;

// MIDI key of the pattern's C with no octave flag: C2 (MIDI 36, C4 = 60).
// With the octave flags that spans C1..B3, plus transpose -- the 303's
// three-octave range (TB303_REFERENCE.md §4.5).
constexpr int kBaseKey = 36;
// Pattern n (0-based) is triggered by MIDI key kFirstTriggerKey + n (C-1 = 0).
constexpr int kFirstTriggerKey = 0;

constexpr int kMinTranspose = -12;
constexpr int kMaxTranspose = 12;

struct Step {
    int note{kNoteRest};    // kNoteRest, 1..12 (C..B) or kNoteTie
    int octave{0};          // -1 down, 0, +1 up
    bool accent{false};
    bool slide{false};

    bool isNote() const { return note >= 1 && note <= 12; }
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
    std::string name(int pattern) const;
    void setName(int pattern, const std::string& name);

    // MIDI key a note step plays in this pattern, or -1 for a rest/tie.
    int keyFor(int pattern, const Step& s) const;

    void clearPattern(int pattern);
    // Loads the factory patterns (the five transcriptions in slots 1-5, the
    // rest empty).
    void loadFactory();

    // Serialised form: see Pattern.cpp. deserialize returns the bytes used,
    // or 0 (and changes nothing) if the data is not a valid pattern bank.
    std::string serialize() const;
    size_t deserialize(const uint8_t* data, size_t size);

private:
    std::atomic<uint8_t> steps_[kNumPatterns][kMaxSteps];
    std::atomic<int8_t> length_[kNumPatterns];
    std::atomic<int8_t> transpose_[kNumPatterns];
    mutable std::mutex nameMutex_;
    std::string names_[kNumPatterns];
};

// "C", "C#", ... for 1..12; "T" for a tie, "" for a rest.
const char* noteName(int note);
// MIDI key name with C4 = 60, e.g. "C-1" for key 0.
std::string keyName(int key);

} // namespace seq
} // namespace acidus

#endif // ACIDUS_SEQ_PATTERN_HPP
