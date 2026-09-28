#include "clap/AcidusClap.hpp"
#include "gui/GuiWindow.hpp"
#include "gui/Graphics.hpp"
#include "gui/Font.hpp"
#include "gui/IControlRenderer.hpp"
#include <fstream>
#include <iostream>
#include <cassert>
#include <cmath>
#include <cstdlib>

int main() {
    acidus::AcidusClap plugin(nullptr);
    acidus::GuiWindow gui(&plugin);

    gui.renderFrame();

    const auto& buffer = gui.getPixelBuffer();
    uint32_t w = gui.getWidth();
    uint32_t h = gui.getHeight();

    std::ofstream ofs("/tmp/acidus_gui_buffer.raw", std::ios::binary);
    ofs.write(reinterpret_cast<const char*>(buffer.data()), buffer.size() * sizeof(uint32_t));
    ofs.close();

    std::cout << "GUI Frame rendered: " << w << "x" << h << ", saved to /tmp/acidus_gui_buffer.raw" << std::endl;

    struct TestOutEvents {
        std::vector<uint16_t> types;
        std::vector<clap_id> paramIds;
        std::vector<double> values;
        std::vector<uint32_t> flags;

        static bool tryPush(const clap_output_events_t* list, const clap_event_header_t* event) {
            auto* self = static_cast<TestOutEvents*>(list->ctx);
            self->types.push_back(event->type);
            if (event->type == CLAP_EVENT_PARAM_GESTURE_BEGIN || event->type == CLAP_EVENT_PARAM_GESTURE_END) {
                const auto* ge = reinterpret_cast<const clap_event_param_gesture_t*>(event);
                self->paramIds.push_back(ge->param_id);
                self->values.push_back(0.0);
            } else if (event->type == CLAP_EVENT_PARAM_VALUE) {
                const auto* ve = reinterpret_cast<const clap_event_param_value_t*>(event);
                self->paramIds.push_back(ve->param_id);
                self->values.push_back(ve->value);
            }
            self->flags.push_back(event->flags);
            return true;
        }
    } testCtx;

    clap_output_events_t mockOutList{};
    mockOutList.ctx = &testCtx;
    mockOutList.try_push = TestOutEvents::tryPush;

    gui.handleMouseDown(140, 100, false);
    gui.handleMouseDrag(140, 20, false); // Drag up 80 pixels
    gui.handleMouseUp();

    double valNormal = 0.0;
    plugin.paramsValue(acidus::PARAM_CUTOFF, &valNormal);
    std::cout << "Cutoff after 80px normal drag: " << valNormal << std::endl;
    assert(valNormal >= 0.99);

    plugin.paramsFlush(nullptr, &mockOutList);

    assert(!testCtx.types.empty());
    assert(testCtx.types.front() == CLAP_EVENT_PARAM_GESTURE_BEGIN);
    assert(testCtx.types.back() == CLAP_EVENT_PARAM_GESTURE_END);
    assert(testCtx.paramIds.front() == acidus::PARAM_CUTOFF);
    std::cout << "GUI output event gesture queue test passed successfully! Events recorded: " << testCtx.types.size() << std::endl;

    gui.handleMouseDown(232, 100, true); // Resonance knob at (232, 100) with Shift
    gui.handleMouseDrag(232, 20, true);  // Drag up 80 pixels with Shift
    gui.handleMouseUp();

    double valFine = 0.0;
    plugin.paramsValue(acidus::PARAM_RESONANCE, &valFine);
    std::cout << "Resonance after 80px fine drag with Shift: " << valFine << std::endl;
    assert(std::abs(valFine - 0.70) < 0.01);

    testCtx.types.clear();
    testCtx.paramIds.clear();
    testCtx.values.clear();
    testCtx.flags.clear();

    clap_event_midi_t midiCcEv{};
    midiCcEv.header.size = sizeof(midiCcEv);
    midiCcEv.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
    midiCcEv.header.type = CLAP_EVENT_MIDI;
    midiCcEv.port_index = 0;
    midiCcEv.data[0] = 0xB0; // Control Change Ch 1
    midiCcEv.data[1] = 74;   // CC 74 (Cutoff)
    midiCcEv.data[2] = 127;  // Max CC value

    struct TestInEvents {
        const clap_event_header_t* ev;
        static uint32_t size(const clap_input_events_t* list) { return 1; }
        static const clap_event_header_t* get(const clap_input_events_t* list, uint32_t index) {
            auto* self = static_cast<TestInEvents*>(list->ctx);
            return self->ev;
        }
    } inCtx;
    inCtx.ev = &midiCcEv.header;

    clap_input_events_t mockInList{};
    mockInList.ctx = &inCtx;
    mockInList.size = TestInEvents::size;
    mockInList.get = TestInEvents::get;

    plugin.paramsFlush(&mockInList, &mockOutList);

    double ccCutoffVal = 0.0;
    plugin.paramsValue(acidus::PARAM_CUTOFF, &ccCutoffVal);
    std::cout << "Cutoff after MIDI CC 74 (127): " << ccCutoffVal << std::endl;
    assert(ccCutoffVal == 1.0);
    assert(!testCtx.types.empty());
    assert(testCtx.types.back() == CLAP_EVENT_PARAM_VALUE);
    assert(testCtx.flags.back() == CLAP_EVENT_DONT_RECORD);

    acidus::Font customFont(6, 8);
    assert(customFont.getWidth() == 6);
    assert(customFont.getHeight() == 8);
    gui.setFont(customFont);
    assert(gui.getFont().getWidth() == 6);

    class TestCustomRenderer : public acidus::IControlRenderer {
    public:
        bool knobDrawn = false;
        bool switchDrawn = false;
        void drawKnob(acidus::Graphics& g, const acidus::Control& ctrl, const acidus::Font& font) override {
            knobDrawn = true;
        }
        void drawToggleSwitch(acidus::Graphics& g, const acidus::Control& ctrl, const acidus::Font& font) override {
            switchDrawn = true;
        }
    };

    auto customRenderer = std::make_unique<TestCustomRenderer>();
    auto* rawPtr = customRenderer.get();
    gui.setControlRenderer(std::move(customRenderer));
    gui.renderFrame();
    assert(rawPtr->knobDrawn);
    assert(rawPtr->switchDrawn);
    std::cout << "Custom Font and IControlRenderer interface tests passed successfully!" << std::endl;

    {
        // Logo plate bubbles: more Cutoff, more bubbles.
        auto countAfter = [](double cutoff) {
            acidus::AcidusClap p(nullptr);
            acidus::GuiWindow w(&p);
            p.onParamValueFromGui(acidus::PARAM_CUTOFF, cutoff);
            w.renderFrame();                     // lays out the logo plate
            size_t n = 0;
            for (int i = 0; i < 300; ++i) {      // 10 s at 30 fps
                w.advanceAnimation(1.0 / 30.0);
                n += w.getBubbleCount();
            }
            return n;
        };
        size_t lo = countAfter(0.0), hi = countAfter(1.0);
        std::cout << "Bubble frames: cutoff 0 -> " << lo << ", cutoff 1 -> " << hi << std::endl;
        if (!(hi > 5 * lo && lo > 0)) { std::cerr << "FAILED: bubble rate does not follow cutoff" << std::endl; return 1; }
    }

#ifdef ACIDUS_CALIBRATION_BUILD
    {
        // Ctrl-click on the logo plate resets the calibration parameters,
        // not the front-panel knobs, and tells the host about each one.
        // Explicit checks: assert() is compiled out in Release builds.
        auto check = [](bool ok, const char* what) {
            if (!ok) { std::cerr << "FAILED: " << what << std::endl; std::exit(1); }
        };
        acidus::AcidusClap calPlugin(nullptr);
        acidus::GuiWindow calGui(&calPlugin);
        calGui.renderFrame();
        clap_param_info_t info{};
        calPlugin.paramsInfo(acidus::PARAM_CUTOFF_BASE_HZ, &info);
        calPlugin.onParamValueFromGui(acidus::PARAM_CUTOFF_BASE_HZ, info.default_value + 100.0);
        calPlugin.onParamValueFromGui(acidus::PARAM_CUTOFF, 0.123);
        TestOutEvents drain;
        clap_output_events_t drainList{&drain, TestOutEvents::tryPush};
        calPlugin.paramsFlush(nullptr, &drainList);

        int lx, ly, lw, lh;
        calGui.getLogoPlateRect(lx, ly, lw, lh);
        check(lw > 0 && lh > 0, "lw > 0 && lh > 0");
        calGui.handleMouseDown(lx + lw / 2, ly + lh / 2, false, false); // plain click: no reset
        double v = 0.0;
        calPlugin.paramsValue(acidus::PARAM_CUTOFF_BASE_HZ, &v);
        check(std::abs(v - (info.default_value + 100.0)) < 1e-9, "std::abs(v - (info.default_value + 100.0)) < 1e-9");

        calGui.handleMouseDown(lx + lw / 2, ly + lh / 2, false, true);  // Ctrl-click: reset
        calPlugin.paramsValue(acidus::PARAM_CUTOFF_BASE_HZ, &v);
        check(std::abs(v - info.default_value) < 1e-9, "std::abs(v - info.default_value) < 1e-9");
        calPlugin.paramsValue(acidus::PARAM_CUTOFF, &v);
        check(std::abs(v - 0.123) < 1e-9, "std::abs(v - 0.123) < 1e-9");

        TestOutEvents resetEvents;
        clap_output_events_t resetList{&resetEvents, TestOutEvents::tryPush};
        calPlugin.paramsFlush(nullptr, &resetList);
        size_t expected = 3 * (acidus::PARAM_EXPERIMENTAL_COUNT - acidus::PARAM_FRONT_PANEL_COUNT);
        check(resetEvents.types.size() == expected, "resetEvents.types.size() == expected");
        for (clap_id id : resetEvents.paramIds) check(id >= acidus::PARAM_FRONT_PANEL_COUNT, "id >= acidus::PARAM_FRONT_PANEL_COUNT");
        std::cout << "Calibration reset (Ctrl-click on logo) test passed: " << expected << " events" << std::endl;
    }
#endif

    std::cout << "All Acidus GUI tests passed successfully!" << std::endl;

    return 0;
}
