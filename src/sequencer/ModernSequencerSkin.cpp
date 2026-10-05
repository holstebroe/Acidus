// Burette's modern skin (ACIDUS_GUI_STYLE=MODERN), to match the Acidus
// modern panel: the same worn silver panel, with backlit green keys for the
// notes and the edited pattern, glowing symbols in recessed wells for
// octave, accent and slide, blue hardware buttons for the title row and
// dark value boxes. Painted at 2x and filtered down by SequencerGui.
#include "SequencerSkin.hpp"
#include "SequencerClap.hpp"
#include "SequencerGui.hpp"
#include "SequencerLayout.hpp"
#include "gui/Graphics.hpp"
#include "gui/modern/DisplayFont.hpp"
#include "gui/modern/LabelFont.hpp"
#include "gui/modern/ModernDraw.hpp"
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace acidus {
namespace seq {

using namespace layout;
using namespace modern;

namespace {

const Color kBlue{ 0.22f, 0.43f, 0.64f };
const Color kGreen{ 0.47f, 0.92f, 0.36f };
const Color kAmber{ 1.00f, 0.66f, 0.12f };
const Color kSlate{ 0.27f, 0.28f, 0.29f };     // dark keys and value boxes
const Color kLightText{ 0.93f, 0.95f, 0.96f };
const Color kDarkText{ 0.06f, 0.11f, 0.04f };
constexpr uint32_t kDimInk = 0x90222326;
constexpr uint32_t kStatusInk = 0xFF8A4A00;
constexpr float kLabelCap = 7.f;
// The playing step's column: a tint behind its cells and a glowing frame
// round it. Raise the alphas (0..1) to make the playhead stand out more.
const Color kPlayhead{ 0.22f, 0.43f, 0.64f };
// Steps past the pattern length fade this far (0..1) into the panel.
constexpr float kInactiveFade = 0.75f;
constexpr float kPlayheadTint = 0.4f;        // behind the cells
constexpr float kPlayheadFrame = 0.85f;       // the frame line
constexpr float kPlayheadGlow = 0.6f;        // its glow outside the column

void label(Graphics& g, const char* text, float x, float capTop, uint32_t argb = kInk) {
    drawText(g, kLabelFont, text, x, capTop, kLabelCap, argb, 0.15f);
}

// Text centred in a box, in the display face.
void boxText(Graphics& g, const char* text, float x, float y, float w, float h, float cap, Color c, float a = 1.f) {
    drawTextCentered(g, kDisplayFont, text, x + w / 2.f, y + (h - cap) / 2.f, cap, toArgb(c, a));
}

// Soft shadow of a rounded box standing off the panel. A compact one stays
// within 3 px of the box (the grid cells' margin; see draw()).
void dropShadow(Graphics& g, float x, float y, float w, float h, float r, float strength, bool compact = false) {
    const float ox = compact ? 0.6f : 1.f, oy = compact ? 1.f : 1.6f, blur = compact ? 1.8f : 3.5f;
    const float cx = x + w / 2 + ox, cy = y + h / 2 + oy;
    shadeBox(g, x - 4, y - 3, x + w + 6, y + h + 7, [&](float px, float py) {
        const float d = sdRoundRect(px - cx, py - cy, w / 2, h / 2, r);
        const float a = strength * (1.f - smoothstep(-1.5f, blur, d));
        return a > 0.f ? toArgb({ 0.04f, 0.04f, 0.03f }, a) : 0u;
    });
}

// A raised hardware key or button: bevelled, lit from the upper left, a
// little brighter at the top.
void raisedKey(Graphics& g, float x, float y, float w, float h, Color base, float r = 3.f, bool compact = false) {
    dropShadow(g, x, y, w, h, r, 0.45f, compact);
    const float s = static_cast<float>(g.getScale());
    const float cx = x + w / 2, cy = y + h / 2;
    const Material m{ base, 0.55f, 0.55f, 0.35f, 24.f };
    shadeBox(g, x - 1, y - 1, x + w + 1, y + h + 1, [&](float px, float py) {
        const float d = sdRoundRect(px - cx, py - cy, w / 2, h / 2, r);
        const float cov = clamp01(-d * s + 0.5f);
        if (cov <= 0.f) return 0u;
        float ux = 0.f, uy = 0.f;
        const float bevel = smoothstep(-1.8f, 0.f, d);
        if (bevel > 0.f) sdRoundRectDir(px - cx, py - cy, w / 2, h / 2, r, ux, uy);
        Color c = shadeLit(m, tiltedNormal(ux, uy, 0.9f * bevel));
        c = scale(c, 1.06f - 0.12f * (py - y) / h);
        return toArgb(c, cov);
    });
}

// A backlit translucent key: glows in its colour, with a halo on the panel
// and a glossy sheen across the top. `glow` 0..1 dims it.
void litKey(Graphics& g, float x, float y, float w, float h, Color base, float glow = 1.f, float r = 3.f,
            bool compact = false) {
    const float haloR = compact ? 2.9f : 5.5f;
    const float s = static_cast<float>(g.getScale());
    const float cx = x + w / 2, cy = y + h / 2;
    shadeBox(g, x - 6, y - 6, x + w + 6, y + h + 6, [&](float px, float py) {
        const float d = sdRoundRect(px - cx, py - cy, w / 2, h / 2, r);
        if (d <= 0.f) return 0u;
        const float a = 0.30f * glow * (1.f - smoothstep(0.f, haloR, d));
        return a > 0.f ? toArgb(base, a) : 0u;
    });
    dropShadow(g, x, y, w, h, r, 0.25f, compact);
    shadeBox(g, x - 1, y - 1, x + w + 1, y + h + 1, [&](float px, float py) {
        const float lx = px - cx, ly = py - cy;
        const float d = sdRoundRect(lx, ly, w / 2, h / 2, r);
        const float cov = clamp01(-d * s + 0.5f);
        if (cov <= 0.f) return 0u;
        // Brightest in the middle, where the lamp is.
        const float rx = lx / (w / 2), ry = ly / (h / 2);
        const float centre = 1.f - 0.35f * std::min(1.f, rx * rx * 0.6f + ry * ry);
        Color c = scale(base, (0.55f + 0.5f * glow) * centre);
        // Rim: darker edge, lit along the top-left.
        float ux = 0.f, uy = 0.f;
        const float rim = smoothstep(-1.6f, 0.f, d);
        if (rim > 0.f) sdRoundRectDir(lx, ly, w / 2, h / 2, r, ux, uy);
        c = scale(c, 1.f - 0.25f * rim);
        c = mix(c, Color{ 1.f, 1.f, 1.f }, 0.35f * rim * clamp01(-(ux * 0.7f + uy)));
        // Glossy sheen on the upper part.
        const float sheen = (1.f - smoothstep(-0.9f, -0.1f, ry)) * smoothstep(-1.f, -0.6f, ry) * 0.18f;
        c = mix(c, Color{ 1.f, 1.f, 1.f }, sheen);
        return toArgb(c, cov);
    });
}

// A recessed well in the panel: dark floor, shadowed upper-left walls and a
// lit lip along the lower right.
void well(Graphics& g, float x, float y, float w, float h, float r = 3.f) {
    const float s = static_cast<float>(g.getScale());
    const float cx = x + w / 2, cy = y + h / 2;
    const Material floor{ kSlate, 0.75f, 0.35f, 0.1f, 10.f };
    shadeBox(g, x - 2, y - 2, x + w + 2, y + h + 2, [&](float px, float py) {
        const float lx = px - cx, ly = py - cy;
        const float d = sdRoundRect(lx, ly, w / 2, h / 2, r);
        float ux = 0.f, uy = 0.f;
        if (d > -2.2f) sdRoundRectDir(lx, ly, w / 2, h / 2, r, ux, uy);
        if (d > 0.f) {
            // The panel's edge around the hole catches the light below
            // and right of it.
            const float a = (1.f - smoothstep(0.f, 1.4f, d)) * clamp01(ux * 0.6f + uy * 0.8f);
            return a > 0.f ? toArgb({ 1.f, 1.f, 1.f }, 0.55f * a) : 0u;
        }
        const float cov = clamp01(-d * s + 0.5f);
        const float wall = smoothstep(-2.2f, 0.f, d);
        Color c = shadeLit(floor, tiltedNormal(-ux, -uy, 1.0f * wall));
        // Inner shadow cast by the upper-left wall.
        const float sh = 1.f - smoothstep(0.f, 5.f, (ly + h / 2) * 0.8f + (lx + w / 2) * 0.3f);
        c = scale(c, 1.f - 0.5f * sh);
        return toArgb(c, cov);
    });
}

// Glowing symbols: a soft halo, then the bright core.
void glowText(Graphics& g, const char* text, float cx, float capTop, float cap, Color c) {
    const float w = textWidth(kDisplayFont, text, cap);
    for (int i = 0; i < 8; ++i) {
        const float a = i * kPi / 4.f;
        drawText(g, kDisplayFont, text, cx - w / 2 + 1.4f * std::cos(a), capTop + 1.4f * std::sin(a), cap,
                 toArgb(c, 0.10f));
    }
    drawText(g, kDisplayFont, text, cx - w / 2, capTop, cap, toArgb(mix(c, Color{ 1.f, 1.f, 1.f }, 0.25f), 1.f));
}

// A filled triangle pointing up or down, centred at (cx, cy), with a glow.
void glowTriangle(Graphics& g, float cx, float cy, float w, float h, bool up, Color c) {
    const float s = static_cast<float>(g.getScale());
    const float apexY = up ? cy - h / 2 : cy + h / 2, baseY = up ? cy + h / 2 : cy - h / 2;
    // Inward distances to the three edges; the minimum is the signed depth.
    const float ex = w / 2, ey = baseY - apexY;
    const float len = std::sqrt(ex * ex + ey * ey);
    auto depth = [&](float px, float py) {
        const float dBase = up ? baseY - py : py - baseY;
        const float rx = std::fabs(px - cx), ry = py - apexY;
        // Distance inside the slanted edge from the apex to (cx +- w/2, baseY).
        const float dSide = (ex * ry - ey * rx) / len * (up ? 1.f : -1.f);
        return std::min(dBase, dSide);
    };
    const Color core = mix(c, Color{ 1.f, 1.f, 1.f }, 0.25f);
    shadeBox(g, cx - w / 2 - 4, cy - h / 2 - 4, cx + w / 2 + 4, cy + h / 2 + 4, [&](float px, float py) {
        const float d = depth(px, py);
        const float cov = clamp01(d * s + 0.5f);
        const float halo = 0.35f * (1.f - smoothstep(-3.f, 0.f, -d)) * (d < 0.f ? 1.f : 0.f);
        if (cov > 0.f) return toArgb(core, cov);
        return halo > 0.f ? toArgb(c, halo) : 0u;
    });
}

// A small round LED: lit green, or a dark lens.
void led(Graphics& g, float cx, float cy, float r, bool on) {
    const float s = static_cast<float>(g.getScale());
    shadeBox(g, cx - r - 4, cy - r - 4, cx + r + 4, cy + r + 4, [&](float px, float py) {
        const float d = std::hypot(px - cx, py - cy);
        if (d > r) {
            const float a = on ? 0.4f * (1.f - smoothstep(r, r + 3.5f, d)) : 0.f;
            const float rim = (1.f - smoothstep(r, r + 0.8f, d)) * 0.5f;
            return a > 0.f ? toArgb(kGreen, a) : (rim > 0.f ? toArgb({ 0.1f, 0.1f, 0.1f }, rim) : 0u);
        }
        const float cov = clamp01((r - d) * s + 0.5f);
        Color c = on ? mix(kGreen, Color{ 1.f, 1.f, 1.f }, 0.4f * (1.f - d / r)) : Color{ 0.18f, 0.2f, 0.18f };
        if (!on && px - cx < 0 && py - cy < 0 && d < r * 0.5f) c = mix(c, Color{ 1.f, 1.f, 1.f }, 0.25f);
        return toArgb(c, cov);
    });
}

// Floppy disk icon, 14 px, for load and save.
void floppy(Graphics& g, float x, float y, Color c) {
    const uint32_t col = toArgb(c, 1.f), hole = toArgb(kBlue, 1.f);
    shadeBox(g, x, y, x + 14, y + 14, [&](float px, float py) {
        if (px - x > 11.f && py - y < 3.f && (px - x - 11.f) > (py - y)) return 0u;   // cut corner
        return col;
    });
    shadeBox(g, x + 3.5f, y, x + 10.5f, y + 5, [&](float, float) { return hole; });           // shutter
    shadeBox(g, x + 8, y + 1, x + 9.5f, y + 4, [&](float, float) { return col; });
    shadeBox(g, x + 2.5f, y + 8, x + 11.5f, y + 14, [&](float, float) { return hole; });       // label
    drawLineAA(g, x + 4.5f, y + 10, x + 9.5f, y + 10, 0.8f, col);
    drawLineAA(g, x + 4.5f, y + 12, x + 9.5f, y + 12, 0.8f, col);
}

void arrow(Graphics& g, float cx, float top, float bottom, bool up, Color c) {
    const uint32_t col = toArgb(c, 1.f);
    drawLineAA(g, cx, up ? top + 3 : top, cx, up ? bottom : bottom - 3, 2.f, col);
    const float tip = up ? top : bottom, back = up ? top + 5 : bottom - 5;
    drawLineAA(g, cx - 3.5f, back, cx, tip, 1.8f, col);
    drawLineAA(g, cx + 3.5f, back, cx, tip, 1.8f, col);
}

class ModernSequencerSkin : public ISequencerSkin {
public:
    int supersample() const override { return 2; }
    void draw(Graphics& g, const SequencerView& v) override;

private:
    std::vector<uint32_t> panel_;   // the worn panel texture, painted once
    struct CellSprite {
        uint32_t key{0};            // what the cell shows; 0 = nothing cached
        int scale{0};
        std::vector<uint32_t> pixels;
    };
    CellSprite cells_[SequencerGui::kRowCount][kMaxSteps];
};

} // namespace

std::unique_ptr<ISequencerSkin> createSequencerSkin() {
    return std::make_unique<ModernSequencerSkin>();
}

void ModernSequencerSkin::draw(Graphics& g, const SequencerView& v) {
    using B = SequencerGui::Button;
    using X = SequencerGui::Box;
    using R = SequencerGui::Row;
    SequencerClap& plugin = v.plugin;
    const PatternBank& bank = plugin.bank();
    const int pattern = plugin.editPattern();
    const int length = bank.length(pattern);
    const int playingPattern = plugin.engine().playingPattern();
    const int playingStep = plugin.engine().playingStep();
    const int highlightStep = (playingPattern == pattern) ? playingStep : -1;
    char buf[96];
    int x, y, w, h;

    const size_t size = static_cast<size_t>(g.getWidth()) * g.getHeight() * g.getScale() * g.getScale();
    if (panel_.size() != size) {
        paintWornPanel(g, static_cast<int>(g.getWidth()), static_cast<int>(g.getHeight()), false);
        panel_.assign(g.getBuffer(), g.getBuffer() + size);
        for (auto& row : cells_) for (auto& c : row) c.key = 0;
    } else {
        std::memcpy(g.getBuffer(), panel_.data(), size * sizeof(uint32_t));
    }

    // --- Title row --------------------------------------------------------
    const float titleCap = 11.5f;
    const float titleTop = kTitleY + (kTitleH - titleCap) / 2.f;
    drawText(g, kDisplayFont, "BURETTE", 14, titleTop, titleCap, kInk, 0.1f);

    SequencerGui::buttonRect(B::Play, x, y, w, h);
    if (v.playing) {
        litKey(g, x, y, w, h, kGreen);
        const uint32_t ink = toArgb(kDarkText, 1.f);
        shadeBox(g, x + w / 2.f - 6, y + 7, x + w / 2.f - 2, y + h - 7.f, [&](float, float) { return ink; });
        shadeBox(g, x + w / 2.f + 2, y + 7, x + w / 2.f + 6, y + h - 7.f, [&](float, float) { return ink; });
    } else {
        raisedKey(g, x, y, w, h, kBlue);
        const float tx = x + w / 2.f - 4.5f, ty = y + h / 2.f;
        const uint32_t col = toArgb(kLightText, 1.f);
        shadeBox(g, tx - 1, ty - 8, tx + 12, ty + 8, [&](float px, float py) {
            const float d = std::min(px - tx, (6.f - std::fabs(py - ty)) - (px - tx) * 6.f / 11.f);
            const float cov = clamp01(d * g.getScale() + 0.5f);
            return cov > 0.f ? toArgb(kLightText, cov) : 0u;
        });
        (void)col;
    }

    std::snprintf(buf, sizeof(buf), "%d", pattern + 1);
    drawText(g, kDisplayFont, buf, 148, titleTop, titleCap, kInk, 0.1f);
    SequencerGui::buttonRect(B::Name, x, y, w, h);
    const std::string shownName = v.editingName ? v.nameDraft : bank.name(pattern);
    const float nameCap = 12.f, nameTop = y + (h - nameCap) / 2.f;
    if (v.editingName) {
        well(g, x, y, w, h);
        drawText(g, kDisplayFont, shownName.c_str(), x + 8.f, nameTop, nameCap, toArgb(kLightText, 1.f));
        const float caretX = x + 8.f + textWidth(kDisplayFont, shownName.c_str(), nameCap) + 1.f;
        drawLineAA(g, caretX, y + 5.f, caretX, y + h - 5.f, 1.6f, toArgb(kGreen, 1.f));
    } else {
        drawText(g, kDisplayFont, shownName.c_str(), x + 8.f, nameTop, nameCap, kInk, 0.1f);
    }

    SequencerGui::buttonRect(B::Init, x, y, w, h);
    if (v.initArmed) {
        litKey(g, x, y, w, h, kAmber);
        boxText(g, "SURE?", x, y, w, h, 10.f, kDarkText);
    } else {
        raisedKey(g, x, y, w, h, kBlue);
        boxText(g, "INIT", x, y, w, h, 10.f, kLightText);
    }

    SequencerGui::buttonRect(B::Load, x, y, w, h);
    raisedKey(g, x, y, w, h, kBlue);
    floppy(g, x + 7.f, y + 6.f, kLightText);
    arrow(g, x + 30.f, y + 6.f, y + 19.f, true, kLightText);
    SequencerGui::buttonRect(B::Save, x, y, w, h);
    raisedKey(g, x, y, w, h, kBlue);
    floppy(g, x + 7.f, y + 6.f, kLightText);
    arrow(g, x + 30.f, y + 6.f, y + 19.f, false, kLightText);

    // MIDI: a grip to drag the pattern out onto a DAW track.
    SequencerGui::buttonRect(B::Midi, x, y, w, h);
    raisedKey(g, x, y, w, h, kBlue);
    for (int gy = 0; gy < 3; ++gy) {
        for (int gx = 0; gx < 2; ++gx) {
            const float dx = x + 8.f + gx * 4.f, dy = y + 9.f + gy * 4.f;
            shadeBox(g, dx - 1.5f, dy - 1.5f, dx + 1.5f, dy + 1.5f, [&](float px, float py) {
                const float d = std::hypot(px - dx, py - dy);
                return d < 1.2f ? toArgb(kLightText, 0.85f) : 0u;
            });
        }
    }
    boxText(g, "MIDI", x + 10.f, y, w - 10.f, h, 10.f, kLightText);

    SequencerGui::boxRect(X::Key, x, y, w, h);
    label(g, "KEY", x - 22.f, y + (h - kLabelCap) / 2.f);
    const int key = plugin.globalTranspose();
    std::snprintf(buf, sizeof(buf), key > 0 ? "+%d" : "%d", key);
    raisedKey(g, x, y, w, h, kBlue);
    boxText(g, buf, x, y, w, h, 11.f, kLightText);

    // --- Pattern buttons ------------------------------------------------------
    int chain[kNumPatterns];
    const int chainCount = SequencerEngine::chainOf(bank, pattern, chain);
    bool inChain[kNumPatterns] = {};
    for (int i = 0; i < chainCount; ++i) inChain[chain[i]] = true;
    label(g, "PATTERN", 14, kPatternY + 4.f);
    SequencerGui::buttonRect(B::Follow, x, y, w, h);
    const bool follow = plugin.followPlaying();
    led(g, x + 7.5f, y + 7.5f, 3.f, follow);
    label(g, "FOLLOW", x + 16.f, y + 4.f, follow ? kInk : kDimInk);
    for (int p = 0; p < kNumPatterns; ++p) {
        SequencerGui::patternButtonRect(p, x, y, w, h);
        const bool editing = p == pattern;
        const bool playingThis = p == playingPattern;
        std::snprintf(buf, sizeof(buf), "%d", p + 1);
        if (editing) {
            litKey(g, x, y, w, h, kGreen, playingThis ? 1.f : 0.8f);
            boxText(g, buf, x, y, w, h, 12.f, kDarkText);
            if (playingThis) {   // playing too: a bright ring
                const float cx = x + w / 2.f, cy = y + h / 2.f;
                shadeBox(g, x - 2, y - 2, x + w + 2, y + h + 2, [&](float px, float py) {
                    const float d = std::fabs(sdRoundRect(px - cx, py - cy, w / 2.f + 1.5f, h / 2.f + 1.5f, 4.f));
                    const float a = 1.f - smoothstep(0.4f, 1.2f, d);
                    return a > 0.f ? toArgb({ 0.9f, 1.f, 0.8f }, 0.9f * a) : 0u;
                });
            }
        } else if (playingThis) {
            glowText(g, buf, x + w / 2.f, y + (h - 12.f) / 2.f, 12.f, Color{ 0.25f, 0.85f, 0.15f });
        } else {
            drawTextCentered(g, kDisplayFont, buf, x + w / 2.f, y + (h - 12.f) / 2.f, 12.f, kInk, 0.1f);
        }
        // Patterns in the edited pattern's chain: a green bar underneath.
        if (inChain[p] && !editing) {
            drawLineAA(g, x + 12.f, y + h - 3.f, x + w - 12.f, y + h - 3.f, 2.f, toArgb({ 0.2f, 0.6f, 0.12f }, 0.9f));
        }
    }

    // --- Grid -----------------------------------------------------------------
    static const char* kLabels[SequencerGui::kRowCount] = { "NOTE", "OCTAVE", "ACCENT", "SLIDE" };
    for (int r = 0; r < SequencerGui::kRowCount; ++r) {
        label(g, kLabels[r], 14, rowTop(r) + (kRowH[r] - kLabelCap) / 2.f);
    }
    const int gridBottom = rowTop(SequencerGui::kRowCount);
    const int gridRight = kGridX + kMaxSteps * kCellW;

    // Grid cells. Shading them is most of a frame's cost, and a redraw
    // (the playhead moving on, an edit) changes only a few, so each cell's
    // finished pixels are kept, keyed by what it shows, and copied back while
    // that stays the same. Everything a cell draws stays inside its own
    // rectangle (the keys are inset 3 px and their halos and shadows are
    // compact), so the copies match a fresh draw exactly.
    constexpr float kInset = 3.f;
    const int ss = g.getScale();
    const int bufW = static_cast<int>(g.getWidth()) * ss;
    for (int s = 0; s < kMaxSteps; ++s) {
        const Step st = bank.step(pattern, s);
        const bool lit = s == highlightStep;
        for (int r = 0; r < SequencerGui::kRowCount; ++r) {
            const R row = static_cast<R>(r);
            SequencerGui::cellRect(row, s, x, y, w, h);
            CellSprite& sprite = cells_[r][s];
            const uint32_t key = 0x10000u | (lit ? 0x100u : 0u) | st.pack();
            const int bx = x * ss, by = y * ss, bw = w * ss, bh = h * ss;
            uint32_t* buffer = g.getBuffer();
            if (sprite.key == key && sprite.scale == ss) {
                for (int j = 0; j < bh; ++j) {
                    std::memcpy(buffer + static_cast<size_t>(by + j) * bufW + bx,
                                sprite.pixels.data() + static_cast<size_t>(j) * bw, bw * sizeof(uint32_t));
                }
                continue;
            }
            // The step playing now: a soft yellow light behind its column.
            if (lit) {
                shadeBox(g, x, y, x + w, y + h, [](float, float) { return toArgb(kPlayhead, kPlayheadTint); });
            }
            const float cx = x + kInset, cy = y + kInset, cw = w - 2 * kInset, ch = h - 2 * kInset;
            const float mid = x + w / 2.f;
            switch (row) {
                case R::Note:
                    if (st.isNote()) {
                        litKey(g, cx, cy, cw, ch, kGreen, 1.f, 3.f, true);
                        boxText(g, noteName(st.note), cx, cy, cw, ch, 15.f, kDarkText);
                    } else if (st.note == kNoteTie) {
                        // A tie: the note carrying on, lit but nameless.
                        litKey(g, cx, cy, cw, ch, kGreen, 0.7f, 3.f, true);
                    } else {
                        raisedKey(g, cx, cy, cw, ch, kSlate, 3.f, true);   // a rest: an unlit key
                    }
                    break;
                case R::Octave:
                    well(g, cx, cy, cw, ch);
                    if (st.octave != 0) {
                        glowTriangle(g, mid, cy + ch / 2.f, 16.f, 11.f, st.octave > 0, Color{ 0.80f, 0.95f, 0.75f });
                    }
                    break;
                case R::Accent:
                    well(g, cx, cy, cw, ch);
                    if (st.accent) glowText(g, "A", mid, cy + (ch - 15.f) / 2.f, 15.f, kAmber);
                    break;
                case R::Slide:
                    well(g, cx, cy, cw, ch);
                    if (st.slide) glowText(g, "S", mid, cy + (ch - 15.f) / 2.f, 15.f, Color{ 0.80f, 0.95f, 0.75f });
                    break;
            }
            sprite.key = key;
            sprite.scale = ss;
            sprite.pixels.resize(static_cast<size_t>(bw) * bh);
            for (int j = 0; j < bh; ++j) {
                std::memcpy(sprite.pixels.data() + static_cast<size_t>(j) * bw,
                            buffer + static_cast<size_t>(by + j) * bufW + bx, bw * sizeof(uint32_t));
            }
        }
    }

    // The playing step's column: a glowing yellow frame round it.
    if (highlightStep >= 0 && highlightStep < kMaxSteps) {
        const float sx = static_cast<float>(kGridX + highlightStep * kCellW);
        const float cx = sx + kCellW / 2.f, cy = (kGridY + gridBottom) / 2.f;
        const float hw = kCellW / 2.f - 1.f, hh = (gridBottom - kGridY) / 2.f - 1.f;
        shadeBox(g, sx - 4, kGridY - 4.f, sx + kCellW + 4, gridBottom + 4.f, [&](float px, float py) {
            const float d = sdRoundRect(px - cx, py - cy, hw, hh, 4.f);
            const float a = kPlayheadFrame * (1.f - smoothstep(0.f, 1.2f, std::fabs(d)))
                            + (d > 0.f ? kPlayheadGlow * (1.f - smoothstep(0.f, 3.5f, d)) : 0.f);
            return a > 0.f ? toArgb(kPlayhead, std::min(a, 1.f)) : 0u;
        });
    }

    // Printed brackets under each group of four steps.
    for (int grp = 0; grp < kMaxSteps / 4; ++grp) {
        const float gx0 = kGridX + grp * 4 * kCellW + 6.f, gx1 = kGridX + (grp + 1) * 4 * kCellW - 6.f;
        const float by = gridBottom + 4.f;
        drawLineAA(g, gx0, by, gx1, by, 1.2f, kDimInk);
        drawLineAA(g, gx0, by - 3.f, gx0, by, 1.2f, kDimInk);
        drawLineAA(g, gx1, by - 3.f, gx1, by, 1.2f, kDimInk);
    }

    // Steps past the pattern length are inactive: faded most of the way back
    // into the panel (its own texture), so they read as unavailable ghosts.
    if (length < kMaxSteps) {
        const int x0 = (kGridX + length * kCellW) * ss, x1 = gridRight * ss;
        const int y0 = (kGridY - 1) * ss, y1 = (gridBottom + 6) * ss;
        const uint32_t keep = static_cast<uint32_t>(256 * (1.f - kInactiveFade));
        uint32_t* buffer = g.getBuffer();
        for (int py = y0; py < y1; ++py) {
            for (int px = x0; px < x1; ++px) {
                const size_t i = static_cast<size_t>(py) * bufW + px;
                const uint32_t c = buffer[i], p = panel_[i];
                const uint32_t rb = ((p & 0xFF00FFu) * (256 - keep) + (c & 0xFF00FFu) * keep) >> 8;
                const uint32_t gr = ((p & 0x00FF00u) * (256 - keep) + (c & 0x00FF00u) * keep) >> 8;
                buffer[i] = 0xFF000000u | (rb & 0xFF00FFu) | (gr & 0x00FF00u);
            }
        }
    }

    // --- Pattern setup row --------------------------------------------------------
    const float setupLabelTop = kSetupY + (kSetupH - kLabelCap) / 2.f;
    label(g, "SETUP", 14, setupLabelTop);
    SequencerGui::boxRect(X::Length, x, y, w, h);
    label(g, "LENGTH", x - 42.f, setupLabelTop);
    std::snprintf(buf, sizeof(buf), "%d", length);
    raisedKey(g, x, y, w, h, kSlate);
    boxText(g, buf, x, y, w, h, 11.f, kLightText);
    SequencerGui::boxRect(X::Transpose, x, y, w, h);
    label(g, "TRANSPOSE", x - 60.f, setupLabelTop);
    const int tr = bank.transpose(pattern);
    std::snprintf(buf, sizeof(buf), tr > 0 ? "+%d" : "%d", tr);
    raisedKey(g, x, y, w, h, kSlate);
    boxText(g, buf, x, y, w, h, 11.f, kLightText);
    SequencerGui::boxRect(X::Next, x, y, w, h);
    label(g, "NEXT", x - 30.f, setupLabelTop);
    const int next = bank.next(pattern);
    std::snprintf(buf, sizeof(buf), next < 0 ? "-" : "%d", next + 1);
    raisedKey(g, x, y, w, h, kSlate);
    boxText(g, buf, x, y, w, h, 11.f, kLightText);
    std::string chainText = "PLAYS ";
    for (int i = 0; i < chainCount; ++i) chainText += std::to_string(chain[i] + 1) + (i + 1 < chainCount ? ">" : "");
    chainText += chainCount > 1 ? "  THEN LOOPS" : "  (LOOPS)";
    label(g, chainText.c_str(), x + w + 16.f, setupLabelTop);

    // --- Footer: trigger key, status line, mouse legend -----------------------------
    const int triggerKey = kFirstTriggerKey + pattern;
    std::snprintf(buf, sizeof(buf), "TRIGGER KEY %s (MIDI %d)    PATTERNS 1-16 = %s TO %s",
                  keyName(triggerKey).c_str(), triggerKey, keyName(kFirstTriggerKey).c_str(),
                  keyName(kFirstTriggerKey + kNumPatterns - 1).c_str());
    label(g, buf, 14, kFooterY);
    if (!v.status.empty()) label(g, v.status.c_str(), 14, kFooterY + 14.f, kStatusInk);
    static const char* kHints[3][2] = {
        { "CLICK", "NEXT" }, { "RIGHT-CLICK", "PREVIOUS" }, { "DRAG UP/DOWN", "SCROLL" },
    };
    for (int i = 0; i < 3; ++i) {
        label(g, kHints[i][0], kHintX, kFooterY + 11.f * i, kDimInk);
        label(g, kHints[i][1], kHintValueX, kFooterY + 11.f * i);
    }
}

} // namespace seq
} // namespace acidus
