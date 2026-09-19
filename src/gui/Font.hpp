#ifndef ACIDUS_FONT_HPP
#define ACIDUS_FONT_HPP

#include <cstdint>
#include <cstddef>

namespace acidus {

class Font {
public:
    Font(uint32_t width, uint32_t height);
    ~Font() = default;

    uint32_t getWidth() const { return width_; }
    uint32_t getHeight() const { return height_; }

    const uint8_t* getGlyph(char c) const;
    int getTextWidth(const char* text) const;

    static Font default5x7();

private:
    uint32_t width_{5};
    uint32_t height_{7};
};

} // namespace acidus

#endif // ACIDUS_FONT_HPP
