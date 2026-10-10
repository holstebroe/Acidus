# Acidus vs. hardware TB-303 calibration report (20261010-094954)

1 reference samples, 53 free parameters. Search: 1321 evaluations in 2.2 min, 0 CMA-ES restarts, stopped because: no progress for 120 s.

## Match quality (mean over samples)

All errors are dB; lower is better. `harm` = full-note harmonic levels (k^-0.5 weighted RMS), `inter` = energy between harmonics, `env` = RMS envelope, `stft` = 1/3-octave levels over time, `peak` = shape (height+width+implicit frequency) of the resonant-peak window on Resonance=max samples only -- weighted more heavily than `harm` so it can't be traded away.

| metric                                         | before | after  |
|------------------------------------------------|--------|--------|
| objective (incl. priors)                       | 4.22   | 1.66   |
| weighted error                                 | 4.21   | 1.62   |
| harmonic error (dB)                            | 2.27   | 0.78   |
| resonant-peak shape error (dB)                 | 2.90   | 0.28   |
| inter-harmonic error (dB)                      | 1.36   | 0.00   |
| envelope error (dB)                            | 4.05   | 4.59   |
| spectrogram error (dB)                         | 6.97   | 6.07   |
| aligned waveform error (10*sqrt(residual/ref)) | 0.00   | 0.00   |
| resonant-peak sweep track error (semitones)    | 6.20   | 1.73   |
| harmonics-over-time error (dB)                 | 6.47   | 4.02   |
| mean |harmonic error| (dB)                     | 1.33   | 0.67   |
| note level error, RMS over notes (dB)          | 0.05   | 1.09   |
| harmonics within 1 dB (%)                      | 47.01  | 69.23  |
| harmonics within 3 dB (%)                      | 93.16  | 100.00 |
| harmonics within 6 dB (%)                      | 100.00 | 100.00 |

Recording gain solved as -15.1 dB (before: -13.2 dB).

## Per set

Mean error of each set's notes, and the RMS of their level errors (the recordings are not normalised, so absolute note levels are compared under one global gain).

| set | notes | before | after | level before (dB) | level after (dB) | harm | env | stft |
|-----|-------|--------|-------|-------------------|------------------|------|-----|------|
| 1B  | 1     | 4.21   | 1.62  | 0.1               | 1.1              | 0.8  | 4.6 | 6.1  |

## Per sample (sorted by remaining error, worst first)

| sample | before | after | harm | peak | inter | env | stft | wave | harm ±3dB | level before | level after | tuning (c) | drift (c) |
|--------|--------|-------|------|------|-------|-----|------|------|-----------|--------------|-------------|------------|-----------|
| 1B-5   | 4.21   | 1.62  | 0.8  | 0.3  | 0.0   | 4.6 | 6.1  | 0.0  | 100%      | -0.1         | +1.1        | +0.0       | 0.00      |

### Resonant peak (Resonance=max samples)

| sample | peak Hz (hw) | peak Hz (before) | peak Hz (after) | height dB (hw) | height dB (before) | height dB (after) |
|--------|--------------|------------------|-----------------|----------------|--------------------|-------------------|
| 1B-5   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |

Height is the resonant peak's level above the fundamental (gain-independent). This is the number that answers "is the acid squelch there": before/after collapsing to something much smaller than hardware's means the peak got flattened away, not just shifted.

`tuning` is measured from the harmonic peaks relative to equal temperament; `drift` is the std-dev of short-time pitch over the note (hardware fluctuation).

### Worst remaining harmonics / spurious high-frequency content

- **1B-5** worst: k3 196Hz ref -8.2 / sim -6.5, k4 262Hz ref -10.4 / sim -8.8, k2 131Hz ref -4.9 / sim -3.4, k5 327Hz ref -11.9 / sim -10.5. Model excess above 4 kHz: none > 3 dB.

## Suspicious samples

No sample stands out: remaining errors are similar across samples and no free knob fit moved far from the labeled positions.

## Fitted knob positions

| knob (label)    | nominal | fitted | sensitivity |
|-----------------|---------|--------|-------------|
| accent (x0)     | 0.00    | 0.000  | 0.000       |
| cutoff (x100)   | 1.00    | 1.000  | 0.000       |
| decay (x50)     | 0.50    | 0.504  | 0.000       |
| envMod (x100)   | 1.00    | 1.000  | 0.000       |
| resonance (x74) | 0.75    | 0.748  | 0.000       |

## Fitted model parameters

`sensitivity` = objective increase for a 5% (of range) nudge. Near-zero means the current references don't constrain that parameter -- its value is not evidence of anything. `AT BOUND` means the optimizer pushed it to the edge of its plausible range, usually a sign that it is compensating for something the model does not implement.

