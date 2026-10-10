# Acidus vs. hardware TB-303 calibration report (20261010-094131)

24 reference samples, 17 free parameters. Search: 0 evaluations in 0.0 min, 0 CMA-ES restarts, stopped because: evaluate-only.

## Match quality (mean over samples)

All errors are dB; lower is better. `harm` = full-note harmonic levels (k^-0.5 weighted RMS), `inter` = energy between harmonics, `env` = RMS envelope, `stft` = 1/3-octave levels over time, `peak` = shape (height+width+implicit frequency) of the resonant-peak window on Resonance=max samples only -- weighted more heavily than `harm` so it can't be traded away.

| metric                                         | before | after |
|------------------------------------------------|--------|-------|
| objective (incl. priors)                       | 2.78   | 2.78  |
| weighted error                                 | 2.78   | 2.78  |
| harmonic error (dB)                            | 2.49   | 2.49  |
| resonant-peak shape error (dB)                 | 2.75   | 2.75  |
| inter-harmonic error (dB)                      | 0.76   | 0.76  |
| envelope error (dB)                            | 4.14   | 4.14  |
| spectrogram error (dB)                         | 8.47   | 8.47  |
| aligned waveform error (10*sqrt(residual/ref)) | 0.00   | 0.00  |
| resonant-peak sweep track error (semitones)    | 3.18   | 3.18  |
| harmonics-over-time error (dB)                 | 7.13   | 7.13  |
| mean |harmonic error| (dB)                     | 2.85   | 2.85  |
| note level error, RMS over notes (dB)          | 4.03   | 4.03  |
| harmonics within 1 dB (%)                      | 27.05  | 27.05 |
| harmonics within 3 dB (%)                      | 59.43  | 59.43 |
| harmonics within 6 dB (%)                      | 89.79  | 89.79 |

Recording gain solved as -8.6 dB (before: -8.6 dB).

## Per set

Mean error of each set's notes, and the RMS of their level errors (the recordings are not normalised, so absolute note levels are compared under one global gain).

| set | notes | before | after | level before (dB) | level after (dB) | harm | env | stft |
|-----|-------|--------|-------|-------------------|------------------|------|-----|------|
| 1A  | 9     | 2.54   | 2.54  | 5.3               | 5.3              | 2.6  | 4.9 | 7.7  |
| 1B  | 5     | 3.46   | 3.46  | 3.0               | 3.0              | 2.4  | 4.2 | 9.5  |
| 1C  | 3     | 2.94   | 2.94  | 0.9               | 0.9              | 2.3  | 2.3 | 10.1 |
| 1D  | 4     | 2.93   | 2.93  | 2.6               | 2.6              | 2.5  | 3.4 | 8.4  |
| 1E  | 2     | 1.42   | 1.42  | 5.2               | 5.2              | 2.8  | 4.5 | 7.0  |
| 1R  | 1     | 3.23   | 3.23  | 3.6               | 3.6              | 1.6  | 4.2 | 8.9  |

## Per sample (sorted by remaining error, worst first)

