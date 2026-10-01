#ifndef ACIDUS_SEQ_SEQUENCER_CLAP_HPP
#define ACIDUS_SEQ_SEQUENCER_CLAP_HPP

#include <clap/clap.h>
#include "Pattern.hpp"
#include "SequencerEngine.hpp"
#include <atomic>
#include <memory>
#include <vector>

namespace acidus {
namespace seq {

class SequencerGui;

// Acidus Seq: a separate CLAP plugin (acidus_seq.clap) that turns held
// pattern-trigger keys (C-1 = pattern 1 ... D#0 = pattern 16) into a
// TB-303-timed note stream on its note output, for Acidus (or any mono synth
// that slides on overlapping notes) chained after it. Notes and MIDI outside
// the trigger range pass straight through.
class SequencerClap {
public:
    explicit SequencerClap(const clap_host_t* host);
    ~SequencerClap();

    const clap_plugin_t* getClapPlugin() const { return &clapPlugin_; }

    bool activate(double sampleRate);
    void reset();
    clap_process_status process(const clap_process_t* process);
    const void* getExtension(const char* id);
    void onMainThread();

    bool stateSave(const clap_ostream_t* stream);
    bool stateLoad(const clap_istream_t* stream);

    void createGui();
    void destroyGui();
    SequencerGui* gui() { return gui_.get(); }

    PatternBank& bank() { return bank_; }
    const SequencerEngine& engine() const { return engine_; }
    // The GUI changed a pattern: tell the host the project is dirty.
    void markStateDirty();

    // Pattern the editor shows (part of the saved state).
    int editPattern() const { return editPattern_.load(std::memory_order_relaxed); }
    void setEditPattern(int p);
    bool followPlaying() const { return follow_.load(std::memory_order_relaxed); }
    void setFollowPlaying(bool f) { follow_.store(f, std::memory_order_relaxed); markStateDirty(); }

private:
    const clap_host_t* host_;
    clap_plugin_t clapPlugin_{};
    PatternBank bank_;
    SequencerEngine engine_{bank_};
    std::unique_ptr<SequencerGui> gui_;
    std::atomic<bool> stateDirty_{false};
    std::atomic<int> editPattern_{0};
    std::atomic<bool> follow_{true};

    std::vector<TriggerEvent> triggers_;
    std::vector<NoteEvent> notes_;
};

} // namespace seq
} // namespace acidus

#endif // ACIDUS_SEQ_SEQUENCER_CLAP_HPP
