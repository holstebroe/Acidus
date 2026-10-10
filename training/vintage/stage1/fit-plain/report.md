# Acidus vs. hardware TB-303 calibration report (20261010-105358)

24 reference samples, 19 free parameters. Search: 949 evaluations in 30.1 min, 0 CMA-ES restarts, stopped because: time cap reached.

## Match quality (mean over samples)

All errors are dB; lower is better. `harm` = full-note harmonic levels (k^-0.5 weighted RMS), `inter` = energy between harmonics, `env` = RMS envelope, `stft` = 1/3-octave levels over time, `peak` = shape (height+width+implicit frequency) of the resonant-peak window on Resonance=max samples only -- weighted more heavily than `harm` so it can't be traded away.

| metric                                         | before | after |
|------------------------------------------------|--------|-------|
| objective (incl. priors)                       | 2.78   | 2.28  |
| weighted error                                 | 2.78   | 2.28  |
| harmonic error (dB)                            | 2.49   | 1.88  |
| resonant-peak shape error (dB)                 | 2.75   | 2.11  |
| inter-harmonic error (dB)                      | 0.76   | 0.53  |
| envelope error (dB)                            | 4.14   | 3.60  |
| spectrogram error (dB)                         | 8.47   | 7.42  |
| aligned waveform error (10*sqrt(residual/ref)) | 0.00   | 0.00  |
| resonant-peak sweep track error (semitones)    | 3.18   | 2.63  |
| harmonics-over-time error (dB)                 | 7.13   | 5.89  |
| mean |harmonic error| (dB)                     | 2.85   | 2.20  |
| note level error, RMS over notes (dB)          | 4.03   | 3.27  |
| harmonics within 1 dB (%)                      | 27.05  | 32.85 |
| harmonics within 3 dB (%)                      | 59.43  | 73.91 |
| harmonics within 6 dB (%)                      | 89.79  | 96.24 |

Recording gain solved as -10.6 dB (before: -8.6 dB).

## Per set

Mean error of each set's notes, and the RMS of their level errors (the recordings are not normalised, so absolute note levels are compared under one global gain).

| set | notes | before | after | level before (dB) | level after (dB) | harm | env | stft |
|-----|-------|--------|-------|-------------------|------------------|------|-----|------|
| 1A  | 9     | 2.54   | 2.20  | 5.3               | 3.5              | 1.9  | 3.3 | 7.0  |
| 1B  | 5     | 3.46   | 2.70  | 3.0               | 1.9              | 1.5  | 4.0 | 7.7  |
| 1C  | 3     | 2.94   | 2.12  | 0.9               | 2.3              | 1.9  | 3.7 | 6.3  |
| 1D  | 4     | 2.93   | 2.15  | 2.6               | 1.5              | 1.6  | 2.7 | 8.1  |
| 1E  | 2     | 1.42   | 1.72  | 5.2               | 6.8              | 3.3  | 5.8 | 8.3  |
| 1R  | 1     | 3.23   | 2.97  | 3.6               | 3.2              | 1.5  | 3.5 | 8.6  |

## Per sample (sorted by remaining error, worst first)

