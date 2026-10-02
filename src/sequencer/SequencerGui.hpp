#ifndef ACIDUS_SEQ_SEQUENCER_GUI_HPP
#define ACIDUS_SEQ_SEQUENCER_GUI_HPP

#include <clap/clap.h>
#include <clap/ext/gui.h>
#include "gui/Font.hpp"
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace acidus {
class Graphics;
namespace seq {

class SequencerClap;

extern const clap_plugin_gui_t g_sequencerGuiExtension;

// The pattern editor: a row of 16 pattern buttons and a Note / Octave /
// Accent / Slide grid for the pattern being edited. Left-click a cell for the
// next value, right-click for the previous one, or left-drag up/down to cycle
// through them. The value boxes (pattern length, transpose and next pattern,
// and the global key transpose) work the same way.
//
// The title row: play/pause (holds the edited pattern's trigger), the
// pattern name (click to type a new one), INIT (click twice to clear the
// pattern), load / save of the pattern bank as a .burette file, and the
// MIDI button, dragged onto a DAW track to drop the pattern there as a
// MIDI clip.
class SequencerGui {
public:
    static constexpr int kWidth = 880;
    static constexpr int kHeight = 336;

    enum class Row { Note, Octave, Accent, Slide };
    static constexpr int kRowCount = 4;
    // Value boxes: the edited pattern's length, transpose and next pattern,
    // and the global key transpose (a host parameter).
    enum class Box { Length, Transpose, Next, Key };
    static constexpr int kBoxCount = 4;

    explicit SequencerGui(SequencerClap* plugin);
    ~SequencerGui();

    bool setParent(const clap_window_t* window);
    bool show();
    bool hide();
    void destroy();

    // Renders the frame into the pixel buffer and, when a window exists,
    // puts it on screen.
    void renderFrame();
    const std::vector<uint32_t>& pixels() const { return pixels_; }

    // Mouse input in window coordinates.
    void mouseDown(int x, int y, bool rightButton);
    void mouseDrag(int x, int y);
    void mouseUp(int x, int y);

    // Keyboard input, used while the name is being edited: printable text,
    // and Enter (keep), Escape (cancel), Backspace.
    enum class Key { Enter, Escape, Backspace };
    void keyText(const char* text);
    void keyPress(Key key);
    // The window lost the keyboard: a name being edited is kept.
    void focusLost();
    bool editingName() const { return editingName_; }

    // Work for the platform layer, which does it outside the GUI lock (file
    // dialogs and drag-and-drop run their own event loops).
    enum Action : uint32_t {
        kActionLoadBank = 1u << 0,      // ask for a .burette file to load
        kActionSaveBank = 1u << 1,      // ask where to save the bank
        kActionDragMidi = 1u << 2,      // start dragging the pattern's MIDI file
        kActionTakeFocus = 1u << 3,     // the name editor wants the keyboard
        kActionReleaseFocus = 1u << 4,  // ... and is done with it
    };
    uint32_t takeActions();
    // A one-line message under the trigger key text (load/save results etc.).
    void setStatus(const std::string& text);
    // A .burette path picked in a load or save dialog.
    void loadBankFrom(const std::string& utf8Path);
    void saveBankTo(const std::string& utf8Path);

    // Layout, for input mapping and tests.
    enum class Button { Play, Name, Init, Load, Save, Midi, Follow };
    static void cellRect(Row row, int step, int& x, int& y, int& w, int& h);
    static void patternButtonRect(int pattern, int& x, int& y, int& w, int& h);
    static void boxRect(Box box, int& x, int& y, int& w, int& h);
    static void buttonRect(Button button, int& x, int& y, int& w, int& h);

private:
    SequencerClap* plugin_;
    std::recursive_mutex mutex_;
    std::vector<uint32_t> pixels_;
    Font font_{Font::classic5x7()};
    std::string lastSignature_;
    int lastPlayingPattern_{-1};
    std::atomic<bool> visible_{true};

    // The control being dragged.
    enum class Target { Idle, Cell, Box, Midi };
    Target dragTarget_{Target::Idle};
    Row dragRow_{Row::Note};
    Box dragBox_{Box::Length};
    int dragStep_{0};
    int dragStartY_{0};
    int dragStartValue_{0};
    bool dragMoved_{false};
    int dragStartX_{0};

    bool editingName_{false};
    std::string nameDraft_;
    bool initArmed_{false};     // INIT clicked once: the next click clears
    std::string status_;
    uint32_t actions_{0};

    void commitName();
    bool playing() const;

    void draw(Graphics& g);
    std::string signature();
    void followPlayingPattern();
    int cellValue(Row row, int step) const;
    void setCellValue(Row row, int step, int value);
    static int valueCount(Row row);
    int boxValue(Box box) const;
    void setBoxValue(Box box, int value);
    void present();

    std::atomic<bool> running_{false};
    std::thread eventThread_;
#if defined(__linux__) && !defined(__APPLE__)
    void* display_{nullptr};
    unsigned long window_{0};
    unsigned long parent_{0};
    struct X11Extras;   // file dialog process and drag source (SequencerGui.cpp)
    std::unique_ptr<X11Extras> x11_;
    void eventLoopX11();
    void performActionsX11(uint32_t actions, int rootX, int rootY, unsigned long time);
#elif defined(_WIN32)
    void* hwnd_{nullptr};
public:
    void paintWin32();
    void performActionsWin32(uint32_t actions);
private:
#endif
};

} // namespace seq
} // namespace acidus

#endif // ACIDUS_SEQ_SEQUENCER_GUI_HPP
