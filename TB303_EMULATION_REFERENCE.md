# Roland TB-303 Accurate Emulation Guide & Technical Specification

This document is intended as a **reference specification for a circuit-informed Roland TB-303 emulation**, with emphasis on reproducing the behavior of an original early-production TB-303 rather than merely reproducing its conventional synthesizer block diagram.

The important principle is:

> **Do not model the TB-303 as a clean oscillator → generic 18/24 dB filter → generic ADSR → VCA.**

A large part of the TB-303 sound comes from the interaction between the oscillator loading, the unusual transistor-diode filter, the filter's surrounding coupling networks, the special Env Mod bias circuit, the two envelope generators, the dual-gang Resonance control, the accent sweep capacitor, the accent/VCA coupling, and the sequencer's gate/slide timing.

The original Roland service documentation identifies the sound-generating sections as a VCO, VCF, VCA, envelope generators and mixer, and shows the actual discrete transistor/diode circuitry, including the BA662A VCA, matched transistor pairs, oscillator trim circuitry and filter ladder. The front-panel controls are not independent DSP parameters: several operate through analog current/voltage summing networks and therefore interact. The service documentation also specifies a 1 V/octave CV system, a six-clock-pulse 1/16-note timing structure, and detailed oscillator and filter calibration procedures.

---

# 0. Reference Hardware Definition

Unless otherwise stated, the target is:

- Original Roland TB-303 Bass Line.
- Early/original analog voice architecture, not Devil Fish.
- Original oscillator, filter, envelope, VCA and mixer topology.
- Original analogue sequencer timing.
- Stock component values and topology rather than later modifications.
- Nominally healthy machine, but with realistic component tolerance and transistor mismatch.
- No attempt to reproduce an individual serial-number calibration unless explicitly configured.

There are real-unit variations caused by:

- transistor matching,
- transistor beta,
- diode forward-voltage characteristics,
- resistor/capacitor tolerances,
- trim-pot settings,
- supply voltage,
- aging,
- capacitor leakage,
- temperature.

Consequently, "exactly like the original" should mean:

1. reproduce the **circuit topology and signal interactions exactly**;
2. reproduce nominal component values;
3. reproduce known calibration targets;
4. allow small component/calibration variation around those targets.

Do not invent fixed DSP curves where a circuit interaction can be modeled directly.

---

# 1. Overall Signal Architecture

A useful high-level representation is:

```text
                 DIGITAL / SEQUENCER
                       |
              6-bit pitch DAC
                       |
                 slide circuit
                       |
                       v
                 +-----------+
                 |    VCO    |
                 | saw core  |
                 +-----------+
                    |     |
                    |     +----> transistor-derived square
                    |
                    +-----------> waveform selector
                                     |
                                     v
                              waveform mixer/
                              coupling network
                                     |
                                     v
                              VCF input network
                                     |
                  +------------------+----------------+
                  |                                   |
                  |                           resonance feedback
                  v                                   ^
            4-pole transistor                    HP/coupling/
            diode ladder                         resonance network
                  |
                  v
                 VCA <--------- VEG / accent VCA control
                  |
                  v
                MIXER
                  |
                  v
              OUTPUT STAGE
````

The filter-control path is separate and considerably more complicated:

```text
Cutoff pot
    |
    +-----------------------------+
    |                             |
Envelope Mod pot                  |
    |                             |
    v                             |
MEG ----> Env Mod / bias network-+----> filter control current
    |
    +----> Accent switch
              |
              +----> Accent pot
              |
              +----> Accent Sweep RC
                         |
                         +--------> filter control current
                         |
                         +--------> accent VCA control
```

The **Accent Sweep circuit is particularly important**. It is not equivalent to simply adding a second envelope to the filter.

---

# 2. Oscillator

## 2.1 Single Oscillator

The TB-303 has one audio oscillator.

It is a saw-core oscillator whose pitch is controlled by an analogue control voltage generated by the sequencer DAC and then modified by the slide circuit.

There are not two independent oscillators and there should be no oscillator detuning, oscillator phase relationship or oscillator reset mechanism unless explicitly added as a non-stock option.

---

## 2.2 VCO Core

The oscillator is a conventional analogue saw-core architecture:

1. a control-current/pitch circuit determines charging current;
2. a timing capacitor integrates that current;
3. a threshold/reset mechanism rapidly discharges the capacitor;
4. this produces the repeating ramp;
5. the same oscillator voltage is processed into the second selectable waveform.

The important emulation consequence is:

> The oscillator should be treated as a continuously running analogue oscillator, not as a digital wavetable restarted on every note.

A note-on changes oscillator frequency through the pitch-control path. The oscillator phase does not need to be reset to zero at every sequencer step.

This matters for the precise shape of note attacks, especially during normal staccato playing.

---

## 2.3 Pitch Calibration

The Roland service procedure specifies:

* A-key oscillator frequency: **110 Hz**.
* Octave ratio: **4:1** for a two-octave interval.
* Width trim is adjusted for the required waveform relationship.

The pitch system should therefore be calibrated around a true 1 V/octave law rather than an arbitrary MIDI-to-Hz mapping.

The original sequencer DAC provides approximately:

```text
1.000 V/octave
1/12 V per semitone
```

over the useful musical range.

The service material specifies a six-bit DAC and a 1 V/octave CV range. Do not replace this with an infinitely precise floating-point MIDI pitch source if the goal is to reproduce the internal hardware path.

For MIDI-operated emulation, MIDI note numbers may of course be converted to the corresponding DAC-equivalent pitch voltage.

---

# 3. Sawtooth Waveform

## 3.1 Do Not Add an Arbitrary 14 kHz Low-Pass

The original TB-303 saw should **not** be defined as:

```text
ideal saw
    -> 14 kHz one-pole LPF
    -> quadratic distortion
```

There is no sufficient circuit evidence for those particular DSP abstractions.

In particular, avoid:

```text
f(x) = x - 0.05*x^2
```

as a fixed "TB-303 saw distortion" function.

Such a function may produce a convincing approximation in some circumstances, but it is not an adequate model of the actual oscillator.

The oscillator's characteristic shape results from the analogue saw-core oscillator, transistor stages, loading and subsequent waveform-selection/coupling circuitry.

---

## 3.2 Saw Wave Shape

The saw should be generated from a continuous analogue ramp rather than a mathematically perfect band-limited wavetable if circuit accuracy is the objective.

At nominal operation:

* the waveform is predominantly a sawtooth;
* its amplitude is determined by the oscillator circuitry;
* its shape is affected by the transistor stages and loads;
* the phase of the discontinuity is determined by the oscillator reset mechanism;
* waveform amplitude and offset should be allowed to differ from the square output.

The oscillator should therefore be modeled internally at a substantially higher rate than the final audio sample rate or by an analytic continuous-time/oversampled implementation.

---

# 4. Square Wave

## 4.1 Square Is Derived From the Oscillator Ramp

The TB-303's square waveform is derived from the oscillator ramp through a transistor-based waveform-shaping circuit.

This is important.

It is **not** equivalent to replacing the oscillator with an independent mathematically perfect ±1 square wave.

The square waveform inherits characteristics from the oscillator and the transistor waveform-forming network.

---

## 4.2 Duty Cycle Is Not a Fixed 46%

A fixed:

```text
46% duty cycle
```

is not sufficient for an accurate emulation.

The square/pulse shape changes with oscillator pitch.

Measurements and circuit analyses of original 303 oscillator behavior indicate that:

* at higher pitches the duty cycle approaches roughly **45%**;
* at very low oscillator frequencies the duty cycle can become substantially wider, around **70%**.

This means that pulse width should be modeled as an emergent consequence of the oscillator/waveshaper circuit, or at minimum as a frequency-dependent calibration curve.

Do not hard-code:

```text
duty = 0.46
```

for the entire musical range.

---

## 4.3 Square Amplitude

The square waveform is not simply another normalized oscillator output.

Its amplitude, DC offset and exact waveform shape differ from the saw.

This affects the filter significantly because the TB-303 filter is nonlinear and therefore reacts differently to the two input waveforms.

An important audible consequence is:

* the saw generally drives the filter harder;
* the square generally drives it less strongly;
* the amount of resonance/distortion therefore differs between the two waveforms even when the front-panel controls are unchanged.

This behavior must emerge from the actual oscillator output amplitude and filter input path, not from an arbitrary "square gets X dB less drive" compensation applied after the filter.

---

# 5. Oscillator Waveform Coupling and High-Pass Effects

The original oscillator and VCF are connected through a network of analogue coupling capacitors and resistive networks.

Therefore:

> Do not implement a dedicated 150 Hz high-pass filter on the square waveform.

That is too specific and generally incorrect as a representation of the circuit.

The familiar similarity between some high-passed square/saw recordings is a consequence of the surrounding analogue coupling network.

The final oscillator waveform entering the filter should be the result of:

```text
VCO
→ waveform selector
→ transistor/resistor stages
→ coupling capacitors
→ filter input network
```

not:

```text
VCO
→ arbitrary waveform shaping
→ waveform-specific 150 Hz HPF
→ ideal filter
```

The coupling capacitors should ideally be represented explicitly because they contribute frequency-dependent phase/amplitude behavior.

---

# 6. VCF Topology

## 6.1 Four-Pole Transistor/Diode Ladder

The central filter is a four-stage diode-ladder type structure implemented using transistors operated as nonlinear diode elements.

The original service schematic shows transistor devices in the ladder rather than discrete semiconductor diodes in the modern colloquial sense.

The ladder is structurally different from a Moog transistor ladder because the stages are not isolated by ideal unity-gain buffers.

Each stage loads the adjacent stages.

This loading is one of the central reasons why the TB-303 filter does not behave like a generic cascade of four identical one-pole filters.

---

# 7. Correct Filter Mathematical Model

A particularly useful idealized representation of the four main ladder nodes is:

```text
dv1/dt = ω [ S(u - v1) - S(v1 - v2) ]

