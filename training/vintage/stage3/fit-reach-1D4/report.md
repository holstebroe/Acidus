# Acidus vs. hardware TB-303 calibration report (20261010-100206)

1 reference samples, 53 free parameters. Search: 4081 evaluations in 6.0 min, 0 CMA-ES restarts, stopped because: time cap reached.

## Match quality (mean over samples)

All errors are dB; lower is better. `harm` = full-note harmonic levels (k^-0.5 weighted RMS), `inter` = energy between harmonics, `env` = RMS envelope, `stft` = 1/3-octave levels over time, `peak` = shape (height+width+implicit frequency) of the resonant-peak window on Resonance=max samples only -- weighted more heavily than `harm` so it can't be traded away.

| metric                                         | before | after  |
|------------------------------------------------|--------|--------|
| objective (incl. priors)                       | 4.49   | 1.55   |
| weighted error                                 | 4.48   | 1.53   |
| harmonic error (dB)                            | 1.86   | 0.81   |
| resonant-peak shape error (dB)                 | 4.11   | 0.90   |
| inter-harmonic error (dB)                      | 0.69   | 1.84   |
| envelope error (dB)                            | 3.23   | 2.04   |
| spectrogram error (dB)                         | 11.51  | 5.36   |
| aligned waveform error (10*sqrt(residual/ref)) | 0.00   | 0.00   |
| resonant-peak sweep track error (semitones)    | 5.17   | 1.40   |
| harmonics-over-time error (dB)                 | 10.34  | 4.12   |
| mean |harmonic error| (dB)                     | 2.01   | 1.06   |
| note level error, RMS over notes (dB)          | 0.73   | 0.17   |
| harmonics within 1 dB (%)                      | 36.23  | 53.62  |
| harmonics within 3 dB (%)                      | 71.01  | 100.00 |
| harmonics within 6 dB (%)                      | 98.55  | 100.00 |

Recording gain solved as -9.6 dB (before: -8.5 dB).

## Per set

Mean error of each set's notes, and the RMS of their level errors (the recordings are not normalised, so absolute note levels are compared under one global gain).

| set | notes | before | after | level before (dB) | level after (dB) | harm | env | stft |
|-----|-------|--------|-------|-------------------|------------------|------|-----|------|
| 1D  | 1     | 4.48   | 1.53  | 0.7               | 0.2              | 0.8  | 2.0 | 5.4  |

## Per sample (sorted by remaining error, worst first)

| sample | before | after | harm | peak | inter | env | stft | wave | harm ±3dB | level before | level after | tuning (c) | drift (c) |
|--------|--------|-------|------|------|-------|-----|------|------|-----------|--------------|-------------|------------|-----------|
| 1D-4   | 4.48   | 1.53  | 0.8  | 0.9  | 1.8   | 2.0 | 5.4  | 0.0  | 100%      | +0.7         | -0.2        | +0.0       | 0.00      |

### Resonant peak (Resonance=max samples)

| sample | peak Hz (hw) | peak Hz (before) | peak Hz (after) | height dB (hw) | height dB (before) | height dB (after) |
|--------|--------------|------------------|-----------------|----------------|--------------------|-------------------|
| 1D-4   | 981          | 14259            | 981             | -6.4           | -140.6             | -6.1              |

Height is the resonant peak's level above the fundamental (gain-independent). This is the number that answers "is the acid squelch there": before/after collapsing to something much smaller than hardware's means the peak got flattened away, not just shifted.

`tuning` is measured from the harmonic peaks relative to equal temperament; `drift` is the std-dev of short-time pitch over the note (hardware fluctuation).

### Worst remaining harmonics / spurious high-frequency content

- **1D-4** worst: k69 4513Hz ref -79.0 / sim -76.5, k70 4578Hz ref -79.6 / sim -77.1, k67 4382Hz ref -77.7 / sim -75.4, k47 3074Hz ref -60.3 / sim -62.6. Model excess above 4 kHz: none > 3 dB.

## Suspicious samples

No sample stands out: remaining errors are similar across samples and no free knob fit moved far from the labeled positions.

## Fitted knob positions

| knob (label)     | nominal | fitted | sensitivity |
|------------------|---------|--------|-------------|
| accent (x100)    | 1.00    | 1.000  | 0.000       |
| cutoff (x25)     | 0.25    | 0.252  | 0.000       |
| decay (x50)      | 0.50    | 0.504  | 0.000       |
| envMod (x50)     | 0.50    | 0.504  | 0.000       |
| resonance (x100) | 1.00    | 1.000  | 0.000       |

## Fitted model parameters

`sensitivity` = objective increase for a 5% (of range) nudge. Near-zero means the current references don't constrain that parameter -- its value is not evidence of anything. `AT BOUND` means the optimizer pushed it to the edge of its plausible range, usually a sign that it is compensating for something the model does not implement.