| sample | before | after | harm | peak | inter | env  | stft | wave | harm ±3dB | level before | level after | tuning (c) | drift (c) |
|--------|--------|-------|------|------|-------|------|------|------|-----------|--------------|-------------|------------|-----------|
| 1B-5   | 5.01   | 5.01  | 5.6  | 2.9  | 3.0   | 5.8  | 7.7  | 0.0  | 18%       | +4.6         | +4.6        | +0.0       | 0.00      |
| 1D-3   | 4.75   | 4.75  | 3.1  | 6.4  | 0.4   | 3.2  | 8.6  | 0.0  | 27%       | +1.5         | +1.5        | +0.0       | 0.00      |
| 1D-4   | 4.47   | 4.47  | 1.9  | 4.1  | 0.7   | 3.2  | 11.5 | 0.0  | 71%       | +0.7         | +0.7        | +0.0       | 0.00      |
| 1C-3   | 3.78   | 3.78  | 4.4  | 2.9  | 0.0   | 1.4  | 7.4  | 0.0  | 46%       | -1.3         | -1.3        | +0.0       | 0.00      |
| 1A-8   | 3.67   | 3.67  | 3.6  | 2.7  | 2.8   | 5.8  | 7.1  | 0.0  | 84%       | +6.1         | +6.1        | +0.0       | 0.00      |
| 1A-3   | 3.45   | 3.45  | 2.3  | 3.3  | 0.2   | 2.3  | 7.5  | 0.0  | 28%       | +0.2         | +0.2        | +0.0       | 0.00      |
| 1B-4   | 3.36   | 3.36  | 1.4  | 2.8  | 0.0   | 4.1  | 10.9 | 0.0  | 93%       | -1.7         | -1.7        | +0.0       | 0.00      |
| 1A-6   | 3.34   | 3.34  | 1.6  | 3.4  | 0.4   | 3.6  | 9.3  | 0.0  | 90%       | +3.0         | +3.0        | +0.0       | 0.00      |
| 1R-1   | 3.23   | 3.23  | 1.6  | 1.4  | 0.2   | 4.2  | 8.9  | 0.0  | 87%       | +3.6         | +3.6        | +0.0       | 0.00      |
| 1B-3   | 3.09   | 3.09  | 1.3  | 2.3  | 0.0   | 3.5  | 9.6  | 0.0  | 93%       | +1.8         | +1.8        | +0.0       | 0.00      |
| 1B-2   | 3.06   | 3.06  | 2.0  | 2.2  | 0.4   | 4.0  | 10.2 | 0.0  | 72%       | +2.8         | +2.8        | +0.0       | 0.00      |
| 1A-7   | 2.92   | 2.92  | 5.7  | -    | 3.9   | 12.1 | 11.1 | 0.0  | 0%        | +12.1        | +12.1       | +0.0       | 0.00      |
| 1B-1   | 2.80   | 2.80  | 1.9  | 2.1  | 0.1   | 3.8  | 9.2  | 0.0  | 62%       | +3.0         | +3.0        | +0.0       | 0.00      |
| 1C-2   | 2.80   | 2.80  | 2.0  | 2.7  | 0.0   | 3.4  | 10.9 | 0.0  | 77%       | +0.4         | +0.4        | +0.0       | 0.00      |
| 1A-5   | 2.73   | 2.73  | 1.6  | 1.4  | 0.2   | 5.0  | 7.7  | 0.0  | 87%       | +3.6         | +3.6        | +0.0       | 0.00      |
| 1A-9   | 2.35   | 2.35  | 2.7  | 2.6  | 1.9   | 3.0  | 5.0  | 0.0  | 76%       | +2.3         | +2.3        | +0.0       | 0.00      |
| 1C-1   | 2.23   | 2.23  | 0.4  | 1.8  | 0.5   | 2.1  | 12.0 | 0.0  | 100%      | -0.5         | -0.5        | +0.0       | 0.00      |
| 1A-2   | 1.85   | 1.85  | 2.0  | 1.7  | 0.0   | 2.7  | 7.7  | 0.0  | 21%       | -1.3         | -1.3        | +0.0       | 0.00      |
| 1E-1   | 1.82   | 1.82  | 4.1  | -    | 2.6   | 6.0  | 7.3  | 0.0  | 24%       | +6.0         | +6.0        | +0.0       | 0.00      |
| 1A-1   | 1.37   | 1.37  | 2.7  | -    | 0.0   | 5.4  | 6.2  | 0.0  | 8%        | -4.4         | -4.4        | +0.0       | 0.00      |
| 1D-1   | 1.27   | 1.27  | 2.9  | -    | 0.0   | 2.5  | 7.6  | 0.0  | 24%       | +0.9         | +0.9        | +0.0       | 0.00      |
| 1A-4   | 1.21   | 1.21  | 1.6  | -    | 0.1   | 4.6  | 7.4  | 0.0  | 81%       | +4.3         | +4.3        | +0.0       | 0.00      |
| 1D-2   | 1.21   | 1.21  | 2.1  | -    | 0.9   | 4.7  | 5.8  | 0.0  | 74%       | +4.8         | +4.8        | +0.0       | 0.00      |
| 1E-2   | 1.02   | 1.02  | 1.5  | -    | 0.0   | 3.0  | 6.7  | 0.0  | 83%       | +4.2         | +4.2        | +0.0       | 0.00      |

