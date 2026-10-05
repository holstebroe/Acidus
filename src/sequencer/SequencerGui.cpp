#include "SequencerGui.hpp"
#include "SequencerClap.hpp"
#include "SequencerLayout.hpp"
#include "gui/FileDialog.hpp"
#include "gui/Graphics.hpp"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>

#if defined(__linux__) && !defined(__APPLE__)
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <cctype>
#include <csignal>
#include <fcntl.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
extern char** environ;
#endif

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <commdlg.h>
#include <ole2.h>
#include <shlobj.h>
#endif

namespace acidus {
namespace seq {

// --- Layout -----------------------------------------------------------------

static constexpr int kDragPixelsPerValue = 12;
static constexpr int kMidiDragPixels = 4;    // movement that turns a press on MIDI into a drag

using namespace layout;

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

void SequencerGui::buttonRect(Button button, int& x, int& y, int& w, int& h) {
    y = kTitleY; h = kTitleH;
    switch (button) {
        case Button::Play:   x = 104; w = 34; break;
        case Button::Name:   x = 178; w = 290; break;
        case Button::Init:   x = 476; w = 66; break;
        case Button::Load:   x = 588; w = 40; break;
        case Button::Save:   x = 632; w = 40; break;
        case Button::Midi:   x = 684; w = 72; break;
        case Button::Follow: x = 10; y = kPatternY + 15; w = 70; h = 15; break;   // under the PATTERN label
    }
}

void SequencerGui::boxRect(Box box, int& x, int& y, int& w, int& h) {
    y = kSetupY; h = kSetupH;
    switch (box) {
        case Box::Length:    x = 148; w = 44; break;
        case Box::Transpose: x = 268; w = 52; break;
        case Box::Next:      x = 366; w = 44; break;
        case Box::Key:       x = kGridX + kMaxSteps * kCellW - 52; y = kTitleY; w = 52; h = kTitleH; break;
    }
}

static bool inside(int px, int py, int x, int y, int w, int h) {
    return px >= x && px < x + w && py >= y && py < y + h;
}

static bool insideButton(int px, int py, SequencerGui::Button b) {
    int x, y, w, h;
    SequencerGui::buttonRect(b, x, y, w, h);
    return inside(px, py, x, y, w, h);
}

// --- Construction -------------------------------------------------------------

SequencerGui::SequencerGui(SequencerClap* plugin) : plugin_(plugin), skin_(createSequencerSkin()) {
    pixels_.assign(static_cast<size_t>(kWidth) * kHeight, 0xFF1C1F22);
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

int SequencerGui::boxValue(Box box) const {
    const int pattern = plugin_->editPattern();
    switch (box) {
        case Box::Length: return plugin_->bank().length(pattern);
        case Box::Transpose: return plugin_->bank().transpose(pattern);
        case Box::Next: return plugin_->bank().next(pattern);
        case Box::Key: return plugin_->globalTranspose();
    }
    return 0;
}

// Box values clamp (they don't cycle like grid cells).
void SequencerGui::setBoxValue(Box box, int value) {
    const int pattern = plugin_->editPattern();
    PatternBank& bank = plugin_->bank();
    switch (box) {
        case Box::Length: bank.setLength(pattern, value); break;
        case Box::Transpose: bank.setTranspose(pattern, value); break;
        case Box::Next: bank.setNext(pattern, std::min(std::max(value, -1), kNumPatterns - 1)); break;
        case Box::Key: plugin_->setTransposeFromGui(value); return;   // marks dirty itself
    }
    plugin_->markStateDirty();
}

// Something is playing: the play button shows pause.
bool SequencerGui::playing() const {
    return plugin_->previewPattern() >= 0 || plugin_->engine().playingPattern() >= 0;
}

// --- Input ------------------------------------------------------------------------

void SequencerGui::mouseDown(int x, int y, bool rightButton) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    dragTarget_ = Target::Idle;
    dragMoved_ = false;
    dragStartX_ = x;
    dragStartY_ = y;
    int bx, by, bw, bh;

    // A click anywhere else ends name editing (keeping the name) and disarms INIT.
    if (editingName_ && !insideButton(x, y, Button::Name)) {
        commitName();
        actions_ |= kActionReleaseFocus;
    }
    const bool armed = initArmed_;
    initArmed_ = false;

    if (insideButton(x, y, Button::Play)) {
        // Pause lets go of the play button's trigger; it cannot stop the host's.
        plugin_->setPreviewPattern(playing() ? -1 : plugin_->editPattern());
    } else if (insideButton(x, y, Button::Name)) {
        if (!editingName_) {
            editingName_ = true;
            nameDraft_ = plugin_->bank().name(plugin_->editPattern());
            actions_ |= kActionTakeFocus;
            status_ = "TYPE A NAME:  ENTER KEEPS IT, ESC CANCELS";
        }
    } else if (insideButton(x, y, Button::Init)) {
        if (armed) {
            plugin_->bank().clearPattern(plugin_->editPattern());
            plugin_->markStateDirty();
            status_ = "PATTERN " + std::to_string(plugin_->editPattern() + 1) + " CLEARED";
        } else {
            initArmed_ = true;
        }
    } else if (insideButton(x, y, Button::Load)) {
        actions_ |= kActionLoadBank;
    } else if (insideButton(x, y, Button::Save)) {
        actions_ |= kActionSaveBank;
    } else if (insideButton(x, y, Button::Midi)) {
        dragTarget_ = Target::Midi;   // a drag, not a click
    } else if (insideButton(x, y, Button::Follow)) {
        plugin_->setFollowPlaying(!plugin_->followPlaying());
    } else {
        for (int p = 0; p < kNumPatterns; ++p) {
            patternButtonRect(p, bx, by, bw, bh);
            if (inside(x, y, bx, by, bw, bh)) {
                plugin_->setEditPattern(p);
                // The play button plays the pattern being edited.
                if (plugin_->previewPattern() >= 0) plugin_->setPreviewPattern(p);
                renderFrame();
                return;
            }
        }
        for (int b = 0; b < kBoxCount; ++b) {
            const Box box = static_cast<Box>(b);
            boxRect(box, bx, by, bw, bh);
            if (!inside(x, y, bx, by, bw, bh)) continue;
            if (box == Box::Key) plugin_->beginTransposeEdit();
            if (rightButton) {
                // No release follows a right-click: it is a whole edit.
                setBoxValue(box, boxValue(box) - 1);
                if (box == Box::Key) plugin_->endTransposeEdit();
            } else {
                // Left button, as for grid cells: +1 on release unless it
                // turns into a drag. The Key gesture ends on release.
                dragTarget_ = Target::Box;
                dragBox_ = box;
                dragStartValue_ = boxValue(box);
            }
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
    renderFrame();
}

void SequencerGui::mouseDrag(int x, int y) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (dragTarget_ == Target::Idle) return;
    if (dragTarget_ == Target::Midi) {
        if (!dragMoved_ && std::abs(x - dragStartX_) + std::abs(y - dragStartY_) >= kMidiDragPixels) {
            dragMoved_ = true;
            actions_ |= kActionDragMidi;
        }
        return;
    }
    const int steps = (dragStartY_ - y) / kDragPixelsPerValue;
    if (steps != 0) dragMoved_ = true;
    if (!dragMoved_) return;
    if (dragTarget_ == Target::Cell) setCellValue(dragRow_, dragStep_, dragStartValue_ + steps);
    else setBoxValue(dragBox_, dragStartValue_ + steps);
    renderFrame();
}

void SequencerGui::mouseUp(int x, int y) {
    (void)x; (void)y;
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (dragTarget_ == Target::Cell && !dragMoved_) {
        setCellValue(dragRow_, dragStep_, dragStartValue_ + 1);
    } else if (dragTarget_ == Target::Box && !dragMoved_) {
        setBoxValue(dragBox_, dragStartValue_ + 1);
    } else if (dragTarget_ == Target::Midi && !dragMoved_) {
        status_ = "DRAG THE MIDI BUTTON ONTO A DAW TRACK";
    }
    if (dragTarget_ == Target::Box && dragBox_ == Box::Key) plugin_->endTransposeEdit();
    dragTarget_ = Target::Idle;
    renderFrame();
}

void SequencerGui::commitName() {
    if (!editingName_) return;
    editingName_ = false;
    const int pattern = plugin_->editPattern();
    if (nameDraft_ != plugin_->bank().name(pattern)) {
        plugin_->bank().setName(pattern, nameDraft_);
        plugin_->markStateDirty();
    }
    status_.clear();
}

void SequencerGui::keyText(const char* text) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!editingName_ || !text) return;
    for (const char* p = text; *p; ++p) {
        char c = *p;
        if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');   // the panel font is upper case
        if (c < 32 || c > 95) continue;
        if (static_cast<int>(nameDraft_.size()) < kMaxNameLength) nameDraft_ += c;
    }
    renderFrame();
}

void SequencerGui::keyPress(Key key) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!editingName_) return;
    switch (key) {
        case Key::Enter:
            commitName();
            actions_ |= kActionReleaseFocus;
            break;
        case Key::Escape:
            editingName_ = false;
            status_.clear();
            actions_ |= kActionReleaseFocus;
            break;
        case Key::Backspace:
            if (!nameDraft_.empty()) nameDraft_.pop_back();
            break;
    }
    renderFrame();
}

