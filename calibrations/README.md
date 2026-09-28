# Calibration profiles

Acidus uses one circuit model. What differs between hardware TB-303s, and
so between reference sources, is a set of constants: trimmer settings,
component tolerances and envelope times. Each profile here holds every
`SynthParameters` calibration constant fitted to one source, plus notes
on where it came from.

| Profile | Source | Notes |
|---|---|---|
| `x0x` | dinsync.info reference recordings (`test/resources/x0x-reference`, 400 notes) | **Current default.** Full knob sweep, saw + square, C2 only. See `docs/X0X_CALIBRATION_2026-09-28.md`. |
| `acidvoice` | Acidvoice single-note samples (`test/resources/303_saw-*.wav`, 13 notes) | The default until 2026-09-28. Saw only, knobs at min/half/max, notes A1-D3. |
| `x0x-sweep` | The x0x unit as recorded, calibrated on its resonant-peak sweeps | **Recommended to try.** Env Mod taper and bias shift, Decay pot taper 18, accent sweep with D24 forward drop, C13 ~25 % below nominal (aged). 400 notes: weighted error 3.55 (x0x 3.43), harmonic 3.01 dB (3.21), sweep 5.44 st (6.39). |
| `x0x-sweep-nominal` | `x0x-sweep` with a new, nominal C13 | "Fresh component" flavour; extrapolated, not recorded. |
| `x0x-envmod-test` | `x0x` with a hand-derived Env Mod law and a very fast C13 (ideal diode) | Superseded by `x0x-sweep`; kept as the "badly worn C13" flavour (C13 behaving like ~0.3 uF). |
| `x0x-acidvoice-trim` | `x0x` with the Acidvoice unit's cutoff range and accent depth | The x0x model with the higher filter trim, for a brighter, higher squelch. |

## Flavours: aged vs. new components

The recordings are of one ~40-year-old unit, so some fitted constants
describe *that unit's* parts, not the schematic's. A profile can
therefore model a specific unit as recorded, or the same circuit with new
components. Both are valid sounds, and the choice is a matter of taste.

Parts where age or tolerance shows up in the sound, and what the x0x
recordings say about them:

| Part | Schematic | x0x unit (fitted) | Effect | Profiles |
|---|---|---|---|---|
| C13 1 uF electrolytic (accent sweep) | R46 x C13 47 ms, VR4b x C13 50 ms | ~25 % faster (35.8 / 35.0 ms) with D24's drop modelled; ~70 % faster with an ideal diode | Accent squelch peaks earlier and higher | `x0x-sweep` (aged), `x0x-sweep-nominal` (new), `x0x-envmod-test` (badly worn) |
| D24 (accent sweep diode) | ideal in the old model | forward drop 0.30 of the MEG swing | Ends C13's charging early: earlier, more pointed accent peak | all `x0x-sweep*` |
| VR6 Decay pot (1 M audio taper) | a = 81 (10 % at mid-travel) | a ~ 18, max tau 0.97 s | Mid-travel Decay settings last longer | all `x0x-sweep*` |
| Env Mod pot | linear law | ~ EnvMod^2 | Little sweep until mid-travel, then a lot | all `x0x-sweep*` |
| TM3 cutoff trim | set by the service procedure | 240 Hz (x0x), ~250-310 Hz (Acidvoice) | Whole filter range up/down an octave | `x0x-acidvoice-trim` borrows Acidvoice's |
| C62, C42, C41 (MEG/VEG timing electrolytics) | per schematic | not yet separated from pot laws | Envelope times | - |

Typical ageing of a small 1980s electrolytic is 10-30 % capacitance loss
(end-of-life criterion -20 %), with drying accelerated by heat.
Leakage and ESR changes are too small to matter in these networks. In
the current model, a worn C13 gives a *stronger* accent squelch: with the
diode drop, a smaller capacitor charges further before D24 stops
conducting. `x0x-sweep-nominal`'s accent peak at E3 p1 is ~3.7 kHz vs.
~4.5 kHz for `x0x-sweep`. This is a model prediction; there is no recording
of this unit with a fresh C13.

### Future: selectable calibrations in the plugin

Each profile is a flat set of `SynthParameters` constants, so the plugin
could load them at run time instead of compiling one set in, e.g. a
"Unit" or "Character" selector (x0x as recorded / x0x new parts /
Acidvoice / ...), or per-part toggles (aged C13, trim high/low) that
apply a subset of a profile. Things to decide first:

