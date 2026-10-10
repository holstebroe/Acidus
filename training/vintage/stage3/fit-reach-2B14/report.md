# Acidus vs. hardware TB-303 calibration report (20261010-100812)

1 reference samples, 53 free parameters. Search: 3676 evaluations in 6.0 min, 0 CMA-ES restarts, stopped because: time cap reached.

## Match quality (mean over samples)

All errors are dB; lower is better. `harm` = full-note harmonic levels (k^-0.5 weighted RMS), `inter` = energy between harmonics, `env` = RMS envelope, `stft` = 1/3-octave levels over time, `peak` = shape (height+width+implicit frequency) of the resonant-peak window on Resonance=max samples only -- weighted more heavily than `harm` so it can't be traded away.

| metric                                         | before | after  |
|------------------------------------------------|--------|--------|
| objective (incl. priors)                       | 4.48   | 0.96   |
| weighted error                                 | 4.47   | 0.92   |
| harmonic error (dB)                            | 6.67   | 0.43   |
| resonant-peak shape error (dB)                 | 2.78   | 0.21   |
| inter-harmonic error (dB)                      | 4.40   | 0.14   |
| envelope error (dB)                            | 2.27   | 2.19   |
| spectrogram error (dB)                         | 8.12   | 5.95   |
| aligned waveform error (10*sqrt(residual/ref)) | 0.00   | 0.00   |
| resonant-peak sweep track error (semitones)    | 4.69   | 0.41   |
| harmonics-over-time error (dB)                 | 6.03   | 3.09   |
| mean |harmonic error| (dB)                     | 1.46   | 0.39   |
| note level error, RMS over notes (dB)          | 0.04   | 0.42   |
| harmonics within 1 dB (%)                      | 37.04  | 93.83  |
| harmonics within 3 dB (%)                      | 97.53  | 100.00 |
| harmonics within 6 dB (%)                      | 100.00 | 100.00 |

Recording gain solved as -13.0 dB (before: -13.1 dB).

## Per set

Mean error of each set's notes, and the RMS of their level errors (the recordings are not normalised, so absolute note levels are compared under one global gain).

| set | notes | before | after | level before (dB) | level after (dB) | harm | env | stft |
|-----|-------|--------|-------|-------------------|------------------|------|-----|------|
| 2B  | 1     | 4.47   | 0.92  | 0.0               | 0.4              | 0.4  | 2.2 | 5.9  |

## Per sample (sorted by remaining error, worst first)

| sample | before | after | harm | peak | inter | env | stft | wave | harm ±3dB | level before | level after | tuning (c) | drift (c) |
|--------|--------|-------|------|------|-------|-----|------|------|-----------|--------------|-------------|------------|-----------|
| 2B-14  | 4.47   | 0.92  | 0.4  | 0.2  | 0.1   | 2.2 | 5.9  | 0.0  | 100%      | -0.0         | +0.4        | +0.0       | 0.00      |

### Resonant peak (Resonance=max samples)

| sample | peak Hz (hw) | peak Hz (before) | peak Hz (after) | height dB (hw) | height dB (before) | height dB (after) |
|--------|--------------|------------------|-----------------|----------------|--------------------|-------------------|
| 2B-14  | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |

Height is the resonant peak's level above the fundamental (gain-independent). This is the number that answers "is the acid squelch there": before/after collapsing to something much smaller than hardware's means the peak got flattened away, not just shifted.

`tuning` is measured from the harmonic peaks relative to equal temperament; `drift` is the std-dev of short-time pitch over the note (hardware fluctuation).

### Worst remaining harmonics / spurious high-frequency content

- **2B-14** worst: k17 1112Hz ref -13.9 / sim -11.7, k18 1177Hz ref -14.1 / sim -12.4, k19 1243Hz ref -14.8 / sim -13.5, k16 1046Hz ref -14.4 / sim -13.3. Model excess above 4 kHz: none > 3 dB.

## Suspicious samples

No sample stands out: remaining errors are similar across samples and no free knob fit moved far from the labeled positions.

## Fitted knob positions

| knob (label)    | nominal | fitted | sensitivity |
|-----------------|---------|--------|-------------|
| accent (x0)     | 0.00    | 0.000  | 0.000       |
| cutoff (x100)   | 1.00    | 1.000  | 0.000       |
| decay (x50)     | 0.50    | 0.504  | 0.000       |
| envMod (x74)    | 0.75    | 0.748  | 0.000       |
| resonance (x74) | 0.75    | 0.748  | 0.000       |

## Fitted model parameters

`sensitivity` = objective increase for a 5% (of range) nudge. Near-zero means the current references don't constrain that parameter -- its value is not evidence of anything. `AT BOUND` means the optimizer pushed it to the edge of its plausible range, usually a sign that it is compensating for something the model does not implement.

