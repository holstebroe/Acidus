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

    // Knob body: a lighter graphite bezel with a darker inset face, so the
    // shadow and the body no longer sit at nearly the same near-black tone.
    uint32_t bodyOuter = ctrl.accentColor ? ctrl.accentColor : 0xFF3A3E42;
    uint32_t bodyInner = ctrl.accentColor ? darken(ctrl.accentColor, 60) : 0xFF222629;

    g.fillCircle(cx, cy, r, bodyOuter);
    g.fillCircle(cx, cy, r - 4, bodyInner);

    g.drawCircle(cx, cy, r, 0xFF08090A, 1);
    g.drawCircle(cx, cy, r - 4, 0xFF5A5E62, 1);

    // Specular highlight, upper-left, to sell the rounded metal/plastic cap.
    g.fillCircle(cx - r / 3, cy - r / 3, r / 4, 0x30FFFFFF);

    double minAngle = -135.0 * 3.14159265358979323846 / 180.0;
    double maxAngle =  135.0 * 3.14159265358979323846 / 180.0;
    double norm = (ctrl.currentVal - ctrl.minVal) / (ctrl.maxVal - ctrl.minVal);
    norm = std::min(std::max(norm, 0.0), 1.0);
    double angle = minAngle + norm * (maxAngle - minAngle);

    int pointerR1 = r - 9;
    int pointerR2 = r - 2;
    int px1 = cx + static_cast<int>(std::sin(angle) * pointerR1);
    int py1 = cy - static_cast<int>(std::cos(angle) * pointerR1);
    int px2 = cx + static_cast<int>(std::sin(angle) * pointerR2);
    int py2 = cy - static_cast<int>(std::cos(angle) * pointerR2);

    g.drawLine(px1, py1, px2, py2, 0xFFF2F2F2, 2);
    g.fillCircle(px2, py2, 2, 0xFF39FF14);

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

    // Symbolic saw/square icons flanking the switch; the active waveform
    // lights up in acid green, the inactive one stays dim.
    uint32_t activeColor = 0xFF39FF14;
    uint32_t dimColor = 0xFF8A8E92;

    int sawY = cy - slotH / 2 - 12;
    int sqrY = cy + slotH / 2 + 12;

    drawSawIcon(g, cx, sawY, isSquare ? dimColor : activeColor);
    drawSquareIcon(g, cx, sqrY, isSquare ? activeColor : dimColor);
}

} // namespace acidus