dv2/dt = ω [ S(v1 - v2) - S(v2 - v3) ]

dv3/dt = ω [ S(v2 - v3) - S(v3 - v4) ]

dv4/dt = 2ω S(v3 - v4)
```

where:

* `u` is the effective ladder input;
* `v1..v4` are the four filter node voltages;
* `ω` is the frequency-dependent ladder coefficient;
* `S(x)` is the nonlinear diode/transistor current relationship.

For a simplified software implementation, `S(x)` can be approximated by a smooth odd nonlinear function such as:

```text
S(x) = tanh(k*x)
```

but a physically better implementation uses an actual diode/BJT exponential relationship.

Do not use a single tanh only at the final filter output.

The nonlinearity occurs **inside the ladder stages**.

---

# 8. Linearized Filter Transfer Function

For the idealized equal-component ladder approximation, the normalized four-pole response can be written approximately as:

```text
H(s) =
1 /
(
    s^4
    + 6.727 s^3
    + 14.142 s^2
    + 9.514 s
    + 1
)
```

This is useful as a validation reference.

The corresponding normalized poles are approximately:

```text
-0.13
-1.04
-2.33
-3.24
```

The poles are therefore widely separated rather than coincident.

This is a much better approximation to the TB-303's linearized filter than simply cascading four identical Butterworth one-poles.

---

# 9. Why the Filter Is Often Called an "18 dB/octave" Filter

There is a long-standing specification conflict around the TB-303 filter:

* four poles imply an asymptotic 24 dB/octave roll-off;
* the actual response over much of the audible transition region can behave much more like an approximately 18 dB/octave filter;
* Roland-associated descriptions commonly refer to the filter as an 18 dB filter.

The useful implementation rule is:

> Do not force the filter to be a textbook 18 dB/octave three-pole filter.

The correct target is the actual four-stage diode/transistor ladder and its surrounding networks.

The apparent 18 dB behavior should emerge from the actual pole positions and loading.

---

# 10. The Filter Is More Than Four Poles

This is a critical point for high-fidelity emulation.

The TB-303 VCF contains numerous coupling capacitors and associated resistive networks around the four main ladder stages.

These additional networks contribute additional poles.

A detailed circuit analysis identifies approximately **six further high-pass/coupling poles** associated with the surrounding filter circuitry.

Therefore a serious emulation should not reduce the whole VCF to:

```text
4-pole low-pass + resonance
```

when trying to reproduce the actual audio response.

A better abstraction is:

```text
input coupling
    ↓
input amplifier / mixer
    ↓
4-stage nonlinear diode ladder
    ↓
output coupling / amplifier
    ↓
VCA
```

with the actual surrounding capacitor networks retained.

These extra poles materially affect:

* low-frequency response;
* transient shape;
* phase;
* resonance shape;
* waveform-dependent distortion;
* the interaction between cutoff and resonance.

---

# 11. Resonance

## 11.1 Resonance Is Feedback, Not a Generic Q Parameter

The Resonance control should not simply select the Q of a textbook low-pass filter.

It changes an analogue feedback path surrounding the nonlinear ladder.

The feedback signal should therefore be taken from the correct filter node and returned to the correct filter input/summing point through the original resistive/capacitive network.

---

## 11.2 Resonance Nonlinearity

The filter's feedback path is nonlinear.

The exact implementation should therefore allow:

* amplitude-dependent feedback;
* asymmetric distortion;
* changing resonance amplitude with input drive;
* harmonic generation inside the feedback loop;
* changing resonance shape as cutoff and input level change.

Do not model resonance as:

```text
Q = 0..1
```

followed by a clean linear filter.

---

# 12. Self-Oscillation

A stock TB-303 does not behave like a classic Moog ladder whose maximum-resonance setting simply turns into a clean, high-level sine oscillator.

The stock circuit is designed so that the normal operating range is below clean sustained self-oscillation.

However:

* very high resonance,
* high cutoff,
* component variation,
* calibration,
* input level,
* feedback phase,
* nonlinear transistor behavior

can bring the filter closer to oscillation.

The correct emulation should therefore reproduce the actual feedback stability boundary instead of artificially preventing resonance from becoming unstable.

Do not simply clamp the filter output whenever resonance approaches 1.0.

---

# 13. Resonance Feedback High-Pass Behavior

The resonance feedback path contains frequency-dependent coupling.

A useful model includes a high-pass/coupling component in the feedback path, approximately in the region of the low hundreds of Hz.

However, this should **not** be reduced to the simple assertion:

```text
feedback HPF = 150 Hz at resonance 0
feedback HPF = 250 Hz at resonance 1
```

That particular mapping is not a sufficiently faithful description of the original circuit.

Instead:

* implement the actual RC network;
* include its loading;
* include the resonance pot resistance;
* include the phase shift;
* include the nonlinear feedback gain;
* allow the resulting low-frequency resonance attenuation to emerge.

One of the characteristic consequences is that resonance behaves differently as cutoff is moved lower, contributing to the characteristic reduction in low-frequency resonant emphasis.

---

# 14. Resonance Pot Is Dual-Gang

The original Resonance control is not merely one potentiometer feeding one parameter.

The service documentation identifies the physical Resonance control as a dual-gang 50 kΩ linear potentiometer.

One section controls the principal resonance feedback level.

The second section participates in the **accent sweep circuit**.

Therefore:

> The Resonance control simultaneously changes ordinary resonance and the shape of accented filter movement.

This is one of the most important control interactions in the TB-303.

A plugin implementation must not treat Resonance and Accent Sweep as independent parameters if the target is the original hardware.

---

# 15. Env Mod Circuit

The TB-303's Env Mod circuit is one of its defining nonlinear control arrangements.

It should not be implemented as simply:

```text
cutoff_Hz += envelope * EnvModDepth
```

The Roland service description specifically describes a biasing circuit in which the Env Mod control changes both the envelope contribution and the bias point around the filter's effective operating region.

The filter driver uses transistor circuitry with an anti-logarithmic response to control current.

The practical consequence is:

* the control is nonlinear;
* increasing Env Mod does not merely add a fixed number of Hz;
* the effective cutoff movement is compressed/expanded by the exponential current conversion;
* the same envelope voltage has a different audible effect at different cutoff settings.

---

# 16. Do Not Use a Simple Hz-Space Cutoff Formula

Avoid:

```text
Base_Cutoff =
    200 * 12.5^Cutoff

