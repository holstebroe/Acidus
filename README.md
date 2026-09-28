# Acidus

![Acidus Hero](acidus_hero.jpg)

**Acidus** is a lightweight, faithful Roland TB-303 bass synthesizer emulator plugin written in modern C++17 using the [CLAP](https://cleveraudio-plug.info/) (Clever Audio Plug-in) standard.

---

## Key Features

- **Pure C++ DSP Engine**: Faithful 8x oversampled coupled diode ladder filter solver with physical BJT thermal voltage scaling and nonlinear saturation.
- **Custom Native Vector/Pixel GUI**: Lightweight pixel-rendered front panel featuring controls for Cutoff, Resonance, Env Mod, Decay, Accent, Waveform, Tuning, and Master Volume, plus a custom Acid Green logo with multi-layer glow.
- **CLAP Standard Support**: Full support for CLAP parameter automation, state save/restore, and host event flushing.
- **Cross-Platform Support**: Linux (X11), Windows (Win32), and macOS (Cocoa).

---

## How Acidus is calibrated against real TB-303s

Acidus aims to sound like the hardware, not like a generic "303-ish" synth.
Two sources of truth drive the model:

1. **The circuit.** The DSP follows the TB-303 service-manual schematics and
   the published circuit analyses (see Credits). The findings are consolidated
   with evidence tags in `docs/TB-303 Reference/TB303_REFERENCE.md`, and
   `acidus_reference_test` checks the engine against them: filter poles and
   frequency response, resonance-loop stability, envelope and slide time
   constants, and control-law shapes.
2. **Recordings of real units.** Every constant the schematic leaves open
   (trimmer settings, pot tapers, component tolerances and ageing) is fitted
   to hardware recordings, never tuned by ear alone.

### Method

- **Reference sets.** 400 notes from the dinsync.info TB-303 reference
  recordings: all five knobs swept at five settings, saw and square, with and
  without accent, levels *not* normalised, so loudness is compared too. The
  set is described per note (knobs, sample ranges, measured timing and
  pitch) in `test/resources/x0x-reference/x0x_reference.md`, with a
  machine-readable manifest next to it. Plus 13 single-note samples from
  Acidvoice, from a second unit.
- **Measurement first.** Tuning, note-on grid, gate length and recording gain
  are measured, not guessed: all notes are C2 at +5.4 cents, and settings
  recorded twice repeat within 0.4 dB, so one global gain holds for all 400
  notes.
- **Rendering through the real engine.** `tools/calibrate_reference.py`
  renders every reference note through the plugin's own `SynthEngine` and
  compares harmonic levels, inter-harmonic content, loudness over time,
  1/3-octave spectrograms, and the **resonant-peak track**: where the
  resonance is, every 10 ms, which captures the Env Mod and accent sweeps.
  A CMA-ES search fits the constants. Held-out note subsets check that a
  fit generalises. Where a value can be read off the data directly (the
  square's duty cycle from its nulled 15th harmonic, the Decay pot's
  taper from the sweep decay times), it is measured rather than searched.
- **Per-unit profiles.** Units differ (the two measured here have cutoff
  trims about an octave apart), so fitted constants are kept per source in
  `calibrations/`, including "aged" and "new component" flavours of the
  same unit.

### Key results so far

All figures are on the same 400 hardware notes; lower is better except
the percentage.

| | Previous fit (13 Acidvoice samples) | `x0x` (current default) | `x0x-sweep` |
|---|---|---|---|
| Harmonic level error (RMS) | 8.0 dB | 3.2 dB | 3.0 dB |
| Harmonics within 3 dB of the hardware | 56 % | 71 % | 71 % |
| Note loudness error (RMS over notes) | 2.8 dB | 1.7 dB | 1.6 dB |
| Resonant-peak sweep error (RMS) | 7.7 semitones | 6.4 semitones | 5.4 semitones |

The recordings also revealed circuit behaviour that is now in the model:
- the Env Mod pot's non-linear taper and its bias shift;
- the Decay pot's taper;
- the square's 53 % duty cycle at C2;
- the accent diode's forward drop, which, with a moderately aged C13,
  explains why this unit's accent squelch peaks 15-20 ms after note-on.

For one heavy acid setting (all knobs at 100 % except Env Mod at 25 %),
the resonant peak now follows the hardware within a few percent from
note-on until it settles, with and without accent.

This is ongoing work toward a very low reference error. Known gaps include:
- Cutoff around 75 % sits about 0.3 octaves low;
- Env Mod depth at 75 % is short;
- resonance is too weak at note start on some high-resonance settings;
- the 15 kHz cutoff clamp;
- and several reference-conformance checks conflict with these particular units.

They are tracked in `docs/X0X_CALIBRATION_2026-09-28.md`, together with the
full procedure, per-set results and every parameter change.

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

### Calibration build

By default the plugin only exposes the eight front-panel controls (including
a Tuning trim, ±700 cents to match the real hardware's documented trim
range -- useful for nudging the plugin into tune against a reference
hardware recording that's itself slightly off-pitch) as CLAP parameters,
matching the real hardware's user-facing surface. A separate build
configuration additionally exposes every hidden circuit-topology constant
(ladder coupling-pole corners, VCA saturation drive, etc.) as automatable
CLAP parameters under `Experimental/...` module paths, each spanning the
full plausible range documented in `TB303_PARAMETER_CONFIDENCE.md`, so an
external fitting/optimization tool -- or a human A/B-ing against a reference
hardware recording -- can push every free constant in the model to its
documented extremes:

```bash
cmake -B build-calibration -DCMAKE_BUILD_TYPE=Release -DACIDUS_CALIBRATION_BUILD=ON
cmake --build build-calibration
```

Nothing in the DSP core depends on which configuration is used -- the
calibration build is the exact same signal path, just with more of its
constants exposed as host-automatable parameters instead of compiled-in
defaults. See `TB303_PARAMETER_CONFIDENCE.md` for what each one backs.

### Calibrating against hardware reference samples

`tools/calibrate_reference.py` fits the model to the hardware TB-303 notes in
`test/resources` (file-name format: `test/resources/reference_resources.md`).
It renders each note through the real `SynthEngine` (via the
`acidus_calibration_render` shared library, built automatically), and runs a
time-capped CMA-ES search over every model constant in `SynthParameters`,
the hardware knob positions and the note timing. It compares full-note
harmonic levels, inter-harmonic content, RMS envelopes and 1/3-octave
spectrograms.

```bash
pip install numpy matplotlib        # matplotlib optional (plots)
python3 tools/calibrate_reference.py                     # 20 min cap, 4 min patience
python3 tools/calibrate_reference.py --evaluate-only     # score the current code
python3 tools/calibrate_reference.py --exclude c5r1 --apply   # drop a suspect sample, write defaults
```

The dinsync.info reference set in `test/resources/x0x-reference` (25 files x
16 notes: a systematic sweep of every knob, both waveforms, with and without
accent, levels not normalised) is described in
`test/resources/x0x-reference/x0x_reference.md` and, machine-readably, in
`x0x_reference_manifest.json` next to it. Both are generated from the
author's knob chart and the audio by `tools/x0x_reference_manifest.py`.
`--manifest` fits against those notes instead (with one global recording
gain, so absolute levels count):

```bash
python3 tools/x0x_reference_manifest.py                  # regenerate manifest + tables (--check: verify)
python3 tools/calibrate_reference.py --manifest --evaluate-only --no-sensitivity
# fast search: a quarter of the notes (--rotate), first 800 ms of each
python3 tools/calibrate_reference.py --manifest --rotate --analysis-ms 800 --no-sensitivity --max-minutes 120
```

`--include`/`--exclude` then match note ids such as `C2-p3-square-acc`.

Fitted constants are kept per source in `calibrations/` (`x0x`, `acidvoice`,
`x0x-acidvoice-trim`); `tools/calibration_profile.py apply <name>` switches
the defaults, and `calibrations/README.md` explains what differs between the
two hardware units (mainly the cutoff trim).

Results go to `calibration_results/<timestamp>/`:
- `report.md` has before/after scores, a per-sample ranking and a list of
  suspicious samples. Samples are flagged when they fit much worse than the
  rest, when a free knob fit moves far from the label, or when two samples
  with different labels sound identical.
- `synth_parameters.txt` is paste-ready C++ for `SynthParameters`.
- `result.json`, plus `renders/` with hardware/before/after audio and plots.

### Running Standalone Test Executables

```bash
./build/acidus_dsp_test
./build/acidus_filter_stability_test
./build/acidus_gui_test
./build/acidus_reference_test    # checks the DSP against docs/TB-303 Reference/TB303_REFERENCE.md
```

`acidus_reference_test` measures frequency response, resonance-loop
stability margin, envelope/slide time constants and control laws against the
sourced claims in the consolidated reference, and exits with the number of
failed checks. See `docs/TB303_REFERENCE_AUDIT_2026-09-27.md` for the current
results and the corrective plan.

---

## Credits

**Hardware reference recordings**
- **Din Sync** ([dinsync.info](http://www.dinsync.info/2010/02/tb-303-reference-recordings-for-x0xb0x.html)):
  *TB-303 reference recordings for x0xb0x builders* (2010), 25 sets x 16
  notes sweeping every knob of a TB-303, recorded without level normalisation.
  The backbone of the calibration (`test/resources/x0x-reference`).
- **Acidvoice** ([acidvoice.com](https://acidvoice.com/)): the TB-303
  single-note samples in `test/resources`.

The recordings remain the work of their authors and are included only as
calibration references.

**Circuit analysis and TB-303 research** (cited throughout
`docs/TB-303 Reference/TB303_REFERENCE.md`)
- Roland Corporation: TB-303 service notes and schematics.
- Tim Stinchcombe: analysis of the TB-303 diode-ladder filter (pole
  positions, full-model frequency response).
- Robin Whittle: TB-303 circuit write-ups and the Devil Fish documentation
  (envelopes, accent sweep network, slide).
- Robin Schmidt: **Open303**, whose source code and fitted constants were an
  important cross-check.
- The KVR Audio "Open303" thread contributors, especially **antto**,
  **mystran**, **aciddose**, **kunn**, **Gordonjcp**, **rv0** and
  **Mike Janney**, for hardware measurements and circuit insight.
- The sonic-potions "303 timing" paper (sequencer and slide timing).
- The x0xb0x project (Adafruit / ladyada) and its builders' community.

**Distortion (MXR Distortion+ emulation)**
- ElectroSmash's circuit analysis, the Aphelion / aionfx project
  documentation, Wampler DIY, and the diode-clipper modelling literature
  (Yeh et al.). See `docs/MXR_Distortion_Plus_Emulation_Compendium.md`.

**Tools**
- [CLAP](https://github.com/free-audio/clap) by the free-audio community.

Roland and TB-303 are trademarks of Roland Corporation; MXR is a trademark
of Dunlop Manufacturing. Acidus is an independent project and is not
affiliated with or endorsed by them.