| parameter              | before    | after     | sensitivity |                                     |
|------------------------|-----------|-----------|-------------|-------------------------------------|
| oscCouplingHz          | 47.24     | 42.622    | 0.000       |                                     |
| oscSawLpfHz            | 40000     | 26832     | 0.000       |                                     |
| oscSawShape            | 0         | 0.003136  | 0.000       |                                     |
| oscSquareDutyDepth     | 0.12      | 0.19344   | 0.000       |                                     |
| oscSquareLevel         | 0.70553   | 0.68162   | 0.000       |                                     |
| filterFeedbackGain     | 17.42     | 25.762    | 0.000       |                                     |
| filterResonanceLimit   | 0.91245   | 0.95745   | 0.000       |                                     |
| filterResonanceSkew    | -0.8718   | -0.65554  | 0.000       |                                     |
| resCouplingHz          | 104.29    | 112.36    | 0.000       |                                     |
| filterCapScale1        | 1.2895    | 1.7732    | 0.000       |                                     |
| filterCapScale2        | 0.69764   | 0.64054   | 0.000       |                                     |
| filterCapScale3        | 0.91275   | 1.2014    | 0.000       |                                     |
| filterCapScale4        | 1.0631    | 1.7043    | 0.000       |                                     |
| filterLadderInputScale | 0.035224  | 0.015334  | 0.000       |                                     |
| filterInputCouplingHz  | 6.0165    | 8.1378    | 0.000       |                                     |
| filterOutputCouplingHz | 20000     | 18958     | 0.000       |                                     |
| filterPostHpHz         | 198.89    | 116.9     | 0.000       |                                     |
| filterNotchHz          | 7.5164    | 6.4302    | 0.000       |                                     |
| filterNotchBandwidthHz | 4.7       | 5.4313    | 0.000       |                                     |
| filterAllpassHz        | 14.008    | 9.3377    | 0.000       |                                     |
| cutoffBaseHz           | 212.18    | 262.18    | 0.000       |                                     |
| cutoffSpanOct          | 3.3213    | 2.651     | 0.000       |                                     |
| cutoffTaperExp         | 1.4289    | 1.0365    | 0.000       |                                     |
| cutoffMaxHz            | 23723     | 23765     | 0.000       |                                     |
| envModScaleC0          | 0.76145   | 0.54832   | 0.000       |                                     |
| envModScaleC0Slope     | 2.8142    | 2.5669    | 0.000       |                                     |
| envModScaleC1          | 0.77794   | 0.98086   | 0.000       |                                     |
| envModScaleC1Slope     | 4.4796    | 2.6293    | 0.000       |                                     |
| envModOffset           | 0.34908   | 0.35439   | 0.000       |                                     |
| envModOffsetCutSlope   | -0.023862 | 0.0088031 | 0.000       |                                     |
| envModTaperExp         | 2         | 2.1991    | 0.000       |                                     |
| envModTaperMid         | 0.68612   | 0.6594    | 0.000       |                                     |
| envModTaperWidth       | 0.12193   | 0.10853   | 0.000       |                                     |
| accentSweepDepthOct    | 7.9269    | 8.1809    | 0.000       |                                     |
| accentVcaDepth         | 2.2843    | 1.5746    | 0.000       |                                     |
| accentChargeBaseSec    | 0.030579  | 0.019266  | 0.000       |                                     |
| accentChargePotSec     | 0.044021  | 0.020876  | 0.000       |                                     |
| accentMixSec           | 0.14498   | 0.12305   | 0.000       |                                     |
| accentDiodeDrop        | 0.30033   | 0.22369   | 0.000       |                                     |
| vcfAttackMs            | 0.1       | 0.070032  | 0.000       |                                     |
| vcaAttackMs            | 1.3011    | 1.4339    | 0.000       |                                     |
| vcaNormalDelayMs       | 4.504     | 3.6555    | 0.000       |                                     |
| vcfDecayMinSec         | 0.056061  | 0.056223  | 0.000       |                                     |
| vcfDecayMaxSec         | 1.1404    | 1.2534    | 0.000       |                                     |
| vcfDecayTaper          | 17.982    | 13.159    | 0.000       |                                     |
| accentDecaySec         | 0.075588  | 0.080469  | 0.000       |                                     |
| vegDecaySec            | 1.0379    | 1.0251    | 0.000       | AT BOUND (model may lack structure) |
| vcaGateOffMs           | 0.81135   | 1.4533    | 0.000       |                                     |
| vcaGateOffAccentMs     | 2.8037    | 1.9238    | 0.000       |                                     |
| vcaResTapRatio         | 1.0503    | 1.5398    | 0.000       |                                     |
| vcaGainSaturationDrive | 0         | 1.841     | 0.000       |                                     |

Timing: note-on offset 1.08 ms (relative to each clip's measured note-on), gate length 1296.9 ms.

## Applying the result

Paste `synth_parameters.txt` into `acidus::SynthParameters` (src/core/SynthEngine.hpp) or rerun with `--apply`.

Audio: `renders/` (hardware / before / after for the 1 worst samples).
