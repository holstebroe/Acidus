# Acidus vs. hardware TB-303 calibration report (20261010-095600)

1 reference samples, 53 free parameters. Search: 4756 evaluations in 6.0 min, 0 CMA-ES restarts, stopped because: time cap reached.

## Match quality (mean over samples)

All errors are dB; lower is better. `harm` = full-note harmonic levels (k^-0.5 weighted RMS), `inter` = energy between harmonics, `env` = RMS envelope, `stft` = 1/3-octave levels over time, `peak` = shape (height+width+implicit frequency) of the resonant-peak window on Resonance=max samples only -- weighted more heavily than `harm` so it can't be traded away.

| metric                                         | before | after |
|------------------------------------------------|--------|-------|
| objective (incl. priors)                       | 5.05   | 1.81  |
| weighted error                                 | 5.04   | 1.80  |
| harmonic error (dB)                            | 3.38   | 2.14  |
| resonant-peak shape error (dB)                 | 4.07   | 1.92  |
| inter-harmonic error (dB)                      | 1.35   | 3.29  |
| envelope error (dB)                            | 2.30   | 1.51  |
| spectrogram error (dB)                         | 8.68   | 4.06  |
| aligned waveform error (10*sqrt(residual/ref)) | 0.00   | 0.00  |
| resonant-peak sweep track error (semitones)    | 7.08   | 0.84  |
| harmonics-over-time error (dB)                 | 9.15   | 3.32  |
| mean |harmonic error| (dB)                     | 3.48   | 2.56  |
| note level error, RMS over notes (dB)          | 0.30   | 0.25  |
| harmonics within 1 dB (%)                      | 12.37  | 22.68 |
| harmonics within 3 dB (%)                      | 44.33  | 63.92 |
| harmonics within 6 dB (%)                      | 85.57  | 92.78 |

Recording gain solved as -12.2 dB (before: -12.1 dB).

## Per set

Mean error of each set's notes, and the RMS of their level errors (the recordings are not normalised, so absolute note levels are compared under one global gain).

| set | notes | before | after | level before (dB) | level after (dB) | harm | env | stft |
|-----|-------|--------|-------|-------------------|------------------|------|-----|------|
| 3A  | 1     | 5.04   | 1.80  | 0.3               | 0.2              | 2.1  | 1.5 | 4.1  |

## Per sample (sorted by remaining error, worst first)

| sample | before | after | harm | peak | inter | env | stft | wave | harm ±3dB | level before | level after | tuning (c) | drift (c) |
|--------|--------|-------|------|------|-------|-----|------|------|-----------|--------------|-------------|------------|-----------|
| 3A-15  | 5.04   | 1.80  | 2.1  | 1.9  | 3.3   | 1.5 | 4.1  | 0.0  | 64%       | +0.3         | +0.2        | +0.0       | 0.00      |

### Resonant peak (Resonance=max samples)

| sample | peak Hz (hw) | peak Hz (before) | peak Hz (after) | height dB (hw) | height dB (before) | height dB (after) |
|--------|--------------|------------------|-----------------|----------------|--------------------|-------------------|
| 3A-15  | 65           | 1635             | 65              | +0.0           | -19.5              | +0.0              |

Height is the resonant peak's level above the fundamental (gain-independent). This is the number that answers "is the acid squelch there": before/after collapsing to something much smaller than hardware's means the peak got flattened away, not just shifted.

`tuning` is measured from the harmonic peaks relative to equal temperament; `drift` is the std-dev of short-time pitch over the note (hardware fluctuation).

### Worst remaining harmonics / spurious high-frequency content

- **3A-15** worst: k77 5036Hz ref -68.1 / sim -74.9, k48 3140Hz ref -49.5 / sim -57.1, k50 3270Hz ref -51.5 / sim -59.9, k52 3401Hz ref -53.3 / sim -61.1. Model excess above 4 kHz: inter-harmonic 4415Hz +11.4dB, inter-harmonic 4284Hz +10.9dB, inter-harmonic 4546Hz +10.9dB, harmonic 4840Hz +4.4dB.

## Suspicious samples

No sample stands out: remaining errors are similar across samples and no free knob fit moved far from the labeled positions.

## Fitted knob positions

| knob (label)    | nominal | fitted | sensitivity |
|-----------------|---------|--------|-------------|
| accent (x100)   | 1.00    | 1.000  | 0.000       |
| cutoff (x50)    | 0.50    | 0.504  | 0.000       |
| decay (x50)     | 0.50    | 0.504  | 0.000       |
| envMod (x50)    | 0.50    | 0.504  | 0.000       |
| resonance (x74) | 0.75    | 0.748  | 0.000       |

## Fitted model parameters

`sensitivity` = objective increase for a 5% (of range) nudge. Near-zero means the current references don't constrain that parameter -- its value is not evidence of anything. `AT BOUND` means the optimizer pushed it to the edge of its plausible range, usually a sign that it is compensating for something the model does not implement.

