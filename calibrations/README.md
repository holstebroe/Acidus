# Calibration profiles

Acidus uses one circuit model. What differs between hardware TB-303s, and
so between reference sources, is a set of constants: trimmer settings,
component tolerances and ageing, pot tapers and envelope times. Each profile
here holds every `SynthParameters` calibration constant for one unit or
flavour, plus notes on where it came from.

| Profile | Source | Status |
|---|---|---|
| `x0x` | dinsync.info reference recordings (`test/resources/x0x-reference`, 400 notes, one ~40-year-old unit) | **Default.** Fitted with the resonant-peak sweep tracker, see `docs/X0X_CALIBRATION_2026-09-28.md`. |
| `acidvoice` | Acidvoice single-note samples (`test/resources/303_saw-*.wav`, 13 notes, a second unit) | Fitted: the `x0x` model with the constants that differ between units refitted to Acidvoice. |
| `factory` | None: every value the schematic or service manual fixes, the rest from `x0x` | The schematic reference, on Stinchcombe's full coupling network. |
| `x0x-circuit` | The x0x set, refitted on Stinchcombe's full coupling network with schematic ladder capacitors | Candidate to replace `x0x`; see `docs/STINCHCOMBE_NETWORK_2026-10-03.md`. |
| `hellfish` | None: `factory` with Devil Fish ranges and community mods | The ultimate 303, tuned for sound. |

## Scores

Each profile scored on each reference set: weighted error over all
features (harmonic levels, inter-harmonic energy, loudness envelope,
1/3-octave spectrogram, resonant-peak shape; lower is better), and the
schematic conformance test.

| Profile | 400 x0x notes | 13 Acidvoice notes | Schematic conformance (`acidus_reference_test --fast`) |
|---|---|---|---|
| `x0x` | **2.61** | 7.00 | 24 / 34 |
| `acidvoice` | 3.36 | **2.07** | 24 / 34 |
| `factory` | 3.75 | 3.40 | **32 / 34** |
| `x0x-circuit` | 2.72 | 8.17 | 28 / 34 |
| `hellfish` | - | - | 22 / 34 (by design; not a model of a unit) |

The two measured units are each best fitted by their own profile. The
schematic `factory` profile lands between them: a new unit, trimmed per the
service manual, is not expected to match either aged unit exactly. `x0x`
in detail, on its 400 notes:

| Metric | `x0x` |
|---|---|
| Harmonic levels, RMS error | 2.3 dB |
| Harmonics within 1 / 3 / 6 dB | 34 / 76 / 92 % |
| Note loudness, RMS error across notes | 1.3 dB |
| Resonant-peak shape, RMS error | 3.5 dB |
| Resonant-peak sweep track, RMS error | 3.2 semitones |
| Loudness envelope, RMS error | 3.1 dB |
| 1/3-octave spectrogram, RMS error | 5.6 dB |

Scores are reproducible with the command in
[`docs/CALIBRATION_COOKBOOK.md`](../docs/CALIBRATION_COOKBOOK.md) section 7
(`--evaluate-only --no-sensitivity`, plus `--w-sweep 1` on the x0x set,
which adds the resonant-peak sweep track to the score).

`factory` fails four conformance checks: B4 (resonant peak height), B6
(low-frequency shape), C5 (square edge ringing) and E9 (fast MEG term in
the VCA). B4 and B6 come from the empirical coupling network around the
filter, C5 and E9 from the oscillator and VCA models; none from
calibration choices. With Stinchcombe's network (`filterCouplingNetwork`
1), `factory` passes B4 and B6
([`STINCHCOMBE_NETWORK_2026-10-03.md`](../docs/STINCHCOMBE_NETWORK_2026-10-03.md)). The measured units fail six more each, all
where the unit differs from a new one: the cutoff trim (both), a slow VEG
(both), the full-Env-Mod peak and a long held-note decay (x0x), and a
weaker Env Mod bias shift and a stronger oscillator-side high-pass
(Acidvoice).

## The profiles

### `x0x`: the dinsync.info unit as recorded

