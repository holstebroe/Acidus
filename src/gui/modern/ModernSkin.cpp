// The modern skin (ACIDUS_GUI_STYLE=MODERN): a worn, silver-painted 303
// panel with knobs shaded per pixel -- dark foot ring, knurled grip with a
// pointer nub, a polished chamfer and a spun-metal top with an incised
// pointer line -- lit from the upper left, with soft drop shadows and
// anti-aliased lettering. Everything is procedural apart from the label
// glyphs (LabelFont.hpp), so the cost over the retro skin is this code,
// ModernDraw.cpp and ~4 KB of glyph data.
//
// GuiWindow paints into a 2x supersampled buffer and box-filters it down;
// this skin shades at that buffer resolution (Graphics::blendPixel), with
// analytic edge coverage, so edges come out smooth after the downsample.
#include "gui/IGuiSkin.hpp"
#include "gui/Graphics.hpp"
#include "gui/Font.hpp"
#include "gui/GuiWindow.hpp"
#include "LabelFont.hpp"
#include "ModernDraw.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

namespace acidus {

namespace {

using namespace modern;

constexpr float kLabelCap = 7.f;   // label cap height

float labelWidth(const char* text) { return textWidth(kLabelFont, text, kLabelCap); }

// Silk-screen ink: anti-aliased, slightly patchy with age.
void drawLabel(Graphics& g, const char* text, float centerX, float capTop, uint32_t argb) {
    drawTextCentered(g, kLabelFont, text, centerX, capTop, kLabelCap, argb, 0.18f);
}

void paintPanel(Graphics& g, int width, int height, int dividerX) {
    paintWornPanel(g, width, height, true);
    // The printed divider before the logo plate.
    drawLineAA(g, dividerX + 0.5f, 18.f, dividerX + 0.5f, height - 18.f, 1.6f, kInk);
}

// --- Knob --------------------------------------------------------------------

// The 303's dial: 7 o'clock to 5 o'clock on the clock hours, 300 degrees.
constexpr float kStartAngle = 120.f * kPi / 180.f;   // 7 o'clock
constexpr float kTotalAngle = 300.f * kPi / 180.f;
constexpr int kGripRidges = 30;

void drawKnobDial(Graphics& g, float cx, float cy, float r) {
    // 11 printed ticks and the 303's index square above 12 o'clock.
    for (int i = 0; i < 11; ++i) {
        const float a = kStartAngle + kTotalAngle * i / 10.f;
        drawLineAA(g, cx + std::cos(a) * (r + 4.5f), cy + std::sin(a) * (r + 4.5f),
                   cx + std::cos(a) * (r + 8.5f), cy + std::sin(a) * (r + 8.5f), 1.4f, kInk);
    }
    shadeBox(g, cx - 2.f, cy - r - 14.f, cx + 2.f, cy - r - 10.f,
             [](float, float) { return kInk; });
}

void drawKnobBody(Graphics& g, float cx, float cy, float R, float pointer, uint32_t capColor) {
    const float s = static_cast<float>(g.getScale());
    const float rOut = R + 0.8f;    // foot ring outer edge
    const float rGrip = R - 1.6f;   // knurled grip outer edge
    const float rCham = 0.72f * R;  // chamfer outer edge
    const float rTop = 0.54f * R;   // flat top

    // Soft contact shadow and a longer drop shadow towards the lower right.
    shadeBox(g, cx - R - 6, cy - R - 6, cx + R + 10, cy + R + 12, [&](float x, float y) {
        const float d0 = std::hypot(x - cx, y - cy);
        const float d1 = std::hypot(x - cx - 2.2f, y - cy - 3.2f);
        const float a = 0.30f * (1.f - smoothstep(R - 1.f, R + 3.f, d0))
                        + 0.34f * (1.f - smoothstep(R - 4.f, R + 6.f, d1));
        return d0 < rOut - 0.5f ? 0u : toArgb({ 0.05f, 0.05f, 0.04f }, std::min(a, 0.6f));
    });

    const bool hasCap = capColor != 0;
    const Color cap = fromArgb(capColor);
    const Material foot{ { 0.15f, 0.15f, 0.16f }, 0.55f, 0.6f, 0.35f, 16.f };
    const Material grip{ { 0.62f, 0.63f, 0.65f }, 0.32f, 0.78f, 0.50f, 14.f };
    const Material chamfer = hasCap ? Material{ cap, 0.38f, 0.72f, 0.40f, 30.f }
                                    : Material{ { 0.78f, 0.79f, 0.80f }, 0.30f, 0.75f, 0.55f, 28.f };
    const Material top = hasCap ? Material{ cap, 0.40f, 0.68f, 0.30f, 40.f }
                                : Material{ { 0.80f, 0.81f, 0.82f }, 0.34f, 0.70f, 0.40f, 22.f };
    const float px = std::cos(pointer), py = std::sin(pointer);

    shadeBox(g, cx - rOut - 1, cy - rOut - 1, cx + rOut + 1, cy + rOut + 1, [&](float x, float y) {
        const float dx = x - cx, dy = y - cy;
        const float d = std::sqrt(dx * dx + dy * dy);
        const float cov = clamp01((rOut - d) * s + 0.5f);
        if (cov <= 0.f) return 0u;
        const float ux = d > 1e-4f ? dx / d : 0.f, uy = d > 1e-4f ? dy / d : 0.f;
        const float tx = -uy, ty = ux;               // tangent, clockwise on screen
        const float along = dx * px + dy * py;       // along the pointer
        const float perp = dx * -py + dy * px;       // across it
        const float theta = std::atan2(dy, dx);

        Color c;
        if (d > rGrip) {
            // Foot ring: steep, dark, with a bright rim on the lit side.
            const float t = (d - rGrip) / (rOut - rGrip);
            c = shadeLit(foot, tiltedNormal(ux, uy, 0.6f + 0.9f * t));
        } else if (d > rCham) {
            // Knurled grip: a steep cone with ridges that turn with the knob.
            const float t = (d - rCham) / (rGrip - rCham);
            const float phase = kGripRidges * (theta - pointer);
            Vec3 n = tiltedNormal(ux, uy, 0.95f + 0.25f * t);
            Material m = grip;
            if (along > 0.f && std::fabs(perp) < 2.2f) {
                // The pointer nub: a smooth raised rib in place of the ridges.
                const float q = perp / 2.2f;
                n = normalize({ n.x * 0.6f - tx * q * 1.2f, n.y * 0.6f - ty * q * 1.2f, n.z });
                m.albedo = { 0.86f, 0.87f, 0.88f };
                m.specular = 0.8f;
            } else {
                const float ridge = std::sin(phase);
                n = normalize({ n.x + tx * 0.75f * ridge, n.y + ty * 0.75f * ridge, n.z });
                m.albedo = scale(m.albedo, 0.5f + 0.5f * (0.5f + 0.5f * std::cos(phase)));
            }
            c = shadeLit(m, n);
            // Dirt settles where the grip meets the top.
            c = scale(c, 0.75f + 0.25f * smoothstep(0.f, 0.25f, t));
        } else if (d > rTop) {
            // Polished chamfer.
            const float t = (d - rTop) / (rCham - rTop);
            c = shadeLit(chamfer, tiltedNormal(ux, uy, 0.42f + 0.18f * t));
        } else {
            // Top: barely domed; on the metal caps, spun-metal streaks
            // towards and away from the light.
            Material m = top;
            if (!hasCap) {
                const float spun = std::cos(2.f * (theta - kLightAzimuth));
                const float rings = hash2(static_cast<int>(d * s * 1.5f), 0, 11) - 0.5f;
                m.albedo = scale(m.albedo, 1.f + 0.07f * spun + 0.03f * rings);
            }
            c = shadeLit(m, tiltedNormal(ux, uy, 0.12f * d / rTop));
        }

        // Incised pointer line across the top and chamfer: dark, with its
        // far wall catching the light.
        if (d < rCham - 0.4f && along > 0.12f * R) {
            const float w = 0.85f;
            const float core = clamp01((w - std::fabs(perp)) * s + 0.5f);
            if (core > 0.f) {
                const Vec3 wall = tiltedNormal(perp > 0 ? -py : py, perp > 0 ? px : -px, 0.9f);
                const float lit = std::max(0.f, dot(wall, kLight));
                const Color groove = hasCap ? scale(cap, 0.25f) : Color{ 0.10f, 0.10f, 0.11f };
                const Color line = mix(groove, Color{ 0.55f, 0.56f, 0.57f }, 0.35f * lit * smoothstep(0.2f, w, std::fabs(perp)));
                c = mix(c, line, core);
            }
        }
        // A little grime and wear everywhere.
        c = scale(c, 0.97f + 0.06f * (valueNoise(x * 1.7f, y * 1.7f, 21) - 0.5f));
        return toArgb(c, cov);
    });
}

// --- Waveform switch ---------------------------------------------------------

void drawSwitchBody(Graphics& g, float cx, float cy, bool isSquare) {
    const float s = static_cast<float>(g.getScale());
    // Shadow under the escutcheon plate.
    shadeBox(g, cx - 12, cy - 25, cx + 14, cy + 28, [&](float x, float y) {
        const float d = sdRoundRect(x - cx - 1.5f, y - cy - 2.2f, 8.5f, 21.f, 3.f);
        const float a = 0.38f * (1.f - smoothstep(-2.f, 4.f, d));
        return a > 0.f ? toArgb({ 0.05f, 0.05f, 0.04f }, a) : 0u;
    });

    const Material plate{ { 0.74f, 0.75f, 0.76f }, 0.35f, 0.70f, 0.45f, 20.f };
    const Material slot{ { 0.07f, 0.07f, 0.08f }, 0.5f, 0.7f, 0.2f, 10.f };
    const Material lever{ { 0.86f, 0.87f, 0.88f }, 0.30f, 0.72f, 0.55f, 26.f };
    const float leverY = cy + (isSquare ? 9.f : -9.f);

    shadeBox(g, cx - 10, cy - 23, cx + 10, cy + 23, [&](float x, float y) {
        const float lx = x - cx, ly = y - cy;
        const float dPlate = sdRoundRect(lx, ly, 8.5f, 21.f, 2.5f);
        const float cov = clamp01(-dPlate * s + 0.5f);
        if (cov <= 0.f) return 0u;
        float ux, uy;
        Color c;
        const float dSlot = sdRoundRect(lx, ly, 4.f, 17.f, 2.f);
        if (dSlot > 0.f) {
            // Plate with a bevelled edge.
            sdRoundRectDir(lx, ly, 8.5f, 21.f, 2.5f, ux, uy);
            const float bevel = smoothstep(-1.6f, 0.f, dPlate);
            c = shadeLit(plate, tiltedNormal(ux, uy, 0.9f * bevel));
            c = scale(c, 1.f + 0.03f * (valueNoise(x * 0.2f, y * 6.f, 31) - 0.5f));
            // The slot's lip.
            c = scale(c, 0.6f + 0.4f * smoothstep(0.f, 1.f, dSlot));
        } else {
            // Recess: walls facing the light are lit, the rest in shadow.
            sdRoundRectDir(lx, ly, 4.f, 17.f, 2.f, ux, uy);
            const float wall = smoothstep(-1.8f, 0.f, dSlot);
            c = shadeLit(slot, tiltedNormal(-ux, -uy, 1.1f * wall));
        }
        // Lever shadow in the slot, then the lever itself.
        const float dShadow = sdRoundRect(lx - 1.2f, y - leverY - 1.8f, 6.5f, 6.5f, 2.f);
        c = scale(c, 1.f - 0.55f * (1.f - smoothstep(-1.5f, 2.5f, dShadow)));
        const float dLever = sdRoundRect(lx, y - leverY, 6.5f, 6.5f, 2.f);
        const float lcov = clamp01(-dLever * s + 0.5f);
        if (lcov > 0.f) {
            sdRoundRectDir(lx, y - leverY, 6.5f, 6.5f, 2.f, ux, uy);
            const float bevel = smoothstep(-2.2f, 0.f, dLever);
            Vec3 n = tiltedNormal(ux, uy, 0.8f * bevel);
            // Three grip grooves across the lever's face.
            const float gy = y - leverY;
            if (bevel <= 0.f && std::fabs(gy) < 4.5f) {
                const float ridge = std::sin(gy * kPi / 1.5f);
                n = normalize({ n.x, n.y + 0.5f * ridge, n.z });
            }
            c = mix(c, shadeLit(lever, n), lcov);
        }
        return toArgb(c, cov);
    });
}

constexpr int kLabelGap = 26;   // label cap top above the control's top edge

class ModernSkin : public IGuiSkin {
public:
    void drawPanel(Graphics& g, int width, int height, int dividerX) override {
        // The texture is costly and never changes: paint it once, then copy.
        const size_t size = static_cast<size_t>(width) * height * g.getScale() * g.getScale();
        if (panelCache_.size() != size || cacheW_ != width || cacheDivider_ != dividerX) {
            paintPanel(g, width, height, dividerX);
            sprites_.clear();
            panelCache_.assign(g.getBuffer(), g.getBuffer() + size);
            cacheW_ = width;
            cacheDivider_ = dividerX;
            return;
        }
        std::memcpy(g.getBuffer(), panelCache_.data(), size * sizeof(uint32_t));
    }