void SequencerGui::focusLost() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!editingName_) return;
    commitName();
    renderFrame();
}

uint32_t SequencerGui::takeActions() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    const uint32_t a = actions_;
    actions_ = 0;
    return a;
}

void SequencerGui::setStatus(const std::string& text) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    status_ = text;
    renderFrame();
}

static std::string upperFileName(const std::string& utf8Path) {
    const size_t slash = utf8Path.find_last_of("/\\");
    std::string name = slash == std::string::npos ? utf8Path : utf8Path.substr(slash + 1);
    for (char& c : name) {
        if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
        else if (static_cast<unsigned char>(c) > 95) c = '?';
    }
    return name;
}

void SequencerGui::loadBankFrom(const std::string& utf8Path) {
    const bool ok = plugin_->loadBankFile(std::filesystem::u8path(utf8Path));
    setStatus(ok ? "LOADED " + upperFileName(utf8Path)
                 : "NOT LOADED: " + upperFileName(utf8Path) + " IS NOT A BURETTE BANK");
}

void SequencerGui::saveBankTo(const std::string& utf8Path) {
    std::string path = utf8Path;
    const std::string ext = ".burette";
    if (path.size() < ext.size() || path.compare(path.size() - ext.size(), ext.size(), ext) != 0) path += ext;
    const bool ok = plugin_->saveBankFile(std::filesystem::u8path(path));
    setStatus(ok ? "SAVED " + upperFileName(path) : "COULD NOT SAVE " + upperFileName(path));
}