### Resonant peak (Resonance=max samples)

| sample | peak Hz (hw) | peak Hz (before) | peak Hz (after) | height dB (hw) | height dB (before) | height dB (after) |
|--------|--------------|------------------|-----------------|----------------|--------------------|-------------------|
| 1A-2   | 131          | 65               | 65              | -1.5           | +0.0               | +0.0              |
| 1A-3   | 196          | 196              | 196             | -0.1           | +0.2               | +0.2              |
| 1A-5   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1A-6   | 327          | 458              | 458             | -1.9           | -0.9               | -0.9              |
| 1A-8   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1A-9   | 1701         | 1701             | 1701            | -3.9           | -3.6               | -3.6              |
| 1B-1   | 65           | 392              | 392             | +0.0           | -4.2               | -4.2              |
| 1B-2   | 65           | 327              | 327             | +0.0           | -4.2               | -4.2              |
| 1B-3   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1B-4   | 65           | 12623            | 12623           | +0.0           | -137.9             | -137.9            |
| 1B-5   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1C-1   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1C-2   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1C-3   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1D-3   | 589          | 262              | 262             | -10.3          | -1.1               | -1.1              |
| 1D-4   | 981          | 14259            | 14259           | -6.4           | -140.6             | -140.6            |
| 1R-1   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |

Height is the resonant peak's level above the fundamental (gain-independent). This is the number that answers "is the acid squelch there": before/after collapsing to something much smaller than hardware's means the peak got flattened away, not just shifted.

`tuning` is measured from the harmonic peaks relative to equal temperament; `drift` is the std-dev of short-time pitch over the note (hardware fluctuation).

### Worst remaining harmonics / spurious high-frequency content

