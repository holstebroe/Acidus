# Training plan: calibrating against a software 303

A small, staged training set for fitting an Acidus profile to another
software emulation. With a plugin as the source, knob positions, note-on
times, gates and pitches are exact and the output has no noise. So you can
choose exactly which knob combinations to render, and you only need to cross
two knobs when they actually interact.

The loop, one stage at a time:

```
MIDI file for stage N  ->  DAW: import, MIDI-learn the knobs, render one WAV
   ->  tools/fit_stage.py <src> N --wav <render>   (align, manifest, fit, profile, A/B audio)
   ->  listen, optionally trim by ear in the plugin and export  ->  stage N+1 starts from it
```

| Stage | Notes | Length | What it adds |
|---|---|---|---|
| 0: separability probes | 48 | 2.6 min | Which knob pairs you can skip crossing (a report, no fit) |
| 1: what you hear first | 24 | 1.3 min | Cutoff range, resonance, Env Mod depth, decay range, accent amount, levels |
| 2: knob laws | 37 | 2.0 min | The curve of each knob between its end points |
| 3: detail | 36 | 1.9 min | Accent network, square, pitch, VCA timing |
| 4: sequences | 16 clips | 0.8 min | Slides, accent stacking, retrigger: for listening (not fitted yet) |

That is 145 single notes in total, against 400 for the dinsync.info protocol
and about 50,000 for a full five-knob, two-waveform, two-accent, four-pitch
grid.

Recording rules and the calibrator itself are described in
[`CALIBRATION_COOKBOOK.md`](CALIBRATION_COOKBOOK.md).

---

## 1. Which knobs interact

A profile is a set of constants in one fixed signal path:

```
pitch -> slide RC -> VCO (saw / square) -> diode ladder -> post HP -> VCA -> out
                                              ^                        ^
   Cutoff law + EnvMod law x MEG(Decay, accent) + accent sweep(C13)    VEG, accent VCA, gate
```

The cutoff CV is a sum in octaves:
`cv = law(Cutoff) + EnvModScale(Cutoff, EnvMod) * (MEG(t) - offset(Cutoff)) + sweep(Resonance, Accent knob, accented MEG)`.
That sum decides which pairs interact:

| Pair | Interacts? | Why | Consequence |
|---|---|---|---|
| Cutoff x Resonance | **yes**, strongly | Resonance ceiling, in-loop coupling, level taps | 5x5 grid (stages 1 + 2) |
| Cutoff x Env Mod | **yes**, linearly in Cutoff | EnvModScale and offset are bilinear in Cutoff | Env Mod sweeps at Cutoff 0, 50, 100 |
| Accent x Resonance | **yes** | Resonance gang B sets the C13 charge path | Accent knob x Resonance grid (stage 3) |
| Accent knob x accent step | **yes** | The knob does nothing on an unaccented step | Accent knob only on accented steps, plus unaccented twins |
| Accent x Env Mod | **yes**, by structure | An accent shortens the MEG decay | Accents at one Env Mod; the model carries the rest |
| Pitch x waveform | **yes** | Square duty depends on frequency | Square at C1-C4 |
| Env Mod x Decay | no: product | Depth from Env Mod, time constant from Decay | Separate 1-D sweeps |
| Env Mod x Resonance | no | Resonance only makes the sweep visible | Sweep at one Resonance (75 %) |
| Decay x Cutoff, Decay x Resonance | no | MEG time constant only | One Decay sweep |
| Decay x accent | no | An accent shorts the Decay pot | No Decay sweep on accents |
| Pitch x Cutoff, Env Mod, accent | no | The 303 has no key tracking | All laws at C2 only |
| Waveform x Env Mod, Decay, accent | no | The filter does not see the waveform | Square only at a few points |
| Gate x filter | no | The MEG is one-shot; the gate only drives the VCA | Gate lengths only at static knobs |
| Slide x Cutoff, Resonance, Env Mod, Decay | no | Slide is an RC on the pitch CV only | Slides at one setting, filter open |
| Slide interval | no for an RC | Same time constant at every interval | Several intervals, to test exactly that |