std::string SequencerGui::signature() {
    const PatternBank& bank = plugin_->bank();
    const int pattern = plugin_->editPattern();
    std::string s;
    s += static_cast<char>(pattern);
    s += static_cast<char>(plugin_->followPlaying());
    s += static_cast<char>(plugin_->globalTranspose());
    s += static_cast<char>(playing());
    s += static_cast<char>(initArmed_);
    s += static_cast<char>(editingName_);
    for (int p = 0; p < kNumPatterns; ++p) s += static_cast<char>(bank.next(p));   // the chain shown
    s += static_cast<char>(bank.length(pattern));
    s += static_cast<char>(bank.transpose(pattern));
    s += static_cast<char>(plugin_->engine().playingPattern());
    s += static_cast<char>(plugin_->engine().playingStep());
    for (int i = 0; i < kMaxSteps; ++i) s += static_cast<char>(bank.step(pattern, i).pack());
    s += bank.name(pattern);
    s += '\n';
    s += nameDraft_;
    s += '\n';
    s += status_;
    return s;
}

void SequencerGui::followPlayingPattern() {
    const int playingNow = plugin_->engine().playingPattern();
    if (playingNow != lastPlayingPattern_) {
        lastPlayingPattern_ = playingNow;
        if (playingNow >= 0 && plugin_->followPlaying() && dragTarget_ == Target::Idle && !editingName_) {
            plugin_->setEditPattern(playingNow);
        }
    }
}