Cutoff =
    Base_Cutoff
    + EnvMod * 350
```

as the primary physical model.

That sort of mapping is useful only as a rough UI approximation.

The original machine uses:

* a potentiometer network;
* transistor biasing;
* current summing;
* nonlinear voltage-to-current conversion;
* calibration trimmers;
* supply-dependent operating points.

The correct software model should operate in an analogue-equivalent **control-current domain** wherever possible.

A practical abstraction is:

```text
I_cutoff =
    I_static(cutoff_pot)
  + I_env(env_voltage, env_mod_pot, bias)
  + I_accent(accent_state, accent_pot, sweep_cap)
  + I_other
```

followed by:

```text
fc = f(I_cutoff)
```

where `f()` is approximately exponential.

This better reproduces the way multiple control sources interact.

---

# 17. Cutoff Control

The physical Cutoff control is a 50 kΩ potentiometer.

Do not assume that a 0–1 knob position corresponds to a simple exponential frequency range of:

```text
200 Hz → 2.5 kHz
```

as a universal law.

The actual effective range depends on:

* transistor characteristics;
* filter calibration;
* bias voltages;
* resonance setting;
* Env Mod circuitry;
* component tolerances.

A nominal software calibration may be established around measured reference hardware, but should be implemented as a calibrated control-current curve rather than a fixed textbook frequency equation.

---

# 18. Filter Tracking

The original TB-303 does **not** have ordinary keyboard/filter tracking as a user control.

The filter cutoff is therefore independent of pitch except for indirect effects caused by:

* oscillator level;
* oscillator waveform;
* pitch-dependent square-wave shape;
* resonance behavior;
* analogue nonlinearities.

Do not implement conventional 1 V/octave filter tracking.

This is a frequent source of "303-like but not actually 303" behavior.

---

# 19. Main Envelope Generator (MEG)

The TB-303 has two envelope generators.

The first is conventionally referred to as the:

```text
Main Envelope Generator (MEG)
```

It primarily drives the VCF.

---

## 19.1 MEG Attack

The MEG has a very fast analogue attack.

A useful nominal software target is approximately:

```text
3 ms
```

but the important point is that the attack should be modeled as an RC/exponential process rather than as an instantaneous step.

The few-millisecond attack is important for the characteristic initial filter transient.

---

## 19.2 MEG Decay

For normal notes, MEG decay is controlled by the Decay potentiometer.

The original service documentation identifies a range approximately:

```text
minimum ≈ 200 ms
maximum ≈ 2.5 s
```

Other measurements/documentation commonly place the upper practical value closer to approximately 2 s.

For emulation, use the actual circuit-equivalent RC curve rather than a linear interpolation in seconds.

A reasonable nominal mapping is:

```text
tau_decay =
    exponential interpolation
    between approximately 0.2 s and 2–2.5 s
```

but an implementation based on the actual potentiometer and capacitor network is preferred.

---

# 20. MEG Gate-Off Behavior

The MEG should **not** be forcibly reset to zero when a staccato gate ends.

Its capacitor follows its analogue discharge trajectory.

When the next trigger arrives, its starting voltage depends on how far it had discharged.

Therefore:

```text
Note-off ≠ envelope reset
```

Instead:

```text
gate off
    ↓
MEG remains in its current discharge trajectory
    ↓
next non-slide trigger recharges/restarts from the existing state
```

This is important because closely spaced notes can begin with a non-zero envelope state.

Do not implement every note as:

```text
envelope = 0
attack from 0
```

unless empirical comparison demonstrates that a specific trigger condition requires it.

---

# 21. Volume Envelope Generator (VEG)

The second envelope drives the VCA.

It is fixed in the stock TB-303 and has no panel control.

It has:

* a very fast attack;
* a relatively long exponential decay;
* a rapid end-of-gate attenuation mechanism.

A practical nominal attack target is around:

```text
3 ms
```

The natural decay is substantially longer than the filter envelope.

Documentation associated with the original circuit places the normal VEG decay in the approximate region of:

```text
3–4 seconds
```

rather than the 4.0 s fixed value in the earlier specification.

---

# 22. VCA Gate-Off Envelope

The end of a normal TB-303 note is not best modeled by simply switching the VCA off at gate-off.

The original hardware includes a rapid final decay mechanism.

A useful approximation is:

```text
normal volume region:
    exponential decay

gate-off:
    short residual decay
    ≈ 10–20 ms overall
```

Measurements reported for original-style circuitry place the end behavior around approximately 16 ms, with roughly:

```text
8 ms normal-volume portion
+
8 ms final linear decay
```

being a useful descriptive approximation.

This should be treated as an analogue transition rather than:

```text
VCA = 0 at gate-off
```

because an instantaneous VCA shutdown creates a noticeably different click spectrum.

---

# 23. VCA Implementation

The original TB-303 uses a Roland **BA662A** voltage-controlled amplifier.

A BA662-like model should be preferred over a generic linear VCA.

The BA662 is a transconductance-based device whose effective gain is controlled by current.

Therefore model:

* control current;
* finite control range;
* finite residual gain;
* input/output loading;
* nonlinear current-to-gain behavior;
* saturation at high signal/control levels;
* noise;
* offsets.

The BA662 should not simply be represented by:

```text
output = input * envelope
```

especially for high-level accented material.

---

# 24. Accent

Accent is not velocity scaling.

The TB-303 accent is a **binary sequencer event** which changes several analogue control paths simultaneously.

On an accented note, the MEG contributes to:

1. filter modulation;
2. accent sweep;
3. VCA control;
4. accelerated filter-envelope decay.

Accent therefore modifies the complete voice behavior.

Do not implement accent as:

```text
velocity > threshold
    → gain += 6 dB
```

alone.

---

# 25. Accent MEG Decay Override

On an accented note, the MEG decay control is bypassed/switched so that the MEG uses approximately the minimum normal decay time.

Nominally:

```text
accent MEG decay ≈ 200 ms
```

independent of the front-panel Decay setting.

This is a real circuit switching function.

Therefore:

```text
normal note:
    MEG decay = Decay knob

accented note:
    MEG decay ≈ minimum / fixed short decay
```

The Accent circuit should not merely multiply the decay coefficient.

---

# 26. Accent VCA Control

The MEG is also routed into the VCA control path during Accent.

Importantly, this contribution is not an instantaneous +6 dB gain step.

The signal goes through additional analogue filtering, including a network associated with approximately:

```text
47 kΩ
+
0.033 µF
```

which softens the accent contribution.

Consequently the accented VCA shape is:

```text
normal VEG
+
accent-derived MEG contribution
```

rather than:

```text
normal VEG * 2
```

The exact control-current summing behavior should be retained.

---

# 27. Accent Sweep Circuit

This is arguably the most important non-obvious feature in the TB-303.

The accent sweep is a memory-bearing analogue circuit.

A representative topology is:

```text
MEG
 |
 diode
 |
47 kΩ
 |
 +----> anti-clockwise end of 100 kΩ section
 |                |
 |                |
 |            100 kΩ
 |                |
 |                +----> 1 µF capacitor to ground
 |                |
 |              wiper
 |                |
 |              100 kΩ
 |                |
 +--------------> filter control-current summing node
```

The 100 kΩ control element is associated with the **second section of the Resonance control**.

This means Resonance changes the accent-sweep behavior.

---

# 28. Accent Sweep at Low Resonance

With the Accent Sweep section near the anti-clockwise end:

* the MEG reaches the filter more directly;
* the transient is more abrupt;
* the capacitor stores relatively little of the signal;
* the accent behaves more like a sharp filter kick.

Therefore:

```text
Resonance low-ish
    → sharper accent filter transient
