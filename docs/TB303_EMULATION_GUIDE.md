# Roland TB-303 Accurate Emulation Guide & Technical Specifications

This document details the hardware non-linearities, knob scaling behaviors, component-level circuit quirks, envelope dynamics, filter topology, and sequencer slide logic used to emulate the Roland TB-303 bass synthesizer.

> **2026 update.** This document was originally AI-written from general knowledge, without citations, and several of its specific numbers turned out to be unsourced or directly contradicted once checked against the Roland factory Service Notes and the two primary technical analyses (Tim Stinchcombe's filter derivation, Robin Whittle's circuit writeups) — see `TB303_RESEARCH_COMPENDIUM.md` for the full sourced write-up and `TB303_PARAMETER_CONFIDENCE.md` for how every number below maps onto the actual constants in `src/core/`. Each item below is now tagged:
>
> - **[CONFIRMED]** — verified against the factory service notes or a primary circuit analysis.
> - **[ESTIMATE]** — a plausible value with no direct citation; a reasonable calibration target, not a spec.
> - **[UNSOURCED / LIKELY INVENTED]** — no source found for this specific number; several are directly contradicted by research. Treat as a free calibration knob, tune by ear against reference recordings.

---

## 1. Oscillator Core & Wave Shaper (Imperfect Waveforms)

