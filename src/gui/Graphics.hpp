#ifndef ACIDUS_GRAPHICS_HPP
#define ACIDUS_GRAPHICS_HPP

#include <cstdint>
#include <cstddef>
#include "Font.hpp"

namespace acidus {

class Graphics {
public:
    Graphics(uint32_t* buffer, uint32_t width, uint32_t height, int scale = 1);
    ~Graphics() = default;

    void clear(uint32_t color);
    void setPixel(int x, int y, uint32_t color);

    void drawLine(int x0, int y0, int x1, int y1, uint32_t color, int thickness = 1);
    void drawRect(int x, int y, int width, int height, uint32_t color);
    void fillRect(int x, int y, int width, int height, uint32_t color);

    void drawCircle(int cx, int cy, int radius, uint32_t color, int thickness = 1);
    void fillCircle(int cx, int cy, int radius, uint32_t color);

    void drawText(const Font& font, const char* text, int x, int y, uint32_t color);

    uint32_t getWidth() const { return logicalWidth_; }
    uint32_t getHeight() const { return logicalHeight_; }

private:
    uint32_t* buffer_{nullptr};
    uint32_t logicalWidth_{0};
    uint32_t logicalHeight_{0};
    int scale_{1};
    uint32_t bufferWidth_{0};
    uint32_t bufferHeight_{0};

    void blendPixel(int bx, int by, uint32_t srcColor);
};

} // namespace acidus

#endif // ACIDUS_GRAPHICS_HPP