```

---

# 29. Accent Sweep at High Resonance

As the second Resonance section moves clockwise:

* more of the accent signal charges the 1 µF capacitor;
* the capacitor voltage changes more slowly;
* the filter receives a smoother, delayed control signal;
* the filter sweep becomes more curved.

This creates the characteristic:

```text
"wow"
"wapp"
"rubbery"
```

accent response associated with high-resonance 303 programming.

This is not equivalent to substituting a conventional ADSR.

---

# 30. Accent Sweep Memory

The 1 µF capacitor does not completely discharge between closely spaced accented notes.

Therefore:

```text
Accent 1
    → capacitor charges

Accent 2
    → capacitor starts above zero

Accent 3
    → starts higher again

Accent 4
    → can produce an even higher filter excursion
```

The result is an increasing series of accent peaks.

This is one of the defining reasons consecutive accented notes can produce a rising "animal-like" or vocal filter sweep.

The emulation must maintain this capacitor state continuously between notes.

Do not clear the accent-sweep state at note boundaries.

---

# 31. Accent Sweep Mathematical Approximation

A physically useful software approximation is:

```text
v_c14/dt =
    diode_charge(
        MEG - v_c14
    )
    /
    R_charge*C
```

with a separate discharge path:

```text
dv_c14/dt =
    -v_c14 / (R_discharge*C)
```

and the pot determining how much of:

```text
MEG
```

and:

```text
v_c14
```

is sent to the filter summing node.

The approximate time constant of a 47 kΩ + 1 µF path is:

```text
τ ≈ 47 ms
```

but the actual behavior is modified by the 100 kΩ section of the Resonance potentiometer and by the diode's nonlinear conduction.

Therefore this should be implemented as an RC state variable, not as a fixed envelope curve.

---

# 32. Accent Is Not a Fixed Cutoff Offset

Do not use:

```text
cutoff += 100..300 Hz
```

as a generic accent rule.

The actual accent contribution depends on:

* MEG amplitude;
* Accent pot position;
* Resonance second-gang position;
* capacitor state;
* diode conduction;
* filter bias;
* current summing;
* current cutoff.

The observed upward movement on consecutive accents is an emergent consequence of the stored capacitor charge.

---

# 33. Sequencer Clock

The TB-303 internal timing uses:

```text
24 clock pulses per quarter note
```

A 1/16 note therefore contains:

```text
6 clock pulses
```

This is fundamental to exact gate/slide reproduction.

Do not use an arbitrary percentage-based note gate.

---

# 34. Normal Gate Timing

For a normal 1/16 note:

```text
clock pulse 0:
    gate rises

clock pulse 3:
    gate remains high through its positive portion

halfway through pulse 3:
    gate falls
```

Thus the nominal gate timing is:

```text
3.5 clock pulses ON
2.5 clock pulses OFF
```

per six-clock 1/16-note period.

This is not exactly 50/50.

Therefore do not use:

```text
gate_on_time = step_time * 0.5
```

as the hardware model.

---

# 35. Slide Timing

The slide flag belongs conceptually to the **preceding note**, because it causes the following note to be tied to it.

For:

```text
Note A [slide]
Note B
```

the behavior is:

```text
Note A:
    normal pitch

At the start of Note B:
    CV changes from A toward B
    gate remains high
    VCA does not close
    filter envelope does not retrigger
    pitch begins gliding
```

The slide signal begins at the start of the following note and lasts through that note.

This is a crucial distinction.

Do not interpret:

```text
slide = current note
```

as:

```text
current note begins sliding before it starts
```

---

# 36. Slide and Gate Interaction

For a slid note:

```text
gate:
    remains continuously high through the boundary

CV:
    changes to the next programmed pitch

slide:
    activates at the next note boundary

VCA:
    remains open

MEG:
    is not retriggered

VEG:
    remains active

VCO:
    continues running while pitch slews
```

This is what creates the characteristic legato transition.

---

# 37. Pitch Glide

The slide mechanism behaves approximately as an analogue lag circuit.

A useful nominal time constant is:

```text
τ_slide ≈ 60 ms
```

for a stock unit.

Do not assume that "slide" means a fixed linear-time ramp.

A first-order analogue lag is a better abstraction:

```text
dV_pitch/dt =
    (V_target - V_pitch) / τ_slide
```

or, in discrete time:

```text
V_pitch[n+1] =
    V_pitch[n]
    + α * (V_target - V_pitch[n])
```

with:

```text
α = 1 - exp(-1/(Fs*τ_slide))
```

The hardware glide should operate on the **analogue-equivalent DAC voltage**, before the oscillator's exponential pitch conversion.

---

# 38. Slide Does Not Reset Oscillator Phase

On a slide:

```text
VCO phase continues
```

while:

```text
frequency changes continuously
```

The oscillator should therefore not be restarted or hard-synced at the destination note.

A phase-resetting wavetable implementation can sound noticeably different.

---

# 39. Normal Note Retriggering

A normal non-slide note produces a new gate event.

The envelopes therefore receive their appropriate trigger/recharge operation.

However, an accurate analogue model should allow residual capacitor voltage to matter.

Do not make all envelope states vanish instantaneously at every note transition.

The actual hardware contains:

* gate switches;
* transistor discharge paths;
* capacitor memory;
* differing recharge/discharge paths.

This means the exact starting voltage of the next envelope depends on the preceding note timing.

---

# 40. Slide Does Not Retrigger Envelopes

When a slide ties two notes:

```text
MEG:
    continues

VEG:
    continues

VCA:
    remains open

oscillator:
    continues

pitch:
    slews
```

There is no new attack pulse at the boundary.

This is essential.

A plug-in which retriggers the filter envelope on every MIDI note while merely applying portamento to pitch will not reproduce the TB-303 slide behavior.

---

# 41. Rests

A rest is not equivalent to a very low note.

A rest removes the note/gate event.

The analogue voice therefore undergoes its actual gate-off behavior.

The oscillator may continue running internally, but the VCA closes according to the envelope circuitry.

---

# 42. Envelope Trigger State

For maximum accuracy, envelopes should be represented as continuous analogue states:

```text
MEG voltage
VEG voltage
accent-sweep capacitor voltage
slide pitch voltage
```

rather than as purely symbolic ADSR phases.

Each state should evolve continuously between sequencer events.

A good internal model contains:

```text
state:
    meg
    veg
    accent_cap
    pitch_cv
    oscillator_phase
