#include "IGuiSkin.hpp"
#include "Graphics.hpp"
#include "Font.hpp"
#include "GuiWindow.hpp"
#include <cmath>
#include <algorithm>

namespace acidus {

namespace {

// Syrebas-style panel: small classic 5x7 labels, silver fluted knobs with a
// conical top, a thin black pointer, 11 dial ticks and the 303's index
// square above the 12 o'clock tick -- plus a softer, deeper drop shadow.
constexpr int kLabelTextScale = 1;
constexpr int kLabelGap = 26;          // label baseline distance above the knob's top edge
constexpr uint32_t kInk = 0xFF16181A;  // label / tick / pointer colour

void drawLabel(Graphics& g, const Font& font, const Control& ctrl, int top) {
    if (!ctrl.label) return;
    int textW = font.getTextWidth(ctrl.label, kLabelTextScale);
    g.drawText(font, ctrl.label, ctrl.x - textW / 2, top, kInk, kLabelTextScale);
}

// The 303 panel's saw: a vertical rise, then a ramp down, twice.
void drawSawIcon(Graphics& g, int cx, int cy, uint32_t color) {
    g.drawLine(cx - 7, cy + 4, cx - 7, cy - 4, color, 1);
    g.drawLine(cx - 7, cy - 4, cx, cy + 4, color, 1);
    g.drawLine(cx, cy + 4, cx, cy - 4, color, 1);
    g.drawLine(cx, cy - 4, cx + 7, cy + 4, color, 1);
}

// The 303 panel's pulse: two humps on a baseline.
void drawSquareIcon(Graphics& g, int cx, int cy, uint32_t color) {
    g.drawLine(cx - 8, cy + 4, cx - 6, cy + 4, color, 1);
    g.drawLine(cx - 6, cy + 4, cx - 6, cy - 4, color, 1);
    g.drawLine(cx - 6, cy - 4, cx - 2, cy - 4, color, 1);
    g.drawLine(cx - 2, cy - 4, cx - 2, cy + 4, color, 1);
    g.drawLine(cx - 2, cy + 4, cx + 2, cy + 4, color, 1);
    g.drawLine(cx + 2, cy + 4, cx + 2, cy - 4, color, 1);
    g.drawLine(cx + 2, cy - 4, cx + 6, cy - 4, color, 1);
    g.drawLine(cx + 6, cy - 4, cx + 6, cy + 4, color, 1);
    g.drawLine(cx + 6, cy + 4, cx + 8, cy + 4, color, 1);
}

// Soft drop shadow: several translucent discs, offset down-right, so the
// knob reads as standing off the panel.
void drawShadow(Graphics& g, int cx, int cy, int r) {
    g.fillCircle(cx + 5, cy + 6, r + 2, 0x14000000);
    g.fillCircle(cx + 4, cy + 5, r + 1, 0x22000000);
    g.fillCircle(cx + 3, cy + 4, r, 0x30000000);
    g.fillCircle(cx + 2, cy + 2, r, 0x40000000);
}

constexpr double kPi = 3.14159265358979323846;

// The retro skin: flat-shaded silver panel and knobs drawn from circles and
// lines, labels in the 5x7 pixel font. The smallest build.
class RetroSkin : public IGuiSkin {
public:
    void drawPanel(Graphics& g, int width, int height, int dividerX) override;
    void drawKnob(Graphics& g, const Control& ctrl, const Font& font) override;
    void drawToggleSwitch(Graphics& g, const Control& ctrl, const Font& font) override;
};

} // namespace

std::unique_ptr<IGuiSkin> createGuiSkin() {
    return std::make_unique<RetroSkin>();
}

void RetroSkin::drawPanel(Graphics& g, int width, int height, int dividerX) {
    // Brushed silver panel background
    g.clear(0xFFDBDFE1);

    // Top & Bottom metallic borders / trims
    g.drawRect(0, 0, width, 12, 0xFFC0C4C8);
    g.drawLine(0, 12, width, 12, 0xFF808488, 1);
    g.drawLine(0, 13, width, 13, 0xFFFFFFFF, 1);

    g.drawLine(0, height - 14, width, height - 14, 0xFF808488, 1);
    g.drawRect(0, height - 13, width, 13, 0xFFC0C4C8);

    // Vertical dividing line separating controls from right title panel
    g.drawLine(dividerX, 14, dividerX, height - 14, 0xFF181818, 2);
}

void RetroSkin::drawKnob(Graphics& g, const Control& ctrl, const Font& font) {
    const int cx = ctrl.x;
    const int cy = ctrl.y;
    const int r = ctrl.radius;

    // Dial: 11 ticks on the clock hours from 7 o'clock to 5 o'clock (a
    // 300 degree sweep, as on the 303), and the index square.
    const double startAngle = 120.0 * kPi / 180.0;
    const double totalAngle = 300.0 * kPi / 180.0;
    for (int i = 0; i < 11; ++i) {
        double a = startAngle + totalAngle * i / 10.0;
        int x1 = cx + static_cast<int>(std::lround(std::cos(a) * (r + 4)));
        int y1 = cy + static_cast<int>(std::lround(std::sin(a) * (r + 4)));
        int x2 = cx + static_cast<int>(std::lround(std::cos(a) * (r + 8)));
        int y2 = cy + static_cast<int>(std::lround(std::sin(a) * (r + 8)));
        g.drawLine(x1, y1, x2, y2, kInk, 1);
    }
    g.fillRect(cx - 2, cy - r - 14, 4, 4, kInk);

    drawShadow(g, cx, cy, r);

    // Skirt: dark bezel, fluted silver ring.
    g.fillCircle(cx, cy, r + 1, 0xFF26292C);
    g.fillCircle(cx, cy, r, 0xFF7C8288);
    g.fillCircle(cx, cy, r - 1, 0xFFAEB4BA);
    for (int deg = 0; deg < 360; deg += 30) {
        double a = deg * kPi / 180.0;
        int x1 = cx + static_cast<int>(std::lround(std::cos(a) * (r - 5)));
        int y1 = cy + static_cast<int>(std::lround(std::sin(a) * (r - 5)));
        int x2 = cx + static_cast<int>(std::lround(std::cos(a) * (r - 1)));
        int y2 = cy + static_cast<int>(std::lround(std::sin(a) * (r - 1)));
        g.drawLine(x1, y1, x2, y2, 0xFF686E74, 1);
    }

    // Conical top face, lit from the upper left: a darker lower-right rim
    // and a lighter upper-left sheen.
    const int faceR = r - 5;
    g.fillCircle(cx, cy, faceR, 0xFFB9BFC4);
    g.fillCircle(cx - 1, cy - 1, faceR - 1, 0xFFD2D7DB);
    g.fillCircle(cx - 2, cy - 2, faceR - 5, 0x30FFFFFF);
    g.drawCircle(cx, cy, faceR, 0xFF888E94, 1);

    // Optional coloured centre cap (Drive, Volume).
    if (ctrl.accentColor) {
        g.fillCircle(cx, cy, faceR / 2, ctrl.accentColor);
        g.drawCircle(cx, cy, faceR / 2, 0xFF303336, 1);
    }

    // Pointer from the centre to the skirt.
    double norm = (ctrl.currentVal - ctrl.minVal) / (ctrl.maxVal - ctrl.minVal);
    norm = std::min(std::max(norm, 0.0), 1.0);
    double a = startAngle + norm * totalAngle;
    int px = cx + static_cast<int>(std::lround(std::cos(a) * (r - 2)));
    int py = cy + static_cast<int>(std::lround(std::sin(a) * (r - 2)));
    g.drawLine(cx, cy, px, py, kInk, 2);

    drawLabel(g, font, ctrl, cy - r - kLabelGap);
}

void RetroSkin::drawToggleSwitch(Graphics& g, const Control& ctrl, const Font& font) {
    const int cx = ctrl.x;
    const int cy = ctrl.y;
    bool isSquare = (ctrl.currentVal >= 0.5);

    drawLabel(g, font, ctrl, cy - 20 - kLabelGap);

    // Recessed slot with a metal frame, casting the same soft shadow.
    g.fillRect(cx - 7, cy - 18, 18, 42, 0x22000000);
    g.fillRect(cx - 8, cy - 19, 17, 41, 0x30000000);
    g.fillRect(cx - 8, cy - 20, 16, 40, 0xFF26292C);
    g.fillRect(cx - 7, cy - 19, 14, 38, 0xFF8A9096);
    g.fillRect(cx - 5, cy - 17, 10, 34, 0xFF1A1C1E);

    // Handle: saw up, square down (matches the icons beside it).
    int hy = isSquare ? (cy + 2) : (cy - 16);
    g.fillRect(cx - 6, hy + 2, 14, 14, 0x40000000);
    g.fillRect(cx - 7, hy, 14, 14, 0xFF2E3134);
    g.fillRect(cx - 6, hy + 1, 12, 12, 0xFFE2E6EA);
    g.fillRect(cx - 4, hy + 3, 8, 8, 0xFFB4BAC0);
    g.drawLine(cx - 5, hy + 7, cx + 5, hy + 7, kInk, 1);

    // Waveform symbols beside the two positions; the active one in dark
    // acid green, the other dim.
    const uint32_t activeColor = 0xFF157A1C;
    const uint32_t dimColor = 0xFF8A8E92;
    drawSawIcon(g, cx - 20, cy - 9, isSquare ? dimColor : activeColor);
    drawSquareIcon(g, cx - 20, cy + 9, isSquare ? activeColor : dimColor);
}

} // namespace acidus
