# Acidus vs. hardware TB-303 calibration report (20261010-183223)

23 reference samples, 19 free parameters. Search: 1002 evaluations in 30.1 min, 0 CMA-ES restarts, stopped because: time cap reached.

## Match quality (mean over samples)

All errors are dB; lower is better. `harm` = full-note harmonic levels (k^-0.5 weighted RMS), `inter` = energy between harmonics, `env` = RMS envelope, `stft` = 1/3-octave levels over time, `peak` = shape (height+width+implicit frequency) of the resonant-peak window on Resonance=max samples only -- weighted more heavily than `harm` so it can't be traded away.

| metric                                         | before | after |
|------------------------------------------------|--------|-------|
| objective (incl. priors)                       | 2.89   | 2.27  |
| weighted error                                 | 2.89   | 2.27  |
| harmonic error (dB)                            | 2.62   | 1.85  |
| resonant-peak shape error (dB)                 | 2.68   | 2.11  |
| inter-harmonic error (dB)                      | 1.13   | 0.62  |
| envelope error (dB)                            | 4.36   | 3.56  |
| spectrogram error (dB)                         | 8.47   | 7.17  |
| aligned waveform error (10*sqrt(residual/ref)) | 0.00   | 0.00  |
| resonant-peak sweep track error (semitones)    | 3.24   | 2.49  |
| harmonics-over-time error (dB)                 | 7.50   | 5.95  |
| mean |harmonic error| (dB)                     | 3.07   | 2.21  |
| note level error, RMS over notes (dB)          | 4.08   | 2.76  |
| harmonics within 1 dB (%)                      | 24.70  | 32.46 |
| harmonics within 3 dB (%)                      | 56.48  | 74.70 |
| harmonics within 6 dB (%)                      | 87.21  | 94.52 |

Recording gain solved as -10.3 dB (before: -8.0 dB).

## Per set

Mean error of each set's notes, and the RMS of their level errors (the recordings are not normalised, so absolute note levels are compared under one global gain).

| set | notes | before | after | level before (dB) | level after (dB) | harm | env | stft |
|-----|-------|--------|-------|-------------------|------------------|------|-----|------|
| 1A  | 9     | 2.69   | 2.28  | 5.3               | 3.4              | 2.0  | 3.6 | 6.8  |
| 1B  | 5     | 3.59   | 2.66  | 3.1               | 2.0              | 1.5  | 3.7 | 7.8  |
| 1C  | 3     | 3.02   | 2.13  | 0.8               | 2.4              | 1.7  | 4.0 | 6.4  |
| 1D  | 3     | 3.16   | 2.48  | 1.4               | 0.5              | 1.7  | 3.0 | 8.2  |
| 1E  | 2     | 1.49   | 1.26  | 5.4               | 3.6              | 2.5  | 3.6 | 6.7  |
| 1R  | 1     | 2.84   | 1.95  | 3.7               | 2.5              | 1.3  | 2.9 | 7.7  |

## Per sample (sorted by remaining error, worst first)