A 40-year-old unit with a low cutoff trim. The service check (Cutoff
centre, Resonance max, Env Mod/Decay/Accent min) rings at ~350 Hz against
the manual's 400-670 Hz. It also shows an aged accent capacitor C13, ~25 %
below 1 uF, a slow VEG (tau 2.7 s vs R123 x C42 = 1.5 s) and a weak C2
fundamental. What the recordings established about the circuit is kept in
every profile:

- the Env Mod S-curve (logistic, steepest at ~69 % of travel);
- the Decay pot taper (a ~ 18, not the ideal 81);
- D24's forward drop (0.30 of the MEG swing);
- the 53 % square duty;
- the ~4.5 ms VCA onset delay on unaccented notes;
- a cutoff that reaches ~24 kHz at full Env Mod.

### `acidvoice`: the Acidvoice unit

The `x0x` model refitted to 13 Acidvoice samples. Only the constants that
plausibly differ between units were free:

- the cutoff law (trim, span, taper) and accent sweep and VCA depths;
- resonance feedback and limit, the Env Mod scale and offset;
- VEG decay, the VCA resonance tap, the post-HP and oscillator coupling.

Everything else (pot laws, D24, square shape) stays at the x0x values: 13
saw-only notes at min/half/max knobs cannot constrain them. The main
difference is the **cutoff range**: the Acidvoice unit's settled cutoff at
Resonance max sits ~0.25 octave higher at knob minimum (206 vs 165 Hz) and
~0.45 octave higher at maximum (1.8 vs 1.3 kHz). It also has a louder
accent (VCA depth 2.3 vs 1.6), more resonance in the output (VCA tap 2.0 vs
1.2) and a VEG of 1.9 s. Part of the apparent trim difference between the
units is the Env Mod law, which the x0x sweeps pin down.

### `factory`: the schematic

Every constant the schematic, parts list or service manual fixes is set to
it, even where that costs fit score on the two recorded units (both are
aged and trimmed differently, so a new unit is not expected to match them).
The reasoning for each value is in
[`docs/CALIBRATION_PARAMETERS.md`](../docs/CALIBRATION_PARAMETERS.md).

| Part | Schematic / service manual | x0x unit (fitted) | `factory` |
|---|---|---|---|
| TM3 cutoff trim | service check rings at 2 ms +/- 0.5 ms (400-670 Hz) | 184 Hz base, check at ~350 Hz | 274 Hz base, check at ~530 Hz |
| Ladder capacitors C19/C24/C26/C18 | 33 / 33 / 33 / 18 nF | scales 1.29 / 0.70 / 0.91 / 1.06 | 1.0 (schematic) |
| Ladder orientation | input pair Q12 saturates on input - feedback, half capacitor on stage 1 (§10.3) | legacy mirrored | circuit orientation |
| Oscillator -> VCF coupling | none (§7.2) | 47 Hz (Open303-style) | off (1 Hz floor) |
| Filter -> VCA taps | R122 100 k, R121 220 k (§12) | ratio 1.25 | 2.2 (wiper on the 100 k) |
| Post-filter high-pass | R122 100 k x C22 10 nF = 159 Hz (§15.4) | 199 Hz | 159 Hz |
| Resonance law | linear VR4a loaded by Q18's ~50 k bias | skew -0.872 | -0.865 (derived) |
| VCA onset | R134 22 k x C41 0.1 uF = 2.2 ms | 1.3 ms | 2.2 ms |
| C13 1 uF (accent sweep) | R46 x C13 47 ms, VR4b x C13 50 ms | 31 / 44 ms, mix 145 ms | 47 / 50 ms, mix 100 ms |
| C42 1 uF (VEG) | R123 x C42 = 1.5 s | 2.7 s | 1.5 s |
| C62 1 uF (MEG) | 68 k / 1.068 M into 1 uF: 68 ms - 1.07 s | 65 ms - 0.98 s | 68 ms - 1.068 s |
| Accented MEG (Decay pot shorted) | 68 ms | 73 ms | 68 ms |