| parameter              | before    | after     | sensitivity |                                     |
|------------------------|-----------|-----------|-------------|-------------------------------------|
| oscCouplingHz          | 47.24     | 45.922    | 0.000       |                                     |
| oscSawLpfHz            | 40000     | 11373     | 0.000       |                                     |
| oscSawShape            | 0         | -0.081539 | 0.000       |                                     |
| oscSquareDutyDepth     | 0.12      | 0.10033   | 0.000       |                                     |
| oscSquareLevel         | 0.70553   | 0.43389   | 0.000       |                                     |
| filterFeedbackGain     | 17.42     | 14.615    | 0.000       |                                     |
| filterResonanceLimit   | 0.91245   | 1.0242    | 0.000       |                                     |
| filterResonanceSkew    | -0.8718   | 5.743     | 0.000       |                                     |
| resCouplingHz          | 104.29    | 105.48    | 0.000       |                                     |
| filterCapScale1        | 1.2895    | 0.73083   | 0.000       |                                     |
| filterCapScale2        | 0.69764   | 1.3885    | 0.000       |                                     |
| filterCapScale3        | 0.91275   | 0.81296   | 0.000       |                                     |
| filterCapScale4        | 1.0631    | 0.88492   | 0.000       |                                     |
| filterLadderInputScale | 0.035224  | 0.07694   | 0.000       |                                     |
| filterInputCouplingHz  | 6.0165    | 17.8      | 0.000       |                                     |
| filterOutputCouplingHz | 20000     | 21815     | 0.000       |                                     |
| filterPostHpHz         | 198.89    | 109.57    | 0.000       |                                     |
| filterNotchHz          | 7.5164    | 14.542    | 0.000       |                                     |
| filterNotchBandwidthHz | 4.7       | 3.5658    | 0.000       |                                     |
| filterAllpassHz        | 14.008    | 7.7658    | 0.000       |                                     |
| cutoffBaseHz           | 212.18    | 437.02    | 0.000       |                                     |
| cutoffSpanOct          | 3.3213    | 3.2995    | 0.000       |                                     |
| cutoffTaperExp         | 1.4289    | 2.2945    | 0.000       |                                     |
| cutoffMaxHz            | 23723     | 14383     | 0.000       |                                     |
| envModScaleC0          | 0.76145   | 0.38006   | 0.000       |                                     |
| envModScaleC0Slope     | 2.8142    | 3.6698    | 0.000       |                                     |
| envModScaleC1          | 0.77794   | 1.0207    | 0.000       |                                     |
| envModScaleC1Slope     | 4.4796    | 4.6797    | 0.000       |                                     |
| envModOffset           | 0.34908   | 0.35217   | 0.000       |                                     |
| envModOffsetCutSlope   | -0.023862 | 0.20921   | 0.000       |                                     |
| envModTaperExp         | 2         | 3.2348    | 0.000       |                                     |
| envModTaperMid         | 0.68612   | 0.50957   | 0.000       |                                     |
| envModTaperWidth       | 0.12193   | 0.071759  | 0.000       |                                     |
| accentSweepDepthOct    | 7.9269    | 3.212     | 0.000       |                                     |
| accentVcaDepth         | 2.2843    | 3.8489    | 0.000       |                                     |
| accentChargeBaseSec    | 0.030579  | 0.011455  | 0.000       |                                     |
| accentChargePotSec     | 0.044021  | 0.076749  | 0.000       |                                     |
| accentMixSec           | 0.14498   | 0.065769  | 0.000       |                                     |
| accentDiodeDrop        | 0.30033   | 0.25812   | 0.000       |                                     |
| vcfAttackMs            | 0.1       | 0.24425   | 0.000       |                                     |
| vcaAttackMs            | 1.3011    | 0.83856   | 0.000       |                                     |
| vcaNormalDelayMs       | 4.504     | 7.5191    | 0.000       |                                     |
| vcfDecayMinSec         | 0.056061  | 0.06477   | 0.000       |                                     |
| vcfDecayMaxSec         | 1.1404    | 0.91534   | 0.000       |                                     |
| vcfDecayTaper          | 17.982    | 81.55     | 0.000       |                                     |
| accentDecaySec         | 0.075588  | 0.10423   | 0.000       |                                     |
| vegDecaySec            | 1.0379    | 1.2558    | 0.000       |                                     |
| vcaGateOffMs           | 0.81135   | 0.48882   | 0.000       |                                     |
| vcaGateOffAccentMs     | 2.8037    | 4.2228    | 0.000       |                                     |
| vcaResTapRatio         | 1.0503    | 1.7599    | 0.000       |                                     |
| vcaGainSaturationDrive | 0         | 0         | 0.000       | AT BOUND (model may lack structure) |

Timing: note-on offset 1.00 ms (relative to each clip's measured note-on), gate length 1303.6 ms.

## Applying the result

Paste `synth_parameters.txt` into `acidus::SynthParameters` (src/core/SynthEngine.hpp) or rerun with `--apply`.

Audio: `renders/` (hardware / before / after for the 1 worst samples).
