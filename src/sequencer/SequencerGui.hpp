#ifndef ACIDUS_SEQ_SEQUENCER_GUI_HPP
#define ACIDUS_SEQ_SEQUENCER_GUI_HPP

#include <clap/clap.h>
#include <clap/ext/gui.h>
#include "gui/Font.hpp"
#include <atomic>
#include <cstdint>
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
// through them. Same for the Length and Transpose boxes.
class SequencerGui {
public:
    static constexpr int kWidth = 880;
    static constexpr int kHeight = 282;

    enum class Row { Note, Octave, Accent, Slide };
    static constexpr int kRowCount = 4;

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

    // Layout, for input mapping and tests.
    static void cellRect(Row row, int step, int& x, int& y, int& w, int& h);
    static void patternButtonRect(int pattern, int& x, int& y, int& w, int& h);
    static void lengthBoxRect(int& x, int& y, int& w, int& h);
    static void transposeBoxRect(int& x, int& y, int& w, int& h);
    static void followBoxRect(int& x, int& y, int& w, int& h);

private:
    SequencerClap* plugin_;
    std::recursive_mutex mutex_;
    std::vector<uint32_t> pixels_;
    Font font_{Font::classic5x7()};
    std::string lastSignature_;
    int lastPlayingPattern_{-1};
    std::atomic<bool> visible_{true};

    // The control being dragged.
    enum class Target { Idle, Cell, Length, Transpose };
    Target dragTarget_{Target::Idle};
    Row dragRow_{Row::Note};
    int dragStep_{0};
    int dragStartY_{0};
    int dragStartValue_{0};
    bool dragMoved_{false};

    void draw(Graphics& g);
    std::string signature();
    void followPlayingPattern();
    int cellValue(Row row, int step) const;
    void setCellValue(Row row, int step, int value);
    static int valueCount(Row row);
    void present();

    std::atomic<bool> running_{false};
    std::thread eventThread_;
#if defined(__linux__) && !defined(__APPLE__)
    void* display_{nullptr};
    unsigned long window_{0};
    void eventLoopX11();
#elif defined(_WIN32)
    void* hwnd_{nullptr};
public:
    void paintWin32();
private:
#endif
};

} // namespace seq
} // namespace acidus

#endif // ACIDUS_SEQ_SEQUENCER_GUI_HPP