Kept from the x0x fit, because the schematic does not fix them: the A-pot
curves of Cutoff, Env Mod and Decay (a real two-segment A pot, which the
recordings measure, not the a = 81 estimate), the Env Mod scale and offset,
D24's drop, the ladder drive, the feedback ceiling and resonance limit,
Open303's empirical coupling network around the filter (in-loop high-pass,
notch, all-pass, input coupling), the square shape and the VCA timing.

Typical ageing of a small 1980s electrolytic is 10-30 % capacitance loss
(end-of-life criterion -20 %), with drying accelerated by heat. Leakage and
ESR changes are too small to matter in these networks. With D24's drop
modelled, a smaller C13 charges further before the diode stops conducting.
So the aged `x0x` accent squelch peaks earlier and higher than
`factory`'s. This is a model prediction; there is no recording of a new
unit.

### `hellfish`: the ultimate 303

Not a model of any unit: the schematic `factory` core with the mods the
community keeps coming back to, set for sound rather than accuracy. Named
Hell Fish because it is inspired by Robin Whittle's Devil Fish
([manual](https://www.firstpr.com.au/rwi/dfish/Devil-Fish-Manual.pdf)) but
no Devil Fish was measured.

| Mod | Origin | Here |
|---|---|---|
| Normal Decay 30 ms - 3 s | Devil Fish | Decay knob spans 30 ms - 3 s |
| Accent Decay | Devil Fish (stock: fixed ~68 ms) | 200 ms |
| Soft Attack | Devil Fish (0.3 - 30 ms) | 3 ms |
| Overdrive into the filter | Devil Fish (up to 66.6x) | 2x; with the circuit-orientation input pair this growls |
| Self-oscillating resonance | Devil Fish; x0xb0x R97 mod | screams from ~94 % of the Resonance knob (limit 1.12 x critical, ceiling 30) |
| Wider Cutoff range | common mod | ~120 Hz - 2.9 kHz settled (4.6 octaves) |
| Deeper Env Mod | Devil Fish | sweep depth x1.4 |
| Bass mod | common mod | post-filter HP 40 Hz, oscillator coupling 20 Hz |
| Harder accent with a tail | Open303-style accent tail | sweep depth 10 oct, VCA depth 2.3, 30 ms accent release |

Not representable yet: Filter Tracking, Filter FM, the Muffler, the Accent
Sweep speed switch, Slide Time, and live mod pots (these are fixed settings
here). Its conformance failures are the mods' intended departures from the
stock schematic.

## Adding a profile from a new source

The whole procedure (recording a reference set, the knob sheet and manifest, fitting,
judging the score, capturing the profile and adding it as a preset) is in
[`docs/CALIBRATION_COOKBOOK.md`](../docs/CALIBRATION_COOKBOOK.md).

## Presets in the plugin

The four profiles are built into the plugin as calibration presets. The
logo plate shows the current one in a small display under the smiley.
**Click it to load the next preset** (X0X -> ACIDVOICE -> FACTORY -> HELL
FISH -> X0X). Loading a preset replaces every calibration constant. The
front-panel knobs keep their positions. The preset is saved with the
project, and projects saved before presets existed load as X0X.

In the calibration build the constants are also CLAP parameters (below),
and the host sees each one change when a preset loads. A star after the
name (`FACTORY*`) means at least one calibration parameter has been
changed since the preset was loaded, so the sound is only *based on* that
preset. Loading a preset again clears the star. This replaces the old
Ctrl-click reset. A Release build has no calibration parameters, so it
never shows a star.

### Export and import

Right-click the logo plate for the calibration menu:

- **EXPORT CALIBRATION...** writes the calibration that plays now as a
  profile in this folder's format (`name`, `source`, `parameters` with all
  55 constants). In the calibration build that includes your parameter
  edits.
- **IMPORT CALIBRATION...** reads a profile into a custom slot after the
  built-in presets, named after the profile's `name` or the file name, and
  selects it. Constants the file lacks keep the value that was playing, so
  a file with just `{"parameters": {"cutoffBaseHz": 260}}` works. Unknown
  keys are ignored. A file that does not parse, or names no known
  constant, changes nothing.
- The slots, the current one marked: picking one is the same as clicking
  the display.

The custom slot is saved with the project. Fitted profiles
(`calibrate_reference.py`, `fit_stage.py`) import directly, and exported
files are valid `--calibration` / `--start` profiles for the tools. On
Linux the file dialog is zenity or kdialog, as for Burette's banks.

To trim a calibration by ear, use the calibration build. There, 50 of the
55 constants are CLAP parameters: all but the oscillator's saw low-pass and
bend, square duty and level, and the MEG attack. Import, adjust the
parameters in the host, and export. A Release build imports and exports
but has nothing to edit.

The presets are compiled from the JSON files: after changing a profile, or
the list in `PRESETS` in `tools/calibration_profile.py`, regenerate
`src/core/CalibrationPresets.hpp` and rebuild:

```bash
python3 tools/calibration_profile.py presets          # regenerate the preset table
python3 tools/calibration_profile.py presets --check  # fail if it is stale
```

## Switching the compiled-in defaults

The `SynthParameters` defaults in `SynthEngine.hpp` are what the
calibrator, the reference tests and the render library start from. The
plugin starts on the first preset, not on the header. Keep the header on
`x0x` unless you are experimenting:

```bash
python3 tools/calibration_profile.py list                 # which profile SynthEngine.hpp matches
python3 tools/calibration_profile.py diff x0x factory
python3 tools/calibration_profile.py apply acidvoice      # rewrites the SynthParameters defaults
cmake --build build                                       # rebuild
python3 tools/calibration_profile.py capture NAME --source "..."   # snapshot the current defaults
```

The calibrator can score or fit from any profile without touching the header:

```bash
python3 tools/calibrate_reference.py --calibration calibrations/acidvoice.json --evaluate-only
python3 tools/calibrate_reference.py --manifest --calibration calibrations/factory.json --evaluate-only --no-sensitivity
python3 tools/sweep_track.py E3-p1 --calibration calibrations/factory.json   # peak tracks vs hardware
```

`archive/` keeps superseded profiles (the first x0x fit, the old-model
Acidvoice fit, and the intermediate sweep experiments) for comparison.

## CLAP mapping (calibration build, `-DACIDUS_CALIBRATION_BUILD=ON`)

Every profile constant that differs between the four profiles is a CLAP
parameter in the calibration build. So all four can be dialled in a host
without rebuilding:

| CLAP id | Name | SynthParameters field |
|---|---|---|
| 32 / 33 / 34 | Cutoff Trim / Knob Span / Knob Taper | `cutoffBaseHz`, `cutoffSpanOct`, `cutoffTaperExp` |
| 29 | Accent Sweep Depth | `accentSweepDepthOct` |
| 35 | Resonance Knob Curve | `filterResonanceSkew` |
| 36-41, 48, 51, 52 | Env Mod law and taper | `envModScale*`, `envModOffset*`, `envModTaper*` |
| 42-47, 49 | MEG / accent timing, D24 drop | `vcfDecayMin/MaxSec`, `accentDecaySec`, `accentCharge*`, `accentMixSec`, `accentDiodeDrop` |
| 50 | Decay pot taper | `vcfDecayTaper` |
| 53 | Cutoff ceiling | `cutoffMaxHz` |
| 54 | Unaccented VCA delay | `vcaNormalDelayMs` |
| 55 | VCA attack (Devil Fish Soft Attack) | `vcaAttackMs` |
| 9-31 | Filter couplings and ladder, VEG/VCA, accent VCA depth, resonance limit | `oscCouplingHz`, `filterFeedbackGain`, `filterPostHpHz`, `vegDecaySec`, `filterLadderInputScale`, ... |

Clicking the preset display on the logo plate reloads a preset, and the
right-click menu exports and imports calibrations (see above).

## Possible extensions

- per-part toggles (aged C13, trim high/low) that apply a subset of a
  profile;
- the preset as an automatable CLAP parameter instead of only saved state;
- for Hell Fish, the mods as real front-panel knobs (Devil Fish style).
