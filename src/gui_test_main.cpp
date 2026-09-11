#include "clap/SyrebasClap.hpp"
#include "gui/GuiWindow.hpp"
#include <fstream>
#include <iostream>

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
    return 0;
}