| sample | before | after | harm | peak | inter | env | stft | wave | harm ±3dB | level before | level after | tuning (c) | drift (c) |
|--------|--------|-------|------|------|-------|-----|------|------|-----------|--------------|-------------|------------|-----------|
| 1A-6   | 3.34   | 3.64  | 1.3  | 3.3  | 0.1   | 2.0 | 8.7  | 0.0  | 94%       | +3.0         | +1.5        | +0.0       | 0.00      |
| 1D-4   | 4.47   | 3.43  | 1.3  | 1.9  | 0.0   | 2.6 | 10.5 | 0.0  | 90%       | +0.7         | -1.8        | +0.0       | 0.00      |
| 1B-5   | 5.01   | 3.32  | 2.0  | 2.9  | 0.5   | 4.4 | 5.4  | 0.0  | 91%       | +4.6         | +1.8        | +0.0       | 0.00      |
| 1A-3   | 3.45   | 3.10  | 2.2  | 2.7  | 0.0   | 1.3 | 7.0  | 0.0  | 28%       | +0.2         | -0.5        | +0.0       | 0.00      |
| 1D-3   | 4.75   | 3.01  | 1.9  | 2.8  | 0.0   | 2.1 | 7.5  | 0.0  | 45%       | +1.5         | +0.3        | +0.0       | 0.00      |
| 1A-8   | 3.67   | 2.98  | 2.3  | 2.7  | 1.3   | 2.9 | 5.7  | 0.0  | 89%       | +6.1         | +3.0        | +0.0       | 0.00      |
| 1R-1   | 3.23   | 2.97  | 1.5  | 1.4  | 0.0   | 3.5 | 8.6  | 0.0  | 87%       | +3.6         | +3.2        | +0.0       | 0.00      |
| 1B-2   | 3.06   | 2.67  | 1.3  | 2.1  | 0.0   | 2.9 | 9.5  | 0.0  | 94%       | +2.8         | +1.6        | +0.0       | 0.00      |
| 1B-1   | 2.80   | 2.66  | 1.5  | 1.9  | 0.0   | 2.7 | 8.6  | 0.0  | 74%       | +3.0         | +1.9        | +0.0       | 0.00      |
| 1A-5   | 2.73   | 2.57  | 1.6  | 1.4  | 0.0   | 4.6 | 7.3  | 0.0  | 85%       | +3.6         | +3.2        | +0.0       | 0.00      |
| 1B-4   | 3.36   | 2.49  | 1.5  | 2.2  | 0.3   | 7.0 | 7.5  | 0.0  | 91%       | -1.7         | -2.8        | +0.0       | 0.00      |
| 1C-3   | 3.78   | 2.47  | 3.0  | 2.5  | 0.0   | 2.5 | 5.6  | 0.0  | 72%       | -1.3         | -3.4        | +0.0       | 0.00      |
| 1E-1   | 1.82   | 2.38  | 5.1  | -    | 3.2   | 8.4 | 9.6  | 0.0  | 15%       | +6.0         | +8.5        | +0.0       | 0.00      |
| 1B-3   | 3.09   | 2.34  | 1.4  | 1.9  | 0.5   | 2.9 | 7.7  | 0.0  | 99%       | +1.8         | +0.2        | +0.0       | 0.00      |
| 1C-2   | 2.80   | 2.22  | 1.9  | 1.9  | 2.6   | 4.4 | 6.0  | 0.0  | 42%       | +0.4         | -1.0        | +0.0       | 0.00      |
| 1A-7   | 2.92   | 1.97  | 3.5  | -    | 1.9   | 8.1 | 8.7  | 0.0  | 89%       | +12.1        | +8.0        | +0.0       | 0.00      |
| 1A-9   | 2.35   | 1.69  | 1.6  | 2.1  | 0.9   | 2.1 | 3.3  | 0.0  | 96%       | +2.3         | -0.4        | +0.0       | 0.00      |
| 1C-1   | 2.23   | 1.66  | 0.8  | 0.9  | 1.6   | 4.1 | 7.2  | 0.0  | 100%      | -0.5         | -1.8        | +0.0       | 0.00      |
| 1A-2   | 1.85   | 1.57  | 1.6  | 1.3  | 0.0   | 1.1 | 8.2  | 0.0  | 39%       | -1.3         | -0.1        | +0.0       | 0.00      |
| 1A-4   | 1.21   | 1.24  | 1.6  | -    | 0.0   | 4.6 | 7.8  | 0.0  | 90%       | +4.3         | +4.7        | +0.0       | 0.00      |
| 1D-1   | 1.27   | 1.11  | 1.9  | -    | 0.0   | 1.8 | 8.2  | 0.0  | 46%       | +0.9         | +2.1        | +0.0       | 0.00      |
| 1E-2   | 1.02   | 1.06  | 1.5  | -    | 0.0   | 3.2 | 7.1  | 0.0  | 90%       | +4.2         | +4.5        | +0.0       | 0.00      |
| 1A-1   | 1.37   | 1.04  | 1.9  | -    | 0.0   | 3.2 | 6.1  | 0.0  | 44%       | -4.4         | -1.5        | +0.0       | 0.00      |
| 1D-2   | 1.21   | 1.04  | 1.2  | -    | 0.0   | 4.2 | 6.3  | 0.0  | 81%       | +4.8         | +1.3        | +0.0       | 0.00      |