```

plus the current:

```text
gate
accent
slide
target_pitch
```

---

# 43. Output VCA and Distortion

The TB-303's characteristic aggressive sound is not produced by one final `tanh()` block.

Nonlinearity occurs at multiple places:

1. oscillator/wave-shaping circuitry;
2. filter transistor/diode stages;
3. resonance feedback path;
4. filter amplifier/mixer stages;
5. BA662 VCA;
6. output/mixer stages.

Therefore a useful model distributes nonlinearity throughout the signal path.

Avoid:

```text
clean oscillator
→ clean filter
→ clean VCA
→ tanh()
```

because this produces a fundamentally different harmonic structure.

---

# 44. Asymmetrical Distortion

The distortion should not necessarily be symmetrical.

Analogue transistor circuits have:

* different positive and negative headroom;
* base-emitter nonlinearities;
* saturation;
* loading;
* bias offsets.

This means:

```text
positive half-cycle
```

and:

```text
negative half-cycle
```

do not necessarily clip identically.

The resulting asymmetry generates:

* even harmonics;
* DC offsets at internal nodes;
* waveform-dependent filter excitation;
* unusual resonance sidebands.

A simple symmetric `tanh(x)` is therefore insufficient for the final implementation.

---

# 45. Input Level to Filter Is Important

The TB-303 filter is strongly dependent on the level of the waveform driving it.

This is particularly important when comparing saw and square.

The filter should receive approximately the actual circuit-level oscillator amplitude.

Do not normalize every oscillator waveform to:

```text
peak = 1.0
RMS = 1.0
```

before the VCF.

That removes an important physical variable.

---

# 46. Waveform-Dependent Filter Drive

The saw and square should be calibrated independently.

A useful calibration procedure is:

1. select saw;
2. measure oscillator output immediately before the VCF;
3. select square;
4. measure its output at the same point;
5. preserve the real amplitude ratio;
6. run both through the same nonlinear filter model.

This naturally produces the different resonance/distortion character of the two waveforms.

---

# 47. Low-Frequency Response

The TB-303 does not have perfectly flat sub-bass response.

The original signal path contains coupling capacitors and loading which cause low-frequency attenuation.

A modern clean digital filter with DC-corrected coupling will therefore tend to sound:

* deeper;
* cleaner;
* more sub-heavy

than the original.

The low-frequency coupling networks should consequently be modeled rather than removed.

As a practical validation point, original-style machines exhibit measurable attenuation in the low bass; documentation associated with later Devil Fish measurements notes that the original response around 32 Hz is significantly below midband.

Do not add a modern "bass enhancement" stage to the stock model.

---

# 48. Power Supply and Bias

The original unit uses several internal rails, including approximately:

```text
+12 V
+6 V
+5.333 V
```

with other internal bias arrangements.

The exact DC operating points matter because transistor currents determine:

* oscillator behavior;
* filter cutoff;
* resonance;
* VCA operation;
* envelope thresholds.

A high-fidelity circuit model should therefore work from the equivalent internal bias voltages rather than assuming an abstract ±15 V modular-synth environment.

---

# 49. Component-Level Variation

For an "ideal stock" reference mode:

```text
all resistor values = nominal
all capacitors = nominal
matched pairs = nominally matched
diodes = nominal
supply = calibrated
trim pots = factory/reference values
```

For a more realistic hardware mode, add small variations to:

```text
transistor Vbe
transistor beta
matched-pair mismatch
capacitor value
capacitor leakage
resistor value
diode Vf
supply voltage
temperature
trim values
```

These variations should be small.

The TB-303 should not randomly drift several semitones or wildly change filter cutoff every voice reset.

Use slow continuous variation instead.

---

# 50. Temperature

Temperature should affect analogue parameters very slowly.

Useful parameters to perturb include:

```text
VCO current scale
VCO tuning
filter bias
transistor nonlinearities
VCA gain
capacitor leakage
```

Use a very low-frequency random process or thermal-state approximation.

Do not use sample-rate noise to model analogue drift.

---

# 51. Noise

A realistic emulation should include very small analogue noise sources.

Noise sources include:

* transistor noise;
* resistor noise;
* VCA noise;
* filter noise;
* power-supply/bias noise.

Noise should be extremely low compared to the oscillator signal.

Do not add a conspicuous white-noise generator merely because "analogue synths have noise."

The purpose is to reproduce:

* silence floor;
* resonance texture;
* very-low-level filter behavior;
* slight instability at high resonance.

---

# 52. Service Calibration References

Important original service procedures include:

### VCO

```text
A = 110 Hz
octave ratio = 4:1
```

with the oscillator width/tuning trims adjusted accordingly.

### VCF

The service procedure uses:

```text
Cutoff = center
Waveform = sawtooth
Resonance = full clockwise
Env Mod = minimum
Decay = minimum
Accent = minimum
```

and adjusts the VCF trimmer against a specified transient/oscillation response.

A software emulation should expose equivalent hidden calibration parameters rather than assuming the front-panel parameter mapping alone defines the entire machine.

---

# 53. Hidden Calibration Parameters

Recommended internal plugin parameters:

```text
vco_scale
vco_offset
vco_width
vco_reset_threshold
vco_wave_amplitude
vco_square_shape
filter_frequency_scale
filter_frequency_offset
filter_stage_mismatch
filter_resonance_scale
filter_feedback_offset
filter_bias
env_attack_scale
env_decay_min
env_decay_max
veg_attack_scale
veg_decay_scale
accent_sweep_charge
accent_sweep_discharge
accent_sweep_gain
vca_scale
vca_offset
vca_nonlinearity
output_level
supply_voltage
temperature
component_variation
```

These should not necessarily be exposed to the user.

They are useful for matching an individual reference TB-303.

---

# 54. Recommended DSP Architecture

A high-quality implementation should use a hybrid analogue/circuit model.

Recommended processing:

```text
1. Sequencer timing
2. DAC-equivalent pitch generation
3. Analogue slide lag
4. Continuous oscillator phase integration
5. Nonlinear waveform generation
6. Envelope state update
7. Accent-sweep capacitor state update
8. Filter control-current summing
9. Nonlinear diode-ladder solve
10. BA662-like VCA
11. Output/mixer coupling network
12. Oversampled output processing
13. Downsample to host sample rate
```

---

# 55. Oversampling

The nonlinear oscillator and filter should be oversampled.

Recommended minimum:

```text
4x
```

Preferably:

```text
8x
```

or higher for a high-quality offline render.

This is particularly important for:

* transistor nonlinearities;
* diode conduction;
* resonance;
* filter self-excitation;
* hard oscillator resets;
* VCA transients;
* accent attacks.

Simply running a nonlinear filter at 44.1 kHz with no oversampling will generate aliasing that was not present in the original analogue hardware.

---

# 56. Nonlinear Filter Solver

A zero-delay-feedback or equivalent implicit nonlinear solver is preferable to an ordinary four-pole IIR cascade.

At every oversampled time step:

1. estimate the stage currents;
2. solve the nonlinear stage equations;
3. include feedback;
4. update the node states;
5. calculate the output.

Possible numerical methods:

```text
Newton-Raphson
```

or a robust fixed-point/Newton hybrid.

The goal is to preserve the simultaneous interaction of:

```text
input
+
feedback
+
four nonlinear stages
+
capacitor states
```

within the same sample interval.

---

# 57. Do Not Use Four Independent State Variables With Independent Saturation

This:

```text
stage1 = LP1(input)
stage2 = LP2(stage1)
stage3 = LP3(stage2)
stage4 = LP4(stage3)