| sample | before | after | harm | peak | inter | env | stft | wave | harm ±3dB | level before | level after | tuning (c) | drift (c) |
|--------|--------|-------|------|------|-------|-----|------|------|-----------|--------------|-------------|------------|-----------|
| 1A-6   | 3.65   | 4.05  | 1.9  | 3.5  | 0.9   | 4.1 | 8.5  | 0.0  | 68%       | +3.4         | +1.7        | +0.0       | 0.00      |
| 1D-4   | 4.71   | 3.60  | 1.2  | 2.0  | 0.4   | 5.1 | 10.1 | 0.0  | 90%       | +1.3         | -0.8        | +0.0       | 0.00      |
| 1A-3   | 3.81   | 3.27  | 2.2  | 2.3  | 0.0   | 4.4 | 6.8  | 0.0  | 35%       | +0.5         | -0.5        | +0.0       | 0.00      |
| 1A-8   | 3.88   | 3.23  | 2.3  | 2.8  | 1.9   | 3.5 | 7.3  | 0.0  | 87%       | +6.1         | +3.5        | +0.0       | 0.00      |
| 1B-5   | 4.99   | 3.00  | 1.9  | 2.9  | 0.9   | 3.1 | 7.1  | 0.0  | 95%       | +4.7         | +2.4        | +0.0       | 0.00      |
| 1B-1   | 3.09   | 2.93  | 1.7  | 1.9  | 0.3   | 2.6 | 9.1  | 0.0  | 65%       | +3.2         | +1.8        | +0.0       | 0.00      |
| 1A-5   | 3.33   | 2.89  | 1.4  | 1.5  | 0.0   | 2.9 | 8.9  | 0.0  | 89%       | +3.8         | +2.6        | +0.0       | 0.00      |
| 1D-3   | 3.58   | 2.89  | 1.5  | 1.7  | 0.3   | 2.0 | 9.2  | 0.0  | 73%       | +1.9         | +0.4        | +0.0       | 0.00      |
| 1B-2   | 3.26   | 2.78  | 1.3  | 2.1  | 0.3   | 2.8 | 9.3  | 0.0  | 94%       | +3.0         | +1.5        | +0.0       | 0.00      |
| 1B-4   | 3.39   | 2.46  | 1.6  | 2.3  | 1.0   | 7.4 | 5.6  | 0.0  | 87%       | -1.4         | -3.0        | +0.0       | 0.00      |
| 1C-3   | 4.16   | 2.38  | 3.0  | 2.6  | 0.0   | 2.6 | 5.7  | 0.0  | 67%       | -1.2         | -3.1        | +0.0       | 0.00      |
| 1C-2   | 2.71   | 2.20  | 1.4  | 2.2  | 1.1   | 4.6 | 7.3  | 0.0  | 95%       | +0.5         | -1.3        | +0.0       | 0.00      |
| 1B-3   | 3.24   | 2.14  | 1.2  | 2.1  | 0.0   | 2.4 | 7.9  | 0.0  | 89%       | +2.0         | +0.2        | +0.0       | 0.00      |
| 1R-1   | 2.84   | 1.95  | 1.3  | 1.4  | 0.3   | 2.9 | 7.7  | 0.0  | 90%       | +3.7         | +2.5        | +0.0       | 0.00      |
| 1A-9   | 2.43   | 1.89  | 1.9  | 2.3  | 2.1   | 1.1 | 4.7  | 0.0  | 87%       | +2.7         | +0.2        | +0.0       | 0.00      |
| 1A-7   | 2.87   | 1.81  | 3.4  | -    | 2.4   | 7.6 | 7.2  | 0.0  | 90%       | +12.1        | +7.5        | +0.0       | 0.00      |
| 1C-1   | 2.19   | 1.81  | 0.8  | 1.5  | 0.2   | 4.9 | 6.0  | 0.0  | 100%      | -0.3         | -2.5        | +0.0       | 0.00      |
| 1E-1   | 1.92   | 1.66  | 4.0  | -    | 2.0   | 4.2 | 7.5  | 0.0  | 27%       | +6.3         | +4.3        | +0.0       | 0.00      |
| 1A-2   | 1.82   | 1.43  | 1.9  | 0.9  | 0.0   | 1.5 | 6.9  | 0.0  | 34%       | -1.0         | -1.2        | +0.0       | 0.00      |
| 1A-1   | 1.32   | 1.18  | 2.3  | -    | 0.0   | 4.5 | 5.5  | 0.0  | 32%       | -4.4         | -4.2        | +0.0       | 0.00      |
| 1D-1   | 1.17   | 0.95  | 2.4  | -    | 0.0   | 1.8 | 5.3  | 0.0  | 24%       | +0.9         | -0.1        | +0.0       | 0.00      |
| 1E-2   | 1.06   | 0.86  | 0.9  | -    | 0.2   | 2.9 | 5.8  | 0.0  | 100%      | +4.3         | +2.6        | +0.0       | 0.00      |
| 1A-4   | 1.07   | 0.82  | 1.1  | -    | 0.0   | 2.8 | 5.3  | 0.0  | 100%      | +4.4         | +2.7        | +0.0       | 0.00      |

### Resonant peak (Resonance=max samples)

