// Performance benchmark: DSP cost per sample (several settings, sample
// rates and block sizes), CLAP parameter sync, GUI frame rendering and, with
// --x11 on Linux (needs a display, e.g. Xvfb), the GUI thread's CPU with a
// real window, shown and hidden. Build in Release:
//     cmake --build build --target acidus_perf_bench
//     build/acidus_perf_bench [--gui-only] [--x11]
// Percentages are of one core in real time. See docs/PERFORMANCE.md.
#include "clap/AcidusClap.hpp"
#include "gui/GuiWindow.hpp"
#include "core/SynthEngine.hpp"
#include <chrono>
#include <cstdio>
#include <cstring>
#include <thread>
#include <vector>
#if defined(__linux__)
#include <sys/resource.h>
#endif
using namespace acidus;
using clk = std::chrono::steady_clock;

#if defined(__linux__)
static double cpuSec() { rusage u; getrusage(RUSAGE_SELF, &u);
    return u.ru_utime.tv_sec + u.ru_stime.tv_sec + 1e-6 * (u.ru_utime.tv_usec + u.ru_stime.tv_usec); }
#endif

// 125 BPM 16ths acid line, some accents and slides; returns ns per output sample.
static double dsp(double sr, float cut, float res, float env, float dec, float acc, float drive, Waveform wf, int block = 256) {
    SynthEngine e; e.setSampleRate(sr); e.reset();
    auto& p = e.getParams();
    p.cutoff = cut; p.resonance = res; p.envMod = env; p.decay = dec; p.accent = acc; p.drive = drive; p.waveform = wf;
    const int notes[16] = {36, 36, 48, 36, 39, 36, 43, 36, 36, 48, 46, 36, 41, 36, 48, 43};
    const int step = static_cast<int>(sr * 60.0 / 125.0 / 4.0);
    const double seconds = 20.0;
    const long total = static_cast<long>(sr * seconds);
    std::vector<float> L(block), R(block);
    long pos = 0; int s = 0; long nextStep = 0; bool on = false; long offAt = 0;
    auto t0 = clk::now();
    while (pos < total) {
        if (pos >= nextStep) {
            float vel = (s % 4 == 0) ? 1.0f : 0.5f;          // accents on the beat
            e.noteOn(notes[s % 16], vel);
            on = true; offAt = nextStep + ((s % 8 == 5) ? step + 10 : step / 2);   // one slide per bar
            nextStep += step; ++s;
        }
        if (on && pos >= offAt) { e.noteOff(notes[(s - 1) % 16]); on = false; }
        long lim = std::min<long>({total - pos, (long)block, nextStep - pos, on ? std::max<long>(1, offAt - pos) : (long)block});
        e.processAudio(L.data(), R.data(), (int)lim);
        pos += lim;
    }
    double ns = std::chrono::duration<double, std::nano>(clk::now() - t0).count();
    return ns / total;
}

