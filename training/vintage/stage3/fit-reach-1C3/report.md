# Acidus vs. hardware TB-303 calibration report (20261010-101133)

1 reference samples, 53 free parameters. Search: 1321 evaluations in 3.3 min, 0 CMA-ES restarts, stopped because: no progress for 120 s.

## Match quality (mean over samples)

All errors are dB; lower is better. `harm` = full-note harmonic levels (k^-0.5 weighted RMS), `inter` = energy between harmonics, `env` = RMS envelope, `stft` = 1/3-octave levels over time, `peak` = shape (height+width+implicit frequency) of the resonant-peak window on Resonance=max samples only -- weighted more heavily than `harm` so it can't be traded away.

| metric                                         | before | after  |
|------------------------------------------------|--------|--------|
| objective (incl. priors)                       | 4.72   | 1.43   |
| weighted error                                 | 4.71   | 1.43   |
| harmonic error (dB)                            | 5.32   | 0.85   |
| resonant-peak shape error (dB)                 | 2.89   | 0.66   |
| inter-harmonic error (dB)                      | 2.83   | 0.69   |
| envelope error (dB)                            | 7.10   | 2.97   |
| spectrogram error (dB)                         | 10.07  | 4.85   |
| aligned waveform error (10*sqrt(residual/ref)) | 0.00   | 0.00   |
| resonant-peak sweep track error (semitones)    | 4.53   | 1.33   |
| harmonics-over-time error (dB)                 | 8.84   | 3.34   |
| mean |harmonic error| (dB)                     | 4.62   | 0.90   |
| note level error, RMS over notes (dB)          | 6.58   | 0.30   |
| harmonics within 1 dB (%)                      | 11.02  | 62.20  |
| harmonics within 3 dB (%)                      | 33.07  | 100.00 |
| harmonics within 6 dB (%)                      | 76.38  | 100.00 |

Recording gain solved as -9.3 dB (before: -0.6 dB).

## Per set

Mean error of each set's notes, and the RMS of their level errors (the recordings are not normalised, so absolute note levels are compared under one global gain).

| set | notes | before | after | level before (dB) | level after (dB) | harm | env | stft |
|-----|-------|--------|-------|-------------------|------------------|------|-----|------|
| 1C  | 1     | 4.71   | 1.43  | 6.6               | 0.3              | 0.8  | 3.0 | 4.9  |

## Per sample (sorted by remaining error, worst first)

| sample | before | after | harm | peak | inter | env | stft | wave | harm ±3dB | level before | level after | tuning (c) | drift (c) |
|--------|--------|-------|------|------|-------|-----|------|------|-----------|--------------|-------------|------------|-----------|
| 1C-3   | 4.71   | 1.43  | 0.8  | 0.7  | 0.7   | 3.0 | 4.9  | 0.0  | 100%      | +6.6         | +0.3        | +0.0       | 0.00      |

### Resonant peak (Resonance=max samples)

| sample | peak Hz (hw) | peak Hz (before) | peak Hz (after) | height dB (hw) | height dB (before) | height dB (after) |
|--------|--------------|------------------|-----------------|----------------|--------------------|-------------------|
| 1C-3   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |

Height is the resonant peak's level above the fundamental (gain-independent). This is the number that answers "is the acid squelch there": before/after collapsing to something much smaller than hardware's means the peak got flattened away, not just shifted.

`tuning` is measured from the harmonic peaks relative to equal temperament; `drift` is the std-dev of short-time pitch over the note (hardware fluctuation).

### Worst remaining harmonics / spurious high-frequency content

- **1C-3** worst: k47 3074Hz ref -31.9 / sim -29.4, k48 3140Hz ref -33.3 / sim -30.9, k46 3009Hz ref -30.5 / sim -28.2, k49 3205Hz ref -34.5 / sim -32.4. Model excess above 4 kHz: none > 3 dB.

## Suspicious samples

No sample stands out: remaining errors are similar across samples and no free knob fit moved far from the labeled positions.

## Fitted knob positions

| knob (label)    | nominal | fitted | sensitivity |
|-----------------|---------|--------|-------------|
| accent (x0)     | 0.00    | 0.000  | 0.000       |
| cutoff (x25)    | 0.25    | 0.252  | 0.000       |
| decay (x100)    | 1.00    | 1.000  | 0.000       |
| envMod (x100)   | 1.00    | 1.000  | 0.000       |
| resonance (x74) | 0.75    | 0.748  | 0.000       |

## Fitted model parameters

`sensitivity` = objective increase for a 5% (of range) nudge. Near-zero means the current references don't constrain that parameter -- its value is not evidence of anything. `AT BOUND` means the optimizer pushed it to the edge of its plausible range, usually a sign that it is compensating for something the model does not implement.