    void drawKnob(Graphics& g, const Control& ctrl, const Font&) override {
        const float cx = ctrl.x + 0.5f, cy = ctrl.y + 0.5f, r = static_cast<float>(ctrl.radius);
        const float labelTop = static_cast<float>(ctrl.y - ctrl.radius - kLabelGap);
        // Labels are not cached: they can be wider than the knob and the
        // knobs sit close, so the cached areas cover only dial and shadow.
        drawLabel(g, ctrl.label, cx, labelTop, kInk);
        if (restore(g, ctrl, cx - r - 11.f, cy - r - 15.f, cx + r + 11.f, cy + r + 14.f)) return;
        double norm = (ctrl.currentVal - ctrl.minVal) / (ctrl.maxVal - ctrl.minVal);
        norm = std::isfinite(norm) ? std::min(std::max(norm, 0.0), 1.0) : 0.0;
        drawKnobDial(g, cx, cy, r);
        drawKnobBody(g, cx, cy, r, kStartAngle + static_cast<float>(norm) * kTotalAngle, ctrl.accentColor);
        store(g);
    }

    void drawToggleSwitch(Graphics& g, const Control& ctrl, const Font&) override {
        const float cx = ctrl.x + 0.5f, cy = ctrl.y + 0.5f;
        const float labelTop = static_cast<float>(ctrl.y - 20 - kLabelGap);
        drawLabel(g, ctrl.label, cx, labelTop, kInk);
        if (restore(g, ctrl, cx - 32.f, cy - 26.f, cx + 32.f, cy + 29.f)) return;
        const bool isSquare = ctrl.currentVal >= 0.5;
        drawSwitchBody(g, cx, cy, isSquare);

        // Waveform symbols beside the two positions, the active one in
        // dark acid green.
        const uint32_t active = 0xFF157A1C, dim = 0xC0707478;
        const uint32_t saw = isSquare ? dim : active, sq = isSquare ? active : dim;
        const float ix = cx - 21.f, sy = cy - 9.f, qy = cy + 9.f;
        // As printed on the 303: the saw rises straight up and ramps down,
        // twice; the pulse is two humps on a baseline.
        const float sawPts[][2] = { { -7, 4 }, { -7, -4 }, { 0, 4 }, { 0, -4 }, { 7, 4 } };
        const float sqPts[][2] = { { -8, 4 }, { -6, 4 }, { -6, -4 }, { -2, -4 }, { -2, 4 }, { 2, 4 },
                                   { 2, -4 }, { 6, -4 }, { 6, 4 }, { 8, 4 } };
        for (int i = 0; i + 1 < 5; ++i)
            drawLineAA(g, ix + sawPts[i][0], sy + sawPts[i][1], ix + sawPts[i + 1][0], sy + sawPts[i + 1][1], 1.3f, saw);
        for (int i = 0; i + 1 < 10; ++i)
            drawLineAA(g, ix + sqPts[i][0], qy + sqPts[i][1], ix + sqPts[i + 1][0], qy + sqPts[i + 1][1], 1.3f, sq);
        store(g);
    }

private:
    // Shading a knob costs far more than copying it, and while one knob is
    // dragged the others redraw unchanged. So each control's finished
    // pixels are kept, keyed by its value, and copied back on a repeat. The
    // area is the control's own (dial and shadow, not overlapping its
    // neighbours'), over the cached panel, so it is the same every time
    // the value is.
    struct Sprite {
        int id{-1};
        double value{0.0};
        int bx{0}, by{0}, bw{0}, bh{0};
        std::vector<uint32_t> pixels;
    };
    std::vector<Sprite> sprites_;
    Sprite* pending_{nullptr};

