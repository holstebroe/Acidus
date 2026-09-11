# Roland TB-303 Accurate Emulation Guide & Technical Specifications

This document summarizes the hardware non-linearities, exact knob scaling behaviors, component-level circuit quirks, envelope dynamics, filter topologies, and sequencer slide logic required to accurately emulate the Roland TB-303 bass synthesizer.

---

## 1. Oscillator Core (Imperfect Waveforms)

The TB-303 oscillator generates raw waveforms with specific analog imperfections and passive filtering before reaching the VCF stage.

### Sawtooth Wave
- **Direction:** Negative-going ramp ($y = -(1 - 2 \cdot \text{phase})$).
- **Rounding:** Passed through a 1-pole low-pass filter fixed at **14 kHz** to round sharp peaks and dampen ultra-high harmonics.
- **Bending / Saturation:** Subjected to mild quadratic distortion:
  $$f(x) = x - 0.05 \cdot x^2$$
  This subtly bends the ramp, adding characteristic analog warmth.

### Square / Pulse Wave
- **Duty Cycle:** Asymmetric pulse wave with a duty cycle fixed between **45% and 47%** (~46%).
- **High-Pass Phase Shift / Tilt:** Passed through a 1-pole high-pass filter fixed at **150 Hz**. This aggressively tilts the top and bottom flat portions of the pulse wave, heavily cutting sub-bass frequencies and introducing the signature mid-range "hollow knock".

---

## 2. Diode Ladder Filter (VCF) Topology

The TB-303 filter is often described as an "18dB/octave" filter, but it is physically a 4-pole diode ladder network with specific component values causing pole frequency spreading.

### Slope Behavior
- 4-pole diode ladder network with spread pole frequencies (capacitors scaled across stages, e.g., $1.0\times, 1.5\times, 3.3\times, 1.0\times$).
- The effective cutoff slope measures **~18 dB/octave** in the primary audible frequency spectrum.

### Non-linear Feedback & Saturation
- **No Clean Self-Oscillation:** Unlike Moog transistor ladders, the 303 feedback loop does NOT self-oscillate as a clean sine whistle.
- **Feedback Saturation:** Saturation function placed directly in the feedback path:
  $$\text{Feedback}(x) = \tanh(x \cdot \text{Resonance\_Gain})$$
- **Passband Compression:** At maximum resonance gain, the signal heavily compresses passband amplitude rather than producing a pure sine whistle.

### Resonance Bass Drop
- An internal high-pass filter is situated within the feedback loop.
- As the Resonance knob is turned clockwise ($0.0 \to 1.0$), the cutoff frequency of this feedback HPF dynamically scales between **150 Hz and 250 Hz**, stripping low frequencies out of the signal path and producing the iconic low-end attenuation at high resonance.

---

## 3. Knob Ranges & Parameter Interactivity

### Cutoff Pot
- Exponential mapping from **200 Hz** (fully CCW, $0.0$) to **2.5 kHz** (fully CW, $1.0$) under zero envelope modulation:
  $$\text{Base\_Cutoff} = 200 \cdot 12.5^{\text{Cutoff}}$$

### Env Mod Pot
- Scales the peak filter envelope sweep depth up to **7.5 kHz**.

### Resonance / Cutoff Interactivity (CV Bleed)
- Negative control-voltage bleed occurs between resonance and cutoff: as Resonance increases from $0.0$ to $1.0$, the baseline cutoff frequency is pulled down by up to **15%**:
  $$\text{Effective\_Cutoff} = \text{Base\_Cutoff} \cdot (1.0 - 0.15 \cdot \text{Resonance})$$

---

## 4. Envelopes & Accent Circuits

The TB-303 does not utilize standard ADSR envelope generators. It uses dedicated RC charging/discharging circuits with gated state logic.

### Sequencer Step Gate Rules
- **Normal Step (No Slide):** Gate remains HIGH for exactly **50% of the 16th-note step duration** (equivalent to a 32nd note). Gate drops LOW for the remaining 50% of the step.
- **Slid Step (Slide = True):** Gate remains HIGH for **100% of the step duration** into the next step.

