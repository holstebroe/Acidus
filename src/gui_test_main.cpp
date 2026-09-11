#include "clap/SyrebasClap.hpp"
#include "gui/GuiWindow.hpp"
#include <fstream>
#include <iostream>
#include <cassert>
#include <cmath>

int main() {
    syrebas::SyrebasClap plugin(nullptr);
    syrebas::GuiWindow gui(&plugin);

    gui.renderFrame();

    const auto& buffer = gui.getPixelBuffer();
    uint32_t w = gui.getWidth();
    uint32_t h = gui.getHeight();

    // Write raw ARGB buffer
    std::ofstream ofs("/tmp/syrebas_gui_buffer.raw", std::ios::binary);
    ofs.write(reinterpret_cast<const char*>(buffer.data()), buffer.size() * sizeof(uint32_t));
    ofs.close();

    std::cout << "GUI Frame rendered: " << w << "x" << h << ", saved to /tmp/syrebas_gui_buffer.raw" << std::endl;

    // Test mouse interaction and Shift fine tuning
    // Cutoff Knob is at x=55, y=100 (minVal=300, maxVal=10000)
    // 1. Standard mouse drag test (isShift = false)
    gui.handleMouseDown(55, 100, false);
    gui.handleMouseDrag(55, 20, false); // Drag up 80 pixels
    double valNormal = 0.0;
    plugin.paramsValue(syrebas::PARAM_CUTOFF, &valNormal);
    std::cout << "Cutoff after 80px normal drag: " << valNormal << " Hz" << std::endl;
    assert(valNormal > 9900.0);

    // 2. Fine mouse drag test (isShift = true)
    gui.handleMouseUp();
    gui.handleMouseDown(130, 100, true); // Resonance knob at (130, 100) with Shift
    gui.handleMouseDrag(130, 20, true);  // Drag up 80 pixels with Shift (from initial 0.5)
    double valFine = 0.0;
    plugin.paramsValue(syrebas::PARAM_RESONANCE, &valFine);
    std::cout << "Resonance after 80px fine drag with Shift: " << valFine << std::endl;
    assert(std::abs(valFine - 0.70) < 0.01);

    std::cout << "Mouse drag and Shift key fine-dialing unit tests passed successfully!" << std::endl;

    return 0;
}
