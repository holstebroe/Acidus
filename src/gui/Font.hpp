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
    int getTextWidth(const char* text, int scale = 1) const;

    // The glyph table was authored 7 columns wide (letters like M/N/W/J use
    // column index 6); a 5-wide font here would silently crop their right side.
    static Font defaultPanelFont();

private:
    uint32_t width_{7};
    uint32_t height_{7};
};

} // namespace acidus

#endif // ACIDUS_FONT_HPP
