#include "GuiWindow.hpp"
#include "clap/SyrebasClap.hpp"
#include <cmath>
#include <cstring>
#include <algorithm>
#include <iostream>

#if defined(__linux__) && !defined(__APPLE__)
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif

#if defined(_WIN32)
#include <windows.h>
#endif

namespace syrebas {

GuiWindow::GuiWindow(SyrebasClap* plugin) : plugin_(plugin) {
    pixelBuffer_.resize(width_ * height_, 0xFF282828);
    initKnobs();
}

GuiWindow::~GuiWindow() {
    destroy();
}

void GuiWindow::initKnobs() {
    knobs_.clear();
    knobs_.push_back({ PARAM_CUTOFF, "CUTOFF", 70, 180, 28, 300.0, 10000.0, 800.0, false });
    knobs_.push_back({ PARAM_RESONANCE, "RESONANCE", 155, 180, 28, 0.0, 1.0, 0.5, false });
    knobs_.push_back({ PARAM_ENV_MOD, "ENV MOD", 240, 180, 28, 0.0, 1.0, 0.5, false });
    knobs_.push_back({ PARAM_DECAY, "DECAY", 325, 180, 28, 0.0, 1.0, 0.5, false });
    knobs_.push_back({ PARAM_ACCENT, "ACCENT", 410, 180, 28, 0.0, 1.0, 0.5, false });
    knobs_.push_back({ PARAM_WAVEFORM, "WAVE", 495, 180, 22, 0.0, 1.0, 0.0, true });
    knobs_.push_back({ PARAM_VOLUME, "VOLUME", 560, 180, 20, 0.0, 1.0, 0.8, false });

    updateKnobValuesFromPlugin();
}

void GuiWindow::updateKnobValuesFromPlugin() {
    if (!plugin_) return;
    for (auto& k : knobs_) {
        double val = 0.0;
        if (plugin_->paramsValue(k.id, &val)) {
            k.currentVal = val;
        }
    }
}

void GuiWindow::drawRect(int x, int y, int w, int h, uint32_t color) {
    int xEnd = std::min(x + w, static_cast<int>(width_));
    int yEnd = std::min(y + h, static_cast<int>(height_));
    int xStart = std::max(0, x);
    int yStart = std::max(0, y);

    for (int py = yStart; py < yEnd; ++py) {
        for (int px = xStart; px < xEnd; ++px) {
            pixelBuffer_[py * width_ + px] = color;
        }
    }
}

void GuiWindow::drawCircle(int cx, int cy, int radius, uint32_t color) {
    int r2 = radius * radius;
    for (int dy = -radius; dy <= radius; ++dy) {
        for (int dx = -radius; dx <= radius; ++dx) {
            if (dx * dx + dy * dy <= r2) {
                int px = cx + dx;
                int py = cy + dy;
                if (px >= 0 && px < static_cast<int>(width_) && py >= 0 && py < static_cast<int>(height_)) {
                    pixelBuffer_[py * width_ + px] = color;
                }
            }
        }
    }
}

void GuiWindow::drawLine(int x0, int y0, int x1, int y1, uint32_t color) {
    int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;

    while (true) {
        if (x0 >= 0 && x0 < static_cast<int>(width_) && y0 >= 0 && y0 < static_cast<int>(height_)) {
            pixelBuffer_[y0 * width_ + x0] = color;
        }
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void GuiWindow::drawTextLogo(int x, int y) {
    drawRect(x, y, 560, 60, 0xFF181818);
    drawRect(x + 5, y + 5, 550, 50, 0xFF2A2A2A);
    drawRect(x + 10, y + 50, 540, 3, 0xFF42A5F5);

    const char* logoText = "SYREBAS 303";
    int posX = x + 20;
    int posY = y + 15;

    for (int i = 0; logoText[i] != '\0'; ++i) {
        char c = logoText[i];
        if (c == ' ') {
            posX += 20;
            continue;
        }
        drawRect(posX, posY, 14, 22, 0xFFE0E0E0);
        drawRect(posX + 3, posY + 3, 8, 16, 0xFF2A2A2A);

        if (c == 'S' || c == '3') {
            drawRect(posX, posY, 14, 5, 0xFF42A5F5);
            drawRect(posX, posY + 9, 14, 4, 0xFF42A5F5);
            drawRect(posX, posY + 17, 14, 5, 0xFF42A5F5);
        } else if (c == 'Y') {
            drawRect(posX, posY, 14, 5, 0xFFFFB74D);
            drawRect(posX + 4, posY + 10, 6, 12, 0xFFFFB74D);
        } else if (c == 'R' || c == 'B' || c == 'A') {
            drawRect(posX, posY, 14, 5, 0xFF81C784);
            drawRect(posX, posY + 10, 14, 4, 0xFF81C784);
        } else if (c == 'E') {
            drawRect(posX, posY, 14, 5, 0xFFBA68C8);
            drawRect(posX, posY + 9, 10, 4, 0xFFBA68C8);
            drawRect(posX, posY + 17, 14, 5, 0xFFBA68C8);
        } else if (c == '0') {
            drawRect(posX, posY, 14, 4, 0xFF42A5F5);
            drawRect(posX, posY + 18, 14, 4, 0xFF42A5F5);
        }
        posX += 22;
    }
}

void GuiWindow::drawKnob(const Knob& knob) {
    drawCircle(knob.x, knob.y, knob.radius + 3, 0xFF121212);
    drawCircle(knob.x, knob.y, knob.radius, 0xFF3A3A3A);
    drawCircle(knob.x, knob.y, knob.radius - 2, 0xFF505050);

    double normVal = (knob.currentVal - knob.minVal) / (knob.maxVal - knob.minVal);
    normVal = (std::min)((std::max)(normVal, 0.0), 1.0);
    double angleRad = (0.75 + normVal * 1.5) * 3.14159265358979323846;

    int ptrX = knob.x + static_cast<int>(std::cos(angleRad) * (knob.radius - 5));
    int ptrY = knob.y + static_cast<int>(std::sin(angleRad) * (knob.radius - 5));

    drawLine(knob.x, knob.y, ptrX, ptrY, 0xFF64B5F6);
    drawCircle(ptrX, ptrY, 2, 0xFFE3F2FD);

    drawRect(knob.x - knob.radius - 5, knob.y + knob.radius + 8, knob.radius * 2 + 10, 16, 0xFF181818);
}

void GuiWindow::renderFrame() {
    updateKnobValuesFromPlugin();

    std::fill(pixelBuffer_.begin(), pixelBuffer_.end(), 0xFF383838);
    drawRect(10, 10, width_ - 20, height_ - 20, 0xFF2B2B2B);

    drawTextLogo(20, 20);

    for (const auto& knob : knobs_) {
        drawKnob(knob);
    }

#if defined(__linux__) && !defined(__APPLE__)
    drawX11Frame();
#elif defined(_WIN32)
    drawWin32Frame();
#elif defined(__APPLE__)
    drawCocoaFrame();
#endif
}

void GuiWindow::handleMouseDown(int x, int y) {
    for (size_t i = 0; i < knobs_.size(); ++i) {
        int dx = x - knobs_[i].x;
        int dy = y - knobs_[i].y;
        if (dx * dx + dy * dy <= knobs_[i].radius * knobs_[i].radius + 50) {
            activeKnobIndex_ = static_cast<int>(i);
            dragStartY_ = y;
            dragStartVal_ = knobs_[i].currentVal;
            break;
        }
    }
}

void GuiWindow::handleMouseDrag(int x, int y) {
    if (activeKnobIndex_ < 0 || activeKnobIndex_ >= static_cast<int>(knobs_.size())) return;

    auto& knob = knobs_[activeKnobIndex_];
    int deltaY = dragStartY_ - y;

    double range = knob.maxVal - knob.minVal;
    double sensitivity = 0.005 * range;
    double newVal = dragStartVal_ + deltaY * sensitivity;

    if (knob.isStepped) {
        newVal = (newVal >= 0.5) ? 1.0 : 0.0;
    } else {
        newVal = (std::min)((std::max)(newVal, knob.minVal), knob.maxVal);
    }

    knob.currentVal = newVal;

    if (plugin_) {
        plugin_->getEngine().getParams().cutoff = (knob.id == PARAM_CUTOFF) ? static_cast<float>(newVal) : plugin_->getEngine().getParams().cutoff;
        plugin_->getEngine().getParams().resonance = (knob.id == PARAM_RESONANCE) ? static_cast<float>(newVal) : plugin_->getEngine().getParams().resonance;
        plugin_->getEngine().getParams().envMod = (knob.id == PARAM_ENV_MOD) ? static_cast<float>(newVal) : plugin_->getEngine().getParams().envMod;
        plugin_->getEngine().getParams().decay = (knob.id == PARAM_DECAY) ? static_cast<float>(newVal) : plugin_->getEngine().getParams().decay;
        plugin_->getEngine().getParams().accent = (knob.id == PARAM_ACCENT) ? static_cast<float>(newVal) : plugin_->getEngine().getParams().accent;
        plugin_->getEngine().getParams().waveform = (knob.id == PARAM_WAVEFORM) ? ((newVal >= 0.5) ? Waveform::Square : Waveform::Saw) : plugin_->getEngine().getParams().waveform;
        plugin_->getEngine().getParams().masterVolume = (knob.id == PARAM_VOLUME) ? static_cast<float>(newVal) : plugin_->getEngine().getParams().masterVolume;
    }

    renderFrame();
}

void GuiWindow::handleMouseUp() {
    activeKnobIndex_ = -1;
}

bool GuiWindow::setParent(const clap_window_t* window) {
    if (!window) return false;
#if defined(__linux__) && !defined(__APPLE__)
    if (std::strcmp(window->api, CLAP_WINDOW_API_X11) == 0) {
        x11ParentWindow_ = window->x11;
        initX11Window();
        return true;
    }
#elif defined(_WIN32)
    if (std::strcmp(window->api, CLAP_WINDOW_API_WIN32) == 0) {
        parentHwnd_ = window->win32;
        initWin32Window();
        return true;
    }
#elif defined(__APPLE__)
    if (std::strcmp(window->api, CLAP_WINDOW_API_COCOA) == 0) {
        parentNsView_ = window->cocoa;
        initCocoaWindow();
        return true;
    }
#endif
    return false;
}

bool GuiWindow::setSize(uint32_t width, uint32_t height) {
    width_ = width;
    height_ = height;
    pixelBuffer_.resize(width_ * height_, 0xFF282828);
    renderFrame();
    return true;
}

bool GuiWindow::show() {
    renderFrame();
    return true;
}

bool GuiWindow::hide() {
    return true;
}

void GuiWindow::destroy() {
    isRunning_ = false;
    if (eventThread_.joinable()) {
        eventThread_.join();
    }
#if defined(__linux__) && !defined(__APPLE__)
    if (x11Display_ && x11Created_) {
        Display* display = static_cast<Display*>(x11Display_);
        XDestroyWindow(display, x11Window_);
        XCloseDisplay(display);
        x11Display_ = nullptr;
        x11Created_ = false;
    }
#endif
}

#if defined(__linux__) && !defined(__APPLE__)
void GuiWindow::initX11Window() {
    if (x11Created_) return;

    Display* display = XOpenDisplay(nullptr);
    if (!display) return;

    x11Display_ = display;
    int screen = DefaultScreen(display);
    Window parent = x11ParentWindow_ ? x11ParentWindow_ : RootWindow(display, screen);

    x11Window_ = XCreateSimpleWindow(display, parent, 0, 0, width_, height_, 0,
                                     BlackPixel(display, screen), WhitePixel(display, screen));

    XSelectInput(display, x11Window_, ExposureMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask);
    XMapWindow(display, x11Window_);
    XFlush(display);

    x11Created_ = true;
    renderFrame();

    isRunning_ = true;
    eventThread_ = std::thread(&GuiWindow::eventLoopX11, this);
}

void GuiWindow::eventLoopX11() {
    if (!x11Display_) return;
    Display* display = static_cast<Display*>(x11Display_);

    while (isRunning_) {
        while (XPending(display) > 0) {
            XEvent ev;
            XNextEvent(display, &ev);

            if (ev.type == Expose) {
                drawX11Frame();
            } else if (ev.type == ButtonPress) {
                handleMouseDown(ev.xbutton.x, ev.xbutton.y);
            } else if (ev.type == MotionNotify) {
                if (ev.xmotion.state & Button1Mask) {
                    handleMouseDrag(ev.xmotion.x, ev.xmotion.y);
                }
            } else if (ev.type == ButtonRelease) {
                handleMouseUp();
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}

void GuiWindow::drawX11Frame() {
    if (!x11Display_ || !x11Created_) return;

    Display* display = static_cast<Display*>(x11Display_);
    int screen = DefaultScreen(display);

    XImage* image = XCreateImage(display, DefaultVisual(display, screen),
                                 24, ZPixmap, 0,
                                 reinterpret_cast<char*>(pixelBuffer_.data()),
                                 width_, height_, 32, 0);

    GC gc = DefaultGC(display, screen);
    XPutImage(display, x11Window_, gc, image, 0, 0, 0, 0, width_, height_);

    image->data = nullptr;
    XDestroyImage(image);
    XFlush(display);
}
#endif

#if defined(_WIN32)
void GuiWindow::initWin32Window() {}
void GuiWindow::drawWin32Frame() {}
#endif

#if defined(__APPLE__)
void GuiWindow::initCocoaWindow() {}
void GuiWindow::drawCocoaFrame() {}
#endif

// CLAP GUI Extension Callbacks
const clap_plugin_gui_t g_syrebasGuiExtension = {
    [](const clap_plugin_t* plugin, const char* api, bool is_floating) -> bool {
#if defined(__linux__) && !defined(__APPLE__)
        return std::strcmp(api, CLAP_WINDOW_API_X11) == 0 && !is_floating;
#elif defined(_WIN32)
        return std::strcmp(api, CLAP_WINDOW_API_WIN32) == 0 && !is_floating;
#elif defined(__APPLE__)
        return std::strcmp(api, CLAP_WINDOW_API_COCOA) == 0 && !is_floating;
#else
        return false;
#endif
    },
    [](const clap_plugin_t* plugin, const char** api, bool* is_floating) -> bool {
#if defined(__linux__) && !defined(__APPLE__)
        *api = CLAP_WINDOW_API_X11;
#elif defined(_WIN32)
        *api = CLAP_WINDOW_API_WIN32;
#elif defined(__APPLE__)
        *api = CLAP_WINDOW_API_COCOA;
#endif
        *is_floating = false;
        return true;
    },
    [](const clap_plugin_t* plugin, const char* api, bool is_floating) -> bool {
        auto* self = static_cast<SyrebasClap*>(plugin->plugin_data);
        self->createGuiWindow();
        return true;
    },
    [](const clap_plugin_t* plugin) {
        auto* self = static_cast<SyrebasClap*>(plugin->plugin_data);
        self->destroyGuiWindow();
    },
    [](const clap_plugin_t* plugin, double scale) -> bool {
        return false;
    },
    [](const clap_plugin_t* plugin, uint32_t* width, uint32_t* height) -> bool {
        *width = 600;
        *height = 320;
        return true;
    },
    [](const clap_plugin_t* plugin) -> bool {
        return false;
    },
    [](const clap_plugin_t* plugin, clap_gui_resize_hints_t* hints) -> bool {
        return false;
    },
    [](const clap_plugin_t* plugin, uint32_t* width, uint32_t* height) -> bool {
        *width = 600;
        *height = 320;
        return true;
    },
    [](const clap_plugin_t* plugin, uint32_t width, uint32_t height) -> bool {
        auto* self = static_cast<SyrebasClap*>(plugin->plugin_data);
        if (self->getGuiWindow()) {
            return self->getGuiWindow()->setSize(width, height);
        }
        return true;
    },
    [](const clap_plugin_t* plugin, const clap_window_t* window) -> bool {
        auto* self = static_cast<SyrebasClap*>(plugin->plugin_data);
        if (!self->getGuiWindow()) {
            self->createGuiWindow();
        }
        return self->getGuiWindow()->setParent(window);
    },
    [](const clap_plugin_t* plugin, const clap_window_t* window) -> bool {
        return false;
    },
    [](const clap_plugin_t* plugin, const char* title) {},
    [](const clap_plugin_t* plugin) -> bool {
        auto* self = static_cast<SyrebasClap*>(plugin->plugin_data);
        if (self->getGuiWindow()) {
            return self->getGuiWindow()->show();
        }
        return false;
    },
    [](const clap_plugin_t* plugin) -> bool {
        auto* self = static_cast<SyrebasClap*>(plugin->plugin_data);
        if (self->getGuiWindow()) {
            return self->getGuiWindow()->hide();
        }
        return false;
    }
};

} // namespace syrebas
