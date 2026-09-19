# Acidus

![Acidus Hero](acidus_hero.jpg)

**Acidus** is a lightweight, faithful Roland TB-303 bass synthesizer emulator plugin written in modern C++17 using the [CLAP](https://cleveraudio-plug.info/) (Clever Audio Plug-in) standard.

---

## Key Features

- **Pure C++ DSP Engine**: Faithful 8x oversampled coupled diode ladder filter solver with physical BJT thermal voltage scaling and nonlinear saturation.
- **Custom Native Vector/Pixel GUI**: Lightweight pixel-rendered front panel featuring controls for Cutoff, Resonance, Env Mod, Decay, Accent, Waveform, and Master Volume, plus a custom Acid Green logo with multi-layer glow.
- **CLAP Standard Support**: Full support for CLAP parameter automation, state save/restore, and host event flushing.
- **Cross-Platform Support**: Linux (X11), Windows (Win32), and macOS (Cocoa).

---

## Building

### Prerequisites

- CMake (>= 3.15)
- C++17 compliant compiler (`GCC`, `Clang`, or `MSVC`)
- Linux: `libx11-dev`

### Build Steps

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The resulting CLAP plugin (`acidus.clap`) will be located in the `build/` directory.

### Running Standalone Test Executables

```bash
./build/acidus_dsp_test
./build/acidus_filter_stability_test
./build/acidus_gui_test
```