- ship the profiles as embedded tables or as JSON next to the plugin;
- whether a profile switch is a CLAP parameter (automatable, saved with the
  project) or a preset;
- keep profile field names in step with `SynthParameters` (the calibrator
  and `calibration_profile.py` already fail on unknown fields).

## Switching

```bash
python3 tools/calibration_profile.py list                 # which profile SynthEngine.hpp matches
python3 tools/calibration_profile.py diff acidvoice x0x
python3 tools/calibration_profile.py apply acidvoice      # rewrites the SynthParameters defaults
cmake --build build                                       # rebuild the plugin
python3 tools/calibration_profile.py capture NAME --source "..."   # snapshot the current defaults
```

The calibrator can score or fit from any profile without touching the header:

```bash
python3 tools/calibrate_reference.py --calibration calibrations/acidvoice.json --evaluate-only
python3 tools/calibrate_reference.py --manifest --calibration calibrations/x0x-acidvoice-trim.json --evaluate-only --no-sensitivity
```

## What actually differs between the two units

The two profiles differ in 35 constants, but most of those differences
don't matter. Starting from `x0x`, each row below refits only the listed
constants to the 13 Acidvoice samples. Lower is better; the full
`acidvoice` fit scores 2.42 and `x0x` unchanged scores 10.80.

| Refitted on Acidvoice (from x0x) | Error | Fitted values |
|---|---|---|
| nothing | 10.80 | |
| `accentSweepDepthOct` | 9.15 | 9.0 oct (at its bound) |
| `filterFeedbackGain` | 10.30 | |
| `filterResonanceSkew` | 10.80 | (no effect: Acidvoice's resonance is only at min/max) |
| `cutoffSpanOct` | 5.07 | |
| **`cutoffBaseHz`** | **3.91** | **312 Hz** (x0x 159 Hz) |
| cutoff law (base, span, taper) | 3.10 | 251 Hz, 3.01 oct, 1.13 |
| cutoff law + feedback gain, resonance curve, post-HP | 2.78 | (no better than adding the accent depth) |
| **cutoff law + `accentSweepDepthOct`** | **2.66** | **255 Hz, 2.98 oct, 1.29, 5.08 oct** -> `x0x-acidvoice-trim` |
| all 16 CLAP-mapped constants | 2.81 | the 4 ladder pole scales rise to x1.6-3.1 (geometric mean ~2, one octave) to fake the missing cutoff trim |

So the main difference is **the cutoff trim**: the Acidvoice unit's whole
Cutoff range sits roughly an octave higher. That range is set by the TM3
trimmer (the service manual's VCF calibration), and trims differ between
units and drift with age. Accent sweep depth is the
second difference (5.1 vs 4.1 octaves). A deeper accent sweep alone cannot
recover the Acidvoice squelch, because four of its five high-resonance
notes are unaccented and resonate around 3 kHz without accent. It does
matter a lot in sequences, where accented steps drive the high squelch.

## CLAP mapping (calibration build, `-DACIDUS_CALIBRATION_BUILD=ON`)

The cutoff knob law and the Resonance knob curve are CLAP parameters, so the
two units can be compared in a host:

| CLAP id | Name | SynthParameters field | x0x | acidvoice | x0x-acidvoice-trim |
|---|---|---|---|---|---|
| 32 | Cutoff Trim (Base Freq) | `cutoffBaseHz` | 159.5 Hz | 248.2 Hz | 254.9 Hz |
| 33 | Cutoff Knob Span | `cutoffSpanOct` | 2.69 oct | 3.16 oct | 2.98 oct |
| 34 | Cutoff Knob Taper | `cutoffTaperExp` | 1.61 | 1.0 | 1.29 |
| 29 | Accent Sweep Depth | `accentSweepDepthOct` | 4.05 oct | 5.0 oct | 5.08 oct |
| 35 | Resonance Knob Curve | `filterResonanceSkew` | -0.56 | 3.0 | -0.56 |

With the x0x defaults, setting ids 32-34 and 29 to the `x0x-acidvoice-trim`
column reproduces that profile exactly. The Acidvoice-specific constants that
are still not CLAP-mapped (the Env Mod law, the accent timing, the envelope
times, the square wave) make up the remaining 2.66 -> 2.42 difference.