int main(int argc, char** argv) {
    bool x11 = false, guiOnly = false;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--x11") == 0) x11 = true;
        if (std::strcmp(argv[i], "--gui-only") == 0) guiOnly = true;
    }
    if (!guiOnly) {
    std::printf("== DSP (20 s acid line, 256-frame blocks), %% of one core in real time\n");
    struct Case { const char* name; float c, r, e, d, a, drv; Waveform w; };
    Case cases[] = {
        {"default knobs, saw", .5f, .5f, .5f, .5f, .5f, 0.f, Waveform::Saw},
        {"heavy acid (res/env/acc max)", .6f, 1.f, 1.f, .7f, 1.f, 0.f, Waveform::Saw},
        {"heavy acid + drive", .6f, 1.f, 1.f, .7f, 1.f, 1.f, Waveform::Saw},
        {"square, res 0", .3f, 0.f, .2f, .3f, 0.f, 0.f, Waveform::Square},
    };
    for (double sr : {44100.0, 48000.0, 96000.0}) {
        for (auto& c : cases) {
            double best = 1e9;
            for (int i = 0; i < 3; ++i) best = std::min(best, dsp(sr, c.c, c.r, c.e, c.d, c.a, c.drv, c.w));
            std::printf("  %6.0f Hz  %-30s %7.1f ns/sample  %5.2f %%\n", sr, c.name, best, best * sr * 1e-9 * 100);
        }
    }
    {   // block size sensitivity
        for (int b : {32, 64, 1024}) {
            double ns = dsp(48000, .6f, 1.f, 1.f, .7f, 1.f, 0.f, Waveform::Saw, b);
            std::printf("  48000 Hz  heavy acid, %4d-frame blocks  %7.1f ns/sample  %5.2f %%\n", b, ns, ns * 48000e-9 * 100);
        }
    }

    std::printf("\n== CLAP parameter sync (per parameter event, e.g. automation)\n");
    {
        AcidusClap p(nullptr);
        auto t0 = clk::now();
        const int N = 200000;
        for (int i = 0; i < N; ++i) p.onParamValueFromGui(PARAM_CUTOFF, (i % 100) / 100.0);
        double ns = std::chrono::duration<double, std::nano>(clk::now() - t0).count() / N;
        std::printf("  onParamValueFromGui (sync + queue): %.0f ns per event\n", ns);
        // drain the queue so memory doesn't matter
    }

    }
    std::printf("\n== GUI rendering (software, no window)\n");
    for (double cut : {0.0, 0.5, 1.0}) {
        AcidusClap p(nullptr);
        GuiWindow w(&p);
        p.onParamValueFromGui(PARAM_CUTOFF, cut);
        w.renderFrame();
        for (int i = 0; i < 300; ++i) w.advanceAnimation(1.0 / 30.0);   // steady-state bubble count
        size_t bubbles = w.getBubbleCount();
        const int N = 200;
        auto t0 = clk::now();
        for (int i = 0; i < N; ++i) w.renderFrame();
        double ms = std::chrono::duration<double, std::milli>(clk::now() - t0).count() / N;
        auto t1 = clk::now();
        for (int i = 0; i < 20000; ++i) w.advanceAnimation(1.0 / 30.0);
        double us = std::chrono::duration<double, std::micro>(clk::now() - t1).count() / 20000;
        std::printf("  cutoff %.1f: %3zu bubbles, renderFrame %.2f ms, bubble update %.2f us; at 30 fps = %.1f %% of a core\n",
                    cut, bubbles, ms, us, ms * 30.0 / 10.0);
    }

#if defined(__linux__) && !defined(__APPLE__)
    if (x11) {
        std::printf("\n== Real X11 window (Xvfb), plugin idle, 10 s\n");
        for (double cut : {0.0, 1.0}) {
            AcidusClap p(nullptr);
            GuiWindow w(&p);
            p.onParamValueFromGui(PARAM_CUTOFF, cut);
            clap_window_t win{CLAP_WINDOW_API_X11, {}};
            win.x11 = 0;   // no host parent: a top-level window
            w.setParent(&win);
            std::this_thread::sleep_for(std::chrono::seconds(1));
            double c0 = cpuSec(); auto t0 = clk::now();
            std::this_thread::sleep_for(std::chrono::seconds(10));
            double c1 = cpuSec(); double wall = std::chrono::duration<double>(clk::now() - t0).count();
            std::printf("  cutoff %.1f: plugin GUI thread %.2f %% of a core\n", cut, (c1 - c0) / wall * 100);
            w.hide();
            c0 = cpuSec(); t0 = clk::now();
            std::this_thread::sleep_for(std::chrono::seconds(5));
            c1 = cpuSec(); wall = std::chrono::duration<double>(clk::now() - t0).count();
            std::printf("  cutoff %.1f: hidden window %.2f %% of a core\n", cut, (c1 - c0) / wall * 100);
            w.destroy();
        }
    }
#else
    (void)x11;
#endif
    return 0;
}
