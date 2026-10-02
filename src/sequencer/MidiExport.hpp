#ifndef ACIDUS_SEQ_MIDI_EXPORT_HPP
#define ACIDUS_SEQ_MIDI_EXPORT_HPP

#include "Pattern.hpp"
#include <string>
#include <vector>

namespace acidus {
namespace seq {

// MIDI file resolution: ticks per quarter note (24 per 16th step).
constexpr int kMidiPpq = 96;
constexpr int kMidiTicksPerStep = kMidiPpq / 4;
// A slid-from note is lengthened this far past the next note's start, so the
// clip's notes overlap and a mono synth glides when the DAW plays them back.
constexpr int kMidiSlideOverlapTicks = 2;

struct MidiNote {
    int start;      // ticks
    int end;        // ticks
    int key;
    int velocity;   // 1..127
};

// The notes triggering `pattern` plays (the pattern, or its whole chain),
// once, with the global key transpose: rendered by SequencerEngine, so
// gates, slides, ties and accents are exactly what playback sends. Notes
// still sounding at the end are cut there; sorted by start.
std::vector<MidiNote> renderPatternNotes(const PatternBank& bank, int pattern, int globalTranspose);

// Those notes as a Standard MIDI File (format 0, one channel, 4/4, named after
// the pattern), as long as the pattern or chain.
std::string patternMidiFile(const PatternBank& bank, int pattern, int globalTranspose);

// A file name for the pattern's MIDI file: its name, made safe, plus ".mid".
std::string midiFileName(const std::string& patternName);

} // namespace seq
} // namespace acidus

#endif // ACIDUS_SEQ_MIDI_EXPORT_HPP
