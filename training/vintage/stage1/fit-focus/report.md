# Acidus vs. hardware TB-303 calibration report (20261010-112443)

24 reference samples, 19 free parameters. Search: 1855 evaluations in 20.9 min, 2 CMA-ES restarts, stopped because: time cap reached.

## Match quality (mean over samples)

All errors are dB; lower is better. `harm` = full-note harmonic levels (k^-0.5 weighted RMS), `inter` = energy between harmonics, `env` = RMS envelope, `stft` = 1/3-octave levels over time, `peak` = shape (height+width+implicit frequency) of the resonant-peak window on Resonance=max samples only -- weighted more heavily than `harm` so it can't be traded away.

| metric                                         | before | after |
|------------------------------------------------|--------|-------|
| objective (incl. priors)                       | 2.78   | 2.26  |
| weighted error                                 | 2.78   | 2.26  |
| harmonic error (dB)                            | 2.49   | 1.83  |
| resonant-peak shape error (dB)                 | 2.75   | 2.00  |
| inter-harmonic error (dB)                      | 0.76   | 0.53  |
| envelope error (dB)                            | 4.14   | 3.45  |
| spectrogram error (dB)                         | 8.47   | 7.54  |
| aligned waveform error (10*sqrt(residual/ref)) | 0.00   | 0.00  |
| resonant-peak sweep track error (semitones)    | 3.18   | 2.70  |
| harmonics-over-time error (dB)                 | 7.13   | 6.03  |
| mean |harmonic error| (dB)                     | 2.85   | 2.22  |
| note level error, RMS over notes (dB)          | 4.03   | 2.99  |
| harmonics within 1 dB (%)                      | 27.05  | 25.67 |
| harmonics within 3 dB (%)                      | 59.43  | 75.82 |
| harmonics within 6 dB (%)                      | 89.79  | 96.69 |

Recording gain solved as -10.8 dB (before: -8.6 dB).

## Per set

Mean error of each set's notes, and the RMS of their level errors (the recordings are not normalised, so absolute note levels are compared under one global gain).

| set | notes | before | after | level before (dB) | level after (dB) | harm | env | stft |
|-----|-------|--------|-------|-------------------|------------------|------|-----|------|
| 1A  | 9     | 2.54   | 2.19  | 5.3               | 3.6              | 2.0  | 3.4 | 7.3  |
| 1B  | 5     | 3.46   | 2.77  | 3.0               | 2.0              | 1.6  | 3.8 | 7.9  |
| 1C  | 3     | 2.94   | 2.15  | 0.9               | 2.7              | 1.8  | 4.1 | 6.4  |
| 1D  | 4     | 2.93   | 2.14  | 2.6               | 1.1              | 1.3  | 2.5 | 8.2  |
| 1E  | 2     | 1.42   | 1.46  | 5.2               | 4.9              | 2.9  | 4.2 | 7.7  |
| 1R  | 1     | 3.23   | 2.76  | 3.6               | 2.5              | 1.5  | 2.9 | 8.9  |

## Per sample (sorted by remaining error, worst first)

