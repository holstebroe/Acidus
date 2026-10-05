#ifndef ACIDUS_SEQ_SEQUENCER_SKIN_HPP
#define ACIDUS_SEQ_SEQUENCER_SKIN_HPP

#include <memory>
#include <string>

namespace acidus {
class Graphics;
namespace seq {

class SequencerClap;

// What a skin needs to paint Burette's window: the plugin (pattern bank,
// engine, settings) and the editor's own state.
struct SequencerView {
    SequencerClap& plugin;
    bool playing;              // the play button holds the pattern's trigger
    bool initArmed;            // INIT clicked once: the next click clears
    bool editingName;
    const std::string& nameDraft;
    const std::string& status; // one-line message (load/save results)
};

// The look of Burette's window. SequencerGui owns the layout
// (SequencerLayout.hpp) and input; a skin only paints. Picked at build time
// by the ACIDUS_GUI_STYLE CMake option, like the Acidus panel's IGuiSkin:
// RETRO (RetroSequencerSkin.cpp) or MODERN (ModernSequencerSkin.cpp).
class ISequencerSkin {
public:
    virtual ~ISequencerSkin() = default;
    // Supersampling factor: SequencerGui hands draw() a buffer this many
    // times the window size and box-filters it down.
    virtual int supersample() const = 0;
    virtual void draw(Graphics& g, const SequencerView& view) = 0;
};

// The skin this build was configured with.
std::unique_ptr<ISequencerSkin> createSequencerSkin();

} // namespace seq
} // namespace acidus

#endif // ACIDUS_SEQ_SEQUENCER_SKIN_HPP
