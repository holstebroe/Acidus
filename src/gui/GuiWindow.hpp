#ifndef ACIDUS_GUI_WINDOW_HPP
#define ACIDUS_GUI_WINDOW_HPP

#include <clap/clap.h>
#include <clap/ext/gui.h>
#include <vector>
#include <cstdint>
#include <atomic>
#include <mutex>
#include <thread>
#include <memory>
#include <chrono>
#include <random>
#include <string>
#include "Font.hpp"
#include "FileDialog.hpp"
#include "IGuiSkin.hpp"

namespace acidus {

class AcidusClap;
class Graphics;

extern const clap_plugin_gui_t g_acidusGuiExtension;

enum class ControlType {
    Knob,
    ToggleSwitch
};

struct Control {
    int id;
    const char* label;
    ControlType type;
    int x, y;
    int radius;
    double minVal, maxVal, currentVal;
    bool isStepped;
    uint32_t accentColor{0}; // 0 = use the renderer's default knob color
};

class GuiWindow {
public:
    // The panel's fixed size, as reported to the host.
    static constexpr uint32_t kDefaultWidth = 854;
    static constexpr uint32_t kDefaultHeight = 180;

    explicit GuiWindow(AcidusClap* plugin);
    ~GuiWindow();

    bool setParent(const clap_window_t* window);
    bool setSize(uint32_t width, uint32_t height);
    bool show();
    bool hide();
    void destroy();

    uint32_t getWidth() const { return width_; }
    // Logo plate rectangle as last drawn (x, y, w, h); for tests.
    void getLogoPlateRect(int& x, int& y, int& w, int& h) const {
        x = logoPlateX_; y = logoPlateY_; w = logoPlateW_; h = logoPlateH_;
    }
    uint32_t getHeight() const { return height_; }
    // Calibration preset label rectangle as last drawn; for tests.
    void getPresetLabelRect(int& x, int& y, int& w, int& h) const {
        x = presetLabelX_; y = presetLabelY_; w = presetLabelW_; h = presetLabelH_;
    }

    const std::vector<uint32_t>& getPixelBuffer() const { return pixelBuffer_; }

    void setFont(const Font& font) { font_ = font; staticKey_.clear(); overlayValid_ = false; }
    const Font& getFont() const { return font_; }

    // Replace the build's skin (tests).
    void setSkin(std::unique_ptr<IGuiSkin> skin) {
        if (skin) {
            skin_ = std::move(skin);
            staticKey_.clear();
        }
    }

    void renderFrame();
    // Advance the logo plate's bubble animation by dt seconds (renderFrame
    // does this with the real frame time; tests call it directly).
    void advanceAnimation(double dt);
    size_t getBubbleCount() const { return bubbles_.size(); }
    // Tests: stop renderFrame from advancing the animation with real time.
    void setAnimationFrozen(bool frozen) { animationFrozen_ = frozen; }
    bool isVisible() const { return visible_; }
    // Tests: the smiley is reacting to an accented note.
    bool isAccentFlashActive() const { return eyeFlash_ > 0.0; }
    // A click on the calibration label (logo plate) loads the next
    // calibration preset.
    void handleMouseDown(int x, int y, bool isShift = false);
    void handleMouseDrag(int x, int y, bool isShift = false);
    void handleMouseUp();

    // Calibration menu: a right-click on the logo plate opens it, with
    // export / import of the calibration (core/CalibrationProfile.hpp) and
    // every calibration slot. A left click picks an item, anywhere else
    // closes it.
    void handleRightClick(int x, int y);
    void handleMouseMove(int x, int y);   // hover highlight
    bool isMenuOpen() const { return menuOpen_; }
    // Tests: the open menu's items.
    int getMenuItemCount() const { return static_cast<int>(menuItems_.size()); }
    std::string getMenuItemLabel(int i) const;
    void getMenuItemRect(int i, int& x, int& y, int& w, int& h) const;

    // Export / import need a file dialog, which the platform layer runs:
    // takeFileRequest() hands out a request once, finishFileRequest() acts
    // on the chosen path (empty: cancelled) and shows the outcome.
    enum class FileRequest { NoFile, Export, Import };
    FileRequest takeFileRequest();
    FileDialogOptions fileDialogOptions(FileRequest request) const;
    void finishFileRequest(FileRequest request, const std::string& path);
    // A short message in the calibration label for a few seconds.
    void showStatus(const std::string& text);
    std::string getStatus() const;

private:
    AcidusClap* plugin_{nullptr};
    uint32_t width_{kDefaultWidth};
    uint32_t height_{kDefaultHeight};

    // The host's UI thread (show/setSize/...) and the window's own event
    // thread both render and handle input; this serialises them.
    mutable std::recursive_mutex guiMutex_;