That table describes **Acidus and the 303**. The emulation you calibrate
against may differ: it may track the keyboard, use an ADSR that releases at
gate-off, or glide at a constant rate. **Stage 0 checks each "no" on the
actual source** before any data is skipped because of it.

---

## 2. Setting up

1. **Write the MIDI files** (once):
   ```bash
   python3 tools/emulation_training_set.py <src> --out test/resources/<src>
   ```
   This writes `<src>-stage0.mid` ... `<src>-stage4.mid`, plus
   `<src>_sheet.csv` and `<src>_sequences.csv` so you can read what each
   note is. `<src>` is a short id for the emulation, for example `abl3`.
2. **Set up the DAW project:** 125 BPM, one instrument track with the
   emulation, at the project sample rate (44.1 or 48 kHz). Effects, drive,
   "analog drift" and random variation off. Tuning centred, highest quality
   or oversampling mode, master bus clean.
3. **MIDI-learn the knobs** to Acidus's CC map, so the same files drive both
   plugins:

   | Knob | Cutoff | Resonance | Env Mod | Decay | Accent | Waveform |
   |---|---|---|---|---|---|---|
   | CC | 71 | 72 | 73 | 74 | 22 | 23 (0 = saw, 127 = square) |

   **Check the learned mapping:** send CC 0, 64 and 127 to each knob and read
   the plugin's own display. It should show the knob's minimum, 50.4 % and
   maximum. If MIDI learn maps to a sub-range or applies a curve, every knob
   value is wrong: fix the mapping in the plugin. If the emulation uses fixed
   CC numbers, write its map with
   `--cc cutoff=74,resonance=71,envmod=12,decay=75,accent_knob=16,waveform=...`.
4. **Accent** is velocity 127 and normal notes are 100. Check that the
   emulation accents at 127 and not at 100.
5. **Set the level once**, using the loudest stage 0 note (P03,
   accented at Resonance 100). Peaks around -12 dBFS are fine. Never change
   the level between stages: the anchor note of each stage checks it.

**About the MIDI files.** Each stage file plays the notes on a fixed grid,
1 tick = 1 ms, with the first note-on at 500 ms. A note-on always comes at
least 1.7 s after the previous gate. Every note's knob CCs are sent 300 ms
before it, in silence, which covers MIDI-learn smoothing. A marker names each
set, and a text event on each note records its knobs. The fitter reads the
knobs from these text events, so it doesn't depend on the CC map. CC is
7-bit, so 25 % cannot be sent exactly (it is 31.75 / 127). Every knob is
snapped to the nearest CC value, and the text event records the position that
value really sets (25 % -> CC 32 -> 25.197 %).

**Rendering.** Import `<src>-stage<N>.mid`, then render (bounce) the whole
song from the start as one WAV: mono or stereo, 24-bit or 32-bit float, no
normalising, no dither and no fades. The fitter aligns the WAV with the MIDI
by itself and stops with an error if the tempo or sample rate is wrong.

---

## 3. Stage 0: separability probes (48 notes, no fit)

```bash
python3 tools/fit_stage.py <src> 0 --wav stage0.wav     # -> test/resources/<src>/stage0/probes.md
```

Each probe renders the four corners of a 2x2 grid over two knobs, A and B,
with the others held fixed, and measures one feature y per note. The
interaction is

```
I = (y[A1,B1] - y[A0,B1]) - (y[A1,B0] - y[A0,B0])
```

The feature is chosen so that "independent" means I = 0 in its unit. For
example, the resonant-peak track is measured in log frequency, so a sweep
that adds the same number of octaves gives I = 0.

The fitter renders the same notes through Acidus and measures the same
interaction there. What counts is **what is left over**: an interaction the
source has and the model can't produce. Interactions the two share (ladder
nonlinearity, coupling filters, the accent switch) are already in the model.