| parameter              | before    | after    | sensitivity |                                     |
|------------------------|-----------|----------|-------------|-------------------------------------|
| oscCouplingHz          | 47.24     | 38.672   | 0.000       |                                     |
| oscSawLpfHz            | 40000     | 10093    | 0.000       |                                     |
| oscSawShape            | 0         | 0.060722 | 0.000       |                                     |
| oscSquareDutyDepth     | 0.12      | 0.018751 | 0.000       |                                     |
| oscSquareLevel         | 0.70553   | 0.79146  | 0.000       |                                     |
| filterFeedbackGain     | 17.42     | 25.207   | 0.000       |                                     |
| filterResonanceLimit   | 0.91245   | 0.9299   | 0.000       |                                     |
| filterResonanceSkew    | -0.8718   | 1.2961   | 0.000       |                                     |
| resCouplingHz          | 104.29    | 62.106   | 0.000       |                                     |
| filterCapScale1        | 1.2895    | 2.1052   | 0.000       |                                     |
| filterCapScale2        | 0.69764   | 0.79103  | 0.000       |                                     |
| filterCapScale3        | 0.91275   | 0.6761   | 0.000       |                                     |
| filterCapScale4        | 1.0631    | 0.85069  | 0.000       |                                     |
| filterLadderInputScale | 0.035224  | 0.027088 | 0.000       |                                     |
| filterInputCouplingHz  | 6.0165    | 3.3355   | 0.000       |                                     |
| filterOutputCouplingHz | 20000     | 14286    | 0.000       |                                     |
| filterPostHpHz         | 198.89    | 64.586   | 0.000       |                                     |
| filterNotchHz          | 7.5164    | 17.929   | 0.000       |                                     |
| filterNotchBandwidthHz | 4.7       | 3.8827   | 0.000       |                                     |
| filterAllpassHz        | 14.008    | 9.9133   | 0.000       |                                     |
| cutoffBaseHz           | 212.18    | 309.28   | 0.000       |                                     |
| cutoffSpanOct          | 3.3213    | 2.4384   | 0.000       |                                     |
| cutoffTaperExp         | 1.4289    | 1.4563   | 0.000       |                                     |
| cutoffMaxHz            | 23723     | 26979    | 0.000       |                                     |
| envModScaleC0          | 0.76145   | 1.0822   | 0.000       |                                     |
| envModScaleC0Slope     | 2.8142    | 3.7295   | 0.000       |                                     |
| envModScaleC1          | 0.77794   | 0.37042  | 0.000       |                                     |
| envModScaleC1Slope     | 4.4796    | 3.1142   | 0.000       |                                     |
| envModOffset           | 0.34908   | 0.42379  | 0.000       |                                     |
| envModOffsetCutSlope   | -0.023862 | -0.18823 | 0.000       |                                     |
| envModTaperExp         | 2         | 4        | 0.000       | AT BOUND (model may lack structure) |
| envModTaperMid         | 0.68612   | 0.64598  | 0.000       |                                     |
| envModTaperWidth       | 0.12193   | 0.2208   | 0.000       |                                     |
| accentSweepDepthOct    | 7.9269    | 7.6122   | 0.000       |                                     |
| accentVcaDepth         | 2.2843    | 1.0541   | 0.000       |                                     |
| accentChargeBaseSec    | 0.030579  | 0.018288 | 0.000       |                                     |
| accentChargePotSec     | 0.044021  | 0.050389 | 0.000       |                                     |
| accentMixSec           | 0.14498   | 0.044535 | 0.000       |                                     |
| accentDiodeDrop        | 0.30033   | 0.13475  | 0.000       |                                     |
| vcfAttackMs            | 0.1       | 0.024584 | 0.000       |                                     |
| vcaAttackMs            | 1.3011    | 1.1069   | 0.000       |                                     |
| vcaNormalDelayMs       | 4.504     | 1.8007   | 0.000       |                                     |
| vcfDecayMinSec         | 0.056061  | 0.07333  | 0.000       |                                     |
| vcfDecayMaxSec         | 1.1404    | 0.89024  | 0.000       |                                     |
| vcfDecayTaper          | 17.982    | 16.499   | 0.000       |                                     |
| accentDecaySec         | 0.075588  | 0.073139 | 0.000       |                                     |
| vegDecaySec            | 1.0379    | 1.0167   | 0.000       | AT BOUND (model may lack structure) |
| vcaGateOffMs           | 0.81135   | 9.5112   | 0.000       |                                     |
| vcaGateOffAccentMs     | 2.8037    | 20.032   | 0.000       |                                     |
| vcaResTapRatio         | 1.0503    | 0.36754  | 0.000       |                                     |
| vcaGainSaturationDrive | 0         | 0.088811 | 0.000       | AT BOUND (model may lack structure) |

Timing: note-on offset 3.38 ms (relative to each clip's measured note-on), gate length 1285.1 ms.

## Applying the result

Paste `synth_parameters.txt` into `acidus::SynthParameters` (src/core/SynthEngine.hpp) or rerun with `--apply`.

Audio: `renders/` (hardware / before / after for the 1 worst samples).
