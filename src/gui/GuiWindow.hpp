#ifndef SYREBAS_GUI_WINDOW_HPP
#define SYREBAS_GUI_WINDOW_HPP

#include <clap/clap.h>
#include <clap/ext/gui.h>
#include <vector>
#include <cstdint>
#include <atomic>
#include <thread>

namespace syrebas {

class SyrebasClap;

extern const clap_plugin_gui_t g_syrebasGuiExtension;

struct Knob {
    int id;
    const char* label;
    int x, y, radius;
    double minVal, maxVal, currentVal;
    bool isStepped;
};

class GuiWindow {
public:
    explicit GuiWindow(SyrebasClap* plugin);
    ~GuiWindow();

    bool setParent(const clap_window_t* window);
    bool setSize(uint32_t width, uint32_t height);
    bool show();
    bool hide();
    void destroy();

    uint32_t getWidth() const { return width_; }
    uint32_t getHeight() const { return height_; }

    void renderFrame();
    void handleMouseDown(int x, int y);
    void handleMouseDrag(int x, int y);
    void handleMouseUp();

private:
    SyrebasClap* plugin_{nullptr};
    uint32_t width_{600};
    uint32_t height_{320};

    std::vector<uint32_t> pixelBuffer_; // ARGB format (32-bit)
    std::vector<Knob> knobs_;

    int activeKnobIndex_{-1};
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

    void initKnobs();
    void updateKnobValuesFromPlugin();
    void drawRect(int x, int y, int w, int h, uint32_t color);
    void drawCircle(int cx, int cy, int radius, uint32_t color);
    void drawLine(int x0, int y0, int x1, int y1, uint32_t color);
    void drawTextLogo(int x, int y);
    void drawKnob(const Knob& knob);
};

} // namespace syrebas

#endif // SYREBAS_GUI_WINDOW_HPP
