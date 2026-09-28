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
| `x0x-envmod-test` | `x0x` with an Env Mod law read off the E3/D3 sweep tracks | Listening test for the Env Mod sweep (taper 2, bias offset 0.35, cutoff trim 240 Hz, accent 5 oct); not a final calibration. |
| `x0x-acidvoice-trim` | `x0x` with the Acidvoice unit's cutoff range and accent depth | The x0x model with the higher filter trim, for a brighter, higher squelch. |

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
