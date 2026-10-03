#include "SequencerEngine.hpp"
#include <algorithm>
#include <cmath>

namespace acidus {
namespace seq {

// Fallback for a gate held into the next step when that step boundary never
// comes (transport stopped or jumped): release this long after the step end.
static constexpr double kHeldGateGraceSec = 0.002;

static int64_t floorToInt(double v) {
    return static_cast<int64_t>(std::floor(v));
}

static int wrap(int64_t n, int length) {
    const int64_t m = n % length;
    return static_cast<int>(m < 0 ? m + length : m);
}

void SequencerEngine::setSampleRate(double sampleRate) {
    sampleRate_ = (std::isfinite(sampleRate) && sampleRate >= 1000.0 && sampleRate <= 1.0e6)
                      ? sampleRate : 44100.0;
}

void SequencerEngine::reset() {
    heldCount_ = 0;
    gridMode_ = false;
    freeRunning_ = false;
    samplesSinceBoundary_ = 1 << 30;
    lastBoundaryPlayed_ = true;
    if (sounding_) pendingRelease_ = true;
    playingPattern_.store(-1, std::memory_order_relaxed);
    playingStep_.store(-1, std::memory_order_relaxed);
}

void SequencerEngine::emit(uint32_t time, bool on, int key, float velocity, bool pressure) {
    if (outCount_ < outCapacity_) out_[outCount_++] = { time, on, key, velocity, pressure };
}

void SequencerEngine::release(uint32_t time) {
    if (sounding_) emit(time, false, soundingKey_, 0.0f);
    sounding_ = false;
    soundingAccent_ = false;
    connected_ = false;
    tieSlide_ = false;
    offCountdown_ = 0;
}

int SequencerEngine::chainOf(const PatternBank& bank, int start, int* patterns) {
    int count = 0;
    for (int p = start; p >= 0 && p < kNumPatterns && count < kNumPatterns; p = bank.next(p)) {
        if (std::find(patterns, patterns + count, p) != patterns + count) break;   // back into the chain
        patterns[count++] = p;
    }
    return count;
}

void SequencerEngine::locate(int start, int64_t stepNum, int& pattern, int& index) const {
    int chain[kNumPatterns];
    const int count = chainOf(bank_, start, chain);
    int lengths[kNumPatterns];
    int total = 0;
    for (int i = 0; i < count; ++i) {
        lengths[i] = std::max(1, bank_.length(chain[i]));
        total += lengths[i];
    }
    int pos = wrap(stepNum, std::max(1, total));
    for (int i = 0; i < count; ++i) {
        if (pos < lengths[i]) { pattern = chain[i]; index = pos; return; }
        pos -= lengths[i];
    }
    pattern = start;
    index = 0;
}

void SequencerEngine::handleTrigger(const TriggerEvent& ev, uint32_t time) {
    if (ev.setTranspose) {
        globalTranspose_ = std::min(std::max(ev.transpose, kMinTranspose), kMaxTranspose);
        return;
    }
    const int pattern = ev.key - kFirstTriggerKey;
    if (pattern < 0 || pattern >= kNumPatterns) return;

    const bool wasActive = heldCount_ > 0;
    int* end = held_ + heldCount_;
    int* it = std::remove(held_, end, pattern);
    heldCount_ = static_cast<int>(it - held_);
    if (!ev.on) return;
    held_[heldCount_++] = pattern;
    if (wasActive) return;   // a pattern change takes effect at the next step
    if (!sounding_) lastStep_ = -1;   // nothing playing until its first step

    if (gridMode_) {
        // Grid note delivered a little late: still play the step it was for.
        const double lateSamples = kLateStartSec * sampleRate_;
        if (!lastBoundaryPlayed_ && static_cast<double>(samplesSinceBoundary_) <= lateSamples) {
            stepBoundary(lastStepNum_, time);
        }
    } else {
        // Transport stopped: start the pattern from its first step, now.
        if (connected_) release(time);
        freeRunning_ = true;
        pos_ = 0.0;
        freeSamples_ = 0;
        lastStepNum_ = -1;
    }
}

void SequencerEngine::stepBoundary(int64_t stepNum, uint32_t time) {
    samplesSinceBoundary_ = 0;
    const int trigger = activePattern();
    if (trigger < 0) {
        // No trigger: no new step. A gate held into this step ends here.
        lastBoundaryPlayed_ = false;
        if (connected_) release(time);
        return;
    }
    lastBoundaryPlayed_ = true;

    int pattern, index, nextPattern, nextIndex;
    locate(trigger, stepNum, pattern, index);
    locate(trigger, stepNum + 1, nextPattern, nextIndex);   // the last step wraps to the first
    const Step step = bank_.step(pattern, index);
    const Step next = bank_.step(nextPattern, nextIndex);
    const bool gateOpen = sounding_ && connected_;
    lastPattern_ = pattern;
    lastStep_ = index;

    if (step.isNote()) {
        const int key = bank_.keyFor(pattern, step, globalTranspose_);
        const float velocity = step.accent ? kAccentVelocity : kNormalVelocity;
        if (gateOpen) {
            // Slide: new note first, then the old note's off, at the same
            // sample. An equal-pitch slide is a tie, unless it changes the
            // accent: then only the accent latch moves, sent as pressure.
            if (key != soundingKey_) {
                emit(time, true, key, velocity);
                emit(time, false, soundingKey_, 0.0f);
                soundingKey_ = key;
            } else if (step.accent != soundingAccent_) {
                emit(time, true, key, step.accent ? kAccentPressure : 0.0f, true);
            }
        } else {
            release(time);
            emit(time, true, key, velocity);
            soundingKey_ = key;
            sounding_ = true;
        }
        soundingAccent_ = step.accent;
        tieSlide_ = step.slide;
    } else if (step.note == kNoteTie && gateOpen) {
        tieSlide_ = tieSlide_ || step.slide;
    } else {
        // A rest, or a tie with no note to extend.
        release(time);
        return;
    }

    connected_ = tieSlide_ || next.note == kNoteTie;
    const double stepSamples = 1.0 / stepsPerSample_;
    offCountdown_ = connected_
        ? static_cast<int64_t>(std::ceil(stepSamples + kHeldGateGraceSec * sampleRate_))
        : std::max<int64_t>(1, static_cast<int64_t>(std::llround(kGateFraction * stepSamples)));
}

uint32_t SequencerEngine::process(uint32_t frames, const TransportInfo& t,
                                  const TriggerEvent* in, uint32_t inCount,
                                  NoteEvent* out, uint32_t outCapacity) {
    out_ = out;
    outCount_ = 0;
    outCapacity_ = out ? outCapacity : 0;

    if (pendingRelease_) {
        pendingRelease_ = false;
        release(0);
    }

    if (t.hasTempo && std::isfinite(t.tempo) && t.tempo >= 1.0 && t.tempo <= 2000.0) {
        lastTempo_ = t.tempo;
    }
    stepsPerSample_ = lastTempo_ / 60.0 * 4.0 / sampleRate_;
    const double sps = stepsPerSample_;

    const bool grid = t.playing && t.hasBeats && std::isfinite(t.songPosBeats)
                      && std::fabs(t.songPosBeats) < 1.0e9;
    double gridStart = 0.0;
    if (grid) {
        gridStart = t.songPosBeats * 4.0 - kBoundaryDelaySamples * sps;
        const double tolerance = std::max(4.0 * sps, 1.0e-3);
        if (!gridMode_ || std::fabs(gridStart - expectedNextPos_) > tolerance) {
            // Playback started or jumped: only boundaries crossed from here on.
            lastStepNum_ = floorToInt(gridStart - sps);
            samplesSinceBoundary_ = 1 << 30;
            lastBoundaryPlayed_ = true;
        }
        gridMode_ = true;
        freeRunning_ = false;
    } else if (gridMode_) {
        // Transport stopped: carry on from where the grid was, so a held
        // trigger or a gate waiting for its next step keeps its timing.
        gridMode_ = false;
        pos_ = expectedNextPos_;
        freeSamples_ = 0;
        freeRunning_ = heldCount_ > 0 || sounding_;
    }
    const double stepSamples = 1.0 / sps;
    if (!grid && stepSamples != freeStepSamples_) {
        // Tempo changed: rebase the free clock at the current position.
        if (freeStepSamples_ > 0.0) pos_ += static_cast<double>(freeSamples_) / freeStepSamples_;
        freeSamples_ = 0;
        freeStepSamples_ = stepSamples;
    }

    uint32_t eventIndex = 0;
    for (uint32_t i = 0; i < frames; ++i) {
        while (eventIndex < inCount && in[eventIndex].time <= i) {
            handleTrigger(in[eventIndex], i);
            ++eventIndex;
        }

        if (sounding_ && offCountdown_ > 0 && --offCountdown_ == 0) {
            release(i);
        }

        if (grid || freeRunning_) {
            const double p = grid ? gridStart + static_cast<double>(i) * sps
                                  : pos_ + static_cast<double>(freeSamples_) / stepSamples;
            const int64_t n = floorToInt(p);
            if (n > lastStepNum_) {
                lastStepNum_ = n;
                stepBoundary(n, i);
            } else if (samplesSinceBoundary_ < (1 << 30)) {
                ++samplesSinceBoundary_;
            }
            if (!grid) {
                ++freeSamples_;
                if (heldCount_ == 0 && !sounding_) freeRunning_ = false;
            }
        }
    }
    // Triggers stamped past the block end (malformed): apply them now.
    for (; eventIndex < inCount; ++eventIndex) {
        handleTrigger(in[eventIndex], frames > 0 ? frames - 1 : 0);
    }

    expectedNextPos_ = grid ? gridStart + static_cast<double>(frames) * sps
                            : pos_ + static_cast<double>(freeSamples_) / stepSamples;

    const bool active = heldCount_ > 0 || sounding_;
    // The chain member playing; before a trigger's first step, the trigger.
    playingPattern_.store(active ? (lastStep_ >= 0 ? lastPattern_ : activePattern()) : -1,
                          std::memory_order_relaxed);
    playingStep_.store(active ? lastStep_ : -1, std::memory_order_relaxed);
    return outCount_;
}

} // namespace seq
} // namespace acidus