stage1 = tanh(stage1)
stage2 = tanh(stage2)
...
```

is better than a generic 24 dB filter but still misses the important loading interaction.

The actual ladder behaves approximately as a coupled diffusion chain:

```text
input ↔ stage1 ↔ stage2 ↔ stage3 ↔ stage4
```

rather than:

```text
input → stage1 → stage2 → stage3 → stage4
```

The bidirectional loading is important.

---

# 58. Cutoff Frequency Definition

The "cutoff" control should be interpreted as the analogue filter control-current setting, not necessarily the -3 dB point.

Because the TB-303 has:

* spread poles;
* nonlinear behavior;
* resonance;
* coupling networks;

the frequency of the resonance peak, -3 dB corner and panel "cutoff" parameter are not identical quantities.

This is why blindly matching a single cutoff frequency measurement can produce an incorrect filter.

---

# 59. Resonance Frequency Movement

At high resonance, the resonant peak does not necessarily sit exactly at the nominal cutoff control frequency.

The nonlinear ladder and feedback network can shift the resonant peak.

This movement should be allowed to emerge from the circuit.

Do not add:

```text
resonance_peak = cutoff * fixed_multiplier
```

as an independent parameter.

---

# 60. Resonance Gain Reduction / Passband Behavior

Increasing resonance changes more than peak amplitude.

Because the feedback path loads the filter:

* the low-frequency/passband response can change;
* filter gain can fall;
* resonance can become frequency-dependent;
* the filter can appear to lose bass as resonance is increased;
* the exact amount depends on cutoff and drive.

This effect is fundamental to the character of the TB-303.

Do not add a separate "bass compensation" stage unless you are explicitly modeling a modified machine.

---

# 61. Accent + Resonance Interaction

Accent Sweep becomes increasingly important as the Resonance control is turned up.

Therefore:

```text
Accent amount
```

and:

```text
Resonance
```

must interact.

The same Accent setting should sound different at:

```text
Resonance = low
```

versus:

```text
Resonance = high
```

not just because resonance itself is higher, but because the **accent sweep circuit changes shape**.

---

# 62. Consecutive Accents

A useful test pattern is:

```text
A A A A
```

with identical pitch and identical Accent.

At moderate/high Resonance, the filter excursion should exhibit a characteristic sequence in which successive accents can rise because of the stored charge in the accent-sweep capacitor.

If every accented note has exactly the same filter-envelope peak, the accent circuit is not being modeled correctly.

---

# 63. Accent Test Matrix

Test:

```text
Accent = off
Accent = on
```

at:

```text
Resonance = 0
Resonance = 0.25
Resonance = 0.5
Resonance = 0.75
Resonance = 1
```

Then test:

```text
one accent
two consecutive accents
four consecutive accents
eight consecutive accents
```

The filter peak should change as the accent-sweep capacitor accumulates charge and then decays.

---

# 64. Slide Test Matrix

Test:

```text
C -> G
C -> C
C -> octave-up C
C -> octave-down C
```

with:

```text
no slide
slide
```

For a slid note:

* the waveform must remain continuous;
* the pitch must glide;
* there must be no fresh attack transient from the envelopes;
* the filter envelope must remain continuous;
* the VCA must remain open.

---

# 65. Normal Gate Test

For a sequence of identical notes without Accent or Slide:

```text
C C C C C C
```

the resulting audio should demonstrate:

* a short onset;
* a characteristic staccato decay;
* residual envelope state;
* no phase reset artifacts;
* approximately 3.5/2.5 clock-pulse gate timing.

---

# 66. Waveform Test

Run:

```text
Saw
Square
```

with:

```text
Cutoff fully open
Resonance minimum
Env Mod minimum
Decay minimum
Accent off
```

and compare:

* waveform amplitude;
* duty cycle;
* harmonic spectrum;
* transient shape;
* low-frequency roll-off;
* waveform symmetry.

The unfiltered oscillator outputs are themselves an important part of an accurate 303 emulation.

---

# 67. Saw/Square Filter-Drive Test

Run identical settings using:

```text
Saw
Square
```

with:

```text
Resonance ≈ 70–90%
```

The filter should respond differently.

Do not compensate the difference away.

It is expected.

---

# 68. Filter Low-Cutoff Test

Set:

```text
Cutoff low
Resonance high
```

and verify that the model does not produce the same clean, strong self-resonating low-frequency sine found in a generic resonant ladder.

The real circuit's feedback/coupling network strongly affects low-frequency resonance.

---

# 69. High-Cutoff Resonance Test

Set:

```text
Cutoff high
Resonance high
```

and drive with:

```text
Saw
Square
```

The model should produce increasing nonlinear resonance and harmonic interaction without immediately turning into an ideal mathematical sine oscillator.

---

# 70. Filter Input Level Test

Multiply oscillator level by:

```text
0.5x
1x
2x
4x
```

and observe the filter.

The filter sound should change nonlinearly.

A clean linear filter followed by a final waveshaper will fail this test.

---

# 71. Output Path

The stock machine contains a mixer/output section after the VCA.

The final model should include the main analogue output coupling behavior where it materially affects the sound.

However:

> Do not confuse the TB-303's line output with the later distortion often added by external mixers, pedals, amplifiers or recordings.

The stock plugin model should represent the synthesizer itself first.

Optional external drive should be separate.

---

# 72. External Input

The stock TB-303 has a mixer/input architecture in the audio circuitry.

The stock emulation should not add modern external-audio filtering or saturation unless that input path is explicitly being emulated.

---

# 73. MIDI Implementation

For a plugin receiving MIDI:

```text
MIDI NOTE
    ↓
quantized pitch voltage
    ↓
slide lag when previous step is slid
    ↓
VCO frequency
```

Accent should be treated as a discrete state.

There is no original velocity-sensitive loudness mechanism.

A velocity-to-amplitude mapping should therefore be disabled in the stock mode.

Recommended MIDI interpretation:

```text
velocity:
    ignored for ordinary notes

optional threshold:
    velocity >= configurable threshold
        → Accent
```

This is an implementation convenience rather than original hardware behavior.

---

# 74. MIDI Note Retriggering

A MIDI note-on should not automatically imply the same behavior as a TB-303 sequencer note.

For maximum hardware fidelity, the plugin should distinguish:

```text
new gate
```

from:

```text
CV update during held gate
```

A useful internal event model is:

```text
NOTE:
    pitch
    gate_on
    accent
    slide_to_next
```

The sequencer decides whether the next event produces:

```text
gate restart
```

or:

```text
CV-only pitch update
```

---

# 75. Internal State Machine

Recommended voice state:

```text
struct TB303State
{
    oscillator_phase;

    pitch_cv;
    pitch_target;
    slide_active;

    gate;

    meg_voltage;
    veg_voltage;

    accent_cap_voltage;

    filter_state_1;
    filter_state_2;
    filter_state_3;
    filter_state_4;

    vca_state;

    previous_accent;
    previous_slide;
};
```

Do not reset all state at every note.

Only the hardware-equivalent gate/switch operations should change state.

---

# 76. Recommended Control Model

Use normalized front-panel controls:

```text
cutoff      0..1
resonance   0..1
env_mod     0..1
decay       0..1
accent      0..1
tuning      approximately ±700 cents
```

but convert them internally through calibrated analogue-equivalent curves.

Do not expose the internal values:

```text
cutoff_hz
Q
attack_seconds
decay_seconds
accent_db
```

as the primary model.

Those are consequences of the circuitry.

---

# 77. Control Potentiometer Types

The original service documentation identifies approximately:

```text
Tuning:
    50 kΩ B

Cutoff:
    50 kΩ A

Env Mod:
    50 kΩ A

Decay:
    1 MΩ A

Resonance:
    dual 50 kΩ B

Accent:
    50 kΩ B

Volume:
    50 kΩ A
```

The taper type matters.

A software control should therefore reproduce the **effective physical taper**, not assume all front-panel knobs are linear.

---

# 78. Potentiometer Taper

For each analogue parameter:

```text
physical knob position
    ↓
pot resistance/taper
    ↓
analogue control voltage/current
    ↓
transistor/current conversion
    ↓
audio parameter
```

A better emulation can therefore be calibrated against measured physical knob positions.

Do not use:

```text
parameter = knob
```

directly.

---

# 79. Nominal Plugin Parameter Curves

For a convenience implementation where full circuit solving is not used, the following approximations are preferable to the original specification:

### Cutoff

Use a nonlinear control-current curve with adjustable calibration:

```text
I_cutoff = f_cutoff(cutoff)
```

rather than an absolute:

```text
Hz = 200 * 12.5^x
```

### Env Mod

Use a nonlinear current increment:

```text
I_env = f_env(meg, env_mod, cutoff_bias)
```

rather than:

```text
Hz += env_mod * constant
```

### Resonance

Use:

```text
feedback_gain = f_resonance(resonance)
```

and independently:

```text
accent_sweep_mix = f_resonance2(resonance)
```

because the hardware uses two mechanically linked pot sections.

---

# 80. Envelope Equations

For an approximate software model:

## MEG attack

```text
dM/dt = (1 - M) / tau_attack
```

with:

```text
tau_attack ≈ 3 ms
```

## MEG decay

```text
dM/dt = -M / tau_decay
```

with:

```text
tau_decay =
    exp_interpolate(
        0.2 s,
        2.0–2.5 s,
        decay
    )
```

## VEG decay

```text
dV/dt = -V / tau_veg
```

with:

```text
tau_veg ≈ 3–4 s
```

followed by the gate-off discharge behavior.

These equations are approximations. A more accurate implementation models the actual RC voltages.

---

# 81. Correct Envelope Triggering

Avoid:

```text
on every note:
    MEG = 0
    VEG = 0
```

Instead:

```text
normal gate transition:
    hardware-equivalent trigger/recharge

slide:
    no new gate transition

gate off:
    hardware-equivalent discharge
```

This distinction is crucial.

---

# 82. Accent State

Accent should cause approximately:

```text
MEG decay → short/fixed decay
MEG → accent sweep
MEG → accent VCA control
```

but it should not necessarily:

```text
VEG = 2 * VEG
```

nor:

```text
filter_cutoff += fixed_Hz
```

nor:

```text
resonance += fixed_Q
```

---

# 83. Filter Driver

A good approximation to the filter driver is:

```text
filter_current =
      current_from_cutoff
    + current_from_env_mod
    + current_from_accent_sweep
    + bias
