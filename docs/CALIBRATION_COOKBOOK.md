# Calibration cookbook: a new source to a plugin preset

How to turn a new reference source into a calibration profile (`calibrations/<name>.json`)
and a selectable preset in the plugin. The source can be:

| Source | Typical situation | Section notes |
|---|---|---|
| **A real TB-303** | You can record as many notes as you like | [2.4](#24-source-specific-notes) |
| **A clone or modded unit** (TD-3, x0xb0x, Devil Fish, ...) | Same idea, but the extra controls must be parked and written down | [2.4](#24-source-specific-notes) |
| **A software plugin or emulation** | Knob values and timing are exact, there is no noise | [2.4](#24-source-specific-notes) |
| **Found samples** with video evidence of the knob positions | Few notes, approximate knobs | [2.4](#24-source-specific-notes) |

The pipeline is the same for all of them:

```
record / collect  ->  place + name files  ->  knob sheet (CSV)  ->  manifest (JSON)
   ->  sanity check  ->  baseline score  ->  fit  ->  read the report
   ->  capture profile  ->  add preset  ->  build + test  ->  document + commit
```

Background you may want first: `README.md` ("How Acidus is calibrated"),
`calibrations/README.md` (the existing profiles and what differs between them),
and `docs/X0X_CALIBRATION_2026-09-28.md` (a complete worked example on 400 notes).

Everything below was run against the repository as it is (the 13 Acidvoice notes
scored through a generated manifest, a fit, `apply` then `capture`, a preset build
and the GUI preset test). Timings are from a 4-core machine.

---

## 0. Setup (once)

```bash
git submodule update --init          # the CLAP headers; the plugin will not build without them
pip install numpy matplotlib         # matplotlib is optional (report plots)
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build                  # plugin, tests, and the acidus_calibration_render library
```

The calibrator renders through `build/acidus_calibration_render.so` (`.dylib` /
`.dll` on macOS / Windows). It builds that library itself if it is missing, and
`--rebuild` forces a rebuild after you change C++ code.

Work on a branch. Steps 10 and 11 edit tracked files.

---

## 1. What the calibration can and cannot learn

Acidus has one circuit model. A profile is a set of about 50 constants (trimmer
settings, pot laws, envelope times, component ageing) and nothing else. Front-panel
knob positions, master volume, drive and tuning are **not** in a profile.

The fit compares, per note: harmonic levels, energy between harmonics, the RMS
envelope, a 1/3-octave spectrogram, the resonant-peak shape (Resonance >= 50 %),
and with `--w-sweep` the resonant-peak frequency over time. It solves **one global
recording gain** for the whole set, so a note's absolute loudness counts. That is
why levels must not be normalised (section 3).

What a set must contain follows from that: every constant group needs notes where it
is audible.

| Constants | Audible in | What to record |
|---|---|---|
| Cutoff law (trim, span, taper) | The settled resonant peak frequency and the harmonic roll-off | Cutoff sweep at Resonance 100 % and at 0 % |
| Resonance feedback, limit, curve | Peak height and width | Resonance sweep |
| Env Mod law (scale, offset, S-curve) | The resonant peak gliding down after note-on | Env Mod sweep at high Resonance |
| Decay pot taper, MEG times | How fast that glide falls | Decay sweep at high Env Mod |
| Accent sweep, C13 network, accent VCA depth | Accented notes: louder, peak higher and earlier | Accent knob sweep on accented steps, plus unaccented twins |
| Square duty, saw/square level | Harmonic comb of the square | Square notes at low Resonance |
| VEG decay, VCA onset, post-HP, couplings | Envelope and low-frequency level | Full-length notes (gate plus release) |
| Key tracking | Level and cutoff versus pitch | Several pitches at fixed knobs. **The x0x set is all C2, so this is unconstrained today.** |

A constant with no notes that exercise it stays at its starting value; the report
marks it `unconstrained`. That is acceptable, but it means "fitted" only covers
what you recorded.

---

## 2. What to record

### 2.1 Conventions

- **Note names follow the tool**: `C2` is MIDI 36, 65.4 Hz, the lowest C of the
  hardware keypad with no transpose (the dinsync set is all `C2`). Acidvoice's
  files use the same scale (`A1` is about 55 Hz). If a name is an octave off, the
  start-up check in section 6 shows a tuning far outside +-60 cents.
- **Knobs are percent of travel**: 0 = fully counter-clockwise, 100 = fully clockwise.
- **Accent has two parts.** The *step* is accented or not (a yes/no per note);
  the *Accent knob* only does anything on an accented step. Record both.
- **Waveform** is `saw` or `square`.
- **One position = one pot setting.** The fit gives each distinct (knob, percent)
  pair one free position shared by all notes that use it (for example every
  "Cutoff 50 %" note shares one). So the same percentage must always mean the same
  physical setting. Do not call two slightly different settings both "50".

### 2.2 The three tiers

Pick one. Each contains the previous.

**Minimum: 40 notes.** Enough to fit the constants that differ between units.
Saw, single pitch except the key-tracking row. A template sheet is in
[`docs/templates/reference_sheet_minimum.csv`](templates/reference_sheet_minimum.csv).

| File | Purpose | Cutoff | Reso | Env Mod | Decay | Accent | Notes |
|---|---|---|---|---|---|---|---|
| `M1` | Cutoff law (settled peak) | **0 25 50 75 100** | 100 | 0 | 0 | 0 | 5 saw |
| `M2` | Roll-off with no resonance | **0 25 50 75 100** | 0 | 0 | 0 | 0 | 5 saw |
| `M3` | Resonance curve | 50 | **0 25 50 75 100** | 0 | 0 | 0 | 5 saw |
| `M4` | Env Mod law | 25 | 75 | **0 25 50 75 100** | 50 | 0 | 5 saw |
| `M5` | Decay taper | 25 | 75 | 75 | **0 25 50 75 100** | 0 | 5 saw |
| `M6` | Accent | 25 | 75 | 50 | 0 | **0 25 50 75 100** | 5 saw, *accented steps*, plus 1 unaccented twin at Accent 50 |
| `M7` | Square | **0 50 100** | 0 | 0 | 0 | 0 | 3 square, plus 1 accented square (Cutoff 50, Reso 75, Env Mod 50, Decay 0, Accent 100) |
| `M8` | Key tracking | 50 | 75 | 25 | 25 | 0 | saw at **C2, C3, C4** (more pitches if the source reaches them) |
| `M9` | Drift / repeat check | 50 | 50 | 0 | 0 | 0 / 50 | the `M3` middle note again, unaccented and accented, recorded **last** |

Why these settings: Resonance 75 in `M4`-`M6` keeps a clear peak without sitting on
self-oscillation, which no feedback constant can pin down. Env Mod, Decay and Accent
rows use Decay 0 or 50 so the glide is long enough to see but the note is not
dominated by the slowest MEG tail. The five-position sweeps are not optional: the
Env Mod law is S-shaped and the Decay pot is far from an ideal taper, and three points
cannot resolve either.

**Standard: about 120 notes.** Every minimum file re-recorded with all four note
kinds: saw, saw accented, square, square accented. (The 4 kinds x 5 positions
layout is what the dinsync files use.) This is what constrains the square and the
accent interaction with every other knob.

**Full: 400 notes.** Reproduce the dinsync.info protocol exactly: five sweeps
(Cutoff, Reso, Env Mod, Decay, Accent) over five base rows (the other four knobs
fixed at 0, 25, 50, 75 and 100 %), four positions each, four note kinds. It is
described in `test/resources/x0x-reference/x0x_reference.md`. The advantage is a
like-for-like comparison with the `x0x` profile: same knob positions, same notes.
The hardest sets there (high Resonance with high Env Mod: E1, E3, D3) are the
ones that show the most model error, so a source that reproduces that protocol is
the most informative.

### 2.3 Rules that apply to every tier

1. **Dry, raw and unnormalised.** No normalise, no compression, no EQ, no reverb, no
   delay, no distortion, no dither-induced gain change, no fades. One fixed gain
   chain for the *whole* set: if you must change gain between files, write the
   change down (the fit cannot undo it).
2. **Headroom.** Set the gain once using the loudest note you will play (Resonance
   100, Accent 100, accented). The dinsync set peaks around -22 dBFS. Never clip.
3. **Single notes, no slides.** One note per slot, slide off, no tied notes. The
   gate length must be the same for every note (the dinsync set: a 1.33 s gate in
   a 3 s slot). Long enough that the Decay sweep is visible: **at least 1 s**
   gate, 1.3 s or more is better. Short gates (Acidvoice: about 165 ms) work but
   leave the long-decay constants unconstrained.
4. **Silence around every note.** At least 100 ms of true silence before the note-on
   (the tool subtracts the DC level of the 50 ms before the note) and a release that
   has died away before the next note. Two to three seconds per slot is comfortable.
5. **Hold everything else fixed and write it down.** Tune at the same setting (centre
   is easiest), same output level, same cables. On a clone or a modded unit this
   includes every control the real 303 does not have (overdrive, Filter Tracking, Muffler,
   Soft Attack, accent sweep speed, ...). They must not move during the session.
6. **Sample rate and depth.** 44.1 or 48 kHz, 24 bit (16 works). Stereo files are
   averaged to mono. Anything else the WAV reader accepts: 16, 24, 32 bit integer
   and 32 bit float.
7. **A repeat note.** Record one setting at the start and again at the very end
   (`M9`). The dinsync repeats agree within 0.4 dB; a much bigger difference means the
   gain moved or the unit drifted, and the whole set is suspect.
8. **Order of knobs you change.** Sweep one knob at a time from a known base and
   log the settings as you go; do not rely on your memory afterwards.

### 2.4 Source-specific notes

**A real TB-303.** Follow 2.2 and 2.3. Also write down: serial or year, whether it
has been serviced, known trimmer settings if you have measured them (the service
check is Cutoff centre, Resonance max, Env Mod/Decay/Accent min: the filter should
ring at about 400-670 Hz, see `calibrations/README.md`), battery or adapter, and
the recording chain. A 40-year-old unit will not match a new one (the `x0x` and
`factory` profiles differ for exactly that reason), so record the state you have.

**A clone or modded unit.** Clones replicate the diode-ladder topology with
different parts and sometimes different pot values or an extra input stage. The
model is fitted to the *behaviour*, so a clone fits well if it is faithful, and badly
if it is not. The fit score tells you which (section 9). A mod that adds controls
(Devil Fish) can only be captured with those controls parked: the profile then
represents that one setting, and the extra controls do not become plugin knobs.
See the `hellfish` profile in `calibrations/README.md` for what "mapped onto
existing constants" looks like.

**A software plugin or emulation.** Use an **offline bounce** from a DAW, one note per
slot on a fixed grid, so the note-on time and gate length are exact (you set them in
the MIDI clip). Knob positions are exact too (copy the plugin's 0..1 or 0..10
value and convert to percent), so fit with `--fix knobs` (the knob positions are
then trusted rather than searched). There is no noise floor and no hardware
drift, so the tuning is exact: give `tune_cents` (probably 0) or measure
it. Expect the fit to expose where the plugin's model differs from the real circuit
(that may be the point). Turn off oversampling-dependent effects, built-in
drive and effects. Check the plugin's licence before committing its renders.

**Found samples with video evidence.** Samples from a video or a sample pack only
count if:

1. the notes are **isolated and dry** (no drums, no other instrument under them, no
   effects, no slides);
2. you can see the knobs at the time each note plays (a fixed camera, a
   close-up, or a hardware still), or the author wrote the settings down;
3. accent state is known or visible (accent LED or step display), otherwise you
   have to guess it for every note.

Read each knob from a paused frame: measure the pointer angle against the minimum
and maximum end stops of *that knob* in the same frame and convert to percent,
`(angle - min) / (max - min)`. Hand-held readings are good to about 5-10 %. **Snap
readings to 0 / 25 / 50 / 75 / 100** unless the footage is clearly finer, because
every distinct (knob, percent) value becomes its own free knob position in the fit
(and a per-note reading error turns into a per-note free parameter with nothing to
constrain it). Write the video URL, timestamp and your reading into the `evidence`
column of the knob sheet (extra columns are ignored by the tools) and in the source
README (section 4), so the reading can be re-checked.

With very few notes (about 10-20), do not fit everything: restrict `--only`
to the constants that plausibly differ between units (section 8.3). The Acidvoice
profile (13 saw notes) is the precedent.

---

## 3. Recording checklist (print this)

- [ ] Source state written down: unit / plugin version, age, mods, every extra control parked.
- [ ] Tune centred (or recorded), output level fixed, **no level normalisation**.
- [ ] One fixed gain for the whole set, verified on the loudest note: no clipping.
- [ ] Same gate length for every note; >= 1 s preferred; slide off.
- [ ] >= 100 ms silence before each note-on; release dead before the next.
- [ ] Notes in the agreed order, knob log written as you go.
- [ ] The repeat note recorded last (`M9`).
- [ ] Export 44.1 / 48 kHz, 24 bit WAV, no trim, no fades, no processing.
- [ ] Note the time of every note-on in every file (MIDI grid, sequencer BPM, or note it
      later from the waveform).

---

## 4. Where to put the files and what to call them

**Do not put a multi-note set into `test/resources/` itself.** The default
`--refs` folder is `test/resources/` and its `303_*.wav` single-note files follow a rigid
file-name grammar (section 5.3). Give every new source its own folder:

```
test/resources/<source-id>/
    README.md                         # where it came from, see the template below
    <source-id>-M1.wav ... -M9.wav    # the audio, one file per sweep (several notes per file)
    <source-id>_notes.csv             # the knob sheet you write (section 5)
    <source-id>_manifest.json         # generated from the CSV
```

`<source-id>` is lowercase, short and stable: `td3`, `tb303-unit2`, `rebirth`,
`yt-acidking-2019`. It is also the profile name later (`calibrations/<source-id>.json`),
so avoid spaces. File names inside the folder only need to match the CSV's `file`
column; the `-M1` / `-M2` labels above are a suggestion (the dinsync files are
`SET-A1.wav` ... `SET-E5.wav`).

Copy this into the source's `README.md` and fill it in:

```markdown
# <source-id>: <one-line description>

- Source type: hardware TB-303 | clone | modded (which) | software plugin (name, version) | found samples
- Who / where / when recorded, licence and whether the audio may be committed:
- Unit state: serial / year, serviced?, trimmer notes, battery or adapter:
- Extra controls and their fixed positions:
- Recording chain: interface, gain, sample rate, bit depth, any processing (should be none):
- Sequencer / timing: BPM, gate length, slot length, how note-on times were obtained:
- Note names / octave convention used and how it was checked:
- Knob evidence (video URL + timestamps, photos, or "exact: plugin values"):
- Known label doubts:
```

**Licensing and size.** Committing audio publishes it: only do so if the licence allows
it, and credit it in the README's Credits as the other sets are. The dinsync and
Acidvoice audio together are about 200 MB in the repository. If you cannot commit
the audio, commit the README, the CSV, the manifest and the profile, and keep the
WAVs out of the repository (add the folder to `.gitignore`); say so in the README.

---

## 5. The manifest: telling the tool what each note is

The calibrator reads a **manifest**, a JSON file that lists every note as a *clip*
with its file, sample range, knobs, waveform, accent, pitch offset, note-on and
gate length. Write a CSV knob sheet instead, and generate the manifest.

### 5.1 The knob sheet (CSV)

One row per note. Start from the template:

```bash
cp docs/templates/reference_sheet_minimum.csv test/resources/<source-id>/<source-id>_notes.csv
sed -i 's/SRC-/<source-id>-/' test/resources/<source-id>/<source-id>_notes.csv
```

| Column | Meaning | Required |
|---|---|---|
| `file` | WAV file, relative to the CSV | yes |
| `note` | `C2`, `A#1`, ... (section 2.1) | yes |
| `waveform` | `saw` or `square` | yes |
| `accent` | `yes` / `no`: was this an **accented step**? | yes |
| `cutoff`, `resonance`, `envmod`, `decay` | knob percent, 0-100 | yes |
| `accent_knob` | Accent knob percent (default 0) | no |
| `note_on_ms` | ms from the start of the file to the note-on. **Required when a file holds more than one note.** Blank on a single-note file = detected from the audio | see left |
| `gate_ms` | note-on to gate-off. Blank = `--gate-ms`; if that is missing too it is detected (rough) | no |
| `tune_cents` | the source's pitch offset from equal temperament. Blank = measured (below) | no |
| `set` | group name for the per-set table in the report (default: the file name) | no |
| `id` | clip id used by `--include` / `--exclude` (default `<set>-<n>`) | no |
| any other column | ignored, use it for `evidence` | no |

Lines starting with `#` are comments. Get the note-on times right: they are the
alignment between your notes and the model, not something the fit can find.
For a DAW bounce or a sequencer with a known grid, compute them. For hand-recorded
notes, read them off the waveform (the start of the sound) and round to a
millisecond. A few ms of error is absorbed by the fitted note-on shift.

### 5.2 Generate the manifest

```bash
python3 tools/make_reference_manifest.py test/resources/<source-id>/<source-id>_notes.csv \
    --description "<source-id>: <what it is>, recorded <when>" \
    --gate-ms 1330 --tune-cents 5.4
# -> test/resources/<source-id>/<source-id>_notes_manifest.json
```

What it does: for each row it cuts a clip from 50 ms before the note-on to the gate
plus `--tail-ms` (default 400 ms) of release (never into the next note of the same
file), and writes the clip list. It fails with a row number on a bad value.

**Tuning.** If you do not pass `--tune-cents` or a `tune_cents` column, it measures
each note's pitch and uses the median over the notes with Resonance and Env Mod
<= 50 %, the ones whose pitch the moving resonant peak does not bias. That is the
method the dinsync manifest used (+5.42 cents). The measurement needs a long steady
note (a gate of about 1 s or more) and only searches +-80 cents, so **for short
notes, pass `--tune-cents` from a tuner or a longer held note**; the tool warns when
the gate is under 500 ms. (On the Acidvoice samples, which have 165 ms gates, it
measured -79 cents where the truth is about -41.)

Once the manifest exists, a generated file should not be edited by hand: change the CSV
and regenerate.

### 5.3 The alternative: the flat file-name grammar

The 13 Acidvoice samples use the original route: the knobs are in the file name,
all files sit directly in `test/resources/`, and `calibrate_reference.py` reads
them with no manifest (`--refs`). It is documented in
`test/resources/reference_resources.md`. Use it only for a handful of single-note
saw/square files; its limits:

- `303_<saw|square>-<note>t<tune>c<d>r<d>e<d>d<d>a<d>.wav` with one digit per knob:
  `0` = 0 %, `1` = 100 %, `5` = 50 %, and any other digit `n` = `n0 %`;
- it cannot say whether a step was accented. By default a non-zero Accent digit means
  accented (`--accent knob`), `--accent all` / `none` override;
- the gate and note-on are detected, not given;
- every file is a whole note from its start.

The manifest route has none of these limits, and is the one to use for anything new.

### 5.4 What a clip record contains (for hand edits and other generators)

If you write a manifest by some other means, `calibrate_reference.py` needs these
keys per clip (see `Reference.from_clip`): `id`, `file`, `set`, `note`, `midi`,
`waveform`, `accent`, `knobs_pct` and `knobs` (keys `cutoff`, `resonance`, `envMod`,
`decay`, `accent`; percent and 0..1), `note_on_sample`, `clip_start_sample`,
`clip_end_sample` (absolute sample indices; end exclusive), `gate_ms`,
`render_tune_cents`; for `--rotate` also `position` and `note_number`. The
top-level `files` and `clips` lists are required; the rest is documentation. The
reference generator for the dinsync set, `tools/x0x_reference_manifest.py`, is an
example of one that parses a chart and measures the onsets from a fixed grid.

---

## 6. Sanity check before fitting

Score the set with nothing fitted. `--evaluate-only` renders each note and compares,
which also loads and analyses every clip, so most mistakes show up here.

```bash
M=test/resources/<source-id>/<source-id>_notes_manifest.json
python3 tools/calibrate_reference.py --manifest $M --calibration calibrations/factory.json \
    --evaluate-only --no-sensitivity --out /tmp/check
```

Runs fast (seconds to a minute). Look at:

1. **`N reference samples`**: is it the number of rows you wrote? (`--include` /
   `--exclude` filter by clip id.)
2. **Per-note lines** (printed when there are 40 notes or fewer; for bigger sets read
   `result.json` -> `references`): `tuning` should be within +-60 cents of zero and
   about the same for every note. An octave-wrong name makes it huge.
3. **`Before: ... ms/eval`**: the cost of one candidate. The total number of
   evaluations in a fit is roughly `minutes x 60000 / ms`. Over a few seconds per
   eval, use a window (`--analysis-ms 800`) or a subset (`--rotate`).
4. **`report.md` -> Per set**: a whole set that scores far worse than the others
   usually has a transcription error for one knob in that file.
5. **Listen**: `/tmp/check/renders/<id>_hardware.wav` against `_before.wav`. The
   worst 40 notes are written (`--max-renders`). You are listening for: the right note,
   the right length, no silence at the start, no clipping.

The report's **Suspicious samples** section flags notes that fit much worse than the
rest, notes whose best free-knob fit moves a knob by more than 0.2, and two notes
with different labels that sound the same (a knob that was not moved). Fix label
errors *now*, with evidence, not later by fitting them away (section 9.4).

---

## 7. Baseline: which profile is closest?

Score the four shipped profiles on the new set. The best one is the fit's starting
point, and the set of numbers is what a new profile has to beat.

```bash
for p in x0x acidvoice factory hellfish; do
  echo "== $p"
  python3 tools/calibrate_reference.py --manifest $M --calibration calibrations/$p.json \
      --evaluate-only --no-sensitivity --out /tmp/base-$p | grep -E "weighted error"   # add --w-sweep 1 if the set has sweeps
done
```

For reference, on the existing sets:

| Profile | 400 x0x notes | 13 Acidvoice notes |
|---|---|---|
| `x0x` | 2.61 | 7.00 |
| `acidvoice` | 3.36 | 2.07 |
| `factory` | 3.75 | 3.40 |

A unit's own fitted profile scores about 2-3 on its set. The schematic
`factory` profile, a new unit, lands at about 3.5-3.9 on aged units, and a
profile fitted to a different unit can be worse than that. The new profile
has to beat `factory` on its own set by a clear margin (section 9), or it is
not worth a preset.

Start the fit from the closest one. If they are all similar, start from `factory`.

---

## 8. Fitting

```bash
python3 tools/calibrate_reference.py --manifest $M --calibration calibrations/factory.json \
    --w-sweep 1 --out calibration_results/<source-id>-A
```

`calibration_results/` is git-ignored. Always pass `--out` so runs have names.

### 8.1 Options that matter

| Option | Use |
|---|---|
| `--calibration <profile>` | Start from that profile instead of the header defaults |
| `--only a,b` / `--fix a,b` | Fit only / hold these. Groups: `osc`, `filter`, `cv`, `env`, `knobs`, `timing`; or a single constant name |
| `--w-sweep 1` | Score the resonant-peak frequency over time (Resonance >= 50 % notes). Needs Env Mod and Decay sweeps; the x0x fit used it |
| `--rotate [0-3]` | Keep every 4th note: a fast subset covering all knob settings. Meaningful for big sets (400, 120); on 40 notes use `--include` / `--exclude` |
| `--analysis-ms 800` | Compare only the first 800 ms of each note (faster; the tail is then unconstrained) |
| `--max-minutes 20 --patience-minutes 4` | Time cap and stop-without-progress |
| `--workers N` | Threads; default: the CPU count |
| `--seed N` | Different seeds give different searches; use two or three and compare |
| `--knob-prior 0.05` | Cost of moving a knob off its nominal position. Raise it (0.3 was used for x0x) if the readings are good, lower it for rough video readings |
| `--fix knobs` | Trust the knob values completely (software sources) |
| `--max-renders 40` | How many worst notes get audio and plots |
| `--no-sensitivity` | Skip the per-parameter sensitivity scan (2 renders per parameter). Turn it **on** for the final run: it is what the `unconstrained` flags come from |

A refit label check (`--refit-seconds`) runs by default **only on the flat
file-name route**, 30 s per sample; pass `--refit-seconds 0` to skip it. (It is off
with `--manifest`.)

### 8.2 A staged plan for a big set (Standard, Full)

This is the procedure that produced the `x0x` profile (see its doc for the numbers).

| Stage | Notes | Window | Free | Why |
|---|---|---|---|---|
| A | `--rotate 0` | `--analysis-ms 800` | everything except constants the window cannot see | Fast, finds the right region |
| B | `--rotate 2` (a **different** quarter) | `--analysis-ms 800` | same | Starting from A's result on notes A never saw: a first generalisation check |
| C | `--rotate 1`, full length | none | `--only` the envelope-tail constants (VEG decay, gate-off release, post-HP, resonance curve) and `timing` | Constrains what the window hides |
| Final | all notes, full length | none | as C, or nothing (`--evaluate-only`) | The number you report. The run that produces the profile should carry `--apply` (section 10.1) |

Each stage after the first starts from the last one's `checkpoint.json`:

```bash
python3 tools/calibrate_reference.py --manifest $M --rotate 0 --analysis-ms 800 --w-sweep 1 \
    --calibration calibrations/factory.json --out calibration_results/<id>-A --max-minutes 120
python3 tools/calibrate_reference.py --manifest $M --rotate 2 --analysis-ms 800 --w-sweep 1 \
    --calibration calibration_results/<id>-A/checkpoint.json --out calibration_results/<id>-B
```

`checkpoint.json` is written whenever the search improves, so a killed run
can be resumed from it. It holds the **model constants only**. It does not include
the cutoff end-stop correction that a fit's report computes from the fitted Cutoff
positions (section 9.4). That is fine between stages (each stage re-finds the knob
positions from nominal), but it means the final profile should come from the fit
run's own `--apply` (section 10.1), not from a checkpoint.

### 8.3 A small set (about 10-40 notes)

62 free parameters against a dozen notes will overfit: every parameter will find a
value that helps those notes and means nothing. Restrict the fit to what differs
between units and that the set can actually see (the Acidvoice precedent, which fitted
these and held the rest at the `x0x` values):

```bash
python3 tools/calibrate_reference.py --manifest $M --calibration calibrations/factory.json \
  --only cutoffBaseHz,cutoffSpanOct,cutoffTaperExp,accentSweepDepthOct,accentVcaDepth,\
filterFeedbackGain,filterResonanceLimit,envModScaleC0,envModScaleC1,envModOffset,\
vegDecaySec,vcaResTapRatio,filterPostHpHz,oscCouplingHz,knobs,timing \
  --out calibration_results/<id>-A
```

With the Minimum set (40 notes) you can add the Env Mod slopes
(`envModScaleC0Slope`, `envModScaleC1Slope`, `envModTaperExp`) and `vcfDecayTaper`,
`vcfDecayMinSec`, `vcfDecayMaxSec`; add the square constants
(`oscSquareDutyDepth`, `oscSquareLevel`) only with a low-pitch square set. When
**unsure whether a constant is free, leave it fixed**: an unfitted constant stays at
a physically sensible value, a wrongly fitted one does not.

Hold out notes to check generalisation: fit with some ids excluded, then score those
alone:

```bash
python3 tools/calibrate_reference.py --manifest $M --exclude M6,M8 ...          # fit without them
python3 tools/calibrate_reference.py --manifest $M --include M6,M8 --evaluate-only \
    --calibration calibration_results/<id>-A/checkpoint.json --no-sensitivity   # score them
```

### 8.4 A software source

As 8.3, but add `--fix knobs` (exact knob values), and use `--fix timing` if the note-on
and gate in the CSV are sample-exact.

### 8.5 How long

The `ms/eval` figure printed at the start, times the evaluations you can afford. A rough
guide from the x0x fit: one 1.75 s note renders in about 0.46 s on one core, so 100 such notes is
about 12 s per evaluation on 4 cores without windowing and a few seconds with
`--analysis-ms 800`. The fit stops at `--max-minutes` or after `--patience-minutes`
without improvement. The search is stochastic, so two seeds that finish within a few percent of
each other is a good sign; two that land far apart means the set does not pin the
constants down.

---

## 9. Reading the result

Open `calibration_results/<id>/report.md` (the same tables are in `result.json`).
Work through it in this order.

### 9.1 Match quality

The first table has before and after for each metric (all errors in dB unless stated; lower
is better). Benchmarks from this repository:

| Metric | `x0x` on its 400 notes | `acidvoice` on its 13 notes | What it tells you |
|---|---|---|---|
| Weighted error | 2.61 | 2.07 | The headline: a weighted mean of everything below |
| Harmonic error | 2.32 | 1.86 | Level of each harmonic: the overall timbre |
| Harmonics within 3 dB | 76 % | 77 % | How much of the spectrum is close |
| Resonant-peak shape | 3.55 | 1.87 | Height and width of the squelch |
| Resonant-peak sweep track | 3.19 st | - | Where the peak is over time (needs `--w-sweep`): Env Mod, Decay, Accent laws |
| RMS envelope | 3.11 | 2.80 | Amplitude over time |
| 1/3-octave spectrogram | 5.60 | 5.03 | Filter motion over time, the noisiest metric |
| Note level (RMS over notes) | 1.28 | 2.59 | Loudness: the one global gain holds for all notes |

How to judge the **weighted error** (these bands are this project's experience, not a
theory; compare like with like, i.e. a similar note mix):

| Weighted error | Reading |
|---|---|
| Below about 2.5 | As good as the best fits here. A profile you can trust on the knob range you recorded |
| 2.5 to 3.5 | Good: the same as the `x0x` fit got on its unit, or the `factory` guess on an unseen one. Usable |
| 3.5 to 5 | Rough. Something systematic is missing: look at 9.2-9.4, and the worst sets |
| Above 5 | No better than an uncalibrated start (the old model scored 5.5 on the dinsync notes). Labels, tuning, gain or a missing model feature |

Two acceptance tests matter more than the absolute number:

1. The new profile scores **clearly better than every existing profile on this set**
   (section 7; "clearly" = at least 0.5 below `factory`).
2. The improvement **holds on notes the fit did not see** (section 8.2 stage B or 8.3's
   held-out notes). A big gap between the fitted set and the held-out notes is overfitting.

The **note level** row is worth a separate look: a high level error with a good
harmonic error means one or a few settings are louder or quieter than the hardware
in a way one gain cannot fix (a gain change during recording, or the accent depth).

### 9.2 Per set and per sample

- **Per set**: the dinsync sets ranged 0.9-2.7 for the easy ones and 4.7-6.1 for
  high-Resonance + high-Env Mod (E1, E3, D3). Expect the same shape: flat low
  sweeps good, the heavy acid sweeps worst. A whole set far off means a transcription
  error for that file's knobs or a timing error (wrong `note_on_ms`).
- **Per sample**, worst first: a single note far above the median is a bad label or a
  bad clip, not a model problem.
- **Resonant peak table**: hardware vs. model peak frequency and height. The height
  column is the gain-independent "is the squelch there" check; if the model's
  height is far below the hardware's the peak has been flattened away to fit
  something else.

### 9.3 Parameters: the model telling you it is wrong

**Fitted model parameters** has a `sensitivity` column and two notes:

- `unconstrained` (sensitivity below 0.005): the set cannot see this constant; its value
  is the start value, not evidence. Fine if you expected it; if it is a constant you
  meant to fit, the set lacks the notes (section 1, table).
- `AT BOUND (model may lack structure)`: the search pushed the constant to the edge of
  its plausible range. A few at-bound constants on a small set is usual; many, or one
  that matters (cutoff span, feedback gain), means the model is compensating for
  something it does not implement, often a clone with a different filter or an
  effect that got into the recording.

Also check that **the values are physically sensible** against the circuit: for
example the four `filterCapScale*` should stay within a factor of about 2 of each other (x0x: 0.70 to 1.29)
(there is a penalty against a formant-like spread) and `vcfDecayMin/MaxSec` and the
accent times stay close to their schematic values unless the unit is aged.

### 9.4 Knob positions and labels

**Fitted knob positions** compares each (knob, percent) with its nominal. For the
x0x set all stayed within 0.03. So:

- within about 0.05: your labels and the model's laws agree;
- 0.05-0.15: readable, but the fit is using the knob to soak up some law error; the profile
  cannot store that (it holds constants, not knob positions). Try a higher `--knob-prior`,
  or a final pass with `--fix knobs`;
- beyond 0.2 with a large improvement in the **label check** / **Suspicious
  samples**: a probably wrong label. See below.

Cutoff is handled differently: if both Cutoff 0 % and 100 % positions were fitted, the
report folds them into the cutoff law ("Cutoff law with the fitted end stops folded
in") and the profile gets that law, so the plugin's knob range matches the hardware's.

**Do not fix a label because the fit likes it.** The repository's precedent
(`test/resources/reference_resources.md`, "Label corrections") is to rename a file only
when three independent things agree: the time course of the resonant peak, a
free-knob fit of that one sample, and the match error with the corrected label. Do the
same, write it in the source README, and re-run.

### 9.5 Listen, look, and cross-check

1. Listen to `renders/<id>_hardware.wav` against `_after.wav` for the worst notes and
   for at least one Resonance-100 accented note. The score can hide a wrong character.
2. Resonant-peak tracks (needs resonant notes from an Env Mod / Decay / Accent sweep):

   ```bash
   python3 tools/sweep_track.py <set>-1 --manifest $M --calibration calibration_results/<id>/checkpoint.json \
       --plot /tmp/sweep.png
   ```

   (ids are prefixes of your clip ids, for example `M4-`). It prints peak frequency
   every few milliseconds for the hardware and the model; the two should fall together.
3. The schematic conformance test (section 10.3) is an independent check that the
   profile has not drifted into something the circuit cannot do.

### 9.6 When it is not good enough

| Symptom | Likely cause | Try |
|---|---|---|
| Every note is off by the same amount of level | The one global gain is wrong for some files | Check the gain log; re-record the odd file; drop it with `--exclude` |
| A whole set is bad | Wrong knob transcription or `note_on_ms` | Fix the CSV, regenerate, rescore |
| One or two notes far worse than the rest | Bad label, clip or noise | Suspicious samples; 9.4; `--exclude` only with a reason written down |
| Good on the fitted notes, bad on held-out notes | Overfitting | Fewer free constants (`--only`), more notes, higher `--knob-prior` |
| Many `AT BOUND`, cutoff span very off | Model lacks a feature, or an effect in the source | Check for overdrive / FX; accept and document |
| Sweep-track error large, peak height fine | Env Mod / Decay / accent timing laws | Add `--w-sweep 2`; needs 5-position Env Mod and Decay sweeps |
| Different seeds give different answers | Underdetermined | More notes; fewer free constants |
| Resonance weak at note start on high-Resonance notes | A known open model item (see `README.md`) | Not a calibration error |

---

## 10. Making the profile

### 10.1 Write the result into the header, capture it, put the header back

`--apply` writes a fit's result into the `SynthParameters` defaults in
`src/core/SynthEngine.hpp`. `capture` snapshots those defaults as a profile. Then
restore the header, so `x0x` stays the default.

**Preferred: put `--apply` on the last fitting run.** That run knows the fitted Cutoff
end stops and folds them into the cutoff law, so the plugin's knob range matches the
hardware's (9.4). Nothing else provides that correction.

```bash
python3 tools/calibrate_reference.py --manifest $M --calibration calibration_results/<id>-B/checkpoint.json \
    --rotate 1 --w-sweep 1 --apply --out calibration_results/<id>-C      # the last fit, with --apply
python3 tools/calibration_profile.py capture <source-id> \
    --source "<one line: what it is, n notes, reference set path>" \
    --notes "<how it was fitted, final scores, known limits>"
git checkout src/core/SynthEngine.hpp        # keep x0x as the compiled-in default
```

**If you only have a checkpoint** (the run was killed, or you do not want to fit again),
apply it with an evaluation run. This writes the constants but **not** the cutoff
correction (knobs are at nominal when nothing is fitted), which is acceptable when the
report's fitted Cutoff positions at 0 % and 100 % stayed within about 0.03 of nominal:

```bash
python3 tools/calibrate_reference.py --manifest $M --calibration calibration_results/<id>-C/checkpoint.json \
    --evaluate-only --apply --no-sensitivity --out calibration_results/<id>-final
```

Notes:

- `capture` reads `SynthEngine.hpp` as the fit left it: run both commands in the same
  working tree, back to back.
- The profile has every constant (about 52), not only the fitted ones. Constants you did
  not fit carry the value of the profile you started from.
- `--apply` only rewrites constants that exist in the header; the report lists how many it wrote.

### 10.2 Describe it

`capture` writes `source`, `captured` (date) and `notes`. Edit
`calibrations/<source-id>.json` and add the fields the other profiles carry:

```json
{
 "source": "...",
 "fitted": "2026-10-01",
 "reference_set": "test/resources/<source-id>/<source-id>_notes_manifest.json",
 "component_state": "as recorded: <age, service, mods, known trimmer settings>",
 "notes": "<stages run, options, final weighted error on all notes and on held-out notes, known gaps>",
 "parameters": { ... }
}
```

Then check it:

```bash
python3 tools/calibration_profile.py show <source-id>
python3 tools/calibration_profile.py diff factory <source-id>   # what moved, and is it plausible
```

### 10.3 Check the conformance test

To run the schematic conformance checks against the new profile, apply it, run, restore:

```bash
python3 tools/calibration_profile.py apply <source-id>
cmake --build build && ./build/acidus_reference_test --fast | tail -1
git checkout src/core/SynthEngine.hpp
```

For reference: `x0x` 24 / 34, `factory` 32 / 34 (the fit on an aged unit fails the
cutoff-trim and VEG checks by design, which are real differences of that unit, see
`calibrations/README.md`). Do not aim for 34 / 34. A profile that fails **many more** checks
than `x0x` has left the circuit's physical range to fit the recordings; look at which
ones and at the `diff`.

---

## 11. Adding it to the plugin

### 11.1 Register the preset

Edit `PRESETS` in `tools/calibration_profile.py`. Two rules:

- **Append at the end.** The preset index is saved in projects; inserting or reordering
  changes what existing projects load. The first entry is also the start-up preset.
- The display name is shown on the logo plate: upper case, **10 characters or less**
  (the plate is sized for `HELL FISH*`).

```python
PRESETS = [("x0x", "X0X"), ("acidvoice", "ACIDVOICE"), ("factory", "FACTORY"),
           ("hellfish", "HELL FISH"), ("<source-id>", "<NAME>")]
```

### 11.2 Regenerate and build

```bash
python3 tools/calibration_profile.py presets           # rewrites src/core/CalibrationPresets.hpp
python3 tools/calibration_profile.py presets --check   # must say "up to date"
cmake --build build                                    # rebuilds acidus.clap and the test programs
./build/acidus_gui_test                                # cycles through every preset
```

`presets` refuses to run if the profile is missing a constant, or has one the engine
does not know (for example after `SynthParameters` gained a field: add it to the profile).
`acidus_gui_test` passing means the new preset loads and the preset cycle includes it
("Calibration preset tests passed (N presets)").

### 11.3 Try it in a host

The plugin is `build/acidus.clap`. Load it, click the small display under the smiley on
the logo plate until it shows your name. The knobs keep their positions, so set them to one
of your reference settings and A/B against the recording. For the constants as host
parameters (and the `*` star when one has moved), build the calibration variant:

```bash
cmake -B build-calibration -DCMAKE_BUILD_TYPE=Release -DACIDUS_CALIBRATION_BUILD=ON
cmake --build build-calibration
```

### 11.4 Make it the default (only if you mean to)

The plugin starts on the first entry in `PRESETS`. Changing which one is first changes
the start-up sound and what old projects load as. The compiled-in header defaults (used by
the calibrator and the tests) are separate; change them with
`python3 tools/calibration_profile.py apply <name>` only if you want that too.

---

## 12. Document and commit

Before committing, fill in the scores so that the next person can see what the profile is:

1. `calibrations/README.md`: a row in the profile table (source, status) and in the
   scores table (the new profile on its own set; every existing profile on the new set; conformance
   count), plus a short "The profiles" section (what differs from the others, in numbers).
2. `README.md`: the row in "Calibration profiles" and the source in **Credits**.
3. A dated write-up in `docs/` if the fit needed judgement (the dinsync one is
   `docs/X0X_CALIBRATION_2026-09-28.md`): stages, options, what it established, open items.
4. `test/resources/<source-id>/README.md` complete (section 4).

Files in the commit:

```
calibrations/<source-id>.json
calibrations/README.md, README.md
src/core/CalibrationPresets.hpp            (generated; must match: presets --check)
tools/calibration_profile.py               (the PRESETS line)
test/resources/<source-id>/                (README, CSV, manifest, and the WAVs if allowed)
docs/...                                   (write-up, if any)
```

`src/core/SynthEngine.hpp` should **not** be in the diff unless you meant to change the
compiled-in defaults. `git status` before committing; `git diff --stat src/core/SynthEngine.hpp`
should be empty.

---

## Appendix A. Command cheat sheet

```bash
# Setup
git submodule update --init && pip install numpy matplotlib
cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build

# Manifest
python3 tools/make_reference_manifest.py <dir>/<id>_notes.csv --gate-ms 1330 --tune-cents 5.4

# Score / baseline
python3 tools/calibrate_reference.py --manifest $M --calibration calibrations/<p>.json --evaluate-only --no-sensitivity --out /tmp/x

# Fit (fast subset, then full)
python3 tools/calibrate_reference.py --manifest $M --rotate 0 --analysis-ms 800 --w-sweep 1 \
    --calibration calibrations/factory.json --out calibration_results/<id>-A --max-minutes 120

# Profile (--apply on the last fitting run)
python3 tools/calibrate_reference.py ... --apply --out calibration_results/<id>-C
python3 tools/calibration_profile.py capture <id> --source "..." --notes "..."
git checkout src/core/SynthEngine.hpp
python3 tools/calibration_profile.py diff factory <id>

# Plugin
# (edit PRESETS)   python3 tools/calibration_profile.py presets && python3 tools/calibration_profile.py presets --check
cmake --build build && ./build/acidus_gui_test
```

## Appendix B. Files this touches

| File | Role |
|---|---|
| `tools/make_reference_manifest.py` | CSV knob sheet to manifest JSON (new sources) |
| `tools/x0x_reference_manifest.py` | The same for the dinsync set only (parses its chart) |
| `tools/calibrate_reference.py` | Scores and fits; writes `report.md`, `result.json`, `checkpoint.json`, `synth_parameters.txt`, `renders/` |
| `tools/sweep_track.py` | Resonant-peak track, hardware vs. model |
| `tools/calibration_profile.py` | `list / show / diff / apply / capture / presets` |
| `src/calibration/CalibrationRender.cpp` | The render library the calibrator calls (the real `SynthEngine`) |
| `src/core/SynthEngine.hpp` | `SynthParameters` defaults (keep = `x0x`) |
| `src/core/CalibrationPresets.hpp` | Generated from `calibrations/*.json`: the plugin's presets |
| `acidus_reference_test` | Schematic conformance checks (34) |
| `docs/templates/reference_sheet_minimum.csv` | The 40-note knob sheet template |