### Resonant peak (Resonance=max samples)

| sample | peak Hz (hw) | peak Hz (before) | peak Hz (after) | height dB (hw) | height dB (before) | height dB (after) |
|--------|--------------|------------------|-----------------|----------------|--------------------|-------------------|
| 1A-2   | 131          | 65               | 65              | -1.5           | +0.0               | +0.0              |
| 1A-3   | 196          | 196              | 196             | -0.1           | +0.2               | +0.1              |
| 1A-5   | 65           | 65               | 15501           | +0.0           | +0.0               | -150.9            |
| 1A-6   | 327          | 458              | 15436           | -1.9           | -0.9               | -138.8            |
| 1A-8   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1A-9   | 1701         | 1701             | 1766            | -3.9           | -3.6               | -6.1              |
| 1B-1   | 65           | 392              | 14978           | +0.0           | -4.2               | -148.7            |
| 1B-2   | 65           | 327              | 15632           | +0.0           | -4.2               | -140.2            |
| 1B-3   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1B-4   | 65           | 12623            | 65              | +0.0           | -137.9             | +0.0              |
| 1B-5   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1C-1   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1C-2   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1C-3   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1D-3   | 589          | 262              | 262             | -10.3          | -1.1               | -2.0              |
| 1D-4   | 981          | 14259            | 15240           | -6.4           | -140.6             | -143.9            |
| 1R-1   | 65           | 65               | 15501           | +0.0           | +0.0               | -150.9            |

Height is the resonant peak's level above the fundamental (gain-independent). This is the number that answers "is the acid squelch there": before/after collapsing to something much smaller than hardware's means the peak got flattened away, not just shifted.

`tuning` is measured from the harmonic peaks relative to equal temperament; `drift` is the std-dev of short-time pitch over the note (hardware fluctuation).

### Worst remaining harmonics / spurious high-frequency content

