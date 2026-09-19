#include "ControlRenderer.hpp"
#include "Graphics.hpp"
#include "Font.hpp"
#include "GuiWindow.hpp"
#include <cmath>
#include <algorithm>

namespace acidus {

void TB303ControlRenderer::drawKnob(Graphics& g, const Control& ctrl, const Font& font) {
    int cx = ctrl.x;
    int cy = ctrl.y;
    int r = ctrl.radius;

    g.fillCircle(cx + 2, cy + 3, r, 0xFF404448);
    g.fillCircle(cx, cy, r, 0xFF181B1D);
    g.drawCircle(cx, cy, r, 0xFF08090A, 1);
    g.drawCircle(cx, cy, r - 3, 0xFF2A2E32, 1);

    double minAngle = -135.0 * 3.14159265358979323846 / 180.0;
    double maxAngle =  135.0 * 3.14159265358979323846 / 180.0;
    double norm = (ctrl.currentVal - ctrl.minVal) / (ctrl.maxVal - ctrl.minVal);
    norm = std::min(std::max(norm, 0.0), 1.0);
    double angle = minAngle + norm * (maxAngle - minAngle);

    int pointerR1 = r - 8;
    int pointerR2 = r - 2;
    int px1 = cx + static_cast<int>(std::sin(angle) * pointerR1);
    int py1 = cy - static_cast<int>(std::cos(angle) * pointerR1);
    int px2 = cx + static_cast<int>(std::sin(angle) * pointerR2);
    int py2 = cy - static_cast<int>(std::cos(angle) * pointerR2);

    g.drawLine(px1, py1, px2, py2, 0xFFFFFFFF, 2);

    if (ctrl.label) {
        int textW = font.getTextWidth(ctrl.label);
        int textX = cx - textW / 2;
        int textY = cy - r - 18;
        g.drawText(font, ctrl.label, textX, textY, 0xFF101010);
    }
}

void TB303ControlRenderer::drawToggleSwitch(Graphics& g, const Control& ctrl, const Font& font) {
    int cx = ctrl.x;
    int cy = ctrl.y;

    int slotW = 10;
    int slotH = 26;
    int slotX = cx - slotW / 2;
    int slotY = cy - slotH / 2;

    g.drawRect(slotX, slotY, slotW, slotH, 0xFF101010);
    g.fillRect(slotX + 1, slotY + 1, slotW - 2, slotH - 2, 0xFF25282A);

    bool isSquare = (ctrl.currentVal >= 0.5);
    int handleW = 16;
    int handleH = 10;
    int handleX = cx - handleW / 2;
    int handleY = isSquare ? (cy + 2) : (cy - handleH - 2);

    g.fillRect(handleX + 1, handleY + 2, handleW, handleH, 0xFF404448);
    g.fillRect(handleX, handleY, handleW, handleH, 0xFFC0C4C8);
    g.drawRect(handleX, handleY, handleW, handleH, 0xFF101010);
    g.drawLine(handleX + 2, handleY + handleH / 2, handleX + handleW - 2, handleY + handleH / 2, 0xFF808488, 1);

    if (ctrl.label) {
        int textW = font.getTextWidth(ctrl.label);
        int textX = cx - textW / 2;
        int textY = cy - 28;
        g.drawText(font, ctrl.label, textX, textY, 0xFF101010);
    }

    g.drawText(font, "SAW", cx - 12, cy - 18, 0xFF202020);
    g.drawText(font, "SQR", cx - 12, cy + 12, 0xFF202020);
}

} // namespace acidus
