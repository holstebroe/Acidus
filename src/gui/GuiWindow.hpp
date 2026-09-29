#ifndef ACIDUS_GUI_WINDOW_HPP
#define ACIDUS_GUI_WINDOW_HPP

#include <clap/clap.h>
#include <clap/ext/gui.h>
#include <vector>
#include <cstdint>
#include <atomic>
#include <thread>
#include <memory>
#include <chrono>
#include <random>
#include "Font.hpp"
#include "IControlRenderer.hpp"

namespace acidus {

class AcidusClap;

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

    void setControlRenderer(std::unique_ptr<IControlRenderer> renderer) {
        if (renderer) {
            controlRenderer_ = std::move(renderer);
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

private:
    AcidusClap* plugin_{nullptr};
    uint32_t width_{1070};
    uint32_t height_{180};

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
    std::unique_ptr<IControlRenderer> controlRenderer_;

    int activeControlIndex_{-1};
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
