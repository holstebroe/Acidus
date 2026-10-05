# Calibration parameters and knob laws

This document explains every knob law and every calibration constant in
Acidus: what part of the circuit it stands for, what the schematic says,
what real units measure, what each shipped profile uses, and what you hear
when it changes. It is the "why" behind `SynthParameters`
(`src/core/SynthEngine.hpp`) and the profiles in `calibrations/`.

How to calibrate against a new source is in
[`CALIBRATION_COOKBOOK.md`](CALIBRATION_COOKBOOK.md). The circuit facts and
their sources are in
[`TB-303 Reference/TB303_REFERENCE.md`](TB-303%20Reference/TB303_REFERENCE.md)
(cited below as §n).

## 1. There is no single "real" TB-303

"Sounds exactly like a 303" has no single answer, because no two 303s sound
exactly alike. Roland built them from parts with wide tolerances, set them
up with trimmers, and they have now aged for about 40 years. Measured
spreads (§20):

| What varies | Observed spread | Why |
|---|---|---|
| Cutoff range | ±25 % allowed by the service procedure; the x0x unit sits ~0.6 oct below the target | TM3 trimmer, set by hand at the factory or a repair shop |
| Octave scale | 0.99-1.03 (ladyada 0.997, rv0 1.027) | TM5 trimmer, expo-converter transistors |
| Maximum resonant peak | 23.5 kHz vs 27.5-28 kHz on two units | Transistor gain, trim, age |
| Resonance ceiling | varies with age, revision and pot wear | VR4 wear, transistor matching |
| Envelope and accent timing | electrolytics are ±20 % new and lose 10-30 % with age | C13, C42, C62 (1 uF electrolytics) set the time constants |
| Square pulse width | varies | JFET pinch-off (Q28), C11 |
| Pot laws | carbon pots of the era are rough two-segment curves | VR3-VR8 |

So Acidus does not aim at one magic sound. It aims at:

1. **The exact circuit structure**, where it is known and affordable. The
   structure is the same in every unit.
2. **Nominal component values and factory calibration targets** as the
   reference point (the `factory` profile).
3. **Measured units as profiles** (`x0x`, `acidvoice`): the same model with
   the constants that differ between units fitted to recordings.
4. **Variation as a fixed, per-profile setting**, never random per note (§20).

### Evidence tags

Each value below carries the tag of its strongest source, as in the
reference:

| Tag | Meaning |
|---|---|
| **S** | Schematic, parts list or Roland service notes |
| **A** | Published circuit analysis (Stinchcombe, mystran) |
| **D** | Derived here from S or A |
| **M** | Measured on hardware by someone else (antto, rv0, ladyada, Robin Schmidt) |
| **I** | Another emulator's choice (mostly Open303) |
| **F** | Fitted by `tools/calibrate_reference.py` to recordings in `test/resources` |
| **E** | Estimate with no source |

Where a fitted value and a schematic value disagree, the disagreement is
evidence, not noise. Either the unit has drifted (an aged capacitor), or
the model has a gap the fit is papering over. Both cases are called out.

### The profiles

| Profile | What it is |
|---|---|
| `x0x` (default) | One ~40-year-old unit, fitted to 400 recorded notes (dinsync.info set) |
| `acidvoice` | A second unit, the `x0x` model refitted to 13 notes |
| `factory` | The schematic: every value the schematic, parts list or service manual fixes is set to it, even where that costs fit score; everything else from the `x0x` fit |
| `hellfish` | Not a 303 that exists: the `factory` core with Devil Fish ranges and community mods, tuned for sound (section 5) |

Scores and the profile procedure are in
[`calibrations/README.md`](../calibrations/README.md).

---

## 2. Front-panel knobs

All panel knobs are 0-1 in the plugin. The law turns knob travel into what
the circuit does with it. The pot types are from the parts list (§1 note 6):
Cutoff, Env Mod and Volume are 50 kΩ-A (audio), Decay is 1 MΩ-A, Tuning and
Accent are 50 kΩ-B (linear), Resonance is a dual 50 kΩ-B.

### A note on "A" (audio) pots