void SequencerGui::renderFrame() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    followPlayingPattern();
    std::string sig = signature();
    if (sig == lastSignature_) return;
    lastSignature_ = std::move(sig);
    const SequencerView view{ *plugin_, playing(), initArmed_, editingName_, nameDraft_, status_ };
    const int ss = skin_->supersample();
    if (ss <= 1) {
        Graphics g(pixels_.data(), kWidth, kHeight);
        skin_->draw(g, view);
    } else {
        // Paint at ss x the size and box-filter down (the modern skin).
        hiRes_.resize(static_cast<size_t>(kWidth) * kHeight * ss * ss);
        Graphics g(hiRes_.data(), kWidth, kHeight, ss);
        skin_->draw(g, view);
        const int hw = kWidth * ss, n = ss * ss;
        for (int y = 0; y < kHeight; ++y) {
            for (int x = 0; x < kWidth; ++x) {
                // Sum the channels of ss x ss pixels in parallel lanes
                // (red+blue, and green), at most 64 pixels per sum.
                uint32_t rb = 0, gr = 0;
                for (int j = 0; j < ss; ++j) {
                    const uint32_t* row = &hiRes_[static_cast<size_t>(y * ss + j) * hw + x * ss];
                    for (int i = 0; i < ss; ++i) {
                        rb += row[i] & 0x00FF00FFu;
                        gr += (row[i] >> 8) & 0xFFu;
                    }
                }
                const uint32_t r = (rb >> 16) / n, b = (rb & 0xFFFFu) / n;
                pixels_[static_cast<size_t>(y) * kWidth + x] = 0xFF000000u | (r << 16) | ((gr / n) << 8) | b;
            }
        }
    }
    present();
}

// --- Platform windows -----------------------------------------------------------

#if defined(__linux__) && !defined(__APPLE__)

// --- Linux: drag-and-drop source (XDND) ----------------------------------------
//
// Drags a file to another application by the XDND protocol
// (freedesktop.org XDND, version 5): the file is offered as a text/uri-list
// on the XdndSelection, which the drop target fetches after XdndDrop.

class XdndSource {
public:
    void init(Display* d, Window w) {
        display_ = d;
        window_ = w;
        const char* names[] = { "XdndAware", "XdndEnter", "XdndPosition", "XdndStatus", "XdndLeave",
                                "XdndDrop", "XdndFinished", "XdndSelection", "XdndActionCopy",
                                "text/uri-list", "TARGETS" };
        XInternAtoms(d, const_cast<char**>(names), 11, False, atoms_);
    }
    bool dragging() const { return dragging_; }

    void begin(const std::string& uriList, int rootX, int rootY, Time time) {
        uriList_ = uriList;
        dragging_ = true;
        target_ = None;
        accepted_ = false;
        statusPending_ = false;
        XSetSelectionOwner(display_, atoms_[kSelection], window_, time);
        XGrabPointer(display_, window_, False, ButtonReleaseMask | PointerMotionMask, GrabModeAsync,
                     GrabModeAsync, None, None, time);
        motion(rootX, rootY, time);
    }

    void motion(int rootX, int rootY, Time time) {
        if (!dragging_) return;
        int version = 0;
        const Window target = targetAt(rootX, rootY, version);
        if (target != target_) {
            if (target_ != None) send(target_, kLeave, 0, 0, 0, 0);
            target_ = target;
            accepted_ = false;
            statusPending_ = false;
            if (target_ != None) {
                version_ = std::min(version, 5);
                send(target_, kEnter, static_cast<long>(version_) << 24, static_cast<long>(atoms_[kUriList]), 0, 0);
            }
        }
        if (target_ == None) return;
        lastX_ = rootX; lastY_ = rootY; lastTime_ = time;
        if (statusPending_) { positionPending_ = true; return; }   // one position per status
        sendPosition();
    }

    void release(Time time) {
        if (!dragging_) return;
        dragging_ = false;
        XUngrabPointer(display_, time);
        if (target_ == None) return;
        if (accepted_) send(target_, kDrop, 0, version_ >= 1 ? static_cast<long>(time) : 0, 0, 0);
        else send(target_, kLeave, 0, 0, 0, 0);
        target_ = None;
    }

