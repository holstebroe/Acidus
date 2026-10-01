#ifndef ACIDUS_SEQ_SEQUENCER_CLAP_HPP
#define ACIDUS_SEQ_SEQUENCER_CLAP_HPP

#include <clap/clap.h>
#include <clap/ext/params.h>
#include "Pattern.hpp"
#include "SequencerEngine.hpp"
#include <atomic>
#include <memory>
#include <mutex>
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

    // Global key transpose: the plugin's one CLAP parameter (automatable).
    static constexpr clap_id kParamTranspose = 0;
    int globalTranspose() const { return globalTranspose_.load(std::memory_order_relaxed); }
    // GUI edits, sent to the host as one automation gesture.
    void beginTransposeEdit();
    void setTransposeFromGui(int semitones);
    void endTransposeEdit();

    bool paramsInfo(uint32_t index, clap_param_info_t* info) const;
    bool paramsValue(clap_id id, double* value) const;
    bool paramsValueToText(clap_id id, double value, char* buf, uint32_t size) const;
    bool paramsTextToValue(clap_id id, const char* text, double* value) const;
    void paramsFlush(const clap_input_events_t* in, const clap_output_events_t* out);

private:
    const clap_host_t* host_;
    clap_plugin_t clapPlugin_{};
    PatternBank bank_;
    SequencerEngine engine_{bank_};
    std::unique_ptr<SequencerGui> gui_;
    std::atomic<bool> stateDirty_{false};
    std::atomic<int> editPattern_{0};
    std::atomic<bool> follow_{true};
    std::atomic<int> globalTranspose_{0};

    // GUI parameter events waiting to go to the host.
    struct ParamOut { uint16_t type; double value; };
    std::mutex paramOutMutex_;
    ParamOut paramOut_[64];
    int paramOutCount_{0};
    void queueParamOut(uint16_t type, double value);
    void pushParamOut(const clap_output_events_t* out);
    void requestFlush();
    // A host parameter event: true (and the new value) if it sets the transpose.
    bool transposeEvent(const clap_event_header_t* h, int& semitones);

    std::vector<TriggerEvent> triggers_;
    std::vector<NoteEvent> notes_;
};

} // namespace seq
} // namespace acidus

#endif // ACIDUS_SEQ_SEQUENCER_CLAP_HPP
