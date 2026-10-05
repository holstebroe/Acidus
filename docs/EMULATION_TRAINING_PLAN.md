# Training plan: calibrating against a software 303

A small, staged training set for fitting an Acidus profile to another
software emulation. With a plugin as the source, knob positions, note-on
times, gates and pitches are exact and the output has no noise. So you can
choose exactly which knob combinations to render, and you only need to cross
two knobs when they actually interact.

The sheets and MIDI files are written by `tools/emulation_training_set.py`.
Recording rules, the manifest and the fitting commands are in
[`CALIBRATION_COOKBOOK.md`](CALIBRATION_COOKBOOK.md). This document covers
what to render and in what order.

| | Notes | What it adds |
|---|---|---|
| Stage 0: separability probes | 48 | Which knob pairs you can skip crossing |
| Stage 1: what you hear first | 24 | Cutoff range, resonance, Env Mod depth, decay range, accent amount, levels |
| Stage 2: knob laws | 37 | The curve of each knob between its end points |
| Stage 3: detail | 36 | Accent network, square, pitch, VCA timing |
| Stage 4: sequences | 16 clips | Slide law, accent stacking, retrigger (needs calibrator work, below) |

That is 145 single notes in total, against 400 for the dinsync.info protocol
and about 50,000 for a full five-knob, two-waveform, two-accent, four-pitch
grid.

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
| Pitch x Resonance, low Cutoff | **yes**, weakly | In-loop high-pass vs the fundamental | Covered by the probe notes, which are fitted too |
| Pitch x waveform | **yes** | Square duty depends on frequency | Square at C1-C4 |
| Env Mod x Decay | no: product | Depth from Env Mod, time constant from Decay | Separate 1-D sweeps |
| Env Mod x Resonance | no | Resonance only makes the sweep visible | Sweep at one Resonance (75 %) |
| Decay x Cutoff, Decay x Resonance | no | MEG time constant only | One Decay sweep |
| Decay x accent | no | An accent shorts the Decay pot | No Decay sweep on accents |
| Pitch x Env Mod, Decay, accent | no | The 303 has no key tracking | All laws at C2 only |
| Waveform x Env Mod, Decay, accent | no | The filter does not see the waveform | Square only at a few points |
| Gate x filter | no | The MEG is one-shot; the gate only drives the VCA | Gate lengths only at static knobs |
| Slide x Cutoff, Resonance, Env Mod, Decay | no | Slide is an RC on the pitch CV only | Slides at one setting, filter open |
| Slide interval | no for an RC | Same time constant at every interval | Several intervals, to test exactly that |

That table describes **Acidus and the 303**. The emulation you calibrate
against may differ: it may track the keyboard, use an ADSR that releases at
gate-off, or glide at a constant rate. **Stage 0 checks each "no" on the
actual source** before any data is skipped because of it.

---

## 2. Conventions for every stage