    void cancel() {
        if (dragging_) release(CurrentTime);
    }

    // XdndStatus and the drop target's request for the data.
    bool handleEvent(const XEvent& ev) {
        if (ev.type == ClientMessage && ev.xclient.message_type == atoms_[kStatus]) {
            if (static_cast<Window>(ev.xclient.data.l[0]) == target_) {
                accepted_ = (ev.xclient.data.l[1] & 1) != 0;
                statusPending_ = false;
                if (positionPending_) { positionPending_ = false; sendPosition(); }
            }
            return true;
        }
        if (ev.type == ClientMessage && ev.xclient.message_type == atoms_[kFinished]) return true;
        if (ev.type == SelectionRequest && ev.xselectionrequest.selection == atoms_[kSelection]) {
            const XSelectionRequestEvent& req = ev.xselectionrequest;
            XEvent reply{};
            reply.xselection.type = SelectionNotify;
            reply.xselection.display = req.display;
            reply.xselection.requestor = req.requestor;
            reply.xselection.selection = req.selection;
            reply.xselection.target = req.target;
            reply.xselection.time = req.time;
            reply.xselection.property = None;
            const Atom property = req.property != None ? req.property : req.target;
            if (req.target == atoms_[kUriList]) {
                XChangeProperty(display_, req.requestor, property, req.target, 8, PropModeReplace,
                                reinterpret_cast<const unsigned char*>(uriList_.data()),
                                static_cast<int>(uriList_.size()));
                reply.xselection.property = property;
            } else if (req.target == atoms_[kTargets]) {
                const Atom targets[2] = { atoms_[kTargets], atoms_[kUriList] };
                XChangeProperty(display_, req.requestor, property, XA_ATOM, 32, PropModeReplace,
                                reinterpret_cast<const unsigned char*>(targets), 2);
                reply.xselection.property = property;
            }
            XSendEvent(display_, req.requestor, False, NoEventMask, &reply);
            XFlush(display_);
            return true;
        }
        return false;
    }

private:
    enum { kAware, kEnter, kPosition, kStatus, kLeave, kDrop, kFinished, kSelection, kActionCopy, kUriList, kTargets };
    Display* display_{nullptr};
    Window window_{0};
    Atom atoms_[11]{};
    std::string uriList_;
    bool dragging_{false};
    Window target_{None};
    int version_{5};
    bool accepted_{false};
    bool statusPending_{false};
    bool positionPending_{false};
    int lastX_{0}, lastY_{0};
    Time lastTime_{CurrentTime};

    void send(Window to, int type, long l1, long l2, long l3, long l4) {
        XEvent ev{};
        ev.xclient.type = ClientMessage;
        ev.xclient.display = display_;
        ev.xclient.window = to;
        ev.xclient.message_type = atoms_[type];
        ev.xclient.format = 32;
        ev.xclient.data.l[0] = static_cast<long>(window_);
        ev.xclient.data.l[1] = l1;
        ev.xclient.data.l[2] = l2;
        ev.xclient.data.l[3] = l3;
        ev.xclient.data.l[4] = l4;
        XSendEvent(display_, to, False, NoEventMask, &ev);
        XFlush(display_);
    }

    void sendPosition() {
        send(target_, kPosition, 0, (static_cast<long>(lastX_) << 16) | (lastY_ & 0xFFFF),
             static_cast<long>(lastTime_), static_cast<long>(atoms_[kActionCopy]));
        statusPending_ = true;
    }