```

Then:

```text
filter_rate =
    exponential(
        filter_current
    )
```

This current-domain model is much closer to the original transistor anti-log circuits than frequency-domain addition.

---

# 84. Diode/Transistor Nonlinearity

For maximum fidelity use a diode-equivalent relationship such as:

```text
I = Is * (exp(V/(n*Vt)) - 1)
```

with appropriate limiting.

A computationally cheaper approximation can be:

```text
I = Is * sinh(V/(n*Vt))
```

or a calibrated smooth odd nonlinearity.

A plain `tanh()` can be used for a real-time approximation, but its coefficient should be calibrated to the ladder's conduction curve.

The important point is that nonlinearities should be applied to **inter-stage voltage differences**.

---

# 85. Filter Stage Mismatch

The idealized filter can use nominally identical stages.

For a more realistic version, introduce very small independent mismatch:

```text
stage1_scale
stage2_scale
stage3_scale
stage4_scale
```

and:

```text
stage1_nonlinearity
...
stage4_nonlinearity
```

Do not make the mismatch audible as obvious detuning.

Its purpose is to reproduce slightly asymmetric harmonic behavior.

---

# 86. Capacitor Variation

The filter's capacitor ratios have a direct influence on pole spacing and therefore on the characteristic response.

If using a circuit approximation, preserve the actual relative capacitor ratios from the service schematic.

Do not replace them with:

```text
four identical capacitors
```

unless the approximation has been independently verified against the original.

---

# 87. Validation Against Linear Response

Before adding nonlinear behavior, validate the filter with:

```text
resonance = minimum
input = small signal
```

and verify:

* DC gain;
* low-frequency response;
* pole locations;
* transition shape;
* high-frequency slope.

Only after the linearized response is correct should nonlinear resonance and high-level drive be introduced.

---

# 88. Validation Against Large-Signal Response

Then validate with:

```text
resonance = high
input = saw
```

and:

```text
input = square
```

Compare:

* harmonic spectra;
* resonant peak;
* asymmetric peak shape;
* low-frequency resonance loss;
* clipping behavior.

The filter should change behavior as the oscillator amplitude changes.

---

# 89. Do Not "Fix" the TB-303

The stock mode should not add:

* bass enhancement;
* filter tracking;
* oscillator drift beyond realistic component drift;
* sub oscillator;
* oscillator detuning;
* modern smoothing;
* velocity amplitude control;
* perfect square wave;
* perfect sine resonance;
* ideal 24 dB filter;
* post-filter compressor;
* stereo widening.

These may be useful optional modes, but they are not the stock TB-303.

---

# 90. Reference Signal-Flow Implementation

A suitable reference implementation is:

```text
SEQ
 |
 | 6-bit pitch value
 v
DAC-equivalent CV
 |
 v
SLIDE RC
 |
 v
VCO current converter
 |
 v
SAW CORE
 |
 +--------> SAW
 |
 +--------> TRANSISTOR WAVESHAPER --> SQUARE
 |
 v
WAVEFORM SELECTOR
 |
 v
INPUT COUPLING NETWORK
 |
 v
NONLINEAR VCF
 |
 +<----- resonance feedback path
 |
 v
VCF output coupling
 |
 v
BA662-style VCA
 |
 +<----- VEG
 |
 +<----- Accent/MEG contribution
 |
 v
MIXER
 |
 v
OUTPUT COUPLING / AMP
 |
 v
AUDIO OUT
```

Control path:

```text
GATE
 |
 +------> MEG
 |
 +------> VEG
 |
 +------> slide/gate logic

MEG
 |
 +------> Env Mod / cutoff current
 |
 +------> Accent VCA path
 |
 +------> Accent Sweep RC
                  |
                  +------> cutoff current
```

---

# 91. Parameter Interaction Matrix

| Control   | Primary function           | Secondary interaction                     |
| --------- | -------------------------- | ----------------------------------------- |
| Tuning    | VCO pitch                  | Changes pitch-dependent square shape      |
| Cutoff    | Static filter bias/current | Interacts with Env Mod and Resonance      |
| Resonance | Filter feedback            | Second pot section controls Accent Sweep  |
| Env Mod   | MEG → filter current       | Changes filter bias/current range         |
| Decay     | MEG decay                  | Accent bypasses this and uses short decay |
| Accent    | Accent circuit amplitude   | Affects MEG decay, VCA, and Accent Sweep  |
| Volume    | Output level               | Changes available final-stage headroom    |

---

# 92. Updated Parameter Summary

| Parameter / Module | Hardware-accurate characteristic                       | Recommended model                            |
| ------------------ | ------------------------------------------------------ | -------------------------------------------- |
| Oscillator         | Single analogue saw-core VCO                           | Continuous phase-integrating oscillator      |
| Saw                | Analogue ramp, not an arbitrary digitally filtered saw | Circuit-derived/oversampled saw              |
| Square             | Transistor-derived from oscillator                     | Frequency-dependent nonlinear waveshaper     |
| Square duty        | Frequency-dependent, not fixed 46%                     | Emergent or calibrated pitch-dependent curve |
| Waveform level     | Saw and square have different drive levels             | Preserve physical amplitude ratio            |
| VCF                | Four-stage nonlinear transistor/diode ladder           | Coupled nonlinear state-space model          |
| Filter poles       | Widely separated rather than four identical poles      | Preserve actual ladder topology              |
| Apparent slope     | Often behaves ≈18 dB/oct in important region           | Do not substitute a 3-pole filter            |
| Coupling networks  | Several additional poles around ladder                 | Model explicitly                             |
| Resonance          | Nonlinear feedback                                     | Circuit feedback loop                        |
| Resonance pot      | Dual-gang                                              | One section for Q, one for accent sweep      |
| Filter feedback    | Frequency-dependent coupling                           | Explicit RC feedback path                    |
| Cutoff             | Nonlinear transistor current control                   | Current-domain model                         |
| Env Mod            | Changes bias and filter current                        | Do not add fixed Hz offset                   |
| MEG attack         | ≈3 ms                                                  | Exponential RC                               |
| MEG decay          | ≈200 ms to ≈2–2.5 s normal                             | RC / calibrated nonlinear taper              |
| Accent MEG decay   | ≈200 ms                                                | Bypass normal Decay network                  |
| VEG attack         | ≈3 ms                                                  | Exponential RC                               |
| VEG decay          | ≈3–4 s                                                 | Fixed analogue RC                            |
| Gate-off VCA tail  | ≈10–20 ms                                              | Hardware-equivalent quick discharge          |
| Accent VCA         | MEG-derived control current                            | RC-smoothed analogue sum                     |
| Accent Sweep       | 47 kΩ + diode + 1 µF + pot network                     | Explicit capacitor state                     |
| Accent memory      | Capacitor retains charge                               | Continuous state, never reset per note       |
| Slide              | Analogue pitch lag                                     | First-order RC slew                          |
| Slide time         | ≈60 ms nominal                                         | Exponential lag                              |
| Slide envelope     | No envelope retrigger                                  | Continuous MEG/VEG                           |
| Normal gate        | 3.5 of 6 clock pulses                                  | Exact clock-edge timing                      |
| Clock              | 24 ppqn                                                | 6 pulses / 16th note                         |
| Pitch CV           | 1 V/oct, six-bit DAC                                   | Quantized DAC-equivalent pitch               |
| VCA                | BA662A                                                 | Current-controlled nonlinear VCA             |
| Output             | Analogue mixer/coupling                                | Include output loading/coupling              |
| Nonlinearity       | Distributed through circuit                            | Multiple nonlinear stages                    |
| Oversampling       | Needed for nonlinear fidelity                          | Prefer 8x or higher                          |
| Variation          | Real component/calibration variation                   | Small continuous tolerances                  |

---

# 93. Things Specifically Removed From the Earlier Specification

The following should **not** be treated as authoritative TB-303 hardware definitions:

```text
Saw → fixed 14 kHz one-pole LPF
Saw → f(x) = x - 0.05x²
Square → fixed 46% duty cycle at all frequencies
Square → dedicated 150 Hz high-pass filter
Filter → four independently cascaded poles
Filter → fixed 18 dB/octave response
Feedback → single tanh() block
Feedback HPF → 150–250 Hz resonance-dependent formula
Cutoff → fixed 200–2500 Hz exponential equation
Env Mod → fixed +350 Hz baseline offset
Resonance → fixed 15% cutoff reduction
Accent → fixed +6 dB VCA boost
Accent → arbitrary +100–300 Hz cutoff accumulation
VCA gate-off → fixed 18 ms exponential release
Slide → generic 70 ms glide independent of sequencer gate
```

These are useful abstractions for a generic "acid bass synth", but they do not constitute a sufficiently accurate representation of the original circuit.

---

# 94. Minimum-Fidelity Implementation

For a practical real-time implementation where full transistor simulation is too expensive, prioritize the following in this order:

```text
1. Exact gate/slide timing
2. Continuous oscillator phase
3. Realistic oscillator waveform generation
4. Pitch-dependent square waveform
5. Nonlinear 4-stage coupled diode ladder
6. Actual resonance feedback architecture
7. Nonlinear Env Mod/current-domain cutoff model
8. MEG/VEG analogue state behavior
9. Accent Sweep capacitor memory
10. Resonance dual-gang Accent interaction
11. BA662-like VCA
12. Output coupling
13. Component variation/noise
```

Do not spend CPU on cosmetic analogue drift while using a generic 24 dB filter.

The filter and accent/gate interactions are far more important.

---

# 95. Best-Accuracy Implementation

For the highest possible fidelity:

```text
- Solve the oscillator from the transistor-level topology.
- Solve the waveform shaper from the actual transistor stages.
- Solve the complete VCF network from the original schematic.
- Preserve every significant coupling capacitor.
- Use nonlinear diode/BJT conduction.
- Solve the ladder implicitly at oversampled rate.
- Model the Env Mod transistor bias circuit.
- Model both sections of the Resonance pot.
- Model the Accent Sweep diode/RC network explicitly.
- Model the MEG and VEG as capacitor voltages.
- Model the BA662 VCA as a nonlinear transconductance stage.
- Model the output/mixer coupling network.
- Reproduce the six-clock sequencer timing exactly.
- Preserve the 1/12-V pitch DAC quantization.
- Perform output calibration against an actual recorded/reference TB-303.
```

---

# 96. Reference Validation Procedure

A serious emulation should be validated at multiple levels.

## Level 1: Static oscillator

Compare:

```text
A = 110 Hz
Saw
Square
```

and verify:

* frequency;
* amplitude;
* duty cycle;
* DC offset;
* harmonic spectrum.

## Level 2: Filter small-signal

Use:

```text
very low oscillator level
Resonance minimum
```

and verify:

* filter response;
* pole positions;
* cutoff calibration;
* coupling-pole behavior.

## Level 3: Filter large-signal

Use:

```text
Saw
Square
Resonance 25%, 50%, 75%, 100%
```

and verify:

* harmonic generation;
* resonance shape;
* low-frequency attenuation;
* nonlinear response.

## Level 4: Envelope

Use repeated identical notes with:

```text
Decay minimum
Decay maximum
```

and measure:

* attack;
* peak timing;
* decay;
* gate-off tail.

## Level 5: Accent

Compare:

```text
one accent
four repeated accents
```

at several Resonance values.

## Level 6: Slide

Compare:

```text
normal C → G
slid C → G
```

and inspect:

* pitch trajectory;
* gate continuity;
* envelope continuity;
* oscillator phase continuity.

---

# 97. Reference Test Sequences

Recommended automated test patterns:

```text
TEST 01:
C C C C C C C C
Saw, low resonance