The reference models an A pot as `R(θ) = (a^θ − 1)/(a − 1)` with a ≈ 81,
which is 10 % of the resistance at mid-travel (§14.1, tagged as an estimate
for the exact curve). Real carbon A pots of the era are closer to **two
straight segments with 15-20 % at mid-travel**. The x0x recordings agree
with that, not with a = 81:

| Env Mod knob | 25 % | 50 % | 75 % |
|---|---|---|---|
| x0x measured sweep depth (fraction of max) | 0.19 | 0.29 | 0.71 |
| a = 81 exponential | 0.03 | 0.10 | 0.33 |
| two-segment, 15 % at mid | 0.08 | 0.15 | 0.58 |
| shipped logistic S-curve | 0.03 | 0.19 | 0.68 |

(The measured depth includes the residual sweep at Env Mod 0, so its low
end reads higher than any pot law.) The Decay pot shows the same: the fitted
taper a ≈ 18 is 19 % at mid-travel. So the fitted curves are the best
available description of the schematic's own A pots, and every profile
keeps them. The exception is Volume, which has no recording to fit and uses
a = 81.

### Cutoff (VR3, 50 kΩ-A, with R47 10 kΩ)

- **Law:** `cutoff = cutoffBaseHz · 2^(cutoffSpanOct · knob^cutoffTaperExp)`,
  before the envelope. The exponential converter (Q10/Q11) makes the knob
  work in octaves; the exponent bends the knob between the ends.
- **Hardware:** the service check (Cutoff centre, Resonance max, everything
  else min, saw on C) must ring with a 2 ms ± 0.5 ms period, 400-670 Hz [S].
  Open303's fit of hardware puts the range at 314-2394 Hz with a plain
  exponential knob [I/M]. How VR3 and R47 load the summing node cannot be
  read reliably from the scan, so the law between the ends is not fixed by
  the schematic.
- **Profiles (settled cutoff at 0 / 25 / 50 / 75 / 100 % travel):**

  | Profile | Base | Span | Exponent | Hz |
  |---|---|---|---|---|
  | `x0x` | 184 Hz | 3.09 oct | 1.43 | 184 / 247 / 408 / 762 / 1569 |
  | `acidvoice` | 220 Hz | 3.20 oct | 2.26 | 220 / 242 / 349 / 698 / 2014 |
  | `factory` | 274 Hz | 3.09 oct | 1.43 | 274 / 368 / 606 / 1132 / 2331 |
  | `hellfish` | 120 Hz | 4.60 oct | 1.43 | 120 / 186 / 392 / 994 / 2910 |

- **What you hear:** the x0x unit is darker at every knob position. Its
  service check rings at ~350 Hz, below the manual's tolerance, so its TM3
  trim is low. That is a trimmer setting, not wear. `acidvoice` packs most
  of its range into the top half of the travel.
- **Why the fitted exponent differs between units:** pot taper, R47 and
  trimmer interact. Two units gave 1.43 and 2.26, and Open303 used 1.0.

### Resonance (VR4, dual 50 kΩ-B)

- **Law:** feedback = `skew(knob) · ceiling`, with
  `skew(x) = (1 − e^(−k·x)) / (1 − e^(−k))`, `k = filterResonanceSkew`.
  The second gang (VR4b) also sets the accent sweep shape (section 3.6) and
  is linear.
- **Hardware [S][D]:** VR4a is a linear pot, but its wiper feeds the Q18
  buffer through C15, and Q18's base bias (R66/R99, 100 kΩ each) loads it
  with about 50 kΩ. A loaded linear pot gives
  `x / (1 + x(1 − x))`: 0.21 / 0.40 / 0.63 at 25 / 50 / 75 %. The
  best-fitting skew is **k = −0.865**.
- **Profiles:** `x0x` and `acidvoice` use the x0x fit, **k = −0.872**;
  `factory` and `hellfish` use the derived −0.865. The two agree within
  1 %: two independent routes (the recordings and the circuit) give the
  same law.
- **Open303 uses k = +3** [I], which brings resonance in early (0.82 at
  mid-travel). It is an ear-tuned choice for its own filter, not a
  schematic law, and it bends the opposite way.
- **What you hear:** resonance builds late. Half-way only gives about 40 %
  of the maximum feedback; most of the squelch arrives in the last third.
