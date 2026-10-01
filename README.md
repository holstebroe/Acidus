# Acidus

![Acidus Hero](acidus_hero.jpg)

**Acidus** is a lightweight, faithful Roland TB-303 bass synthesizer emulator plugin written in modern C++17 using the [CLAP](https://cleveraudio-plug.info/) (Clever Audio Plug-in) standard.

---

## Key Features

- **Pure C++ DSP Engine**: Faithful 8x oversampled coupled diode ladder filter solver with physical BJT thermal voltage scaling and nonlinear saturation.
- **Custom Native Vector/Pixel GUI**: Lightweight pixel-rendered front panel featuring controls for Cutoff, Resonance, Env Mod, Decay, Accent, Waveform, Tuning, and Master Volume, plus a custom Acid Green logo with multi-layer glow.
- **CLAP Standard Support**: Full support for CLAP parameter automation, state save/restore, and host event flushing.
- **Cross-Platform Support**: Linux (X11), Windows (Win32), and macOS (Cocoa).
- **Burette**: a separate TB-303-style pattern sequencer plugin (`burette.clap`) that sends Acidus the notes it needs for real 303 gate, slide, tie and accent timing. See [Burette](#burette).

---

## Burette

*A burette is the lab tube that releases acid in measured drops; this one
releases it a 16th note at a time.*

`burette.clap` is a small note-output plugin, built separately so
`acidus.clap` stays the same size. Put it before Acidus in the same chain (or
route its note output to Acidus).

- **16 patterns**, up to 16 steps each, triggered by MIDI keys **C-1 (pattern 1)
  to D#0 (pattern 16)**. A pattern plays while its key is held; the newest
  held key wins, and a change takes effect at the next step. Releasing the key
  never cuts a step short: a started step plays out its gate, slide or tie.
- **Locked to the host.** Steps are 16th notes at the host tempo, on the
  host's grid: step *n* of the song plays pattern step *n mod length*, so
  starting playback anywhere (or looping) gives the same notes as playing
  through. Playback started mid-step begins at the next step. With the
  transport stopped, a trigger key starts the pattern at once from step 1.
- **303 timing** (`docs/TB-303 Reference/TB303_REFERENCE.md` §4):
  - normal steps gate for half a step;
  - **slide** holds the gate to the end of the step, and the next note-on is
    sent *before* the previous note-off, so Acidus glides without
    retriggering its envelopes. A slide on a tied note applies at the end of
    the tie, and a slide on the last step wraps to the first;
  - **tie** (`T`) extends the previous note's gate; a slide to the same pitch
    is also a tie;
  - **accent** is velocity 127, other notes 100 (Acidus accents at 102 and up).
- **Pitch**: C..B and high C (C') with octave down / none / up spans C1-C4,
  the 303's range (C2 = MIDI 36 is the middle octave). On top come a
  per-pattern **transpose** and a global **KEY** transpose (both ±12
  semitones). KEY is an automatable host parameter, applied sample-accurately
  from the next note, like the 303's track transpose; editing it in the GUI
  records automation.
- **Pattern chaining** (the 303's track mode): each pattern's **NEXT** names
  the pattern played after it. Triggering A plays A > NEXT(A) > ... until a
  link is unset or points back into the chain, then loops from A. The chain
  counts as one long pattern on the host grid, and slides and ties carry
  across pattern boundaries.
- Other notes (above D#0) and MIDI (CCs etc.) pass straight through.

**Editing.** Click a pattern number to edit it (FOLLOW makes the editor jump
to the pattern that starts playing). In the grid, left-click a cell for the
next value, right-click for the previous one, or left-drag up/down to cycle.
Note cells go rest -> C ... B -> C' -> tie; a tie is shown as a dimmed note
fill, a rest as a dark empty cell. The LENGTH, TRANSPOSE, NEXT and KEY boxes
work the same way; steps past the length are shaded, and the pattern
buttons of the edited pattern's chain are underlined. Slots 1-7 hold
original demo patterns, 6 and 7 chained.

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
- **Per-unit profiles.** Units differ (the two measured here differ in
  cutoff range, accent level and envelope times), so fitted constants are kept per source in
  `calibrations/`, next to extrapolated "factory new" and Devil Fish
  profiles (see below).

### Key results so far

All figures are on the same 400 hardware notes; lower is better except
the percentages.

| | Previous fit (13 Acidvoice samples) | First x0x fit | `x0x` (current default) |
|---|---|---|---|
| Weighted error (all features) | 5.53 | 3.25 | **2.61** |
| Harmonic level error (RMS) | 8.0 dB | 3.2 dB | **2.3 dB** |
| Harmonics within 3 dB / 6 dB of the hardware | 56 % / 69 % | 71 % / 86 % | **76 % / 92 %** |
| Note loudness error (RMS over notes) | 2.8 dB | 1.7 dB | **1.3 dB** |
| Resonant-peak sweep error (RMS) | 7.5 semitones | 4.8 semitones | **3.3 semitones** |
| 1/3-octave spectrogram error | 9.1 dB | 6.5 dB | **5.6 dB** |

Every one of the 25 knob sets improved; the hardest remaining are E1, E3
and D3 (high resonance with high Env Mod), at 4.7-6.1 against 0.9-2.7 for
the rest.

The recordings also revealed circuit behaviour that is now in the model:
- the Env Mod pot's S-shaped depth curve (steepest around 68 % of travel)
  and its bias shift;
- the Decay pot's taper (much longer mid-travel decays than an ideal
  audio taper);
- the square's 53 % duty cycle at C2;
- the accent diode's forward drop, which, with a moderately aged C13,
  explains why this unit's accent squelch peaks 15-20 ms after note-on;
- a ~4.5 ms delay before the VCA opens on unaccented notes;
- a filter cutoff that reaches beyond 15 kHz at full Env Mod.

For one heavy acid setting (all knobs at 100 % except Env Mod at 25 %),
the resonant peak now follows the hardware within a few percent from
note-on until it settles, with and without accent.

### Calibration profiles

Units differ, so Acidus keeps fitted constants per source in
`calibrations/` and builds them in as calibration presets. The current
preset is shown on the logo plate; **click it to switch**. The knobs keep
their positions, and the preset is saved with the project. In the
calibration build, a star after the name means a calibration parameter was
changed after the preset was loaded.

| Profile | What it is |
|---|---|
| `x0x` (default) | The dinsync.info unit as recorded: 40 years old, low cutoff trim, aged C13. |
| `acidvoice` | The Acidvoice unit: the same circuit model refitted to its 13 samples; higher cutoff range, louder accent. |
| `factory` | Best guess at a new TB-303: nominal component values and the service-manual cutoff trim. Passes 30 of 34 schematic conformance checks. |
| `devilfish` | Best guess at a Devil Fish modded 303 (wider decay and accent ranges, more filter drive). No samples, so unverified. |

This is ongoing work toward a very low reference error. Known gaps include:
- resonance is too weak at note start on some high-resonance settings;
- the x0x unit's weak C2 fundamental is modelled by a 200 Hz high-pass
  rather than by an identified circuit element;
- the x0x set is all C2, so nothing yet constrains key tracking;
- several reference-conformance checks conflict with this particular unit
  (cutoff trim, VEG decay), and the `factory` profile passes them.

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

The resulting CLAP plugins (`acidus.clap` and the sequencer, `burette.clap`) will be located in the `build/` directory.

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

To calibrate against a new source (more samples, a hardware unit or clone, a
software plugin, or found samples with video evidence of the knobs), follow
[`docs/CALIBRATION_COOKBOOK.md`](docs/CALIBRATION_COOKBOOK.md): what to record, how
to name and place the files, the knob sheet and manifest
(`tools/make_reference_manifest.py`), the fit, how to judge it, and how to turn it
into a plugin preset.

Fitted constants are kept per source in `calibrations/` (`x0x`, `acidvoice`,
`factory`, `devilfish`) and compiled into the plugin's presets with
`tools/calibration_profile.py presets`; `calibrations/README.md` explains
what differs between them.

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
./build/burette_test             # sequencer timing, host sync, state and GUI editing
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