- **Notes:** C2 = MIDI 36 (the tools' convention). Range C1-C4.
- **Base note:** saw, C2, unaccented, Env Mod 0, Decay 0, Accent knob 0,
  gate 1300 ms.
- **Grid:** 125 BPM, 480 PPQ, so 1 tick = 1 ms. One WAV per set, first
  note-on at 500 ms. Slots are at least 3 s, and a note-on always comes at
  least 1.7 s after the previous gate ends.
- **Accent:** velocity 127; normal notes are 100. Check the emulation's
  accent threshold, or use its own sequencer and accent flags.
- **Slide:** the next note-on comes 10 ms before the previous note-off, as
  Burette sends it.
- **Knobs:** set by MIDI CC from the generated MIDI files (`--cc default`).
  MIDI-learn the emulation's Cutoff, Resonance, Env Mod, Decay, Accent and
  Waveform to CC 71, 72, 73, 74, 22 and 23, which is Acidus's own map. The
  same files then drive both plugins. Each CC is sent 300 ms before its
  note, in silence. See section 10 for checking the learned mapping. Without
  MIDI learn, use plugin automation at the sheet's values instead.
- **Fixed for the whole set:** tuning centre, volume, any drive or effects
  off, highest quality or oversampling mode, any "analog drift" or random
  variation off, 48 kHz / 24-bit offline bounce, no normalising.
- **Fit with `--fix knobs`** and `--tune-cents 0` (or the measured offset).
  The knob positions are exact, so the optimiser should not move them.

---

## 3. Stage 0: separability probes (48 notes)

Each probe renders the four corners of a 2x2 grid over two knobs, A and B,
with the other knobs held fixed. Compute one feature y per note and the
interaction

```
I = (y[A1,B1] - y[A0,B1]) - (y[A1,B0] - y[A0,B0])
```

Pick y so that "independent" means I = 0 in that unit. For example, sweep
depth is measured in octaves, not Hz. Peak tracks come from
`tools/sweep_track.py`, levels and envelopes from `tools/analyze_audio.py`.

| Set | Pair | Fixed | Feature y | "Independent" if |
|---|---|---|---|---|
| P01 | pitch (C1, C3) x Cutoff (50, 100) | Reso 100 | settled resonant-peak frequency, Hz | the peak does not move with pitch (no key tracking) |
| P02 | pitch x Env Mod (25, 100) | Cut 25, Reso 75, Dec 50 | sweep depth, oct | \|I\| < 0.1 oct |
| P03 | pitch x accent step | Cut 25, Reso 100, Env 50, Dec 50, Acc 100 | accent peak shift, oct; level, dB | < 0.1 oct, < 0.5 dB |
| P04 | accent step x Decay (0, 100) | Cut 25, Reso 75, Env 75, Acc 100 | peak track | the two accented notes match |
| P05 | Env Mod (50, 100) x Decay (25, 75) | Cut 25, Reso 75 | fit `A*exp(-t/tau)+B` to the track | A only follows Env Mod, tau only Decay (within 5 %) |
| P06 | Cutoff (25, 75) x Decay (25, 75) | Reso 75, Env 100 | tau | within 5 % |
| P07 | Resonance (50, 100) x Env Mod (25, 100) | Cut 25, Dec 50 | sweep depth, oct | < 0.1 oct |
| P08 | gate (150, 1300 ms) x accent step | Cut 25, Reso 75, Env 100, Dec 75, Acc 100 | first 150 ms; release shape | first 150 ms identical; note the release |
| P09 | waveform x Env Mod (25, 100) | Cut 25, Reso 75, Dec 50 | peak track, oct | < 0.1 oct |
| P10 | accent x Cutoff (25, 75) x Env Mod (0, 100) | Reso 100, Dec 50, Acc 100 | accented minus unaccented track, oct | the same at all four corners |
| P11 | Accent knob (0, 100), unaccented | Cut 25, Reso 75, Env 50, Dec 50 | the whole note | the two notes null |
| P12 | the same note twice | Cut 50, Reso 75, Env 50, Dec 50 | magnitude spectrogram | identical (phase may differ) |

**When a probe fails**, cross that pair in the stage that sweeps one of the
two knobs. For example, if P02 fails, render set 2B at C1 and C3 as well. If
P01 shows key tracking, Acidus has no constant for it: no amount of data will
fit it, so leave the set at one pitch and record the gap in the profile's
README. If P12 fails, find the randomisation switch before going on.

Probe notes are ordinary training notes: they also go into the manifest.

---

## 4. Stage 1: what you hear first (24 notes)

The constants that make most of the audible difference, from the fewest
notes: each knob at 0 / 50 / 100 %.

| Set | Notes | Knobs |
|---|---|---|
| 1A | 9 | Cutoff {0, 50, 100} x Resonance {0, 50, 100} |
| 1B | 5 | Env Mod {0, 50, 100} at Cut 50, Reso 75, Dec 50; Env Mod 100 at Cutoff 0 and 100 |
| 1C | 3 | Decay {0, 50, 100}, Cut 25, Reso 75, Env 100, **gate 2500 ms** |
| 1D | 4 | Accent knob 100 on Reso {0, 100}, each with an unaccented twin; Cut 25, Env 50, Dec 50 |
| 1E | 2 | square at Cutoff 50; saw with a **4000 ms gate** (VEG decay) |
| 1R | 1 | anchor: Cut 50, Reso 50, render it last |

Fit only the scale constants. Keep the curve shapes at their current values,
because three points per knob cannot pin down a shape:

```bash
M=test/resources/<src>/<src>_notes_manifest.json
python3 tools/calibrate_reference.py --manifest $M --calibration calibrations/factory.json \
  --fix knobs --w-sweep 1 --max-minutes 20 \
  --only cutoffBaseHz,cutoffSpanOct,filterFeedbackGain,filterResonanceLimit,\
envModScaleC0Slope,envModScaleC1Slope,envModOffset,vcfDecayMinSec,vcfDecayMaxSec,\
accentDecaySec,accentSweepDepthOct,accentVcaDepth,vegDecaySec,vcaResTapRatio,\
oscSquareLevel,timing
```

## 5. Stage 2: knob laws (37 notes)

The shape of each knob between its end points.

| Set | Notes | Knobs |
|---|---|---|
| 2A | 16 | the rest of the 5x5 Cutoff x Resonance grid (25 and 75 %) |
| 2B | 14 | Env Mod {25, 60, 70, 75, 80, 90} at Cutoff 50 (the S-curve is steep at 50-90 %); Env Mod {0, 25, 50, 75} at Cutoff 0 and 100 |
| 2C | 6 | Decay {10, 25, 40, 60, 75, 90}, gate 2500 ms |
| 2R | 1 | anchor |

The Env Mod and Decay sweeps have more points than the 5 % steps of the
dinsync set, at no extra crossing cost. Fit adds the shapes and the filter
core, and frees the stage 1 constants again:
`--only cv,filter,vcfDecayMinSec,vcfDecayMaxSec,vcfDecayTaper,vegDecaySec,vcaResTapRatio,timing`.

## 6. Stage 3: detail (36 notes)

| Set | Notes | Knobs |
|---|---|---|
| 3A | 15 | Accent knob {0, 25, 50, 75, 100} x Reso {0, 50, 100} on accented steps (the rest of 1D), unaccented twin at Reso 50, accented square |
| 3B | 5 | square, Cutoff {0, 50, 100} x Reso {0, 100} |
| 3C | 6 | saw and square at C1, C3, C4, Cutoff 100 (octave scale, duty law, low-end coupling) |
| 3D | 9 | gates {30, 60, 120, 250 ms}, unaccented and accented; accented 4000 ms |
| 3R | 1 | anchor |

Then fit everything: no `--only`, still with `--fix knobs`. Check
`vcoOctaveScale` directly from the 3C pitches with a tuner. The optimiser
does not fit it (section 9).

## 7. Stage 4: sequences (16 clips)

Rendered from `<src>_sequences.csv`. Knobs are held fixed for each clip.

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
use Q1-Q6 as a listening and pitch-track comparison: render them through
Acidus with the fitted profile and compare.

---

## 8. Between stages: the manual validation gate

Before you render the next stage, check the one you just fitted.

1. **Predict the next stage.** Render the next stage's notes from the source
   first, then score them with the current profile and nothing fitted:
   ```bash
   python3 tools/calibrate_reference.py --manifest $M --calibration calibrations/<src>.json \
     --evaluate-only --include 2A,2B,2C --out /tmp/predict
   ```
   A small error on notes the fit never saw means the stage generalises. The
   gap between that score and the score after fitting is what the next stage
   adds. If the gap is small, stop there.
2. **Check the anchor.** Each stage's `nR` note must score the same as the
   previous one, within 0.2 dB. If it does not, the gain chain or a hidden
   setting changed between rendering sessions.
3. **Listen A/B.** Play the same test pattern through the source and through
   Acidus with the profile (Burette or a MIDI riff with slides and accents).
   Use three snapshots: low (Cut 25, Reso 50, Env 25), squelch (Cut 25,
   Reso 90, Env 75, Dec 50) and open (Cut 75, Reso 75, Env 50). Also do one
   slow automated Cutoff sweep at Reso 75. Listen for brightness,
   resonance bite, sweep speed and accent punch, in that order.
4. **Read the report.** In `report.md`, check the per-set table, the
   `unconstrained` parameters and any parameter sitting at a bound. A bound
   means the model is compensating for something; see `X0X_CALIBRATION` for
   the post-HP example.
5. **Capture the profile.** Run `tools/calibration_profile.py capture` (see
   the cookbook) so the next stage starts from it.

Stop at the first stage that passes the listening test and leaves only a
small prediction gap. Stages 2 and 3 are refinements.

---

## 9. What the optimiser varies

The calibrator can vary the 51 constants in its `MODEL_PARAMS` table (groups
`osc`, `filter`, `cv`, `env`). It also varies one position per distinct
(knob, percent) pair (group `knobs`), plus a global note-on shift and gate
offset (group `timing`). It solves one global level gain directly.
`--only` and `--fix` narrow the set.

These never vary:

- the panel knobs when you pass `--fix knobs` (do so for an emulation);
- Volume, Drive and Tuning (the manifest's `tune_cents` sets the tuning);
- `filterLadderTopology`, a switch: try both by hand;
- `vcoOctaveScale`: measure it from 3C;
- the slide time constant, which is hard-coded.

Mixed gate lengths are rendered per note. The calibrator used to render
every note of a set with one fitted gate. It now uses each clip's own
`gate_ms` when a manifest's gates differ by more than 60 ms, and the fitted
`gateMs` shifts them all. Sets with one nominal gate, such as the dinsync
set, keep the old behaviour.

---

## 10. Commands

```bash
# Sheets and MIDI for stages 0-1, knobs by CC (rerun with --through 2, 3, 4 as you go)
python3 tools/emulation_training_set.py <src> --out test/resources/<src> --midi --cc default --through 1

# Other CC numbers, if the emulation has a fixed map instead of MIDI learn
#   --cc cutoff=74,resonance=71,envmod=12,decay=75,accent_knob=16

# After rendering <src>-<set>.wav for each MIDI file:
python3 tools/make_reference_manifest.py test/resources/<src>/<src>_notes.csv --tune-cents 0
```

`--through N` writes the stages up to N, so the sheet lists only files you
have rendered. Re-render nothing: later stages never repeat an earlier note.
CC is 7-bit, so 25 % cannot be sent exactly (it is 31.75 / 127). With
`--cc`, each knob is snapped to the nearest CC value and the sheet records
the exact position that value sets (25 % -> CC 32 -> 25.197 %). The fit then
sees the knob the emulation actually got.

**Check the MIDI learn before rendering.** Send CC 0, 64 and 127 to each
learned knob and read the plugin's own value display: it should read the
knob's minimum, 50.4 % and maximum. Some MIDI-learn implementations scale to
a sub-range, apply a curve, or smooth CC changes over tens of ms. The 300 ms
lead covers smoothing, but a range or a curve makes every sheet value wrong.
Remap the learn in the plugin, or go back to automation.

Self-test: rendering the full sheet through Acidus itself and scoring it
against Acidus gives a weighted error of 0.45, with 98 % of harmonics within
3 dB. The pipeline itself adds almost no error, so what a real fit leaves is
the difference between the two emulations.
