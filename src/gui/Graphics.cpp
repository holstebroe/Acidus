#include "Graphics.hpp"
#include <cmath>
#include <algorithm>

namespace syrebas {

Graphics::Graphics(uint32_t* buffer, int width, int height, int scale)
    : buffer_(buffer), width_(width), height_(height), scale_(scale),
      bufferWidth_(width * scale), bufferHeight_(height * scale) {}

void Graphics::clear(uint32_t color) {
    if (!buffer_) return;
    int size = bufferWidth_ * bufferHeight_;
    std::fill(buffer_, buffer_ + size, color);
}

uint32_t Graphics::blendColors(uint32_t src, uint32_t dst, float alpha) {
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

void Graphics::drawRect(int x, int y, int w, int h, uint32_t color) {
    if (!buffer_) return;
    int xStart = (std::max)(0, x * scale_);
    int yStart = (std::max)(0, y * scale_);
    int xEnd = (std::min)(bufferWidth_, (x + w) * scale_);
    int yEnd = (std::min)(bufferHeight_, (y + h) * scale_);

    for (int py = yStart; py < yEnd; ++py) {
        for (int px = xStart; px < xEnd; ++px) {
            buffer_[py * bufferWidth_ + px] = color;
        }
    }
}

void Graphics::drawCircle(int cx, int cy, int radius, uint32_t color) {
    if (!buffer_) return;
    float scaleF = static_cast<float>(scale_);
    float cx2 = cx * scaleF + (scaleF * 0.5f);
    float cy2 = cy * scaleF + (scaleF * 0.5f);
    float r2 = radius * scaleF;

    int minX = (std::max)(0, static_cast<int>(cx2 - r2 - 2.0f));
    int maxX = (std::min)(bufferWidth_ - 1, static_cast<int>(cx2 + r2 + 2.0f));
    int minY = (std::max)(0, static_cast<int>(cy2 - r2 - 2.0f));
    int maxY = (std::min)(bufferHeight_ - 1, static_cast<int>(cy2 + r2 + 2.0f));

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
                int idx = py * bufferWidth_ + px;
                buffer_[idx] = blendColors(color, buffer_[idx], alpha);
            }
        }
    }
}

void Graphics::drawCircleOutline(int cx, int cy, int radius, uint32_t color) {
    if (!buffer_) return;
    float scaleF = static_cast<float>(scale_);
    float cx2 = cx * scaleF + (scaleF * 0.5f);
    float cy2 = cy * scaleF + (scaleF * 0.5f);
    float rOuter = radius * scaleF;
    float rInner = (radius - 1) * scaleF;

    int minX = (std::max)(0, static_cast<int>(cx2 - rOuter - 2.0f));
    int maxX = (std::min)(bufferWidth_ - 1, static_cast<int>(cx2 + rOuter + 2.0f));
    int minY = (std::max)(0, static_cast<int>(cy2 - rOuter - 2.0f));
    int maxY = (std::min)(bufferHeight_ - 1, static_cast<int>(cy2 + rOuter + 2.0f));

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
                int idx = py * bufferWidth_ + px;
                buffer_[idx] = blendColors(color, buffer_[idx], alpha);
            }
        }
    }
}

void Graphics::drawLine(int x0, int y0, int x1, int y1, uint32_t color, int thickness) {
    if (!buffer_) return;
    float scaleF = static_cast<float>(scale_);
    float p0x = x0 * scaleF + (scaleF * 0.5f);
    float p0y = y0 * scaleF + (scaleF * 0.5f);
    float p1x = x1 * scaleF + (scaleF * 0.5f);
    float p1y = y1 * scaleF + (scaleF * 0.5f);
    float halfThick = thickness * (scaleF * 0.5f);

    float l2 = (p1x - p0x) * (p1x - p0x) + (p1y - p0y) * (p1y - p0y);

    int minX = (std::max)(0, static_cast<int>((std::min)(p0x, p1x) - halfThick - 2.0f));
    int maxX = (std::min)(bufferWidth_ - 1, static_cast<int>((std::max)(p0x, p1x) + halfThick + 2.0f));
    int minY = (std::max)(0, static_cast<int>((std::min)(p0y, p1y) - halfThick - 2.0f));
    int maxY = (std::min)(bufferHeight_ - 1, static_cast<int>((std::max)(p0y, p1y) + halfThick + 2.0f));

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
                int idx = py * bufferWidth_ + px;
                buffer_[idx] = blendColors(color, buffer_[idx], alpha);
            }
        }
    }
}

void Graphics::drawChar(int x, int y, char c, uint32_t color, const Font& font, int scale) {
    const uint8_t* glyph = font.getGlyphData(c);
    if (!glyph) return;

    int w = font.getWidth();
    int h = font.getHeight();

    for (int col = 0; col < w; ++col) {
        uint8_t line = glyph[col];
        for (int row = 0; row < h; ++row) {
            if (line & (1 << row)) {
                drawRect(x + col * scale, y + row * scale, scale, scale, color);
            }
        }
    }
}

void Graphics::drawText(int x, int y, const char* text, uint32_t color, const Font& font, int scale) {
    int currX = x;
    int charWidth = font.getWidth();
    while (*text) {
        drawChar(currX, y, *text, color, font, scale);
        currX += (charWidth + 1) * scale;
        text++;
    }
}

void Graphics::drawSyrebasTitle(int x, int y) {
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

} // namespace syrebas
