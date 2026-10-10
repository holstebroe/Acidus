# Acidus vs. hardware TB-303 calibration report (20261010-184244)

23 reference samples, 7 free parameters. Search: 331 evaluations in 10.0 min, 0 CMA-ES restarts, stopped because: time cap reached.

## Match quality (mean over samples)

All errors are dB; lower is better. `harm` = full-note harmonic levels (k^-0.5 weighted RMS), `inter` = energy between harmonics, `env` = RMS envelope, `stft` = 1/3-octave levels over time, `peak` = shape (height+width+implicit frequency) of the resonant-peak window on Resonance=max samples only -- weighted more heavily than `harm` so it can't be traded away.

| metric                                         | before | after |
|------------------------------------------------|--------|-------|
| objective (incl. priors)                       | 2.31   | 2.22  |
| weighted error                                 | 2.31   | 2.22  |
| harmonic error (dB)                            | 1.85   | 1.81  |
| resonant-peak shape error (dB)                 | 2.11   | 2.08  |
| inter-harmonic error (dB)                      | 0.60   | 0.53  |
| envelope error (dB)                            | 3.93   | 3.12  |
| spectrogram error (dB)                         | 7.27   | 7.19  |
| aligned waveform error (10*sqrt(residual/ref)) | 0.00   | 0.00  |
| resonant-peak sweep track error (semitones)    | 2.50   | 2.49  |
| harmonics-over-time error (dB)                 | 6.08   | 6.01  |
| mean |harmonic error| (dB)                     | 2.22   | 2.27  |
| note level error, RMS over notes (dB)          | 2.76   | 2.10  |
| harmonics within 1 dB (%)                      | 32.20  | 29.48 |
| harmonics within 3 dB (%)                      | 74.52  | 72.41 |
| harmonics within 6 dB (%)                      | 94.28  | 95.50 |

Recording gain solved as -9.8 dB (before: -10.3 dB).

## Per set

Mean error of each set's notes, and the RMS of their level errors (the recordings are not normalised, so absolute note levels are compared under one global gain).

| set | notes | before | after | level before (dB) | level after (dB) | harm | env | stft |
|-----|-------|--------|-------|-------------------|------------------|------|-----|------|
| 1A  | 9     | 2.34   | 2.22  | 3.4               | 2.4              | 2.0  | 3.0 | 6.8  |
| 1B  | 5     | 2.70   | 2.68  | 2.0               | 2.1              | 1.6  | 3.8 | 8.0  |
| 1C  | 3     | 2.13   | 2.05  | 2.4               | 2.0              | 1.6  | 3.4 | 6.6  |
| 1D  | 3     | 2.52   | 2.49  | 0.5               | 0.5              | 1.9  | 2.7 | 8.5  |
| 1E  | 2     | 1.27   | 0.94  | 3.6               | 1.8              | 2.0  | 2.0 | 5.6  |
| 1R  | 1     | 1.97   | 2.08  | 2.5               | 3.1              | 1.5  | 3.5 | 8.2  |

## Per sample (sorted by remaining error, worst first)

