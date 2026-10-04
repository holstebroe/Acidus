# Roland TB-303 — Consolidated Emulation Reference

*Consolidated 2026-09-27. Supersedes `TB303_EMULATION_REFERENCE.md`, `TB303_EMULATION_GUIDE.md` and `TB303_RESEARCH_COMPENDIUM.md` (kept for history). Adds new material from Tim Stinchcombe's diode-ladder analyses, the full KVR "Open303" thread (1,155 posts, 2009–2025), the Open303 source code, and a fresh reading of the service-manual schematics in this folder.*

The goal is a **circuit-informed** emulation of a stock, early-production TB-303: reproduce the topology and the interactions between its parts, use nominal component values, hit the factory calibration targets, and treat everything else as a calibration parameter that is tuned against recordings.

---

## 0. How to read this document

### 0.1 Evidence tags

Every number carries a tag saying where it comes from. When two tags disagree, the higher one in this list usually wins, but read the notes.

| Tag | Meaning |
| --- | --- |
| **[S]** | Read directly from the Roland Service Notes (Feb 19 1982): schematic, parts list, calibration page. The pages in this folder are `roland.TB-303.schem-3/4/5/7/8.gif`. |
| **[A]** | Published circuit analysis: Tim Stinchcombe (filter), Robin Whittle (envelopes, accent, slide). |
| **[M]** | Measured on real hardware and reported with method: rv0's two TB-303s, Mike Janney (ABL author), Gordonjcp, antto's x0xb0x. |
| **[D]** | Calculated in this document from [S] component values, or from an [A] transfer function. |
| **[I]** | A constant from a working emulation that was fitted by ear or against recordings (Open303, antto's VB-303, ABL). It sounds right, but it is not a circuit value. |
| **[E]** | Estimate or community figure with no traceable measurement. Treat as a free calibration knob. |
| **[?]** | Disputed. Both sides are given. |

### 0.2 What changed versus the three earlier documents

The earlier files corrected each other several times. Some of those corrections turned out to be wrong. The main changes are listed below; §24 has the full table.

1. **The filter pole values −0.13, −1.04, −2.33, −3.24 are sourced.** They come from Stinchcombe's derivation for a diode ladder whose bottom capacitor is half the others. The schematic confirms this: C18 is 0.018 µF and the other three are 0.033 µF. The compendium's "no traceable derivation" note was wrong. **[A][S]**
2. **The "8–10 Hz" and "150 Hz" filter high-pass figures are not in conflict.** The ~8 Hz figure is a *resonant peak* that appears in Stinchcombe's complete 10-pole, 6-zero model when resonance is high. The 150 Hz figure is Open303's single lumped high-pass that stands in for that whole network in a simplified 4-pole filter. Putting one ~9 Hz pole into a simplified loop is neither model, which is why it caused a regression. See §11. **[A][I][D]**
3. **Decay "200 ms – 2.5 s" is printed on the schematic.** It is the time to fall to **10 %**, not a time constant. The matching RC is R136 68 kΩ + VR6 1 MΩ into C62 1 µF, giving τ ≈ 68 ms to 1.07 s. **[S][D]**
4. **Slide.** The circuit time constant is **τ = 22 ms** (100 kΩ R-2R Thévenin resistance × C35 0.22 µF, so f_c = 7.23 Hz). It reaches 90 % in about 51 ms and 95 % in about 66 ms. That is where Whittle's "60 ms" comes from. Measured on a real TB-303 and a TD-3. **[S][D][M]**
5. **VEG (VCA envelope).** The time constant is **τ ≈ 1.2–1.5 s** (R123 1.5 MΩ × C42 1 µF; Open303 fits 1.23 s). "3–4 s" is the time to fall to 10 %, not τ. **[S][D][I]**
6. **Pot assignments are confirmed by the parts list.** Cutoff and Env Mod are 50 kΩ-A. Tuning and Accent are 50 kΩ-B. Decay is 1 MΩ-A. Resonance is a dual 50 kΩ-B. The compendium gave Decay's part number to the **Tempo** pot. **[S]**
7. **The accent-sweep pot section is 50 kΩ (VR4b), not 100 kΩ as Whittle writes.** It is still the second gang of the Resonance pot. **[S]**
8. **The resonance pot also feeds the VCA.** Filter output reaches the VCA input through two AC-coupled paths, one of which appears to be tapped from the resonance-pot network. This partly offsets the large loss in pass-band level as resonance goes up. **[S, trace-level, medium confidence][M]**
9. **Gate length is disputed.** Whittle gives 3.5 of 6 clocks (58 %). Timing a real TB-303 recording gives ≈ 50.6 % of the step. See §4.3. **[?]**

---

## 1. Reference target and principles

- The target is a stock Roland TB-303 Bass Line: original analogue voice, original sequencer timing, no Devil Fish or other modifications.
- Units vary a lot. rv0 compared his two machines and a friend's six and found clearly audible differences in tuning scale, maximum cutoff, envelope time and resonance ceiling (§20). "Exactly like the original" should mean: exact topology, nominal values, factory calibration targets, and small, stable tolerances around them.
- **Model voltages, currents, capacitor states and gate states first. Derive audible parameters from them.** Envelopes, the accent capacitor, slide, oscillator phase and filter states are continuous state. They are never reset at note boundaries, and they keep evolving while the VCA is closed.
- These are **not** stock, so do not add them: a second oscillator or sub-oscillator, keyboard filter tracking, velocity-to-level, a perfect square wave, a clean sine at maximum resonance, a textbook 18 dB 3-pole filter, bass compensation, or a post-filter compressor.

---

## 2. Architecture

### 2.1 Audio path [S]

```text
6-bit R-2R DAC (IC9/IC10, R74–R90) ─► IC11b buffer ─► [slide: IC12 + C35 0.22 µF] ─► CV out / VCO
VCO: Q26 expo pair + IC11a → C33 10 nF ramp core (SCR reset Q24/Q25/Q27) → Q28 JFET buffer
   ├─► SAW (ramp ≈ 12 V → 5.5 V, falling, sharp reset)
   └─► Q8 pulse shaper → SQUARE (≈ 8 V / 5 V levels)
S1 waveform switch → VCF input coupling → 4-stage transistor "diode" ladder (Q12…Q17, Q22, Q23)
   → Q21 differential output amp → Q19 follower → C14 1 µF
   ├─► VR4a Resonance → C15 1 µF → Q18 → R94/C23 → R97 10 kΩ → back into Q12 (feedback)
   └─► VCA inputs (two AC-coupled paths: R121 220 kΩ + C21 10 nF, R122 100 kΩ + C22 10 nF)
IC15 BA662A VCA → C38 1 µF → VR8 Volume → mixer Q33/Q34 (+ MIX IN) → OUTPUT (10 kΩ)
                                                   └─► IC14 LA4140 headphone amp
```

### 2.2 Control path [S]

```text
CPU ─► GATE ─► Q36/Q37 ─► VEG (Q31/Q32, C42 1 µF) ─► VCA control (IC15 pin 1)
    └─► TRIGGER ─► MEG one-shot (Q38) ─► C62 1 µF ─► Q4/Q40 buffer ─► MEG out
    └─► ACCENT (latched by IC13 4013, clocked by the SLIDE strobe)
    └─► SLIDE (switches IC12 so C35 either follows the DAC or integrates through it)

MEG ─► VR5 Env Mod ─► Q10/Q11 antilog (bias Q9) ─► ladder current
MEG ─► [IC12 accent switch] ─► VR7 Accent ─┬─► R120/C36/R119 ─► VCA control (accent level)
                                            └─► D24 + R46 47 kΩ ─► VR4b (50 kΩ, 2nd gang of Resonance) ─► C13 1 µF
                                                  wiper ─► summing resistor ─► Q10 base (accent sweep)
VR3 Cutoff + R47 ─┐
TM3 "FREQ" trim ──┼─► summing network (R73 100 kΩ, R63 220 kΩ, R72 100 kΩ) ─► Q10/Q11 antilog
MEG / Env Mod ────┘
```

The mapping from each summing resistor to its source can't be traced reliably from the scan. Whittle says the accent sweep enters through "a 100 kΩ mixing resistor", which fits R72.

### 2.3 Polarity and coupling notes

- The class-A mixer/output stage **inverts** the signal after the VCF and VCA. This matters as soon as any nonlinearity is asymmetric. Open303 also feeds the filter with `-oscillator`. **[M: aciddose][I]**
- Counting every coupling capacitor, there are **about seven high-pass sections between the oscillator and the output**. Several of them sit inside the resonance loop. **[A: Stinchcombe 5 sections around the VCF][M: aciddose]**

---

## 3. Global specifications and rails

| Item | Value | Tag |
| --- | --- | --- |
| Internal rails | +12 V, +6 V, +5.333 V (IC16 AN6562 and Q42 regulators; DC-DC converter T2 MC102C) | S |
| DAC reference | 5.333 V over 64 steps = 83.33 mV = 1/12 V per semitone | S, D |
| Tuning control | about ±700 cents (VR2 50 kΩ-B) | S |
| Tempo | quarter note = 40–300 BPM | S |
| CV Out | +1 V to +5 V, 1 V/oct. Key C = 1.000 V, high C = 4.000 V (TP5/TM6 calibration) | S |
| Gate Out | 0 V off / +12 V on | S |
| Audio out impedance | 10 kΩ. Headphones 8–30 Ω | S |
| Mix In | 100 kΩ, unity gain | S |
| Power | 4 × 1.5 V cells or 9 V adaptor, 80–120 mA | S |
| CPU | µPD650C-133, clock ≈ 2.2 µs/cycle (≈ 450 kHz) | S |
| Semiconductors | NPN 2SC945(P), PNP 2SA733(P); matched duals 2SC1583(F) and 2SC2291(F); JFET 2SK30; diodes 1S2473 (Si), 1SS133 (Si), 1S188FM (Ge) | S |

---

## 4. Sequencer, gate and control timing

### 4.1 Clock [S][A]

24 pulses per quarter note, so 6 pulses per 16th-note step. Behaviour is the same with the internal clock and with DIN sync. **[M: rv0]**

### 4.2 Control signals seen by the analogue section [M: Mike Janney][A]

The voice sees only **Pitch CV** plus three logic signals: **Gate, Accent and Slide**. An emulation that handles all eight Gate/Accent/Slide combinations will reproduce any sequencer.

- **The Slide line is also the latch strobe.** The CPU pulses SLIDE at the start of **every** note (≈ 44 µs) to clock the pitch latch (IC9 4174) and the accent flip-flop (IC13 4013). SLIDE then stays high for slid notes. Between two consecutive slides it drops low for ≈ 9 µs. **[M: sonic-potions "303 timing" paper, via mystran]**
  - Consequence: each new note briefly puts the slide capacitor into "integrate" mode. With a 22 ms slide τ, a 44 µs strobe moves the pitch by only ≈ 0.2 %. Gordonjcp measured on a TB-303 and a TD-3 that the glide-off slew is too fast to see on a 400 MHz scope. **This glitch can be ignored.** **[D][M]**
- **Accent is latched.** It stays asserted through following tied or slid steps until the latch is clocked again. **[M: Mike Janney]**

### 4.3 Gate length [?]

| Source | Normal-step gate |
| --- | --- |
| Whittle (from the clock logic) | Gate rises at pulse 0 and falls halfway through pulse 3: **3.5 / 6 = 58.3 %** |
| antto, timed on rv0's TB-303 at 40 BPM | 8390 / 16568 samples = **50.6 %** (excluding the VCA release). He called the 7/12 figure a myth. |
| antto, earlier estimate | ≈ 52.5 % |
| Open303 | 50 % (`stepLength = 0.5`) |

**Recommendation:** make gate length a calibration parameter, defaulting to **0.5 step** plus the VCA release (§15). Keep 7/12 as an option, and check it against a DC-coupled recording at a slow tempo.

A slid step keeps the gate high through the whole step and into the next one: 1.5 steps for one slide, 2.5 for two. **[M: antto][A]**

### 4.4 Slide semantics [A][M]

- The slide flag sits on note A, but the glide happens at the **start of note B**. The gate stays high across the boundary, the CV steps to B, the slide RC integrates towards it, the envelopes are **not** retriggered, and the oscillator phase is untouched.
- It needs no look-ahead: when the slide flag is set, the sequencer simply doesn't drop the gate.
- A slide between two equal pitches is stored as a tie. Ties extend the gate with no new pitch and cannot carry an accent. A slide on the last step wraps round to the first. **[M: Mike Janney, rv0]** The only audible difference between the two is the accent latch; see §4.6, which also flags how far "stored as a tie" is confirmed.
- An accent on a step reached by a slide is allowed. It switches the MEG to the short decay **mid-envelope**, with no retrigger. How much "wow" you get depends on how much MEG charge is left. **[M: antto]**

### 4.5 Pattern programming (for a faithful sequencer) [M: rv0, Mike Janney]

- Pitch Mode stores a list of notes, each with Up, Down, Accent and Slide (UDAS). Time Mode stores a list of steps: `G` (new note, which takes the next pitch), `O` (tie), `-` (rest). Each `G` consumes the next pitch from the list.
- A pattern can't start with `O`, and `O` must follow `G` or `O`. The 303 compacts redundant equal-pitch slides into ties (§4.6).
- The sequencer covers 3 octaves (low C with Down = C1 up to high C with Up = C4), with transpose. **4 octaves via track Key Shift.** Maximum note ≈ 5.33 V (≈ 659 Hz before Tune). **[S][M]**

### 4.6 Equal-pitch slide vs tie: the accent latch [S][M][D][?]

*Analysis 2026-10-03, for Burette/Acidus.* A tie (`O`) and a slide into a new note (`G`) of the **same pitch** look identical to most of the voice, but not to the accent latch.

**What is the same.** In both cases the gate stays high across the step boundary (§4.3, §4.4), the pitch CV does not change, so the slide RC has nothing to integrate, and neither the MEG nor the VEG is retriggered (§14.2, §15.1). The SLIDE strobe of a new note briefly puts the slide capacitor into "integrate" mode (§4.2), but with no CV step that does nothing, and the glitch is negligible anyway.

**What differs: the accent latch.**

- The ACCENT line is a flip-flop, **IC13 (4013), clocked by the SLIDE strobe** that the CPU sends at the start of **every new note**, slid notes included. **[S: §16.1; M: sonic-potions "303 timing" paper, via mystran, §4.2]**
- A **tie** sends no strobe, so the latch keeps whatever the tied-from note set: an accent stays asserted through ties. **[M: Mike Janney, §4.2]**
- A **slid-to note** clocks the latch with its own accent bit. An accent on a step reached by a slide is allowed, switches the MEG to the short decay **mid-envelope with no retrigger**, and gives as much "wow" as the MEG has charge left. **[M: antto, §4.4]**
- The accent switch (IC12) does three things at once (§16.1): it shorts the Decay pot (MEG τ drops to ≈ 68–87 ms), it routes the MEG through VR7 to the VCA, and it feeds the MEG to the accent sweep network (§16.2). With the latch clocked under a held gate, all three switch **from the MEG's current, decaying level**. A fresh accented note starts from a full MEG.
- The VCA accent tail follows the ACCENT line (§15.2): for tied accented notes it ends in the middle of the last step. For accent-then-slide-to-normal it lasts to the end of the step. **[M: Mike Janney, antto]**

**The six combinations.** A is the first note, B the next step at the same pitch; "slide" is A's slide flag (§4.4: the flag sits on the slid-from note).

| # | A | B | 303 behaviour | Evidence |
| --- | --- | --- | --- | --- |
| 1 | normal | tie | Gate held, envelopes keep decaying, no accent | [M] |
| 2 | accent | tie | Gate held, accent stays latched; VCA accent tail ends mid last step | [M: Janney, antto] |
| 3 | normal + slide | normal | Latch re-clocked to "no accent": no change. Same as 1 | [D] from §4.2/§16.1 |
| 4 | normal + slide | accent | Latch set: MEG decay switches to the accent τ, VCA accent and accent sweep switch in from the decaying MEG; no retrigger. Small, soft "wow", bigger with more MEG left (long Decay, short gap) and with the Accent knob up | [M: antto] for a slid-to accent (different pitch); [D] for equal pitch |
| 5 | accent + slide | normal | Latch cleared: MEG decay back to the Decay pot, VCA accent and sweep removed mid-note | [D] from §4.2/§16.1; consistent with the "accent-then-slide-to-normal" tail [M] |
| 6 | accent + slide | accent | Latch set again: no change. Same as 2 | [D] |

**Open question [?].** §4.4/§4.5 report that the 303 stores an equal-pitch slide as a tie ("compacts redundant equal-pitch slides into ties", Mike Janney, rv0). If the firmware does that at programming time, a stock 303 plays **cases 4 and 5 as ties** and drops the slid-to note's accent bit (case 4) or keeps the latched accent (case 5). Cases 1, 2, 3 and 6 sound the same either way. No source in this folder describes hearing case 4 or 5 on a stock unit. To settle it: program case 4 (normal slide into an accented note of the same pitch) and case 1 on a 303, with Decay up and Accent up, and compare the VCA level and the cutoff after the boundary, or probe the ACCENT line (IC13 Q) for a rising edge at the boundary.

**What Acidus and Burette do.** Burette keeps the distinction the user programmed: a tie is a tie, and an equal-pitch slide plays as a tie **unless the accent changes** (cases 4 and 5). Then Burette sends **polyphonic pressure** on the held key, both as a CLAP pressure note expression and as MIDI poly aftertouch (`0xA0`), because hosts differ in which of the two they pass between plugins (one tested host passed MIDI poly aftertouch but dropped the expression). Its MIDI export writes the poly aftertouch. The value is 1.0 for accent on and 0 for accent off. Note-ons and note-offs stay paired, which a second note-on for a held key would break (CLAP and MIDI both expect one note-off per note-on). Acidus treats pressure on the held note as the latch: pressure ≥ 0.5 is an accent, and `Envelope::setAccent` switches the MEG decay τ and the accent VCA/sweep paths with no retrigger. Pressure on any other key, after the note-off, or as channel pressure does nothing. A synth that gets both events sees the same accent twice, which changes nothing. A host that routes neither, or the VST3 build of Burette (clap-wrapper drops events other than note-on/off that a plugin sends out), loses the change, and the step falls back to a tie. Cases 4 and 5 are therefore an extension pending the hardware check above. **[D][E]**

---

## 5. Pitch CV and tuning

### 5.1 DAC [S]

A 6-bit R-2R ladder (200 kΩ / 100 kΩ, 0.1 % matched) is driven from the 5.333 V reference through the 4050 buffers. That gives 64 levels of 1/12 V. It is buffered by IC11b (AN6562, R107 2.2 kΩ).

### 5.2 Calibration targets [S]

- **TM6:** CV key C = 1.000 V and high C = 4.000 V. The service text asks for low C to high C = +1.000 V ± 3 mV, and +2.000 V ± 3 mV over two octaves with Transpose Up.
- **TM4:** A key = **110 Hz**. The CV at the A key is **2.75 V**.
- **TM5 (width):** tap low C and high C alternately and adjust for a clean 2:1. Then two octaves must measure **4:1 ± 0.5 %**.
- Mapping implied by these: V = 1.000 V ↔ C1 = 32.70 Hz; low C key (no transpose) = 2.000 V = 65.41 Hz; A = 2.75 V = 110 Hz. So **f = 32.703 × 2^(V − 1)**, before Tune. **[D]**
- The VCF calibration key "C" drawn on the schematic has a 15 ms period (65.4 Hz), which agrees with this. **[S]**

### 5.3 Real-unit scale error [M]

Measured V/oct scale factors: ladyada's unit ≈ 0.997; rv0's unit A ≈ 1.027–1.030; antto's first measurement 0.9917; ABL's measured unit was also visibly off. Error is worse at low Tune settings. **[M: rv0]** Model this as `V_eff = V × scale + offset`, with scale around 1.00 ± 0.03, as a hidden "character" parameter.

### 5.4 VCO temperature compensation [S]

R100 is a 560 Ω **posistor** (PTC) in the expo converter. It is the only intended tempco correction. Any drift model should be slow and small.

---

## 6. Slide (portamento)

| Quantity | Value | Tag |
| --- | --- | --- |
| Capacitor | C35 = 0.22 µF | S |
| Source impedance with slide on | R-2R Thévenin resistance = **100 kΩ** whatever the bit pattern (each node always sees 2 × 200 kΩ in parallel) | D (mystran, aciddose, Gordonjcp) |
| Time constant | **τ = 22 ms** (f_c = 7.23 Hz) | D, M |
| Time to 90 % / 95 % | 50.7 ms / 66 ms. Whittle's "60 ms" is a settle time, not τ. | D |
| Measured by fitting | ≈ 7.23–7.5 Hz one-pole LPF; changing it by ±0.5 Hz is barely audible | M (antto vs rv0's 303 and his x0xb0x) |
| Slide off | C35 is driven straight from the op-amp output, so the slew is effectively instant. Use τ ≤ 0.1 ms or bypass. | S, M |

- **Apply the lag to the CV in volts (1 V/oct), before the exponential converter.** Pitch then moves linearly in semitones along an exponential approach. Open303 applies a 60 ms lag to **frequency in Hz**. That gives a different curve and a longer effective time. Treat it as an Open303 simplification, not hardware behaviour. **[I]**
- The slide time is **fixed**: it does not follow tempo. **[M: antto, confirmed with slow/fast recordings]**
- During back-to-back slides the RC keeps integrating from wherever it is. The strobe glitch (§4.2) is negligible.

```text
v_cv += (v_target − v_cv) · (1 − exp(−1 / (Fs · 0.022)))   // slide on
v_cv  =  v_target                                            // slide off
f_vco =  32.703 · 2^(v_cv·scale + tune − 1)
```

---
## 7. VCO core

### 7.1 Circuit [S]

- **Expo converter:** Q26 (2SC1583 matched pair) with IC11a (AN6562) and C34 1 nF compensation. R104 2.2 kΩ; R102 100 Ω; TM4 (tune) and TM5 (width, 4.7 kΩ) set offset and scale. R100 is a posistor (§5.4).
- **Core:** timing capacitor **C33 = 0.01 µF**, charged by the expo current. Reset is a discrete SCR (Q24/Q25/Q27 + D25) that dumps the capacitor when it crosses the threshold.
- **Buffer:** Q28, a 2SK30(O) N-JFET source follower with R105 10 kΩ. The schematic shows the ramp **between ≈ 12 V and ≈ 5.5 V: a sharp step up at reset, then a linear fall**.
- The JFET buffer softly clips the top of the ramp. The JFET's pinch-off voltage varies a lot from part to part (aciddose quotes an effective bias spread of about 1–6 V), so the clip point moves between units. This changes the square's pulse width (§8). **[M: aciddose]**

### 7.2 Behaviour

- A single oscillator that runs freely. **Its phase is never reset**, not at note-on, slide or retrigger. **[A][M]**
- The saw reaching the VCF is essentially a clean, linear ramp. antto measured it on an x0xb0x: it goes straight into the VCF with no high-pass except the VCF's own input coupling. **[M]** "The raw oscillator signal is fairly unremarkable. The odd shapes seen in recordings come from the filter's coupling networks and loading." **[M: Mike Janney, who measured it decoupled]**
- Any "14 kHz saw low-pass" or `x − 0.05x²` saw bend is **not** supported by any source. Drop them. **[E → removed]**
- **Polarity:** take the ramp as falling, from the schematic. The output stage inverts again (§2.3). Match the final polarity to recordings: in most DC-coupled recordings the saw rises from −1 to +1 and then drops. **[M: antto]**
- Top of range: 6-bit DAC max ≈ 5.33 V → ≈ 659 Hz at centre Tune, a little over 1 kHz at maximum Tune. Aliasing from the naive shaper only matters at the top of that range. **[D: aciddose]**

---

## 8. Square wave: pulse shaper Q8

### 8.1 Circuit [S]

```text
ramp (Q28 source) ──┬── R35 100 kΩ ─────────┬──► base of Q8 (PNP)
                    └── C10 10 nF ── R34 10 kΩ ┘   (a high shelf: fast edges pass through the 10 kΩ branch)
Q8 emitter ── R45 22 kΩ ── +V,   with C11 1 µF to ground   (emitter bias reservoir, τ = 22 ms)
Q8 collector ── R36 10 kΩ ── ground/ref  →  pulse out (≈ 8 V / 5 V on the schematic drawing)
```

Here is how it works, from aciddose's SPICE netlist and antto's x0xb0x probing. **[M][D]**

- While the ramp is above the emitter voltage, Q8 is **cut off**: that is the flat part of the pulse.
- As the ramp falls, Q8 turns on through a smooth **knee** and then **saturates**. That gives one **soft, frequency-dependent edge**. The other edge is **hard**, because it comes from the ramp reset.
- The small-signal gain is about 20× (aciddose). antto's fits used tanh gains of about 40–85.
- **C11 + R45 is a lossy integrator** (22 kΩ × 1 µF = 22 ms; 90 % in ≈ 50 ms). It holds the switching threshold, so the **pulse width depends on pitch and on recent pitch history**. After a pitch jump, PW and amplitude settle over roughly 50 ms, which adds a small "pop" at note onsets. **[D][M: aciddose, mystran]**
- At low pitches the triangular charge/discharge of C11 produces the small **"bump"** that antto saw in recordings near the negative peak. It moves towards the edge as pitch rises. **[M]**

### 8.2 Observed behaviour [M]

- **Pulse width depends on pitch.** Whittle and Olney give ≈ 45 % at high pitch rising to ≈ 70–71 % at the lowest notes. antto fitted a DC offset that tracks pitch: +0.385 at 17 Hz, 0.0077 at 109 Hz, −0.033 at 147 Hz. The square is **strongly asymmetric at low notes, close to symmetric around 100–120 Hz, and slightly reversed above that**. ABL measured a similar duty-versus-note curve (Mike Janney's plots). The exact curve varies between units.
- **Resonance is excited by one edge only.** With high cutoff and high resonance the filter rings after the hard edge but hardly at all after the soft edge. This follows directly from the soft knee. **It is one of the most audible square-wave traits.** A square built from a comparator, or from two BLEP saws, will not do it.
- Peak amplitudes change with pitch. On higher notes the negative peak is louder.

### 8.3 Model options (best first)

1. **Circuit level.** Integrate the netlist (ramp → R35 ∥ (C10 + R34) → PNP exponential junction with emitter RC → collector load) at ≥ 8× oversampling. aciddose's LTspice netlist (KVR, 2020) is a ready reference for generating target waveforms at each semitone.
2. **aciddose behavioural model** ([M], "fairly decent", edge slightly too sharp):
   ```text
   ramp  = 1 − 2·phase
   sat   = max(ramp − 0.6, 0)^2 ;  buf = ramp − 1.5·sat          // JFET top clip
   hp    = HP_80Hz(buf) ;          mix = 0.2·hp + buf
   src  += (shape(mix, 0.1) + 1.8 − src) · k(10 Hz)            // emitter reservoir
   pulse = max(min(shape(−85·mix + wcorr, 1.0), src), 0)
   out   = HP_10Hz(pulse)
   shape(x,c) = x / (|x| + c) · (1 + c)
   ```
3. **antto's fitted model** [I]: saw → 1-pole HP ≈ 13–15 Hz (or two in series at ≈ 9.25 Hz plus −0.055 DC) → `tanh(−G·x)` with G ≈ 41–60 → add ≈ 0.25–1× raw saw → HP ≈ 20–27 Hz → clip the negative side at a pitch-dependent level (for the bump). Using the HP instead of a fixed offset gives the history-dependent PW for free.
4. **Open303 (static)** [I]: `square = −tanh(69.98·saw + 4.37)`, shifted 180°, then band-limited as a wavetable. It has the soft/hard edge asymmetry but a **fixed** PW. That is acceptable for real-time use, but pitch-dependent PW is part of the character.

**Recommendation:** use option 1 or 2 at 4–8× oversampling for the reference model. Use a wavetable or low-order segment BLEP for the real-time model, with PW and edge width driven by a lossy integrator (~22 ms) of pitch. The soft edge width scales with 1/f.

---

## 9. Waveform levels and switching

- Saw and square reach the VCF at **different levels and DC offsets**. On the schematic the waveform sketches show the saw spanning ≈ 5.5–12 V and the square ≈ 5–8 V (roughly 2:1 in peak-to-peak), each at a different DC level. After the VCF's input coupling, the levels as heard differ further. **Keep the physical level ratio. Do not normalise either wave.** The nonlinear ladder and the one-edge excitation then produce the familiar saw/square difference by themselves. **[S][A]**
- antto set the two levels by matching a TB-303 recording of both waves with resonance at minimum and cutoff fully open. **Calibrate the same way:** same note and settings, saw vs square, compare RMS and peak before resonance is added. **[M]**
- The saw/square switch S1 is a plain mechanical switch. Switching mid-note gives an ugly transient in recordings, and that is authentic. **[M: antto]**

---
## 10. VCF core: the transistor "diode" ladder

### 10.1 Circuit [S]

- Four stages. Each stage is a pair of NPN transistors **wired as diodes** (Q13/Q15, Q14/Q17, Q16/Q23, Q22 2SC2291 at the top), with a capacitor across each rung.
- **C18 = 0.018 µF** (bottom) and **C19 = C24 = C26 = 0.033 µF**. The bottom capacitor is about half the others. This is the design Stinchcombe analyses (C₁ = C/2).
- **Q12** (2SC1583 matched pair) is the input differential pair. One base takes the audio and the other takes the resonance feedback (via R97 10 kΩ). Its tail current, from the antilog pair Q10/Q11, sets the cutoff.
- The output is taken differentially across the top capacitor. It goes through C27/C25 (0.1 µF) into the Q21 (2SC1583) differential amplifier (R113/R114 100 kΩ, Q20 tail), then the Q19 emitter follower and C14 (1 µF).
- Unlike the Moog ladder, **the stages are not buffered**. Each transistor's base connects to the next stage, so every stage loads its neighbours and the poles spread widely. The filter is still 4th order. **[A]**

### 10.2 Small-signal transfer function of the core [A: Stinchcombe]

General diode ladder, with *d* diodes at the top, bottom capacitor C₁, others C, and a = 2V_T/I_x:

```text
−d/H(s) = 16a⁴s⁴·d·C₁C³ + 8a³s³(C₁C²(1+5d) + dC³) + 4a²s²(C₁C(4+6d) + C²(1+4d)) + 2as(C₁(3+d) + C(3+3d)) + 1
```

TB-303 case (d = 1, C₁ = C/2):

```text
H_tb(s) = −1 / ( s⁴/ωc⁴ + 2^(11/4)·s³/ωc³ + 10√2·s²/ωc² + 2^(13/4)·s/ωc + 1 )
        = −1 / ( s⁴ + 6.727s³ + 14.142s² + 9.514s + 1 )        (s normalised to ωc)

ωc = 1 / (2^(3/4)·a·C) = I_x / (2^(7/4)·V_T·C)
normalised poles: −0.128, −1.038, −2.325, −3.236      (all real, widely spread)
```

Derived from the above **[D]**:

| Quantity | Value |
| --- | --- |
| −3 dB point with no feedback | **0.126 ωc**, set by the dominant pole. The other three poles sit 3.0, 4.2 and 4.7 octaves above it. |
| Feedback gain for self-oscillation | **k = 17**, at **ω = 1.189 ωc** (1 + k at the frequency where the phase reaches −180°). Moog: k = 4. Kunn and antto confirm 17 independently. |
| Pass-band gain with feedback k | 1/(1+k). At the oscillation threshold that is **−25.1 dB** (Moog −14 dB). This is the TB-303's heavy loss of level and bass as resonance goes up. |
| Per-stage asymptotic slopes (SPICE) | −6.1, −11.9, −17.9, −24.1 dB/oct; measured −23.9 dB/oct |

- **On "18 dB":** it is a 4-pole, 24 dB/oct filter. Because the poles are so far apart, the slope in the audible transition band is closer to 18 dB/oct, and it varies between 18 and 24 dB/oct with cutoff and between units (Mike Janney). Roland's "18 dB" label and the Moog-patent theory remain folklore. Do **not** build a 3-pole filter. **[A][M]**
- With the bottom capacitor halved, the response shifts up by 2^(1/4) (×1.189) compared with equal capacitors. The pole *shape* hardly changes. **[A]**

### 10.3 Large-signal (nonlinear) equations [A: mystran, from the diode-pair relation]

Each diode-connected pair splits the stage current as I₁ − I₂ = I_c·tanh(ΔV/2V_T), where ΔV is the difference between **adjacent capacitor voltages**. This is what separates it from a Moog ladder, where each stage sees only its own voltage.

```text
dV1/dt = (Ic/C1)·[ tanh(Vin'/2VT) − tanh((V1−V2)/2VT) ]        Vin' = input − feedback
dV2/dt = (Ic/C) ·[ tanh((V1−V2)/2VT) − tanh((V2−V3)/2VT) ]
dV3/dt = (Ic/C) ·[ tanh((V2−V3)/2VT) − tanh((V3−V4)/2VT) ]
dV4/dt = (Ic/C) ·[ tanh((V3−V4)/2VT) − tanh(V4/2VT) ]
```

Kunn's explicit discretisation, with a = I_c/(4C·V_T) per sample and the "2" coming from the half-size bottom capacitor:

```text
y1 += 2a·( tanh(x − k·y4) − tanh(y1 − y2) )
y2 +=  a·( tanh(y1 − y2)   − tanh(y2 − y3) )
y3 +=  a·( tanh(y2 − y3)   − tanh(y3 − y4) )
y4 +=  a·( tanh(y3 − y4)   − tanh(y4) )
```

Cheap version (linear stages, one nonlinearity): `y1 += 2a((x − k·tanh(y4)) − y1 + y2); y2 += a(y1 − 2y2 + y3); y3 += a(y2 − 2y3 + y4); y4 += a(y3 − 2y4)`.

- Tuning is not linear in `a`. Kunn: f ≈ 2a at k = 0 and √2·a at k = 17. antto's fits for 4× oversampling (as used in Open303's TB-303 mode) [I]:
  ```text
  fx = fc/Fs_os · √0.5 (per Open303: fx = ωc/(2π·√2))
  a  = (0.00045522346 + 6.1922189·fx) / (1 + 12.358354·fx + 4.4156345·fx²)
  k0 = fx·(fx·(fx·(fx·(fx·(fx+7198.6997)−5837.7917)−476.47308)+614.95611)+213.87126)+16.998792   // → 17 at DC
  g  = ((k0/17 − 1)·r + 1)·(1 + r);   k = k0·r;   out = 2·g·y4
  ```
- **Open303's TB-303 mode is linear**: no tanh anywhere in the loop. Its character comes from the network around the filter, the envelopes and the square wave. A circuit-faithful model should put the tanh terms back and set the input level so resonance can build a little above the input amplitude before it saturates (antto). Level matters: the filter must change character between 0.5× and 4× input (§23). **[I][M]**
- **Solver:** use an implicit/ZDF (TPT) or Newton solve, or at least 2nd-order Runge–Kutta (Heun), at **≥ 4× oversampling**. The highest core pole sits at more than 3× nominal cutoff, and the resonant peak reaches **25–36 kHz** at full Env Mod plus Accent (§13.3), so 88.2 kHz or more is needed internally. mystran reports Heun gives essentially exact resonance up to Fs/4 with no compensation. **[M][D]**
- Keep the transistor mismatch between stages small (§20). Note that the real nonlinearity is exponential per junction and becomes tanh only for each differential pair.

---

## 11. The complete VCF, including coupling capacitors

### 11.1 Stinchcombe's full model [A]

Stinchcombe annotated **five high-pass (coupling-capacitor) sections** around the TB-303 VCF. neotec's reading of them: section 1 acts on the input, 2 and 3 on the output and the loop, 4 on the loop only, 5 on the input and loop. Most are **inside the resonance loop**. His Laplace model, checked against a component-level SPICE simulation, is:

```text
                              1.06·s³·(s+109.9)(s+34.0)(s+7.41)
H(s) = ─────────────────────────────────────────────────────────────────────────────────────────
       L(s)·(s+97.5)(s+38.5)(s+4.45)(s+578.1)(s+20.0)(s+7.41)  +  18.7·k·s⁴·(s+46.5)(s+4.40)

L(s) = s⁴/ωc⁴ + 2^(11/4)s³/ωc³ + 10√2·s²/ωc² + 2^(13/4)s/ωc + 1,   ωc = 2π·820 in his plots,
       k = resonance 0…1 (Laplace block 4; plotted at 0, 0.2 … 0.99).  s in rad/s.
```

It has 10 poles and 6 zeros. He built a TBX-303 clone and measured on it: the lower resonant peak is real, and extra loading from his test set-up made it **oscillate at about 8 Hz**. **[A][M]**

### 11.2 What this model predicts [D] (evaluated here, ωc = 2π·820)

| k | Low-frequency peak | Main resonant peak | Level at 100 Hz | Level at 30 Hz |
| --- | --- | --- | --- | --- |
| 0 | (none; broad plateau) | (none) | −5.0 dB | −9.9 dB |
| 0.2 | 20 Hz, −12.6 dB | 432 Hz, −13.6 dB | −14.4 dB | −13.1 dB |
| 0.4 | 13 Hz, −13.0 dB | 706 Hz, −14.1 dB | −18.8 dB | −17.0 dB |
| 0.6 | 11 Hz, −12.5 dB | 854 Hz, −12.2 dB | −21.7 dB | −20.0 dB |
| 0.8 | 9.6 Hz, −11.6 dB | 957 Hz, −8.1 dB | −23.9 dB | −22.2 dB |
| 1.0 | **8.7 Hz, −10.3 dB** | **1034 Hz, +4.1 dB** | −25.7 dB | −24.1 dB |

- The loop becomes unstable at k ≈ **1.065**, at 1056 Hz. At k = 1 the main pole pair has a damping ratio of 0.014 (Q ≈ 36). This matches "very close to, but not quite, self-oscillation". **[D]**
- Pass-band level falls by **≈ 21 dB** from k = 0 to 1. The **sub-bass hump near 8–20 Hz stays about 15 dB above the 100 Hz level** at high resonance, and it resonates (ζ ≈ 0.16 at k = 1). Transients excite it, and the resulting slow offset shifts the operating point of the nonlinear ladder. That modulation is part of the "squelch". **[D][M: mystran]**
- **The resonant frequency rises with k at a fixed control current**, here from ~430 Hz at k = 0.2 to 1034 Hz at k = 1. The "cutoff" knob sets a current, not a frequency. antto noted the same on hardware: with resonance at zero the filter sits at a different frequency. **Calibrate cutoff against the resonant peak at a stated resonance setting.** **[D][M]**

### 11.3 Reconciling the models (what to implement)

1. **Circuit-faithful (reference) option.** Put the core ODEs (§10.3) and all coupling RCs (the Stinchcombe sections, with the schematic values: C15 1 µF, C23 1 µF, R97 10 kΩ, C14 1 µF, C17 1 µF, C27/C25 0.1 µF, and so on) into one implicit solve. This reproduces the 8 Hz hump, the bass loss and the peak shift together. The main pitfall: a **single** very low pole placed inside a simplified loop, without the other zeros and poles, lets almost the whole signal back through the feedback and over-drives the ladder. That is exactly the regression seen in 2026-09.
2. **Empirical (fast) option: the Open303 topology** [I], matched to hardware by ear and scope:
   ```text
   x  = −osc
   x  = HP1(x, 44.486 Hz)                         // pre-filter HP (4× rate)
   y  = ladder(x − HP_fb(k·y4, 150 Hz))           // feedback HP inside the loop
   y  = AP(y, 14.008 Hz) → HP2(y, 24.167 Hz) → Notch(y, 7.5164 Hz, BW 4.7 oct)   // outside the loop, 1× rate
   ```
   The notch and all-pass reproduce the sub-audio phase and level of the real network as it shows up in recordings. The 150 Hz feedback HP gives the "resonance fades at low cutoff" behaviour.
3. Other published empirical sets, for comparison [I]:

| Source | Input HP | Feedback HP | Output HP | Notes |
| --- | --- | --- | --- | --- |
| Open303 | 44.5 Hz | 150 Hz | 24.2 Hz + AP 14 Hz + notch 7.5 Hz | reference implementation |
| antto (VB-303), final | 17 Hz (saw) / ~15 Hz; 0.7 Hz actual DC block | ≈ 122 Hz (range 70–140) | ≈ 44 Hz | plus a residual feedback of 0.025 at Resonance = 0 (see §12) |
| antto, earlier | 25 Hz | 120 Hz | 32 Hz | fitted to a different recording chain |
| neotec (CSTBJ) | 2 × 60 Hz | 320 Hz | 30 → 210 Hz, rising with resonance | |

- Recordings made through AC-coupled interfaces add one more HP pole. rv0's recordings were **DC-coupled** (MOTU), so they show the TB's own final HP and even the accent-driven VCA level on the saw's flat top. Use DC-coupled references where possible. **[M]**
- With resonance high, the loop HP stops low cutoffs from resonating strongly. With a low saw note the resonant ringing has **almost the same amplitude at any cutoff and dies away just before the next saw edge**. Use this as a visual check. **[M: antto]**

---

## 12. Resonance

- **VR4 is a dual 50 kΩ-B (linear) pot** (parts list K162T00W-50kB × 2). **[S]**
  - **Gang A (feedback):** the filter output (Q19/C14 side) is attenuated by VR4a. The wiper goes through **C15 1 µF → Q18 buffer (R67 2.2 kΩ, R66/R99 100 kΩ, C22 1 µF) → R94 10 kΩ / C23 1 µF → R97 10 kΩ → the second base of Q12**. So feedback passes through at least two 1 µF coupling stages, which are loop high-passes. **[S]**
  - **Gang B (accent sweep):** VR4b sits between R46 47 kΩ (fed from the accent MEG through D24) and **C13 1 µF**. Its wiper feeds the cutoff summing node (§16). **[S][A]**
  - **Filter output to the VCA:** the VCA input takes the filter output through **R122 100 kΩ + C22 10 nF** and **R121 220 kΩ + C21 10 nF**. Tracing the scan suggests one comes from the top of VR4a and the other from its wiper. As resonance goes up, more signal therefore reaches the VCA, which partly offsets the ~20 dB pass-band loss. antto found this on the x0xb0x and had earlier modelled it as a "minimum resonance of 0.025". **[S trace — medium confidence][M]**
- **Law:** the pot is linear, but loop gain versus rotation is not. Open303 skews it: r = (1 − e^(−3R)) / (1 − e^(−3)). **[I]**
- **No clean self-oscillation in stock form.** Maximum resonance is close to the threshold (§11.2). Lowering R97 (10 kΩ) on an x0xb0x raises resonance past oscillation, where the nonlinearities limit it and the input "disappears" into the resonance. That is useful to validate the saturation behaviour. **[M: antto]**
- **Accent does not raise resonance.** The apparent extra resonance on accents comes from the higher cutoff (resonance is stronger at high frequencies) and the louder VCA. **[M: antto, several recordings]** Do not add Q on accent.
- **Nonlinear feedback:** high resonance with drive gives the characteristic "enveloped" ringing that bounces inside the loop, not a clean oscillation. Keep the per-stage tanh terms (§10.3). Asymmetric clipping in the loop adds even harmonics. **[M]**
- **Unit variation:** the resonance ceiling varies between units, with age, and with pot wear. One of rv0's units has a worn pot that gives a modulated "crying" sound at one setting. Expose `resonance_max_scale`. **[M]**

---

## 13. Cutoff, Env Mod and the control-current summing node

### 13.1 Circuit and Roland's description [S]

- **Q10/Q11 form an antilog (exponential) voltage-to-current converter** that sets the ladder current. **Q9** sets the bias.
- The summing network includes R73 100 kΩ (with TM3 470 kΩ "FREQ" trim), R63 220 kΩ and R72 100 kΩ. VR3 Cutoff is 50 kΩ-A (audio taper) with R47 10 kΩ. VR5 Env Mod is 50 kΩ-A with R61 10 kΩ.
- **Service Notes, "VCF Envelope Modulation"** (paraphrased): an ordinary VCF adds the envelope to a fixed base, so deeper modulation only opens the filter further and spends more time in a range where little changes audibly. In the TB-303, Q9 develops a bias that puts the starting cutoff in the middle of the range, so a small envelope voltage gives a large audible change. **Turning VR5's wiper towards terminal 3 raises the envelope voltage at Q10's base *and* shifts the bias so the cutoff drops — the same as turning Cutoff anticlockwise.** Because Q10/Q11 convert voltage to current exponentially, the cutoff stays in a useful range for most of the sweep.
- **Consequence:** Env Mod is a **bipolar, bias-shifting modulation in the exponential (octave) domain**. It is not `cutoff_Hz += env·depth`. Even at Env Mod = 0 a small residual modulation remains. **[S][M: antto, Whittle]**

### 13.2 Measured mapping (Open303, from hardware measurements) [I/M]

```text
c   = position of the Cutoff knob mapped exponentially between 313.8 Hz and 2394.4 Hz (0…1)
e   = Env Mod 0…1
envScaler = (1−c)·(3.774·e + 0.737) + c·(4.195·e + 0.864)          // octaves per unit MEG
envOffset = 0.0483·c + 0.2944
f_cut     = cutoff_nominal · 2^( envScaler·(MEG − envOffset) + accentGain·AccentSweep )
```

- At Env Mod = 1 and Cutoff at minimum: the steady-state cutoff drops about 1.3 octaves below nominal, and the MEG peak lifts it about 3.2 octaves above.
- kunn's fit to antto's measurements has the same form: `f = Fc·2^((env − o)·A)`, with o ≈ 0.27–0.32 and A ≈ 0.73–0.88 at minimum Env Mod and 5.3–5.65 at maximum.
- **Why the MEG sweep looks "logistic" and not exponential:** an exponential RC decay pushed through an antilog converter. antto's fitted shaper curves and Robin's "0.56·e + 0.44·e³" fit are approximations of `2^(A·e^(−t/τ))`. **Model the RC and the antilog. Do not fit a shaper curve.** **[D][M]**

### 13.3 Measured anchors (resonant peak frequency) [M]

| Condition | Resonant peak | Source |
| --- | --- | --- |
| Service calibration: Cutoff centre, Res max, EnvMod/Decay/Acc min, saw, key C | ringing period **2 ms ± 0.5 ms** at TP6 → **≈ 500 Hz (400–670 Hz)**; TP6 ≈ 5.2 V DC, ≈ 0.6 V ringing | S |
| Cutoff knob span, EnvMod min, Res max | ≈ **300–2400 Hz** (Open303 fit 313.8–2394 Hz) | M/I |
| Cutoff max, EnvMod min (start of note) | 3.2–3.5 kHz | M: antto |
| Cutoff max, EnvMod max (MEG peak) | **≈ 25–28 kHz** (rv0 unit A 27.5–28 kHz, unit B 23.5 kHz) | M |
| Same with Accent | up to **36.4 kHz** | M: rv0 unit A |
| Full CV range of the filter (midified control, 0–127) | ≈ 200 Hz – 25 kHz | M: ABL / Mike Janney |

Tuning of the core: ωc ∝ I_x/(V_T·C) (§10.2). The +5.333 V reference and TM3 set the absolute point. V_T is temperature dependent (∝ T), so the cutoff drifts with temperature unless the drift is compensated. **[A][D]**

---
## 14. MEG — Main (filter) Envelope Generator

### 14.1 Circuit [S]

- **Storage capacitor C62 = 1 µF.** It is charged by a one-shot (Q38, triggered through C54 0.047 µF with R151 22 kΩ and R148/R149 10 kΩ; pulse ≈ 1 ms) through **D37 + R152 100 Ω**. The charge τ is ≈ 0.1 ms, so **each new trigger recharges C62 to full in well under 1 ms**. There is no 3 ms attack: antto lists the "3 ms attack" as busted. The softer onset heard on accents comes from the accent-sweep RC (§16), not from the MEG. **[S][D][M]**
- **Buffer:** Q4/Q40 Darlington follower with Q39 (2SK30(Y) JFET) as its load.
- **Discharge:** through **R136 68 kΩ + VR6 Decay (1 MΩ, A taper)**. On accented steps the 4066 switch (IC12) **shorts VR6**, leaving R136 alone.
- The schematic prints: **"DECAY VR MAX T = 2.5 s, MIN T = 200 ms"**, drawn from 100 % to **10 %**. So T is the 90 % decay time, T = τ·ln 10.

| | Derived from components [D] | From the printed T90 [S] |
| --- | --- | --- |
| Decay min (and every accented note) | τ = 68 kΩ × 1 µF = **68 ms** (T90 = 157 ms) | T90 = 200 ms → τ ≈ **87 ms** |
| Decay max | τ = 1.068 MΩ × 1 µF = **1.07 s** (T90 = 2.46 s) | T90 = 2.5 s → τ ≈ **1.09 s** |

- Measurements agree with these. antto timed the full visible sweep (peak to rest on a spectrogram) at 0.341–0.385 s minimum and 4.3–4.6 s maximum on rv0's unit. That is about 4 τ, as expected once the antilog is applied. The Devil Fish's 30 ms–3 s is a **modified** range. **[M]**
- Open303 uses τ = 200 ms for accented notes, and its plug-in maps normal Decay to 200–2000 ms as τ. That is longer than the circuit. Treat it as [I] and prefer the values above. **[I]**
- **Pot law:** VR6 is audio (A) taper. A good model is R(θ) = R_tot·(a^θ − 1)/(a − 1) with a ≈ 81 (10 % at mid-rotation). Use the same law for all "A" pots (Cutoff, Env Mod, Volume). "B" pots (Tuning, Resonance, Accent) are linear. **[S][E for the exact curve]**

### 14.2 Behaviour

- **One-shot:** the MEG decays whether or not the gate is still high. A slide does not retrigger it; note-off does not reset it. **[A][M]**
- **Retrigger:** C62 is refilled from wherever it has decayed to. Because the charge is so fast, the peak is reached every time. The history that matters lives in the **accent capacitor C13** and in the **filter's own states**, not in the MEG peak.
- **Accent switch mid-envelope:** when an accented step follows a slide, the switch changes the decay τ immediately and the MEG is not retriggered (§4.4). **[M]**
- Model the MEG as a capacitor voltage. The cutoff shape comes from the antilog (§13.2). Its output also feeds the VCA (§15.3) and the accent network (§16).

---

## 15. VEG (volume envelope) and the BA662A VCA

### 15.1 VEG circuit [S]

- The VCA envelope generator is **Q31/Q32** with **C42 = 1 µF** and **R123 = 1.5 MΩ**. A trigger discharges C42 through Q32 (R132/R130 100 Ω). The trigger drive is smoothed by **R134 22 kΩ + C41 0.1 µF** (τ ≈ 2.2 ms). C42 then relaxes through R123. Q31 buffers it into the VCA control input (IC15 pin 1) through **R131 220 kΩ**.
- **Decay τ ≈ R123 × C42 = 1.5 s** [D]. Open303 fitted **1.23 s** [I/M]. The earlier "3–4 s" was a T90-style figure (1.5 s × ln 10 ≈ 3.45 s), not a time constant.
- The VEG is also **one-shot**: a pattern of nothing but slides fades out even though the gate stays high. **[M: rv0, antto]**

### 15.2 Onset and gate-off [M][I]

- Onset: the VCA takes **a few ms (up to ~5 ms)** to open at a new note (antto, x0xb0x and 303 recordings). This matches the 2.2 ms RC on the trigger drive. Open303 uses an instant attack plus a 200 Hz 2-pole low-pass on the amplitude envelope (≈ 1 ms smoothing).
- Gate-off: a **fast release**. Open303 uses τ = **1 ms** for normal notes and **50 ms** for accented notes (the accent "tail"). The "8 ms + 8 ms ≈ 16 ms" figure in earlier documents is unsourced; treat release τ as calibration, 1–10 ms.
- **Two accent behaviours** (Mike Janney, confirmed by antto on a 303 and his x0xb0x): (1) a smooth, discharging accent that **keeps the VCA open after the gate closes**, lasting until the ACCENT line drops; (2) a shorter, "barkier" accent with a visible jump in VCA level. The VCA accent tail follows the **ACCENT signal**: it can appear without a note, and it stops as soon as ACCENT goes low. For normal accented notes, and for accent-then-slide-to-normal, the tail lasts to the end of the step. For **tied** accented notes it lasts only to the middle of the last step. **[M]**

### 15.3 VCA control-current sum [S][I]

```text
I_vca ∝ VEG                                      (via R131 220 kΩ)
      + c_meg · MEG                               (MEG leaks into the VCA on every note)
      + Accent · MEG_acc  shaped by R120 22 kΩ / C36 0.033 µF / R119 47 kΩ   (τ ≈ 0.7–1.6 ms)
```

- Robin's fit to an unmodified 303 gave amplitude = e^(−t/1.23 s) + **0.76**·e^(−t/58 ms), where the fast term follows the Decay knob, i.e. the MEG. Open303 ships **0.45·MEG** on normal notes plus **4·Accent·MEG** on accented notes, added to the VEG while the note is on. **[I/M]**
- **Accent is not +6 dB.** It is a control-current sum shaped by the MEG's short accent decay. At maximum Accent and minimum Resonance, accented notes are much louder. **[A][M]**

### 15.4 BA662A [S]

- Roland's parts list calls it a "vari-conductance amp". It is an OTA whose transconductance is set by the control current into pin 1. The inputs (pins 2 and 3) are differential, with 2.2 kΩ networks (R124/R125/R126, C37 10 µF). It has an output buffer (pins 7 to 8), then C38 1 µF. The closest surviving datasheet is Behringer's V662A reissue.
- Model it as `out = G(I_ctl)·tanh(v_in/(2V_T·n))`, with a finite gain range, a small control feedthrough (DC thump when the control jumps — aciddose notes "DC thumps/pops") and a soft ceiling. Do not use a plain multiply for accented high-level notes.
- The filter output reaches the VCA through two AC-coupled paths (§12). With the BA662 input resistance, those 10 nF capacitors form HP corners in the tens to low hundreds of Hz. That is one physical source of the "post-filter HP" in empirical models. **[S][D]**

---

## 16. Accent

### 16.1 Logic [S][M]

- **IC13 (4013) latches ACCENT**, clocked by the SLIDE strobe. On an accented step the IC12 switch (a) **shorts the Decay pot** (MEG τ ≈ 68–87 ms), (b) routes the MEG through **VR7 Accent (50 kΩ-B)** to the VCA path and (c) to the **accent sweep** network.
- Accent is on or off; there is no velocity. With Decay at minimum and Accent at 0, accented and normal notes are identical. **[M: antto]**

### 16.2 Accent sweep network [S][A]

```text
MEG_acc ─► D24 ─► R46 47 kΩ ─► VR4b (50 kΩ, 2nd gang of Resonance) ─► C13 1 µF ─► ground
                                        │ wiper
                                        └─► mixing resistor (Whittle: 100 kΩ; likely R72) ─► Q10 summing node
```

- Whittle writes 100 kΩ for the pot. The schematic and parts list give **VR4 = 50 kΩ-B × 2**. Use 50 kΩ. **[S]**
- **Resonance low** (wiper at the R46 end): the summing node gets the MEG almost directly. The accent is a sharp filter kick.
- **Resonance high** (wiper at the C13 end): the node sees mostly the capacitor voltage, a delayed, rounded bump — the "wow". **[A]**
- **Time constants [D]:** charging through 47 kΩ + (0–50 kΩ of pot) into 1 µF gives **τ ≈ 47–97 ms**. D24 stops C13 discharging back into the MEG, so it can only discharge through the pot, wiper and mixing resistor into the summing node (≈ 100–150 kΩ, **τ ≈ 0.1–0.15 s**). At 120 BPM (125 ms per 16th) successive accents therefore start from a partly charged C13 and **each peak is higher** than the last.
- **Measured:** at Cutoff max and Env Mod 0, the normal envelope starts at ~3.5 kHz, while an accented note **rises** to ~6.8 kHz (about +1 octave) before falling. At fast tempos the notes after an accent also start higher, because C13 is still discharging. The effect depends on the **time between accents**, not on BPM as such. **[M: antto, mystran]**

### 16.3 Model options

1. **Circuit (recommended):** diode, R46, the pot as a two-resistor divider set by Resonance, C13 as a state variable, and the wiper current into the antilog summing node. Never reset C13.
2. aciddose's behavioural form: `smoothed = f·follower(MEG_acc, att, rel); cutoff_oct += lerp(MEG_acc, smoothed, resonance)`, with attack, release and f also depending on resonance. **[I]**
3. Open303: `accentGain·LeakyIntegrator(τ = 15 ms)(MEG while accented)`, added in octaves. This is simple but ignores resonance and the diode memory. Olney's reconstruction found that an **additive** mix (sweep added to the normal MEG path) sounds right; a hard switch would lengthen the audible decay. **[I]**

---

## 17. Output stage

- IC15 → C38 1 µF → **VR8 Volume (50 kΩ-A)** → mixer **Q33/Q34** (class A, inverting; MIX IN is summed here, 100 kΩ, unity) → OUTPUT (10 kΩ). A tap feeds **IC14 LA4140** for headphones. **[S]**
- The coupling capacitors add further HP poles after the VCA. The Devil Fish documentation notes that a stock unit's response around 32 Hz is well below mid-band. Open303 folds these into its post-filter chain (§11.3). **[S][I]**
- Keep external colouration (mixer drive, pedals, recording converters) as **separate, optional** stages. Many famous "303 sounds" include it.

---

## 18. Where the nonlinearities are

1. **VCO buffer and pulse shaper:** JFET top clip; PNP knee and saturation. This gives the square's soft edge and pitch-dependent PW (§7, §8).
2. **Ladder:** a tanh per diode pair, acting on the difference between **adjacent** capacitor voltages (§10.3).
3. **Input pair Q12:** a differential tanh on (input − feedback).
4. **Feedback buffer and coupling:** asymmetric headroom (Q18 follower).
5. **BA662A:** OTA input tanh, plus control feedthrough.
6. **Mixer/output:** class-A stage, asymmetric. It also inverts.

Distribute the nonlinearity; a single `tanh` at the output is wrong. Signal levels at each point matter, so calibrate **input drive** into the ladder so that: (a) with resonance at 0 the saw is only mildly rounded; (b) at maximum resonance the ringing can grow slightly above the saw amplitude before it is limited (antto). **[M]**

---

## 19. Noise, drift and temperature

- Add a very low, band-limited noise floor (transistor and resistor noise). It matters mostly for self-excited behaviour near maximum resonance and for the silence floor.
- Drift: slow and coherent. Expo converters and the filter core scale with V_T ∝ T. R100 (posistor) compensates the VCO. Model temperature as one slow state that moves the VCO scale/offset and the filter current slightly. Never use per-sample random pitch jitter. **[S][D]**

---

## 20. Unit-to-unit variation (give these hidden parameters)

| Parameter | Observed spread | Source |
| --- | --- | --- |
| V/oct scale | ≈ 0.99–1.03 (ladyada 0.997; rv0 1.027) | M |
| Max resonant peak (Cutoff and Env Mod max) | 23.5 kHz vs 27.5–28 kHz between two units; 36 kHz with Accent | M: rv0 |
| MEG decay scale | Most units ≈ rv0's. The unit on Josh Wink's "Higher State of Consciousness" needs ≈ 2.3× longer decays (possibly modified) | M: antto |
| Resonance ceiling | varies with age, revision and pot wear | M |
| Square PW curve and "bump" position | varies (JFET pinch-off, C11 tolerance) | M, S |
| Cutoff calibration | TM3 tolerance: 2 ms ± 0.5 ms ring period, i.e. ±25 % | S |
| Component tolerance | resistors ±5 %, electrolytics ±20 % (C13, C42, C62, C11 set the time constants) | E |

rv0 reported that the 303s he has heard clearly differ on A/B, and that power supply condition affects the sound (failing parts add noise and dullness). Provide a reference "nominal" preset plus a few measured unit presets, and keep the variation **static per instance**, not random per note.

---
## 21. Recommended implementation architecture

### 21.1 Rates

- Control and sequencer at the host rate (or at 1 kHz or higher for the envelopes, which is plenty for RC states).
- Oscillator, pulse shaper, VCF (and ideally the VCA input stage) at **≥ 4× oversampling**, **8×** for offline or reference rendering. The shaper's soft edge and the >27 kHz resonant peaks need it. Open303 uses 4× with an elliptic quarter-band anti-alias filter. Note: that filter is unstable in single-precision float, as reported by an ESP32 port. **[I][M]**
- **Keep processing when the VCA is closed.** The oscillator runs, the filter keeps ringing and its low-frequency states keep settling. What happens between notes is part of the sound (mystran, aciddose). Open303 resets the oscillator phase and filter states only when completely idle, as a CPU shortcut. Don't do that in a reference model. **[M]**

### 21.2 Voice state (never cleared at note boundaries)

```text
cv_slide           // volts, slide RC (τ 22 ms when on)
osc_phase / ramp   // free-running
shaper_emitter     // C11 reservoir (τ 22 ms)
vcf: V1..V4 + every coupling-capacitor voltage (C14, C15, C17, C23, C25, C27, input coupling…)
meg                // C62
veg                // C42
accent_cap         // C13
vca_ctl_smoothing  // C36, C41
output_coupling    // C38, C56 …
latched_accent, gate, slide
```

### 21.3 Per-sample order

1. Sequencer or MIDI events → Gate, Accent (latched), Slide, 6-bit pitch code.
2. DAC voltage → slide RC → tune/scale → exponential → ramp increment.
3. Ramp → JFET clip → pulse shaper (with the C11 state) → waveform switch.
4. MEG (fast charge, τ from Decay or accent short) → Env Mod bias network. MEG_acc → Accent pot → accent-sweep RC (diode, R46, VR4b(Resonance), C13).
5. Sum the control currents (Cutoff + TM3 + Env Mod·MEG with bias shift + accent-sweep wiper) → antilog → ladder current I_x.
6. Solve the VCF ODEs **implicitly**, with the coupling-capacitor sections inside the loop, or use the empirical Open303 structure (§11.3).
7. VCA input = two AC-coupled filter taps (resonance-dependent) → OTA with I_ctl = VEG + c·MEG + Accent path.
8. Output coupling → **inversion** → volume → mixer → decimate.

### 21.4 Parameters

- **Panel (0–1, through the physical pot laws):** Tuning (B), Cutoff (A), Resonance (B, dual), Env Mod (A), Decay (A), Accent (B), Volume (A), Waveform.
- **Hidden calibration:** VCO scale/offset (TM4/TM5/TM6), filter frequency (TM3), resonance ceiling, input drive, stage mismatch, MEG τ min/max, VEG τ, release τ, accent-sweep R/C, VCA MEG leak, slide τ, JFET clip level, shaper gain and C11, HP sections, temperature, unit preset.

---

## 22. MIDI mapping (plug-in convenience, not hardware behaviour)

- Note-on with no note held → new gate plus trigger. Note-on while a note is held (overlap) → **slide**: gate stays high, slide RC on, no retrigger. This is how Open303 does it.
- Accent from velocity above a threshold (Open303 uses ≥ 100). Otherwise velocity has no effect on level.
- Pitch goes through the **6-bit DAC** (quantised 1/12 V) and then the slide RC, in volts.
- Provide an internal 303-style sequencer (Time Mode/Pitch Mode, ties, accent latch) because piano-roll programming loses the "slide belongs to the previous note" feel. antto and others estimate the sequencer is a large share of the character.

---

## 23. Validation test matrix

Validate from the bottom up. Use DC-coupled, ≥ 88.2 kHz reference recordings where possible (rv0's recordings are the model to follow).

| # | Test | Pass criterion |
| --- | --- | --- |
| 1 | CV and pitch law | A key = 110 Hz; C → C = 1.000 V ± 3 mV; two octaves = 4:1 ± 0.5 %; DAC steps exactly 1/12 V |
| 2 | Raw waveforms (Cutoff max, Resonance min) at C1…C4 and Tune min to max | Saw shape and level; square PW versus pitch (asymmetric low, near 50 % around 100–120 Hz); soft vs hard edge; bump near the negative peak at low notes |
| 3 | One-edge excitation | Square, high cutoff and resonance: ringing after the hard edge only |
| 4 | Small-signal VCF | Resonance 0: core poles 0.128/1.038/2.325/3.236·ωc, 24 dB asymptote; 18–24 dB/oct apparent slope in band |
| 5 | Resonance sweep | Pass band drops by about 20–25 dB from 0 to max; resonant peak moves up with k; sub-bass hump ~8–20 Hz; no clean self-oscillation (unstable only at ~106 % of max) |
| 6 | Service VCF calibration | Cutoff centre, Res max, saw, key C: TP6-equivalent ringing period 2 ms ± 0.5 ms |
| 7 | Cutoff/Env Mod map | Peaks as in §13.3 (300–2400 Hz knob span; 3.2–3.5 kHz at Cutoff max/EnvMod 0; ~25–28 kHz at full modulation) |
| 8 | MEG timing | Visual sweep ends at ≈ 0.34–0.39 s (Decay min) and ≈ 4.3–4.6 s (Decay max); τ 68–87 ms and ≈ 1.07 s |
| 9 | VEG | Held-gate decay τ ≈ 1.2–1.5 s; all-slide pattern fades out; onset takes a few ms |
| 10 | Gate timing | Measure the gate duty (0.5 vs 0.583) against the reference at 40 BPM |
| 11 | Slide | C→G, octave up/down: single-pole approach in volts, τ 22 ms (90 % ≈ 51 ms), independent of tempo; no retrigger; continuous phase |
| 12 | Accent single | Decay shortened; louder through the control-current path; filter bump ≈ +1 octave, rounder at high Resonance |
| 13 | Accent chains | `A A A A` and 8× at Resonance 0/0.25/0.5/0.75/1, slow and fast tempo: peaks rise with shorter spacing and higher Resonance; notes after an accent start higher at fast tempo |
| 14 | Accent after slide | normal → slide → accented: decay switches mid-envelope, no retrigger, "wow" depends on remaining MEG |
| 15 | VCA accent tail | Tail lasts to the end of the step (normal) or mid last step (tie); stops when ACCENT drops |
| 16 | Drive linearity | Input ×0.5/1/2/4: character changes nonlinearly (fails if the filter is linear with an output shaper) |
| 17 | Sample-rate invariance | Same sound at 44.1, 96 and 192 kHz (antto's 44.1 vs 192 kHz test) |
| 18 | Full-mix | A/B against DC-coupled pattern recordings with matching knob positions |

---

## 24. Corrections to earlier documents (status of every disputed claim)

| Earlier claim (document) | Status now | Basis |
| --- | --- | --- |
| Normalised poles −0.13, −1.04, −2.33, −3.24 "unsourced" (Compendium §6, Reference §8 note) | **Sourced**: Stinchcombe's d = 1, C₁ = C/2 ladder; schematic C18 = 0.018 µF | A, S |
| "Six further coupling poles; composite corner 8–10 Hz" | Stinchcombe's full model has **10 poles / 6 zeros** (6 extra poles). The **~8.7 Hz is a resonant peak at high k**, not an HP corner | A, D |
| Feedback HP 150 Hz (Open303) vs 8–10 Hz | Both valid in their own models; 150 Hz is a lumped empirical substitute (§11.3) | I, D |
| Feedback HP 150→250 Hz swept with resonance (Guide) | Unsourced | — |
| Square fixed 46 % / square-only 150 Hz HPF / pitch-tracking 80–115 Hz coupling HPF | All wrong. PW comes from the Q8/C11 shaper; the saw goes straight into the VCF | S, M |
| Osc coupling HP 44.5 Hz (project default) | Keep as an empirical value (= Open303 pre-filter HP) | I |
| Saw 14 kHz LPF and `x − 0.05x²` | Remove | — |
| Ladder capacitors 10/15/33/10 nF (Guide) | **Wrong**: 33/33/33/18 nF | S |
| Cutoff 200 Hz – 2.5 kHz (Guide) | Knob span ≈ 314–2394 Hz (measured, EnvMod min, resonant peak); service anchor ≈ 500 Hz at centre | M, I, S |
| Env Mod +350 Hz, "up to 7.5 kHz" | Wrong model; peaks reach 25–36 kHz; use the octave-domain bias shift | S, M |
| Resonance → 15 % cutoff bleed | Not supported. At fixed current, the resonant peak **rises** with k (§11.2) | D |
| MEG 200 ms – 2.5 s "not confirmed" | **Printed on the schematic** as 90 % times; τ 68–87 ms … 1.07 s | S, D |
| MEG attack ≈ 3 ms | **No**: C62 recharges through 100 Ω in < 1 ms; the soft accent onset comes from C13 | S, D, M |
| VEG decay 3–4 s | That is T90; **τ ≈ 1.2–1.5 s** | S, D, I |
| Gate-off tail 16 ms (8 + 8) | Unsourced; Open303 1 ms normal / 50 ms accent; calibrate | I |
| Slide τ 60 ms (Compendium "confirmed") | **τ = 22 ms**; 60 ms ≈ the 95 % settle time; apply in volts | S, D, M |
| Gate 3.5/6 clocks (Compendium "confirmed") | **Disputed**: 50.6 % measured | ? |
| Resonance dual-gang; 2nd gang drives accent sweep | Confirmed; pot is **50 kΩ** (not 100 kΩ) | S |
| Accent sweep 47 kΩ + 1 µF + diode; VCA 47 kΩ + 0.033 µF | Confirmed (R46, C13, D24; R119, C36, plus R120 22 kΩ) | S |
| Decay pot part K161100FAE-1MC (Compendium) | Wrong — that is **Tempo**. Decay = K161B0-1MA | S |
| Pot–control assignment "uncertain" | Confirmed by the parts list | S |
| Accent +6 dB / accent adds resonance / accent +100–300 Hz | All wrong (§15, §16, §12) | M |
| Stock filter never self-oscillates | Confirmed: maximum resonance sits just below threshold | A, D, M |

---

## 25. Open questions (worth measuring on a real unit)

1. Normal gate duty: 7/12 or ≈ 0.5? Use a DC-coupled gate-out plus audio at 40 BPM.
2. Exact source of each summing resistor at Q10 (R63, R72, R73), and the exact tap points of R121/C21 and R122/C22 on VR4a.
3. VEG release circuit on gate-off (which part pulls the VCA down, and its τ).
4. The VCA control law of a real BA662A (the V662A datasheet as a proxy).
5. Level at the ladder input in volts (sets how hard the tanh works). Measure at the Q12 base with saw and square.
6. Pulse-shaper PW versus pitch, and settling after a jump, on a raw-oscillator tap (Borg-mod style).
7. The accent-sweep peak versus Resonance and spacing, measured as a control voltage at Q10's base.

---

## 26. Notes for this project's code (`src/core/`)

These refer to the implementation described in the earlier documents. The `TB303_PARAMETER_CONFIDENCE.md` file mentioned there is not in this folder.

- `Oscillator::couplingHz_` = 44.5 Hz, fixed: **keep** (= Open303 highpass1). The earlier pitch-tracking and 80–115 Hz attempts were correctly reverted.
- `Filter::resCouplingHz_` = 150 Hz inside the loop: **keep for the empirical model**. The missing parts are the **out-of-loop all-pass 14.0 Hz, HP 24.2 Hz and notch 7.52 Hz (BW 4.7 oct)**. Add them after the filter, at 1× rate.
- For a faithful mode: implement §11.3 option 1 (coupling network inside an implicit solve), not a single low HP pole in the loop.
- Slide: switch to an RC in **volts** with τ = 22 ms if it currently lags Hz with 60 ms.
- MEG τ: min ≈ 68–87 ms (also accent), max ≈ 1.07 s, A-taper pot law, instant charge.
- VEG τ ≈ 1.2–1.5 s, not 3–4 s; add the MEG leak (≈ 0.45·MEG) and the accent term.
- Accent sweep: RC state with a diode and a resonance-dependent split (§16.3 option 1), never reset.
- Square: pitch-dependent PW with ~22 ms settling (§8.3).

---

## 27. Sources

**Primary hardware documents**
- Roland TB-303 Service Notes, 1st ed., 19 Feb 1982: block diagram, main board schematic, calibration and parts list (`roland.TB-303.schem-3/4/5/7/8.gif` in this folder). Full scan: [synthfool.com PDF](https://synthfool.com/docs/Roland/TB303/Roland%20TB-303%20Service%20Notes.pdf); text: [archive.org](https://archive.org/stream/synthmanual-roland-tb-303-service-notes/rolandtb-303servicenotes_djvu.txt)

**Circuit analyses**
- Tim Stinchcombe, "Analysis of the Moog transistor ladder and derivative filters / diode ladder filters": [timstinchcombe.co.uk (diode)](https://www.timstinchcombe.co.uk/index.php?pge=diode)
- Tim Stinchcombe, "A comprehensive TB-303 diode ladder filter model" (updated Dec 2022): [timstinchcombe.co.uk (diode2)](https://www.timstinchcombe.co.uk/index.php?pge=diode2)
- Robin Whittle, "TB-303's unique sounds": [firstpr.com.au](https://www.firstpr.com.au/rwi/dfish/303-unique.html); slide timing: [303-slide.html](https://www.firstpr.com.au/rwi/dfish/303-slide.html); Devil Fish manual (values there are *modified*): [PDF](https://www.firstpr.com.au/rwi/dfish/Devil-Fish-Manual.pdf)
- A. Olney, *ct-modular-book* ch. 13: [GitHub](https://github.com/aolney/ct-modular-book/blob/master/13-tb-303.Rmd)
- Behringer V662A datasheet (BA662A reissue): [Music Tribe CDN](https://cdn.mediavalet.com/aunsw/musictribe/MG0zPwOCgECHib5rnTL-rA/LXFVv3YzekKv8pRtNdQIzg/Original/Datasheet_CA_V662A_2020-10-12_Rev.0_Signed.pdf)

**Measurements and discussion**
- KVR "Open303 – open source 303 emulation project" thread (2009–2025, 77 pages): [kvraudio.com t=262829](https://www.kvraudio.com/forum/viewtopic.php?t=262829). Key contributors cited: Robin Schmidt ("Music Engineer"), antto (VB-303, x0xb0x measurements), rv0 (DC-coupled recordings of two stock 303s), Mike Janney (Audiorealism ABL), kunn and mystran (ladder equations), Tim Stinchcombe, aciddose (VCO/shaper SPICE netlist), Gordonjcp (glide measurement on a TB-303 and TD-3), neotec.
- Sonic Potions, TB-303 sequencer timing paper (slide/accent strobe widths), cited in the thread.

**Reference implementation**
- Robin Schmidt, Open303: [GitHub](https://github.com/RobinSchmidt/Open303). Constants here were read from `Source/DSPCode/rosic_Open303.cpp/.h`, `rosic_TeeBeeFilter.h`, `rosic_MipMappedWaveTable.cpp`, `rosic_AcidPattern.cpp`.
- Forks tuned against a TD-3 and a Devil Fish (ifso, 2025) and db303 (dfl, TPT diode ladder) are mentioned in the thread's final pages.

**Superseded project documents (history):** `TB303_EMULATION_REFERENCE.md`, `TB303_EMULATION_GUIDE.md`, `TB303_RESEARCH_COMPENDIUM.md`.