| sample | peak Hz (hw) | peak Hz (before) | peak Hz (after) | height dB (hw) | height dB (before) | height dB (after) |
|--------|--------------|------------------|-----------------|----------------|--------------------|-------------------|
| 1A-2   | 131          | 65               | 65              | -1.7           | +0.0               | +0.0              |
| 1A-3   | 196          | 196              | 196             | -0.7           | +0.2               | -0.1              |
| 1A-5   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1A-6   | 327          | 458              | 15305           | -2.4           | -0.9               | -125.9            |
| 1A-8   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1A-9   | 1701         | 1701             | 1831            | -5.4           | -3.6               | -8.7              |
| 1B-1   | 65           | 392              | 458             | +0.0           | -4.2               | -6.5              |
| 1B-2   | 65           | 327              | 65              | +0.0           | -4.2               | +0.0              |
| 1B-3   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1B-4   | 65           | 12623            | 65              | +0.0           | -137.9             | +0.0              |
| 1B-5   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1C-1   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1C-2   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1C-3   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1D-3   | 65           | 262              | 262             | +0.0           | -1.1               | -2.4              |
| 1D-4   | 981          | 14259            | 15567           | -6.8           | -140.6             | -137.0            |
| 1R-1   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |

Height is the resonant peak's level above the fundamental (gain-independent). This is the number that answers "is the acid squelch there": before/after collapsing to something much smaller than hardware's means the peak got flattened away, not just shifted.

`tuning` is measured from the harmonic peaks relative to equal temperament; `drift` is the std-dev of short-time pitch over the note (hardware fluctuation).

### Worst remaining harmonics / spurious high-frequency content