| parameter              | before    | after    | sensitivity |                                     |
|------------------------|-----------|----------|-------------|-------------------------------------|
| oscCouplingHz          | 47.24     | 55.043   | 0.000       |                                     |
| oscSawLpfHz            | 40000     | 32268    | 0.000       |                                     |
| oscSawShape            | 0         | -0.4     | 0.000       | AT BOUND (model may lack structure) |
| oscSquareDutyDepth     | 0.12      | 0.020213 | 0.000       |                                     |
| oscSquareLevel         | 0.70553   | 0.4555   | 0.000       |                                     |
| filterFeedbackGain     | 17.42     | 20.33    | 0.000       |                                     |
| filterResonanceLimit   | 0.91245   | 0.95128  | 0.000       |                                     |
| filterResonanceSkew    | -0.8718   | 0.87512  | 0.000       |                                     |
| resCouplingHz          | 104.29    | 157.51   | 0.000       | AT BOUND (model may lack structure) |
| filterCapScale1        | 1.2895    | 2.0993   | 0.000       |                                     |
| filterCapScale2        | 0.69764   | 0.94241  | 0.000       |                                     |
| filterCapScale3        | 0.91275   | 2.0242   | 0.000       |                                     |
| filterCapScale4        | 1.0631    | 1.9138   | 0.000       |                                     |
| filterLadderInputScale | 0.035224  | 0.022533 | 0.000       |                                     |
| filterInputCouplingHz  | 6.0165    | 5.7341   | 0.000       |                                     |
| filterOutputCouplingHz | 20000     | 37609    | 0.000       |                                     |
| filterPostHpHz         | 198.89    | 251.89   | 0.000       |                                     |
| filterNotchHz          | 7.5164    | 4.6311   | 0.000       |                                     |
| filterNotchBandwidthHz | 4.7       | 9.1291   | 0.000       |                                     |
| filterAllpassHz        | 14.008    | 17.037   | 0.000       |                                     |
| cutoffBaseHz           | 212.18    | 124.83   | 0.000       |                                     |
| cutoffSpanOct          | 3.3213    | 3.684    | 0.000       |                                     |
| cutoffTaperExp         | 1.4289    | 3.0278   | 0.000       |                                     |
| cutoffMaxHz            | 23723     | 13407    | 0.000       |                                     |
| envModScaleC0          | 0.76145   | 1.2542   | 0.000       |                                     |
| envModScaleC0Slope     | 2.8142    | 2.7289   | 0.000       |                                     |
| envModScaleC1          | 0.77794   | 1.0346   | 0.000       |                                     |
| envModScaleC1Slope     | 4.4796    | 3.8453   | 0.000       |                                     |
| envModOffset           | 0.34908   | 0.32196  | 0.000       |                                     |
| envModOffsetCutSlope   | -0.023862 | -0.1142  | 0.000       |                                     |
| envModTaperExp         | 2         | 3.4584   | 0.000       |                                     |
| envModTaperMid         | 0.68612   | 0.44313  | 0.000       |                                     |
| envModTaperWidth       | 0.12193   | 0.28885  | 0.000       |                                     |
| accentSweepDepthOct    | 7.9269    | 3.1653   | 0.000       |                                     |
| accentVcaDepth         | 2.2843    | 2.3583   | 0.000       |                                     |
| accentChargeBaseSec    | 0.030579  | 0.026215 | 0.000       |                                     |
| accentChargePotSec     | 0.044021  | 0.036157 | 0.000       |                                     |
| accentMixSec           | 0.14498   | 0.063756 | 0.000       |                                     |
| accentDiodeDrop        | 0.30033   | 0.20875  | 0.000       |                                     |
| vcfAttackMs            | 0.1       | 0.033142 | 0.000       |                                     |
| vcaAttackMs            | 1.3011    | 1.1812   | 0.000       |                                     |
| vcaNormalDelayMs       | 4.504     | 1.5368   | 0.000       |                                     |
| vcfDecayMinSec         | 0.056061  | 0.076614 | 0.000       |                                     |
| vcfDecayMaxSec         | 1.1404    | 1.3358   | 0.000       |                                     |
| vcfDecayTaper          | 17.982    | 1.6645   | 0.000       |                                     |
| accentDecaySec         | 0.075588  | 0.12839  | 0.000       |                                     |
| vegDecaySec            | 1.0379    | 2.1988   | 0.000       |                                     |
| vcaGateOffMs           | 0.81135   | 3.5677   | 0.000       |                                     |
| vcaGateOffAccentMs     | 2.8037    | 1.7499   | 0.000       |                                     |
| vcaResTapRatio         | 1.0503    | 2.0803   | 0.000       |                                     |
| vcaGainSaturationDrive | 0         | 0        | 0.000       | AT BOUND (model may lack structure) |

Timing: note-on offset 1.33 ms (relative to each clip's measured note-on), gate length 1301.8 ms.

## Applying the result

Paste `synth_parameters.txt` into `acidus::SynthParameters` (src/core/SynthEngine.hpp) or rerun with `--apply`.

Audio: `renders/` (hardware / before / after for the 1 worst samples).