    // The XDND-aware window under the root position, or None.
    Window targetAt(int rootX, int rootY, int& version) {
        const Window root = DefaultRootWindow(display_);
        Window w = root;
        for (int depth = 0; depth < 32; ++depth) {
            if (w != root && w != window_) {
                Atom type; int format; unsigned long count, after; unsigned char* data = nullptr;
                if (XGetWindowProperty(display_, w, atoms_[kAware], 0, 1, False, XA_ATOM, &type, &format,
                                       &count, &after, &data) == Success && data) {
                    const bool aware = type == XA_ATOM && count == 1;
                    if (aware) version = static_cast<int>(*reinterpret_cast<Atom*>(data));
                    XFree(data);
                    if (aware && version >= 3) return w;
                } else if (data) {
                    XFree(data);
                }
            }
            int x, y;
            Window child = None;
            if (!XTranslateCoordinates(display_, root, w, rootX, rootY, &x, &y, &child) || child == None) break;
            w = child;
        }
        return None;
    }
};

struct SequencerGui::X11Extras {
    FileDialogProcess dialog;
    XdndSource drag;
    std::string lastDir;
};

// A file:// URI (RFC 8089) for a local path.
static std::string fileUri(const std::string& path) {
    static const char* hex = "0123456789ABCDEF";
    std::string uri = "file://";
    for (unsigned char c : path) {
        if (std::isalnum(c) || std::strchr("/-_.~", c)) uri += static_cast<char>(c);
        else { uri += '%'; uri += hex[c >> 4]; uri += hex[c & 15]; }
    }
    return uri;
}

bool SequencerGui::setParent(const clap_window_t* window) {
    if (!window || !window->api || std::strcmp(window->api, CLAP_WINDOW_API_X11) != 0) return false;
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (display_) return true;
    Display* display = XOpenDisplay(nullptr);
    if (!display) return false;
    display_ = display;
    const int screen = DefaultScreen(display);
    const Window parent = window->x11 ? static_cast<Window>(window->x11) : RootWindow(display, screen);
    parent_ = parent;
    window_ = XCreateSimpleWindow(display, parent, 0, 0, kWidth, kHeight, 0,
                                  BlackPixel(display, screen), WhitePixel(display, screen));
    XSelectInput(display, window_, ExposureMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask
                                   | KeyPressMask | FocusChangeMask);
    XMapWindow(display, window_);
    XFlush(display);
    x11_ = std::make_unique<X11Extras>();
    x11_->drag.init(display, window_);
    lastSignature_.clear();
    renderFrame();
    running_ = true;
    eventThread_ = std::thread(&SequencerGui::eventLoopX11, this);
    return true;
}

void SequencerGui::performActionsX11(uint32_t actions, int rootX, int rootY, unsigned long time) {
    Display* display = static_cast<Display*>(display_);
    if (actions & kActionTakeFocus) XSetInputFocus(display, window_, RevertToParent, CurrentTime);
    if (actions & kActionReleaseFocus) {
        Window focus; int revert;
        XGetInputFocus(display, &focus, &revert);
        if (focus == window_) XSetInputFocus(display, parent_, RevertToParent, CurrentTime);
    }
    if (actions & (kActionLoadBank | kActionSaveBank)) {
        const bool save = (actions & kActionSaveBank) != 0;
        if (x11_->lastDir.empty()) {
            const char* home = std::getenv("HOME");
            x11_->lastDir = home ? home : "/";
        }
        std::string start = x11_->lastDir + "/";
        if (save) start += "BURETTE BANK.burette";
        FileDialogOptions options;
        options.save = save;
        options.title = save ? "Save pattern bank" : "Load pattern bank";
        options.filterName = "Burette banks";
        options.extension = "burette";
        options.startPath = start;
        if (!x11_->dialog.start(options)) {
            setStatus("NO FILE DIALOG: INSTALL ZENITY OR KDIALOG");
        }
    }
    if (actions & kActionDragMidi) {
        const std::filesystem::path file = plugin_->writePatternMidi(plugin_->editPattern());
        if (file.empty()) setStatus("COULD NOT WRITE THE MIDI FILE");
        else x11_->drag.begin(fileUri(file.string()) + "\r\n", rootX, rootY, static_cast<Time>(time));
    }
    XFlush(display);
}