- **Unit variation:** the ceiling (`filterResonanceLimit`, section 3.3)
  varies with age and pot wear; `acidvoice` fits 0.96 of critical,
  `x0x` 1.00.

### Env Mod (VR5, 50 kΩ-A, with R61 10 kΩ)

- **Law:** octaves per unit MEG =
  `(1 − c)·(C0 + C0Slope·t) + c·(C1 + C1Slope·t)`, settled bias shift
  `−envScaler·(envModOffset + envModOffsetCutSlope·c)`, where c is the
  Cutoff knob and t is the Env Mod knob through the pot law (logistic
  S-curve, mid 0.69, width 0.12). From Open303's fit of hardware [I/M],
  refitted to the x0x sweeps [F].
- **Hardware [S]:** Env Mod is not "add envelope". It is a bias shift in
  the octave domain: turning it up raises the envelope *and* lowers the
  settled cutoff (Service Notes, §13.1). Env Mod 0 still leaves a residual
  sweep of ~0.7 oct.
- **Measured [M]:** x0x sweep depth 0.96 / 1.50 / 3.60 / 5.06 oct at 25 /
  50 / 75 / 100 %; the settled cutoff drops 0.35 × the added depth.
- **Profiles:** the S-curve is the same in all profiles (it is the pot).
  `acidvoice` has a deeper residual (C0 0.93 vs 0.76) and a smaller bias
  shift (offset 0.18 vs 0.33). `hellfish` multiplies the depth slopes by
  1.4 for a wider sweep.
- **What you hear:** little change in the first half of the knob, then the
  wah opens up quickly between 50 and 90 %.

### Decay (VR6, 1 MΩ-A)

- **Law:** MEG time constant `τ = min + (max − min)·(a^x − 1)/(a − 1)`.
- **Hardware [S][D]:** τ = 68 kΩ × 1 µF = **68 ms** at minimum,
  (68 kΩ + 1 MΩ) × 1 µF = **1.07 s** at maximum. The schematic prints
  T90 = 200 ms / 2.5 s, which agrees.
- **Profiles:**

  | Profile | Min | Max | Taper a | τ at 25 / 50 / 75 % |
  |---|---|---|---|---|
  | `x0x` | 65 ms | 0.98 s | 18 | 0.12 / 0.24 / 0.48 s |
  | `factory` | 68 ms | 1.07 s | 18 | 0.13 / 0.26 / 0.52 s |
  | `hellfish` | 30 ms | 3.0 s | 18 | Devil Fish range |
  | (a = 81 for comparison) | 65 ms | 0.98 s | 81 | 0.09 / 0.16 / 0.36 s |

- **Why x0x is shorter:** C62 (1 µF electrolytic) about 8 % low, inside
  normal ageing.