### VCA Amplitude Envelope (A-D-Gated State Logic)
Operates on two distinct circuit phases:
1. **Phase 1: Gate = HIGH**
   - **Attack:** 3.0ms exponential RC curve rise to peak 1.0.
   - **Decay:** As long as Gate remains HIGH, slow exponential discharge toward 0.0 with a **4.0-second time constant**.
2. **Phase 2: Gate = LOW (Quick Drain Correction)**
   - When Note-Off occurs (Gate = LOW), the VCA decay target instantly switches to 0.0, and the time constant overrides to discharge to silence ($-60\text{ dB}$) within **15ms to 20ms** (~18ms), preventing long trailing releases.

### VCF Filter Envelope (MEG - Main Envelope Generator)
Completely decoupled from the VCA quick drain logic:
1. **Phase 1: Gate = HIGH**
   - **Attack:** Fixed exponential RC curve at **3.5ms** up to 1.0.
   - **Decay:** Exponential decay toward 0.0. Mapped exponentially to the Decay knob position from **200ms** (fully CCW) to **2.5 seconds** (fully CW).
2. **Phase 2: Gate = LOW (Uninterrupted Decay)**
   - When Note-Off occurs, the VCF envelope does NOT snap or quick-drain. It continues tracking along its exponential decay path toward 0.0 at the rate specified by the Decay knob.

### Accent Logic (Note_Accent = True)
When an accented step is triggered ($\text{velocity} \ge 0.8$):
- **VCF Decay Override:** Forces the filter decay time constant directly to its absolute minimum (**~200ms**), ignoring the physical position of the Decay knob.
- **VCF Env Depth:** Forces Env Mod depth to **100%** for that note trigger, producing a sharp frequency "chirp".
- **VCA Saturation Boost:** Applies a **+6 dB gain boost** (+2.0x multiplier) into the VCA stage, deliberately driving asymmetric/heavy `tanh` clipping in the VCA output amplifier.

---

## 5. Sequencer Slide Logic

### Pitch Glide
- When a Slide is active between notes, target frequency passes through a 1-pole lag filter with an RC time constant tracking between **60ms and 80ms** (~70ms).

### Re-Trigger Logic (Legato vs. Staccato)
- **Slide = False (Non-Slid Note):** Retriggers both VCF and VCA envelopes to start their attack phases from their current voltage levels (smooth retrigger without resetting voltage to 0.0).
- **Slide = True (Slid Note):** Does NOT trigger the attack phase of either envelope. The VCF envelope continues its natural exponential decay uninterrupted, while the VCA envelope gate remains high.

---

## Summary Matrix

| Parameter / Module | Characteristic / Behavior | Formula / Value |
| :--- | :--- | :--- |
| **Sawtooth** | Negative-going, 14 kHz LPF, quadratic curve | $f(x) = x - 0.05x^2$ |
| **Square** | Asymmetric duty cycle, 150 Hz HPF tilt | Duty 46%, 1-pole 150 Hz HPF |
| **Diode Filter Slope** | 4-pole diode ladder network | Effective ~18 dB/octave |
| **Filter Feedback Sat** | Feedback path non-linear compression | $\text{Feedback}(x) = \tanh(x \cdot \text{ResGain})$ |
| **Resonance Bass Drop**| Dynamic feedback HPF cutoff | 150 Hz – 250 Hz based on Resonance knob |
| **Cutoff Pot Range** | Exponential mapping (zero env mod) | 200 Hz to 2.5 kHz |
| **Cutoff/Res CV Bleed** | Baseline cutoff reduction | $\text{Effective\_Cutoff} = \text{Base\_Cutoff} \cdot (1.0 - 0.15 \cdot \text{Res})$ |
| **VCF Env Attack/Decay**| Fixed RC attack, Decay knob range | Attack 3.5ms; Decay 200ms to 2.5s; Uninterrupted by Note-Off |
| **VCA Env Attack/Decay**| Fixed RC attack, Gate HIGH / LOW decay | Attack 3.0ms; Gate HIGH tau = 4.0s; Gate LOW quick drain = ~18ms |
| **Accent Behavior** | VCF Decay forced to min, 100% Env Mod, VCA boost | VCF Decay = 200ms; +6 dB VCA boost into asymmetric saturation |
| **Slide Pitch Glide** | 1-pole lag filter | RC time constant 60ms – 80ms (~70ms) |
