# TB-303 Reference Audit and Corrective Plan (2026-09-27)

This audit compares `src/core/` against the consolidated reference `docs/TB-303 Reference/TB303_REFERENCE.md` (the "ref" below). Each finding cites the ref section and its evidence tag.

Every numeric claim is checked by a new executable, `acidus_reference_test` (`src/reference_conformance_test_main.cpp`). It measures the DSP core against sourced targets: frequency response, loop-stability margin, time constants and control laws. It **does not** use the hardware samples in `test/resources`, because the knob positions for those are uncertain.

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target acidus_reference_test
./build/acidus_reference_test          # ~50 s; --fast skips the margin sweeps (B8, B9)
```

The exit code is the number of failed checks. **Current code: 11 pass, 25 fail, 1 informational.** The full table is at the end.

---

## 1. Why the squelch drifted

The squelch is a fast MEG blip driving the cutoff through an exponential converter, into a 4-pole diode ladder sitting just below its oscillation threshold. Accent then kicks both the filter and the VCA. Four things have moved away from that.

### 1.1 The resonance loop sits too far from its threshold (B4, B8, B9)

At Resonance = 1 the shipped loop needs its feedback raised ×1.24 before it oscillates. Stinchcombe's full model (§11.2 [A][D]) puts max resonance at ×1.065. As a result the resonant peak is **12 dB weaker relative to the pass band** than the circuit analysis predicts: −2.9 dB measured against +9.1 dB.

The measured margin curve across cutoff has the right *shape*, but sits uniformly ~16 % too high:

| wc | 102 Hz | 205 Hz | 410 Hz | 820 Hz | 1.6 kHz | 3.3 kHz |
| --- | --- | --- | --- | --- | --- | --- |
| Stinchcombe threshold k (k = 1 is max) | 2.42 | 1.58 | 1.22 | 1.06 | 0.98 | 0.95 |
| Shipped code | 2.66 | 1.79 | 1.42 | 1.24 | 1.16 | 1.12 |

Root cause:

- The calibrated `filterCapScale1..4` (0.28 / 0.71 / 0.34 / 0.53) move the core's critical gain from 17 to 18.3.
- `filterFeedbackGain` stayed pinned at 17.04 (commit 7d0eca8: "feedback gain lands at the self-oscillation ceiling". That ceiling was the calibrator's bound, not the loop's).
- The cutoff-dependent "headroom" term that used to compensate was calibrated down to 21 Hz, which effectively removes it.

The cap scales are also **unsourced**. The ladder equations already contain the C18 = C/2 half capacitor. With all scales at 1.0, the linear core reproduces Stinchcombe's poles −0.128 / −1.038 / −2.325 / −3.236 ωc exactly. That was verified analytically and measured by test A1 (0.31 dB worst deviation). The comment in `Filter.hpp` that calls 1.0 "coincident poles" is wrong.

**Verified fix (what-if run):**

- Cap scales back to 1.0.
- Fixed 100 Hz in-loop high-pass (inside antto's measured 70–140 Hz, §11.3 [I]).
- `resCouplingTrackHz` = 0 and the headroom term removed. The ref (§24) marks the 150→250 Hz sweep as unsourced.
- `filterFeedbackGain` = 18.5.
- `cutoffBaseHz` rescaled ×0.43 to keep the cutoff anchors.

That configuration passes B2, B3, B4 (+8.2 dB), B6, B7 and B8 (×1.057). The margin profile becomes 2.27 / 1.52 / 1.20 / 1.06 / 0.99 / 0.95, which tracks Stinchcombe within a few percent. E1 and E2 still pass.

The one remaining conflict is **at high cutoff**. Stinchcombe's own model crosses the threshold above ωc ≈ 1.4 kHz, and so does the corrected code (B1 fails at cutoffHz ≥ 3200). The ref also says a stock unit has "no clean self-oscillation" (§12 [M]). This is a decision point (§4).

### 1.2 The envelopes run about 3× too slow (D1–D7)

The squelch lives in the MEG's *speed*:

| Envelope | Shipped | Ref |
| --- | --- | --- |
| MEG decay τ | 200 ms – 2.5 s | 68–87 ms – 1.07 s. The printed 200 ms / 2.5 s are 90 % times, not τ (§14.1 [S][D]) |
| Accented-note MEG τ | 331 ms | Same as Decay min: 68–87 ms (the Decay pot is shorted) |
| MEG attack | 3.6 ms τ, 10 ms to 95 % | < 1 ms. The "3 ms attack" is busted (§14.1 [S][D][M]) |
| Decay knob law | Exponential in time: 707 ms at 12 o'clock | A-taper resistance: ≈ 168 ms at 12 o'clock ([E]) |
| VEG τ | 3.5 s | 1.2–1.5 s. "3–4 s" is T90 (§15.1 [S][D][I]) |

### 1.3 The Env Mod law is inverted and too shallow; the zap is clamped (E4–E7)

- **Bias shift has the wrong sign.** Raising Env Mod *raises* the settled cutoff by 0.81 oct. The service notes say it shifts the bias so the cutoff *drops*: Open303 gives ≈ −1.3 oct (§13.1 [S]).
- **Sweep is too shallow.** Full Env Mod sweeps 3.1 oct against 4.5–5.7 oct (§13.2 [I/M]).
- **No residual sweep.** There is no MEG sweep at Env Mod = 0, where the ref has ≈ 0.74–0.88 oct.
- **The top is clamped.** The engine clamps cutoff at 15 kHz and the filter at 18 kHz. The resonant peak at full modulation therefore tops out around 6 kHz, against 23.5–28 kHz measured and up to 36 kHz with accent (§13.3 [M]).

### 1.4 The VCA flattens everything (E8–E10) and accent memory is gated off (E11)

- **Saturated VCA gain.** `vcaGainSaturationDrive` = 6.9 maps the control through `tanh(6.9·x)/tanh(6.9)`. The VCA is therefore wide open almost regardless of VEG: a held note decays 0.0 dB between 0.2 s and 1.0 s, where the ref expects 4.6–5.9 dB.
- **Weak accent.** Accent adds only +0.8 dB; the ref says accented notes are "much louder" (§15.3 [A][M]).
- **No MEG leak on normal notes.** The ref has 0.45·MEG (Open303) or 0.76·e^(−t/58 ms) (Whittle fit) (§15.3). It was removed in 8931cd6 because accented notes stopped releasing. The ref's version is added *while the note is on*, which avoids that bug.
- **C13 memory is gated.** The accent-sweep contribution is multiplied by the current note's accent flag. A normal note straight after an accent does not start higher, which the ref says it should (§16.2 [M]).

### 1.5 Less bass reaches the ladder (C1)

`oscCouplingHz` = 120 Hz, stacked on `filterInputCouplingHz` = 60 Hz, costs a 55 Hz saw 7.2 dB of fundamental before the ladder. The ref says the saw goes straight into the VCF (§7.2 [M]). The lumped empirical value is Open303's single 44.5 Hz (§11.3 / §26 [I]).

That also changes how hard the low end drives the nonlinearity.

---

## 2. Deviations with no automated test (structural)

| # | Code | Ref | Impact |
| --- | --- | --- | --- |
| S1 | **The ladder is mirrored.** `Filter.cpp` applies `tanh(u − v1)` at the input stage, puts the ×2 (half capacitor) on stage 4 (the output end) and has no terminal `tanh(v4)`. | §10.3 [A]: the Q12 input pair saturates on `tanh(x − k·y4)`, the half capacitor sits on stage 1, and the top stage terminates in `tanh(y4)`. | **Linearly identical.** I checked this: same eigenvalues and the same numerator, and A1 matches Stinchcombe. The large-signal behaviour differs, though. In the code the first tanh sees a *difference* that `v1` tracks, so the input pair never clips on (input − feedback) the way Q12 does. That clipping is where the "growl" and the level-dependent squelch come from (§11.3 antto, §18). |
| S2 | Only one lumped high-pass sits inside the loop. | §11.1 [A]: 5 coupling sections, most inside the loop. | This is why B5 (the sub-bass hump at 8–20 Hz, +15 dB over 100 Hz at max res) fails even after the §1.1 fix: +1.5 dB against +15.4 dB. The ref calls that slow offset "part of the squelch" (§11.2 [D][M: mystran]). |
| S3 | The square has two hard PolyBLEP edges, a fixed duty curve (58.6 % at 110 Hz) and a 0.75 level. | §8 [S][M]: one soft edge (Q8 knee), PW that depends on pitch and pitch history (C11, 22 ms), ~50 % at 100–120 Hz; §9: saw/square ≈ 2:1 p-p. | C3, C4 and C5 fail. The one-edge ringing is "one of the most audible square-wave traits". |
| S4 | The saw goes through a 14 kHz LPF and `x − s·x²` (s = −0.17 after calibration). | §7.2: "not supported by any source — drop them". | C2 fails (4.5 % bend). |
| S5 | Slide is a 60 ms lag on frequency in Hz, and `noteOff()` snaps the pitch. | §6 [S][D][M]: RC on the CV in volts, τ = 22 ms. | C6 fails: 48 / 74 ms to 63 % up / down, against 22 / 22 ms. |
| S6 | The engine skips processing (oscillator, filter) while silent. | §21.1: keep processing; filter states keep settling between notes. | Minor. |
| S7 | No resonance-dependent tap from the filter to the VCA. | §12 [S trace, medium confidence]: two AC-coupled taps partly offset the ~20 dB pass-band loss. | Level balance across Resonance. |
| S8 | Out-of-loop stages are `filterPostHpHz` = 5 Hz and `filterAllpassHz` = 24.3 Hz. | §11.3 / §26 [I]: Open303's HP is 24.2 Hz and its all-pass 14.0 Hz. The calibrator swapped their roles. | Sub-audio phase. Also note the all-pass is what currently sets the output polarity (C7 passes only because it inverts the whole audio band). |
| S9 | Decimation is an 8-sample boxcar average; upsampling is linear interpolation. | §21.1: ≥4× with a proper anti-alias filter; peaks run to 25–36 kHz. | Needed once the 15 / 18 kHz clamps come off. |

Tooling notes:

- `acidus_filter_stability_test` sweeps `Filter`'s own member defaults, not the shipped `SynthParameters`. Its "no self-oscillation" result has never covered the configuration that ships.
- `docs/TB303_PARAMETER_CONFIDENCE.md` still lists several values that the new ref overturns: slide 60 ms "Confirmed", VEG plausible range 2.5–5 s, MEG attack, and so on.
- The "plausible range" comments in `SynthEngine.hpp` no longer match the defaults beside them. For example, `oscCouplingHz` is 120 against a stated 30–60, and `filterFeedbackGain` is 17.04 against 12–17.

---

## 3. Corrective plan

Each phase has a gate: the tests listed must pass before moving on. Hardware-sample calibration stays off until Phase 3 is done. When it comes back, it may only fit [I]/[E] parameters, within the ref's ranges, and must keep this suite green.

### Phase 1 — Restore the resonance loop (parameters plus deleting two hacks)

1. Set `filterCapScale1..4` = 1.0. Rescale `cutoffBaseHz` by ≈ 0.43 (797.6 → ≈ 341.5) and trim `cutoffSpanOct` slightly so E1, E2 and E3 pass.
2. Set `resCouplingHz` = 100 Hz (bounded 70–150, §11.3). Delete `resCouplingTrackHz` and the `feedbackHeadroomHz / cutoffHz` term.
3. Set `filterFeedbackGain` ≈ 18.5, so that B8 ≈ ×1.06 at the 1 kHz peak. Then apply the high-cutoff decision from §4.
4. Set `oscCouplingHz` = 44.5 Hz (Open303) or remove it, and set `filterInputCouplingHz` so there is **one** pre-ladder HP totalling ≈ 44.5 Hz.
5. Restore the out-of-loop stages to Open303's values: post-HP 24.2 Hz, all-pass 14.0 Hz, notch 7.52 Hz with BW 4.7 (§26). Make output polarity an explicit inversion rather than a side effect of the all-pass (§2.3).

**Gate:** A1, B1 (per the §4 decision), B2–B4, B6–B9, C1, E1–E3.

### Phase 2 — Envelopes, Env Mod, VCA and accent (control path)

1. **MEG.** Near-instant charge (τ ≈ 0.1 ms). Decay τ = R·C with R = 68 kΩ + VR6(θ), where VR6 follows the A-taper law R = 1 MΩ·(81^θ − 1)/80. That gives τ from 68 ms to 1.07 s. On accented steps, R = 68 kΩ. Drop `vcfAttackMs` and `accentDecaySec` as free parameters. Gate: D1–D5.
2. **VEG.** τ ≈ 1.23–1.5 s. Onset through a ≈ 2.2 ms RC. Gate: D6, D7.
3. **Env Mod law** in the octave domain with the bias shift (§13.2): `f = f_nom · 2^(envScaler(c,e)·(MEG − envOffset(c)) + accent)`, including the residual at e = 0. Gate: E4–E6.
4. **Remove the 15 / 18 kHz cutoff clamps.** Let the resonant peak reach ~25–36 kHz, and add a real decimation filter (S9). Gate: E7. Also check sample-rate invariance at 44.1, 96 and 192 kHz (ref test #17).
5. **VCA.** `I_ctl = VEG + c_meg·MEG` (while the gate is on) `+ Accent·MEG_acc` through the R120/C36/R119 network. Put the OTA tanh on the *signal* (`G(I)·tanh(v/2nVT)`), not on the control current; `vcaGainSaturationDrive` goes away or becomes a mild ceiling. Gate: E8–E10, plus a check that accented notes release (the 8931cd6 regression).
6. **Accent sweep.** Model C13 as a state variable: a diode, charge through R46 plus the Resonance-dependent part of VR4b (τ 47–97 ms), discharge through the wiper and mixing resistor (τ ≈ 0.1–0.15 s). Its wiper current reaches the summing node **on every note**, not only on accented ones. Gate: E11, E12.
7. **Slide.** One-pole RC on the pitch CV in semitones, τ = 22 ms, with no snap on note-off. Gate: C6.

### Phase 3 — Structural fidelity

1. **Ladder nonlinearity per §10.3** (S1). Input-pair `tanh(x − k·y4)`, half capacitor on stage 1, terminal `tanh(y4)`. A1 must still pass: the linear response doesn't change. Then set `ladderInputScale` by the §18 criteria: at Res 0 the saw is only mildly rounded; at Res max the ringing grows slightly above the saw amplitude before it limits. Add a drive-linearity test at input ×0.5 / 1 / 2 / 4 (ref test #16).
2. **Pulse shaper** (S3), using ref §8.3 option 2 (aciddose) or 3 (antto) at the oversampled rate, and the saw/square level ratio (§9). Gate: C3, C4, C5.
3. **Saw.** Drop the LPF and the quadratic bend (S4). Gate: C2.
4. **Optional "faithful" loop.** Stinchcombe's coupling sections inside an implicit solve (§11.3 option 1) (S2). Gate: B5. Of everything here this carries the most risk, which is why it comes last. The 2026-09 regression came from putting *one* low pole into the loop without the rest of the network.
5. **Resonance-dependent VCA taps** (S7), and keep processing while idle (S6).

### Phase 4 — Documentation and tooling

- Update `TB303_PARAMETER_CONFIDENCE.md` and the range comments in `SynthEngine.hpp` to match the new ref.
- Point `acidus_filter_stability_test` at `SynthParameters` defaults, or retire it in favour of B1/B9.
- **Calibrator.** Freeze every [S]/[A]/[D]-sourced constant. Bound the [I]/[E] ones by the ranges in the ref. Reject any fit that fails this suite.

---

## 4. Decisions needed

1. **Max resonance at high cutoff.**
   - The situation: with the loop fixed as in §1.1, Resonance = 1 goes just past threshold at cutoffHz ≥ ~3 kHz. That is what Stinchcombe's model predicts (threshold k = 0.98 at 1.6 kHz and 0.95 at 3.3 kHz). The hardware reports say "no clean self-oscillation" [M].
   - **(a)** Accept faint, nonlinearity-limited oscillation at the extreme top (model-faithful).
   - **(b)** Cap the feedback so the margin never drops below ≈ 1.02 (hardware-report-faithful). This replaces the removed headroom term with a principled one.
   - I'd lean to (b), but it changes the character at the top of the sweep, so it's your call.
2. **Phase 3.4 (full Stinchcombe network): do it, or stay with the empirical Open303 topology?** The empirical topology passes everything except B5 once Phase 1 is done.

---

## 5. What already agrees with the ref (keep)

- The ladder's linear core, which is exactly Stinchcombe's H_tb(s) with the scales at 1.0 (A1). This is also the basis for the Newton/trapezoid solver.
- The resonance skew law (Open303), and "no Q boost on accent".
- Pass-band loss with resonance (B2, B3), the low-frequency shape at Res 0 (B6), and the peak rising with resonance (B7).
- Cutoff anchors at knob centre and minimum (E1, E2).
- The accent-chain rise (E12), a free-running oscillator phase, MIDI overlap → slide, and no output-gain "bass compensation".

---

## 6. Current results (shipped defaults, 2026-09-27)

| ID | Ref | Check | Measured | Target | Result |
|---|---|---|---|---|---|
| A1 | §10.2 | Solver control: equal cap scales (1,1,1,1), couplings removed | best-fit wc = 0.822 x 2pi*cutoffHz, worst deviation from H_tb(s) = 0.31 dB | worst deviation <= 1.0 dB (poles -0.128/-1.038/-2.325/-3.236 wc, 24 dB/oct asymptote) | PASS |
| A2 | §10.2 | Shipped defaults: calibrated capScale1..4, couplings removed | best-fit wc = 0.352 x 2pi*cutoffHz, worst deviation from H_tb(s) = 0.38 dB | worst deviation <= 1.0 dB | PASS (magnitude hides it; the cap scales show up in B8/B9) |
| B1 | §12 | No self-oscillation at Resonance = 1 (shipped defaults), cutoffHz 100..12800 | stable at every tested cutoff | stable everywhere | PASS |
| B2 | §11.2 | Pass-band loss at 100 Hz, Resonance 0 -> 1 (peak matched to 1034 Hz) | 19.9 dB | 20.6 dB +/- 3 | PASS |
| B3 | §11.2 | Pass-band loss at 30 Hz, Resonance 0 -> 1 | 14.6 dB | 14.2 dB +/- 3 | PASS |
| B4 | §11.2 | Resonant peak height at Resonance 1, relative to the Resonance-0 level at 100 Hz | -2.9 dB | +9.1 dB +/- 3 | **FAIL** |
| B5 | §11.2 | Sub-bass hump at Resonance 1 (VCF output): gain(8.7 Hz) - gain(100 Hz) | +1.3 dB | +15.4 dB +/- 4 | **FAIL** |
| B6 | §11.2 | Low-frequency shape at Resonance 0: gain(30 Hz) - gain(100 Hz) | -3.3 dB | -4.8 dB +/- 3 | PASS |
| B7 | §11.2 | Peak frequency rises with Resonance: f(1.0)/f(0.5) | 1.08 | > 1.05 | PASS |
| B8 | §11.2 | Oscillation margin at the 1 kHz peak | x1.240 | x1.02 .. x1.15 | **FAIL** |
| B9 | §11.1/§12 | Oscillation margin across cutoff | x2.66 / 1.79 / 1.42 / 1.24 / 1.16 / 1.12 | x2.42 / 1.58 / 1.22 / 1.06 / 1.02 / 1.02 (+/-15 %) | **FAIL** |
| C1 | §7.2/§11.3 | Pre-filter high-pass: fundamental loss of a 55 Hz saw | 7.2 dB | <= 2.5 dB | **FAIL** |
| C2 | §7.2 | Saw ramp linearity | 4.52 % of p-p | <= 1 % | **FAIL** |
| C3 | §8.2 | Square pulse width at 110 Hz | 58.6 % | 47..53 % | **FAIL** |
| C4 | §9 | Saw / square p-p level ratio | 1.31 | 1.6..2.6 (~2) | **FAIL** |
| C5 | §8.2 | Square: ringing after the soft vs the hard edge | -0.5 dB | <= -6 dB | **FAIL** |
| C6 | §6 | Slide, 63 % time up / down | 47.9 / 74.2 ms | 22 / 22 ms | **FAIL** |
| C7 | §2.3/§7.2 | Output polarity of the saw | sharp edge falls | sharp edge falls | PASS (via the all-pass, see S8) |
| D1 | §14.1 | MEG charge time to 95 % | 10.16 ms | <= 1 ms | **FAIL** |
| D2 | §14.1 | MEG τ at Decay min | 200 ms | 68..87 ms | **FAIL** |
| D3 | §14.1 | MEG τ at Decay max | 2.50 s | ~1.07 s | **FAIL** |
| D4 | §14.1 | MEG τ at Decay 12 o'clock | 707 ms | ~168 ms [E] | INFO |
| D5 | §14.1/§16.1 | MEG τ on accented notes | 331 ms | 68..87 ms | **FAIL** |
| D6 | §15.1 | VEG τ, gate held | 3.49 s | 1.2..1.5 s | **FAIL** |
| D7 | §15.2 | VEG onset to 90 % | 6.7 ms | <= ~5 ms (accept 6) | **FAIL** |
| E1 | §13.3 | Service VCF calibration ring (Cutoff centre, Res max, key C) | 577 Hz | 400..670 Hz [S] | PASS |
| E2 | §13.3 | Cutoff knob minimum, settled | 333 Hz | 235..390 Hz | PASS |
| E3 | §13.3 | Cutoff knob maximum, settled | 3008 Hz | 1800..3000 Hz | **FAIL** (marginal) |
| E4 | §13.1/§13.2 | Residual MEG sweep at Env Mod 0 | 0.00 oct | >= 0.5 oct | **FAIL** |
| E5 | §13.1 | Env Mod bias shift (settled, Env Mod 1 vs 0) | +0.81 oct | <= -0.8 oct | **FAIL** |
| E6 | §13.2 | MEG sweep depth at Env Mod 1 | 3.14 oct | 4.5..5.7 oct | **FAIL** |
| E7 | §13.3 | Resonant peak at full Cutoff + Env Mod | ~6.2 kHz | ~25-28 kHz (>= 20) | **FAIL** |
| E8 | §15.1/§15.3 | Held-note VCA decay 0.2 s -> 1.0 s | 0.0 dB | 3.5..8 dB | **FAIL** |
| E9 | §15.3 | Fast MEG term in the VCA (normal note) | 0.97 | >= 1.35 | **FAIL** |
| E10 | §15.3 | Accent loudness (Accent max, Res min) | +0.8 dB | >= +3 dB | **FAIL** |
| E11 | §16.2 | Normal note after an accent starts higher (C13) | +0.01 oct | >= +0.1 oct | **FAIL** |
| E12 | §16.2 | Accent chain, 4th peak vs 1st | +0.16 oct | > +0.1 oct | PASS |

### Test caveats

- **B-series scope.** B2–B9 compare the `Filter` class with the out-of-loop post-HP and notch parked, because Stinchcombe's model ends at the VCF output. The peak is matched to his 1034 Hz by searching cutoffHz, so only shapes and ratios are compared, never absolute cutoff labels.
- **What A1/A2 can see.** They check the ladder at Resonance 0, where the cap scales barely change the magnitude (0.38 dB). The cap-scale damage shows up in the critical gain, which is what B8 and B9 measure.
- **Tolerances.** They are deliberately wide (±3 dB, ±15 %, TM3's ±25 %) because units vary (§20). A failure here is a structural miss, not unit-to-unit spread.
- **Test-only probes.** `SynthEngine::getLastCutoffHz()` and `Oscillator::getCurrentFreqHz()` were added as read-only hooks. They don't change the DSP.