- **Measured [M]:** x0x MEG τ 0.08-0.14 / 0.24-0.30 / 0.54-0.74 /
  0.90-1.00 s at 25 / 50 / 75 / 100 %. One famous unit (Josh Wink's) needs
  about 2.3× longer decays, possibly modified (§20).

### Accent (VR7, 50 kΩ-B)

- **Law:** linear. It scales both the accent VCA term and the accent sweep.
  Matches the hardware [S].

### Tuning (VR2, 50 kΩ-B)

- **Law:** linear in cents, ±700 cents (a fifth each way) [S]. MIDI CC 24
  maps 64 to exactly 0. The octave scale (`vcoOctaveScale`, section 3.1)
  applies to it too, since both feed the same expo converter.

### Waveform

- A switch, saw or square. The square is made from the saw by a PNP shaper,
  so its shape follows the saw core (section 3.1).

### Volume (VR8, 50 kΩ-A)

- **Law:** audio taper a = 81 (−20 dB at mid-travel), before a fixed output
  amplifier (×8) and before the pedal, as on a real 303 into a pedal (§17).
  No recording constrains its exact curve.

### Drive and auto output (MXR Distortion+, not part of the 303)

- **Drive:** the pedal's gain sweeps 9.5-46.6 dB evenly in dB, which
  tracks the reverse-log Distortion pot within ~2 dB. Drive 0 is the
  footswitch (bypass).
- **Auto output:** a static trim standing in for the pedal's Output knob,
  `kAutoOutputAmount` in `src/core/Distortion.cpp`. See the comment there.

---

## 3. Calibration constants

Values are listed `x0x / acidvoice / factory / hellfish` where they differ. `x0x` and
`acidvoice` are best fits to their recordings; `factory` is the schematic
wherever the schematic fixes the value; `hellfish` is tuned for sound.

### 3.1 Oscillator

| Parameter | Value | Source | What it is and does |
|---|---|---|---|
| `vcoOctaveScale` | 1.0 everywhere | S target, M spread | TM5 width trim. 1 = exact 2:1 octaves, which the service procedure sets to ±0.5 % over two octaves. Real units: 0.99-1.03. Pivots on A2 = 110 Hz (the TM4 point). At 1.03, A3 plays 36 cents sharp and the 3-octave range ends about a semitone out at each end. Nobody has measured it on the x0x or Acidvoice units. |
| `oscSquareDutyDepth` | 0.12 | M (x0x) | Square duty = 0.45 + depth·e^(−f/180 Hz). 0.12 gives 53.3 % at C2 (65 Hz) and 51.5 % at 110 Hz, measured from the nulled 15th harmonic. antto's fit on another unit was 0.25 (wider pulses at low notes). The duty sets which harmonics vanish, so it is a strong part of the square's character. It varies with the JFET's pinch-off. |
| `oscSquareLevel` | 0.60 | F, M | Square level relative to the saw. Reference: the saw's peak-to-peak is about twice the square's (§9). |
| `oscCouplingHz` | 47 / 55 / 1 (off) / 20 Hz | I (Open303 44.5 Hz); S: none | One-pole high-pass between oscillator and filter. The hardware has **no** high-pass here (§7.2, antto measured it), so `factory` turns it off (1 Hz is the floor). In the fitted profiles it is part of Open303's empirical network that stands in for coupling capacitors elsewhere; at C2, 47 Hz cuts the fundamental ~1.8 dB. |
| `oscSawLpfHz` | 40 kHz (off) | E, removed | A saw low-pass with no source (§7.2: "drop them"). Bypassed. |
| `oscSawShape` | 0 (off) | E, removed | A quadratic saw bend with no source. Off. |

Fixed in code, not calibration parameters:

- **Slide** τ = 22 ms (C35 0.22 µF × 100 kΩ R-2R Thévenin), on the CV in volts [S][D][M].
- **Pitch reference:** A2 = 110 Hz at centre Tuning (TM4) [S].
- **Phase** never resets [M].

### 3.2 Filter core (the diode ladder)

| Parameter | Value | Source | What it is and does |
|---|---|---|---|
| `filterCapScale1..4` | 1.29 / 0.70 / 0.91 / 1.06 (fits); 1.0 (`factory`, `hellfish`) | F; S = 1.0 | Per-stage pole-frequency scales. **1.0 = the schematic** (C19/C24/C26 33 nF, C18 18 nF), and with 1.0 the core matches Stinchcombe's transfer function within 0.3 dB (conformance A1). The fitted values are not capacitor drift: these are film and ceramic parts, and 30 % spread is implausible. They compensate for something else the model gets wrong (likely the simplified coupling network, section 3.3). Setting them to 1.0 in the old `factory` made its fit to both recorded units worse (x0x 3.52 → 3.67, Acidvoice 3.36 → 3.95) and changed no conformance check. `factory` uses 1.0 anyway: it is the schematic. |
| `filterLadderTopology` | 0 / 0 / 1 / 1 | S/A for 1 | 0 = the original, mirrored ladder; 1 = the circuit orientation (§10.3: the Q12 input pair saturates on input − feedback, the half capacitor on stage 1). Both are linearly identical; they differ when driven hard, where 1 gives the input-pair "growl". The fitted profiles were fitted with 0, and switching `x0x` to 1 without a refit worsens its score (2.75 → 3.03). |
| `filterLadderInputScale` | 0.035 / 0.035 / 0.035 / 0.070 | F, M criteria | How hard the signal drives the ladder's tanh stages. §18: at Resonance 0 the saw should only be mildly rounded; at maximum the ringing should grow a little above the saw before it limits. `hellfish` doubles it (the Devil Fish Overdrive at 2×). |
| `filterFeedbackGain` | 19.3 / 17.9 / 19.3 / 30 | D, F | Feedback ceiling at Resonance 1. The bare ladder oscillates at k = 17 [A]; the coupling network raises the real threshold. Fitted so maximum resonance sits just below self-oscillation near 1 kHz (conformance B8). |
| `filterResonanceLimit` | 1.00 / 0.96 / 1.00 / 1.12 | F | Cap on the feedback as a fraction of the loop's critical gain. A stock 303 sits just below oscillation (Q ≈ 36 at k = 1, §11.2). Lower = less scream at full Resonance; Acidvoice is the tamer unit. Above 1 the filter self-oscillates: `hellfish` at 1.12 (with the ceiling at 30) sings on its own from ~94 % of the knob at every cutoff from 250 Hz up, bounded by the ladder's nonlinearity. |
| `filterResonanceSkew` | −0.872 | F, D | The Resonance pot law; section 2. |
| `cutoffMaxHz` | 23.7 kHz | F, M | Ceiling of the cutoff CV. Measured units reach 23.5-28 kHz at full Env Mod (§13.3). |

### 3.3 Coupling network around the filter

The real VCF has about seven coupling capacitors, several inside the
resonance loop (§11). The shipped profiles use Open303's empirical stand-in
for them (§11.3 option 2). `filterCouplingNetwork` 1 switches to
Stinchcombe's full network instead (§11.3 option 1); see
[`STINCHCOMBE_NETWORK_2026-10-03.md`](STINCHCOMBE_NETWORK_2026-10-03.md) for
how it compares against the recordings. With the Open303 topology, several
values below are emulator choices rather than schematic values.

| Parameter | Value | Source | What it is and does |
|---|---|---|---|
| `resCouplingHz` | 104 Hz | I, F | High-pass inside the resonance loop. Open303 150 Hz, antto 122 Hz (range 70-140). It is why resonance fades at low cutoff: low notes ring with about the same amplitude at any cutoff. |
| `filterInputCouplingHz` | 6.0 Hz | F | VCF input coupling. Low values keep the fundamental's phase (waveform shape) and the sub-bass hump near 8-20 Hz that Stinchcombe's model predicts. |
| `filterPostHpHz` | 199 / 189 / 159 / 40 Hz | F; S/D 159 Hz | High-pass after the filter. Open303 uses 24 Hz [I]. The schematic supports a corner far above that: the filter reaches the VCA through R122 100 kΩ + C22 10 nF (159 Hz) and R121 220 kΩ + C21 10 nF (72 Hz) [S][D, §15.4]. At C2 the fitted 199 Hz cuts the fundamental ~10 dB (Open303's 24 Hz: ~0.6 dB). This is a large part of the 303's thin low end, and it is probably design, not ageing. `factory` uses the 100 kΩ tap's 159 Hz; `hellfish`'s bass mod drops it to 40 Hz. |
| `filterNotchHz`, `filterNotchBandwidthHz` | 7.5 Hz, 4.7 oct | I | Open303's sub-audio notch. Reproduces the low-frequency phase and level of the real network in recordings. Not a circuit part. |
| `filterAllpassHz` | 14 Hz | I | Open303's sub-audio all-pass; same role. |
| `filterOutputCouplingHz` | 20 kHz | E | A low-pass for stray capacitance and buffer bandwidth. |
| `filterCouplingNetwork` | 0 (all profiles) | A for 1 | 0 = the Open303 topology above. 1 = Stinchcombe's 10-pole / 6-zero network: a 5-section input network and a 6-section network in the feedback loop, solved with the ladder; the output stage's inversion is applied at the filter output. With 1, `resCouplingHz`, `filterInputCouplingHz`, the notch and the all-pass are unused, and his model puts the feedback ceiling at 18.7. `factory` with 1 (ceiling 18.7, limit 0.98) passes 32 of 34 conformance checks. |
| `filterNetworkTimeScale` | 1.0 | S = 1.0 | Network 1 only: multiplies every RC time constant of the network (its 1 uF electrolytics are +-20 %). A fit to the x0x set lands at ~0.9. |

### 3.4 Envelopes: MEG (filter envelope)

| Parameter | Value | Source | What it is and does |
|---|---|---|---|
| `vcfAttackMs` | 0.1 ms | S/D | MEG charge: D37 + R152 100 Ω into C62. Effectively instant. |
| `vcfDecayMinSec`, `vcfDecayMaxSec`, `vcfDecayTaper` | see section 2, Decay | S, F | The Decay knob. |
| `accentDecaySec` | 73 / 73 / 68 / 200 ms | S, F | MEG decay on accented notes: the accent switch shorts VR6, leaving R136 68 kΩ × C62 1 µF = 68 ms. The Devil Fish adds a separate Accent Decay knob; `hellfish` parks it at 200 ms. |

### 3.5 VEG and VCA (volume)

| Parameter | Value | Source | What it is and does |
|---|---|---|---|
| `vegDecaySec` | 2.68 / 1.91 / 1.5 / 1.5 s | S, F | Held-note volume decay. Schematic: R123 1.5 MΩ × C42 1 µF = **1.5 s** [S]; Open303 fitted 1.23 s [I/M]. After 1 s a held note has dropped 5.8 dB at 1.5 s, 4.5 dB at 1.91 s, and 3.2 dB at 2.68 s. The x0x unit's slow VEG is unexplained (a high C42 or a different R123). It fails conformance E8 by design. |
| `vcaAttackMs` | 1.3 / 1.3 / 2.2 / 3 ms | F; S 2.2 ms | VCA onset. The trigger drive is smoothed by R134 22 kΩ × C41 0.1 µF = 2.2 ms [S]; antto measured "a few ms, up to ~5" [M]. The x0x unit opens faster. `hellfish` parks the Devil Fish's Soft Attack at 3 ms. |
| `vcaNormalDelayMs` | 4.5 ms | M (x0x) | Unaccented notes start sounding ~4.5 ms after accented ones in the x0x recordings, with the same gate time. The circuit mechanism is unknown. |
| `vcaGateOffMs` | 0.81 ms | F, I | Release at gate-off. Open303 1 ms. |
| `vcaGateOffAccentMs` | 2.8 / 2.8 / 2.8 / 30 ms | F | Release of accented notes. Open303 50 ms ("accent tail"). Neither recorded unit shows a long tail; `hellfish` adds a 30 ms one. |
| `vcaResTapRatio` | 1.25 / 1.96 / 2.2 / 2.2 | S trace, F | The filter reaches the VCA through two taps, one from the top of the Resonance pot and one from its wiper (§12, medium-confidence trace). More resonance sends more signal to the VCA, partly offsetting the ladder's ~20 dB pass-band loss. The resistors (100 kΩ, 220 kΩ) give a ratio of 0.45 or 2.2 depending on which goes where. Level rise from Resonance 0 to 1: +3.2 dB at 0.45, +7.0 dB at 1.25, +9.4 dB at 1.96, +10.1 dB at 2.2. Both fits fall between the two readings, nearer 2.2, so `factory` takes 2.2 (the wiper on the 100 kΩ). |
| `vcaGainSaturationDrive` | 0 (linear) | E | A soft ceiling on the VCA's control-to-gain law. Off: the BA662A's ceiling is modelled at the signal input instead. |

### 3.6 Accent sweep (C13 network)

The accent sweep is the "wow" on accented notes (§16.2): the accented MEG
charges C13 1 µF through D24, R46 47 kΩ and the Resonance pot's second
gang; the pot's wiper feeds the cutoff. At low Resonance the cutoff gets a
sharp kick; at high Resonance a delayed, rounded bump. C13 is never reset,
so a run of accents builds up: each peak is higher than the last.

| Parameter | Value | Source | What it is and does |
|---|---|---|---|
| `accentChargeBaseSec` | 31 / 31 / 47 / 47 ms | S, F | R46 47 kΩ × C13. Schematic 47 ms. |
| `accentChargePotSec` | 44 / 44 / 50 / 50 ms | S, F | VR4b 50 kΩ × C13 at full Resonance. Schematic 50 ms. |
| `accentMixSec` | 145 / 145 / 100 / 100 ms | S/E, F | C13's discharge through the mixing resistor (Whittle: 100 kΩ, likely R72). |
| `accentDiodeDrop` | 0.30 of the MEG swing | F, S mechanism | D24's forward drop ends C13's charging early, so the sweep peaks sooner (15-20 ms after note-on in the x0x unit). Without it, matching the recordings needed C13 ≈ 0.3 µF, a 70 % loss. With it, C13 fits ~25 % low, normal ageing for a 40-year-old electrolytic. |
| `accentSweepDepthOct` | 8.6 / 7.9 / 8.6 / 10 oct per unit | F | How strongly the wiper voltage moves the cutoff (summing-resistor ratio). antto measured ~+1 oct on an accented note at Cutoff max [M]. |
| `accentVcaDepth` | 1.62 / 2.29 / 1.62 / 2.3 | F, I | The accented MEG's push on the VCA (R120/C36/R119 path). Open303 uses 4 × Accent × MEG. Acidvoice's accents are noticeably louder. |

At full Resonance, the C13 charge time constant is 75 ms on the x0x unit
(aged) and 97 ms on `factory` (nominal). So on an aged unit the accent wow
peaks earlier and higher, and successive accents stack faster.

---

## 4. The model's known gaps

Not every difference from a real unit is a calibration value. These
are open model items (see `calibrations/README.md` and the conformance test
`acidus_reference_test`), and the fitted values partly compensate for them:

- **B4, resonant peak height** and **B6, low-frequency shape**: the
  simplified coupling network (section 3.3). The fitted capacitor scales
  and coupling corners lean on this.
- **C5, square edge ringing.**
- **E9, the fast MEG term in the VCA**: Robin Schmidt's fit of a real 303
  shows a strong fast decay in the volume (0.76·e^(−t/58 ms)) that the
  model reproduces only partly.
- **Resonance too weak in the attack** at high Resonance with high Env Mod
  (the x0x sets E1, D3, E3, D2).

## 5. Hell Fish: the ultimate 303

`hellfish` is not a model of a real unit. It starts from the schematic
`factory` and adds the modifications people have kept coming back to over
40 years of modding, set for sound rather than accuracy:

| Mod | Origin | What it does | Parameters |
|---|---|---|---|
| Long and short decays | Devil Fish Normal Decay | Decay knob spans 30 ms (tight blips) to 3 s (long sweeps) | `vcfDecayMinSec` 0.03, `vcfDecayMaxSec` 3.0 |
| Accent decay | Devil Fish Accent Decay | Accented notes sweep for 200 ms instead of 68 ms | `accentDecaySec` 0.2 |
| Soft attack | Devil Fish Soft Attack | Rounds the click at note-on | `vcaAttackMs` 3 |
| Overdrive | Devil Fish Overdrive | Twice the level into the ladder; with the circuit-orientation input pair, the filter growls when driven | `filterLadderInputScale` 0.070, `filterLadderTopology` 1 |
| Self-oscillation | Devil Fish; x0xb0x R97 mod | The top ~6 % of the Resonance knob makes the filter sing on its own | `filterResonanceLimit` 1.12, `filterFeedbackGain` 30 |
| Wide cutoff | common range mod | Cutoff reaches lower and higher: ~120 Hz - 2.9 kHz settled | `cutoffBaseHz` 120, `cutoffSpanOct` 4.6 |
| Deeper Env Mod | Devil Fish | 1.4× the sweep depth | `envModScaleC0Slope`, `envModScaleC1Slope` |
| Bass mod | common mod | Much fuller low end (the stock 159-199 Hz post-filter high-pass cuts a C2 fundamental by ~10 dB) | `filterPostHpHz` 40, `oscCouplingHz` 20 |
| Harder accent with a tail | Open303-style tail | Bigger wow and louder accents that ring on for 30 ms after the gate | `accentSweepDepthOct` 10, `accentVcaDepth` 2.3, `vcaGateOffAccentMs` 30 |

Still missing, because the model has no place for them yet: the Devil
Fish's Filter Tracking, Filter FM, Muffler, Accent Sweep speed switch and
Slide Time. Hell Fish fails 12 of the 34 schematic conformance checks; that
is the point.

## 6. Changing a value

- To try a value by ear, build with `-DACIDUS_CALIBRATION_BUILD=ON`; every
  constant above appears in the host's parameter list under
  "Experimental/...".
- To keep it, put it in a profile (`calibrations/<name>.json`) and
  regenerate the presets (`python3 tools/calibration_profile.py presets`).
- To fit it to recordings, see the cookbook.