| sample | before | after | harm | peak | inter | env | stft | wave | harm ±3dB | level before | level after | tuning (c) | drift (c) |
|--------|--------|-------|------|------|-------|-----|------|------|-----------|--------------|-------------|------------|-----------|
| 1A-6   | 4.09   | 4.08  | 2.0  | 3.5  | 1.0   | 4.2 | 8.5  | 0.0  | 55%       | +1.7         | +1.9        | +0.0       | 0.00      |
| 1D-4   | 3.60   | 3.62  | 1.6  | 1.8  | 1.1   | 5.0 | 10.0 | 0.0  | 81%       | -0.8         | +0.6        | +0.0       | 0.00      |
| 1A-3   | 3.32   | 3.24  | 2.2  | 2.3  | 0.0   | 4.4 | 6.6  | 0.0  | 35%       | -0.5         | -0.6        | +0.0       | 0.00      |
| 1A-8   | 3.30   | 3.09  | 2.1  | 2.8  | 1.3   | 2.8 | 7.0  | 0.0  | 88%       | +3.5         | +2.7        | +0.0       | 0.00      |
| 1B-1   | 2.96   | 3.03  | 1.9  | 1.9  | 0.3   | 3.0 | 9.5  | 0.0  | 49%       | +1.7         | +2.2        | +0.0       | 0.00      |
| 1A-5   | 2.94   | 3.02  | 1.7  | 1.5  | 0.1   | 3.5 | 9.3  | 0.0  | 85%       | +2.6         | +3.2        | +0.0       | 0.00      |
| 1B-5   | 3.06   | 3.01  | 2.0  | 2.9  | 0.8   | 3.3 | 7.1  | 0.0  | 95%       | +2.4         | +2.5        | +0.0       | 0.00      |
| 1D-3   | 2.95   | 2.86  | 1.4  | 1.7  | 0.3   | 1.9 | 9.2  | 0.0  | 76%       | +0.4         | +0.4        | +0.0       | 0.00      |
| 1B-2   | 2.80   | 2.85  | 1.4  | 2.1  | 0.4   | 3.2 | 9.6  | 0.0  | 95%       | +1.5         | +1.9        | +0.0       | 0.00      |
| 1C-3   | 2.36   | 2.37  | 2.9  | 2.6  | 0.0   | 2.4 | 5.8  | 0.0  | 68%       | -3.1         | -2.9        | +0.0       | 0.00      |
| 1B-4   | 2.48   | 2.32  | 1.4  | 2.3  | 1.0   | 6.6 | 5.5  | 0.0  | 88%       | -3.0         | -2.6        | +0.0       | 0.00      |
| 1B-3   | 2.20   | 2.18  | 1.3  | 2.1  | 0.0   | 2.9 | 8.2  | 0.0  | 89%       | +0.2         | +0.6        | +0.0       | 0.00      |
| 1C-2   | 2.22   | 2.13  | 1.3  | 2.1  | 1.1   | 3.9 | 7.6  | 0.0  | 96%       | -1.3         | -0.8        | +0.0       | 0.00      |
| 1R-1   | 1.97   | 2.08  | 1.5  | 1.4  | 0.4   | 3.5 | 8.2  | 0.0  | 85%       | +2.5         | +3.1        | +0.0       | 0.00      |
| 1A-9   | 1.99   | 2.03  | 2.4  | 2.3  | 2.7   | 1.3 | 5.0  | 0.0  | 62%       | +0.2         | +1.1        | +0.0       | 0.00      |
| 1C-1   | 1.83   | 1.67  | 0.6  | 1.3  | 0.3   | 4.1 | 6.3  | 0.0  | 100%      | -2.5         | -1.9        | +0.0       | 0.00      |
| 1A-2   | 1.53   | 1.41  | 1.6  | 0.9  | 0.0   | 0.9 | 7.7  | 0.0  | 45%       | -1.2         | +0.2        | +0.0       | 0.00      |
| 1A-7   | 1.81   | 1.25  | 2.7  | -    | 0.7   | 4.2 | 5.8  | 0.0  | 76%       | +7.5         | +4.0        | +0.0       | 0.00      |
| 1E-1   | 1.67   | 1.04  | 3.1  | -    | 0.5   | 1.1 | 5.3  | 0.0  | 33%       | +4.3         | +0.3        | +0.0       | 0.00      |
| 1D-1   | 1.01   | 1.00  | 2.5  | -    | 0.0   | 1.1 | 6.3  | 0.0  | 24%       | -0.1         | +0.4        | +0.0       | 0.00      |
| 1A-1   | 1.24   | 0.99  | 1.8  | -    | 0.0   | 3.0 | 5.9  | 0.0  | 48%       | -4.2         | -2.2        | +0.0       | 0.00      |
| 1E-2   | 0.87   | 0.84  | 0.9  | -    | 0.0   | 2.9 | 5.8  | 0.0  | 100%      | +2.6         | +2.5        | +0.0       | 0.00      |
| 1A-4   | 0.84   | 0.83  | 1.1  | -    | 0.0   | 2.8 | 5.4  | 0.0  | 90%       | +2.7         | +2.6        | +0.0       | 0.00      |

### Resonant peak (Resonance=max samples)

| sample | peak Hz (hw) | peak Hz (before) | peak Hz (after) | height dB (hw) | height dB (before) | height dB (after) |
|--------|--------------|------------------|-----------------|----------------|--------------------|-------------------|
| 1A-2   | 131          | 65               | 65              | -1.7           | +0.0               | +0.0              |
| 1A-3   | 196          | 196              | 196             | -0.7           | -0.1               | -0.1              |
| 1A-5   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1A-6   | 327          | 458              | 458             | -2.4           | -1.5               | -1.5              |
| 1A-8   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1A-9   | 1701         | 1831             | 1831            | -5.4           | -8.7               | -8.7              |
| 1B-1   | 65           | 458              | 15109           | +0.0           | -6.5               | -148.7            |
| 1B-2   | 65           | 65               | 15436           | +0.0           | +0.0               | -140.8            |
| 1B-3   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1B-4   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1B-5   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1C-1   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1C-2   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1C-3   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1D-3   | 65           | 12623            | 262             | +0.0           | -139.7             | -2.4              |
| 1D-4   | 981          | 14455            | 14259           | -6.8           | -135.2             | -133.1            |
| 1R-1   | 65           | 65               | 14324           | +0.0           | +0.0               | -148.7            |