    bool restore(Graphics& g, const Control& ctrl, float x0, float y0, float x1, float y1) {
        const int s = g.getScale();
        const int bufW = static_cast<int>(g.getWidth()) * s, bufH = static_cast<int>(g.getHeight()) * s;
        const int bx = std::max(0, static_cast<int>(std::floor(x0 * s)));
        const int by = std::max(0, static_cast<int>(std::floor(y0 * s)));
        const int bw = std::min(bufW, static_cast<int>(std::ceil(x1 * s))) - bx;
        const int bh = std::min(bufH, static_cast<int>(std::ceil(y1 * s))) - by;
        pending_ = nullptr;
        if (bw <= 0 || bh <= 0) return false;
        Sprite* sp = nullptr;
        for (auto& e : sprites_) if (e.id == ctrl.id) sp = &e;
        if (!sp) { sprites_.emplace_back(); sp = &sprites_.back(); sp->id = ctrl.id; }
        uint32_t* buf = g.getBuffer();
        if (sp->value == ctrl.currentVal && sp->bx == bx && sp->by == by && sp->bw == bw && sp->bh == bh
            && sp->pixels.size() == static_cast<size_t>(bw) * bh) {
            for (int y = 0; y < bh; ++y) {
                std::memcpy(buf + static_cast<size_t>(by + y) * bufW + bx, sp->pixels.data() + static_cast<size_t>(y) * bw,
                            bw * sizeof(uint32_t));
            }
            return true;
        }
        sp->value = ctrl.currentVal;
        sp->bx = bx; sp->by = by; sp->bw = bw; sp->bh = bh;
        sp->pixels.clear();
        pending_ = sp;
        return false;
    }

    void store(Graphics& g) {
        if (!pending_) return;
        const int bufW = static_cast<int>(g.getWidth()) * g.getScale();
        Sprite& sp = *pending_;
        sp.pixels.resize(static_cast<size_t>(sp.bw) * sp.bh);
        for (int y = 0; y < sp.bh; ++y) {
            std::memcpy(sp.pixels.data() + static_cast<size_t>(y) * sp.bw,
                        g.getBuffer() + static_cast<size_t>(sp.by + y) * bufW + sp.bx, sp.bw * sizeof(uint32_t));
        }
        pending_ = nullptr;
    }

    std::vector<uint32_t> panelCache_;
    int cacheW_{0}, cacheDivider_{0};
};

} // namespace

std::unique_ptr<IGuiSkin> createGuiSkin() {
    return std::make_unique<ModernSkin>();
}

} // namespace acidus
