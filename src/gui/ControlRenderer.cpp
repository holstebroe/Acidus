#include "ControlRenderer.hpp"
#include "Graphics.hpp"
#include "Font.hpp"
#include "GuiWindow.hpp"
#include <cmath>
#include <algorithm>

namespace acidus {

namespace {

constexpr int kLabelTextScale = 2;
constexpr int kLabelYOffset = 58; // distance above the control's y-center

void drawLabel(Graphics& g, const Font& font, const Control& ctrl) {
    if (!ctrl.label) return;
    int textW = font.getTextWidth(ctrl.label, kLabelTextScale);
    int textX = ctrl.x - textW / 2;
    int textY = ctrl.y - kLabelYOffset;
    g.drawText(font, ctrl.label, textX, textY, 0xFF101010, kLabelTextScale);
}

// A rising ramp with a sharp vertical drop, drawn twice across the icon's width.
void drawSawIcon(Graphics& g, int cx, int cy, uint32_t color) {
    g.drawLine(cx - 8, cy + 6, cx, cy - 6, color, 2);
    g.drawLine(cx, cy - 6, cx, cy + 6, color, 2);
    g.drawLine(cx, cy + 6, cx + 8, cy - 6, color, 2);
}

// A square wave: high, drop, low, rise, high.
void drawSquareIcon(Graphics& g, int cx, int cy, uint32_t color) {
    g.drawLine(cx - 8, cy - 6, cx - 3, cy - 6, color, 2);
    g.drawLine(cx - 3, cy - 6, cx - 3, cy + 6, color, 2);
    g.drawLine(cx - 3, cy + 6, cx + 3, cy + 6, color, 2);
    g.drawLine(cx + 3, cy + 6, cx + 3, cy - 6, color, 2);
    g.drawLine(cx + 3, cy - 6, cx + 8, cy - 6, color, 2);
}

// Darkens an opaque 0xAARRGGBB color to `percent`/100 of its original brightness.
uint32_t darken(uint32_t color, int percent) {
    uint32_t r = ((color >> 16) & 0xFF) * percent / 100;
    uint32_t gr = ((color >> 8) & 0xFF) * percent / 100;
    uint32_t b = (color & 0xFF) * percent / 100;
    return 0xFF000000 | (r << 16) | (gr << 8) | b;
}

constexpr double kPi = 3.14159265358979323846;

} // namespace

void TB303ControlRenderer::drawKnob(Graphics& g, const Control& ctrl, const Font& font) {
    int cx = ctrl.x;
    int cy = ctrl.y;
    int r = ctrl.radius;

    // Soft, layered drop shadow - offsets decrease and alpha builds up so the
    // shadow reads as a smooth dark crescent rather than a flat hard-edged disc.
    g.fillCircle(cx + 4, cy + 5, r, 0x30000000);
    g.fillCircle(cx + 3, cy + 4, r, 0x50000000);
    g.fillCircle(cx + 2, cy + 3, r, 0x90000000);

    // A flat matte top (no dome/specular highlight) ringed by a knurled grip,
    // taking a cue from the 303's grooved knobs without copying them outright.
    uint32_t faceColor = ctrl.accentColor ? ctrl.accentColor : 0xFF34383C;
    uint32_t rimColor = darken(faceColor, 55);
    uint32_t notchColor = 0xFF6E7276;
    int faceR = r - 6;

    g.fillCircle(cx, cy, r, rimColor);
    const int notches = 24;
    for (int i = 0; i < notches; ++i) {
        double a = (2.0 * kPi * i) / notches;
        int x1 = cx + static_cast<int>(std::round(std::sin(a) * r));
        int y1 = cy - static_cast<int>(std::round(std::cos(a) * r));
        int x2 = cx + static_cast<int>(std::round(std::sin(a) * (faceR + 1)));
        int y2 = cy - static_cast<int>(std::round(std::cos(a) * (faceR + 1)));
        g.drawLine(x1, y1, x2, y2, notchColor, 1);
    }
    g.drawCircle(cx, cy, r, 0xFF08090A, 1);

    g.fillCircle(cx, cy, faceR, faceColor);
    g.drawCircle(cx, cy, faceR, 0xFF08090A, 1);

    double minAngle = -135.0 * kPi / 180.0;
    double maxAngle =  135.0 * kPi / 180.0;
    double norm = (ctrl.currentVal - ctrl.minVal) / (ctrl.maxVal - ctrl.minVal);
    norm = std::min(std::max(norm, 0.0), 1.0);
    double angle = minAngle + norm * (maxAngle - minAngle);

    int pointerR1 = faceR - 9;
    int pointerR2 = faceR - 2;
    int px1 = cx + static_cast<int>(std::sin(angle) * pointerR1);
    int py1 = cy - static_cast<int>(std::cos(angle) * pointerR1);
    int px2 = cx + static_cast<int>(std::sin(angle) * pointerR2);
    int py2 = cy - static_cast<int>(std::cos(angle) * pointerR2);

    g.drawLine(px1, py1, px2, py2, 0xFFF2F2F2, 2);

    drawLabel(g, font, ctrl);
}

void TB303ControlRenderer::drawToggleSwitch(Graphics& g, const Control& ctrl, const Font& font) {
    int cx = ctrl.x;
    int cy = ctrl.y;

    drawLabel(g, font, ctrl);

    bool isSquare = (ctrl.currentVal >= 0.5);

    int slotW = 12;
    int slotH = 34;
    int slotX = cx - slotW / 2;
    int slotY = cy - slotH / 2;

    // Soft drop shadow, matching the knobs' shadow language.
    g.fillRect(slotX + 2, slotY + 3, slotW, slotH, 0x40000000);
    g.fillRect(slotX + 1, slotY + 2, slotW, slotH, 0x70000000);

    g.fillRect(slotX, slotY, slotW, slotH, 0xFF1E2124);
    g.drawRect(slotX, slotY, slotW, slotH, 0xFF08090A);

    int handleW = 22;
    int handleH = 13;
    int handleX = cx - handleW / 2;
    int handleY = isSquare ? (cy + 3) : (cy - handleH - 3);

    g.fillRect(handleX + 1, handleY + 2, handleW, handleH, 0x60000000);
    g.fillRect(handleX, handleY, handleW, handleH, 0xFFD4D8DA);
    g.drawRect(handleX, handleY, handleW, handleH, 0xFF08090A);
    g.drawLine(handleX + 2, handleY + handleH / 2, handleX + handleW - 2, handleY + handleH / 2, 0xFF8A8E92, 1);

    // Symbolic saw/square icons flanking the switch; the active waveform is
    // drawn in a dark, saturated green (bright acid green washes out against
    // the light panel), the inactive one stays dim.
    uint32_t activeColor = 0xFF157A1C;
    uint32_t dimColor = 0xFF8A8E92;

    int sawY = cy - slotH / 2 - 12;
    int sqrY = cy + slotH / 2 + 12;

    drawSawIcon(g, cx, sawY, isSquare ? dimColor : activeColor);
    drawSquareIcon(g, cx, sqrY, isSquare ? activeColor : dimColor);
}

} // namespace acidus
