// The modern skin (ACIDUS_GUI_STYLE=MODERN): a worn, silver-painted 303
// panel with knobs shaded per pixel -- dark foot ring, knurled grip with a
// pointer nub, a polished chamfer and a spun-metal top with an incised
// pointer line -- lit from the upper left, with soft drop shadows and
// anti-aliased lettering. Everything is procedural apart from the label
// glyphs (LabelFontData.hpp), so the cost over the retro skin is this code
// plus ~4 KB of glyph data.
//
// GuiWindow paints into a 2x supersampled buffer and box-filters it down;
// this skin shades at that buffer resolution (Graphics::blendPixel), with
// analytic edge coverage, so edges come out smooth after the downsample.
#include "gui/IGuiSkin.hpp"
#include "gui/Graphics.hpp"
#include "gui/Font.hpp"
#include "gui/GuiWindow.hpp"
#include "LabelFontData.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

namespace acidus {

namespace {

constexpr float kPi = 3.14159265358979f;

struct Vec3 { float x, y, z; };
struct Color { float r, g, b; };

inline float clamp01(float v) { return v < 0.f ? 0.f : (v > 1.f ? 1.f : v); }
inline float smoothstep(float e0, float e1, float x) {
    const float t = clamp01((x - e0) / (e1 - e0));
    return t * t * (3.f - 2.f * t);
}
inline Vec3 normalize(Vec3 v) {
    const float l = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    return { v.x / l, v.y / l, v.z / l };
}
inline float dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline Color mix(Color a, Color b, float t) {
    return { a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t };
}
inline Color scale(Color c, float k) { return { c.r * k, c.g * k, c.b * k }; }
inline Color fromArgb(uint32_t c) {
    return { ((c >> 16) & 0xFF) / 255.f, ((c >> 8) & 0xFF) / 255.f, (c & 0xFF) / 255.f };
}
inline uint32_t toArgb(Color c, float alpha) {
    auto ch = [](float v) { return static_cast<uint32_t>(clamp01(v) * 255.f + 0.5f); };
    return (ch(alpha) << 24) | (ch(c.r) << 16) | (ch(c.g) << 8) | ch(c.b);
}

// Deterministic hash noise, so the wear looks the same on every open.
inline uint32_t hashU(uint32_t x) {
    x ^= x >> 16; x *= 0x7FEB352Du; x ^= x >> 15; x *= 0x846CA68Bu; x ^= x >> 16;
    return x;
}
inline float hash2(int x, int y, uint32_t seed) {
    return (hashU(static_cast<uint32_t>(x) * 0x1F1F1F1Fu ^ hashU(static_cast<uint32_t>(y) + seed * 0x9E3779B9u)) & 0xFFFFFF)
           / 16777216.f;
}
float valueNoise(float x, float y, uint32_t seed) {
    const int xi = static_cast<int>(std::floor(x)), yi = static_cast<int>(std::floor(y));
    float fx = x - xi, fy = y - yi;
    fx = fx * fx * (3.f - 2.f * fx);
    fy = fy * fy * (3.f - 2.f * fy);
    const float a = hash2(xi, yi, seed), b = hash2(xi + 1, yi, seed);
    const float c = hash2(xi, yi + 1, seed), d = hash2(xi + 1, yi + 1, seed);
    return a + (b - a) * fx + (c - a) * fy + (a - b - c + d) * fx * fy;
}
float fbm(float x, float y, uint32_t seed) {
    return 0.5f * valueNoise(x, y, seed) + 0.3f * valueNoise(2.1f * x, 2.1f * y, seed + 1)
           + 0.2f * valueNoise(4.3f * x, 4.3f * y, seed + 2);
}

// One light, upper left and in front, for every shaded part.
const Vec3 kLight = normalize({ -0.45f, -0.62f, 0.66f });
const Vec3 kHalf = normalize({ kLight.x, kLight.y, kLight.z + 1.f });
const float kLightAzimuth = std::atan2(kLight.y, kLight.x);

struct Material {
    Color albedo;
    float ambient, diffuse, specular, shininess;
};

Color shadeLit(const Material& m, Vec3 n) {
    const float diff = std::max(0.f, dot(n, kLight));
    const float spec = std::pow(std::max(0.f, dot(n, kHalf)), m.shininess) * m.specular;
    const float k = m.ambient + m.diffuse * diff;
    return { m.albedo.r * k + spec, m.albedo.g * k + spec, m.albedo.b * k + spec };
}

// A surface tilted `tilt` radians away from the viewer towards (ux, uy).
inline Vec3 tiltedNormal(float ux, float uy, float tilt) {
    const float s = std::sin(tilt);
    return { ux * s, uy * s, std::cos(tilt) };
}

// Calls f(lx, ly) for every buffer pixel whose centre lies in the logical
// box, and blends the colour it returns (alpha 0 = skip).
template <class F>
void shadeBox(Graphics& g, float x0, float y0, float x1, float y1, F&& f) {
    const int s = g.getScale();
    const int bx0 = std::max(0, static_cast<int>(std::floor(x0 * s)));
    const int by0 = std::max(0, static_cast<int>(std::floor(y0 * s)));
    const int bx1 = std::min(static_cast<int>(g.getWidth()) * s, static_cast<int>(std::ceil(x1 * s)));
    const int by1 = std::min(static_cast<int>(g.getHeight()) * s, static_cast<int>(std::ceil(y1 * s)));
    for (int by = by0; by < by1; ++by) {
        const float ly = (by + 0.5f) / s;
        for (int bx = bx0; bx < bx1; ++bx) {
            const uint32_t c = f((bx + 0.5f) / s, ly);
            if (c >> 24) g.blendPixel(bx, by, c);
        }
    }
}

// Anti-aliased line with round caps, `width` logical pixels wide.
void drawLineAA(Graphics& g, float x0, float y0, float x1, float y1, float width, uint32_t argb) {
    const float hw = 0.5f * width;
    const float s = static_cast<float>(g.getScale());
    const float dx = x1 - x0, dy = y1 - y0;
    const float len2 = std::max(dx * dx + dy * dy, 1e-6f);
    const float a = ((argb >> 24) & 0xFF) / 255.f;
    const Color c = fromArgb(argb);
    shadeBox(g, std::min(x0, x1) - hw - 1, std::min(y0, y1) - hw - 1,
             std::max(x0, x1) + hw + 1, std::max(y0, y1) + hw + 1,
             [&](float x, float y) {
                 const float t = clamp01(((x - x0) * dx + (y - y0) * dy) / len2);
                 const float ex = x - (x0 + t * dx), ey = y - (y0 + t * dy);
                 const float cov = clamp01((hw - std::sqrt(ex * ex + ey * ey)) * s + 0.5f);
                 return cov > 0.f ? toArgb(c, a * cov) : 0u;
             });
}

// Signed distance to a rounded rectangle centred at the origin, half size
// (hx, hy), corner radius r: negative inside.
inline float sdRoundRect(float x, float y, float hx, float hy, float r) {
    const float qx = std::fabs(x) - hx + r, qy = std::fabs(y) - hy + r;
    const float ox = std::max(qx, 0.f), oy = std::max(qy, 0.f);
    return std::sqrt(ox * ox + oy * oy) + std::min(std::max(qx, qy), 0.f) - r;
}
// Outward unit direction of the rounded rectangle's distance field.
inline void sdRoundRectDir(float x, float y, float hx, float hy, float r, float& ux, float& uy) {
    const float e = 0.05f;
    ux = sdRoundRect(x + e, y, hx, hy, r) - sdRoundRect(x - e, y, hx, hy, r);
    uy = sdRoundRect(x, y + e, hx, hy, r) - sdRoundRect(x, y - e, hx, hy, r);
    const float l = std::sqrt(ux * ux + uy * uy);
    if (l > 1e-6f) { ux /= l; uy /= l; } else { ux = 0.f; uy = 0.f; }
}

// --- Lettering ---------------------------------------------------------------

const labelfont::Glyph* glyphFor(char c) {
    if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
    if (c < labelfont::kFirst || c > labelfont::kLast) c = ' ';
    return &labelfont::kGlyphs[c - labelfont::kFirst];
}

// Width in logical pixels. The glyphs are stored at 2x (the GUI's
// supersampling), so other buffer scales resample them.
float labelWidth(const char* text) {
    float w = 0.f;
    for (const char* p = text; *p; ++p) w += glyphFor(*p)->advance16 / 16.f;
    return w / 2.f;
}

// Silk-screen ink: anti-aliased glyphs, slightly patchy with age.
void drawLabel(Graphics& g, const char* text, float centerX, float capTop, uint32_t argb) {
    if (!text) return;
    const int s = g.getScale();
    const float k = s / 2.f;   // buffer pixels per glyph pixel
    const float a = ((argb >> 24) & 0xFF) / 255.f;
    const Color c = fromArgb(argb);
    float penX = std::round((centerX - labelWidth(text) / 2.f) * s);
    const float penY = std::round(capTop * s - labelfont::kCapTop * k);
    for (const char* p = text; *p; ++p) {
        const labelfont::Glyph* gl = glyphFor(*p);
        const int x0 = static_cast<int>(penX + gl->dx * k), y0 = static_cast<int>(penY + gl->dy * k);
        const int w = static_cast<int>(std::ceil(gl->w * k)), h = static_cast<int>(std::ceil(gl->h * k));
        for (int y = 0; y < h; ++y) {
            const int gy = std::min<int>(gl->h - 1, static_cast<int>(y / k));
            for (int x = 0; x < w; ++x) {
                const int gx = std::min<int>(gl->w - 1, static_cast<int>(x / k));
                const int i = gy * gl->w + gx;
                const uint8_t byte = labelfont::kAlpha[gl->offset + i / 2];
                const int nib = (i & 1) ? (byte & 0x0F) : (byte >> 4);
                if (!nib) continue;
                const float wear = 0.82f + 0.18f * valueNoise((x0 + x) * 0.35f, (y0 + y) * 0.35f, 77);
                g.blendPixel(x0 + x, y0 + y, toArgb(c, a * wear * nib / 15.f));
            }
        }
        penX += gl->advance16 / 16.f * k;
    }
}

// --- Panel -------------------------------------------------------------------

constexpr uint32_t kInk = 0xF0222326;   // printed panel ink
const Color kPaint{ 0.785f, 0.790f, 0.775f };   // aged silver paint

void paintPanel(Graphics& g, int width, int height, int dividerX) {
    const int s = g.getScale();
    const int bw = width * s, bh = height * s;
    uint32_t* buf = g.getBuffer();
    const Color grimeTint{ 0.60f, 0.56f, 0.47f };
    for (int by = 0; by < bh; ++by) {
        const float y = (by + 0.5f) / s;
        // Brushed grain runs along the panel: one random brightness per
        // buffer row, smeared a little along x.
        for (int bx = 0; bx < bw; ++bx) {
            const float x = (bx + 0.5f) / s;
            Color c = kPaint;
            float shade = 1.f;
            shade += 0.028f * (valueNoise(x * 0.015f, by * 0.9f, 3) - 0.5f);
            shade += 0.018f * (hash2(bx, by, 5) - 0.5f);
            // Big soft patches of grime, heavier towards the edges.
            const float edge = std::min(std::min(y, height - y) / 40.f, 1.f);
            const float grime = smoothstep(0.45f, 0.85f, fbm(x / 70.f, y / 45.f, 9)) * 0.55f
                                + (1.f - edge) * 0.35f;
            c = mix(c, grimeTint, 0.16f * grime);
            // Gentle vignette.
            const float vx = (x / width - 0.5f) * 2.f, vy = (y / height - 0.5f) * 2.f;
            shade *= 1.f - 0.06f * (vx * vx * 0.5f + vy * vy);
            // Top and bottom extrusions: a darker, rounded metal trim.
            if (y < 12.f || y > height - 13.f) {
                const float t = y < 12.f ? y / 12.f : (height - y) / 13.f;
                c = { 0.66f, 0.67f, 0.665f };
                shade = (0.82f + 0.22f * std::sin(t * kPi * 0.9f + 0.2f))
                        * (1.f + 0.03f * (valueNoise(x * 0.01f, by * 0.7f, 4) - 0.5f));
            }
            buf[static_cast<size_t>(by) * bw + bx] = toArgb(scale(c, shade), 1.f);
        }
    }
    // Trim edges: a dark seam and a lit lip where the trim meets the panel.
    drawLineAA(g, 0, 12.f, static_cast<float>(width), 12.f, 1.f, 0xC0303234);
    drawLineAA(g, 0, 13.f, static_cast<float>(width), 13.f, 1.f, 0xA0FFFFFF);
    drawLineAA(g, 0, height - 14.f, static_cast<float>(width), height - 14.f, 1.f, 0x80FFFFFF);
    drawLineAA(g, 0, height - 13.f, static_cast<float>(width), height - 13.f, 1.f, 0xC0303234);
    drawLineAA(g, 0, 0.5f, static_cast<float>(width), 0.5f, 1.f, 0x70FFFFFF);

    // Light scratches and scuffs from years of use.
    uint32_t seed = 0xAC1D;
    auto rnd = [&seed]() { seed = hashU(seed + 0x9E3779B9u); return (seed & 0xFFFFFF) / 16777216.f; };
    for (int i = 0; i < 40; ++i) {
        const float x = rnd() * dividerX, y = 16.f + rnd() * (height - 32.f);
        const float len = 3.f + 22.f * rnd() * rnd();
        const float ang = (rnd() - 0.5f) * 0.9f + (rnd() < 0.3f ? kPi / 2 : 0.f);
        const uint32_t col = rnd() < 0.7f ? 0x18FFFFFFu : 0x0E000000u;
        drawLineAA(g, x, y, x + len * std::cos(ang), y + len * std::sin(ang), 0.35f, col);
    }

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
        const float halfW = std::max(r + 11.f, ctrl.label ? labelWidth(ctrl.label) / 2.f + 2.f : 0.f);
        if (restore(g, ctrl, cx - halfW, labelTop - 2.f, cx + halfW, cy + r + 14.f)) return;
        double norm = (ctrl.currentVal - ctrl.minVal) / (ctrl.maxVal - ctrl.minVal);
        norm = std::isfinite(norm) ? std::min(std::max(norm, 0.0), 1.0) : 0.0;
        drawKnobDial(g, cx, cy, r);
        drawKnobBody(g, cx, cy, r, kStartAngle + static_cast<float>(norm) * kTotalAngle, ctrl.accentColor);
        drawLabel(g, ctrl.label, cx, labelTop, kInk);
        store(g);
    }

    void drawToggleSwitch(Graphics& g, const Control& ctrl, const Font&) override {
        const float cx = ctrl.x + 0.5f, cy = ctrl.y + 0.5f;
        const float labelTop = static_cast<float>(ctrl.y - 20 - kLabelGap);
        const float halfW = std::max(32.f, ctrl.label ? labelWidth(ctrl.label) / 2.f + 2.f : 0.f);
        if (restore(g, ctrl, cx - halfW, labelTop - 2.f, cx + halfW, cy + 29.f)) return;
        const bool isSquare = ctrl.currentVal >= 0.5;
        drawLabel(g, ctrl.label, cx, labelTop, kInk);
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
    // area is the control's own (dial, shadow, label), over the cached
    // panel, so it is the same every time the value is.
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