TEST 02:
C C C C C C C C
Square, low resonance

TEST 03:
C C C C
Saw, high resonance

TEST 04:
C C C C
Accent every note

TEST 05:
C C C C
Accent every note, high Resonance

TEST 06:
C G C G
Slide every transition

TEST 07:
C G C G
No slide

TEST 08:
C C C C
Accent on notes 1,2,3,4

TEST 09:
C C C C
Accent 1 and 4 only

TEST 10:
low C → high C
Saw vs Square
high resonance
```

Each test should be compared both in:

```text
time domain
```

and:

```text
frequency domain
```

---

# 98. Golden Reference Measurements

For the best possible emulation, create a reference dataset from a known original machine.

Record simultaneously where possible:

```text
VCO waveform before filter
VCF input
VCF output
VCA output
final line output
gate
pitch CV
```

for:

```text
Saw
Square
low resonance
high resonance
minimum/maximum cutoff
minimum/maximum Env Mod
minimum/maximum Decay
Accent
Slide
```

A plugin should then be calibrated against the measured analogue behavior.

This is substantially more reliable than trying to infer every coefficient from a general "TB-303 sound" description.

---

# 99. Critical Audible Behaviors

A successful emulation should reproduce all of the following:

```text
1. The square is not a perfect square.
2. Square behavior changes with pitch.
3. Saw and square drive the filter differently.
4. The filter does not behave like a generic four-pole ladder.
5. Resonance changes more than peak Q.
6. Low-frequency resonance behaves differently from high-frequency resonance.
7. Env Mod acts through a nonlinear bias/current system.
8. Accent shortens the MEG.
9. Accent makes the VCA louder through an analogue control path.
10. Accent pushes the filter through a second control path.
11. The accent sweep has analogue memory.
12. Consecutive accents can rise progressively.
13. Resonance changes Accent Sweep behavior.
14. Slide extends the gate rather than merely gliding pitch.
15. Slide does not retrigger the envelopes.
16. Oscillator phase continues through note transitions.
17. VCA gate-off behavior is not an instantaneous mute.
18. Realistic nonlinearities occur throughout the voice.
19. Low-frequency coupling is part of the sound.
20. Small analogue imperfections remain stable and coherent.
```

---

# 100. Final Implementation Principle

The defining characteristic of a faithful TB-303 emulation is not one magic filter equation.

It is the **coupling of the analogue systems**:

```text
OSCILLATOR
    ↓
waveform-dependent drive
    ↓
NONLINEAR FILTER
    ↕
RESONANCE FEEDBACK
    ↕
CUTOFF / ENVELOPE / ACCENT CURRENT SUMMING
    ↕
ACCENT SWEEP MEMORY
    ↓
VCA
    ↕
VOLUME / ACCENT CONTROL
```

combined with:

```text
SEQUENCER TIMING
    ↓
GATE
    ↓
ENVELOPES

SEQUENCER TIMING
    ↓
SLIDE
    ↓
PITCH LAG
```

The most important design rule is therefore:

> **Model voltages, currents, capacitor states, gate states and feedback paths first; derive audible parameters second.**

A TB-303 emulator based on this principle will remain faithful when:

* cutoff and resonance interact,
* oscillator level changes,
* saw and square are switched,
* accents are repeated,
* slides occur,
* notes are shortened,
* cutoff is very low,
* resonance is very high,
* consecutive notes occur before envelope capacitors have fully discharged.

That behavior is what distinguishes an actual TB-303 model from a conventional subtractive synthesizer with a 303-style preset.

---

# Sources / Primary References

The principal hardware reference is the Roland TB-303 Service Notes, dated February 19, 1982, which documents the VCO, VCF, VCA, envelope circuitry, component values, calibration procedures, DAC/CV system and PCB/schematic information.

Robin Whittle's analysis of the original TB-303 documents the MEG/VEG architecture, Accent Sweep circuit, Resonance-pot dual function, 47 kΩ / 1 µF / 100 kΩ accent-sweep network, accent capacitor memory and the unusual interaction of Accent, Resonance and the filter.

Tim Stinchcombe's analysis of the TB-303 diode ladder is useful for understanding the four-pole coupled ladder, its non-coincident poles, nonlinear behavior and distinction from a conventional buffered Moog-style ladder.

The original circuit documentation and these analyses should be regarded as more authoritative than simplified commercial descriptions of the TB-303 filter as merely "an 18 dB filter".