- **1A-1** worst: k10 654Hz ref -48.4 / sim -55.4, k9 589Hz ref -45.3 / sim -51.6, k13 850Hz ref -56.6 / sim -65.5, k17 1112Hz ref -65.4 / sim -76.3. Model excess above 4 kHz: none > 3 dB.
- **1A-2** worst: k10 654Hz ref -42.3 / sim -47.5, k9 589Hz ref -39.0 / sim -43.6, k11 720Hz ref -45.5 / sim -51.1, k21 1374Hz ref -68.4 / sim -77.2. Model excess above 4 kHz: none > 3 dB.
- **1A-3** worst: k13 850Hz ref -45.1 / sim -52.0, k14 916Hz ref -47.4 / sim -54.9, k12 785Hz ref -43.0 / sim -48.9, k23 1504Hz ref -65.8 / sim -75.3. Model excess above 4 kHz: none > 3 dB.
- **1A-4** worst: k1 65Hz ref 0.0 / sim 4.7, k2 131Hz ref -4.7 / sim -0.5, k38 2485Hz ref -76.1 / sim -79.7, k36 2355Hz ref -74.1 / sim -77.5. Model excess above 4 kHz: none > 3 dB.
- **1A-5** worst: k5 327Hz ref -8.9 / sim -4.0, k6 392Hz ref -12.3 / sim -7.4, k3 196Hz ref -5.4 / sim -1.0, k2 131Hz ref -4.0 / sim 0.2. Model excess above 4 kHz: none > 3 dB.
- **1A-6** worst: k7 458Hz ref -7.6 / sim 0.0, k6 392Hz ref -4.6 / sim 1.2, k8 523Hz ref -9.8 / sim -5.0, k3 196Hz ref -6.2 / sim -2.8. Model excess above 4 kHz: none > 3 dB.
- **1A-7** worst: k3 196Hz ref -8.4 / sim 6.5, k4 262Hz ref -10.7 / sim 3.6, k2 131Hz ref -4.9 / sim 9.3, k5 327Hz ref -12.5 / sim 0.8. Model excess above 4 kHz: harmonic 4251Hz +3.8dB, harmonic 4055Hz +3.8dB, harmonic 4121Hz +3.8dB.
- **1A-8** worst: k5 327Hz ref -11.9 / sim -3.4, k6 392Hz ref -13.0 / sim -4.5, k7 458Hz ref -13.8 / sim -5.5, k4 262Hz ref -10.4 / sim -2.0. Model excess above 4 kHz: none > 3 dB.
- **1A-9** worst: k22 1439Hz ref -15.4 / sim -9.9, k21 1374Hz ref -16.3 / sim -10.9, k20 1308Hz ref -17.0 / sim -11.6, k23 1504Hz ref -13.9 / sim -8.6. Model excess above 4 kHz: harmonic 5690Hz +3.5dB, harmonic 5756Hz +3.2dB, harmonic 8307Hz +3.0dB.
- **1B-1** worst: k6 392Hz ref -8.2 / sim -2.9, k7 458Hz ref -9.6 / sim -4.5, k8 523Hz ref -11.0 / sim -6.8, k3 196Hz ref -6.4 / sim -2.5. Model excess above 4 kHz: none > 3 dB.
- **1B-2** worst: k6 392Hz ref -9.5 / sim -4.3, k5 327Hz ref -8.1 / sim -3.0, k7 458Hz ref -10.6 / sim -6.0, k8 523Hz ref -11.6 / sim -7.6. Model excess above 4 kHz: none > 3 dB.
- **1B-3** worst: k4 262Hz ref -8.8 / sim -5.0, k5 327Hz ref -10.4 / sim -6.6, k6 392Hz ref -11.6 / sim -8.0, k3 196Hz ref -6.5 / sim -3.1. Model excess above 4 kHz: none > 3 dB.
- **1B-4** worst: k13 850Hz ref -17.6 / sim -23.4, k15 981Hz ref -18.4 / sim -29.2, k14 916Hz ref -18.1 / sim -26.2, k16 1046Hz ref -19.2 / sim -32.2. Model excess above 4 kHz: none > 3 dB.
- **1B-5** worst: k9 589Hz ref -14.8 / sim -6.7, k8 523Hz ref -14.4 / sim -6.4, k168 10988Hz ref -49.7 / sim -41.9, k167 10923Hz ref -49.6 / sim -41.8. Model excess above 4 kHz: harmonic 15763Hz +12.5dB, harmonic 15828Hz +12.4dB, harmonic 15894Hz +12.4dB, inter-harmonic 15796Hz +11.2dB.
- **1C-1** worst: k8 523Hz ref -18.2 / sim -20.9, k2 131Hz ref -6.0 / sim -4.7, k1 65Hz ref 0.0 / sim -1.2, k4 262Hz ref -12.9 / sim -12.1. Model excess above 4 kHz: none > 3 dB.
- **1C-2** worst: k46 3009Hz ref -42.0 / sim -58.4, k51 3336Hz ref -47.9 / sim -62.5, k52 3401Hz ref -48.8 / sim -63.3, k50 3270Hz ref -46.9 / sim -61.7. Model excess above 4 kHz: none > 3 dB.
- **1C-3** worst: k28 1831Hz ref -19.1 / sim -33.6, k27 1766Hz ref -18.9 / sim -31.9, k29 1897Hz ref -19.4 / sim -35.2, k30 1962Hz ref -19.6 / sim -36.7. Model excess above 4 kHz: none > 3 dB.
- **1D-1** worst: k18 1177Hz ref -51.2 / sim -58.1, k19 1243Hz ref -52.9 / sim -60.0, k20 1308Hz ref -54.6 / sim -61.9, k21 1374Hz ref -56.2 / sim -63.7. Model excess above 4 kHz: none > 3 dB.
- **1D-2** worst: k3 196Hz ref -9.1 / sim -1.8, k4 262Hz ref -11.8 / sim -4.9, k2 131Hz ref -4.9 / sim 1.4, k5 327Hz ref -13.9 / sim -7.9. Model excess above 4 kHz: none > 3 dB.
- **1D-3** worst: k25 1635Hz ref -50.7 / sim -58.7, k24 1570Hz ref -49.3 / sim -57.3, k26 1701Hz ref -52.1 / sim -60.0, k27 1766Hz ref -53.5 / sim -61.3. Model excess above 4 kHz: none > 3 dB.
- **1D-4** worst: k43 2812Hz ref -55.8 / sim -62.1, k44 2878Hz ref -57.0 / sim -62.7, k42 2747Hz ref -54.8 / sim -60.5, k45 2943Hz ref -57.9 / sim -63.0. Model excess above 4 kHz: none > 3 dB.
- **1E-1** worst: k29 1897Hz ref -78.0 / sim -66.2, k27 1766Hz ref -74.6 / sim -63.8, k15 981Hz ref -47.8 / sim -64.2, k8 523Hz ref -36.6 / sim -26.6. Model excess above 4 kHz: none > 3 dB.
- **1E-2** worst: k1 65Hz ref 0.0 / sim 4.5, k2 131Hz ref -4.7 / sim -0.7, k38 2485Hz ref -76.5 / sim -80.1, k37 2420Hz ref -75.6 / sim -79.0. Model excess above 4 kHz: none > 3 dB.
- **1R-1** worst: k5 327Hz ref -8.9 / sim -4.0, k6 392Hz ref -12.2 / sim -7.4, k3 196Hz ref -5.4 / sim -1.1, k2 131Hz ref -4.0 / sim 0.2. Model excess above 4 kHz: none > 3 dB.

