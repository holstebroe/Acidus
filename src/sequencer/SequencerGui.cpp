#include "SequencerGui.hpp"
#include "SequencerClap.hpp"
#include "gui/Graphics.hpp"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>

#if defined(__linux__) && !defined(__APPLE__)
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace acidus {
namespace seq {

// --- Layout -----------------------------------------------------------------

static constexpr int kGridX = 100;
static constexpr int kCellW = 48;
static constexpr int kTitleY = 10, kTitleH = 26;
static constexpr int kPatternY = 46, kPatternH = 30;
static constexpr int kGridY = 88;
static constexpr int kRowH[SequencerGui::kRowCount] = { 52, 36, 36, 36 };
static constexpr int kFooterY = 260;
static constexpr int kDragPixelsPerValue = 12;

// --- Colours (ARGB) -----------------------------------------------------------

// Graphite panel with acid-green notes, after the Acidus logo plate.
static constexpr uint32_t kBg = 0xFF1C1F22;
static constexpr uint32_t kPanelDark = 0xFF2C3034;
static constexpr uint32_t kGridLine = 0xFF464B50;
static constexpr uint32_t kLight = 0xFFD6DADE;        // labels and symbols
static constexpr uint32_t kInk = 0xFF0E1A08;          // text on light and green fills
static constexpr uint32_t kNoteBg = 0xFF6FE03A;       // a note
static constexpr uint32_t kTieBg = 0xFF2F6A1C;        // a tie: the note's colour, dimmed
static constexpr uint32_t kRestBg = 0xFF121416;       // a rest: darker than the panel
static constexpr uint32_t kAccentMark = 0xFFFFB020;
static constexpr uint32_t kInactiveShade = 0xB4000000;
static constexpr uint32_t kAcid = 0xFF39FF14;         // playing pattern
static constexpr uint32_t kPlayMark = 0xFFFFD21E;     // playing step
static constexpr uint32_t kPlayTint = 0x40FFD21E;
static constexpr uint32_t kDarkText = 0xFF0E1A08;

static int rowTop(int row) {
    int y = kGridY;
    for (int r = 0; r < row; ++r) y += kRowH[r];
    return y;
}

void SequencerGui::cellRect(Row row, int step, int& x, int& y, int& w, int& h) {
    const int r = static_cast<int>(row);
    x = kGridX + step * kCellW;
    y = rowTop(r);
    w = kCellW;
    h = kRowH[r];
}

void SequencerGui::patternButtonRect(int pattern, int& x, int& y, int& w, int& h) {
    x = kGridX + pattern * kCellW + 2;
    y = kPatternY;
    w = kCellW - 4;
    h = kPatternH;
}

void SequencerGui::followBoxRect(int& x, int& y, int& w, int& h) {
    x = 560; y = kTitleY; w = 76; h = kTitleH;
}

void SequencerGui::lengthBoxRect(int& x, int& y, int& w, int& h) {
    x = 700; y = kTitleY; w = 44; h = kTitleH;
}

void SequencerGui::transposeBoxRect(int& x, int& y, int& w, int& h) {
    x = kGridX + kMaxSteps * kCellW - 52; y = kTitleY; w = 52; h = kTitleH;
}

static bool inside(int px, int py, int x, int y, int w, int h) {
    return px >= x && px < x + w && py >= y && py < y + h;
}

// --- Construction -------------------------------------------------------------

SequencerGui::SequencerGui(SequencerClap* plugin) : plugin_(plugin) {
    pixels_.assign(static_cast<size_t>(kWidth) * kHeight, kBg);
    renderFrame();
}

SequencerGui::~SequencerGui() {
    destroy();
}

bool SequencerGui::show() {
    visible_ = true;
    lastSignature_.clear();
    renderFrame();
    return true;
}

bool SequencerGui::hide() {
    visible_ = false;
    return true;
}

// --- Cell values ----------------------------------------------------------------

int SequencerGui::valueCount(Row row) {
    switch (row) {
        case Row::Note: return kNoteValueCount;
        case Row::Octave: return 3;
        default: return 2;
    }
}

int SequencerGui::cellValue(Row row, int step) const {
    const Step s = plugin_->bank().step(plugin_->editPattern(), step);
    switch (row) {
        case Row::Note: return s.note;
        case Row::Octave: return s.octave + 1;
        case Row::Accent: return s.accent ? 1 : 0;
        case Row::Slide: return s.slide ? 1 : 0;
    }
    return 0;
}

void SequencerGui::setCellValue(Row row, int step, int value) {
    const int count = valueCount(row);
    value = ((value % count) + count) % count;   // values cycle
    const int pattern = plugin_->editPattern();
    Step s = plugin_->bank().step(pattern, step);
    switch (row) {
        case Row::Note: s.note = value; break;
        case Row::Octave: s.octave = value - 1; break;
        case Row::Accent: s.accent = value != 0; break;
        case Row::Slide: s.slide = value != 0; break;
    }
    plugin_->bank().setStep(pattern, step, s);
    plugin_->markStateDirty();
}

// --- Input ------------------------------------------------------------------------

void SequencerGui::mouseDown(int x, int y, bool rightButton) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    const int delta = rightButton ? -1 : 1;
    const int pattern = plugin_->editPattern();
    PatternBank& bank = plugin_->bank();
    dragTarget_ = Target::Idle;
    dragMoved_ = false;
    dragStartY_ = y;
    int bx, by, bw, bh;

    for (int p = 0; p < kNumPatterns; ++p) {
        patternButtonRect(p, bx, by, bw, bh);
        if (inside(x, y, bx, by, bw, bh)) {
            plugin_->setEditPattern(p);
            renderFrame();
            return;
        }
    }
    followBoxRect(bx, by, bw, bh);
    if (inside(x, y, bx, by, bw, bh)) {
        plugin_->setFollowPlaying(!plugin_->followPlaying());
        renderFrame();
        return;
    }
    lengthBoxRect(bx, by, bw, bh);
    if (inside(x, y, bx, by, bw, bh)) {
        bank.setLength(pattern, bank.length(pattern) + delta);
        plugin_->markStateDirty();
        if (!rightButton) { dragTarget_ = Target::Length; dragStartValue_ = bank.length(pattern); }
        renderFrame();
        return;
    }
    transposeBoxRect(bx, by, bw, bh);
    if (inside(x, y, bx, by, bw, bh)) {
        bank.setTranspose(pattern, bank.transpose(pattern) + delta);
        plugin_->markStateDirty();
        if (!rightButton) { dragTarget_ = Target::Transpose; dragStartValue_ = bank.transpose(pattern); }
        renderFrame();
        return;
    }
    for (int r = 0; r < kRowCount; ++r) {
        for (int s = 0; s < kMaxSteps; ++s) {
            cellRect(static_cast<Row>(r), s, bx, by, bw, bh);
            if (!inside(x, y, bx, by, bw, bh)) continue;
            const Row row = static_cast<Row>(r);
            if (rightButton) {
                setCellValue(row, s, cellValue(row, s) - 1);
            } else {
                // Left button: the value steps up on release, unless the
                // press turns into a drag.
                dragTarget_ = Target::Cell;
                dragRow_ = row;
                dragStep_ = s;
                dragStartValue_ = cellValue(row, s);
            }
            renderFrame();
            return;
        }
    }
}

void SequencerGui::mouseDrag(int x, int y) {
    (void)x;
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (dragTarget_ == Target::Idle) return;
    const int steps = (dragStartY_ - y) / kDragPixelsPerValue;
    if (steps != 0) dragMoved_ = true;
    if (!dragMoved_) return;
    const int pattern = plugin_->editPattern();
    PatternBank& bank = plugin_->bank();
    switch (dragTarget_) {
        case Target::Cell: setCellValue(dragRow_, dragStep_, dragStartValue_ + steps); break;
        case Target::Length: bank.setLength(pattern, dragStartValue_ + steps); plugin_->markStateDirty(); break;
        case Target::Transpose: bank.setTranspose(pattern, dragStartValue_ + steps); plugin_->markStateDirty(); break;
        default: break;
    }
    renderFrame();
}

void SequencerGui::mouseUp(int x, int y) {
    (void)x; (void)y;
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (dragTarget_ == Target::Cell && !dragMoved_) {
        setCellValue(dragRow_, dragStep_, dragStartValue_ + 1);
    }
    dragTarget_ = Target::Idle;
    renderFrame();
}

// --- Drawing ----------------------------------------------------------------------

static void fillTriangle(Graphics& g, int cx, int top, int w, int h, bool up, uint32_t color) {
    for (int r = 0; r < h; ++r) {
        const int rowFromApex = up ? r : h - 1 - r;
        const int rw = std::max(1, (rowFromApex * w) / std::max(1, h - 1));
        g.fillRect(cx - rw / 2, top + r, rw, 1, color);
    }
}

static void drawCentered(Graphics& g, const Font& f, const char* text, int x, int y, int w, int h,
                         uint32_t color, int scale) {
    const int tw = f.getTextWidth(text, scale) - scale;   // no spacing after the last glyph
    const int th = static_cast<int>(f.getHeight()) * scale;
    g.drawText(f, text, x + (w - tw) / 2, y + (h - th) / 2, color, scale);
}

static void drawBox(Graphics& g, const Font& f, int x, int y, int w, int h, const char* text,
                    uint32_t bg, uint32_t fg, int scale) {
    g.fillRect(x, y, w, h, bg);
    g.drawRect(x, y, w, h, kGridLine);
    drawCentered(g, f, text, x, y, w, h, fg, scale);
}

void SequencerGui::draw(Graphics& g) {
    const PatternBank& bank = plugin_->bank();
    const int pattern = plugin_->editPattern();
    const int length = bank.length(pattern);
    const int playingPattern = plugin_->engine().playingPattern();
    const int playingStep = plugin_->engine().playingStep();
    const int highlightStep = (playingPattern == pattern) ? playingStep : -1;
    char buf[64];

    g.clear(kBg);

    // Title row.
    g.drawText(font_, "ACIDUS SEQ", 14, kTitleY + 6, kLight, 2);
    std::snprintf(buf, sizeof(buf), "%d  %s", pattern + 1, bank.name(pattern).c_str());
    g.drawText(font_, buf, 160, kTitleY + 6, kLight, 2);
    int x, y, w, h;
    followBoxRect(x, y, w, h);
    const bool follow = plugin_->followPlaying();
    drawBox(g, font_, x, y, w, h, "FOLLOW", follow ? kLight : kPanelDark, follow ? kInk : kLight, 2);
    lengthBoxRect(x, y, w, h);
    g.drawText(font_, "LENGTH", x - 40, y + 10, kLight, 1);
    std::snprintf(buf, sizeof(buf), "%d", length);
    drawBox(g, font_, x, y, w, h, buf, kPanelDark, kLight, 2);
    transposeBoxRect(x, y, w, h);
    g.drawText(font_, "TRANSPOSE", x - 58, y + 10, kLight, 1);
    const int tr = bank.transpose(pattern);
    std::snprintf(buf, sizeof(buf), tr > 0 ? "+%d" : "%d", tr);
    drawBox(g, font_, x, y, w, h, buf, kPanelDark, kLight, 2);

    // Pattern buttons.
    g.drawText(font_, "PATTERN", 14, kPatternY + 11, kLight, 1);
    for (int p = 0; p < kNumPatterns; ++p) {
        patternButtonRect(p, x, y, w, h);
        const bool editing = p == pattern;
        const bool playing = p == playingPattern;
        uint32_t bg = editing ? kLight : kPanelDark;
        uint32_t fg = editing ? kInk : kLight;
        if (playing) { bg = kAcid; fg = kDarkText; }
        std::snprintf(buf, sizeof(buf), "%d", p + 1);
        drawBox(g, font_, x, y, w, h, buf, bg, fg, 2);
        if (playing && editing) {
            g.drawRect(x + 2, y + 2, w - 4, h - 4, kLight);
            g.drawRect(x + 3, y + 3, w - 6, h - 6, kLight);
        }
    }

    // Row labels.
    static const char* kLabels[kRowCount] = { "NOTE", "OCTAVE", "ACCENT", "SLIDE" };
    for (int r = 0; r < kRowCount; ++r) {
        g.drawText(font_, kLabels[r], 14, rowTop(r) + (kRowH[r] - 7) / 2, kLight, 1);
    }

    // Grid cells.
    for (int s = 0; s < kMaxSteps; ++s) {
        const Step st = bank.step(pattern, s);
        for (int r = 0; r < kRowCount; ++r) {
            const Row row = static_cast<Row>(r);
            cellRect(row, s, x, y, w, h);
            if (row == Row::Note) {
                if (st.isNote()) {
                    g.fillRect(x, y, w, h, kNoteBg);
                    drawCentered(g, font_, noteName(st.note), x, y, w, h, kDarkText, 3);
                } else {
                    // Ties and rests are told apart by fill alone.
                    g.fillRect(x, y, w, h, st.note == kNoteTie ? kTieBg : kRestBg);
                }
            } else if (row == Row::Octave) {
                if (st.octave != 0) {
                    fillTriangle(g, x + w / 2, y + h / 2 - 7, 18, 14, st.octave > 0, kLight);
                }
            } else if (row == Row::Accent) {
                if (st.accent) drawCentered(g, font_, "A", x, y, w, h, kAccentMark, 3);
            } else {
                if (st.slide) drawCentered(g, font_, "S", x, y, w, h, kLight, 3);
            }
        }
    }

    // Grid lines.
    const int gridBottom = rowTop(kRowCount);
    const int gridRight = kGridX + kMaxSteps * kCellW;
    for (int s = 0; s <= kMaxSteps; ++s) {
        const int lx = kGridX + s * kCellW;
        g.fillRect(lx - ((s % 4 == 0) ? 1 : 0), kGridY, (s % 4 == 0) ? 3 : 1, gridBottom - kGridY, kGridLine);
    }
    for (int r = 0; r <= kRowCount; ++r) {
        g.fillRect(kGridX, rowTop(r), gridRight - kGridX, 1, kGridLine);
    }

    // Steps past the pattern length are inactive.
    if (length < kMaxSteps) {
        const int sx = kGridX + length * kCellW + 2;
        g.fillRect(sx, kGridY, gridRight - sx + 2, gridBottom - kGridY + 1, kInactiveShade);
    }

    // The step playing now.
    if (highlightStep >= 0 && highlightStep < kMaxSteps) {
        const int sx = kGridX + highlightStep * kCellW;
        g.fillRect(sx, kGridY, kCellW, gridBottom - kGridY, kPlayTint);
        for (int t = 0; t < 3; ++t) {
            g.drawRect(sx - 1 + t, kGridY - 1 + t, kCellW + 3 - 2 * t, gridBottom - kGridY + 3 - 2 * t, kPlayMark);
        }
    }

    // Footer.
    std::snprintf(buf, sizeof(buf), "TRIGGER KEY %s  (PATTERNS 1-16 = C-1 TO D#0)",
                  keyName(kFirstTriggerKey + pattern).c_str());
    g.drawText(font_, buf, 14, kFooterY, kLight, 1);
    g.drawText(font_, "CLICK +  RIGHT-CLICK -  DRAG UP/DOWN", gridRight - 211, kFooterY, kLight, 1);
}

std::string SequencerGui::signature() {
    const PatternBank& bank = plugin_->bank();
    const int pattern = plugin_->editPattern();
    std::string s;
    s += static_cast<char>(pattern);
    s += static_cast<char>(plugin_->followPlaying());
    s += static_cast<char>(bank.length(pattern));
    s += static_cast<char>(bank.transpose(pattern));
    s += static_cast<char>(plugin_->engine().playingPattern());
    s += static_cast<char>(plugin_->engine().playingStep());
    for (int i = 0; i < kMaxSteps; ++i) s += static_cast<char>(bank.step(pattern, i).pack());
    s += bank.name(pattern);
    return s;
}

void SequencerGui::followPlayingPattern() {
    const int playing = plugin_->engine().playingPattern();
    if (playing != lastPlayingPattern_) {
        lastPlayingPattern_ = playing;
        if (playing >= 0 && plugin_->followPlaying() && dragTarget_ == Target::Idle) {
            plugin_->setEditPattern(playing);
        }
    }
}

void SequencerGui::renderFrame() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    followPlayingPattern();
    std::string sig = signature();
    if (sig == lastSignature_) return;
    lastSignature_ = std::move(sig);
    Graphics g(pixels_.data(), kWidth, kHeight);
    draw(g);
    present();
}

