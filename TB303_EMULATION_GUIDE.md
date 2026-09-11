# Roland TB-303 Accurate Emulation Guide & Technical Specifications

This document details the exact hardware non-linearities, exact knob scaling behaviors, component-level circuit quirks, envelope dynamics, filter topologies, and sequencer slide logic required to accurately emulate the Roland TB-303 bass synthesizer.

---

## 1. Oscillator Core & Wave Shaper (Imperfect Waveforms)

The TB-303 does not have two independent oscillators. It features a single negative-going sawtooth integrator. The Square wave is created by sending the sawtooth through a single-transistor differential comparator wrapper.

### Sawtooth Wave
- Direction: Negative-going ramp (y = -(1 - 2 * phase)).
- Rounding: Passed through a 1-pole passive low-pass filter fixed at 14 kHz to round sharp peaks and dampen ultra-high harmonics.
- Bending / Saturation: Subjected to mild quadratic distortion due to transistor buffer loading:
  f(x) = x - 0.05 * x^2

### Square / Pulse Wave
- Generation Quirk: The square wave is derived directly from the processed sawtooth. It inherits the 14 kHz rounding and the quadratic bending on its transitions.
- Duty Cycle: Asymmetric pulse wave with a duty cycle fixed between 45% and 47% (~46%).
- High-Pass Phase Shift / Tilt: Passed through a 1-pole high-pass coupling capacitor filter fixed at 150 Hz. This aggressively tilts the top and bottom flat portions of the pulse wave. This phase distortion shifts the peak-to-peak amplitude, causing the square wave to drive the filter inputs into asymmetric distortion differently than the sawtooth.

---

## 2. Diode Ladder Filter (VCF) Topology

The TB-303 filter is physically a 4-pole diode ladder network, but its biasing configuration creates unique attenuation characteristics.

### Slope Behavior
- 4-pole diode ladder network with spread pole frequencies (capacitors scaled across stages: 10nF, 15nF, 33nF, 10nF).
- The effective cutoff slope measures ~18 dB/octave in the primary audible frequency spectrum because the poles never align perfectly on the same cutoff frequency.

### Non-linear Feedback & Saturation
- No Clean Self-Oscillation: The feedback loop will never self-oscillate as a clean sine whistle.
- Feedback Saturation: A soft-clipping function must be placed directly in the feedback path. The hardware uses a transistor pair that clips asymmetrically before saturation:
  Feedback(x) = tanh(x * Resonance_Gain)
- Passband Compression: At maximum resonance gain, the feedback network actively compresses and dampens the audio signal's fundamental passband amplitude.

### Resonance Bass Drop & Feedback HPF
- A high-pass filter (0.01uF capacitor into a 100kOhm load) sits directly inside the feedback loop path.
- As the Resonance knob is turned clockwise (0.0 to 1.0), the cutoff frequency of this feedback HPF dynamically scales between 150 Hz and 250 Hz, stripping low-frequency energy exclusively from the resonance feedback path, causing the low-end of the overall synth sound to thin out significantly.

---

## 3. Knob Ranges, Voltages & Parameter Interactivity

In the physical unit, the pots scale internal Control Voltages (CV) that bleed into each other. Standard independent parameter mappings will fail to match real knob positions.

### Cutoff Pot & Tuning Calibration
- Absolute Floor: With Cutoff, Env Mod, and Decay at 0, the absolute lowest cutoff frequency floor is ~200 Hz.
- Base Cutoff Pot Range: Exponential mapping from 200 Hz (fully CCW, 0.0) to 2.5 kHz (fully CW, 1.0):
  Base_Cutoff = 200 * 12.5^(Cutoff_Knob_Value)

### Env Mod Pot & Cross-Modulation Interaction
- The Ceiling: Scales the peak filter envelope sweep depth. At maximum (1.0), the envelope sweeps the cutoff up to 7.5 kHz.
- The Interaction Quirk: The Env Mod knob actively offsets the baseline cutoff. Turning the Env Mod knob clockwise pushes the static baseline cutoff frequency up even if the envelope is sitting at 0. 
  Cutoff_Offset = Env_Mod_Knob_Value * 350 Hz

### Resonance / Cutoff Interactivity (CV Bleed)
- Negative control-voltage bleed occurs directly inside the VCF biasing circuit. As Resonance increases from 0.0 to 1.0, the baseline cutoff frequency is pulled down by up to 15%:
  Effective_Cutoff = (Base_Cutoff + Cutoff_Offset) * (1.0 - (0.15 * Resonance))

---

## 4. Envelopes & Accent Circuits

The TB-303 uses discrete resistor-capacitor (RC) discharging circuits with gated state logic.

