#include "GuiWindow.hpp"
#include "clap/SyrebasClap.hpp"
#include <cmath>
#include <cstring>
#include <algorithm>
#include <iostream>

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

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace syrebas {

// 5x7 ASCII bitmap font (characters 32 to 95)
static const uint8_t font5x7[64][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // ' ' (32)
    {0x00, 0x00, 0x5f, 0x00, 0x00}, // '!'
    {0x00, 0x07, 0x00, 0x07, 0x00}, // '"'
    {0x14, 0x7f, 0x14, 0x7f, 0x14}, // '#'
    {0x24, 0x2a, 0x7f, 0x2a, 0x12}, // '$'
    {0x23, 0x13, 0x08, 0x64, 0x62}, // '%'
    {0x36, 0x49, 0x55, 0x22, 0x50}, // '&'
    {0x00, 0x05, 0x03, 0x00, 0x00}, // '\''
    {0x00, 0x1c, 0x22, 0x41, 0x00}, // '('
    {0x00, 0x41, 0x22, 0x1c, 0x00}, // ')'
    {0x14, 0x08, 0x3e, 0x08, 0x14}, // '*'
    {0x08, 0x08, 0x3e, 0x08, 0x08}, // '+'
    {0x00, 0x50, 0x30, 0x00, 0x00}, // ','
    {0x08, 0x08, 0x08, 0x08, 0x08}, // '-'
    {0x00, 0x60, 0x60, 0x00, 0x00}, // '.'
    {0x20, 0x10, 0x08, 0x04, 0x02}, // '/'
    {0x3e, 0x51, 0x49, 0x45, 0x3e}, // '0'
    {0x00, 0x42, 0x7f, 0x40, 0x00}, // '1'
    {0x42, 0x61, 0x51, 0x49, 0x46}, // '2'
    {0x21, 0x41, 0x45, 0x4b, 0x31}, // '3'
    {0x18, 0x14, 0x12, 0x7f, 0x10}, // '4'
    {0x27, 0x45, 0x45, 0x45, 0x39}, // '5'
    {0x3c, 0x4a, 0x49, 0x49, 0x30}, // '6'
    {0x01, 0x71, 0x09, 0x05, 0x03}, // '7'
    {0x36, 0x49, 0x49, 0x49, 0x36}, // '8'
    {0x06, 0x49, 0x49, 0x29, 0x1e}, // '9'
    {0x00, 0x36, 0x36, 0x00, 0x00}, // ':'
    {0x00, 0x56, 0x36, 0x00, 0x00}, // ';'
    {0x08, 0x14, 0x22, 0x41, 0x00}, // '<'
    {0x14, 0x14, 0x14, 0x14, 0x14}, // '='
    {0x00, 0x41, 0x22, 0x14, 0x08}, // '>'
    {0x02, 0x01, 0x51, 0x09, 0x06}, // '?'
    {0x32, 0x49, 0x79, 0x41, 0x3e}, // '@'
    {0x7e, 0x11, 0x11, 0x11, 0x7e}, // 'A'
    {0x7f, 0x49, 0x49, 0x49, 0x36}, // 'B'
    {0x3e, 0x41, 0x41, 0x41, 0x22}, // 'C'
    {0x7f, 0x41, 0x41, 0x22, 0x1c}, // 'D'
    {0x7f, 0x49, 0x49, 0x49, 0x41}, // 'E'
    {0x7f, 0x09, 0x09, 0x09, 0x01}, // 'F'
    {0x3e, 0x41, 0x49, 0x49, 0x7a}, // 'G'
    {0x7f, 0x08, 0x08, 0x08, 0x7f}, // 'H'
    {0x00, 0x41, 0x7f, 0x41, 0x00}, // 'I'
    {0x20, 0x40, 0x41, 0x3f, 0x01}, // 'J'
    {0x7f, 0x08, 0x14, 0x22, 0x41}, // 'K'
    {0x7f, 0x40, 0x40, 0x40, 0x40}, // 'L'
    {0x7f, 0x02, 0x0c, 0x02, 0x7f}, // 'M'
    {0x7f, 0x04, 0x08, 0x10, 0x7f}, // 'N'
    {0x3e, 0x41, 0x41, 0x41, 0x3e}, // 'O'
    {0x7f, 0x09, 0x09, 0x09, 0x06}, // 'P'
    {0x3e, 0x41, 0x51, 0x21, 0x5e}, // 'Q'
    {0x7f, 0x09, 0x19, 0x29, 0x46}, // 'R'
    {0x46, 0x49, 0x49, 0x49, 0x31}, // 'S'
    {0x01, 0x01, 0x7f, 0x01, 0x01}, // 'T'
    {0x3f, 0x40, 0x40, 0x40, 0x3f}, // 'U'
    {0x1f, 0x20, 0x40, 0x20, 0x1f}, // 'V'
    {0x3f, 0x40, 0x38, 0x40, 0x3f}, // 'W'
    {0x63, 0x14, 0x08, 0x14, 0x63}, // 'X'
    {0x07, 0x08, 0x70, 0x08, 0x07}, // 'Y'
    {0x61, 0x51, 0x49, 0x45, 0x43}  // 'Z'
};

GuiWindow::GuiWindow(SyrebasClap* plugin) : plugin_(plugin) {
    pixelBuffer_.resize(width_ * height_, 0xFFDBDFE1);
    initControls();
}

GuiWindow::~GuiWindow() {
    destroy();
}

void GuiWindow::initControls() {
    controls_.clear();
    // 5 Main Knobs
    controls_.push_back({ PARAM_CUTOFF, "CUT OFF FREQ", ControlType::Knob, 55, 100, 20, 0.0, 1.0, 0.5, false });
    controls_.push_back({ PARAM_RESONANCE, "RESONANCE", ControlType::Knob, 130, 100, 20, 0.0, 1.0, 0.5, false });
    controls_.push_back({ PARAM_ENV_MOD, "ENV MOD", ControlType::Knob, 205, 100, 20, 0.0, 1.0, 0.5, false });
    controls_.push_back({ PARAM_DECAY, "DECAY", ControlType::Knob, 280, 100, 20, 0.0, 1.0, 0.5, false });
    controls_.push_back({ PARAM_ACCENT, "ACCENT", ControlType::Knob, 355, 100, 20, 0.0, 1.0, 0.5, false });

    // Waveform Toggle Switch
    controls_.push_back({ PARAM_WAVEFORM, "WAVEFORM", ControlType::ToggleSwitch, 425, 100, 15, 0.0, 1.0, 0.0, true });

    // Master Volume Knob
    controls_.push_back({ PARAM_VOLUME, "VOLUME", ControlType::Knob, 485, 100, 18, 0.0, 1.0, 0.8, false });

    updateKnobValuesFromPlugin();
}

void GuiWindow::updateKnobValuesFromPlugin() {
    if (!plugin_) return;
    for (size_t i = 0; i < controls_.size(); ++i) {
        if (static_cast<int>(i) == activeControlIndex_) continue;
        double val = 0.0;
        if (plugin_->paramsValue(controls_[i].id, &val)) {
            controls_[i].currentVal = val;
        }
    }
}

static inline uint32_t blendColors(uint32_t src, uint32_t dst, float alpha) {
    if (alpha <= 0.0f) return dst;
    if (alpha >= 1.0f) return src;

    uint32_t sr = (src >> 16) & 0xFF;
    uint32_t sg = (src >> 8) & 0xFF;
    uint32_t sb = src & 0xFF;

    uint32_t dr = (dst >> 16) & 0xFF;
    uint32_t dg = (dst >> 8) & 0xFF;
    uint32_t db = dst & 0xFF;

    uint32_t r = static_cast<uint32_t>(sr * alpha + dr * (1.0f - alpha) + 0.5f);
    uint32_t g = static_cast<uint32_t>(sg * alpha + dg * (1.0f - alpha) + 0.5f);
    uint32_t b = static_cast<uint32_t>(sb * alpha + db * (1.0f - alpha) + 0.5f);

    return 0xFF000000 | (r << 16) | (g << 8) | b;
}

void GuiWindow::drawRect(int x, int y, int w, int h, uint32_t color) {
    int hW = width_ * 2;
    int hH = height_ * 2;
    int xStart = (std::max)(0, x * 2);
    int yStart = (std::max)(0, y * 2);
    int xEnd = (std::min)(hW, (x + w) * 2);
    int yEnd = (std::min)(hH, (y + h) * 2);

    for (int py = yStart; py < yEnd; ++py) {
        for (int px = xStart; px < xEnd; ++px) {
            hiResBuffer_[py * hW + px] = color;
        }
    }
}

void GuiWindow::drawCircle(int cx, int cy, int radius, uint32_t color) {
    int hW = width_ * 2;
    int hH = height_ * 2;
    float cx2 = cx * 2.0f + 1.0f;
    float cy2 = cy * 2.0f + 1.0f;
    float r2 = radius * 2.0f;

    int minX = (std::max)(0, static_cast<int>(cx2 - r2 - 2.0f));
    int maxX = (std::min)(hW - 1, static_cast<int>(cx2 + r2 + 2.0f));
    int minY = (std::max)(0, static_cast<int>(cy2 - r2 - 2.0f));
    int maxY = (std::min)(hH - 1, static_cast<int>(cy2 + r2 + 2.0f));

    for (int py = minY; py <= maxY; ++py) {
        float dy = py - cy2;
        for (int px = minX; px <= maxX; ++px) {
            float dx = px - cx2;
            float dist = std::hypot(dx, dy);
            float alpha = 0.0f;
            if (dist <= r2 - 0.75f) {
                alpha = 1.0f;
            } else if (dist < r2 + 0.75f) {
                alpha = (r2 + 0.75f - dist) / 1.5f;
            }
            if (alpha > 0.0f) {
                int idx = py * hW + px;
                hiResBuffer_[idx] = blendColors(color, hiResBuffer_[idx], alpha);
            }
        }
    }
}

void GuiWindow::drawCircleOutline(int cx, int cy, int radius, uint32_t color) {
    int hW = width_ * 2;
    int hH = height_ * 2;
    float cx2 = cx * 2.0f + 1.0f;
    float cy2 = cy * 2.0f + 1.0f;
    float rOuter = radius * 2.0f;
    float rInner = (radius - 1) * 2.0f;

    int minX = (std::max)(0, static_cast<int>(cx2 - rOuter - 2.0f));
    int maxX = (std::min)(hW - 1, static_cast<int>(cx2 + rOuter + 2.0f));
    int minY = (std::max)(0, static_cast<int>(cy2 - rOuter - 2.0f));
    int maxY = (std::min)(hH - 1, static_cast<int>(cy2 + rOuter + 2.0f));

    for (int py = minY; py <= maxY; ++py) {
        float dy = py - cy2;
        for (int px = minX; px <= maxX; ++px) {
            float dx = px - cx2;
            float dist = std::hypot(dx, dy);
            float alpha = 0.0f;
            if (dist >= rInner - 0.75f && dist <= rOuter + 0.75f) {
                float aOuter = (dist <= rOuter - 0.75f) ? 1.0f : ((rOuter + 0.75f - dist) / 1.5f);
                float aInner = (dist >= rInner + 0.75f) ? 1.0f : ((dist - (rInner - 0.75f)) / 1.5f);
                alpha = (std::min)(aOuter, aInner);
            }
            if (alpha > 0.0f) {
                int idx = py * hW + px;
                hiResBuffer_[idx] = blendColors(color, hiResBuffer_[idx], alpha);
            }
        }
    }
}

void GuiWindow::drawLine(int x0, int y0, int x1, int y1, uint32_t color, int thickness) {
    int hW = width_ * 2;
    int hH = height_ * 2;

    float p0x = x0 * 2.0f + 1.0f;
    float p0y = y0 * 2.0f + 1.0f;
    float p1x = x1 * 2.0f + 1.0f;
    float p1y = y1 * 2.0f + 1.0f;
    float halfThick = thickness * 1.0f; // in 2x pixels

    float l2 = (p1x - p0x) * (p1x - p0x) + (p1y - p0y) * (p1y - p0y);

    int minX = (std::max)(0, static_cast<int>((std::min)(p0x, p1x) - halfThick - 2.0f));
    int maxX = (std::min)(hW - 1, static_cast<int>((std::max)(p0x, p1x) + halfThick + 2.0f));
    int minY = (std::max)(0, static_cast<int>((std::min)(p0y, p1y) - halfThick - 2.0f));
    int maxY = (std::min)(hH - 1, static_cast<int>((std::max)(p0y, p1y) + halfThick + 2.0f));

    for (int py = minY; py <= maxY; ++py) {
        float pyf = static_cast<float>(py);
        for (int px = minX; px <= maxX; ++px) {
            float pxf = static_cast<float>(px);
            float dist = 0.0f;
            if (l2 == 0.0f) {
                dist = std::hypot(pxf - p0x, pyf - p0y);
            } else {
                float t = ((pxf - p0x) * (p1x - p0x) + (pyf - p0y) * (p1y - p0y)) / l2;
                t = (std::min)((std::max)(t, 0.0f), 1.0f);
                float projX = p0x + t * (p1x - p0x);
                float projY = p0y + t * (p1y - p0y);
                dist = std::hypot(pxf - projX, pyf - projY);
            }

            float alpha = 0.0f;
            if (dist <= halfThick - 0.75f) {
                alpha = 1.0f;
            } else if (dist < halfThick + 0.75f) {
                alpha = (halfThick + 0.75f - dist) / 1.5f;
            }

            if (alpha > 0.0f) {
                int idx = py * hW + px;
                hiResBuffer_[idx] = blendColors(color, hiResBuffer_[idx], alpha);
            }
        }
    }
}

void GuiWindow::drawChar(int x, int y, char c, uint32_t color, int scale) {
    if (c >= 'a' && c <= 'z') c = c - 'a' + 'A';
    if (c < 32 || c > 95) return;

    int idx = c - 32;
    for (int col = 0; col < 5; ++col) {
        uint8_t line = font5x7[idx][col];
        for (int row = 0; row < 7; ++row) {
            if (line & (1 << row)) {
                drawRect(x + col * scale, y + row * scale, scale, scale, color);
            }
        }
    }
}

void GuiWindow::drawText(int x, int y, const char* text, uint32_t color, int scale) {
    int currX = x;
    while (*text) {
        drawChar(currX, y, *text, color, scale);
        currX += 6 * scale;
        text++;
    }
}

void GuiWindow::drawSyrebasTitle(int x, int y) {
    // Draw Roland TB-303 styled "Syrebas" logo using drawing primitives
    // S
    drawRect(x, y, 22, 6, 0xFF121212);
    drawRect(x, y, 6, 16, 0xFF121212);
    drawRect(x, y + 15, 22, 6, 0xFF121212);
    drawRect(x + 16, y + 18, 6, 17, 0xFF121212);
    drawRect(x, y + 32, 22, 6, 0xFF121212);

    // y
    int yX = x + 28;
    drawRect(yX, y + 12, 5, 12, 0xFF121212);
    drawRect(yX + 11, y + 12, 5, 26, 0xFF121212);
    drawRect(yX, y + 20, 16, 5, 0xFF121212);
    drawRect(yX, y + 33, 16, 5, 0xFF121212);

    // r
    int rX = x + 54;
    drawRect(rX, y + 12, 5, 26, 0xFF121212);
    drawRect(rX, y + 12, 14, 5, 0xFF121212);
    drawRect(rX + 12, y + 15, 5, 8, 0xFF121212);

    // e
    int eX = x + 75;
    drawRect(eX, y + 12, 16, 5, 0xFF121212);
    drawRect(eX, y + 12, 5, 26, 0xFF121212);
    drawRect(eX, y + 22, 14, 5, 0xFF121212);
    drawRect(eX, y + 33, 16, 5, 0xFF121212);

    // b
    int bX = x + 97;
    drawRect(bX, y, 5, 38, 0xFF121212);
    drawRect(bX, y + 18, 16, 5, 0xFF121212);
    drawRect(bX + 12, y + 21, 5, 14, 0xFF121212);
    drawRect(bX, y + 33, 16, 5, 0xFF121212);

    // a
    int aX = x + 119;
    drawRect(aX, y + 18, 14, 5, 0xFF121212);
    drawRect(aX + 11, y + 18, 5, 20, 0xFF121212);
    drawRect(aX, y + 26, 14, 4, 0xFF121212);
    drawRect(aX, y + 33, 14, 5, 0xFF121212);
    drawRect(aX, y + 26, 4, 12, 0xFF121212);

    // s
    int s2X = x + 139;
    drawRect(s2X, y + 18, 14, 4, 0xFF121212);
    drawRect(s2X, y + 18, 4, 9, 0xFF121212);
    drawRect(s2X, y + 25, 14, 4, 0xFF121212);
    drawRect(s2X + 10, y + 27, 4, 9, 0xFF121212);
    drawRect(s2X, y + 34, 14, 4, 0xFF121212);
}

void GuiWindow::drawKnob(const Control& knob) {
    // Label centered above knob
    int labelLen = static_cast<int>(strlen(knob.label));
    int labelX = knob.x - (labelLen * 6) / 2;
    drawText(labelX, knob.y - knob.radius - 28, knob.label, 0xFF101010, 1);

    // Circular dial tick marks
    int numTicks = 11;
    double startAngle = 135.0 * M_PI / 180.0; // 7 o'clock
    double totalAngle = 270.0 * M_PI / 180.0; // Clockwise to 5 o'clock

    for (int i = 0; i < numTicks; ++i) {
        double norm = static_cast<double>(i) / (numTicks - 1);
        double angle = startAngle + norm * totalAngle;

        int rIn = knob.radius + 4;
        int rOut = knob.radius + 8;

        int x1 = knob.x + static_cast<int>(std::cos(angle) * rIn);
        int y1 = knob.y + static_cast<int>(std::sin(angle) * rIn);
        int x2 = knob.x + static_cast<int>(std::cos(angle) * rOut);
        int y2 = knob.y + static_cast<int>(std::sin(angle) * rOut);

        drawLine(x1, y1, x2, y2, 0xFF202020, 1);

        // 12 o'clock tick mark (i == 5) gets the iconic 303 black square above it
        if (i == 5) {
            drawRect(knob.x - 2, knob.y - knob.radius - 14, 4, 4, 0xFF101010);
        }
    }

    // Outer shadow / bezel
    drawCircle(knob.x + 1, knob.y + 1, knob.radius + 2, 0xFF888A8C);
    drawCircle(knob.x, knob.y, knob.radius + 1, 0xFF202020);

    // Knob body (Metallic fluted silver)
    drawCircle(knob.x, knob.y, knob.radius, 0xFF808488);
    drawCircle(knob.x, knob.y, knob.radius - 1, 0xFFB4B8BC);

    // Fluted ridges around skirt
    for (int a = 0; a < 360; a += 30) {
        double rad = a * M_PI / 180.0;
        int rx1 = knob.x + static_cast<int>(std::cos(rad) * (knob.radius - 4));
        int ry1 = knob.y + static_cast<int>(std::sin(rad) * (knob.radius - 4));
        int rx2 = knob.x + static_cast<int>(std::cos(rad) * knob.radius);
        int ry2 = knob.y + static_cast<int>(std::sin(rad) * knob.radius);
        drawLine(rx1, ry1, rx2, ry2, 0xFF606468, 1);
    }

    // Conical top face
    drawCircle(knob.x, knob.y, knob.radius - 4, 0xFFD4D8DC);
    drawCircleOutline(knob.x, knob.y, knob.radius - 4, 0xFF909498);

    // Pointer indicator line
    double normVal = (knob.currentVal - knob.minVal) / (knob.maxVal - knob.minVal);
    normVal = (std::min)((std::max)(normVal, 0.0), 1.0);
    double ptrAngle = startAngle + normVal * totalAngle;

    int ptrX = knob.x + static_cast<int>(std::cos(ptrAngle) * (knob.radius - 3));
    int ptrY = knob.y + static_cast<int>(std::sin(ptrAngle) * (knob.radius - 3));

    drawLine(knob.x, knob.y, ptrX, ptrY, 0xFF101010, 2);
    drawCircle(ptrX, ptrY, 1, 0xFF101010);
}

void GuiWindow::drawToggleSwitch(const Control& ctrl) {
    // Label above switch
    int labelLen = static_cast<int>(strlen(ctrl.label));
    int labelX = ctrl.x - (labelLen * 6) / 2;
    drawText(labelX, ctrl.y - 35, ctrl.label, 0xFF101010, 1);

    // SAW / SQUARE labels beside positions
    drawText(ctrl.x - 28, ctrl.y - 18, "SQR", 0xFF202020, 1);
    drawText(ctrl.x - 28, ctrl.y + 10, "SAW", 0xFF202020, 1);

    // Outer metal frame box
    drawRect(ctrl.x - 8, ctrl.y - 20, 16, 40, 0xFF202020);
    drawRect(ctrl.x - 7, ctrl.y - 19, 14, 38, 0xFF888C90);
    drawRect(ctrl.x - 5, ctrl.y - 17, 10, 34, 0xFF181818);

    // Toggle handle
    bool isSquare = (ctrl.currentVal >= 0.5);
    int handleY = isSquare ? (ctrl.y - 16) : (ctrl.y + 2);

    drawRect(ctrl.x - 7, handleY, 14, 14, 0xFF303030);
    drawRect(ctrl.x - 6, handleY + 1, 12, 12, 0xFFE0E4E8);
    drawRect(ctrl.x - 4, handleY + 3, 8, 8, 0xFFB0B4B8);
    drawLine(ctrl.x - 5, handleY + 7, ctrl.x + 5, handleY + 7, 0xFF101010, 1);
}

void GuiWindow::renderFrame() {
    updateKnobValuesFromPlugin();

    int hW = width_ * 2;
    int hH = height_ * 2;
    if (hiResBuffer_.size() != static_cast<size_t>(hW * hH)) {
        hiResBuffer_.resize(hW * hH);
    }

    // 1. Brushed silver panel background
    std::fill(hiResBuffer_.begin(), hiResBuffer_.end(), 0xFFDBDFE1);

    // Top & Bottom metallic borders / trims
    drawRect(0, 0, width_, 12, 0xFFC0C4C8);
    drawLine(0, 12, width_, 12, 0xFF808488, 1);
    drawLine(0, 13, width_, 13, 0xFFFFFFFF, 1);

    drawLine(0, height_ - 14, width_, height_ - 14, 0xFF808488, 1);
    drawRect(0, height_ - 13, width_, 13, 0xFFC0C4C8);

    // Vertical dividing line separating controls from right title panel
    drawLine(530, 14, 530, height_ - 14, 0xFF181818, 2);

    // 2. Draw Controls
    for (const auto& ctrl : controls_) {
        if (ctrl.type == ControlType::Knob) {
            drawKnob(ctrl);
        } else if (ctrl.type == ControlType::ToggleSwitch) {
            drawToggleSwitch(ctrl);
        }
    }

    // 3. Draw Title Logo "Syrebas"
    drawSyrebasTitle(545, 65);

    // 4. Downsample hiResBuffer_ (2x2 box filter) into pixelBuffer_
    pixelBuffer_.resize(width_ * height_);
    for (uint32_t py = 0; py < height_; ++py) {
        for (uint32_t px = 0; px < width_; ++px) {
            uint32_t p00 = hiResBuffer_[(2 * py) * hW + (2 * px)];
            uint32_t p01 = hiResBuffer_[(2 * py) * hW + (2 * px + 1)];
            uint32_t p10 = hiResBuffer_[(2 * py + 1) * hW + (2 * px)];
            uint32_t p11 = hiResBuffer_[(2 * py + 1) * hW + (2 * px + 1)];

            uint32_t r = (((p00 >> 16) & 0xFF) + ((p01 >> 16) & 0xFF) + ((p10 >> 16) & 0xFF) + ((p11 >> 16) & 0xFF) + 2) >> 2;
            uint32_t g = (((p00 >> 8) & 0xFF) + ((p01 >> 8) & 0xFF) + ((p10 >> 8) & 0xFF) + ((p11 >> 8) & 0xFF) + 2) >> 2;
            uint32_t b = ((p00 & 0xFF) + (p01 & 0xFF) + (p10 & 0xFF) + (p11 & 0xFF) + 2) >> 2;

            pixelBuffer_[py * width_ + px] = 0xFF000000 | (r << 16) | (g << 8) | b;
        }
    }

#if defined(__linux__) && !defined(__APPLE__)
    drawX11Frame();
#elif defined(_WIN32)
    drawWin32Frame();
#elif defined(__APPLE__)
    drawCocoaFrame();
#endif
}

void GuiWindow::handleMouseDown(int x, int y, bool isShift) {
    lastShiftState_ = isShift;
    for (size_t i = 0; i < controls_.size(); ++i) {
        auto& ctrl = controls_[i];
        if (ctrl.type == ControlType::Knob) {
            int dx = x - ctrl.x;
            int dy = y - ctrl.y;
            if (dx * dx + dy * dy <= (ctrl.radius + 10) * (ctrl.radius + 10)) {
                activeControlIndex_ = static_cast<int>(i);
                dragStartY_ = y;
                dragStartVal_ = ctrl.currentVal;
                if (plugin_) {
                    plugin_->onBeginEditFromGui(ctrl.id);
                }
                break;
            }
        } else if (ctrl.type == ControlType::ToggleSwitch) {
            if (std::abs(x - ctrl.x) <= 20 && std::abs(y - ctrl.y) <= 25) {
                if (plugin_) {
                    plugin_->onBeginEditFromGui(ctrl.id);
                }
                double newVal = (ctrl.currentVal >= 0.5) ? 0.0 : 1.0;
                ctrl.currentVal = newVal;
                if (plugin_) {
                    plugin_->onParamValueFromGui(ctrl.id, newVal);
                    plugin_->onEndEditFromGui(ctrl.id);
                }
                renderFrame();
                break;
            }
        }
    }
}

void GuiWindow::handleMouseDrag(int x, int y, bool isShift) {
    if (activeControlIndex_ < 0 || activeControlIndex_ >= static_cast<int>(controls_.size())) return;

    auto& ctrl = controls_[activeControlIndex_];
    if (ctrl.type != ControlType::Knob) return;

    if (isShift != lastShiftState_) {
        dragStartY_ = y;
        dragStartVal_ = ctrl.currentVal;
        lastShiftState_ = isShift;
    }

    int deltaY = dragStartY_ - y;

    double range = ctrl.maxVal - ctrl.minVal;
    double sensitivity = (isShift ? 0.0025 : 0.0125) * range;
    double newVal = dragStartVal_ + deltaY * sensitivity;

    newVal = (std::min)((std::max)(newVal, ctrl.minVal), ctrl.maxVal);
    ctrl.currentVal = newVal;

    if (plugin_) {
        plugin_->onParamValueFromGui(ctrl.id, newVal);
    }

    renderFrame();
}

void GuiWindow::handleMouseUp() {
    if (activeControlIndex_ >= 0 && activeControlIndex_ < static_cast<int>(controls_.size())) {
        if (plugin_) {
            plugin_->onEndEditFromGui(controls_[activeControlIndex_].id);
        }
    }
    activeControlIndex_ = -1;
}

bool GuiWindow::setParent(const clap_window_t* window) {
    if (!window) return false;
#if defined(__linux__) && !defined(__APPLE__)
    if (std::strcmp(window->api, CLAP_WINDOW_API_X11) == 0) {
        x11ParentWindow_ = window->x11;
        initX11Window();
        return true;
    }
#elif defined(_WIN32)
    if (std::strcmp(window->api, CLAP_WINDOW_API_WIN32) == 0) {
        parentHwnd_ = window->win32;
        initWin32Window();
        return true;
    }
#elif defined(__APPLE__)
    if (std::strcmp(window->api, CLAP_WINDOW_API_COCOA) == 0) {
        parentNsView_ = window->cocoa;
        initCocoaWindow();
        return true;
    }
#endif
    return false;
}

bool GuiWindow::setSize(uint32_t width, uint32_t height) {
    width_ = width;
    height_ = height;
    pixelBuffer_.resize(width_ * height_, 0xFFDBDFE1);
    renderFrame();
    return true;
}

bool GuiWindow::show() {
    renderFrame();
    return true;
}

bool GuiWindow::hide() {
    return true;
}

void GuiWindow::destroy() {
    isRunning_ = false;
    if (eventThread_.joinable()) {
        eventThread_.join();
    }
#if defined(__linux__) && !defined(__APPLE__)
    if (x11Display_ && x11Created_) {
        Display* display = static_cast<Display*>(x11Display_);
        XDestroyWindow(display, x11Window_);
        XCloseDisplay(display);
        x11Display_ = nullptr;
        x11Created_ = false;
    }
#endif
}

#if defined(__linux__) && !defined(__APPLE__)
void GuiWindow::initX11Window() {
    if (x11Created_) return;

    Display* display = XOpenDisplay(nullptr);
    if (!display) return;

    x11Display_ = display;
    int screen = DefaultScreen(display);
    Window parent = x11ParentWindow_ ? x11ParentWindow_ : RootWindow(display, screen);

    x11Window_ = XCreateSimpleWindow(display, parent, 0, 0, width_, height_, 0,
                                     BlackPixel(display, screen), WhitePixel(display, screen));

    XSelectInput(display, x11Window_, ExposureMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask);
    XMapWindow(display, x11Window_);
    XFlush(display);

    x11Created_ = true;
    renderFrame();

    isRunning_ = true;
    eventThread_ = std::thread(&GuiWindow::eventLoopX11, this);
}

void GuiWindow::eventLoopX11() {
    if (!x11Display_) return;
    Display* display = static_cast<Display*>(x11Display_);

    while (isRunning_) {
        while (XPending(display) > 0) {
            XEvent ev;
            XNextEvent(display, &ev);

            if (ev.type == Expose) {
                drawX11Frame();
            } else if (ev.type == ButtonPress) {
                bool isShift = (ev.xbutton.state & ShiftMask) != 0;
                handleMouseDown(ev.xbutton.x, ev.xbutton.y, isShift);
            } else if (ev.type == MotionNotify) {
                if (ev.xmotion.state & Button1Mask) {
                    bool isShift = (ev.xmotion.state & ShiftMask) != 0;
                    handleMouseDrag(ev.xmotion.x, ev.xmotion.y, isShift);
                }
            } else if (ev.type == ButtonRelease) {
                handleMouseUp();
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}

void GuiWindow::drawX11Frame() {
    if (!x11Display_ || !x11Created_) return;

    Display* display = static_cast<Display*>(x11Display_);
    int screen = DefaultScreen(display);

    XImage* image = XCreateImage(display, DefaultVisual(display, screen),
                                 24, ZPixmap, 0,
                                 reinterpret_cast<char*>(pixelBuffer_.data()),
                                 width_, height_, 32, 0);

    GC gc = DefaultGC(display, screen);
    XPutImage(display, x11Window_, gc, image, 0, 0, 0, 0, width_, height_);

    image->data = nullptr;
    XDestroyImage(image);
    XFlush(display);
}
#endif

#if defined(_WIN32)
static const wchar_t* kSyrebasClassName = L"SyrebasWindowCLASS";
static bool g_win32ClassRegistered = false;

static LRESULT CALLBACK SyrebasWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    GuiWindow* gui = reinterpret_cast<GuiWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (msg) {
        case WM_CREATE: {
            CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            gui = reinterpret_cast<GuiWindow*>(cs->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(gui));
            SetTimer(hwnd, 1, 16, NULL);
            return 0;
        }
        case WM_TIMER: {
            if (gui) {
                gui->renderFrame();
            }
            return 0;
        }
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            if (gui) {
                gui->drawWin32Frame();
            }
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_LBUTTONDOWN: {
            if (gui) {
                int x = LOWORD(lParam);
                int y = HIWORD(lParam);
                bool isShift = (wParam & MK_SHIFT) != 0;
                SetCapture(hwnd);
                gui->handleMouseDown(x, y, isShift);
            }
            return 0;
        }
        case WM_MOUSEMOVE: {
            if (gui && (wParam & MK_LBUTTON)) {
                int x = LOWORD(lParam);
                int y = HIWORD(lParam);
                bool isShift = (wParam & MK_SHIFT) != 0;
                gui->handleMouseDrag(x, y, isShift);
            }
            return 0;
        }
        case WM_LBUTTONUP: {
            if (gui) {
                ReleaseCapture();
                gui->handleMouseUp();
            }
            return 0;
        }
        case WM_DESTROY: {
            KillTimer(hwnd, 1);
            return 0;
        }
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void GuiWindow::initWin32Window() {
    if (hwnd_) return;

    HINSTANCE hInstance = GetModuleHandleW(NULL);

    if (!g_win32ClassRegistered) {
        WNDCLASSW wc = {};
        wc.lpfnWndProc = SyrebasWndProc;
        wc.hInstance = hInstance;
        wc.lpszClassName = kSyrebasClassName;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        RegisterClassW(&wc);
        g_win32ClassRegistered = true;
    }

    HWND parent = static_cast<HWND>(parentHwnd_);

    hwnd_ = CreateWindowExW(
        0, kSyrebasClassName, L"Syrebas 303",
        WS_CHILD | WS_VISIBLE,
        0, 0, width_, height_,
        parent, NULL, hInstance, this
    );

    renderFrame();
}

void GuiWindow::drawWin32Frame() {
    if (!hwnd_) return;

    HDC hdc = GetDC(static_cast<HWND>(hwnd_));
    if (!hdc) return;

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width_;
    bmi.bmiHeader.biHeight = -static_cast<int>(height_);
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    SetDIBitsToDevice(
        hdc,
        0, 0, width_, height_,
        0, 0, 0, height_,
        pixelBuffer_.data(),
        &bmi,
        DIB_RGB_COLORS
    );

    ReleaseDC(static_cast<HWND>(hwnd_), hdc);
}
#endif

#if defined(__APPLE__)
void GuiWindow::initCocoaWindow() {}
void GuiWindow::drawCocoaFrame() {}
#endif

// CLAP GUI Extension Callbacks
const clap_plugin_gui_t g_syrebasGuiExtension = {
    [](const clap_plugin_t* plugin, const char* api, bool is_floating) -> bool {
#if defined(__linux__) && !defined(__APPLE__)
        return std::strcmp(api, CLAP_WINDOW_API_X11) == 0 && !is_floating;
#elif defined(_WIN32)
        return std::strcmp(api, CLAP_WINDOW_API_WIN32) == 0 && !is_floating;
#elif defined(__APPLE__)
        return std::strcmp(api, CLAP_WINDOW_API_COCOA) == 0 && !is_floating;
#else
        return false;
#endif
    },
    [](const clap_plugin_t* plugin, const char** api, bool* is_floating) -> bool {
#if defined(__linux__) && !defined(__APPLE__)
        *api = CLAP_WINDOW_API_X11;
#elif defined(_WIN32)
        *api = CLAP_WINDOW_API_WIN32;
#elif defined(__APPLE__)
        *api = CLAP_WINDOW_API_COCOA;
#endif
        *is_floating = false;
        return true;
    },
    [](const clap_plugin_t* plugin, const char* api, bool is_floating) -> bool {
        auto* self = static_cast<SyrebasClap*>(plugin->plugin_data);
        self->createGuiWindow();
        return true;
    },
    [](const clap_plugin_t* plugin) {
        auto* self = static_cast<SyrebasClap*>(plugin->plugin_data);
        self->destroyGuiWindow();
    },
    [](const clap_plugin_t* plugin, double scale) -> bool {
        return false;
    },
    [](const clap_plugin_t* plugin, uint32_t* width, uint32_t* height) -> bool {
        *width = 710;
        *height = 180;
        return true;
    },
    [](const clap_plugin_t* plugin) -> bool {
        return false;
    },
    [](const clap_plugin_t* plugin, clap_gui_resize_hints_t* hints) -> bool {
        return false;
    },
    [](const clap_plugin_t* plugin, uint32_t* width, uint32_t* height) -> bool {
        *width = 710;
        *height = 180;
        return true;
    },
    [](const clap_plugin_t* plugin, uint32_t width, uint32_t height) -> bool {
        auto* self = static_cast<SyrebasClap*>(plugin->plugin_data);
        if (self->getGuiWindow()) {
            return self->getGuiWindow()->setSize(width, height);
        }
        return true;
    },
    [](const clap_plugin_t* plugin, const clap_window_t* window) -> bool {
        auto* self = static_cast<SyrebasClap*>(plugin->plugin_data);
        if (!self->getGuiWindow()) {
            self->createGuiWindow();
        }
        return self->getGuiWindow()->setParent(window);
    },
    [](const clap_plugin_t* plugin, const clap_window_t* window) -> bool {
        return false;
    },
    [](const clap_plugin_t* plugin, const char* title) {},
    [](const clap_plugin_t* plugin) -> bool {
        auto* self = static_cast<SyrebasClap*>(plugin->plugin_data);
        if (self->getGuiWindow()) {
            return self->getGuiWindow()->show();
        }
        return false;
    },
    [](const clap_plugin_t* plugin) -> bool {
        auto* self = static_cast<SyrebasClap*>(plugin->plugin_data);
        if (self->getGuiWindow()) {
            return self->getGuiWindow()->hide();
        }
        return false;
    }
};

} // namespace syrebas