## Suspicious samples

- **1D-1**: spectrum is within 1.0 dB of 1E-2 although the cutoff/envMod/decay/accent label differs -- one of the two labels is probably wrong
- **1A-4**: spectrum is within 1.2 dB of 1D-1 although the cutoff/envMod/decay/accent label differs -- one of the two labels is probably wrong
- **1B-5**: error 5.01 is 1.8x the median
- **1D-3**: error 4.75 is 1.7x the median
- **1D-4**: error 4.47 is 1.6x the median

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

Cutoff law with the fitted end stops folded in (so the plugin's knob range equals the hardware's): `cutoffBaseHz = 212.2`, `cutoffSpanOct = 3.321`, `cutoffTaperExp = 1.429`.

## Fitted model parameters

`sensitivity` = objective increase for a 5% (of range) nudge. Near-zero means the current references don't constrain that parameter -- its value is not evidence of anything. `AT BOUND` means the optimizer pushed it to the edge of its plausible range, usually a sign that it is compensating for something the model does not implement.

| parameter            | before   | after    | sensitivity |  |
|----------------------|----------|----------|-------------|--|
| oscSquareLevel       | 0.70553  | 0.70553  | 0.000       |  |
| filterFeedbackGain   | 17.42    | 17.42    | 0.000       |  |
| filterResonanceLimit | 0.91245  | 0.91245  | 0.000       |  |
| cutoffBaseHz         | 212.18   | 212.18   | 0.000       |  |
| cutoffSpanOct        | 3.3213   | 3.3213   | 0.000       |  |
| envModScaleC0Slope   | 2.8142   | 2.8142   | 0.000       |  |
| envModScaleC1Slope   | 4.4796   | 4.4796   | 0.000       |  |
| envModOffset         | 0.34908  | 0.34908  | 0.000       |  |
| accentSweepDepthOct  | 7.9269   | 7.9269   | 0.000       |  |
| accentVcaDepth       | 2.2843   | 2.2843   | 0.000       |  |
| vcfDecayMinSec       | 0.056061 | 0.056061 | 0.000       |  |
| vcfDecayMaxSec       | 1.1404   | 1.1404   | 0.000       |  |
| accentDecaySec       | 0.075588 | 0.075588 | 0.000       |  |
| vegDecaySec          | 1.0379   | 1.0379   | 0.000       |  |
| vcaResTapRatio       | 1.0503   | 1.0503   | 0.000       |  |

Timing: note-on offset 0.00 ms (relative to each clip's measured note-on), gate length 1300.0 ms.

## Applying the result

Paste `synth_parameters.txt` into `acidus::SynthParameters` (src/core/SynthEngine.hpp) or rerun with `--apply`.

Audio: `renders/` (hardware / before / after for the 24 worst samples).