Height is the resonant peak's level above the fundamental (gain-independent). This is the number that answers "is the acid squelch there": before/after collapsing to something much smaller than hardware's means the peak got flattened away, not just shifted.

`tuning` is measured from the harmonic peaks relative to equal temperament; `drift` is the std-dev of short-time pitch over the note (hardware fluctuation).

### Worst remaining harmonics / spurious high-frequency content

- **1A-1** worst: k20 1308Hz ref -71.3 / sim -78.1, k19 1243Hz ref -69.3 / sim -76.0, k18 1177Hz ref -67.5 / sim -74.0, k17 1112Hz ref -65.8 / sim -71.7. Model excess above 4 kHz: none > 3 dB.
- **1A-2** worst: k25 1635Hz ref -74.3 / sim -80.2, k21 1374Hz ref -67.9 / sim -73.0, k20 1308Hz ref -66.0 / sim -70.9, k22 1439Hz ref -69.5 / sim -74.9. Model excess above 4 kHz: none > 3 dB.
- **1A-3** worst: k25 1635Hz ref -68.9 / sim -76.5, k24 1570Hz ref -67.2 / sim -74.7, k23 1504Hz ref -65.7 / sim -73.0, k16 1046Hz ref -51.3 / sim -58.2. Model excess above 4 kHz: none > 3 dB.
- **1A-4** worst: k39 2551Hz ref -76.8 / sim -80.1, k38 2485Hz ref -75.9 / sim -79.1, k37 2420Hz ref -74.9 / sim -78.0, k36 2355Hz ref -73.9 / sim -76.9. Model excess above 4 kHz: none > 3 dB.
- **1A-5** worst: k6 392Hz ref -12.5 / sim -7.6, k5 327Hz ref -9.2 / sim -4.7, k7 458Hz ref -15.4 / sim -11.1, k3 196Hz ref -5.6 / sim -1.8. Model excess above 4 kHz: none > 3 dB.
- **1A-6** worst: k7 458Hz ref -8.4 / sim -1.3, k16 1046Hz ref -38.3 / sim -32.7, k17 1112Hz ref -40.7 / sim -35.2, k8 523Hz ref -10.6 / sim -5.2. Model excess above 4 kHz: none > 3 dB.
- **1A-7** worst: k3 196Hz ref -8.4 / sim -1.4, k4 262Hz ref -10.8 / sim -4.2, k2 131Hz ref -5.0 / sim 1.1, k5 327Hz ref -12.6 / sim -7.0. Model excess above 4 kHz: none > 3 dB.
- **1A-8** worst: k5 327Hz ref -12.0 / sim -6.7, k6 392Hz ref -13.1 / sim -7.9, k4 262Hz ref -10.4 / sim -5.3, k7 458Hz ref -13.9 / sim -8.9. Model excess above 4 kHz: none > 3 dB.
- **1A-9** worst: k5 327Hz ref -12.2 / sim -7.7, k6 392Hz ref -13.4 / sim -8.9, k7 458Hz ref -14.4 / sim -9.9, k4 262Hz ref -10.6 / sim -6.3. Model excess above 4 kHz: harmonic 7391Hz +3.8dB, harmonic 7326Hz +3.7dB, harmonic 7260Hz +3.6dB, inter-harmonic 4415Hz +3.2dB.
- **1B-1** worst: k7 458Hz ref -10.1 / sim -5.8, k6 392Hz ref -8.7 / sim -4.5, k49 3205Hz ref -78.3 / sim -74.2, k48 3140Hz ref -77.4 / sim -73.3. Model excess above 4 kHz: none > 3 dB.
- **1B-2** worst: k6 392Hz ref -10.0 / sim -5.7, k5 327Hz ref -8.6 / sim -4.4, k7 458Hz ref -11.1 / sim -7.5, k8 523Hz ref -12.1 / sim -9.2. Model excess above 4 kHz: none > 3 dB.
- **1B-3** worst: k35 2289Hz ref -24.5 / sim -28.2, k34 2224Hz ref -24.2 / sim -27.9, k36 2355Hz ref -25.3 / sim -28.6, k29 1897Hz ref -22.7 / sim -25.9. Model excess above 4 kHz: none > 3 dB.
- **1B-4** worst: k16 1046Hz ref -18.9 / sim -23.2, k18 1177Hz ref -20.3 / sim -24.5, k17 1112Hz ref -19.8 / sim -23.9, k15 981Hz ref -18.7 / sim -22.5. Model excess above 4 kHz: none > 3 dB.
- **1B-5** worst: k9 589Hz ref -14.9 / sim -9.6, k8 523Hz ref -14.5 / sim -9.3, k10 654Hz ref -15.1 / sim -9.9, k7 458Hz ref -13.9 / sim -8.7. Model excess above 4 kHz: none > 3 dB.
- **1C-1** worst: k1 65Hz ref 0.0 / sim -2.2, k8 523Hz ref -19.1 / sim -21.1, k7 458Hz ref -17.4 / sim -19.3, k9 589Hz ref -19.5 / sim -21.9. Model excess above 4 kHz: none > 3 dB.
- **1C-2** worst: k21 1374Hz ref -20.7 / sim -24.6, k20 1308Hz ref -20.3 / sim -24.0, k22 1439Hz ref -21.7 / sim -25.1, k19 1243Hz ref -20.5 / sim -23.4. Model excess above 4 kHz: none > 3 dB.
- **1C-3** worst: k38 2485Hz ref -22.1 / sim -29.7, k39 2551Hz ref -22.6 / sim -30.2, k37 2420Hz ref -21.7 / sim -29.2, k36 2355Hz ref -21.4 / sim -28.8. Model excess above 4 kHz: none > 3 dB.
- **1D-1** worst: k34 2224Hz ref -72.8 / sim -79.5, k33 2158Hz ref -71.6 / sim -78.3, k32 2093Hz ref -70.5 / sim -77.1, k31 2028Hz ref -69.4 / sim -75.9. Model excess above 4 kHz: none > 3 dB.
- **1D-3** worst: k27 1766Hz ref -52.9 / sim -57.7, k28 1831Hz ref -54.4 / sim -59.1, k29 1897Hz ref -55.8 / sim -60.4, k26 1701Hz ref -51.6 / sim -56.1. Model excess above 4 kHz: none > 3 dB.
- **1D-4** worst: k28 1831Hz ref -46.1 / sim -40.8, k29 1897Hz ref -46.0 / sim -41.1, k45 2943Hz ref -56.4 / sim -60.4, k70 4578Hz ref -79.5 / sim -75.9. Model excess above 4 kHz: harmonic 4578Hz +3.6dB, harmonic 4644Hz +3.5dB, harmonic 4513Hz +3.4dB.
- **1E-1** worst: k13 850Hz ref -42.8 / sim -54.5, k15 981Hz ref -48.0 / sim -69.8, k28 1831Hz ref -69.8 / sim -80.0, k29 1897Hz ref -78.5 / sim -68.6. Model excess above 4 kHz: none > 3 dB.
- **1E-2** worst: k1 65Hz ref 0.0 / sim 2.7, k2 131Hz ref -4.7 / sim -2.2, k38 2485Hz ref -77.5 / sim -79.4, k37 2420Hz ref -76.5 / sim -78.3. Model excess above 4 kHz: none > 3 dB.
- **1R-1** worst: k6 392Hz ref -12.5 / sim -7.6, k5 327Hz ref -9.2 / sim -4.7, k7 458Hz ref -15.3 / sim -11.1, k3 196Hz ref -5.6 / sim -1.8. Model excess above 4 kHz: none > 3 dB.