- **1A-1** worst: k17 1112Hz ref -65.4 / sim -72.2, k21 1374Hz ref -72.7 / sim -80.5, k20 1308Hz ref -71.0 / sim -78.6, k18 1177Hz ref -67.5 / sim -74.5. Model excess above 4 kHz: none > 3 dB.
- **1A-2** worst: k21 1374Hz ref -68.4 / sim -74.8, k25 1635Hz ref -74.7 / sim -81.9, k19 1243Hz ref -64.8 / sim -70.6, k17 1112Hz ref -60.8 / sim -66.1. Model excess above 4 kHz: none > 3 dB.
- **1A-3** worst: k14 916Hz ref -47.4 / sim -54.3, k13 850Hz ref -45.1 / sim -51.5, k23 1504Hz ref -65.8 / sim -74.5, k26 1701Hz ref -70.5 / sim -79.5. Model excess above 4 kHz: none > 3 dB.
- **1A-4** worst: k1 65Hz ref 0.0 / sim 4.9, k2 131Hz ref -4.7 / sim -0.1, k3 196Hz ref -8.7 / sim -5.2, k4 262Hz ref -13.0 / sim -9.9. Model excess above 4 kHz: none > 3 dB.
- **1A-5** worst: k6 392Hz ref -12.3 / sim -7.6, k5 327Hz ref -8.9 / sim -4.4, k7 458Hz ref -15.2 / sim -11.3, k3 196Hz ref -5.4 / sim -1.6. Model excess above 4 kHz: none > 3 dB.
- **1A-6** worst: k7 458Hz ref -7.6 / sim -1.0, k8 523Hz ref -9.8 / sim -5.8, k6 392Hz ref -4.6 / sim -1.3, k14 916Hz ref -31.2 / sim -28.7. Model excess above 4 kHz: none > 3 dB.
- **1A-7** worst: k3 196Hz ref -8.4 / sim 2.6, k4 262Hz ref -10.7 / sim -0.3, k2 131Hz ref -4.9 / sim 5.2, k5 327Hz ref -12.5 / sim -3.0. Model excess above 4 kHz: none > 3 dB.
- **1A-8** worst: k5 327Hz ref -11.9 / sim -6.4, k6 392Hz ref -13.0 / sim -7.5, k4 262Hz ref -10.4 / sim -5.0, k7 458Hz ref -13.8 / sim -8.5. Model excess above 4 kHz: none > 3 dB.
- **1A-9** worst: k26 1701Hz ref -3.9 / sim -8.5, k32 2093Hz ref -11.5 / sim -15.2, k31 2028Hz ref -10.5 / sim -13.8, k25 1635Hz ref -7.2 / sim -10.2. Model excess above 4 kHz: none > 3 dB.
- **1B-1** worst: k6 392Hz ref -8.2 / sim -4.2, k7 458Hz ref -9.6 / sim -5.7, k45 2943Hz ref -75.3 / sim -72.2, k48 3140Hz ref -78.0 / sim -74.9. Model excess above 4 kHz: none > 3 dB.
- **1B-2** worst: k5 327Hz ref -8.1 / sim -4.3, k6 392Hz ref -9.5 / sim -5.8, k7 458Hz ref -10.6 / sim -7.6, k16 1046Hz ref -21.8 / sim -24.8. Model excess above 4 kHz: none > 3 dB.
- **1B-3** worst: k24 1570Hz ref -21.0 / sim -24.0, k30 1962Hz ref -23.3 / sim -26.6, k31 2028Hz ref -24.1 / sim -27.0, k86 5625Hz ref -53.9 / sim -51.2. Model excess above 4 kHz: none > 3 dB.
- **1B-4** worst: k15 981Hz ref -18.4 / sim -22.9, k16 1046Hz ref -19.2 / sim -23.6, k14 916Hz ref -18.1 / sim -22.1, k1 65Hz ref 0.0 / sim -3.9. Model excess above 4 kHz: none > 3 dB.
- **1B-5** worst: k9 589Hz ref -14.8 / sim -9.6, k10 654Hz ref -14.9 / sim -9.8, k8 523Hz ref -14.4 / sim -9.5, k7 458Hz ref -13.8 / sim -9.1. Model excess above 4 kHz: harmonic 15501Hz +4.3dB, harmonic 15698Hz +4.3dB, harmonic 15436Hz +4.3dB.
- **1C-1** worst: k7 458Hz ref -17.4 / sim -19.4, k1 65Hz ref 0.0 / sim -2.0, k8 523Hz ref -18.2 / sim -21.5, k2 131Hz ref -6.0 / sim -7.7. Model excess above 4 kHz: inter-harmonic 4088Hz +3.3dB.
- **1C-2** worst: k105 6868Hz ref -77.6 / sim -74.1, k104 6802Hz ref -77.2 / sim -73.6, k102 6672Hz ref -76.3 / sim -72.8, k103 6737Hz ref -76.8 / sim -73.2. Model excess above 4 kHz: inter-harmonic 4088Hz +3.7dB, harmonic 6868Hz +3.6dB, harmonic 6802Hz +3.6dB, harmonic 6672Hz +3.5dB.
- **1C-3** worst: k38 2485Hz ref -21.7 / sim -29.3, k37 2420Hz ref -21.4 / sim -29.0, k36 2355Hz ref -21.1 / sim -28.6, k39 2551Hz ref -22.3 / sim -29.8. Model excess above 4 kHz: none > 3 dB.
- **1D-1** worst: k35 2289Hz ref -74.0 / sim -79.5, k34 2224Hz ref -73.0 / sim -78.3, k33 2158Hz ref -71.9 / sim -77.2, k32 2093Hz ref -70.8 / sim -75.9. Model excess above 4 kHz: none > 3 dB.
- **1D-2** worst: k11 720Hz ref -21.7 / sim -26.6, k10 654Hz ref -20.6 / sim -24.8, k12 785Hz ref -22.7 / sim -28.2, k9 589Hz ref -19.4 / sim -22.8. Model excess above 4 kHz: none > 3 dB.
- **1D-3** worst: k28 1831Hz ref -54.9 / sim -60.5, k27 1766Hz ref -53.5 / sim -59.1, k26 1701Hz ref -52.1 / sim -57.6, k29 1897Hz ref -56.3 / sim -61.7. Model excess above 4 kHz: none > 3 dB.
- **1D-4** worst: k46 3009Hz ref -58.8 / sim -63.2, k47 3074Hz ref -60.3 / sim -64.5, k45 2943Hz ref -57.9 / sim -61.9, k48 3140Hz ref -61.9 / sim -65.6. Model excess above 4 kHz: none > 3 dB.
- **1E-1** worst: k29 1897Hz ref -78.0 / sim -63.7, k27 1766Hz ref -74.6 / sim -61.4, k8 523Hz ref -36.6 / sim -24.2, k10 654Hz ref -41.1 / sim -28.9. Model excess above 4 kHz: none > 3 dB.
- **1E-2** worst: k1 65Hz ref 0.0 / sim 4.8, k2 131Hz ref -4.7 / sim -0.3, k3 196Hz ref -8.8 / sim -5.4, k4 262Hz ref -13.2 / sim -10.1. Model excess above 4 kHz: none > 3 dB.
- **1R-1** worst: k6 392Hz ref -12.2 / sim -7.6, k5 327Hz ref -8.9 / sim -4.5, k3 196Hz ref -5.4 / sim -1.6, k7 458Hz ref -15.1 / sim -11.3. Model excess above 4 kHz: none > 3 dB.

