# Stinchcombe's full coupling network vs the Open303 topology (2026-10-03)

**Question.** Does replacing Open303's empirical coupling network around the
filter (input HP, one in-loop HP, out-of-loop notch and all-pass;
TB303_REFERENCE.md §11.3 option 2) with Tim Stinchcombe's full 10-pole /
6-zero network (§11.1, §11.3 option 1) make Acidus closer to the hardware?

**Answer so far.** The network makes the filter match Stinchcombe's analysis
almost exactly, at equal budget it fits the x0x recordings better than the
current topology, and it is best where the current model is weakest (high
Resonance with high Env Mod). Against the shipped `x0x` profile, which had a
much longer fit, it ties. Not yet a preset; see "Next" below.

## Implementation

`filterCouplingNetwork` 1 (Filter.hpp, `StinchcombeNetwork`). His model,

```text
H(s) = 1.06 s^3 (s+109.9)(s+34.0)(s+7.41)
       / [ L(s) D(s) + 18.7 k s^4 (s+46.5)(s+4.40) ]
D(s) = (s+97.5)(s+38.5)(s+4.45)(s+578.1)(s+20.0)(s+7.41)     (rad/s)
```

is exactly the loop `u = Fin x - k Floop y4`, `out = 1.06 y4` around the
ladder, with `Fin = s^3 (s+109.9)(s+34.0)(s+7.41) / D` and
`Floop = s^4 (s+46.5)(s+4.40) / D`. Both are cascades of first-order
sections with real roots (bilinear, double precision, at the 8x rate); the
loop path's output is affine in the step's y4, so it goes into the existing
Newton solve unchanged. The linear model only fixes the product of the input
and output networks; all of it is placed ahead of the ladder, so the sub-bass
hump reaches the ladder's operating point. The output stage's inversion
(§2.3) is applied at the filter output, which the Open303 chain got from its
all-pass. `filterNetworkTimeScale` scales every RC time constant.

The critical-gain search covers both +-180 deg crossings of the loop (main
resonance and sub-bass hump). Filter CPU cost: +4 %.

## Conformance

`acidus_reference_test --fast --params <profile>`:

| Check | `factory` (Open303) | `factory` + network (ceiling 18.7, limit 0.98) | Stinchcombe |
|---|---|---|---|
| B2 pass-band loss 100 Hz | 20.2 dB | 20.6 dB | 20.6 dB |
| B3 pass-band loss 30 Hz | 15.2 dB | 14.2 dB | 14.2 dB |
| B4 resonant peak height | +0.5 dB **FAIL** | +9.0 dB | +9.1 dB |
| B5 sub-bass hump | +15.5 dB | +15.4 dB | +15.4 dB |
| B6 low end at Resonance 0 | +2.6 dB **FAIL** | -4.8 dB | -4.8 dB |
| Total | 30 / 34 | **32 / 34** | |

The two remaining failures (C5 square edge ringing, E9 fast MEG term in the
VCA) are in the oscillator and VCA, not the network.

## Fit to the x0x recordings

Three arms from the shipped `x0x` profile, identical flags and budgets:

- **X**: Open303 topology, ladder cap scales free (the shipped model).
- **C**: Open303 topology, cap scales at the schematic's 1.0.
- **N**: Stinchcombe network, cap scales at 1.0.

Free: the filter group, the cutoff law (`cutoffBaseHz`, `cutoffSpanOct`,
`cutoffTaperExp`), `accentSweepDepthOct`, `accentVcaDepth`,
`vcaResTapRatio`, `envModOffset`. Envelope, oscillator and pot-taper
constants stay at the `x0x` values. Scoring adds `--w-harmt 2` (every
harmonic's level over time, 40 ms frames every 10 ms) to `--w-sweep 1`.

1. **Outer knob positions** (40 notes: A1-A5 and E1-E5 at position 4, every
   knob at an end stop; 24 of them at Resonance 100 %), knobs fixed at the
   chart values, 900 ms, 30 min each. All three land at 2.60-2.67.
2. **All positions** (`--rotate 0`, 100 notes), knob positions free with
   prior 0.3, cutoff per row, 900 ms, 28 min each.
3. **Score** on all 400 notes, full length, nominal knobs.

| All 400 notes | shipped `x0x` | X | C | **N** |
|---|---|---|---|---|
| Weighted error | **3.35** | 3.43 | 3.41 | 3.36 |
| Harmonics over time (dB) | 5.30 | 5.38 | 5.39 | **5.27** |
| Resonant-peak sweep track (semitones) | 3.19 | 3.70 | 3.25 | **3.04** |
| Resonant-peak shape (dB) | 3.55 | **3.51** | 3.66 | 3.68 |
| Harmonic levels (dB) | **2.32** | 2.39 | 2.34 | 2.35 |
| Note level, RMS over notes (dB) | 1.28 | **1.12** | 1.60 | 1.82 |
| Harmonics within 3 dB | **76 %** | 74 % | 76 % | 76 % |

Mean weighted error per row (rows set the four fixed knobs: A 0 %, B 25 %,
C 50 %, D 75 %, E 100 %):

| Row | shipped | X | C | N |
|---|---|---|---|---|
| A | **1.90** | 1.96 | 1.91 | 2.05 |
| B | 2.69 | **2.38** | 2.57 | 2.47 |
| C | 2.70 | 2.57 | 2.77 | **2.57** |
| D | 4.67 | **4.52** | 4.85 | 5.03 |
| E | 4.80 | 5.70 | 4.95 | **4.67** |

Findings:

- **At equal budget the network fits best** (3.36 against 3.41 and 3.43),
  and it has the best harmonics over time and resonant-peak track of all
  four, including the shipped profile.
- **It is best in the E row** (Resonance 100 %, Env Mod and Decay high),
  the known weak spot (`X0X_CALIBRATION_2026-09-28.md`, gap 1), and worst in
  the D row and at Resonance 0 (A row).
- **Schematic cap scales cost nothing**: C (1.0) against X (fitted) is 3.41
  against 3.43. The fitted scales were compensating, not measuring.
- **Note levels are worse** with the network (1.82 dB RMS against 1.28).
  The network's 92 Hz section attenuates the C2 fundamental, and the fit has
  not rebalanced the level constants yet.
- The N fit put `filterResonanceLimit` at 1.014 and the network time scale
  at 0.90. With the limit above 1 it self-oscillates at cutoffHz >= 6.4 kHz
  (B1 fails), and its feedback (17.9) gives a lower peak than Stinchcombe's
  (B4 +3.7 dB). Its other failures are the x0x unit's own (trim, VEG).
- An early round of this comparison was invalid: the fits stopped on the
  patience rule after a handful of CMA-ES generations (~10 s per
  evaluation). Smaller subsets, one fit at a time, and a per-thread cache of
  the critical-gain table (it was rebuilt for every rendered note) fixed it.

## Next

- A full staged refit of both topologies with equal, longer budgets and all
  constants free, to decide against the shipped profile rather than at
  28 minutes per stage.
- Bound `filterResonanceLimit` below 1 for stock-unit profiles (B1).
- Then, if N still wins, a `factory`-style network profile and a refit of
  `x0x` with it.