void SequencerGui::eventLoopX11() {
  try {
    Display* display = static_cast<Display*>(display_);
    while (running_) {
        while (XPending(display) > 0) {
            XEvent ev;
            XNextEvent(display, &ev);
            if (x11_->drag.handleEvent(ev)) continue;
            int rootX = 0, rootY = 0;
            Time time = CurrentTime;
            if (ev.type == Expose) {
                std::lock_guard<std::recursive_mutex> lock(mutex_);
                present();
            } else if (ev.type == ButtonPress && (ev.xbutton.button == Button1 || ev.xbutton.button == Button3)) {
                mouseDown(ev.xbutton.x, ev.xbutton.y, ev.xbutton.button == Button3);
                rootX = ev.xbutton.x_root; rootY = ev.xbutton.y_root; time = ev.xbutton.time;
            } else if (ev.type == MotionNotify && (ev.xmotion.state & Button1Mask)) {
                if (x11_->drag.dragging()) {
                    x11_->drag.motion(ev.xmotion.x_root, ev.xmotion.y_root, ev.xmotion.time);
                } else {
                    mouseDrag(ev.xmotion.x, ev.xmotion.y);
                }
                rootX = ev.xmotion.x_root; rootY = ev.xmotion.y_root; time = ev.xmotion.time;
            } else if (ev.type == ButtonRelease && ev.xbutton.button == Button1) {
                x11_->drag.release(ev.xbutton.time);
                mouseUp(ev.xbutton.x, ev.xbutton.y);
            } else if (ev.type == KeyPress) {
                char text[16] = {};
                KeySym sym = NoSymbol;
                const int n = XLookupString(&ev.xkey, text, sizeof(text) - 1, &sym, nullptr);
                if (sym == XK_Return || sym == XK_KP_Enter) keyPress(Key::Enter);
                else if (sym == XK_Escape) keyPress(Key::Escape);
                else if (sym == XK_BackSpace) keyPress(Key::Backspace);
                else if (n > 0) { text[n] = 0; keyText(text); }
            } else if (ev.type == FocusOut && ev.xfocus.mode == NotifyNormal
                       && ev.xfocus.detail != NotifyPointer && ev.xfocus.detail != NotifyInferior) {
                // Not the pointer-window notices taking the focus itself
                // causes, nor a grab's: the keyboard really went elsewhere.
                focusLost();
            }
            const uint32_t actions = takeActions();
            if (actions) performActionsX11(actions, rootX, rootY, time);
        }
        std::string path;
        bool save = false;
        if (x11_->dialog.poll(path, save) && !path.empty()) {
            x11_->lastDir = std::filesystem::path(path).parent_path().string();
            if (save) saveBankTo(path);
            else loadBankFrom(path);
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
        x11_->drag.cancel();
        x11_->dialog.cancel();
        x11_.reset();
        XDestroyWindow(display, window_);
        XCloseDisplay(display);
        display_ = nullptr;
    }
}

#elif defined(_WIN32)

static const wchar_t* kClassName = L"BuretteWindowClass";

// The drag source half of OLE drag-and-drop: drop on release, cancel on Escape.
class DropSource : public IDropSource {
public:
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override {
        if (!ppv) return E_POINTER;
        if (riid == IID_IUnknown || riid == IID_IDropSource) {
            *ppv = static_cast<IDropSource*>(this);
            AddRef();
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return static_cast<ULONG>(InterlockedIncrement(&refs_)); }
    ULONG STDMETHODCALLTYPE Release() override {
        const LONG r = InterlockedDecrement(&refs_);
        if (r == 0) delete this;
        return static_cast<ULONG>(r);
    }
    HRESULT STDMETHODCALLTYPE QueryContinueDrag(BOOL escape, DWORD keys) override {
        if (escape) return DRAGDROP_S_CANCEL;
        if (!(keys & MK_LBUTTON)) return DRAGDROP_S_DROP;
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE GiveFeedback(DWORD) override { return DRAGDROP_S_USEDEFAULTCURSORS; }

private:
    LONG refs_{1};
};

// Drags a file as the Explorer does (CF_HDROP and the shell formats), which
// is what DAWs accept for MIDI files. Modal until the drop.
static bool dragFile(HWND hwnd, const std::filesystem::path& file) {
    const HRESULT ole = OleInitialize(nullptr);   // S_FALSE: already initialised on this thread
    bool ok = false;
    PIDLIST_ABSOLUTE pidl = ILCreateFromPathW(file.wstring().c_str());
    if (pidl) {
        IShellFolder* folder = nullptr;
        PCUITEMID_CHILD child = nullptr;
        if (SUCCEEDED(SHBindToParent(pidl, IID_IShellFolder, reinterpret_cast<void**>(&folder), &child))) {
            IDataObject* data = nullptr;
            if (SUCCEEDED(folder->GetUIObjectOf(hwnd, 1, &child, IID_IDataObject, nullptr,
                                                reinterpret_cast<void**>(&data)))) {
                auto* source = new DropSource();
                DWORD effect = DROPEFFECT_NONE;
                ok = DoDragDrop(data, source, DROPEFFECT_COPY, &effect) == DRAGDROP_S_DROP;
                source->Release();
                data->Release();
            }
            folder->Release();
        }
        ILFree(pidl);
    }
    if (SUCCEEDED(ole)) OleUninitialize();
    return ok;
}

// The common open / save dialog for .burette files. Empty if cancelled.
static std::filesystem::path bankFileDialog(HWND hwnd, bool save) {
    FileDialogOptions options;
    options.save = save;
    options.title = save ? "Save pattern bank" : "Load pattern bank";
    options.filterName = "Burette banks";
    options.extension = "burette";
    if (save) options.startPath = "Burette bank.burette";
    return runFileDialog(hwnd, options);
}

void SequencerGui::performActionsWin32(uint32_t actions) {
    HWND hwnd = static_cast<HWND>(hwnd_);
    if (!hwnd) return;
    if (actions & kActionTakeFocus) SetFocus(hwnd);
    if ((actions & kActionReleaseFocus) && GetFocus() == hwnd) {
        if (HWND parent = GetParent(hwnd)) SetFocus(parent);
    }
    if (actions & kActionDragMidi) {
        const std::filesystem::path file = plugin_->writePatternMidi(plugin_->editPattern());
        if (file.empty()) {
            setStatus("COULD NOT WRITE THE MIDI FILE");
        } else {
            ReleaseCapture();
            dragFile(hwnd, file);
            mouseUp(0, 0);   // the drag loop took the button release
        }
    }
    if (actions & (kActionLoadBank | kActionSaveBank)) {
        const bool save = (actions & kActionSaveBank) != 0;
        const std::filesystem::path file = bankFileDialog(hwnd, save);
        if (!file.empty()) {
            const std::string utf8 = file.u8string();
            if (save) saveBankTo(utf8);
            else loadBankFrom(utf8);
        }
    }
}

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
                gui->performActionsWin32(gui->takeActions());
            }
            return 0;
        case WM_MOUSEMOVE:
            if (gui && (wParam & MK_LBUTTON)) {
                gui->mouseDrag(x, y);
                gui->performActionsWin32(gui->takeActions());
            }
            return 0;
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
            ReleaseCapture();
            if (gui && msg == WM_LBUTTONUP) gui->mouseUp(x, y);
            return 0;
        case WM_KEYDOWN:
            if (gui && gui->editingName()) {
                if (wParam == VK_RETURN) gui->keyPress(SequencerGui::Key::Enter);
                else if (wParam == VK_ESCAPE) gui->keyPress(SequencerGui::Key::Escape);
                else if (wParam == VK_BACK) gui->keyPress(SequencerGui::Key::Backspace);
                gui->performActionsWin32(gui->takeActions());
                return 0;
            }
            break;
        case WM_CHAR:
            if (gui && gui->editingName()) {
                if (wParam >= 32 && wParam < 127) {
                    const char text[2] = { static_cast<char>(wParam), 0 };
                    gui->keyText(text);
                }
                return 0;
            }
            break;
        case WM_GETDLGCODE:
            return DLGC_WANTALLKEYS | DLGC_WANTCHARS;   // Enter and Escape are ours while typing
        case WM_KILLFOCUS:
            if (gui) gui->focusLost();
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
    hwnd_ = CreateWindowExW(0, kClassName, L"Burette", WS_CHILD | WS_VISIBLE, 0, 0, kWidth, kHeight,
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