- **1A-1** worst: k19 1243Hz ref -69.3 / sim -77.6, k18 1177Hz ref -67.5 / sim -75.5, k20 1308Hz ref -71.3 / sim -79.5, k17 1112Hz ref -65.8 / sim -73.2. Model excess above 4 kHz: none > 3 dB.
- **1A-2** worst: k25 1635Hz ref -74.3 / sim -81.3, k21 1374Hz ref -67.9 / sim -74.2, k20 1308Hz ref -66.0 / sim -72.2, k22 1439Hz ref -69.5 / sim -76.1. Model excess above 4 kHz: none > 3 dB.
- **1A-3** worst: k24 1570Hz ref -67.2 / sim -74.4, k25 1635Hz ref -68.9 / sim -76.1, k23 1504Hz ref -65.7 / sim -72.7, k16 1046Hz ref -51.3 / sim -58.1. Model excess above 4 kHz: none > 3 dB.
- **1A-4** worst: k1 65Hz ref 0.0 / sim 2.9, k39 2551Hz ref -76.8 / sim -79.7, k38 2485Hz ref -75.9 / sim -78.6, k2 131Hz ref -4.7 / sim -2.0. Model excess above 4 kHz: none > 3 dB.
- **1A-5** worst: k6 392Hz ref -12.5 / sim -8.1, k5 327Hz ref -9.2 / sim -5.3, k7 458Hz ref -15.4 / sim -11.6, k3 196Hz ref -5.6 / sim -2.4. Model excess above 4 kHz: none > 3 dB.
- **1A-6** worst: k7 458Hz ref -8.4 / sim -1.5, k16 1046Hz ref -38.3 / sim -33.0, k8 523Hz ref -10.6 / sim -5.4, k17 1112Hz ref -40.7 / sim -35.4. Model excess above 4 kHz: none > 3 dB.
- **1A-7** worst: k3 196Hz ref -8.4 / sim 2.1, k4 262Hz ref -10.8 / sim -0.7, k2 131Hz ref -5.0 / sim 4.6, k5 327Hz ref -12.6 / sim -3.4. Model excess above 4 kHz: none > 3 dB.
- **1A-8** worst: k5 327Hz ref -12.0 / sim -5.9, k6 392Hz ref -13.1 / sim -7.1, k4 262Hz ref -10.4 / sim -4.5, k7 458Hz ref -13.9 / sim -8.1. Model excess above 4 kHz: none > 3 dB.
- **1A-9** worst: k26 1701Hz ref -5.4 / sim -9.8, k5 327Hz ref -12.2 / sim -8.5, k25 1635Hz ref -7.3 / sim -11.0, k6 392Hz ref -13.4 / sim -9.7. Model excess above 4 kHz: none > 3 dB.
- **1B-1** worst: k7 458Hz ref -10.1 / sim -6.2, k6 392Hz ref -8.7 / sim -4.9, k49 3205Hz ref -78.3 / sim -74.6, k48 3140Hz ref -77.4 / sim -73.7. Model excess above 4 kHz: none > 3 dB.
- **1B-2** worst: k6 392Hz ref -10.0 / sim -6.2, k5 327Hz ref -8.6 / sim -4.9, k7 458Hz ref -11.1 / sim -7.9, k16 1046Hz ref -21.8 / sim -24.9. Model excess above 4 kHz: none > 3 dB.
- **1B-3** worst: k35 2289Hz ref -24.5 / sim -28.2, k34 2224Hz ref -24.2 / sim -27.9, k36 2355Hz ref -25.3 / sim -28.6, k29 1897Hz ref -22.7 / sim -26.0. Model excess above 4 kHz: none > 3 dB.
- **1B-4** worst: k16 1046Hz ref -18.9 / sim -23.4, k18 1177Hz ref -20.3 / sim -24.7, k17 1112Hz ref -19.8 / sim -24.0, k1 65Hz ref 0.0 / sim -4.2. Model excess above 4 kHz: none > 3 dB.
- **1B-5** worst: k9 589Hz ref -14.9 / sim -9.8, k8 523Hz ref -14.5 / sim -9.4, k7 458Hz ref -13.9 / sim -8.8, k6 392Hz ref -13.1 / sim -8.0. Model excess above 4 kHz: none > 3 dB.
- **1C-1** worst: k1 65Hz ref 0.0 / sim -3.0, k8 523Hz ref -19.1 / sim -21.3, k7 458Hz ref -17.4 / sim -19.6, k2 131Hz ref -6.2 / sim -8.3. Model excess above 4 kHz: none > 3 dB.
- **1C-2** worst: k21 1374Hz ref -20.7 / sim -24.7, k20 1308Hz ref -20.3 / sim -24.1, k22 1439Hz ref -21.7 / sim -25.2, k19 1243Hz ref -20.5 / sim -23.6. Model excess above 4 kHz: none > 3 dB.
- **1C-3** worst: k38 2485Hz ref -22.1 / sim -29.6, k39 2551Hz ref -22.6 / sim -30.1, k37 2420Hz ref -21.7 / sim -29.2, k36 2355Hz ref -21.4 / sim -28.7. Model excess above 4 kHz: none > 3 dB.
- **1D-1** worst: k34 2224Hz ref -72.8 / sim -79.0, k33 2158Hz ref -71.6 / sim -77.9, k35 2289Hz ref -73.8 / sim -80.2, k32 2093Hz ref -70.5 / sim -76.7. Model excess above 4 kHz: none > 3 dB.
- **1D-3** worst: k27 1766Hz ref -52.9 / sim -57.9, k28 1831Hz ref -54.4 / sim -59.4, k29 1897Hz ref -55.8 / sim -60.7, k26 1701Hz ref -51.6 / sim -56.4. Model excess above 4 kHz: none > 3 dB.
- **1D-4** worst: k44 2878Hz ref -55.5 / sim -61.0, k43 2812Hz ref -55.1 / sim -59.0, k45 2943Hz ref -56.4 / sim -62.9, k15 981Hz ref -6.8 / sim -12.6. Model excess above 4 kHz: none > 3 dB.
- **1E-1** worst: k29 1897Hz ref -78.5 / sim -65.1, k27 1766Hz ref -75.1 / sim -62.9, k15 981Hz ref -48.0 / sim -63.9, k25 1635Hz ref -71.1 / sim -60.9. Model excess above 4 kHz: none > 3 dB.
- **1E-2** worst: k1 65Hz ref 0.0 / sim 2.8, k2 131Hz ref -4.7 / sim -2.1, k3 196Hz ref -8.9 / sim -7.1, k6 392Hz ref -21.4 / sim -19.8. Model excess above 4 kHz: none > 3 dB.
- **1R-1** worst: k6 392Hz ref -12.5 / sim -8.2, k5 327Hz ref -9.2 / sim -5.3, k7 458Hz ref -15.3 / sim -11.7, k3 196Hz ref -5.6 / sim -2.4. Model excess above 4 kHz: none > 3 dB.

## Suspicious samples