| Probe | Pair | Fixed | Feature | Left over must be |
|---|---|---|---|---|
| P01 | pitch (C1, C2) x Cutoff (50, 100) | Reso 100 | settled peak frequency | < 1 semitone: no key tracking |
| P02 | pitch x Env Mod (25, 100) | Cut 75, Reso 75, Dec 50 | peak track | < 1 st |
| P03 | pitch x accent | Cut 75, Reso 100, Env 0, Dec 50, Acc 100 | peak track; peak level | < 1 st; < 1 dB |
| P04 | accent x Decay (0, 100) | Cut 75, Reso 75, Env 75, Acc 100 | the two accented notes | < 1 st (Decay is shorted) |
| P05 | Env Mod (50, 75) x Decay (25, 75) | Cut 75, Reso 75, gate 2.5 s | decay time constant of the track | < 15 % change |
| P06 | Cutoff (75, 100) x Decay (25, 75) | Reso 75, Env 50, gate 2.5 s | decay time constant | < 15 % change |
| P07 | Resonance (75, 100) x Env Mod (25, 100) | Cut 75, Dec 50 | peak track | < 1 st |
| P08 | gate (150, 1300 ms) x accent | Cut 75, Reso 75, Env 100, Dec 75 | first 140 ms of the spectrogram | < 1 dB: nothing depends on the gate before it ends |
| P09 | waveform x Env Mod (25, 100) | Cut 75, Reso 75, Dec 50 | peak track | < 2 st (the square tracks less cleanly) |
| P10 | accent x Cutoff (50, 100) | Reso 100, Env 0, Dec 50, Acc 100 | peak track | < 1 st |
| P11 | Accent knob (0, 100), unaccented | Cut 25, Reso 75, Env 50, Dec 50 | spectrogram | < 0.5 dB |
| P12 | the same note twice | Cut 50, Reso 75, Env 50, Dec 50 | spectrogram | < 0.1 dB: the source is repeatable |

The tracked probes sit at Resonance 75-100 %, Cutoff 50-100 % and C1/C2.
Those settings keep the resonant peak clearly above the low harmonics, so it
can be placed between them. "Inconclusive" means the peak was lost in one
corner, or a decay time constant was longer than the gate can show. Nothing
contradicts the 303 structure there.

The probes were checked against Acidus itself. Using the `acidvoice`,
`factory` and `hellfish` profiles as the "source" against an `x0x` start,
all four profiles pass every probe: all have the 303's structure. A source
with key tracking added (about 0.5 octave per octave at Cutoff 75) fails
P01 with 2 semitones left over, and the other probes stay clean.

**When a probe fails**, cross that pair in the stage that sweeps one of the
two knobs. The report says which: for example, a failed P02 means repeat set
2B at C1. Edit `note_sets()` in `tools/emulation_training_set.py` and write
the MIDI files again; stages you have rendered keep the same notes. If P01
shows key tracking, Acidus has no constant for it, and no amount of data
will fit it. Keep the later stages at C2 and note the gap. If P12 fails,
find the randomisation switch before going on.

The probe notes stay out of the fits (`--with-probes` adds them). They hold
little information the stages don't, and they would make every fit about
three times slower.

---

## 4. Stage 1: what you hear first (24 notes)

```bash
python3 tools/fit_stage.py <src> 1 --wav stage1.wav
```

The constants that make most of the audible difference, from the fewest
notes: each knob at 0 / 50 / 100 %.

| Set | Notes | Knobs |
|---|---|---|
| 1A | 9 | Cutoff {0, 50, 100} x Resonance {0, 50, 100} |
| 1B | 5 | Env Mod {0, 50, 100} at Cut 50, Reso 75, Dec 50; Env Mod 100 at Cutoff 0 and 100 |
| 1C | 3 | Decay {0, 50, 100}, Cut 25, Reso 75, Env 100, **gate 2500 ms** |
| 1D | 4 | Accent knob 100 on Reso {0, 100}, each with an unaccented twin; Cut 25, Env 50, Dec 50 |
| 1E | 2 | square at Cutoff 50; saw with a **4000 ms gate** (VEG decay) |
| 1R | 1 | anchor: Cut 50, Reso 50 |

Fitted: only the scale constants. These are cutoff base and span, feedback
gain and resonance limit, Env Mod depth and offset, decay minimum and
maximum, the accent decay, accent sweep and accent level, VEG decay, the
resonance tap, square level, and timing. The curve shapes keep the starting
profile's values, because three points per knob cannot pin a shape down.
The fit starts from `calibrations/x0x.json` unless you pass `--start`.

## 5. Stage 2: knob laws (37 notes)

