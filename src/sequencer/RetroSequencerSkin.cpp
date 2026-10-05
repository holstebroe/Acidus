#include "SequencerSkin.hpp"
#include "SequencerClap.hpp"
#include "SequencerGui.hpp"
#include "SequencerLayout.hpp"
#include "gui/Font.hpp"
#include "gui/Graphics.hpp"
#include <algorithm>
#include <cstdio>
#include <string>

namespace acidus {
namespace seq {

using namespace layout;

namespace {

// --- Colours (ARGB) -----------------------------------------------------------

// Graphite panel with acid-green notes, after the Acidus logo plate.
static constexpr uint32_t kBg = 0xFF1C1F22;
static constexpr uint32_t kPanelDark = 0xFF2C3034;
static constexpr uint32_t kGridLine = 0xFF464B50;
static constexpr uint32_t kLight = 0xFFD6DADE;        // labels and symbols
static constexpr uint32_t kDim = 0xFF7D848A;          // secondary text
static constexpr uint32_t kInk = 0xFF0E1A08;          // text on light and green fills
static constexpr uint32_t kNoteBg = 0xFF6FE03A;       // a note, and a tie
static constexpr uint32_t kChainMark = 0xFF2F6A1C;    // underline of chained pattern buttons
static constexpr uint32_t kRestBg = 0xFF121416;       // a rest: darker than the panel
static constexpr uint32_t kAccentMark = 0xFFFFB020;
static constexpr uint32_t kInactiveShade = 0xB4000000;
static constexpr uint32_t kAcid = 0xFF39FF14;         // playing pattern
static constexpr uint32_t kPlayMark = 0xFFFFD21E;     // playing step
static constexpr uint32_t kPlayTint = 0x40FFD21E;
static constexpr uint32_t kDarkText = 0xFF0E1A08;
static constexpr uint32_t kMidiBg = 0xFF3EC6FF;       // the MIDI drag button: unlike anything else


// The retro skin: flat fills and the 5x7 pixel font on a graphite panel.
// The smallest build.
class RetroSequencerSkin : public ISequencerSkin {
public:
    int supersample() const override { return 1; }
    void draw(Graphics& g, const SequencerView& v) override;

private:
    Font font_{Font::classic5x7()};
};

} // namespace

std::unique_ptr<ISequencerSkin> createSequencerSkin() {
    return std::make_unique<RetroSequencerSkin>();
}

// --- Drawing ----------------------------------------------------------------------

static void fillTriangle(Graphics& g, int cx, int top, int w, int h, bool up, uint32_t color) {
    for (int r = 0; r < h; ++r) {
        const int rowFromApex = up ? r : h - 1 - r;
        const int rw = std::max(1, (rowFromApex * w) / std::max(1, h - 1));
        g.fillRect(cx - rw / 2, top + r, rw, 1, color);
    }
}

// A triangle pointing right, `w` wide and `h` high, apex at the right.
static void fillTriangleRight(Graphics& g, int left, int cy, int w, int h, uint32_t color) {
    for (int c = 0; c < w; ++c) {
        const int ch = std::max(1, ((w - c) * h) / w);
        g.fillRect(left + c, cy - ch / 2, 1, ch, color);
    }
}

// A 16x16 floppy disk at (x, y), drawn in `color` on `bg`.
static void drawFloppy(Graphics& g, int x, int y, uint32_t color, uint32_t bg) {
    g.fillRect(x, y, 16, 16, color);
    g.fillRect(x + 13, y, 3, 3, bg);            // the cut corner
    g.fillRect(x + 4, y, 8, 6, bg);             // metal shutter
    g.fillRect(x + 9, y + 1, 2, 4, color);      // its window
    g.fillRect(x + 3, y + 9, 10, 7, bg);        // label
    g.fillRect(x + 5, y + 11, 6, 1, color);
    g.fillRect(x + 5, y + 13, 6, 1, color);
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

void RetroSequencerSkin::draw(Graphics& g, const SequencerView& v) {
    const PatternBank& bank = v.plugin.bank();
    const int pattern = v.plugin.editPattern();
    const int length = bank.length(pattern);
    const int playingPattern = v.plugin.engine().playingPattern();
    const int playingStep = v.plugin.engine().playingStep();
    const int highlightStep = (playingPattern == pattern) ? playingStep : -1;
    char buf[64];
    int x, y, w, h;

    g.clear(kBg);

    // Title row: name, play, pattern name, INIT, load/save, MIDI drag, key.
    g.drawText(font_, "BURETTE", 14, kTitleY + 6, kAcid, 2);

    SequencerGui::buttonRect(SequencerGui::Button::Play, x, y, w, h);
    const bool isPlaying = v.playing;
    g.fillRect(x, y, w, h, isPlaying ? kAcid : kPanelDark);
    g.drawRect(x, y, w, h, kGridLine);
    if (isPlaying) {
        g.fillRect(x + w / 2 - 6, y + 7, 4, 12, kInk);   // pause
        g.fillRect(x + w / 2 + 2, y + 7, 4, 12, kInk);
    } else {
        fillTriangleRight(g, x + w / 2 - 5, y + h / 2, 12, 14, kLight);
    }

    std::snprintf(buf, sizeof(buf), "%d", pattern + 1);
    g.drawText(font_, buf, 148, kTitleY + 6, kLight, 2);
    SequencerGui::buttonRect(SequencerGui::Button::Name, x, y, w, h);
    g.fillRect(x, y, w, h, v.editingName ? kRestBg : kBg);
    g.drawRect(x, y, w, h, v.editingName ? kAcid : kGridLine);
    const std::string shownName = v.editingName ? v.nameDraft : bank.name(pattern);
    g.drawText(font_, shownName.c_str(), x + 8, y + 6, kLight, 2);
    if (v.editingName) {
        const int cx = x + 8 + font_.getTextWidth(shownName.c_str(), 2);
        g.fillRect(cx, y + 5, 2, 16, kAcid);   // caret
    }

    SequencerGui::buttonRect(SequencerGui::Button::Init, x, y, w, h);
    if (v.initArmed) drawBox(g, font_, x, y, w, h, "SURE?", kAccentMark, kInk, 2);
    else drawBox(g, font_, x, y, w, h, "INIT", kPanelDark, kLight, 2);

    // Load: a floppy with an arrow out of it; save: an arrow into it.
    SequencerGui::buttonRect(SequencerGui::Button::Load, x, y, w, h);
    g.fillRect(x, y, w, h, kPanelDark);
    g.drawRect(x, y, w, h, kGridLine);
    drawFloppy(g, x + 6, y + 5, kLight, kPanelDark);
    g.fillRect(x + 28, y + 11, 3, 9, kLight);
    fillTriangle(g, x + 29, y + 5, 9, 6, true, kLight);
    SequencerGui::buttonRect(SequencerGui::Button::Save, x, y, w, h);
    g.fillRect(x, y, w, h, kPanelDark);
    g.drawRect(x, y, w, h, kGridLine);
    drawFloppy(g, x + 6, y + 5, kLight, kPanelDark);
    g.fillRect(x + 28, y + 5, 3, 9, kLight);
    fillTriangle(g, x + 29, y + 14, 9, 6, false, kLight);

    // MIDI: a coloured tab with a grip, to drag onto a DAW track.
    SequencerGui::buttonRect(SequencerGui::Button::Midi, x, y, w, h);
    g.fillRect(x, y, w, h, kMidiBg);
    for (int gy = 0; gy < 3; ++gy) {
        for (int gx = 0; gx < 2; ++gx) g.fillRect(x + 6 + gx * 4, y + 8 + gy * 4, 2, 2, kInk);
    }
    drawCentered(g, font_, "MIDI", x + 10, y, w - 10, h, kInk, 2);

    SequencerGui::boxRect(SequencerGui::Box::Key, x, y, w, h);
    g.drawText(font_, "KEY", x - 22, y + 10, kLight, 1);
    const int key = v.plugin.globalTranspose();
    std::snprintf(buf, sizeof(buf), key > 0 ? "+%d" : "%d", key);
    drawBox(g, font_, x, y, w, h, buf, kPanelDark, kLight, 2);

    // Pattern buttons; the edited pattern's chain is underlined.
    int chain[kNumPatterns];
    const int chainCount = SequencerEngine::chainOf(bank, pattern, chain);
    bool inChain[kNumPatterns] = {};
    for (int i = 0; i < chainCount; ++i) inChain[chain[i]] = true;
    g.drawText(font_, "PATTERN", 14, kPatternY + 4, kLight, 1);
    // FOLLOW: a small LED toggle under the label.
    SequencerGui::buttonRect(SequencerGui::Button::Follow, x, y, w, h);
    const bool follow = v.plugin.followPlaying();
    g.fillRect(x + 4, y + 4, 7, 7, follow ? kAcid : kRestBg);
    g.drawRect(x + 4, y + 4, 7, 7, follow ? kAcid : kGridLine);
    g.drawText(font_, "FOLLOW", x + 16, y + 4, follow ? kLight : kDim, 1);
    for (int p = 0; p < kNumPatterns; ++p) {
        SequencerGui::patternButtonRect(p, x, y, w, h);
        const bool editing = p == pattern;
        const bool playingThis = p == playingPattern;
        uint32_t bg = editing ? kLight : kPanelDark;
        uint32_t fg = editing ? kInk : kLight;
        if (playingThis) { bg = kAcid; fg = kDarkText; }
        std::snprintf(buf, sizeof(buf), "%d", p + 1);
        drawBox(g, font_, x, y, w, h, buf, bg, fg, 2);
        if (inChain[p] && !editing) g.fillRect(x + 6, y + h - 5, w - 12, 3, kChainMark);
        if (playingThis && editing) {
            g.drawRect(x + 2, y + 2, w - 4, h - 4, kLight);
            g.drawRect(x + 3, y + 3, w - 6, h - 6, kLight);
        }
    }

    // Row labels.
    static const char* kLabels[SequencerGui::kRowCount] = { "NOTE", "OCTAVE", "ACCENT", "SLIDE" };
    for (int r = 0; r < SequencerGui::kRowCount; ++r) {
        g.drawText(font_, kLabels[r], 14, rowTop(r) + (kRowH[r] - 7) / 2, kLight, 1);
    }

    // Grid cells.
    for (int s = 0; s < kMaxSteps; ++s) {
        const Step st = bank.step(pattern, s);
        for (int r = 0; r < SequencerGui::kRowCount; ++r) {
            const SequencerGui::Row row = static_cast<SequencerGui::Row>(r);
            SequencerGui::cellRect(row, s, x, y, w, h);
            if (row == SequencerGui::Row::Note) {
                if (st.isNote()) {
                    g.fillRect(x, y, w, h, kNoteBg);
                    drawCentered(g, font_, noteName(st.note), x, y, w, h, kDarkText, 3);
                } else {
                    // A tie: the note fill without a name, so it reads as the note
                    // carrying on. A rest: a dark empty cell.
                    g.fillRect(x, y, w, h, st.note == kNoteTie ? kNoteBg : kRestBg);
                }
            } else if (row == SequencerGui::Row::Octave) {
                if (st.octave != 0) {
                    fillTriangle(g, x + w / 2, y + h / 2 - 7, 18, 14, st.octave > 0, kLight);
                }
            } else if (row == SequencerGui::Row::Accent) {
                if (st.accent) drawCentered(g, font_, "A", x, y, w, h, kAccentMark, 3);
            } else {
                if (st.slide) drawCentered(g, font_, "S", x, y, w, h, kLight, 3);
            }
        }
    }

    // Grid lines.
    const int gridBottom = rowTop(SequencerGui::kRowCount);
    const int gridRight = kGridX + kMaxSteps * kCellW;
    for (int s = 0; s <= kMaxSteps; ++s) {
        const int lx = kGridX + s * kCellW;
        g.fillRect(lx - ((s % 4 == 0) ? 1 : 0), kGridY, (s % 4 == 0) ? 3 : 1, gridBottom - kGridY, kGridLine);
    }
    for (int r = 0; r <= SequencerGui::kRowCount; ++r) {
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

    // Pattern setup row.
    g.drawText(font_, "SETUP", 14, kSetupY + 10, kLight, 1);
    SequencerGui::boxRect(SequencerGui::Box::Length, x, y, w, h);
    g.drawText(font_, "LENGTH", x - 42, y + 10, kLight, 1);
    std::snprintf(buf, sizeof(buf), "%d", length);
    drawBox(g, font_, x, y, w, h, buf, kPanelDark, kLight, 2);
    SequencerGui::boxRect(SequencerGui::Box::Transpose, x, y, w, h);
    g.drawText(font_, "TRANSPOSE", x - 60, y + 10, kLight, 1);
    const int tr = bank.transpose(pattern);
    std::snprintf(buf, sizeof(buf), tr > 0 ? "+%d" : "%d", tr);
    drawBox(g, font_, x, y, w, h, buf, kPanelDark, kLight, 2);
    SequencerGui::boxRect(SequencerGui::Box::Next, x, y, w, h);
    g.drawText(font_, "NEXT", x - 30, y + 10, kLight, 1);
    const int next = bank.next(pattern);
    std::snprintf(buf, sizeof(buf), next < 0 ? "-" : "%d", next + 1);
    drawBox(g, font_, x, y, w, h, buf, kPanelDark, kLight, 2);
    std::string chainText = "PLAYS ";
    for (int i = 0; i < chainCount; ++i) chainText += std::to_string(chain[i] + 1) + (i + 1 < chainCount ? ">" : "");
    chainText += chainCount > 1 ? "  THEN LOOPS" : "  (LOOPS)";
    g.drawText(font_, chainText.c_str(), x + w + 16, y + 10, kLight, 1);

    // Footer: the trigger key and a status line; the mouse legend at the right.
    const int triggerKey = kFirstTriggerKey + pattern;
    std::snprintf(buf, sizeof(buf), "TRIGGER KEY %s (MIDI %d)    PATTERNS 1-16 = %s TO %s",
                  keyName(triggerKey).c_str(), triggerKey, keyName(kFirstTriggerKey).c_str(),
                  keyName(kFirstTriggerKey + kNumPatterns - 1).c_str());
    g.drawText(font_, buf, 14, kFooterY, kLight, 1);
    if (!v.status.empty()) g.drawText(font_, v.status.c_str(), 14, kFooterY + 14, kAccentMark, 1);
    static const char* kHints[3][2] = {
        { "CLICK", "NEXT" }, { "RIGHT-CLICK", "PREVIOUS" }, { "DRAG UP/DOWN", "SCROLL" },
    };
    for (int i = 0; i < 3; ++i) {
        g.drawText(font_, kHints[i][0], kHintX, kFooterY + 11 * i, kDim, 1);
        g.drawText(font_, kHints[i][1], kHintValueX, kFooterY + 11 * i, kLight, 1);
    }
}

} // namespace seq
} // namespace acidus