| parameter              | before    | after    | sensitivity |                                     |
|------------------------|-----------|----------|-------------|-------------------------------------|
| oscCouplingHz          | 47.24     | 44.454   | 0.000       |                                     |
| oscSawLpfHz            | 40000     | 26997    | 0.000       |                                     |
| oscSawShape            | 0         | 0.29456  | 0.000       |                                     |
| oscSquareDutyDepth     | 0.12      | 0.078575 | 0.000       |                                     |
| oscSquareLevel         | 0.70553   | 0.98714  | 0.000       |                                     |
| filterFeedbackGain     | 17.42     | 17.365   | 0.000       |                                     |
| filterResonanceLimit   | 0.91245   | 0.90444  | 0.000       |                                     |
| filterResonanceSkew    | -0.8718   | 1.2994   | 0.000       |                                     |
| resCouplingHz          | 104.29    | 130.72   | 0.000       |                                     |
| filterCapScale1        | 1.2895    | 1.2556   | 0.000       |                                     |
| filterCapScale2        | 0.69764   | 0.81891  | 0.000       |                                     |
| filterCapScale3        | 0.91275   | 1.3138   | 0.000       |                                     |
| filterCapScale4        | 1.0631    | 1.059    | 0.000       |                                     |
| filterLadderInputScale | 0.035224  | 0.036419 | 0.000       |                                     |
| filterInputCouplingHz  | 6.0165    | 7.0685   | 0.000       |                                     |
| filterOutputCouplingHz | 20000     | 13457    | 0.000       |                                     |
| filterPostHpHz         | 198.89    | 149.05   | 0.000       |                                     |
| filterNotchHz          | 7.5164    | 5.2454   | 0.000       |                                     |
| filterNotchBandwidthHz | 4.7       | 4.8685   | 0.000       |                                     |
| filterAllpassHz        | 14.008    | 10.889   | 0.000       |                                     |
| cutoffBaseHz           | 212.18    | 356.21   | 0.000       |                                     |
| cutoffSpanOct          | 3.3213    | 2.0156   | 0.000       |                                     |
| cutoffTaperExp         | 1.4289    | 1.9609   | 0.000       |                                     |
| cutoffMaxHz            | 23723     | 20242    | 0.000       |                                     |
| envModScaleC0          | 0.76145   | 1.2555   | 0.000       |                                     |
| envModScaleC0Slope     | 2.8142    | 3.9384   | 0.000       |                                     |
| envModScaleC1          | 0.77794   | 0.71477  | 0.000       |                                     |
| envModScaleC1Slope     | 4.4796    | 4.9934   | 0.000       |                                     |
| envModOffset           | 0.34908   | 0.40543  | 0.000       |                                     |
| envModOffsetCutSlope   | -0.023862 | 0.12316  | 0.000       |                                     |
| envModTaperExp         | 2         | 1.6008   | 0.000       |                                     |
| envModTaperMid         | 0.68612   | 0.73789  | 0.000       |                                     |
| envModTaperWidth       | 0.12193   | 0.094997 | 0.000       |                                     |
| accentSweepDepthOct    | 7.9269    | 6.9399   | 0.000       |                                     |
| accentVcaDepth         | 2.2843    | 0.78409  | 0.000       |                                     |
| accentChargeBaseSec    | 0.030579  | 0.042952 | 0.000       |                                     |
| accentChargePotSec     | 0.044021  | 0.089943 | 0.000       |                                     |
| accentMixSec           | 0.14498   | 0.055237 | 0.000       |                                     |
| accentDiodeDrop        | 0.30033   | 0.26638  | 0.000       |                                     |
| vcfAttackMs            | 0.1       | 0.32027  | 0.000       |                                     |
| vcaAttackMs            | 1.3011    | 0.78567  | 0.000       |                                     |
| vcaNormalDelayMs       | 4.504     | 3.9883   | 0.000       |                                     |
| vcfDecayMinSec         | 0.056061  | 0.075077 | 0.000       |                                     |
| vcfDecayMaxSec         | 1.1404    | 1.0214   | 0.000       |                                     |
| vcfDecayTaper          | 17.982    | 75.069   | 0.000       |                                     |
| accentDecaySec         | 0.075588  | 0.10296  | 0.000       |                                     |
| vegDecaySec            | 1.0379    | 1.2713   | 0.000       |                                     |
| vcaGateOffMs           | 0.81135   | 0.76603  | 0.000       |                                     |
| vcaGateOffAccentMs     | 2.8037    | 1.4422   | 0.000       |                                     |
| vcaResTapRatio         | 1.0503    | 0.2299   | 0.000       |                                     |
| vcaGainSaturationDrive | 0         | 0.19863  | 0.000       | AT BOUND (model may lack structure) |

Timing: note-on offset 1.06 ms (relative to each clip's measured note-on), gate length 2504.7 ms.

## Applying the result

Paste `synth_parameters.txt` into `acidus::SynthParameters` (src/core/SynthEngine.hpp) or rerun with `--apply`.

Audio: `renders/` (hardware / before / after for the 1 worst samples).
