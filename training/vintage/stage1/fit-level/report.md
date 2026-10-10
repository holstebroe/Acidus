# Acidus vs. hardware TB-303 calibration report (20261010-113726)

24 reference samples, 5 free parameters. Search: 313 evaluations in 10.1 min, 0 CMA-ES restarts, stopped because: time cap reached.

## Match quality (mean over samples)

All errors are dB; lower is better. `harm` = full-note harmonic levels (k^-0.5 weighted RMS), `inter` = energy between harmonics, `env` = RMS envelope, `stft` = 1/3-octave levels over time, `peak` = shape (height+width+implicit frequency) of the resonant-peak window on Resonance=max samples only -- weighted more heavily than `harm` so it can't be traded away.

| metric                                         | before | after |
|------------------------------------------------|--------|-------|
| objective (incl. priors)                       | 2.31   | 2.25  |
| weighted error                                 | 2.31   | 2.25  |
| harmonic error (dB)                            | 1.89   | 1.81  |
| resonant-peak shape error (dB)                 | 2.11   | 2.10  |
| inter-harmonic error (dB)                      | 0.53   | 0.46  |
| envelope error (dB)                            | 3.93   | 3.52  |
| spectrogram error (dB)                         | 7.54   | 7.34  |
| aligned waveform error (10*sqrt(residual/ref)) | 0.00   | 0.00  |
| resonant-peak sweep track error (semitones)    | 2.63   | 2.63  |
| harmonics-over-time error (dB)                 | 6.03   | 5.85  |
| mean |harmonic error| (dB)                     | 2.20   | 2.22  |
| note level error, RMS over notes (dB)          | 3.26   | 2.35  |
| harmonics within 1 dB (%)                      | 32.65  | 27.07 |
| harmonics within 3 dB (%)                      | 73.83  | 72.72 |
| harmonics within 6 dB (%)                      | 96.24  | 97.08 |

Recording gain solved as -9.9 dB (before: -10.6 dB).

## Per set

Mean error of each set's notes, and the RMS of their level errors (the recordings are not normalised, so absolute note levels are compared under one global gain).

| set | notes | before | after | level before (dB) | level after (dB) | harm | env | stft |
|-----|-------|--------|-------|-------------------|------------------|------|-----|------|
| 1A  | 9     | 2.26   | 2.22  | 3.5               | 2.9              | 1.9  | 3.5 | 7.1  |
| 1B  | 5     | 2.75   | 2.79  | 1.9               | 2.0              | 1.6  | 4.4 | 8.2  |
| 1C  | 3     | 2.12   | 2.11  | 2.3               | 2.0              | 1.8  | 3.5 | 6.3  |
| 1D  | 4     | 2.17   | 2.18  | 1.5               | 1.0              | 1.7  | 3.0 | 8.0  |
| 1E  | 2     | 1.71   | 0.96  | 6.8               | 2.4              | 2.1  | 2.2 | 5.5  |
| 1R  | 1     | 3.02   | 3.06  | 3.2               | 3.3              | 1.6  | 3.9 | 9.1  |

## Per sample (sorted by remaining error, worst first)