- **1D-1**: spectrum is within 0.9 dB of 1E-2 although the cutoff/envMod/decay/accent label differs -- one of the two labels is probably wrong
- **1A-4**: spectrum is within 1.2 dB of 1D-1 although the cutoff/envMod/decay/accent label differs -- one of the two labels is probably wrong
- **1A-6**: error 4.05 is 1.8x the median
- **1D-4**: error 3.60 is 1.6x the median

## Fitted knob positions

| knob (label)     | nominal | fitted | sensitivity |
|------------------|---------|--------|-------------|
| accent (x0)      | 0.00    | 0.000  | 0.000       |
| accent (x100)    | 1.00    | 1.000  | 0.000       |
| cutoff (x0)      | 0.00    | 0.000  | 0.000       |
| cutoff (x100)    | 1.00    | 1.000  | 0.000       |
| cutoff (x25)     | 0.25    | 0.252  | 0.000       |
| cutoff (x50)     | 0.50    | 0.504  | 0.000       |
| decay (x0)       | 0.00    | 0.000  | 0.000       |
| decay (x100)     | 1.00    | 1.000  | 0.000       |
| decay (x50)      | 0.50    | 0.504  | 0.000       |
| envMod (x0)      | 0.00    | 0.000  | 0.000       |
| envMod (x100)    | 1.00    | 1.000  | 0.000       |
| envMod (x50)     | 0.50    | 0.504  | 0.000       |
| resonance (x0)   | 0.00    | 0.000  | 0.000       |
| resonance (x100) | 1.00    | 1.000  | 0.000       |
| resonance (x50)  | 0.50    | 0.504  | 0.000       |
| resonance (x74)  | 0.75    | 0.748  | 0.000       |

Cutoff law with the fitted end stops folded in (so the plugin's knob range equals the hardware's): `cutoffBaseHz = 240.4`, `cutoffSpanOct = 3.295`, `cutoffTaperExp = 1.429`.

## Fitted model parameters

`sensitivity` = objective increase for a 5% (of range) nudge. Near-zero means the current references don't constrain that parameter -- its value is not evidence of anything. `AT BOUND` means the optimizer pushed it to the edge of its plausible range, usually a sign that it is compensating for something the model does not implement.

| parameter            | before   | after    | sensitivity |                                     |
|----------------------|----------|----------|-------------|-------------------------------------|
| oscSquareLevel       | 0.70553  | 0.68211  | 0.000       |                                     |
| filterFeedbackGain   | 17.42    | 15.128   | 0.000       |                                     |
| filterResonanceLimit | 0.91245  | 0.90578  | 0.000       |                                     |
| cutoffBaseHz         | 212.18   | 240.4    | 0.000       |                                     |
| cutoffSpanOct        | 3.3213   | 3.2947   | 0.000       |                                     |
| envModScaleC0Slope   | 2.8142   | 5.2838   | 0.000       |                                     |
| envModScaleC1Slope   | 4.4796   | 3.4753   | 0.000       |                                     |
| envModOffset         | 0.34908  | 0.36463  | 0.000       |                                     |
| accentSweepDepthOct  | 7.9269   | 8.8376   | 0.000       | AT BOUND (model may lack structure) |
| accentVcaDepth       | 2.2843   | 2.5108   | 0.000       |                                     |
| vcfDecayMinSec       | 0.056061 | 0.061889 | 0.000       |                                     |
| vcfDecayMaxSec       | 1.1404   | 1.0858   | 0.000       |                                     |
| accentDecaySec       | 0.075588 | 0.058055 | 0.000       |                                     |
| vegDecaySec          | 1.0379   | 1.0108   | 0.000       | AT BOUND (model may lack structure) |
| vcaResTapRatio       | 1.0503   | 1.3672   | 0.000       |                                     |
| vcaCutoffLevelDb     | 0        | -1.1582  | 0.000       |                                     |
| vcaCutoffLevelResDb  | 0        | -0.22109 | 0.000       |                                     |

Timing: note-on offset -0.24 ms (relative to each clip's measured note-on), gate length 1309.4 ms.

## Applying the result

Paste `synth_parameters.txt` into `acidus::SynthParameters` (src/core/SynthEngine.hpp) or rerun with `--apply`.

Audio: `renders/` (hardware / before / after for the 23 worst samples).