| sample | before | after | harm | peak | inter | env | stft | wave | harm ±3dB | level before | level after | tuning (c) | drift (c) |
|--------|--------|-------|------|------|-------|-----|------|------|-----------|--------------|-------------|------------|-----------|
| 1A-6   | 3.34   | 3.91  | 1.6  | 3.6  | 0.2   | 1.8 | 9.4  | 0.0  | 81%       | +3.0         | +1.2        | +0.0       | 0.00      |
| 1D-4   | 4.47   | 3.55  | 1.0  | 2.0  | 1.0   | 2.1 | 10.8 | 0.0  | 97%       | +0.7         | -1.5        | +0.0       | 0.00      |
| 1B-5   | 5.01   | 3.48  | 2.2  | 2.9  | 0.5   | 4.4 | 5.6  | 0.0  | 87%       | +4.6         | +2.2        | +0.0       | 0.00      |
| 1A-8   | 3.67   | 3.00  | 2.3  | 2.7  | 1.6   | 3.2 | 5.9  | 0.0  | 88%       | +6.1         | +3.4        | +0.0       | 0.00      |
| 1D-3   | 4.75   | 2.96  | 1.3  | 1.8  | 0.1   | 1.9 | 8.3  | 0.0  | 78%       | +1.5         | -0.0        | +0.0       | 0.00      |
| 1B-1   | 2.80   | 2.86  | 2.0  | 1.8  | 0.0   | 2.4 | 9.1  | 0.0  | 42%       | +3.0         | +1.5        | +0.0       | 0.00      |
| 1R-1   | 3.23   | 2.76  | 1.5  | 1.5  | 0.0   | 2.9 | 8.9  | 0.0  | 91%       | +3.6         | +2.5        | +0.0       | 0.00      |
| 1B-2   | 3.06   | 2.75  | 1.0  | 2.0  | 0.0   | 2.5 | 9.8  | 0.0  | 97%       | +2.8         | +1.2        | +0.0       | 0.00      |
| 1A-3   | 3.45   | 2.69  | 2.0  | 1.3  | 0.0   | 1.3 | 7.7  | 0.0  | 38%       | +0.2         | -0.8        | +0.0       | 0.00      |
| 1B-4   | 3.36   | 2.54  | 1.6  | 2.3  | 0.0   | 7.6 | 7.6  | 0.0  | 91%       | -1.7         | -3.3        | +0.0       | 0.00      |
| 1A-5   | 2.73   | 2.46  | 1.7  | 1.5  | 0.0   | 4.2 | 7.6  | 0.0  | 80%       | +3.6         | +2.5        | +0.0       | 0.00      |
| 1C-3   | 3.78   | 2.30  | 2.9  | 2.5  | 0.0   | 2.6 | 5.6  | 0.0  | 72%       | -1.3         | -3.4        | +0.0       | 0.00      |
| 1C-2   | 2.80   | 2.27  | 1.6  | 2.1  | 2.2   | 4.9 | 6.0  | 0.0  | 100%      | +0.4         | -1.5        | +0.0       | 0.00      |
| 1B-3   | 3.09   | 2.22  | 1.2  | 1.9  | 0.3   | 2.2 | 7.5  | 0.0  | 100%      | +1.8         | -0.1        | +0.0       | 0.00      |
| 1A-7   | 2.92   | 2.06  | 3.7  | -    | 2.1   | 8.4 | 8.9  | 0.0  | 88%       | +12.1        | +8.3        | +0.0       | 0.00      |
| 1E-1   | 1.82   | 1.99  | 4.5  | -    | 2.6   | 6.1 | 8.4  | 0.0  | 21%       | +6.0         | +6.1        | +0.0       | 0.00      |
| 1A-9   | 2.35   | 1.92  | 1.8  | 2.3  | 1.0   | 2.0 | 3.7  | 0.0  | 93%       | +2.3         | -0.3        | +0.0       | 0.00      |
| 1C-1   | 2.23   | 1.88  | 1.0  | 1.0  | 1.2   | 4.9 | 7.4  | 0.0  | 80%       | -0.5         | -2.9        | +0.0       | 0.00      |
| 1A-2   | 1.85   | 1.44  | 1.5  | 0.7  | 0.0   | 1.6 | 8.6  | 0.0  | 46%       | -1.3         | -1.2        | +0.0       | 0.00      |
| 1A-1   | 1.37   | 1.14  | 2.0  | -    | 0.0   | 4.1 | 6.1  | 0.0  | 36%       | -4.4         | -3.3        | +0.0       | 0.00      |
| 1A-4   | 1.21   | 1.08  | 1.2  | -    | 0.0   | 3.5 | 7.5  | 0.0  | 95%       | +4.3         | +3.5        | +0.0       | 0.00      |
| 1D-1   | 1.27   | 1.04  | 1.8  | -    | 0.0   | 1.6 | 7.8  | 0.0  | 44%       | +0.9         | +0.8        | +0.0       | 0.00      |
| 1D-2   | 1.21   | 1.01  | 1.2  | -    | 0.1   | 4.3 | 5.8  | 0.0  | 78%       | +4.8         | +1.4        | +0.0       | 0.00      |
| 1E-2   | 1.02   | 0.93  | 1.2  | -    | 0.0   | 2.4 | 6.9  | 0.0  | 95%       | +4.2         | +3.4        | +0.0       | 0.00      |

