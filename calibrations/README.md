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
| `factory` | None: `x0x` with ageing parts reset to the schematic and the service-manual cutoff trim | Best guess at a new unit. |
| `hellfish` | None: `factory` with some Devil Fish ranges | Loose guess; no samples. |

## Scores

Weighted error (all features; lower is better) of each profile on each
reference set:

| Profile | 400 x0x notes | 13 Acidvoice notes | Schematic conformance (`acidus_reference_test --fast`) |
|---|---|---|---|
| `x0x` | **2.61** | 7.12 | 24 / 34 |
| `acidvoice` | 3.22 | **2.13** | 24 / 34 |
| `factory` | 3.49 | 3.28 | **30 / 34** |
| `hellfish` | - | - | 25 / 34 (by design, see below) |

The `x0x` fit on the 400 notes, against earlier models:

| Metric | Old Acidvoice-only model | First x0x fit | `x0x` |
|---|---|---|---|
| weighted error | 5.53 | 3.25 | **2.61** |
| harmonic level error | 7.97 dB | 3.21 dB | **2.31 dB** |
| resonant-peak shape | 6.17 dB | 3.91 dB | **3.54 dB** |
| resonant-peak sweep track | 7.52 st | 4.84 st | **3.34 st** |
| RMS envelope | 3.85 dB | 3.38 dB | **2.93 dB** |
| 1/3-octave spectrogram | 9.07 dB | 6.50 dB | **5.57 dB** |
| note level (RMS over notes) | 2.78 dB | 1.69 dB | **1.29 dB** |
| harmonics within 3 / 6 dB | 56 / 69 % | 71 / 86 % | **76 / 92 %** |

`factory` fails four conformance checks: B4 (resonant peak height), B6
(low-frequency shape), C5 (square edge ringing) and E9 (fast MEG term in
the VCA). These are open model items, not calibration choices.

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
1.2) and a VEG of 1.9 s. With the old model the trim difference looked like
a whole octave, but part of that was the Env Mod law, which the x0x fit now
explains. The profile scores 2.13 on the 13 samples; the refit itself
reached 1.97 with per-sample knob offsets and timing, which a profile does
not carry. The old-model fit (`archive/acidvoice-2026-09-27-old-model.json`)
scored 2.42.

### `factory`: best guess at a new unit

`x0x` with the parts that drift or age reset to nominal:

| Part | Schematic / service manual | x0x unit (fitted) | `factory` |
|---|---|---|---|
| TM3 cutoff trim | service check rings at 2 ms +/- 0.5 ms (400-670 Hz) | 184 Hz base, check at ~350 Hz | 274 Hz base, check at ~525 Hz |
| C13 1 uF (accent sweep) | R46 x C13 47 ms, VR4b x C13 50 ms | 31 / 44 ms, mix 145 ms | 47 / 50 ms, mix 100 ms |
| C42 1 uF (VEG) | R123 x C42 = 1.5 s | 2.7 s | 1.5 s |
| C62 1 uF (MEG) | 68 k / 1.068 M into 1 uF: 68 ms - 1.07 s | 65 ms - 0.98 s | 68 ms - 1.068 s |
| Accented MEG (Decay pot shorted) | 68 ms | 73 ms | 68 ms |

The pot laws, D24's drop, VCA attack and onset delay stay at the x0x
values. They are properties of the circuit and the pots, not of ageing.
The 200 Hz post-filter high-pass also stays. It models the x0x unit's weak
C2 fundamental, and it is not yet known whether that is ageing or design.

Typical ageing of a small 1980s electrolytic is 10-30 % capacitance loss
(end-of-life criterion -20 %), with drying accelerated by heat. Leakage and
ESR changes are too small to matter in these networks. With D24's drop
modelled, a smaller C13 charges further before the diode stops conducting.
So the aged `x0x` accent squelch peaks earlier and higher than
`factory`'s. This is a model prediction; there is no recording of a new
unit.

### `hellfish`: a loose guess at a Devil Fish

Named Hell Fish because it is not a model of a real Devil Fish: no Devil
Fish was measured, and only the control ranges come from a source. It takes
Robin Whittle's Devil Fish mod
([manual](https://www.firstpr.com.au/rwi/dfish/Devil-Fish-Manual.pdf)) on
a `factory` unit, with its extra controls parked at a typical setting and
mapped onto existing constants:

| Devil Fish control | DF range | Here |
|---|---|---|
| Normal Decay | 30 ms - 3 s | Decay knob spans 30 ms - 3 s |
| Accent Decay | 30 ms - 3 s (stock: fixed ~68 ms) | 200 ms |
| Soft Attack | 0.3 - 30 ms | 3 ms |
| Overdrive (filter input level) | up to 66.6x | 2x |

Not representable yet: Filter Tracking, Filter FM, the Muffler, the Accent
Sweep speed switch, Slide Time, and live DF pots (these are fixed settings
here). Its conformance failures (MEG and accent decay times, VEG onset,
cutoff minimum) are the mod's intended departures from the stock
schematic.

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

Clicking the preset display on the logo plate reloads a preset (see above).

## Future work on presets

- per-part toggles (aged C13, trim high/low) that apply a subset of a
  profile;
- the preset as an automatable CLAP parameter instead of only saved state;
- for the Hell Fish, the Devil Fish's extra controls as real front-panel knobs.