    std::vector<uint32_t> pixelBuffer_; // ARGB format (32-bit)
    std::vector<uint32_t> hiResBuffer_; // 2x supersampled buffer
    // Render caches (see renderFrame): the static layer, redrawn when a knob
    // value changes, and the logo plate's lettering as a transparent overlay.
    std::vector<uint32_t> staticBuffer_;
    std::vector<uint32_t> logoOverlay_;
    std::vector<uint32_t> logoGlow_;
    std::vector<double> staticKey_;
    bool overlayValid_{false};
    std::atomic<bool> visible_{true};
    bool animationFrozen_{false};
    // Accent reaction and logo glow pulse.
    static constexpr double kEyeFlashSec = 0.18;
    uint32_t lastAccentCount_{0};
    double eyeFlash_{0.0};
    double glowPulse() const;   // halo strength, 0.7..1
    int dirtyX_{0}, dirtyY_{0}, dirtyW_{0}, dirtyH_{0};   // region the last renderFrame repainted
    std::vector<Control> controls_;
    bool lastShiftState_{false};

    Font font_{Font::classic5x7()};
    std::unique_ptr<IGuiSkin> skin_;

    int activeControlIndex_{-1};

    // Calibration menu (see handleRightClick).
    static constexpr int kMenuExport = -1;
    static constexpr int kMenuImport = -2;
    static constexpr int kMenuSeparator = -3;
    struct MenuItem { std::string label; int action; };   // action: a slot index or kMenu*
    std::vector<MenuItem> menuItems_;
    bool menuOpen_{false};
    bool menuDirty_{false};    // the menu was just closed: repaint where it was
    int menuX_{0}, menuY_{0}, menuW_{0};
    int menuHover_{-1};
    static constexpr int kMenuItemH = 13;
    static constexpr int kMenuSeparatorH = 5;
    int menuItemAt(int x, int y) const;
    void openMenu(int x, int y);
    void closeMenu();
    void drawMenu(Graphics& g);
    FileRequest pendingFile_{FileRequest::NoFile};
    std::string lastDir_;
    std::string status_;
    std::chrono::steady_clock::time_point statusUntil_{};
    int logoPlateX_{0}, logoPlateY_{0}, logoPlateW_{0}, logoPlateH_{0}; // set by drawAcidusTitle
    int presetLabelX_{0}, presetLabelY_{0}, presetLabelW_{0}, presetLabelH_{0}; // set by drawAcidusTitle

    // Acid bubbles rising in the logo plate; positions are plate-relative.
    struct Bubble { double x, y, r, speed, phase; };
    std::vector<Bubble> bubbles_;
    std::mt19937 rng_{0xAC1D};
    double bubbleSpawnAccum_{0.0};
    double animTime_{0.0};
    std::chrono::steady_clock::time_point lastFrameTime_{};
    bool haveLastFrameTime_{false};
    void drawBubbles(Graphics& g);
    void drawSmiley(Graphics& g, int cx, int cy);
    int dragStartY_{0};
    double dragStartVal_{0.0};
    // Double-click on a knob resets it to its default value.
    static constexpr std::chrono::milliseconds kDoubleClickWindow{400};
    int lastClickControl_{-1};
    std::chrono::steady_clock::time_point lastClickTime_{};

    std::atomic<bool> isRunning_{false};
    std::thread eventThread_;

#if defined(__linux__) && !defined(__APPLE__)
    void* x11Display_{nullptr};
    unsigned long x11Window_{0};
    unsigned long x11ParentWindow_{0};
    bool x11Created_{false};

    void initX11Window();
    void drawX11Frame();
    void eventLoopX11();
    FileDialogProcess fileDialog_;
#elif defined(_WIN32)
    void* hwnd_{nullptr};
    void* parentHwnd_{nullptr};
public:
    void initWin32Window();
    void drawWin32Frame();
private:
#elif defined(__APPLE__)
    void* nsView_{nullptr};
    void* parentNsView_{nullptr};
    void initCocoaWindow();
    void drawCocoaFrame();
#endif

    void initControls();
    void updateKnobValuesFromPlugin();
    // Parts of the logo plate, drawn in this order around the bubbles.
    static constexpr unsigned kTitlePlate = 1;       // box and shadow (static layer)
    static constexpr unsigned kTitleForeground = 2;  // screws, lettering, tagline (overlay)
    static constexpr unsigned kTitleGlow = 8;        // the lettering's halo (pulsing overlay)
    static constexpr unsigned kTitleDynamic = 4;     // smiley and preset label (every frame)
    void drawAcidusTitle(Graphics& g, int x, int y, unsigned parts);
};

} // namespace acidus

#endif // ACIDUS_GUI_WINDOW_HPP
