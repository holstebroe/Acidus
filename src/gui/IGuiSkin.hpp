#ifndef ACIDUS_IGUI_SKIN_HPP
#define ACIDUS_IGUI_SKIN_HPP

#include <memory>

namespace acidus {

class Graphics;
struct Control;
class Font;

// The look of the Acidus panel: its background and controls. GuiWindow owns
// the layout, input and the animated logo plate; a skin only paints. Two
// skins exist, picked at build time by the ACIDUS_GUI_STYLE CMake option:
// RETRO (RetroSkin.cpp, the small pixel-drawn default) and MODERN
// (ModernSkin.cpp, shaded knobs and anti-aliased lettering). Only the
// chosen one is compiled in.
class IGuiSkin {
public:
    virtual ~IGuiSkin() = default;
    // Panel background, trims and the divider before the logo plate at
    // dividerX. Called first, on a cleared buffer.
    virtual void drawPanel(Graphics& g, int width, int height, int dividerX) = 0;
    virtual void drawKnob(Graphics& g, const Control& ctrl, const Font& font) = 0;
    virtual void drawToggleSwitch(Graphics& g, const Control& ctrl, const Font& font) = 0;
};

// The skin this build was configured with (defined by the one skin source
// compiled in).
std::unique_ptr<IGuiSkin> createGuiSkin();

} // namespace acidus

#endif // ACIDUS_IGUI_SKIN_HPP
