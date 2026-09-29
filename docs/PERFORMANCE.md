# Performance profile (2026-09-29)

Measured with `acidus_perf_bench` (Release, -O3) and callgrind on a 2.1 GHz
Xeon cloud VM; a desktop CPU is typically 1.5-2x faster. Percentages are of
one core in real time. The DSP load is one mono voice playing a 125 BPM
16th-note acid line with accents and slides.

```bash
cmake --build build --target acidus_perf_bench
build/acidus_perf_bench                 # DSP + GUI rendering
xvfb-run build/acidus_perf_bench --gui-only --x11   # + a real X11 window (Linux)
```

## Results

| | Before | After |
|---|---|---|
| DSP, 48 kHz, typical settings | 7.5 us/sample, 36 % | 3.5 us/sample, 17 % |
| DSP, 48 kHz, with the Drive pedal | 10.0 us/sample, 48 % | 5.1 us/sample, 25 % |
| DSP, 96 kHz, with the Drive pedal | 94 % | 49 % |
| GUI frame render (software) | 5-6 ms | 0.25-0.45 ms |
| GUI thread, window open, X11 | 15.5 % | 2-2.6 % |
| GUI thread, window hidden | 15.5 % (kept animating) | 0.35 % |
| Windows GUI repaint rate | ~60 fps (16 ms timer) | ~30 fps |
| CLAP parameter event (sync) | 62 ns | ~70-100 ns |

Block size (32-1024 frames) makes no difference; the cost is per sample. The
bubble animation itself is negligible: updating 62 bubbles takes < 0.5 us.

## What was expensive, and what changed

1. **Filter critical-gain search (59 % of DSP time).**
   `Filter::criticalFeedbackGain()` ran a 48-step bisection (about 190 atan,
   48 atan2 and 50 exp calls) every sample. Its cache only hit while the
   cutoff was unchanged, and the envelopes move it every sample. It is now a
   1024-point table over log cutoff, rebuilt only when the ladder poles, the
   feedback coupling or the sample rate change (~5 ms, not per sample).
   `criticalFeedbackGainExact()` keeps the direct search. The table matches
   it within 1.4e-5.
2. **Filter coupling coefficients.** Two `exp()` calls per sample are now
   cached until a corner frequency or the rate changes (bit-identical).
3. **Drive pedal diode solver.** sinh and cosh of the same argument now come
   from one `expm1`. The 5-iteration Newton solve stops once the step is
   below float resolution. That halves the pedal's cost; the output matches
   to -137 dB (float rounding).
4. **GUI.** Every frame redrew the whole panel at 2x supersampling,
   including the ACIDUS lettering 25 times for its glow, although only the
   logo plate animates. Now:
   - the panel and knobs are cached and redrawn only when a knob value
     changes;
   - the lettering, screws and tagline are drawn once into an overlay;
   - each tick repaints, downsamples and uploads (X11) only the plate.

   Output is pixel-identical to the old renderer (checked on first frames
   and on cached vs. full redraws with bubbles). Hidden windows stop
   animating, and the Windows timer now matches the ~30 fps on Linux.

Sound and calibration are unchanged. The x0x evaluation on all 400 notes
scores exactly as before (weighted 2.61, harmonic 2.31 dB, sweep 3.34 st),
and so does `acidvoice` on its 13 samples (2.13). Renders differ from the old
code only near self-oscillation, by -58 to -72 dB relative to the signal.
Perturbing the old exact critical gain by one part in 10^7 gives the same
difference: that is the resonant loop's own sensitivity, not an error.

## What remains, and options

The remaining DSP cost is the model itself. The 8x-oversampled diode ladder
calls `tanh` 128 times per sample (4 stages x (1 + 3 Newton iterations) x
8), about 70 % of the synth without Drive. Options, all of which change
the sound slightly and would need a calibration re-check:

- a fast rational `tanh` approximation (typically 3-5x faster than libm's);
- a fixed internal rate (~350-400 kHz) instead of 8x the host rate, which
  halves the cost at 88.2/96 kHz;
- stopping the ladder's Newton iterations early when converged (it does
  not fully converge in 3 steps, so this is not bit-identical).