### Resonant peak (Resonance=max samples)

| sample | peak Hz (hw) | peak Hz (before) | peak Hz (after) | height dB (hw) | height dB (before) | height dB (after) |
|--------|--------------|------------------|-----------------|----------------|--------------------|-------------------|
| 1A-2   | 131          | 65               | 65              | -1.5           | +0.0               | +0.0              |
| 1A-3   | 196          | 196              | 196             | -0.1           | +0.2               | +0.5              |
| 1A-5   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1A-6   | 327          | 458              | 458             | -1.9           | -0.9               | -0.3              |
| 1A-8   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1A-9   | 1701         | 1701             | 1831            | -3.9           | -3.6               | -6.6              |
| 1B-1   | 65           | 392              | 15436           | +0.0           | -4.2               | -145.5            |
| 1B-2   | 65           | 327              | 65              | +0.0           | -4.2               | +0.0              |
| 1B-3   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1B-4   | 65           | 12623            | 65              | +0.0           | -137.9             | +0.0              |
| 1B-5   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1C-1   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1C-2   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1C-3   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1D-3   | 589          | 262              | 12296           | -10.3          | -1.1               | -149.3            |
| 1D-4   | 981          | 14259            | 15305           | -6.4           | -140.6             | -141.7            |
| 1R-1   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |

Height is the resonant peak's level above the fundamental (gain-independent). This is the number that answers "is the acid squelch there": before/after collapsing to something much smaller than hardware's means the peak got flattened away, not just shifted.

`tuning` is measured from the harmonic peaks relative to equal temperament; `drift` is the std-dev of short-time pitch over the note (hardware fluctuation).

### Worst remaining harmonics / spurious high-frequency content