| Set | Notes | Knobs |
|---|---|---|
| 2A | 16 | the rest of the 5x5 Cutoff x Resonance grid (25 and 75 %) |
| 2B | 14 | Env Mod {25, 60, 70, 75, 80, 90} at Cutoff 50 (the S-curve is steep at 50-90 %); Env Mod {0, 25, 50, 75} at Cutoff 0 and 100 |
| 2C | 6 | Decay {10, 25, 40, 60, 75, 90}, gate 2500 ms |
| 2R | 1 | anchor |

Fitted (groups `cv` and `filter`, plus the Decay law, VEG decay and the
resonance tap): the curve shapes and the filter core. The stage 1 constants
are freed again. The fit uses stages 1 and 2 together and starts from
`calibrations/<src>-stage1.json`.

## 6. Stage 3: detail (36 notes)

| Set | Notes | Knobs |
|---|---|---|
| 3A | 15 | Accent knob {0, 25, 50, 75, 100} x Reso {0, 50, 100} on accented steps (the rest of 1D), unaccented twin at Reso 50, accented square |
| 3B | 5 | square, Cutoff {0, 50, 100} x Reso {0, 100} |
| 3C | 6 | saw and square at C1, C3, C4, Cutoff 100 (octave scale, duty law, low-end coupling) |
| 3D | 9 | gates {30, 60, 120, 250 ms}, unaccented and accented; accented 4000 ms |
| 3R | 1 | anchor |

Fitted: everything the calibrator can vary (section 9), on stages 1-3.

## 7. Stage 4: sequences (16 clips, listening only)

| Set | Clips | What |
|---|---|---|
| Q1 | 6 | slide +1, +3, +7, +12, -12, +24 semitones, filter open (Cut 100, Reso 0, Env 0). With an RC every interval has the same time constant; with a constant-rate glide the time grows with the interval |
| Q2 | 1 | +12 slide under Cut 25, Reso 75, Env 50: the pitch track must match Q1's |
| Q3 | 1 | four slid 16ths (a slide that starts mid-glide) |
| Q4 | 3 | slide under Env 100, Dec 75: no retrigger; normal to accent and accent to normal (accent switched mid-envelope) |
| Q5 | 4 | four accented 16ths at Reso 100 and 0; accented 8ths; alternating normal and accent (C13 stacking) |
| Q6 | 1 | four unaccented 16ths at Decay 100 (MEG retrigger from partial charge) |

Ties are not in the set: over MIDI a tie is only a longer gate, and stage 3
already covers gates.

**These clips cannot be fitted yet.** `calibrate_reference.py` renders one
note per clip, and the slide time constant (22 ms) is hard-coded in
`src/core/Oscillator.cpp`. Fitting stage 4 needs a sequence clip type in the
manifest and the render library, and slide tau made a parameter. Until then,
render `<src>-stage4.mid` through both the emulation and Acidus with the
fitted profile, and compare by ear.

---

## 8. After each fit: listen, then decide

`fit_stage.py` prints the error per set (before -> after) and writes these
files to `test/resources/<src>/stage<N>/`:

| File | What |
|---|---|
| `ab.wav` | Every note of the stage: the source, 0.3 s of silence, then Acidus with the new profile. Same scale for both and level-matched by the fitted gain |
| `acidus.wav` | Acidus with the new profile on the render's timeline. Drop it on a track next to your render and switch between them |
| `fit/report.md` | The calibrator's report: per-set table, parameters at a bound, worst notes |
| `calibrations/<src>-stage<N>.json` | The new profile; the next stage starts from it (or from your trimmed export of it, below) |

Before you render the next stage:

1. **Listen** to `ab.wav`, in this order: brightness (1A), resonance bite
   (1A, Reso 100), sweep depth and speed (1B, 1C), accent punch (1D).
2. **Check the anchor.** The `nR` set must score about the same in every
   stage. A jump means the level or a setting changed between renders.
3. **Read `report.md`**: parameters sitting at a bound mean the model is
   compensating for something (see `X0X_CALIBRATION_2026-09-28.md` for the
   post-HP example).
