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

## Full staged refit (both topologies, equal budget)

Two arms, one at a time on 4 cores, every constant free except those the
shipped x0x fit also held (saw LPF/bend, square duty, notch, all-pass,
output coupling, MEG attack, VCA saturation), `--bound
filterResonanceLimit=0.9:0.995`, no early stopping:

- **O**: Open303 topology, cap scales free (the shipped model), from `x0x`.
- **N**: Stinchcombe network, cap scales at 1.0, from `x0x` with the network on.

| Stage | Notes | Window | Knobs | Time | O | N |
|---|---|---|---|---|---|---|
| 1 | 40, every knob at an end stop | 900 ms | chart values | 35 min | 3.24 -> 2.61 | 3.68 -> 2.64 |
| 2 | quarter 0 (100) | 900 ms | free, prior 0.3 | 45 min | 3.77 -> 2.89 | 3.55 -> 2.90 |
| 3 | quarter 2 (100) | full | free, prior 0.3 | 40 min | 3.47 -> 2.89 | 3.37 -> 2.70 |

All 400 notes, full length (300 of them never fitted in stages 2-3):

| Metric | shipped `x0x` | O | **N** |
|---|---|---|---|
| Weighted error | **3.35** | 3.48 | 3.39 |
| Resonant-peak shape (dB) | 3.55 | 3.83 | **3.24** |
| Harmonics over time (dB) | 5.30 | 5.34 | **5.29** |
| Resonant-peak sweep track (semitones) | **3.19** | 3.75 | 3.67 |
| Harmonic levels (dB) | 2.32 | **2.30** | 2.53 |
| Envelope (dB) | 3.11 | 3.22 | **3.06** |
| Note level, RMS (dB) | 1.28 | 1.27 | **1.27** |
| Conformance (`--fast`) | 24 / 34 | 22 / 34 | **28 / 34** |

Per row: N is best in D (4.51 against 4.67 shipped, 5.16 O), O in B, the
shipped profile in A, C and E.

What the network fit chose, against the shipped `x0x`:

| Constant | x0x | O | N | Schematic |
|---|---|---|---|---|
| `filterFeedbackGain` | 19.29 | 19.12 | **18.78** | 18.7 (Stinchcombe) |
| `filterPostHpHz` | 199 | 204 | **69** | 159 / 72 (the two VCA taps) |
| `vegDecaySec` | 2.68 | 2.91 | **1.54** | 1.5 (R123 x C42) |
| cap scales | 1.29 / 0.70 / 0.91 / 1.06 | 0.71 / 0.42 / 0.74 / 1.23 | 1.0 (fixed) | 1.0 |
| `filterNetworkTimeScale` | - | - | 1.11 | 1.0 |

Findings:

- **At equal budget the network wins again** (3.39 against 3.48), and
  passes 28 conformance checks against 22: it keeps B4 and B6 and fails
  only C1, D5, E9 and the unit's cutoff trim (E1-E3).
- **It removes the compensations.** Fitted freely, the Open303 model needs
  a 200 Hz post-filter high-pass, a 2.7-2.9 s VEG and wildly spread ladder
  capacitors to match this unit. With the network the same data puts the
  feedback at Stinchcombe's value, the VEG at the schematic's 1.5 s and the
  post-HP near the 220 k VCA tap's 72 Hz, with schematic capacitors. The
  low end the old fit got from a 200 Hz high-pass comes from the coupling
  network instead.
- **Neither 2-hour refit beats the shipped profile overall** (3.35), which
  had a longer multi-stage fit. N is better than it on peak shape,
  harmonics over time, envelope and the D row, worse on the sweep track,
  harmonic levels and the E row.

## Second unit: Acidvoice (13 notes)

Does the x0x result repeat on an independent unit? Two arms, 20 min each
(~12,000 evaluations), the free set of the original `acidvoice` fit
(cutoff law, accent depths, feedback and limit, Env Mod scale and offset,
VEG, VCA tap, post-HP, oscillator coupling, knobs, timing), `--w-harmt 2`,
limit bounded to 0.995:

- **AC**: from `x0x-circuit` (network on), plus `filterNetworkTimeScale`.
- **AO**: from `x0x` (Open303 topology), the original `acidvoice` recipe.

Predicted beforehand: a tie on score (all 13 notes have Env Mod 0, where
the network gained least on x0x), with the network fit landing on schematic
values (post-HP ~70 Hz, feedback ~18.7, VEG 1.5-1.9 s, time scale 0.9-1.1).

| Standard scoring (13 notes) | `acidvoice` (shipped) | AO | AC |
|---|---|---|---|
| Weighted error | **2.07** | 2.28 | 2.99 |
| Harmonic levels (dB) | | 1.79 | 2.30 |
| Resonant-peak shape (dB) | | 2.58 | 3.82 |
| Note level, RMS (dB) | | 2.25 | 3.05 |

| Constant | `acidvoice` | AO | AC | predicted | `x0x-circuit` |
|---|---|---|---|---|---|
| `filterPostHpHz` | 189 | 231 | **80** | ~70 | 69 |
| `vegDecaySec` | 1.91 | 1.66 | **1.58** | 1.5-1.9 | 1.54 |
| `filterFeedbackGain` | 17.9 | 17.7 | 17.6 | ~18.7 | 18.8 |
| `filterNetworkTimeScale` | - | - | 0.79 | 0.9-1.1 | 1.11 |
| `accentVcaDepth` | 2.29 | 1.83 | 3.48 | | 2.09 |

- **The score prediction was wrong**: on this unit the network fits worse
  (2.99 against 2.28 at equal budget), mostly in the resonant-peak shape and
  note levels; the high accent VCA depth looks like compensation.
- **The low-end prediction held**: on the second unit too, the network
  replaces the ~200 Hz post-filter high-pass with ~80 Hz, next to the 220 k
  VCA tap's 72 Hz, and the VEG lands near R123 x C42.
- **Feedback and time scale did not**: the feedback stays at this unit's
  lower 17.6 in both topologies (it is the tamer unit), and the network's
  time constants come out 0.79 here against 1.11 on x0x. Within electrolytic
  tolerance (+-20 % around ~0.95), but it means the two units do not agree
  on one network.
- Caveats: 13 saw notes, Env Mod 0 throughout, lower pitches (A1-D3, down
  to 53 Hz, below the x0x set's C2) where the network's 92 Hz section bites
  hardest, and an unknown recording chain (an AC-coupled interface adds a
  high-pass the fit would absorb).

No Acidvoice circuit preset: the network is not better for this unit.

## Next

- The network is the better model at equal effort, closer to the schematic
  and with fewer compensating constants; the shipped `x0x` still edges it
  on the overall score. Options: a `factory` profile on the network (no fit
  involved; 32 / 34), and a longer N fit (more rounds of stages 2-3 on the
  other quarters) to beat 3.35 before making it the `x0x` default.
