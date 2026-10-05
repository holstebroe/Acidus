// GUI performance test: frame render times of the Acidus panel
// (acidus_gui_perf_test) or the Burette editor (burette_gui_perf_test; the
// two plugins can't share a binary), in whichever GUI style this build has
// (ACIDUS_GUI_STYLE), with no window (software rendering only). Fails if a
// typical frame blows its budget, so a skin change that makes the GUI
// sluggish shows up in a test run:
//     cmake --build build --target acidus_gui_perf_test burette_gui_perf_test
//     build/acidus_gui_perf_test && build/burette_gui_perf_test
// The budgets are deliberately loose (several times the measured cost on a
// laptop core) so a slow CI machine passes; the printed numbers are the
// thing to compare between builds. Build in Release.
#if defined(GUI_PERF_BURETTE)
#include "sequencer/SequencerClap.hpp"
#include "sequencer/SequencerGui.hpp"
#else
#include "clap/AcidusClap.hpp"
#include "gui/GuiWindow.hpp"
#endif
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <functional>
#include <vector>

using clk = std::chrono::steady_clock;

namespace {

int failures = 0;

#if defined(ACIDUS_GUI_STYLE_MODERN)
constexpr const char* kStyle = "MODERN";
#else
constexpr const char* kStyle = "RETRO";
#endif

// Runs `frame` n times after a few warm-up runs; reports the median and
// worst time and checks the median against `budgetMs`.
void measure(const char* name, int n, double budgetMs, const std::function<void(int)>& frame) {
    for (int i = 0; i < 3; ++i) frame(i);
    std::vector<double> ms;
    ms.reserve(n);
    for (int i = 0; i < n; ++i) {
        const auto t0 = clk::now();
        frame(i + 3);
        ms.push_back(std::chrono::duration<double, std::milli>(clk::now() - t0).count());
    }
    std::sort(ms.begin(), ms.end());
    const double median = ms[ms.size() / 2], worst = ms.back();
    const bool ok = median <= budgetMs;
    if (!ok) ++failures;
    std::printf("  %-44s median %7.3f ms  worst %7.3f ms  budget %5.1f ms  %s\n", name, median, worst, budgetMs,
                ok ? "ok" : "FAIL");
}

// Time of one call, for one-off costs such as the first frame.
void once(const char* name, double budgetMs, const std::function<void()>& f) {
    const auto t0 = clk::now();
    f();
    const double ms = std::chrono::duration<double, std::milli>(clk::now() - t0).count();
    const bool ok = ms <= budgetMs;
    if (!ok) ++failures;
    std::printf("  %-44s        %7.3f ms                  budget %5.1f ms  %s\n", name, ms, budgetMs, ok ? "ok" : "FAIL");
}

#if defined(GUI_PERF_BURETTE)
const clap_host_t* testHost() {
    static clap_host_t host{};
    host.clap_version = CLAP_VERSION;
    host.name = "gui_perf_test";
    host.get_extension = [](const clap_host_t*, const char*) -> const void* { return nullptr; };
    host.request_restart = [](const clap_host_t*) {};
    host.request_process = [](const clap_host_t*) {};
    host.request_callback = [](const clap_host_t*) {};
    return &host;
}

void buretteEditor() {
    using acidus::seq::SequencerGui;
    std::printf("Burette editor (%dx%d):\n", SequencerGui::kWidth, SequencerGui::kHeight);
    acidus::seq::SequencerClap plugin(testHost());
    once("first frame (window open)", 600.0, [&] { plugin.createGui(); });
    SequencerGui* gui = plugin.gui();
    // Nothing changed: the GUI's own check skips the redraw. Polled at 30 Hz.
    measure("poll frame (nothing changed)", 200, 2.0, [&](int) { gui->renderFrame(); });
    // A step changes (an edit, or the playhead moving on): a full redraw. At
    // 140 BPM the playhead moves about 9 times a second.
    const int pattern = plugin.editPattern();
    measure("step change frame (full redraw)", 60, 20.0, [&](int i) {
        acidus::seq::Step st = plugin.bank().step(pattern, i % acidus::seq::kMaxSteps);
        st.accent = !st.accent;
        plugin.bank().setStep(pattern, i % acidus::seq::kMaxSteps, st);
        gui->renderFrame();
    });
    // Switching patterns redraws the whole grid with new content.
    measure("pattern switch frame", 32, 40.0, [&](int i) {
        plugin.setEditPattern(i % acidus::seq::kNumPatterns);
        gui->renderFrame();
    });
    plugin.destroyGui();
}

#else
void acidusPanel() {
    std::printf("Acidus panel (%ux%u):\n", acidus::GuiWindow::kDefaultWidth, acidus::GuiWindow::kDefaultHeight);
    acidus::AcidusClap plugin(nullptr);
    std::unique_ptr<acidus::GuiWindow> gui;
    // The first frame paints everything, including (modern) the panel texture.
    once("first frame (window open)", 400.0, [&] {
        gui = std::make_unique<acidus::GuiWindow>(&plugin);
        gui->renderFrame();
    });
    // Idle: only the logo plate animates (bubbles, glow), 60 times a second.
    measure("idle frame (logo animation)", 120, 8.0, [&](int) {
        gui->advanceAnimation(1.0 / 60.0);
        gui->renderFrame();
    });
    // A knob being dragged: its value changes every frame, which redraws the
    // static layer.
    gui->handleMouseDown(120, 100, false);   // the Cut Off Freq knob
    measure("knob drag frame (value changes)", 120, 16.0, [&](int i) {
        gui->handleMouseDrag(120, 100 - (i % 60), false);
        gui->renderFrame();
    });
    gui->handleMouseUp();
    // Automation moving every knob at once: nothing can come from a cache.
    measure("automation frame (all knobs change)", 60, 33.0, [&](int i) {
        for (clap_id id : { acidus::PARAM_CUTOFF, acidus::PARAM_RESONANCE, acidus::PARAM_ENV_MOD, acidus::PARAM_DECAY,
                            acidus::PARAM_ACCENT, acidus::PARAM_VOLUME, acidus::PARAM_DRIVE }) {
            plugin.setParamValueFromGui(id, ((i * 7 + id * 13) % 100) / 100.0);
        }
        gui->renderFrame();
    });
}

#endif

} // namespace

int main() {
    std::printf("GUI performance, %s style (software rendering, no window)\n", kStyle);
#if defined(GUI_PERF_BURETTE)
    buretteEditor();
#else
    acidusPanel();
#endif
    if (failures) {
        std::printf("%d GUI timing(s) over budget\n", failures);
        return 1;
    }
    std::printf("All GUI timings within budget\n");
    return 0;
}