## Suspicious samples

- **1D-1**: spectrum is within 1.0 dB of 1E-2 although the cutoff/envMod/decay/accent label differs -- one of the two labels is probably wrong
- **1A-4**: spectrum is within 1.2 dB of 1D-1 although the cutoff/envMod/decay/accent label differs -- one of the two labels is probably wrong

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

Cutoff law with the fitted end stops folded in (so the plugin's knob range equals the hardware's): `cutoffBaseHz = 222`, `cutoffSpanOct = 3.341`, `cutoffTaperExp = 1.429`.

## Fitted model parameters

`sensitivity` = objective increase for a 5% (of range) nudge. Near-zero means the current references don't constrain that parameter -- its value is not evidence of anything. `AT BOUND` means the optimizer pushed it to the edge of its plausible range, usually a sign that it is compensating for something the model does not implement.

| parameter            | before   | after    | sensitivity |                                     |
|----------------------|----------|----------|-------------|-------------------------------------|
| oscSquareLevel       | 0.70553  | 0.9262   | 0.000       |                                     |
| filterFeedbackGain   | 17.42    | 16.393   | 0.000       |                                     |
| filterResonanceLimit | 0.91245  | 0.91309  | 0.000       |                                     |
| cutoffBaseHz         | 212.18   | 222.01   | 0.000       |                                     |
| cutoffSpanOct        | 3.3213   | 3.3414   | 0.000       |                                     |
| envModScaleC0Slope   | 2.8142   | 5.1991   | 0.000       |                                     |
| envModScaleC1Slope   | 4.4796   | 4.069    | 0.000       |                                     |
| envModOffset         | 0.34908  | 0.34816  | 0.000       |                                     |
| accentSweepDepthOct  | 7.9269   | 7.5466   | 0.000       |                                     |
| accentVcaDepth       | 2.2843   | 1.4816   | 0.000       |                                     |
| vcfDecayMinSec       | 0.056061 | 0.061906 | 0.000       |                                     |
| vcfDecayMaxSec       | 1.1404   | 1.0862   | 0.000       |                                     |
| accentDecaySec       | 0.075588 | 0.084449 | 0.000       |                                     |
| vegDecaySec          | 1.0379   | 1        | 0.000       | AT BOUND (model may lack structure) |
| vcaResTapRatio       | 1.0503   | 1.0708   | 0.000       |                                     |
| vcaCutoffLevelDb     | 0        | -2.004   | 0.000       |                                     |
| vcaCutoffLevelResDb  | 0        | -0.47047 | 0.000       |                                     |

Timing: note-on offset -0.65 ms (relative to each clip's measured note-on), gate length 1311.1 ms.

## Applying the result

Paste `synth_parameters.txt` into `acidus::SynthParameters` (src/core/SynthEngine.hpp) or rerun with `--apply`.

Audio: `renders/` (hardware / before / after for the 24 worst samples).