- **1A-1** worst: k17 1112Hz ref -65.4 / sim -72.3, k21 1374Hz ref -72.7 / sim -80.7, k20 1308Hz ref -71.0 / sim -78.7, k18 1177Hz ref -67.5 / sim -74.6. Model excess above 4 kHz: none > 3 dB.
- **1A-2** worst: k21 1374Hz ref -68.4 / sim -73.8, k25 1635Hz ref -74.7 / sim -80.9, k19 1243Hz ref -64.8 / sim -69.6, k23 1504Hz ref -71.7 / sim -77.5. Model excess above 4 kHz: none > 3 dB.
- **1A-3** worst: k23 1504Hz ref -65.8 / sim -72.6, k26 1701Hz ref -70.5 / sim -77.8, k20 1308Hz ref -60.5 / sim -66.8, k15 981Hz ref -49.8 / sim -55.4. Model excess above 4 kHz: none > 3 dB.
- **1A-4** worst: k1 65Hz ref 0.0 / sim 3.7, k2 131Hz ref -4.7 / sim -1.2, k3 196Hz ref -8.7 / sim -6.2, k4 262Hz ref -13.0 / sim -10.8. Model excess above 4 kHz: none > 3 dB.
- **1A-5** worst: k6 392Hz ref -12.3 / sim -7.6, k7 458Hz ref -15.2 / sim -11.2, k5 327Hz ref -8.9 / sim -4.9, k8 523Hz ref -18.3 / sim -14.8. Model excess above 4 kHz: none > 3 dB.
- **1A-6** worst: k7 458Hz ref -7.6 / sim -0.8, k8 523Hz ref -9.8 / sim -4.6, k14 916Hz ref -31.2 / sim -27.2, k17 1112Hz ref -39.0 / sim -35.4. Model excess above 4 kHz: none > 3 dB.
- **1A-7** worst: k3 196Hz ref -8.4 / sim 2.9, k4 262Hz ref -10.7 / sim 0.0, k2 131Hz ref -4.9 / sim 5.5, k5 327Hz ref -12.5 / sim -2.7. Model excess above 4 kHz: none > 3 dB.
- **1A-8** worst: k5 327Hz ref -11.9 / sim -6.0, k6 392Hz ref -13.0 / sim -7.2, k4 262Hz ref -10.4 / sim -4.6, k7 458Hz ref -13.8 / sim -8.2. Model excess above 4 kHz: none > 3 dB.
- **1A-9** worst: k26 1701Hz ref -3.9 / sim -9.2, k25 1635Hz ref -7.2 / sim -10.7, k32 2093Hz ref -11.5 / sim -14.9, k5 327Hz ref -12.1 / sim -8.9. Model excess above 4 kHz: none > 3 dB.
- **1B-1** worst: k48 3140Hz ref -78.0 / sim -73.4, k49 3205Hz ref -78.8 / sim -74.3, k47 3074Hz ref -77.1 / sim -72.6, k45 2943Hz ref -75.3 / sim -70.8. Model excess above 4 kHz: none > 3 dB.
- **1B-2** worst: k6 392Hz ref -9.5 / sim -6.1, k5 327Hz ref -8.1 / sim -4.9, k7 458Hz ref -10.6 / sim -7.7, k8 523Hz ref -11.6 / sim -9.4. Model excess above 4 kHz: none > 3 dB.
- **1B-3** worst: k30 1962Hz ref -23.3 / sim -26.2, k24 1570Hz ref -21.0 / sim -23.7, k29 1897Hz ref -22.7 / sim -25.8, k31 2028Hz ref -24.1 / sim -26.6. Model excess above 4 kHz: none > 3 dB.
- **1B-4** worst: k1 65Hz ref 0.0 / sim -4.5, k15 981Hz ref -18.4 / sim -22.8, k16 1046Hz ref -19.2 / sim -23.5, k14 916Hz ref -18.1 / sim -22.1. Model excess above 4 kHz: none > 3 dB.
- **1B-5** worst: k9 589Hz ref -14.8 / sim -9.3, k10 654Hz ref -14.9 / sim -9.7, k8 523Hz ref -14.4 / sim -9.1, k7 458Hz ref -13.8 / sim -8.8. Model excess above 4 kHz: harmonic 15501Hz +3.9dB, harmonic 15436Hz +3.9dB, harmonic 15240Hz +3.8dB.
- **1C-1** worst: k7 458Hz ref -17.4 / sim -20.7, k1 65Hz ref 0.0 / sim -3.1, k2 131Hz ref -6.0 / sim -8.6, k6 392Hz ref -15.9 / sim -18.4. Model excess above 4 kHz: none > 3 dB.
- **1C-2** worst: k19 1243Hz ref -20.2 / sim -23.5, k16 1046Hz ref -18.8 / sim -21.7, k17 1112Hz ref -19.4 / sim -22.3, k80 5232Hz ref -66.2 / sim -63.3. Model excess above 4 kHz: none > 3 dB.
- **1C-3** worst: k38 2485Hz ref -21.7 / sim -29.1, k37 2420Hz ref -21.4 / sim -28.7, k39 2551Hz ref -22.3 / sim -29.6, k36 2355Hz ref -21.1 / sim -28.3. Model excess above 4 kHz: none > 3 dB.
- **1D-1** worst: k35 2289Hz ref -74.0 / sim -79.1, k34 2224Hz ref -73.0 / sim -78.0, k33 2158Hz ref -71.9 / sim -76.8, k36 2355Hz ref -75.1 / sim -80.3. Model excess above 4 kHz: none > 3 dB.
- **1D-2** worst: k11 720Hz ref -21.7 / sim -25.5, k12 785Hz ref -22.7 / sim -27.1, k3 196Hz ref -9.1 / sim -5.5, k13 850Hz ref -23.8 / sim -28.5. Model excess above 4 kHz: none > 3 dB.
- **1D-3** worst: k28 1831Hz ref -54.9 / sim -59.2, k29 1897Hz ref -56.3 / sim -60.6, k27 1766Hz ref -53.5 / sim -57.7, k30 1962Hz ref -57.8 / sim -61.8. Model excess above 4 kHz: none > 3 dB.
- **1D-4** worst: k47 3074Hz ref -60.3 / sim -63.5, k48 3140Hz ref -61.9 / sim -65.0, k1 65Hz ref 0.0 / sim -2.7, k46 3009Hz ref -58.8 / sim -61.5. Model excess above 4 kHz: none > 3 dB.
- **1E-1** worst: k29 1897Hz ref -78.0 / sim -64.1, k27 1766Hz ref -74.6 / sim -61.8, k10 654Hz ref -41.1 / sim -30.1, k8 523Hz ref -36.6 / sim -25.6. Model excess above 4 kHz: none > 3 dB.
- **1E-2** worst: k1 65Hz ref 0.0 / sim 3.6, k2 131Hz ref -4.7 / sim -1.3, k3 196Hz ref -8.8 / sim -6.3, k4 262Hz ref -13.2 / sim -10.9. Model excess above 4 kHz: none > 3 dB.
- **1R-1** worst: k6 392Hz ref -12.2 / sim -7.7, k5 327Hz ref -8.9 / sim -5.0, k7 458Hz ref -15.1 / sim -11.2, k8 523Hz ref -18.0 / sim -14.8. Model excess above 4 kHz: none > 3 dB.