The TB-303 does not have two independent oscillators. **[CONFIRMED]** It features a single negative-going sawtooth integrator. The Square wave is created by sending the sawtooth through a transistor waveshaping stage. **[CONFIRMED — structure; "single-transistor differential comparator" is this document's paraphrase, not a cited circuit detail]**

### Sawtooth Wave
- Direction: Negative-going ramp (y = -(1 - 2 * phase)).
- Rounding: passed through a 1-pole passive low-pass filter fixed at 14 kHz to round sharp peaks. **[UNSOURCED / LIKELY INVENTED]** — no source gives this figure; the earlier `TB303_EMULATION_REFERENCE.md` explicitly lists "a fixed 14 kHz one-pole LPF" as something to *avoid* presenting as an authoritative hardware definition (its §93/§3.1). Some rounding from real transistor-buffer loading is plausible; the specific 14 kHz corner is not verified.
- Bending / Saturation: mild quadratic distortion, `f(x) = x - 0.05 * x^2`. **[UNSOURCED / LIKELY INVENTED]** — same status; also explicitly flagged for removal in `TB303_EMULATION_REFERENCE.md` §3.1/§93. Keep as a real-time-cheap saturation stand-in for "some oscillator nonlinearity exists," but do not treat the exponent or coefficient as measured.

### Square / Pulse Wave
- Generation: derived directly from the processed sawtooth via the transistor waveshaper; inherits whatever rounding/bending the saw stage applies.
- Duty cycle: **pitch-dependent**, not fixed — **[CONFIRMED, and this corrects the document's own earlier claim]**. Robin Whittle's analysis (independently corroborated by Andrew Olney's modular reconstruction) puts it at roughly **45%** at higher pitches, widening to roughly **70–71%** at the lowest oscillator frequencies. A fixed 45–47% duty cycle across the whole range is not accurate and should not be used.
- High-pass "tilt": the square (and, per the corrected research, the saw as well) passes through a coupling-capacitor network shared with the VCF input, whose corner sits in roughly the **80–115 Hz region and tracks pitch** — **[ESTIMATE, better-supported region]** — not a fixed, square-only 150 Hz HPF. **[The original "square-only, fixed 150 Hz" framing is now UNSOURCED / CONTRADICTED]**: research indicates the high-pass-like coloration is a property of the shared oscillator→VCF coupling network affecting *both* waveforms (a high-passed square resembles the reference saw and vice versa), not a filter bolted onto the square path alone.
- **[2026-09 UPDATE — IMPLEMENTED, ESTIMATE, corrected]** `Oscillator::processNextSample` (`src/core/Oscillator.cpp`) applies a single one-pole HPF to whichever waveform is currently selected (both saw and square pass through it). A first attempt made this corner pitch-track, swinging ±15% around ~98 Hz to land in the 80–115 Hz region above — this caused a large, audible regression, since that corner sat at or above the fundamental of ordinary TB-303 bass notes (e.g. a 65 Hz C2), so the "coupling network" was instead attenuating the played note itself. Reverted to a **fixed** corner (`couplingHz_`, default 44.5 Hz, plausible range **30–60 Hz** — exposed as the CLAP parameter "Osc Coupling Freq"), cross-checked against RobinSchmidt/Open303's equivalent stage (`highpass1.setCutoff(44.486)`), which is also fixed, not pitch-tracking. The saw's separate 14 kHz LPF/quadratic-bend stage above is unaffected. See `TB303_PARAMETER_CONFIDENCE.md` for the full rationale, the regression, and its fix.

---

## 2. Diode Ladder Filter (VCF) Topology

The TB-303 filter is physically a 4-pole diode-ladder network in which the stages are **not** isolated by unity-gain buffers, so each stage loads its neighbors. **[CONFIRMED — structural point, Stinchcombe]**

### Slope Behavior
- 4-pole diode ladder, spread pole frequencies. **[CONFIRMED — structurally 4-pole/24 dB asymptotic]**
- Specific per-stage capacitor values (e.g. "10 nF, 15 nF, 33 nF, 10 nF") — **[UNSOURCED / LIKELY INVENTED]**. No source consulted gives component-level VCF capacitor values; the factory service notes' own VCF trim target (TM3) was not legibly recoverable from the scan consulted for the research pass. Treat any specific capacitor-ratio numbers as tunable calibration parameters, not schematic values.
- The filter is commonly called "18 dB/octave" despite being physically 4-pole/24 dB, because the poles are unevenly spaced so the roll-off *behaves* closer to 18 dB/octave over much of the audible transition region. **[CONFIRMED as a real, contested point]** — genuinely disputed across sources, not settled; there's also an unconfirmed theory that "18 dB" was chosen to avoid an explicit comparison to the patented Moog ladder.

### Non-linear Feedback & Saturation
- No clean self-oscillation: the stock filter does not cleanly self-oscillate into a pure sine at max resonance the way a buffered Moog-style ladder does. **[CONFIRMED — Wikipedia's spec sheet states this explicitly]**
- A soft-clipping nonlinearity belongs *inside* the feedback path (not just at the output). **[CONFIRMED — directional]**; the specific functional form (`tanh`, its gain coefficient, symmetry) is an implementation choice, not a documented circuit equation.
- Passband compression at high resonance (the filter loses gain/bass as resonance increases). **[CONFIRMED — directional, well-corroborated by multiple sources]**

### Resonance Bass Drop & Feedback HPF
- **[ESTIMATE, corrected twice — see history below]** A feedback high-pass filter around 150 Hz (this project's original figure, +100 Hz with Resonance) sitting *inside* the resonance feedback loop. The mechanism ("resonance steals bass, except right around a boosted low-frequency coupling-pole region" — model the mechanism, treat the exact corner as tunable) is well-corroborated across sources.
- **History:** an earlier pass of this document (based on two thin secondary-source summaries) argued the real coupling-pole corner sits at ≈8–10 Hz instead, and this project implemented that as a 2-pole ~9 Hz network. That change caused a large, audible regression: it let nearly the *entire* ladder output back into the feedback loop (previously only content above ~150–250 Hz did), which — combined with this filter's existing per-stage tanh saturation — produced a much harsher, over-resonant sound at ordinary bassline cutoffs, confirmed by direct comparison against reference recordings. **[2026-09 UPDATE — REVERTED]** `Filter::processFaithfulSample` (`src/core/Filter.cpp`) is back to a single one-pole HPF anchored at ~150 Hz (`resCouplingHz_` in `Filter.hpp`, plausible range **100–250 Hz**), cross-checked against RobinSchmidt/Open303 (`github.com/RobinSchmidt/Open303`), a well-regarded, independently ear/measurement-tuned open-source TB-303 emulation whose equivalent stage (`TeeBeeFilter::setFeedbackHighpassCutoff(150.0)`) uses exactly this fixed 150 Hz value. `Filter::processAccurateSample` was never touched (frozen, separately-tuned mode users already like). Treat the ≈8–10 Hz secondary-source claim as not corroborated by this cross-check. See `TB303_PARAMETER_CONFIDENCE.md` for the full history and validation.

---

## 3. Knob Ranges, Voltages & Parameter Interactivity

In the physical unit, the pots scale internal control voltages that interact through analogue summing networks — front-panel positions do not map to independent DSP parameters. **[CONFIRMED as a general principle]**

### Cutoff Pot & Tuning Calibration
- Absolute floor / exponential range (e.g. "200 Hz fully CCW to 2.5 kHz fully CW", `Base_Cutoff = 200 * 12.5^knob`) — **[UNSOURCED / LIKELY INVENTED]**. No source consulted (including the factory service notes' own calibration procedure, which targets a transient/oscillation behavior rather than an absolute Hz range) confirms a specific cutoff frequency range. This remains a reasonable, commonly-used community estimate — treat it as a calibration target to verify against a reference recording, not a documented spec.
- VCO reference calibration (**not** the VCF cutoff range): **A = 110 Hz**, **two-octave interval = 4:1 ± 0.5%** — **[CONFIRMED]**, taken verbatim from the factory service notes' VCO trim procedure (TM4 reference trim, TM5 width trim).
- Pitch law: **1.000 V/octave**, calibrated to **±3 mV** tolerance, via a 6-bit pitch DAC — **[CONFIRMED]**, from the CV-Out spec (+1 V to +5 V, 1 V/octave) and the TM6 CV-calibration checkpoint.

### Env Mod Pot & Cross-Modulation Interaction
- The *mechanism* — Env Mod both scales the MEG's contribution to filter cutoff **and** shifts the filter's DC bias point, via a dedicated bias transistor (Q9) and an anti-log current converter (Q10/Q11) — is **[CONFIRMED]**, documented explicitly in the factory service notes under "VCF Envelope Modulation." Do not implement this as `cutoff_Hz += envelope * envModDepth`.
- The specific numbers ("sweeps cutoff up to 7.5 kHz at max," "+350 Hz baseline offset per knob unit") — **[UNSOURCED / LIKELY INVENTED]**. No Hz figures for Env Mod's range are given by any source consulted. Keep the *current-domain, bias-shifting* architecture; treat the Hz-equivalent magnitudes as free calibration parameters.

### Resonance / Cutoff Interactivity (CV Bleed)
- A claim that increasing Resonance pulls the baseline cutoff down by up to 15% via CV bleed — **[UNSOURCED / LIKELY INVENTED]**. This specific interaction and its 15% figure were not found in, or contradicted by, any source consulted; it is not addressed in the research at all. What *is* confirmed is a different, related effect: the Resonance pot's **second gang** feeds the Accent Sweep network (§4 below), not a global cutoff-CV bleed. If a resonance→cutoff coupling exists in the stock circuit, it was not identified in this research pass — treat this parameter as speculative and validate/remove it against reference recordings.

---

## 4. Envelopes & Accent Circuits

The TB-303 uses discrete RC discharging circuits with gated state logic, not ADSR blocks. **[CONFIRMED as a general principle]**

### Sequencer Step Gate Rules
- Normal step (no slide): gate HIGH for **3.5 of 6 clock pulses**, LOW for the remaining 2.5 — **[CONFIRMED]**, from the 24-ppqn / 6-pulses-per-16th-note clock structure (Whittle, independently corroborated by Olney). **This document's earlier "exactly 50%" claim was wrong** — the correct ratio is 3.5:2.5, not 3:3.
- Slid step: gate remains HIGH for the full step, tying into the next step. **[CONFIRMED]**

### VCA Amplitude Envelope (Gated State Logic)
Two circuit phases, both **[CONFIRMED]** as a mechanism:
1. Gate HIGH: fast attack, then slow exponential discharge toward 0 for as long as gate stays high.
2. Gate LOW: a quick-drain path discharges the VCA rapidly rather than continuing the slow decay.

Specific numbers:
- Attack ≈3 ms — **[ESTIMATE]**, order-of-magnitude "very fast," not independently sourced to a specific millisecond figure.
- Gate-HIGH decay time constant — **[UNSOURCED ESTIMATE]**. No primary source gives a specific stock VEG (Volume Envelope Generator) time constant; Whittle describes it only as "rather long." Treat any single value (3 s, 3.5 s, 4 s) as a calibration target, not a spec — do not present any one of them as more authoritative than another without a reference recording to check against.
- Gate-LOW quick-drain time (≈15–20 ms) — **[UNSOURCED ESTIMATE]**. Plausible order of magnitude, not found in any source consulted.

### VCF Filter Envelope (MEG — Main Envelope Generator)
Decoupled from the VCA's quick-drain logic — **[CONFIRMED]**: on gate-off, the MEG does **not** reset or snap; it continues along its existing exponential decay toward 0, ignoring note-off entirely (only the front-panel Decay setting, or the accent override below, governs its rate).
- Attack ≈3–3.5 ms — **[ESTIMATE]**, same status as the VCA attack above.
- Decay range mapped exponentially to the Decay knob, roughly 200 ms (fully CCW) to 2–2.5 s (fully CW) — **[ESTIMATE, plausible working target]**. Not independently confirmed against the stock service notes (whose VCF calibration procedure targets a transient/oscillation behavior at the trimmer, not the Decay pot's end-stops). Do **not** borrow the Devil Fish manual's 30 ms–3 s range for this — that is an explicitly *modified* range, not stock.

### Accent Logic & Energy Accumulation
When an accented step triggers, three things happen simultaneously, sourced from the MEG through a switch closed only on accented steps — **[CONFIRMED topology, Whittle]**:
- **VCF Decay Override [CONFIRMED mechanism]**: the Decay pot is bypassed; MEG runs at the same short, fixed decay it would use if Decay were fully CCW (i.e. the low end of the Decay range above — not necessarily an independently-fixed "200 ms," but whatever that low end actually is).
- **VCF Env Depth "forced to 100%"** — **[UNSOURCED framing]**. The confirmed mechanism is that the Accent Sweep circuit routes MEG into the filter through a *second*, RC-shaped path (below) in parallel with the normal Env Mod path — not literally forcing the Env Mod knob's own depth to 100%. Model it as an added current-summing contribution, not a knob override.
- **VCA gain boost "+6 dB"** — **[UNSOURCED / CONTRADICTED]**. Research confirms the accent's contribution to the VCA is a **control-current summation** — the MEG's short decay envelope added into the VCA's control current through an RC network of **47 kΩ + 0.033 µF** (a real, sourced component pair — τ ≈ 1.55 ms) — not a fixed +6 dB gain multiply. A fixed-dB step will not reproduce the softened, envelope-shaped accent loudness the real circuit produces.
- **Accent Sweep capacitor accumulation [CONFIRMED mechanism, exact topology sourced]**: MEG → diode → 47 kΩ → the anti-clockwise end of a 100 kΩ pot (this pot is the **second gang of the Resonance pot** — not an independent Accent-only element) → wiper → 100 kΩ mixing resistor → filter cutoff-current summing node, with a **1 µF** capacitor to ground at the pot's clockwise end. This capacitor does not fully discharge between closely-spaced accented notes, so consecutive accents produce a *rising* series of filter peaks. The direct-path time constant is τ ≈ 47 kΩ × 1 µF ≈ **47 ms**, further shaped by the 100 kΩ Resonance-gang resistance and diode nonlinearity (so the effective time constant varies continuously with the Resonance knob). The specific "cutoff drifts up by 100–300 Hz over 3–4 accents" figure in the original spec is **[UNSOURCED / LIKELY INVENTED]** — no Hz magnitude for this effect was found; the mechanism (rising accent peaks from persistent capacitor charge) is real, the Hz number is not.

---

## 5. Sequencer Slide Logic

### Pitch Glide
- Slide passes the target pitch through a 1-pole lag filter with RC time constant **60 ms** — **[CONFIRMED]**, explicitly documented in Whittle's Devil Fish manual as the *stock* value (before the mod extends it). The earlier "60–80 ms (~70 ms)" range in this document softened a number that is actually pinned down; use 60 ms.

### Re-Trigger Logic (Legato vs. Staccato)
- Non-slid note: produces a genuine new gate/trigger event for both envelopes. **[CONFIRMED]** Whether the envelopes literally reset to 0 or continue from their current (not-yet-fully-discharged) capacitor voltage is a real analogue-memory effect that should be preserved — **[CONFIRMED as a mechanism]**; closely-spaced retriggers should start from wherever the capacitor actually is, not from a hard-coded zero.
- Slid note: **does not** retrigger either envelope's attack phase. **[CONFIRMED]** MEG continues its natural exponential decay uninterrupted; the VCA gate remains open (no attack retrigger, no gate-off). Oscillator phase is never reset by a slide.

---

## Summary Matrix

| Parameter / Module | Characteristic / Behavior | Formula / Value | Confidence |
| :--- | :--- | :--- | :--- |
| Oscillator count | Single VCO, no detune, continuous phase | — | **CONFIRMED** |
| Square duty cycle | Pitch-dependent | ≈45% (high pitch) → ≈70–71% (low pitch) | **CONFIRMED** |
| Saw 14 kHz LPF + quadratic bend | Rounding/saturation stand-in | `x - 0.05x²` after 14 kHz LPF | **UNSOURCED** |
| Oscillator/VCF coupling "HPF" | Shared coupling network, both waveforms, pitch-tracking | ≈80–115 Hz, tracks pitch; implemented as shared 1-pole ±15%-around-98Hz (range 70–120 Hz), 2026-09 | **ESTIMATE, IMPLEMENTED** (corrects: not square-only, not fixed 150 Hz) |
| Diode filter structure | 4-pole, unbuffered, loaded stages | — | **CONFIRMED** |
| Diode filter capacitor values | Per-stage spread | e.g. 10/15/33/10 nF | **UNSOURCED** |
| "18 dB" vs 24 dB | Uneven pole spacing → apparent 18 dB behavior | — | **CONFIRMED** (contested but real) |
| Self-oscillation | Stock filter does not cleanly self-oscillate | — | **CONFIRMED** |
| Feedback low-frequency coupling pole | Resonant, boosts sub-100 Hz at high Resonance | A 2026-09 attempt at ≈8–10 Hz caused a large regression and was reverted; RobinSchmidt/Open303's equivalent stage uses a fixed 150 Hz, which this project now matches (range 100–250 Hz) | **ESTIMATE, cross-validated vs. Open303** (the ≈8–10 Hz secondary-source claim did not hold up in practice) |
| Cutoff pot range | Exponential, knob → Hz | ≈200 Hz–2.5 kHz (community estimate) | **UNSOURCED** |
| VCO reference cal. | A key, 2-octave ratio | 110 Hz, 4:1 ± 0.5% | **CONFIRMED** |
| Pitch law | 1 V/oct, 6-bit DAC | ±3 mV/oct tolerance | **CONFIRMED** |
| Env Mod mechanism | Bias-shift + anti-log current, not Hz offset | Q9 bias, Q10/Q11 anti-log pair | **CONFIRMED** |
| Env Mod Hz magnitude | Sweep depth / baseline offset | e.g. +350 Hz, up to 7.5 kHz | **UNSOURCED** |
| Resonance→cutoff CV bleed | Global cutoff reduction with Resonance | 15% | **UNSOURCED** (not addressed by any source) |
| Resonance pot | Dual-gang 50 kΩ | Gang 2 → Accent Sweep | **CONFIRMED** (factory parts list) |
| Gate timing | Non-slide 1/16 step | 3.5 of 6 clock pulses ON | **CONFIRMED** (corrects: not 50/50) |
| VCF (MEG) attack | Fast RC rise | ≈3–3.5 ms | **ESTIMATE** |
| VCF (MEG) decay range | Decay-knob-mapped | ≈200 ms–2–2.5 s | **ESTIMATE** |
| VCF gate-off behavior | Uninterrupted decay, no reset | — | **CONFIRMED** |
| VCA (VEG) attack | Fast RC rise | ≈3 ms | **ESTIMATE** |
| VCA (VEG) gate-high decay | Slow exponential | 3–4 s (no fixed figure sourced) | **UNSOURCED ESTIMATE** |
| VCA gate-off quick drain | Fast discharge on note-off | ≈15–20 ms | **UNSOURCED ESTIMATE** |
| Accent → VCA path | Control-current summation, RC-softened | 47 kΩ + 0.033 µF (τ≈1.55 ms) | **CONFIRMED** |
| Accent VCA "+6 dB" | Fixed gain step | — | **UNSOURCED / CONTRADICTED** |
| Accent → filter (Accent Sweep) | Diode + RC + dual-gang-Resonance pot + capacitor memory | 47 kΩ, 1 µF (τ≈47 ms direct path), 100 kΩ ×2 | **CONFIRMED** (exact topology) |
| Accent cutoff drift magnitude | Rising accent peaks | 100–300 Hz over 3–4 accents | **UNSOURCED** (mechanism confirmed, Hz figure not) |
| Slide time constant | Pitch lag | 60 ms (stock) | **CONFIRMED** |
| Slide envelope behavior | No retrigger, gate stays high | — | **CONFIRMED** |
