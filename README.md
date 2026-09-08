# Syrebas

**Syrebas** is a lightweight, accurate Roland TB-303 bass synthesizer emulator plugin written in C++ using the [CLAP](https://cleveraudio-plug.info/) (Clever Audio Plug-in) standard.

## Features

- **Accurate Physical Modeling DSP**:
  - Zero-Delay Feedback (ZDF) 4-pole TPT ladder filter topology.
  - Asymmetric capacitor stage sizing ($C_1 = 33\text{nF}$, $C_2 = 22\text{nF}$, $C_3 = 10\text{nF}$, $C_4 = 1\mu\text{F}$).
  - Diode ladder $\tanh()$ saturation per stage and in the high-pass feedback loop ($C_{13}/R_{23}$ network).
  - $4\times$ oversampling engine for aliasing protection and non-linear squelching.
- **Classic 303 Sound Engine**:
  - Sawtooth and Square waveforms.
  - Pitch slide / portamento (~60ms glide when notes overlap).
  - Constant note volume with high MIDI velocity triggering accent (envelope pop and filter drive boost).
- **Simple & Responsive GUI**:
  - Lightweight embedded UI with custom rendering (logo and 303 knobs).
  - Essential knobs: Cutoff Frequency, Resonance, Env Mod, Decay, Accent.
- **Zero Heavy Dependencies**: Built without JUCE for minimal binary size and resource usage.

## Building from Source

### Prerequisites
- C++17 compliant compiler (GCC, Clang, or MSVC)
- CMake 3.15 or higher
- Linux: `libx11-dev` (for GUI windowing on X11)

### Build Instructions
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

The resulting CLAP plugin (`syrebas.clap`) will be located in the `build/` directory.

## License

MIT License. See [LICENSE](LICENSE) for details.
