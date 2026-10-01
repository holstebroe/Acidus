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

int PatternBank::keyFor(int pattern, const Step& s) const {
    if (!s.isNote()) return -1;
    const int key = kBaseKey + (s.note - 1) + 12 * s.octave + transpose(pattern);
    return std::min(std::max(key, 0), 127);
}

void PatternBank::clearPattern(int pattern) {
    if (!validIndex(pattern)) return;
    for (int i = 0; i < kMaxSteps; ++i) setStep(pattern, i, Step{});
    setLength(pattern, kMaxSteps);
    setTranspose(pattern, 0);
    setName(pattern, "PATTERN " + std::to_string(pattern + 1));
}

// Factory pattern text: one token per step. A token is a note (C, C#, ...,
// B), "T" (tie) or "-" (rest), followed by lower-case flags: u = octave up,
// d = octave down, a = accent, s = slide.
struct FactoryPattern {
    const char* name;
    int transpose;
    const char* steps;
};

static const FactoryPattern kFactory[] = {
    { "DA FUNK", 5,
      "Dd Fu Ds A#u A#as Fd Cdas A#d A#us D Cus Duas D Ds Dds D#s" },
    { "ACID TRACKS 2", 0,
      "Aus Ad Cda A#da Ed C#ua C#d A#a" },
    { "BRAIN TOOL", 0,
      "Dd Da D D D Da D D" },
    { "OVERPOWERED 2", 5,
      "D#d D#d D#ua D# D#u D#s F#u D#s D#das G#ua D#ds T D#u F#u D# C" },
    { "RAGA BHAIRAV 1", 0,
      "D T Du T D T Du D T D#u T D# D#u T Du T" },
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
        setName(p, f.name);
        ++p;
    }
}

// State: "A3SQ", version byte, then per pattern: length, transpose (int8),
// name length, name bytes, kMaxSteps packed step bytes.
static constexpr char kMagic[4] = { 'A', '3', 'S', 'Q' };
static constexpr uint8_t kVersion = 1;

std::string PatternBank::serialize() const {
    std::string out(kMagic, 4);
    out += static_cast<char>(kVersion);
    for (int p = 0; p < kNumPatterns; ++p) {
        out += static_cast<char>(length(p));
        out += static_cast<char>(static_cast<int8_t>(transpose(p)));
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
    if (!data || size < 5 || std::memcmp(data, kMagic, 4) != 0 || data[4] != kVersion) return 0;
    // Validate the whole blob first, so a truncated one changes nothing.
    size_t pos = 5;
    for (int p = 0; p < kNumPatterns; ++p) {
        if (pos + 3 > size) return 0;
        pos += 3 + data[pos + 2] + kMaxSteps;
        if (pos > size) return 0;
    }
    pos = 5;
    for (int p = 0; p < kNumPatterns; ++p) {
        const int len = data[pos];
        const int tr = static_cast<int8_t>(data[pos + 1]);
        const size_t nameLen = data[pos + 2];
        pos += 3;
        setLength(p, len);
        setTranspose(p, tr);
        setName(p, std::string(reinterpret_cast<const char*>(data + pos), nameLen));
        pos += nameLen;
        for (int i = 0; i < kMaxSteps; ++i) setStep(p, i, Step::unpack(data[pos + i]));
        pos += kMaxSteps;
    }
    return pos;
}

const char* noteName(int note) {
    static const char* kNames[] = { "", "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B", "T" };
    return (note >= 0 && note < kNoteValueCount) ? kNames[note] : "";
}

std::string keyName(int key) {
    if (key < 0 || key > 127) return "?";
    return std::string(noteName(1 + key % 12)) + std::to_string(key / 12 - 1);
}

} // namespace seq
} // namespace acidus