### Sequencer Step Gate Rules
- Normal Step (No Slide): Gate remains HIGH for exactly 50% of the 16th-note step duration. Gate drops LOW for the remaining 50% of the step.
- Slid Step (Slide = True): Gate remains HIGH for 100% of the step duration, tying seamlessly into the next step.

### VCA Amplitude Envelope (A-D-Gated State Logic)
Operates on two distinct circuit phases:
1. Phase 1: Gate = HIGH
   - Attack: 3.0ms exponential RC curve rise to peak 1.0.
   - Decay: As long as Gate remains HIGH, slow exponential discharge toward 0.0 with a 4.0-second time constant (tau).
2. Phase 2: Gate = LOW (Quick Drain Correction)
   - When Note-Off occurs (Gate = LOW), a transistor disconnects the power rail. The capacitor drains immediately through a parallel resistor. Override the time constant to discharge to silence (-60 dB) within 15ms to 20ms (~18ms).

### VCF Filter Envelope (MEG - Main Envelope Generator)
Completely decoupled from the VCA quick drain logic:
1. Phase 1: Gate = HIGH
   - Attack: Fixed exponential RC curve at 3.5ms up to 1.0.
   - Decay: Exponential decay toward 0.0. Mapped exponentially to the Decay knob position from 200ms (fully CCW) to 2.5 seconds (fully CW).
2. Phase 2: Gate = LOW (Uninterrupted Decay)
   - When Note-Off occurs, the VCF envelope does NOT reset or snap. It continues tracking along its exponential decay path toward 0.0 at the rate specified by the Decay knob, ignoring the note-off event.

### Accent Logic & Energy Accumulation
When an accented step is triggered (velocity >= 0.8):
- VCF Decay Override: Forces the filter decay time constant directly to its absolute minimum (~200ms), ignoring the physical position of the Decay knob.
- VCF Env Depth: Forces Env Mod depth to 100% for that note trigger, producing a sharp frequency "chirp".
- VCA Saturation Boost: Applies a +6 dB gain boost into the VCA stage, driving heavy asymmetric tanh clipping in the output stage.
- The Capacitor Accumulation Quirk: If multiple Accents are triggered in rapid succession, the Accent capacitor cannot discharge fully between notes. This causes the baseline filter cutoff to temporarily drift upward by an extra 100 Hz to 300 Hz over the course of 3 to 4 consecutive accented notes, creating a rising tension effect.

---

## 5. Sequencer Slide Logic

### Pitch Glide
- When a Slide is active between notes, target frequency passes through a 1-pole lag filter with an RC time constant tracking between 60ms and 80ms (~70ms).

### Re-Trigger Logic (Legato vs. Staccato)
- Slide = False (Non-Slid Note): Retriggers both VCF and VCA envelopes to start their attack phases from their current voltage levels (smooth retrigger catching the existing tail, rather than zeroing out).
- Slide = True (Slid Note): Does NOT trigger the attack phase of either envelope. The VCF envelope continues its natural exponential decay uninterrupted, while the VCA envelope gate remains high.

---

## Updated Summary Matrix

| Parameter / Module | Characteristic / Behavior | Formula / Value |
| :--- | :--- | :--- |
| Sawtooth | Negative-going, 14 kHz LPF, quadratic curve | f(x) = x - 0.05x^2 |
| Square | Asymmetric duty cycle, derived from Saw, 150 Hz HPF tilt | Duty 46%, 1-pole 150 Hz HPF phase tilt |
| Diode Filter Slope | 4-pole diode ladder network | Effective ~18 dB/octave due to pole spreading |
| Filter Feedback Sat | Feedback path non-linear clipping | Feedback(x) = tanh(x * ResGain) |
| Resonance Bass Drop| Dynamic feedback HPF cutoff | 150 Hz to 250 Hz based on Resonance knob |
| Cutoff Pot Range | Exponential mapping (zero env mod) | 200 Hz to 2.5 kHz |
| Env Mod Cutoff Offset| Knob position raises baseline cutoff | Cutoff_Offset = Env_Mod_Knob * 350 Hz |
| Cutoff/Res CV Bleed | Baseline cutoff reduction | Effective_Cutoff = (Base + Offset) * (1.0 - 0.15 * Res) |
| VCF Env Attack/Decay| Fixed RC attack, Decay knob range | Attack 3.5ms; Decay 200ms to 2.5s; Uninterrupted by Note-Off |
| VCA Env Attack/Decay| Fixed RC attack, Gate HIGH / LOW decay | Attack 3.0ms; Gate HIGH tau = 4.0s; Gate LOW quick drain = ~18ms |
| Accent Behavior | Decay floor override, max Env Mod, VCA boost, cap accumulation | VCF Decay = 200ms; +6 dB VCA boost; Cutoff accumulator drift |
| Slide Pitch Glide | 1-pole lag filter | RC time constant 60ms to 80ms (~70ms) |
