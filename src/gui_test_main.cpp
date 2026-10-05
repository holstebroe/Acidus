#include "clap/AcidusClap.hpp"
#include "gui/GuiWindow.hpp"
#include "gui/Graphics.hpp"
#include "gui/Font.hpp"
#include "gui/IGuiSkin.hpp"
#include "core/CalibrationPresets.hpp"
#include <algorithm>
#include <cstring>
#include <string>
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

    gui.handleMouseDown(120, 100, false);
    gui.handleMouseDrag(120, 20, false); // Drag up 80 pixels
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

    gui.handleMouseDown(186, 100, true); // Resonance knob at (186, 100) with Shift
    gui.handleMouseDrag(186, 20, true);  // Drag up 80 pixels with Shift
    gui.handleMouseUp();

    double valFine = 0.0;
    plugin.paramsValue(acidus::PARAM_RESONANCE, &valFine);
    std::cout << "Resonance after 80px fine drag with Shift: " << valFine << std::endl;
    assert(std::abs(valFine - 0.70) < 0.01);

    // Double-click on a knob resets it to its default, without dragging.
    gui.handleMouseDown(186, 100, false);
    gui.handleMouseUp();
    gui.handleMouseDown(186, 100, false);
    gui.handleMouseDrag(186, 20, false);   // ignored: the double-click started no drag
    gui.handleMouseUp();
    double valReset = 0.0, resDefault = -1.0;
    plugin.paramsValue(acidus::PARAM_RESONANCE, &valReset);
    plugin.paramsDefaultValue(acidus::PARAM_RESONANCE, &resDefault);
    std::cout << "Resonance after double-click: " << valReset << std::endl;
    assert(std::abs(valReset - resDefault) < 1e-9);

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

    // Tuning CC: 64 is exactly centre, and both ends reach the full ±700.
    midiCcEv.data[1] = acidus::MIDI_PARAM_TUNE;
    const struct { uint8_t cc; double cents; } tuneCases[] = {
        { 0, -700.0 }, { 32, -350.0 }, { 64, 0.0 }, { 127, 700.0 } };
    for (const auto& tc : tuneCases) {
        midiCcEv.data[2] = tc.cc;
        plugin.paramsFlush(&mockInList, &mockOutList);
        double tune = 1e9;
        plugin.paramsValue(acidus::PARAM_TUNE, &tune);
        std::cout << "Tuning after MIDI CC " << acidus::MIDI_PARAM_TUNE << " (" << int(tc.cc) << "): " << tune << std::endl;
        assert(std::abs(tune - tc.cents) < 1e-9);
    }

    acidus::Font customFont(6, 8);
    assert(customFont.getWidth() == 6);
    assert(customFont.getHeight() == 8);
    gui.setFont(customFont);
    assert(gui.getFont().getWidth() == 6);

    class TestCustomRenderer : public acidus::IGuiSkin {
    public:
        bool panelDrawn = false;
        bool knobDrawn = false;
        bool switchDrawn = false;
        void drawKnob(acidus::Graphics& g, const acidus::Control& ctrl, const acidus::Font& font) override {
            knobDrawn = true;
        }
        void drawToggleSwitch(acidus::Graphics& g, const acidus::Control& ctrl, const acidus::Font& font) override {
            switchDrawn = true;
        }
        void drawPanel(acidus::Graphics& g, int width, int height, int dividerX) override {
            panelDrawn = true;
        }
    };

    auto customRenderer = std::make_unique<TestCustomRenderer>();
    auto* rawPtr = customRenderer.get();
    gui.setSkin(std::move(customRenderer));
    gui.renderFrame();
    assert(rawPtr->panelDrawn);
    assert(rawPtr->knobDrawn);
    assert(rawPtr->switchDrawn);
    std::cout << "Custom Font and IGuiSkin interface tests passed successfully!" << std::endl;

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

    {
        // Calibration presets: a click on the logo plate's label cycles
        // them, the front-panel knobs stay, the state keeps the preset.
        // Explicit checks: assert() is compiled out in Release builds.
        auto check = [](bool ok, const char* what) {
            if (!ok) { std::cerr << "FAILED: " << what << std::endl; std::exit(1); }
        };
        const auto* presets = acidus::calibrationPresets();
        acidus::AcidusClap calPlugin(nullptr);
        acidus::GuiWindow calGui(&calPlugin);
        calGui.renderFrame();
        check(std::string(calPlugin.calibrationPresetName()) == "X0X", "starts on X0X");
        check(!calPlugin.isCalibrationModified(), "fresh preset not modified");
        calPlugin.onParamValueFromGui(acidus::PARAM_CUTOFF, 0.123);
        check(!calPlugin.isCalibrationModified(), "a front-panel knob is not a calibration change");
        TestOutEvents drain;
        clap_output_events_t drainList{&drain, TestOutEvents::tryPush};
        calPlugin.paramsFlush(nullptr, &drainList);

        int lx, ly, lw, lh;
        calGui.getPresetLabelRect(lx, ly, lw, lh);
        check(lw > 0 && lh > 0, "preset label drawn");
        int px, py, pw, ph;
        calGui.getLogoPlateRect(px, py, pw, ph);
        check(lx >= px && ly >= py && lx + lw <= px + pw && ly + lh <= py + ph, "preset label inside the logo plate");
        calGui.handleMouseDown(px + pw / 2, py + 12);   // the logo itself: nothing happens
        check(calPlugin.calibrationPresetIndex() == 0, "click outside the label does not cycle");

        calGui.handleMouseDown(lx + lw / 2, ly + lh / 2);
        check(std::string(calPlugin.calibrationPresetName()) == "ACIDVOICE", "click cycles to ACIDVOICE");
        check(std::abs(calPlugin.getEngine().getParams().cutoffBaseHz - presets[1].params.cutoffBaseHz) < 1e-4f,
              "engine uses the preset's constants");
        double v = 0.0;
        calPlugin.paramsValue(acidus::PARAM_CUTOFF, &v);
        check(std::abs(v - 0.123) < 1e-9, "knob position kept");
        check(std::abs(calPlugin.getEngine().getParams().cutoff - 0.123f) < 1e-6f, "engine knob kept");
        for (int i = 0; i < acidus::AcidusClap::calibrationPresetCount() - 1; ++i) {
            calGui.handleMouseDown(lx + lw / 2, ly + lh / 2);
        }
        check(calPlugin.calibrationPresetIndex() == 0, "cycling wraps around to the first preset");
        calPlugin.selectCalibrationPreset(3, true);
        check(std::string(calPlugin.calibrationPresetName()) == "HELL FISH", "HELL FISH preset");
        check(std::abs(calPlugin.getEngine().getParams().vcaAttackMs - presets[3].params.vcaAttackMs) < 1e-5f,
              "non-default constant applied");
        for (int i = 0; i < acidus::AcidusClap::calibrationPresetCount(); ++i) {
            calPlugin.selectCalibrationPreset(i, true);
            calGui.renderFrame();
            calGui.getPresetLabelRect(lx, ly, lw, lh);
            check(lx >= px && ly >= py && lx + lw <= px + pw && ly + lh <= py + ph,
                  "every preset's label fits inside the logo plate");
        }
        calPlugin.selectCalibrationPreset(4, true);
        check(std::string(calPlugin.calibrationPresetName()) == "X0X CIRCUIT", "X0X CIRCUIT preset");
        check(calPlugin.getEngine().getParams().filterCouplingNetwork == 1.0f, "X0X CIRCUIT uses Stinchcombe's network");
        calPlugin.selectCalibrationPreset(3, true);

#ifdef ACIDUS_CALIBRATION_BUILD
        TestOutEvents presetEvents;
        clap_output_events_t presetList{&presetEvents, TestOutEvents::tryPush};
        calPlugin.paramsFlush(nullptr, &presetList);
        check(presetEvents.types.size() >= 3u * (acidus::PARAM_EXPERIMENTAL_COUNT - acidus::PARAM_FRONT_PANEL_COUNT),
              "host told about every calibration parameter");
        for (clap_id id : presetEvents.paramIds) check(id >= acidus::PARAM_FRONT_PANEL_COUNT, "only calibration parameters sent");
        calPlugin.paramsValue(acidus::PARAM_VCA_ATTACK_MS, &v);
        check(std::abs(v - presets[3].params.vcaAttackMs) < 1e-6, "calibration parameter loaded from the preset");
        // Every preset loads exactly: no calibration parameter is clamped to
        // its range (which would play the preset wrong and show a star).
        for (int i = 0; i < acidus::AcidusClap::calibrationPresetCount(); ++i) {
            calPlugin.selectCalibrationPreset(i, true);
            check(!calPlugin.isCalibrationModified(), "a freshly selected preset is not modified");
            const auto& e = calPlugin.getEngine().getParams();
            const auto& want = presets[i].params;
            check(e.filterFeedbackGain == want.filterFeedbackGain && e.accentSweepDepthOct == want.accentSweepDepthOct &&
                  e.oscCouplingHz == want.oscCouplingHz && e.vcfDecayMaxSec == want.vcfDecayMaxSec,
                  "engine plays the preset's exact values");
        }
        calPlugin.paramsFlush(nullptr, &presetList);
        calPlugin.selectCalibrationPreset(3, true);
        calPlugin.onParamValueFromGui(acidus::PARAM_CUTOFF_BASE_HZ, presets[3].params.cutoffBaseHz + 50.0);
        check(calPlugin.isCalibrationModified(), "edited calibration parameter marks the preset modified");
        calGui.renderFrame();   // label now "HELL FISH*"
#endif

        // State round trip: preset, calibration edits and knobs survive.
        struct MemStream {
            std::vector<uint8_t> data;
            size_t pos{0};
            static int64_t write(const clap_ostream_t* s, const void* buf, uint64_t n) {
                auto* m = static_cast<MemStream*>(s->ctx);
                const auto* b = static_cast<const uint8_t*>(buf);
                size_t k = std::min<uint64_t>(n, 7);   // short writes, as hosts may do
                m->data.insert(m->data.end(), b, b + k);
                return static_cast<int64_t>(k);
            }
            static int64_t read(const clap_istream_t* s, void* buf, uint64_t n) {
                auto* m = static_cast<MemStream*>(s->ctx);
                size_t k = std::min<uint64_t>({n, 5, m->data.size() - m->pos});
                std::memcpy(buf, m->data.data() + m->pos, k);
                m->pos += k;
                return static_cast<int64_t>(k);
            }
        };
        MemStream mem;
        clap_ostream_t os{&mem, MemStream::write};
        check(calPlugin.stateSave(&os), "state saved");
        acidus::AcidusClap restored(nullptr);
        clap_istream_t is{&mem, MemStream::read};
        check(restored.stateLoad(&is), "state loaded");
        check(restored.calibrationPresetIndex() == 3, "preset restored");
        restored.paramsValue(acidus::PARAM_CUTOFF, &v);
        check(std::abs(v - 0.123) < 1e-9, "knob restored");
#ifdef ACIDUS_CALIBRATION_BUILD
        check(restored.isCalibrationModified(), "calibration edit restored");
#endif
        check(std::abs(restored.getEngine().getParams().vcaAttackMs - presets[3].params.vcaAttackMs) < 1e-5f,
              "restored engine uses the preset");
#ifdef ACIDUS_CALIBRATION_BUILD
        {
            // A loaded project keeps its edits (above); cycling presets resets them.
            acidus::AcidusClap cycled(nullptr);
            MemStream again;
            again.data = mem.data;
            clap_istream_t ais{&again, MemStream::read};
            check(cycled.stateLoad(&ais) && cycled.isCalibrationModified(), "edit survives a project load");
            cycled.cycleCalibrationPresetFromGui();
            check(!cycled.isCalibrationModified(), "cycling presets resets the calibration");
            for (int i = 1; i < acidus::AcidusClap::calibrationPresetCount(); ++i) cycled.cycleCalibrationPresetFromGui();
            check(cycled.calibrationPresetIndex() == 3 && !cycled.isCalibrationModified(),
                  "cycling back to the saved preset gives the preset, not the edit");
        }
#endif

        // Legacy state (bare doubles, before presets): the first preset.
        MemStream legacy;
        std::vector<double> vals(acidus::PARAM_COUNT);
        for (clap_id id = 0; id < acidus::PARAM_COUNT; ++id) restored.paramsValue(id, &vals[id]);
        vals[acidus::PARAM_CUTOFF] = 0.77;
        const auto* raw = reinterpret_cast<const uint8_t*>(vals.data());
        legacy.data.assign(raw, raw + vals.size() * sizeof(double));
        clap_istream_t lis{&legacy, MemStream::read};
        check(restored.stateLoad(&lis), "legacy state loaded");
        check(restored.calibrationPresetIndex() == 0, "legacy state uses the first preset");
        restored.paramsValue(acidus::PARAM_CUTOFF, &v);
        check(std::abs(v - 0.77) < 1e-9, "legacy knob loaded");
        std::cout << "Calibration preset tests passed ("
                  << acidus::AcidusClap::calibrationPresetCount() << " presets)" << std::endl;
    }

    {
        // The smiley reacts to accented notes; the logo glow follows the
        // host's bar while the transport plays.
        auto check = [](bool ok, const char* what) {
            if (!ok) { std::cerr << "FAILED: " << what << std::endl; std::exit(1); }
        };
        struct InEvents {
            std::vector<clap_event_note_t> notes;
            static uint32_t size(const clap_input_events_t* l) {
                return static_cast<uint32_t>(static_cast<InEvents*>(l->ctx)->notes.size());
            }
            static const clap_event_header_t* get(const clap_input_events_t* l, uint32_t i) {
                return &static_cast<InEvents*>(l->ctx)->notes[i].header;
            }
        };
        auto sendNote = [](acidus::AcidusClap& p, double velocity) {
            InEvents ev;
            clap_event_note_t n{};
            n.header.size = sizeof(n);
            n.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
            n.header.type = CLAP_EVENT_NOTE_ON;
            n.key = 36;
            n.velocity = velocity;
            ev.notes.push_back(n);
            clap_input_events_t in{&ev, InEvents::size, InEvents::get};
            p.paramsFlush(&in, nullptr);
        };
        acidus::AcidusClap p(nullptr);
        acidus::GuiWindow w(&p);
        w.setAnimationFrozen(true);
        w.renderFrame();
        sendNote(p, 0.5);
        w.renderFrame();
        check(!w.isAccentFlashActive(), "a normal note does not make the smiley react");
        auto before = w.getPixelBuffer();
        sendNote(p, 1.0);
        w.renderFrame();
        check(w.isAccentFlashActive(), "an accented note makes the smiley react");
        check(w.getPixelBuffer() != before, "the eyes change on an accent");
        for (int i = 0; i < 3; ++i) w.advanceAnimation(0.1);   // steps are capped at 0.1 s
        check(!w.isAccentFlashActive(), "the reaction is brief");

        double phase = -1.0;
        check(!p.transportBarPhase(phase), "no transport: free-running pulse");
        clap_event_transport_t tr{};
        tr.flags = CLAP_TRANSPORT_HAS_TEMPO | CLAP_TRANSPORT_HAS_BEATS_TIMELINE | CLAP_TRANSPORT_IS_PLAYING;
        tr.tempo = 120.0;
        tr.song_pos_beats = static_cast<clap_beattime>(6.5 * CLAP_BEATTIME_FACTOR);
        tr.bar_start = static_cast<clap_beattime>(4.0 * CLAP_BEATTIME_FACTOR);
        tr.tsig_num = 4;
        tr.tsig_denom = 4;
        clap_process_t proc{};
        proc.transport = &tr;
        p.process(&proc);
        check(p.transportBarPhase(phase), "playing transport: synced pulse");
        check(std::abs(phase - 0.625) < 0.01, "bar phase from the host's beat position");
        tr.flags = CLAP_TRANSPORT_HAS_TEMPO;   // stopped
        p.process(&proc);
        check(!p.transportBarPhase(phase), "stopped transport: free-running pulse");
        std::cout << "Accent smiley and bar-synced glow tests passed" << std::endl;
    }

    {
        // Skins may cache what they draw (the modern one keeps each knob's
        // pixels by value): turning a knob away and back must give the same
        // frame as before, and a moved knob must actually look different.
        acidus::AcidusClap p(nullptr);
        acidus::GuiWindow w(&p);
        auto check = [](bool ok, const char* what) {
            if (!ok) { std::cerr << "FAIL: " << what << std::endl; std::exit(1); }
        };
        w.setAnimationFrozen(true);
        w.renderFrame();
        const auto original = w.getPixelBuffer();
        double cutoff = 0.0;
        p.paramsValue(acidus::PARAM_CUTOFF, &cutoff);
        p.setParamValueFromGui(acidus::PARAM_CUTOFF, cutoff > 0.5 ? 0.1 : 0.9);
        w.renderFrame();
        check(w.getPixelBuffer() != original, "a turned knob redraws");
        p.setParamValueFromGui(acidus::PARAM_CUTOFF, cutoff);
        w.renderFrame();
        check(w.getPixelBuffer() == original, "turning it back restores the same frame");
        std::cout << "Skin redraw consistency test passed" << std::endl;
    }

    std::cout << "All Acidus GUI tests passed successfully!" << std::endl;

    return 0;
}