// --- Platform windows -----------------------------------------------------------

#if defined(__linux__) && !defined(__APPLE__)

bool SequencerGui::setParent(const clap_window_t* window) {
    if (!window || !window->api || std::strcmp(window->api, CLAP_WINDOW_API_X11) != 0) return false;
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (display_) return true;
    Display* display = XOpenDisplay(nullptr);
    if (!display) return false;
    display_ = display;
    const int screen = DefaultScreen(display);
    const Window parent = window->x11 ? static_cast<Window>(window->x11) : RootWindow(display, screen);
    window_ = XCreateSimpleWindow(display, parent, 0, 0, kWidth, kHeight, 0,
                                  BlackPixel(display, screen), WhitePixel(display, screen));
    XSelectInput(display, window_, ExposureMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask);
    XMapWindow(display, window_);
    XFlush(display);
    lastSignature_.clear();
    renderFrame();
    running_ = true;
    eventThread_ = std::thread(&SequencerGui::eventLoopX11, this);
    return true;
}

void SequencerGui::eventLoopX11() {
  try {
    Display* display = static_cast<Display*>(display_);
    while (running_) {
        while (XPending(display) > 0) {
            XEvent ev;
            XNextEvent(display, &ev);
            if (ev.type == Expose) {
                std::lock_guard<std::recursive_mutex> lock(mutex_);
                present();
            } else if (ev.type == ButtonPress && (ev.xbutton.button == Button1 || ev.xbutton.button == Button3)) {
                mouseDown(ev.xbutton.x, ev.xbutton.y, ev.xbutton.button == Button3);
            } else if (ev.type == MotionNotify && (ev.xmotion.state & Button1Mask)) {
                mouseDrag(ev.xmotion.x, ev.xmotion.y);
            } else if (ev.type == ButtonRelease && ev.xbutton.button == Button1) {
                mouseUp(ev.xbutton.x, ev.xbutton.y);
            }
        }
        if (visible_) renderFrame();   // follows the playing step
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
  } catch (...) {
    // An exception escaping a thread function would std::terminate the host.
  }
}

void SequencerGui::present() {
    if (!display_) return;
    Display* display = static_cast<Display*>(display_);
    const int screen = DefaultScreen(display);
    XImage* image = XCreateImage(display, DefaultVisual(display, screen), 24, ZPixmap, 0,
                                 reinterpret_cast<char*>(pixels_.data()), kWidth, kHeight, 32, 0);
    if (!image) return;
    XPutImage(display, window_, DefaultGC(display, screen), image, 0, 0, 0, 0, kWidth, kHeight);
    image->data = nullptr;
    XDestroyImage(image);
    XFlush(display);
}

void SequencerGui::destroy() {
    running_ = false;
    if (eventThread_.joinable()) eventThread_.join();
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (display_) {
        Display* display = static_cast<Display*>(display_);
        XDestroyWindow(display, window_);
        XCloseDisplay(display);
        display_ = nullptr;
    }
}

#elif defined(_WIN32)

static const wchar_t* kClassName = L"AcidusSeqWindowClass";

static LRESULT CALLBACK seqWndProcImpl(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    auto* gui = reinterpret_cast<SequencerGui*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    const int x = static_cast<short>(LOWORD(lParam));
    const int y = static_cast<short>(HIWORD(lParam));
    switch (msg) {
        case WM_CREATE: {
            auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
            SetTimer(hwnd, 1, 16, NULL);
            return 0;
        }
        case WM_TIMER:
            if (gui) gui->renderFrame();
            return 0;
        case WM_PAINT: {
            PAINTSTRUCT ps;
            BeginPaint(hwnd, &ps);
            if (gui) gui->paintWin32();
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
            if (gui) {
                SetCapture(hwnd);
                gui->mouseDown(x, y, msg == WM_RBUTTONDOWN);
            }
            return 0;
        case WM_MOUSEMOVE:
            if (gui && (wParam & MK_LBUTTON)) gui->mouseDrag(x, y);
            return 0;
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
            ReleaseCapture();
            if (gui && msg == WM_LBUTTONUP) gui->mouseUp(x, y);
            return 0;
        case WM_CONTEXTMENU:
            return 0;   // right-click is ours
        case WM_DESTROY:
            KillTimer(hwnd, 1);
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

static LRESULT CALLBACK seqWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    try {
        return seqWndProcImpl(hwnd, msg, wParam, lParam);
    } catch (...) {
        return DefWindowProcW(hwnd, msg, wParam, lParam);   // never unwind through the OS
    }
}

bool SequencerGui::setParent(const clap_window_t* window) {
    if (!window || !window->api || std::strcmp(window->api, CLAP_WINDOW_API_WIN32) != 0) return false;
    if (hwnd_) return true;
    HINSTANCE instance = GetModuleHandleW(NULL);
    static bool registered = false;
    if (!registered) {
        WNDCLASSW wc = {};
        wc.lpfnWndProc = seqWndProc;
        wc.hInstance = instance;
        wc.lpszClassName = kClassName;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        RegisterClassW(&wc);
        registered = true;
    }
    hwnd_ = CreateWindowExW(0, kClassName, L"Acidus Seq", WS_CHILD | WS_VISIBLE, 0, 0, kWidth, kHeight,
                            static_cast<HWND>(window->win32), NULL, instance, this);
    lastSignature_.clear();
    renderFrame();
    return hwnd_ != nullptr;
}

void SequencerGui::paintWin32() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    present();
}

void SequencerGui::present() {
    if (!hwnd_) return;
    HDC hdc = GetDC(static_cast<HWND>(hwnd_));
    if (!hdc) return;
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = kWidth;
    bmi.bmiHeader.biHeight = -kHeight;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    SetDIBitsToDevice(hdc, 0, 0, kWidth, kHeight, 0, 0, 0, kHeight, pixels_.data(), &bmi, DIB_RGB_COLORS);
    ReleaseDC(static_cast<HWND>(hwnd_), hdc);
}

void SequencerGui::destroy() {
    if (hwnd_) {
        DestroyWindow(static_cast<HWND>(hwnd_));
        hwnd_ = nullptr;
    }
}

#else   // macOS: no native view yet (same as the Acidus GUI)

bool SequencerGui::setParent(const clap_window_t*) { return false; }
void SequencerGui::present() {}
void SequencerGui::destroy() {}

#endif

// --- CLAP gui extension ---------------------------------------------------------

#if defined(__linux__) && !defined(__APPLE__)
static const char* const kWindowApi = CLAP_WINDOW_API_X11;
#elif defined(_WIN32)
static const char* const kWindowApi = CLAP_WINDOW_API_WIN32;
#else
static const char* const kWindowApi = nullptr;
#endif

static SequencerClap* pluginOf(const clap_plugin_t* p) {
    return static_cast<SequencerClap*>(p->plugin_data);
}

const clap_plugin_gui_t g_sequencerGuiExtension = {
    // is_api_supported
    [](const clap_plugin_t*, const char* api, bool isFloating) -> bool {
        return kWindowApi && api && std::strcmp(api, kWindowApi) == 0 && !isFloating;
    },
    // get_preferred_api
    [](const clap_plugin_t*, const char** api, bool* isFloating) -> bool {
        if (!api || !isFloating || !kWindowApi) return false;
        *api = kWindowApi;
        *isFloating = false;
        return true;
    },
    // create
    [](const clap_plugin_t* p, const char*, bool) -> bool {
        try { pluginOf(p)->createGui(); } catch (...) { return false; }
        return true;
    },
    // destroy
    [](const clap_plugin_t* p) {
        try { pluginOf(p)->destroyGui(); } catch (...) {}
    },
    // set_scale
    [](const clap_plugin_t*, double) -> bool { return false; },
    // get_size
    [](const clap_plugin_t*, uint32_t* w, uint32_t* h) -> bool {
        if (!w || !h) return false;
        *w = SequencerGui::kWidth;
        *h = SequencerGui::kHeight;
        return true;
    },
    // can_resize
    [](const clap_plugin_t*) -> bool { return false; },
    // get_resize_hints
    [](const clap_plugin_t*, clap_gui_resize_hints_t*) -> bool { return false; },
    // adjust_size
    [](const clap_plugin_t*, uint32_t* w, uint32_t* h) -> bool {
        if (!w || !h) return false;
        *w = SequencerGui::kWidth;
        *h = SequencerGui::kHeight;
        return true;
    },
    // set_size
    [](const clap_plugin_t*, uint32_t w, uint32_t h) -> bool {
        return w == static_cast<uint32_t>(SequencerGui::kWidth) && h == static_cast<uint32_t>(SequencerGui::kHeight);
    },
    // set_parent
    [](const clap_plugin_t* p, const clap_window_t* window) -> bool {
        try {
            pluginOf(p)->createGui();
            return pluginOf(p)->gui()->setParent(window);
        } catch (...) { return false; }
    },
    // set_transient
    [](const clap_plugin_t*, const clap_window_t*) -> bool { return false; },
    // suggest_title
    [](const clap_plugin_t*, const char*) {},
    // show
    [](const clap_plugin_t* p) -> bool {
        try { return pluginOf(p)->gui() && pluginOf(p)->gui()->show(); } catch (...) { return false; }
    },
    // hide
    [](const clap_plugin_t* p) -> bool {
        try { return pluginOf(p)->gui() && pluginOf(p)->gui()->hide(); } catch (...) { return false; }
    }
};

} // namespace seq
} // namespace acidus