## Suspicious samples

- **1D-1**: spectrum is within 0.9 dB of 1E-2 although the cutoff/envMod/decay/accent label differs -- one of the two labels is probably wrong
- **1A-4**: spectrum is within 1.2 dB of 1D-1 although the cutoff/envMod/decay/accent label differs -- one of the two labels is probably wrong
- **1A-6**: error 4.08 is 1.9x the median
- **1D-4**: error 3.62 is 1.7x the median

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

| parameter           | before   | after   | sensitivity |  |
|---------------------|----------|---------|-------------|--|
| oscSquareLevel      | 0.68211  | 0.4287  | 0.000       |  |
| accentVcaDepth      | 2.5108   | 3.4986  | 0.000       |  |
| vcaResTapRatio      | 1.3672   | 2.0876  | 0.000       |  |
| vcaCutoffLevelDb    | -1.1582  | -2.8276 | 0.000       |  |
| vcaCutoffLevelResDb | -0.22109 | 0.09905 | 0.000       |  |

Timing: note-on offset -0.46 ms (relative to each clip's measured note-on), gate length 1311.3 ms.

## Applying the result

Paste `synth_parameters.txt` into `acidus::SynthParameters` (src/core/SynthEngine.hpp) or rerun with `--apply`.

Audio: `renders/` (hardware / before / after for the 23 worst samples).
