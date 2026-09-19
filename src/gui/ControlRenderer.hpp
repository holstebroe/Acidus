#ifndef ACIDUS_CONTROL_RENDERER_HPP
#define ACIDUS_CONTROL_RENDERER_HPP

#include "IControlRenderer.hpp"

namespace acidus {

class TB303ControlRenderer : public IControlRenderer {
public:
    void drawKnob(Graphics& g, const Control& ctrl, const Font& font) override;
    void drawToggleSwitch(Graphics& g, const Control& ctrl, const Font& font) override;
};

} // namespace acidus

#endif // ACIDUS_CONTROL_RENDERER_HPP
