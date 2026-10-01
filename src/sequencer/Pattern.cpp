#include "Pattern.hpp"
#include <algorithm>
#include <cstring>

namespace acidus {
namespace seq {

uint8_t Step::pack() const {
    const int n = (note >= kNoteRest && note <= kNoteTie) ? note : kNoteRest;
    const int o = std::min(std::max(octave, -1), 1) + 1;
    return static_cast<uint8_t>(n | (o << 4) | (accent ? 0x40 : 0) | (slide ? 0x80 : 0));
}

Step Step::unpack(uint8_t b) {
    Step s;
    s.note = b & 0x0F;
    if (s.note > kNoteTie) s.note = kNoteRest;
    const int o = (b >> 4) & 0x03;
    s.octave = (o > 2 ? 1 : o) - 1;
    s.accent = (b & 0x40) != 0;
    s.slide = (b & 0x80) != 0;
    return s;
}

static bool validIndex(int pattern, int index = 0) {
    return pattern >= 0 && pattern < kNumPatterns && index >= 0 && index < kMaxSteps;
}

PatternBank::PatternBank() {
    loadFactory();
}

Step PatternBank::step(int pattern, int index) const {
    if (!validIndex(pattern, index)) return Step{};
    return Step::unpack(steps_[pattern][index].load(std::memory_order_relaxed));
}

void PatternBank::setStep(int pattern, int index, const Step& s) {
    if (!validIndex(pattern, index)) return;
    steps_[pattern][index].store(s.pack(), std::memory_order_relaxed);
}

int PatternBank::length(int pattern) const {
    if (!validIndex(pattern)) return kMaxSteps;
    return length_[pattern].load(std::memory_order_relaxed);
}

void PatternBank::setLength(int pattern, int length) {
    if (!validIndex(pattern)) return;
    length_[pattern].store(static_cast<int8_t>(std::min(std::max(length, 1), kMaxSteps)),
                           std::memory_order_relaxed);
}

int PatternBank::transpose(int pattern) const {
    if (!validIndex(pattern)) return 0;
    return transpose_[pattern].load(std::memory_order_relaxed);
}

void PatternBank::setTranspose(int pattern, int semitones) {
    if (!validIndex(pattern)) return;
    transpose_[pattern].store(
        static_cast<int8_t>(std::min(std::max(semitones, kMinTranspose), kMaxTranspose)),
        std::memory_order_relaxed);
}

std::string PatternBank::name(int pattern) const {
    if (!validIndex(pattern)) return {};
    std::lock_guard<std::mutex> lock(nameMutex_);
    return names_[pattern];
}

void PatternBank::setName(int pattern, const std::string& name) {
    if (!validIndex(pattern)) return;
    std::string clean;
    for (char c : name) {
        if (static_cast<int>(clean.size()) >= kMaxNameLength) break;
        clean += (c >= 32 && c < 127) ? c : '?';
    }
    std::lock_guard<std::mutex> lock(nameMutex_);
    names_[pattern] = clean;
}

int PatternBank::next(int pattern) const {
    if (!validIndex(pattern)) return -1;
    return next_[pattern].load(std::memory_order_relaxed);
}

void PatternBank::setNext(int pattern, int next) {
    if (!validIndex(pattern)) return;
    next_[pattern].store(static_cast<int8_t>(next >= 0 && next < kNumPatterns ? next : -1),
                         std::memory_order_relaxed);
}

int PatternBank::keyFor(int pattern, const Step& s, int extraTranspose) const {
    if (!s.isNote()) return -1;
    const int key = kBaseKey + (s.note - 1) + 12 * s.octave + transpose(pattern) + extraTranspose;
    return std::min(std::max(key, 0), 127);
}

void PatternBank::clearPattern(int pattern) {
    if (!validIndex(pattern)) return;
    for (int i = 0; i < kMaxSteps; ++i) setStep(pattern, i, Step{});
    setLength(pattern, kMaxSteps);
    setTranspose(pattern, 0);
    setNext(pattern, -1);
    setName(pattern, "PATTERN " + std::to_string(pattern + 1));
}

// Factory pattern text: one token per step. A token is a note (C, C#, ...,
// B, C' = high C), "T" (tie) or "-" (rest), followed by lower-case flags: u = octave up,
// d = octave down, a = accent, s = slide.
struct FactoryPattern {
    const char* name;
    int transpose;
    int next;           // chained pattern (1-based), 0 = loop
    const char* steps;
};

// Original demo patterns, each showing off a 303 technique.
static const FactoryPattern kFactory[] = {
    // Octave jumps with long slide runs and three accents.
    { "OCTAVE JUMPER", 5, 0,
      "Cd Cu D#s Gu F A#das Gd Cu Cus D# A#ds Cuas C Ds Fds F#s" },
    // An 8-step line with accents carrying the groove.
    { "SQUELCH 8", 0, 0,
      "Eus Ed Ga Bda A Gua Ed Da" },
    // One note, rhythm from accents and octave drops only.
    { "ONE NOTE PUMP", 0, 0,
      "Ed E Ea E Ed E Ea Es" },
    // A pedal note with octave jumps, a tie and slides.
    { "PEDAL DRIVE", 3, 0,
      "Fda Fu F Fds G#u T F Fu Fda Cs Fd Fu A#ua Fd Fus D#" },
    // Long tied notes over a flat second.
    { "PHRYGIAN DRONE", 0, 0,
      "E T T Eu E T Fu T E Eu T F T Eu E T" },
    // A two-pattern chain (6 > 7 > 6 ...) reaching the 303's high C.
    { "CHAIN A", 0, 7,
      "Cd C D#s Fa Gs G#u T Cus" },
    { "CHAIN B", 0, 6,
      "C'ua C' Gs Fa D#s T Cd C'" },
};

static int parseFactorySteps(const char* text, Step* out) {
    static const char* kLetters = "C D EF G A B";   // index = semitone of the natural
    int count = 0;
    const char* p = text;
    while (*p && count < kMaxSteps) {
        while (*p == ' ') ++p;
        if (!*p) break;
        Step s;
        if (*p == 'T') { s.note = kNoteTie; ++p; }
        else if (*p == '-') { s.note = kNoteRest; ++p; }
        else {
            const char* hit = std::strchr(kLetters, *p);
            int semitone = hit && *p != ' ' ? static_cast<int>(hit - kLetters) : 0;
            ++p;
            if (*p == '#') { ++semitone; ++p; }
            s.note = 1 + semitone % 12;
            if (*p == '\'' && s.note == 1) { s.note = kNoteHighC; ++p; }
        }
        for (; *p && *p != ' '; ++p) {
            if (*p == 'u') s.octave = 1;
            else if (*p == 'd') s.octave = -1;
            else if (*p == 'a') s.accent = true;
            else if (*p == 's') s.slide = true;
        }
        out[count++] = s;
    }
    return count;
}

void PatternBank::loadFactory() {
    for (int p = 0; p < kNumPatterns; ++p) clearPattern(p);
    int p = 0;
    for (const auto& f : kFactory) {
        Step steps[kMaxSteps];
        const int n = parseFactorySteps(f.steps, steps);
        for (int i = 0; i < n; ++i) setStep(p, i, steps[i]);
        setLength(p, n);
        setTranspose(p, f.transpose);
        setNext(p, f.next - 1);
        setName(p, f.name);
        ++p;
    }
}

// State: "A3SQ", version byte, then per pattern: length, transpose (int8),
// next (int8, version 2), name length, name bytes, kMaxSteps packed step
// bytes. Version 1 had no next byte and numbered the tie 13 (now high C).
static constexpr char kMagic[4] = { 'A', '3', 'S', 'Q' };
static constexpr uint8_t kVersion = 2;

std::string PatternBank::serialize() const {
    std::string out(kMagic, 4);
    out += static_cast<char>(kVersion);
    for (int p = 0; p < kNumPatterns; ++p) {
        out += static_cast<char>(length(p));
        out += static_cast<char>(static_cast<int8_t>(transpose(p)));
        out += static_cast<char>(static_cast<int8_t>(next(p)));
        const std::string n = name(p);
        out += static_cast<char>(n.size());
        out += n;
        for (int i = 0; i < kMaxSteps; ++i) {
            out += static_cast<char>(steps_[p][i].load(std::memory_order_relaxed));
        }
    }
    return out;
}

size_t PatternBank::deserialize(const uint8_t* data, size_t size) {
    if (!data || size < 5 || std::memcmp(data, kMagic, 4) != 0) return 0;
    const int version = data[4];
    if (version < 1 || version > kVersion) return 0;
    const size_t header = version >= 2 ? 4 : 3;   // length, transpose, [next,] name length
    // Validate the whole blob first, so a truncated one changes nothing.
    size_t pos = 5;
    for (int p = 0; p < kNumPatterns; ++p) {
        if (pos + header > size) return 0;
        pos += header + data[pos + header - 1] + kMaxSteps;
        if (pos > size) return 0;
    }
    pos = 5;
    for (int p = 0; p < kNumPatterns; ++p) {
        setLength(p, data[pos]);
        setTranspose(p, static_cast<int8_t>(data[pos + 1]));
        setNext(p, version >= 2 ? static_cast<int8_t>(data[pos + 2]) : -1);
        const size_t nameLen = data[pos + header - 1];
        pos += header;
        setName(p, std::string(reinterpret_cast<const char*>(data + pos), nameLen));
        pos += nameLen;
        for (int i = 0; i < kMaxSteps; ++i) {
            Step s = Step::unpack(data[pos + i]);
            if (version == 1 && s.note == kNoteHighC) s.note = kNoteTie;
            setStep(p, i, s);
        }
        pos += kMaxSteps;
    }
    return pos;
}

const char* noteName(int note) {
    static const char* kNames[] = { "", "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B", "C'", "T" };
    return (note >= 0 && note < kNoteValueCount) ? kNames[note] : "";
}

std::string keyName(int key) {
    if (key < 0 || key > 127) return "?";
    return std::string(noteName(1 + key % 12)) + std::to_string(key / 12 - 1);
}

} // namespace seq
} // namespace acidus