## Suspicious samples

- **1D-1**: spectrum is within 1.0 dB of 1E-2 although the cutoff/envMod/decay/accent label differs -- one of the two labels is probably wrong
- **1A-4**: spectrum is within 1.2 dB of 1D-1 although the cutoff/envMod/decay/accent label differs -- one of the two labels is probably wrong
- **1A-6**: error 3.91 is 1.7x the median
- **1D-4**: error 3.55 is 1.6x the median
- **1B-5**: error 3.48 is 1.5x the median

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

Cutoff law with the fitted end stops folded in (so the plugin's knob range equals the hardware's): `cutoffBaseHz = 245`, `cutoffSpanOct = 3.243`, `cutoffTaperExp = 1.429`.

## Fitted model parameters

`sensitivity` = objective increase for a 5% (of range) nudge. Near-zero means the current references don't constrain that parameter -- its value is not evidence of anything. `AT BOUND` means the optimizer pushed it to the edge of its plausible range, usually a sign that it is compensating for something the model does not implement.

| parameter            | before   | after    | sensitivity |                                     |
|----------------------|----------|----------|-------------|-------------------------------------|
| oscSquareLevel       | 0.70553  | 0.7923   | 0.000       |                                     |
| filterFeedbackGain   | 17.42    | 16.142   | 0.000       |                                     |
| filterResonanceLimit | 0.91245  | 0.92169  | 0.000       |                                     |
| cutoffBaseHz         | 212.18   | 245.01   | 0.000       |                                     |
| cutoffSpanOct        | 3.3213   | 3.2427   | 0.000       |                                     |
| envModScaleC0Slope   | 2.8142   | 5.0312   | 0.000       |                                     |
| envModScaleC1Slope   | 4.4796   | 4.0253   | 0.000       |                                     |
| envModOffset         | 0.34908  | 0.36668  | 0.000       |                                     |
| accentSweepDepthOct  | 7.9269   | 8.6367   | 0.000       |                                     |
| accentVcaDepth       | 2.2843   | 2.327    | 0.000       |                                     |
| vcfDecayMinSec       | 0.056061 | 0.05653  | 0.000       |                                     |
| vcfDecayMaxSec       | 1.1404   | 1.1216   | 0.000       |                                     |
| accentDecaySec       | 0.075588 | 0.063384 | 0.000       |                                     |
| vegDecaySec          | 1.0379   | 1.0253   | 0.000       | AT BOUND (model may lack structure) |
| vcaResTapRatio       | 1.0503   | 1.1735   | 0.000       |                                     |
| vcaCutoffLevelDb     | 0        | -1.1851  | 0.000       |                                     |
| vcaCutoffLevelResDb  | 0        | -0.21485 | 0.000       |                                     |

Timing: note-on offset -0.60 ms (relative to each clip's measured note-on), gate length 1310.6 ms.

## Applying the result

Paste `synth_parameters.txt` into `acidus::SynthParameters` (src/core/SynthEngine.hpp) or rerun with `--apply`.

Audio: `renders/` (hardware / before / after for the 24 worst samples).
