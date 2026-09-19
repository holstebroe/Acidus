#ifndef ACIDUS_ICONTROL_RENDERER_HPP
#define ACIDUS_ICONTROL_RENDERER_HPP

namespace acidus {

class Graphics;
struct Control;
class Font;

class IControlRenderer {
public:
    virtual ~IControlRenderer() = default;
    virtual void drawKnob(Graphics& g, const Control& ctrl, const Font& font) = 0;
    virtual void drawToggleSwitch(Graphics& g, const Control& ctrl, const Font& font) = 0;
};

} // namespace acidus

#endif // ACIDUS_ICONTROL_RENDERER_HPP