| sample | before | after | harm | peak | inter | env | stft | wave | harm ±3dB | level before | level after | tuning (c) | drift (c) |
|--------|--------|-------|------|------|-------|-----|------|------|-----------|--------------|-------------|------------|-----------|
| 1A-6   | 3.70   | 3.79  | 1.5  | 3.3  | 0.1   | 2.8 | 9.2  | 0.0  | 90%       | +1.5         | +2.0        | +0.0       | 0.00      |
| 1D-4   | 3.42   | 3.46  | 1.4  | 1.7  | 0.8   | 2.7 | 10.6 | 0.0  | 88%       | -1.8         | +0.1        | +0.0       | 0.00      |
| 1B-5   | 3.38   | 3.45  | 2.2  | 2.9  | 0.6   | 4.8 | 6.1  | 0.0  | 91%       | +1.8         | +2.2        | +0.0       | 0.00      |
| 1A-3   | 3.21   | 3.20  | 2.2  | 2.6  | 0.0   | 2.2 | 7.4  | 0.0  | 28%       | -0.5         | -0.3        | +0.0       | 0.00      |
| 1R-1   | 3.02   | 3.06  | 1.6  | 1.4  | 0.0   | 3.9 | 9.1  | 0.0  | 85%       | +3.2         | +3.3        | +0.0       | 0.00      |
| 1D-3   | 3.05   | 3.01  | 1.7  | 2.7  | 0.1   | 2.7 | 7.5  | 0.0  | 67%       | +0.3         | +0.7        | +0.0       | 0.00      |
| 1A-8   | 2.98   | 2.96  | 2.2  | 2.7  | 1.2   | 3.1 | 5.3  | 0.0  | 89%       | +3.0         | +2.9        | +0.0       | 0.00      |
| 1B-1   | 2.69   | 2.78  | 1.8  | 1.9  | 0.0   | 3.3 | 9.2  | 0.0  | 60%       | +1.9         | +2.3        | +0.0       | 0.00      |
| 1B-2   | 2.72   | 2.76  | 1.3  | 2.1  | 0.0   | 3.6 | 9.9  | 0.0  | 95%       | +1.6         | +2.0        | +0.0       | 0.00      |
| 1A-5   | 2.61   | 2.65  | 1.7  | 1.4  | 0.0   | 4.8 | 7.9  | 0.0  | 85%       | +3.2         | +3.4        | +0.0       | 0.00      |
| 1B-4   | 2.54   | 2.49  | 1.3  | 2.2  | 0.4   | 7.0 | 7.6  | 0.0  | 92%       | -2.8         | -2.4        | +0.0       | 0.00      |
| 1B-3   | 2.42   | 2.48  | 1.6  | 1.9  | 0.6   | 3.3 | 8.2  | 0.0  | 92%       | +0.2         | +0.6        | +0.0       | 0.00      |
| 1C-3   | 2.41   | 2.35  | 2.7  | 2.5  | 0.0   | 2.3 | 5.1  | 0.0  | 73%       | -3.4         | -3.0        | +0.0       | 0.00      |
| 1C-2   | 2.27   | 2.31  | 2.0  | 1.9  | 2.8   | 4.3 | 6.4  | 0.0  | 33%       | -1.0         | -0.6        | +0.0       | 0.00      |
| 1A-9   | 1.77   | 1.90  | 2.2  | 2.1  | 1.3   | 2.5 | 4.3  | 0.0  | 76%       | -0.4         | +0.5        | +0.0       | 0.00      |
| 1A-2   | 1.68   | 1.69  | 1.6  | 1.3  | 0.0   | 2.3 | 8.6  | 0.0  | 43%       | -0.1         | +0.3        | +0.0       | 0.00      |
| 1C-1   | 1.67   | 1.67  | 0.8  | 0.9  | 1.7   | 3.8 | 7.4  | 0.0  | 100%      | -1.8         | -1.5        | +0.0       | 0.00      |
| 1A-7   | 1.98   | 1.58  | 2.7  | -    | 1.0   | 6.1 | 7.7  | 0.0  | 92%       | +8.0         | +5.8        | +0.0       | 0.00      |
| 1D-1   | 1.17   | 1.19  | 2.3  | -    | 0.0   | 2.3 | 8.0  | 0.0  | 32%       | +2.1         | +1.2        | +0.0       | 0.00      |
| 1A-1   | 1.12   | 1.14  | 2.0  | -    | 0.0   | 3.9 | 6.4  | 0.0  | 40%       | -1.5         | -1.9        | +0.0       | 0.00      |
| 1A-4   | 1.27   | 1.11  | 1.3  | -    | 0.0   | 4.0 | 7.3  | 0.0  | 88%       | +4.6         | +3.5        | +0.0       | 0.00      |
| 1D-2   | 1.03   | 1.03  | 1.2  | -    | 0.0   | 4.4 | 6.1  | 0.0  | 81%       | +1.3         | +1.4        | +0.0       | 0.00      |
| 1E-1   | 2.38   | 1.01  | 2.9  | -    | 0.3   | 2.1 | 4.4  | 0.0  | 36%       | +8.5         | +0.2        | +0.0       | 0.00      |
| 1E-2   | 1.05   | 0.91  | 1.3  | -    | 0.0   | 2.3 | 6.5  | 0.0  | 88%       | +4.5         | +3.4        | +0.0       | 0.00      |

### Resonant peak (Resonance=max samples)

| sample | peak Hz (hw) | peak Hz (before) | peak Hz (after) | height dB (hw) | height dB (before) | height dB (after) |
|--------|--------------|------------------|-----------------|----------------|--------------------|-------------------|
| 1A-2   | 131          | 65               | 65              | -1.5           | +0.0               | +0.0              |
| 1A-3   | 196          | 196              | 196             | -0.1           | +0.1               | +0.1              |
| 1A-5   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1A-6   | 327          | 458              | 458             | -1.9           | -0.8               | -0.8              |
| 1A-8   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1A-9   | 1701         | 1766             | 1766            | -3.9           | -6.1               | -6.1              |
| 1B-1   | 65           | 392              | 392             | +0.0           | -4.6               | -4.6              |
| 1B-2   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1B-3   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1B-4   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1B-5   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1C-1   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1C-2   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1C-3   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1D-3   | 589          | 262              | 262             | -10.3          | -2.0               | -2.1              |
| 1D-4   | 981          | 14455            | 15436           | -6.4           | -133.6             | -138.3            |
| 1R-1   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |

Height is the resonant peak's level above the fundamental (gain-independent). This is the number that answers "is the acid squelch there": before/after collapsing to something much smaller than hardware's means the peak got flattened away, not just shifted.

`tuning` is measured from the harmonic peaks relative to equal temperament; `drift` is the std-dev of short-time pitch over the note (hardware fluctuation).

### Worst remaining harmonics / spurious high-frequency content

- **1A-1** worst: k17 1112Hz ref -65.4 / sim -72.9, k21 1374Hz ref -72.7 / sim -81.7, k14 916Hz ref -59.1 / sim -65.1, k20 1308Hz ref -71.0 / sim -79.6. Model excess above 4 kHz: none > 3 dB.
- **1A-2** worst: k21 1374Hz ref -68.4 / sim -74.5, k25 1635Hz ref -74.7 / sim -81.8, k19 1243Hz ref -64.8 / sim -70.3, k17 1112Hz ref -60.8 / sim -65.7. Model excess above 4 kHz: none > 3 dB.
- **1A-3** worst: k14 916Hz ref -47.4 / sim -54.1, k13 850Hz ref -45.1 / sim -51.2, k23 1504Hz ref -65.8 / sim -74.3, k26 1701Hz ref -70.5 / sim -79.5. Model excess above 4 kHz: none > 3 dB.
- **1A-4** worst: k1 65Hz ref 0.0 / sim 3.8, k2 131Hz ref -4.7 / sim -1.2, k38 2485Hz ref -76.1 / sim -79.4, k37 2420Hz ref -75.2 / sim -78.3. Model excess above 4 kHz: none > 3 dB.
- **1A-5** worst: k6 392Hz ref -12.3 / sim -7.4, k5 327Hz ref -8.9 / sim -4.3, k7 458Hz ref -15.2 / sim -11.1, k3 196Hz ref -5.4 / sim -1.4. Model excess above 4 kHz: none > 3 dB.
- **1A-6** worst: k7 458Hz ref -7.6 / sim -0.6, k8 523Hz ref -9.8 / sim -5.3, k6 392Hz ref -4.6 / sim -0.9, k14 916Hz ref -31.2 / sim -28.2. Model excess above 4 kHz: none > 3 dB.
- **1A-7** worst: k3 196Hz ref -8.4 / sim 0.3, k4 262Hz ref -10.7 / sim -2.6, k2 131Hz ref -4.9 / sim 2.9, k5 327Hz ref -12.5 / sim -5.3. Model excess above 4 kHz: none > 3 dB.
- **1A-8** worst: k5 327Hz ref -11.9 / sim -6.5, k6 392Hz ref -13.0 / sim -7.7, k4 262Hz ref -10.4 / sim -5.2, k7 458Hz ref -13.8 / sim -8.7. Model excess above 4 kHz: none > 3 dB.
- **1A-9** worst: k5 327Hz ref -12.1 / sim -8.2, k6 392Hz ref -13.3 / sim -9.4, k7 458Hz ref -14.3 / sim -10.4, k4 262Hz ref -10.6 / sim -6.8. Model excess above 4 kHz: harmonic 5690Hz +3.6dB, harmonic 5756Hz +3.4dB, harmonic 6475Hz +3.2dB.
- **1B-1** worst: k6 392Hz ref -8.2 / sim -3.8, k7 458Hz ref -9.6 / sim -5.3, k45 2943Hz ref -75.3 / sim -71.8, k48 3140Hz ref -78.0 / sim -74.5. Model excess above 4 kHz: none > 3 dB.
- **1B-2** worst: k5 327Hz ref -8.1 / sim -3.9, k6 392Hz ref -9.5 / sim -5.4, k7 458Hz ref -10.6 / sim -7.2, k3 196Hz ref -6.1 / sim -3.2. Model excess above 4 kHz: none > 3 dB.
- **1B-3** worst: k86 5625Hz ref -53.9 / sim -50.8, k85 5560Hz ref -53.4 / sim -50.2, k84 5494Hz ref -52.9 / sim -49.7, k83 5429Hz ref -52.4 / sim -49.2. Model excess above 4 kHz: inter-harmonic 4938Hz +3.3dB, inter-harmonic 4611Hz +3.3dB, harmonic 5625Hz +3.2dB, harmonic 5560Hz +3.2dB.
- **1B-4** worst: k15 981Hz ref -18.4 / sim -22.5, k16 1046Hz ref -19.2 / sim -23.1, k14 916Hz ref -18.1 / sim -21.7, k17 1112Hz ref -19.5 / sim -23.8. Model excess above 4 kHz: none > 3 dB.
- **1B-5** worst: k9 589Hz ref -14.8 / sim -9.2, k10 654Hz ref -14.9 / sim -9.4, k8 523Hz ref -14.4 / sim -9.1, k7 458Hz ref -13.8 / sim -8.7. Model excess above 4 kHz: harmonic 15501Hz +4.8dB, harmonic 15698Hz +4.8dB, harmonic 15436Hz +4.8dB.
- **1C-1** worst: k8 523Hz ref -18.2 / sim -21.1, k7 458Hz ref -17.4 / sim -19.0, k1 65Hz ref 0.0 / sim -1.6, k2 131Hz ref -6.0 / sim -7.3. Model excess above 4 kHz: inter-harmonic 4088Hz +3.7dB.
- **1C-2** worst: k105 6868Hz ref -77.6 / sim -73.7, k104 6802Hz ref -77.2 / sim -73.2, k103 6737Hz ref -76.8 / sim -72.8, k102 6672Hz ref -76.3 / sim -72.4. Model excess above 4 kHz: inter-harmonic 4088Hz +4.2dB, harmonic 6868Hz +4.0dB, harmonic 6802Hz +4.0dB, harmonic 6737Hz +3.9dB.
- **1C-3** worst: k38 2485Hz ref -21.7 / sim -28.9, k37 2420Hz ref -21.4 / sim -28.5, k36 2355Hz ref -21.1 / sim -28.1, k39 2551Hz ref -22.3 / sim -29.3. Model excess above 4 kHz: none > 3 dB.
- **1D-1** worst: k30 1962Hz ref -68.5 / sim -74.7, k31 2028Hz ref -69.7 / sim -76.0, k29 1897Hz ref -67.3 / sim -73.4, k32 2093Hz ref -70.8 / sim -77.2. Model excess above 4 kHz: none > 3 dB.
- **1D-2** worst: k11 720Hz ref -21.7 / sim -26.6, k10 654Hz ref -20.6 / sim -24.8, k12 785Hz ref -22.7 / sim -28.2, k9 589Hz ref -19.4 / sim -22.8. Model excess above 4 kHz: none > 3 dB.
- **1D-3** worst: k28 1831Hz ref -54.9 / sim -60.0, k27 1766Hz ref -53.5 / sim -58.6, k26 1701Hz ref -52.1 / sim -57.1, k29 1897Hz ref -56.3 / sim -61.2. Model excess above 4 kHz: none > 3 dB.
- **1D-4** worst: k70 4578Hz ref -79.6 / sim -75.7, k69 4513Hz ref -79.0 / sim -75.1, k68 4448Hz ref -78.3 / sim -74.6, k67 4382Hz ref -77.7 / sim -74.0. Model excess above 4 kHz: harmonic 4578Hz +3.9dB, harmonic 4513Hz +3.9dB, harmonic 4644Hz +3.8dB.
- **1E-1** worst: k13 850Hz ref -42.6 / sim -55.3, k15 981Hz ref -47.8 / sim -70.1, k28 1831Hz ref -69.8 / sim -80.7, k17 1112Hz ref -52.7 / sim -63.9. Model excess above 4 kHz: none > 3 dB.
- **1E-2** worst: k1 65Hz ref 0.0 / sim 3.7, k2 131Hz ref -4.7 / sim -1.4, k38 2485Hz ref -76.5 / sim -79.8, k37 2420Hz ref -75.6 / sim -78.7. Model excess above 4 kHz: none > 3 dB.
- **1R-1** worst: k6 392Hz ref -12.2 / sim -7.4, k5 327Hz ref -8.9 / sim -4.3, k3 196Hz ref -5.4 / sim -1.4, k7 458Hz ref -15.1 / sim -11.2. Model excess above 4 kHz: none > 3 dB.

## Suspicious samples

- **1D-1**: spectrum is within 1.0 dB of 1E-2 although the cutoff/envMod/decay/accent label differs -- one of the two labels is probably wrong
- **1A-4**: spectrum is within 1.2 dB of 1D-1 although the cutoff/envMod/decay/accent label differs -- one of the two labels is probably wrong
- **1A-6**: error 3.79 is 1.6x the median

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

| parameter           | before   | after    | sensitivity |  |
|---------------------|----------|----------|-------------|--|
| oscSquareLevel      | 0.9262   | 0.38901  | 0.000       |  |
| accentVcaDepth      | 1.4816   | 2.3468   | 0.000       |  |
| vcaResTapRatio      | 1.0708   | 1.7357   | 0.000       |  |
| vcaCutoffLevelDb    | -2.004   | -2.5749  | 0.000       |  |
| vcaCutoffLevelResDb | -0.47047 | -0.25653 | 0.000       |  |

Timing: note-on offset 0.00 ms (relative to each clip's measured note-on), gate length 1300.0 ms.

## Applying the result

Paste `synth_parameters.txt` into `acidus::SynthParameters` (src/core/SynthEngine.hpp) or rerun with `--apply`.

Audio: `renders/` (hardware / before / after for the 24 worst samples).