4. **Optional: predict the next stage.** Render it, then score it with the
   current profile and nothing fitted. A small error on notes the fit never
   saw means the profile already generalises, and you can stop there:
   ```bash
   python3 tools/fit_stage.py <src> 2 --wav stage2.wav --evaluate-only
   ```

Not happy with a stage? Rerun it with more time (`--minutes 60`), or start
from another profile (`--start calibrations/factory.json`).

### Trim by ear, then fit the next stage from the trim

1. Load Acidus (the calibration build, `-DACIDUS_CALIBRATION_BUILD=ON`, so
   the constants are host parameters) next to the emulation, both playing
   the same MIDI.
2. Right-click the logo plate, then **IMPORT CALIBRATION...**, and pick
   `calibrations/<src>-stage<N>.json`. The display shows `<SRC>-STAGE<N>`.
3. Adjust the constants in the host's parameter list (`Experimental/...`)
   until it sounds right. The display shows a star while it differs from
   the imported file.
4. Right-click, then **EXPORT CALIBRATION...**, and save as
   `calibrations/<src>-stage<N>-trimmed.json`.
5. Render stage N+1 and run `fit_stage.py` as usual. With no `--start`, it
   starts from the newest `calibrations/<src>-stage<N>*.json`, which is now
   your trim. It prints the profile it starts from. The fit only frees the
   next stage's parameters plus what it shares with earlier stages. A
   constant you trimmed that no stage fits keeps your value.

Any profile file works with `--start`, including one exported from a
release build or edited by hand.

---

## 9. What the optimiser varies

The calibrator can vary the 51 constants in its `MODEL_PARAMS` table (groups
`osc`, `filter`, `cv`, `env`). It also varies one position per distinct
(knob, percent) pair (group `knobs`), plus a global note-on shift and gate
offset (group `timing`). It solves one global level gain directly.
`fit_stage.py` always passes `--fix knobs`, because a plugin's knob
positions are exact, and narrows the rest per stage with `--only`.

These never vary:

- Volume, Drive and Tuning (`--tune-cents` sets the source's tuning,
  default 0);
- `filterLadderTopology`, a switch: try both by hand;
- `vcoOctaveScale`: measure it from 3C with a tuner;
- the slide time constant, which is hard-coded.

Mixed gate lengths are rendered per note. The calibrator used to render
every note with one fitted gate. It now uses each clip's own `gate_ms` when
a manifest's gates differ by more than 60 ms, and the fitted `gateMs` shifts
them all. Sets with one nominal gate, such as the dinsync set, keep the old
behaviour.

---

## 10. Commands

```bash
# Once
python3 tools/emulation_training_set.py <src> --out test/resources/<src>

# Per stage, after rendering <src>-stage<N>.mid in the DAW
python3 tools/fit_stage.py <src> 0 --wav ~/renders/<src>-stage0.wav   # probe report
python3 tools/fit_stage.py <src> 1 --wav ~/renders/<src>-stage1.wav   # fit, profile, A/B
python3 tools/fit_stage.py <src> 2 --wav ~/renders/<src>-stage2.wav
python3 tools/fit_stage.py <src> 3 --wav ~/renders/<src>-stage3.wav

# Options: --minutes 30 (fit time), --start PROFILE, --alone (this stage's notes only),
#          --with-probes, --tune-cents C, --evaluate-only
# Dry run with Acidus as the source:
python3 tools/fit_stage.py <src> 1 --rehearse calibrations/acidvoice.json
```

`fit_stage.py` copies the render to `test/resources/<src>/stage<N>/`. Its
first run builds the render library (`build/acidus_calibration_render`),
which needs the `clap` and `clap-wrapper` submodules.

A fit evaluation renders every note of the stages it fits. On 4 cores,
stage 1 (24 notes) takes about 2.5 s per evaluation, and stage 3 (97 notes)
about 10 s. More cores scale this down almost linearly. The default 30
minutes is enough for stage 1. Give stages 2 and 3 an hour or more
(`--minutes 90`) if you can.

A self-test confirms the pipeline adds almost no error. Rendering the whole
set through Acidus and scoring it against the same Acidus gives a weighted
error of 0.45, with 98 % of harmonics within 3 dB. What a real fit leaves
is the difference between the two emulations.
