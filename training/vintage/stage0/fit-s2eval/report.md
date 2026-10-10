# Acidus vs. hardware TB-303 calibration report (20261010-153308)

44 reference samples, 17 free parameters. Search: 0 evaluations in 0.0 min, 0 CMA-ES restarts, stopped because: evaluate-only.

## Match quality (mean over samples)

All errors are dB; lower is better. `harm` = full-note harmonic levels (k^-0.5 weighted RMS), `inter` = energy between harmonics, `env` = RMS envelope, `stft` = 1/3-octave levels over time, `peak` = shape (height+width+implicit frequency) of the resonant-peak window on Resonance=max samples only -- weighted more heavily than `harm` so it can't be traded away.

| metric                                         | before | after |
|------------------------------------------------|--------|-------|
| objective (incl. priors)                       | 2.71   | 2.71  |
| weighted error                                 | 2.71   | 2.71  |
| harmonic error (dB)                            | 1.58   | 1.58  |
| resonant-peak shape error (dB)                 | 2.25   | 2.25  |
| inter-harmonic error (dB)                      | 0.81   | 0.81  |
| envelope error (dB)                            | 3.44   | 3.44  |
| spectrogram error (dB)                         | 8.19   | 8.19  |
| aligned waveform error (10*sqrt(residual/ref)) | 0.00   | 0.00  |
| resonant-peak sweep track error (semitones)    | 2.41   | 2.41  |
| harmonics-over-time error (dB)                 | 5.05   | 5.05  |
| mean |harmonic error| (dB)                     | 1.63   | 1.63  |
| note level error, RMS over notes (dB)          | 1.67   | 1.67  |
| harmonics within 1 dB (%)                      | 43.41  | 43.41 |
| harmonics within 3 dB (%)                      | 85.17  | 85.17 |
| harmonics within 6 dB (%)                      | 98.70  | 98.70 |

Recording gain solved as -12.4 dB (before: -12.4 dB).

## Per set

Mean error of each set's notes, and the RMS of their level errors (the recordings are not normalised, so absolute note levels are compared under one global gain).

| set | notes | before | after | level before (dB) | level after (dB) | harm | env | stft |
|-----|-------|--------|-------|-------------------|------------------|------|-----|------|
| P01 | 4     | 2.91   | 2.91  | 1.7               | 1.7              | 1.7  | 3.7 | 10.0 |
| P02 | 4     | 2.64   | 2.64  | 1.4               | 1.4              | 0.9  | 2.9 | 14.6 |
| P03 | 4     | 4.51   | 4.51  | 1.3               | 1.3              | 2.0  | 2.7 | 17.2 |
| P04 | 4     | 2.06   | 2.06  | 2.1               | 2.1              | 1.3  | 4.3 | 6.1  |
| P05 | 4     | 1.66   | 1.66  | 1.3               | 1.3              | 1.4  | 2.7 | 4.6  |
| P06 | 4     | 1.61   | 1.61  | 2.2               | 2.2              | 1.9  | 2.8 | 5.2  |
| P07 | 4     | 3.92   | 3.92  | 1.2               | 1.2              | 1.0  | 3.6 | 5.3  |
| P08 | 4     | 2.41   | 2.41  | 1.9               | 1.9              | 1.2  | 6.8 | 7.6  |
| P09 | 4     | 2.50   | 2.50  | 1.7               | 1.7              | 2.5  | 2.7 | 5.8  |
| P10 | 4     | 3.56   | 3.56  | 2.2               | 2.2              | 2.8  | 2.5 | 6.6  |
| P11 | 2     | 2.44   | 2.44  | 0.3               | 0.3              | 0.8  | 3.7 | 7.5  |
| P12 | 2     | 1.51   | 1.51  | 1.0               | 1.0              | 0.6  | 2.6 | 6.5  |

## Per sample (sorted by remaining error, worst first)

| sample | before | after | harm | peak | inter | env | stft | wave | harm ±3dB | level before | level after | tuning (c) | drift (c) |
|--------|--------|-------|------|------|-------|-----|------|------|-----------|--------------|-------------|------------|-----------|
| P07-4  | 9.85   | 9.85  | 0.8  | 24.8 | 2.0   | 2.0 | 7.2  | 0.0  | 96%       | +0.9         | +0.9        | +0.0       | 0.00      |
| P03-2  | 6.48   | 6.48  | 2.1  | 7.9  | 2.4   | 2.6 | 28.1 | 0.0  | 77%       | +1.5         | +1.5        | +0.0       | 0.00      |
| P03-4  | 5.03   | 5.03  | 2.4  | 9.1  | 1.8   | 3.5 | 5.6  | 0.0  | 51%       | +1.5         | +1.5        | +0.0       | 0.00      |
| P10-4  | 4.62   | 4.62  | 5.5  | 8.0  | 4.8   | 4.2 | 4.8  | 0.0  | 31%       | +4.0         | +4.0        | +0.0       | 0.00      |
| P10-2  | 4.42   | 4.42  | 1.7  | 7.0  | 0.6   | 1.7 | 8.4  | 0.0  | 76%       | +0.7         | +0.7        | +0.0       | 0.00      |
| P03-1  | 4.11   | 4.11  | 1.8  | 1.7  | 0.1   | 2.3 | 28.5 | 0.0  | 84%       | +1.1         | +1.1        | +0.0       | 0.00      |
| P09-4  | 3.59   | 3.59  | 3.8  | 2.8  | 0.4   | 2.7 | 4.4  | 0.0  | 75%       | -2.0         | -2.0        | +0.0       | 0.00      |
| P02-2  | 3.58   | 3.58  | 0.7  | 0.6  | 0.6   | 2.5 | 19.7 | 0.0  | 100%      | +1.4         | +1.4        | +0.0       | 0.00      |
| P01-2  | 3.34   | 3.34  | 2.6  | 3.0  | 1.1   | 2.7 | 17.8 | 0.0  | 61%       | +2.6         | +2.6        | +0.0       | 0.00      |
| P02-1  | 3.33   | 3.33  | 0.9  | 0.3  | 0.0   | 2.4 | 27.7 | 0.0  | 100%      | +1.5         | +1.5        | +0.0       | 0.00      |
| P01-1  | 3.21   | 3.21  | 0.9  | 2.2  | 0.1   | 7.8 | 7.7  | 0.0  | 96%       | +0.8         | +0.8        | +0.0       | 0.00      |
| P08-4  | 3.12   | 3.12  | 1.8  | 0.9  | 1.8   | 8.4 | 5.8  | 0.0  | 80%       | +2.9         | +2.9        | +0.0       | 0.00      |
| P10-3  | 2.77   | 2.77  | 3.2  | 4.2  | 1.1   | 2.4 | 6.2  | 0.0  | 33%       | +2.0         | +2.0        | +0.0       | 0.00      |
| P09-3  | 2.75   | 2.75  | 4.4  | 2.4  | 0.9   | 3.0 | 5.0  | 0.0  | 31%       | -2.0         | -2.0        | +0.0       | 0.00      |
| P08-1  | 2.73   | 2.73  | 0.7  | 0.3  | 0.6   | 7.7 | 12.4 | 0.0  | 100%      | +0.4         | +0.4        | +0.0       | 0.00      |
| P04-1  | 2.65   | 2.65  | 1.4  | 1.2  | 2.4   | 6.6 | 5.0  | 0.0  | 95%       | +1.9         | +1.9        | +0.0       | 0.00      |
| P11-1  | 2.56   | 2.56  | 0.9  | 0.6  | 0.5   | 5.4 | 6.9  | 0.0  | 100%      | +0.3         | +0.3        | +0.0       | 0.00      |
| P01-4  | 2.55   | 2.55  | 2.2  | 3.7  | 1.0   | 2.4 | 6.9  | 0.0  | 74%       | +1.9         | +1.9        | +0.0       | 0.00      |
| P01-3  | 2.52   | 2.52  | 1.0  | 2.1  | 0.1   | 2.0 | 7.5  | 0.0  | 96%       | +0.6         | +0.6        | +0.0       | 0.00      |
| P10-1  | 2.43   | 2.43  | 0.9  | 1.5  | 0.0   | 1.9 | 6.9  | 0.0  | 98%       | +0.3         | +0.3        | +0.0       | 0.00      |
| P03-3  | 2.42   | 2.42  | 1.5  | 2.0  | 0.1   | 2.2 | 6.7  | 0.0  | 84%       | +0.9         | +0.9        | +0.0       | 0.00      |
| P07-3  | 2.40   | 2.40  | 1.6  | 2.2  | 0.7   | 5.2 | 4.5  | 0.0  | 85%       | +0.9         | +0.9        | +0.0       | 0.00      |
| P08-2  | 2.34   | 2.34  | 1.4  | 0.2  | 2.0   | 8.2 | 8.7  | 0.0  | 98%       | +2.0         | +2.0        | +0.0       | 0.00      |
| P11-2  | 2.32   | 2.32  | 0.7  | 0.6  | 0.0   | 2.0 | 8.2  | 0.0  | 98%       | +0.3         | +0.3        | +0.0       | 0.00      |
| P04-3  | 2.18   | 2.18  | 1.3  | 0.6  | 0.9   | 3.8 | 7.1  | 0.0  | 90%       | +2.6         | +2.6        | +0.0       | 0.00      |
| P05-3  | 2.15   | 2.15  | 1.4  | 1.0  | 0.3   | 1.8 | 6.9  | 0.0  | 88%       | +1.5         | +1.5        | +0.0       | 0.00      |
| P04-4  | 2.11   | 2.11  | 1.2  | 0.6  | 1.1   | 4.9 | 5.8  | 0.0  | 90%       | +2.6         | +2.6        | +0.0       | 0.00      |
| P02-4  | 2.02   | 2.02  | 0.9  | 0.8  | 0.0   | 2.7 | 7.0  | 0.0  | 100%      | +1.4         | +1.4        | +0.0       | 0.00      |
| P09-2  | 1.95   | 1.95  | 0.9  | 0.7  | 0.1   | 2.6 | 6.7  | 0.0  | 100%      | +1.4         | +1.4        | +0.0       | 0.00      |
| P07-2  | 1.87   | 1.87  | 1.0  | 0.8  | 0.9   | 4.7 | 4.0  | 0.0  | 100%      | +1.4         | +1.4        | +0.0       | 0.00      |
| P05-4  | 1.75   | 1.75  | 1.9  | 0.4  | 1.1   | 4.8 | 4.2  | 0.0  | 64%       | +1.0         | +1.0        | +0.0       | 0.00      |
| P06-3  | 1.73   | 1.73  | 1.9  | 0.4  | 0.8   | 2.6 | 5.7  | 0.0  | 94%       | +2.9         | +2.9        | +0.0       | 0.00      |
| P09-1  | 1.73   | 1.73  | 0.9  | 0.4  | 0.0   | 2.5 | 7.2  | 0.0  | 100%      | +1.5         | +1.5        | +0.0       | 0.00      |
| P06-4  | 1.72   | 1.72  | 3.1  | 0.4  | 1.5   | 5.5 | 3.9  | 0.0  | 42%       | +2.8         | +2.8        | +0.0       | 0.00      |
| P02-3  | 1.63   | 1.63  | 0.9  | 0.4  | 1.1   | 4.0 | 3.9  | 0.0  | 100%      | +1.5         | +1.5        | +0.0       | 0.00      |
| P07-1  | 1.58   | 1.58  | 0.8  | 0.4  | 0.1   | 2.6 | 5.5  | 0.0  | 100%      | +1.5         | +1.5        | +0.0       | 0.00      |
| P12-1  | 1.58   | 1.58  | 0.6  | 0.3  | 0.0   | 2.6 | 7.3  | 0.0  | 100%      | +1.0         | +1.0        | +0.0       | 0.00      |
| P06-2  | 1.54   | 1.54  | 0.8  | 0.4  | 0.0   | 1.3 | 6.7  | 0.0  | 100%      | +1.1         | +1.1        | +0.0       | 0.00      |
| P06-1  | 1.46   | 1.46  | 1.8  | 0.4  | 0.2   | 1.9 | 4.5  | 0.0  | 70%       | +1.5         | +1.5        | +0.0       | 0.00      |
| P08-3  | 1.46   | 1.46  | 0.8  | 0.4  | 1.0   | 2.8 | 3.7  | 0.0  | 100%      | +1.1         | +1.1        | +0.0       | 0.00      |
| P12-2  | 1.45   | 1.45  | 0.7  | 0.3  | 0.0   | 2.6 | 5.8  | 0.0  | 100%      | +1.0         | +1.0        | +0.0       | 0.00      |
| P05-2  | 1.39   | 1.39  | 0.8  | 0.4  | 0.3   | 1.9 | 4.4  | 0.0  | 100%      | +1.1         | +1.1        | +0.0       | 0.00      |
| P05-1  | 1.34   | 1.34  | 1.6  | 0.4  | 1.1   | 2.1 | 2.8  | 0.0  | 90%       | +1.5         | +1.5        | +0.0       | 0.00      |
| P04-2  | 1.31   | 1.31  | 1.3  | 0.4  | 0.0   | 2.0 | 6.4  | 0.0  | 100%      | +0.2         | +0.2        | +0.0       | 0.00      |

### Resonant peak (Resonance=max samples)

| sample | peak Hz (hw) | peak Hz (before) | peak Hz (after) | height dB (hw) | height dB (before) | height dB (after) |
|--------|--------------|------------------|-----------------|----------------|--------------------|-------------------|
| P01-1  | 327          | 14618            | 14618           | -4.2           | -147.0             | -147.0            |
| P01-2  | 1668         | 1733             | 1733            | -6.6           | -9.6               | -9.6              |
| P01-3  | 327          | 392              | 392             | -1.9           | -2.8               | -2.8              |
| P01-4  | 1701         | 1766             | 1766            | -3.9           | -7.6               | -7.6              |
| P02-1  | 33           | 33               | 33              | +0.0           | +0.0               | +0.0              |
| P02-2  | 33           | 33               | 33              | +0.0           | +0.0               | +0.0              |
| P02-3  | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| P02-4  | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| P03-1  | 1406         | 1275             | 1275            | -16.5          | -16.2              | -16.2             |
| P03-2  | 2616         | 2485             | 2485            | -12.5          | -18.2              | -18.2             |
| P03-3  | 1374         | 1308             | 1308            | -13.0          | -13.4              | -13.4             |
| P03-4  | 2616         | 2485             | 2485            | -11.5          | -17.1              | -17.1             |
| P04-1  | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| P04-2  | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| P04-3  | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| P04-4  | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| P05-1  | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| P05-2  | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| P05-3  | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| P05-4  | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| P06-1  | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| P06-2  | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| P06-3  | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| P06-4  | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| P07-1  | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| P07-2  | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| P07-3  | 1504         | 1439             | 1439            | -13.2          | -15.0              | -15.0             |
| P07-4  | 7783         | 5887             | 5887            | -26.2          | -28.6              | -28.6             |
| P08-1  | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| P08-2  | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| P08-3  | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| P08-4  | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| P09-1  | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| P09-2  | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| P09-3  | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| P09-4  | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| P10-1  | 654          | 458              | 458             | -9.5           | -4.4               | -4.4              |
| P10-2  | 1439         | 1243             | 1243            | -8.8           | -11.8              | -11.8             |
| P10-3  | 1701         | 65               | 65              | -4.9           | +0.0               | +0.0              |
| P10-4  | 5756         | 65               | 65              | -16.9          | +0.0               | +0.0              |
| P11-1  | 65           | 10465            | 10465           | +0.0           | -137.4             | -137.4            |
| P11-2  | 65           | 10465            | 10465           | +0.0           | -137.4             | -137.4            |
| P12-1  | 65           | 14782            | 14782           | +0.0           | -138.5             | -138.5            |
| P12-2  | 65           | 14782            | 14782           | +0.0           | -138.5             | -138.5            |

Height is the resonant peak's level above the fundamental (gain-independent). This is the number that answers "is the acid squelch there": before/after collapsing to something much smaller than hardware's means the peak got flattened away, not just shifted.

`tuning` is measured from the harmonic peaks relative to equal temperament; `drift` is the std-dev of short-time pitch over the note (hardware fluctuation).

### Worst remaining harmonics / spurious high-frequency content

- **P01-1** worst: k13 425Hz ref -8.7 / sim -3.6, k14 458Hz ref -10.2 / sim -6.0, k10 327Hz ref -4.2 / sim -7.6, k15 490Hz ref -11.7 / sim -8.8. Model excess above 4 kHz: none > 3 dB.
- **P01-2** worst: k145 4742Hz ref -60.4 / sim -54.2, k146 4775Hz ref -60.1 / sim -54.5, k111 3630Hz ref -47.4 / sim -42.5, k147 4807Hz ref -59.7 / sim -54.8. Model excess above 4 kHz: harmonic 4742Hz +6.2dB, harmonic 4709Hz +5.9dB, harmonic 4677Hz +5.6dB.
- **P01-3** worst: k7 458Hz ref -7.6 / sim -3.7, k5 327Hz ref -1.9 / sim -5.3, k6 392Hz ref -4.6 / sim -2.1, k32 2093Hz ref -61.2 / sim -63.7. Model excess above 4 kHz: none > 3 dB.
- **P01-4** worst: k93 6083Hz ref -64.6 / sim -60.2, k3 196Hz ref -8.3 / sim -4.3, k4 262Hz ref -10.5 / sim -6.6, k2 131Hz ref -4.9 / sim -1.2. Model excess above 4 kHz: harmonic 6083Hz +4.4dB, harmonic 6148Hz +4.3dB, harmonic 6017Hz +3.7dB.
- **P02-1** worst: k6 196Hz ref -10.3 / sim -8.1, k5 164Hz ref -8.9 / sim -6.8, k7 229Hz ref -11.3 / sim -9.2, k4 131Hz ref -7.2 / sim -5.2. Model excess above 4 kHz: none > 3 dB.
- **P02-2** worst: k8 262Hz ref -11.4 / sim -8.7, k7 229Hz ref -10.9 / sim -8.3, k6 196Hz ref -10.0 / sim -7.6, k9 294Hz ref -11.5 / sim -9.3. Model excess above 4 kHz: none > 3 dB.
- **P02-3** worst: k3 196Hz ref -7.9 / sim -5.6, k2 131Hz ref -4.8 / sim -2.7, k4 262Hz ref -9.7 / sim -7.7, k1 65Hz ref 0.0 / sim 1.8. Model excess above 4 kHz: none > 3 dB.
- **P02-4** worst: k4 262Hz ref -9.0 / sim -6.2, k3 196Hz ref -7.7 / sim -5.1, k2 131Hz ref -4.8 / sim -2.7, k26 1701Hz ref -21.6 / sim -23.3. Model excess above 4 kHz: none > 3 dB.
- **P03-1** worst: k20 654Hz ref -9.3 / sim -13.1, k85 2780Hz ref -56.2 / sim -52.4, k86 2812Hz ref -56.5 / sim -52.8, k84 2747Hz ref -55.4 / sim -52.0. Model excess above 4 kHz: harmonic 4742Hz +5.6dB, harmonic 4775Hz +5.6dB, harmonic 4709Hz +5.6dB.
- **P03-2** worst: k155 5069Hz ref -58.2 / sim -51.8, k47 1537Hz ref -23.5 / sim -17.4, k24 785Hz ref -15.4 / sim -9.5, k25 818Hz ref -16.1 / sim -10.3. Model excess above 4 kHz: inter-harmonic 5020Hz +7.9dB, inter-harmonic 4987Hz +7.7dB, inter-harmonic 5053Hz +7.5dB, harmonic 5036Hz +7.0dB.
- **P03-3** worst: k81 5298Hz ref -79.8 / sim -75.3, k80 5232Hz ref -79.2 / sim -74.7, k79 5167Hz ref -78.6 / sim -74.2, k78 5102Hz ref -78.0 / sim -73.7. Model excess above 4 kHz: harmonic 5298Hz +4.5dB, harmonic 5232Hz +4.5dB, harmonic 5167Hz +4.4dB.
- **P03-4** worst: k77 5036Hz ref -56.0 / sim -48.8, k76 4971Hz ref -55.6 / sim -48.5, k75 4906Hz ref -55.2 / sim -48.4, k74 4840Hz ref -55.0 / sim -48.4. Model excess above 4 kHz: harmonic 5036Hz +7.2dB, inter-harmonic 4611Hz +7.2dB, harmonic 4971Hz +7.1dB, inter-harmonic 4546Hz +7.1dB.
- **P04-1** worst: k5 327Hz ref -8.6 / sim -4.6, k4 262Hz ref -8.7 / sim -5.1, k18 1177Hz ref -21.2 / sim -24.6, k3 196Hz ref -7.4 / sim -4.3. Model excess above 4 kHz: inter-harmonic 4088Hz +8.3dB, inter-harmonic 4153Hz +7.5dB, inter-harmonic 4219Hz +6.7dB.
- **P04-2** worst: k46 3009Hz ref -22.7 / sim -25.5, k45 2943Hz ref -22.4 / sim -25.2, k44 2878Hz ref -22.1 / sim -24.9, k47 3074Hz ref -23.1 / sim -25.9. Model excess above 4 kHz: none > 3 dB.
- **P04-3** worst: k5 327Hz ref -10.5 / sim -6.9, k4 262Hz ref -9.7 / sim -6.2, k3 196Hz ref -7.8 / sim -4.5, k2 131Hz ref -4.7 / sim -1.7. Model excess above 4 kHz: inter-harmonic 4088Hz +3.6dB, inter-harmonic 4153Hz +3.5dB, inter-harmonic 4219Hz +3.5dB.
- **P04-4** worst: k5 327Hz ref -10.5 / sim -6.9, k4 262Hz ref -9.7 / sim -6.2, k3 196Hz ref -7.8 / sim -4.5, k2 131Hz ref -4.7 / sim -1.7. Model excess above 4 kHz: inter-harmonic 4480Hz +3.8dB, inter-harmonic 4546Hz +3.8dB, inter-harmonic 4415Hz +3.8dB.
- **P05-1** worst: k34 2224Hz ref -35.7 / sim -39.5, k35 2289Hz ref -37.4 / sim -41.0, k36 2355Hz ref -38.9 / sim -42.3, k37 2420Hz ref -40.3 / sim -43.6. Model excess above 4 kHz: none > 3 dB.
- **P05-2** worst: k3 196Hz ref -7.9 / sim -6.0, k2 131Hz ref -4.8 / sim -3.0, k4 262Hz ref -9.8 / sim -8.1, k27 1766Hz ref -19.8 / sim -21.5. Model excess above 4 kHz: none > 3 dB.
- **P05-3** worst: k25 1635Hz ref -22.0 / sim -25.4, k5 327Hz ref -9.1 / sim -5.8, k24 1570Hz ref -21.6 / sim -25.0, k29 1897Hz ref -23.7 / sim -27.7. Model excess above 4 kHz: none > 3 dB.
- **P05-4** worst: k114 7456Hz ref -69.4 / sim -66.0, k112 7326Hz ref -68.6 / sim -65.2, k115 7522Hz ref -69.7 / sim -66.4, k118 7718Hz ref -70.9 / sim -67.5. Model excess above 4 kHz: inter-harmonic 4088Hz +3.6dB, harmonic 7456Hz +3.4dB, harmonic 7326Hz +3.4dB, harmonic 7522Hz +3.4dB.
- **P06-1** worst: k39 2551Hz ref -42.1 / sim -45.8, k38 2485Hz ref -40.8 / sim -44.8, k40 2616Hz ref -43.3 / sim -46.9, k37 2420Hz ref -39.5 / sim -43.6. Model excess above 4 kHz: none > 3 dB.
- **P06-2** worst: k3 196Hz ref -7.9 / sim -6.0, k2 131Hz ref -4.8 / sim -3.0, k4 262Hz ref -9.8 / sim -8.1, k26 1701Hz ref -19.0 / sim -20.7. Model excess above 4 kHz: none > 3 dB.
- **P06-3** worst: k3 196Hz ref -8.2 / sim -4.3, k4 262Hz ref -10.4 / sim -6.6, k2 131Hz ref -4.9 / sim -1.2, k5 327Hz ref -12.0 / sim -8.3. Model excess above 4 kHz: none > 3 dB.
- **P06-4** worst: k128 8372Hz ref -67.3 / sim -61.8, k129 8437Hz ref -67.6 / sim -62.1, k127 8307Hz ref -66.9 / sim -61.4, k126 8241Hz ref -66.5 / sim -61.1. Model excess above 4 kHz: harmonic 8372Hz +5.5dB, harmonic 8437Hz +5.5dB, harmonic 8503Hz +5.5dB, inter-harmonic 4415Hz +3.4dB.
- **P07-1** worst: k3 196Hz ref -7.8 / sim -5.6, k2 131Hz ref -4.8 / sim -2.7, k4 262Hz ref -9.7 / sim -7.7, k1 65Hz ref 0.0 / sim 1.8. Model excess above 4 kHz: none > 3 dB.
- **P07-2** worst: k4 262Hz ref -9.0 / sim -6.2, k3 196Hz ref -7.7 / sim -5.1, k2 131Hz ref -4.8 / sim -2.7, k28 1831Hz ref -22.3 / sim -24.0. Model excess above 4 kHz: none > 3 dB.
- **P07-3** worst: k84 5494Hz ref -79.8 / sim -75.0, k83 5429Hz ref -79.2 / sim -74.5, k82 5363Hz ref -78.7 / sim -74.1, k81 5298Hz ref -78.2 / sim -73.6. Model excess above 4 kHz: harmonic 5494Hz +4.7dB, harmonic 5429Hz +4.7dB, harmonic 5363Hz +4.6dB.
- **P07-4** worst: k163 10661Hz ref -56.4 / sim -67.2, k4 262Hz ref -9.4 / sim -6.6, k5 327Hz ref -9.2 / sim -6.5, k3 196Hz ref -7.8 / sim -5.6. Model excess above 4 kHz: inter-harmonic 4284Hz +11.4dB, inter-harmonic 5265Hz +7.2dB, inter-harmonic 5788Hz +6.3dB.
- **P08-1** worst: k41 2682Hz ref -22.5 / sim -24.6, k42 2747Hz ref -22.4 / sim -24.5, k40 2616Hz ref -22.8 / sim -24.8, k89 5821Hz ref -31.1 / sim -33.3. Model excess above 4 kHz: none > 3 dB.
- **P08-2** worst: k53 3466Hz ref -31.3 / sim -28.3, k54 3532Hz ref -31.9 / sim -28.9, k55 3597Hz ref -32.3 / sim -29.4, k56 3663Hz ref -32.6 / sim -30.1. Model excess above 4 kHz: inter-harmonic 7097Hz +6.6dB, inter-harmonic 7162Hz +6.6dB, inter-harmonic 7031Hz +6.5dB, harmonic 7326Hz +4.3dB.
- **P08-3** worst: k91 5952Hz ref -35.8 / sim -38.5, k92 6017Hz ref -36.0 / sim -38.9, k90 5887Hz ref -35.6 / sim -38.0, k89 5821Hz ref -35.4 / sim -37.6. Model excess above 4 kHz: none > 3 dB.
- **P08-4** worst: k3 196Hz ref -7.9 / sim -3.8, k4 262Hz ref -9.1 / sim -5.2, k2 131Hz ref -5.0 / sim -1.4, k49 3205Hz ref -31.7 / sim -28.2. Model excess above 4 kHz: inter-harmonic 5134Hz +6.6dB, inter-harmonic 5200Hz +6.5dB, inter-harmonic 5069Hz +6.5dB, harmonic 4971Hz +4.7dB.
- **P09-1** worst: k3 196Hz ref -7.9 / sim -5.6, k2 131Hz ref -4.8 / sim -2.7, k4 262Hz ref -9.7 / sim -7.6, k1 65Hz ref 0.0 / sim 1.8. Model excess above 4 kHz: none > 3 dB.
- **P09-2** worst: k4 262Hz ref -9.0 / sim -6.2, k3 196Hz ref -7.7 / sim -5.1, k2 131Hz ref -4.7 / sim -2.7, k23 1504Hz ref -20.4 / sim -22.1. Model excess above 4 kHz: none > 3 dB.
- **P09-3** worst: k15 981Hz ref -17.1 / sim -49.9, k13 850Hz ref -14.8 / sim -29.0, k17 1112Hz ref -19.5 / sim -32.4, k32 2093Hz ref -44.7 / sim -54.9. Model excess above 4 kHz: harmonic 4709Hz +6.8dB, harmonic 4840Hz +6.2dB, harmonic 4578Hz +6.1dB.
- **P09-4** worst: k15 981Hz ref -19.9 / sim -51.8, k13 850Hz ref -18.0 / sim -33.6, k17 1112Hz ref -22.0 / sim -35.8, k11 720Hz ref -16.2 / sim -26.3. Model excess above 4 kHz: harmonic 4121Hz +3.8dB, harmonic 4251Hz +3.0dB.
- **P10-1** worst: k5 327Hz ref -3.8 / sim -7.3, k13 850Hz ref -22.8 / sim -20.4, k7 458Hz ref -6.5 / sim -4.1, k12 785Hz ref -16.4 / sim -14.3. Model excess above 4 kHz: none > 3 dB.
- **P10-2** worst: k24 1570Hz ref -20.8 / sim -26.6, k25 1635Hz ref -23.9 / sim -29.0, k26 1701Hz ref -26.5 / sim -31.2, k61 3990Hz ref -61.9 / sim -67.3. Model excess above 4 kHz: none > 3 dB.
- **P10-3** worst: k98 6410Hz ref -62.1 / sim -56.4, k99 6475Hz ref -62.5 / sim -56.8, k122 7980Hz ref -72.0 / sim -66.4, k123 8045Hz ref -72.4 / sim -66.8. Model excess above 4 kHz: harmonic 6410Hz +5.7dB, harmonic 6475Hz +5.7dB, harmonic 7980Hz +5.7dB.
- **P10-4** worst: k177 11577Hz ref -69.0 / sim -56.4, k178 11642Hz ref -67.6 / sim -56.9, k179 11708Hz ref -67.5 / sim -57.4, k3 196Hz ref -8.3 / sim -2.4. Model excess above 4 kHz: harmonic 11512Hz +14.6dB, harmonic 11446Hz +14.0dB, harmonic 11381Hz +13.2dB, inter-harmonic 10105Hz +12.6dB.
- **P11-1** worst: k12 785Hz ref -28.4 / sim -25.4, k13 850Hz ref -32.0 / sim -29.2, k11 720Hz ref -24.1 / sim -21.4, k43 2812Hz ref -78.2 / sim -75.7. Model excess above 4 kHz: none > 3 dB.
- **P11-2** worst: k11 720Hz ref -24.6 / sim -21.5, k12 785Hz ref -27.9 / sim -25.4, k13 850Hz ref -31.3 / sim -29.2, k10 654Hz ref -20.0 / sim -18.0. Model excess above 4 kHz: none > 3 dB.
- **P12-1** worst: k5 327Hz ref -8.1 / sim -6.3, k2 131Hz ref -4.3 / sim -2.9, k13 850Hz ref -15.7 / sim -17.0, k6 392Hz ref -9.5 / sim -8.3. Model excess above 4 kHz: none > 3 dB.
- **P12-2** worst: k5 327Hz ref -8.1 / sim -6.3, k13 850Hz ref -15.6 / sim -17.0, k2 131Hz ref -4.3 / sim -2.9, k12 785Hz ref -14.6 / sim -15.9. Model excess above 4 kHz: none > 3 dB.

## Suspicious samples

- **P11-1**: spectrum is within 0.4 dB of P11-2 although the accent label differs -- one of the two labels is probably wrong
- **P07-4**: error 9.85 is 4.2x the median
- **P03-2**: error 6.48 is 2.7x the median
- **P03-4**: error 5.03 is 2.1x the median
- **P10-4**: error 4.62 is 2.0x the median
- **P10-2**: error 4.42 is 1.9x the median
- **P03-1**: error 4.11 is 1.7x the median
- **P09-4**: error 3.59 is 1.5x the median
- **P02-2**: error 3.58 is 1.5x the median

## Fitted knob positions

| knob (label)     | nominal | fitted | sensitivity |
|------------------|---------|--------|-------------|
| accent (x0)      | 0.00    | 0.000  | 0.000       |
| accent (x100)    | 1.00    | 1.000  | 0.000       |
| cutoff (x100)    | 1.00    | 1.000  | 0.000       |
| cutoff (x25)     | 0.25    | 0.252  | 0.000       |
| cutoff (x50)     | 0.50    | 0.504  | 0.000       |
| cutoff (x74)     | 0.75    | 0.748  | 0.000       |
| decay (x0)       | 0.00    | 0.000  | 0.000       |
| decay (x100)     | 1.00    | 1.000  | 0.000       |
| decay (x25)      | 0.25    | 0.252  | 0.000       |
| decay (x50)      | 0.50    | 0.504  | 0.000       |
| decay (x74)      | 0.75    | 0.748  | 0.000       |
| envMod (x0)      | 0.00    | 0.000  | 0.000       |
| envMod (x100)    | 1.00    | 1.000  | 0.000       |
| envMod (x25)     | 0.25    | 0.252  | 0.000       |
| envMod (x50)     | 0.50    | 0.504  | 0.000       |
| envMod (x74)     | 0.75    | 0.748  | 0.000       |
| resonance (x100) | 1.00    | 1.000  | 0.000       |
| resonance (x74)  | 0.75    | 0.748  | 0.000       |

## Fitted model parameters

`sensitivity` = objective increase for a 5% (of range) nudge. Near-zero means the current references don't constrain that parameter -- its value is not evidence of anything. `AT BOUND` means the optimizer pushed it to the edge of its plausible range, usually a sign that it is compensating for something the model does not implement.

| parameter            | before   | after    | sensitivity |                                     |
|----------------------|----------|----------|-------------|-------------------------------------|
| oscSquareLevel       | 0.38901  | 0.38901  | 0.000       |                                     |
| filterFeedbackGain   | 16.803   | 16.803   | 0.000       |                                     |
| filterResonanceLimit | 0.92752  | 0.92752  | 0.000       |                                     |
| cutoffBaseHz         | 264.11   | 264.11   | 0.000       |                                     |
| cutoffSpanOct        | 3.1565   | 3.1565   | 0.000       |                                     |
| envModScaleC0Slope   | 4.9925   | 4.9925   | 0.000       |                                     |
| envModScaleC1Slope   | 3.3817   | 3.3817   | 0.000       |                                     |
| envModOffset         | 0.38205  | 0.38205  | 0.000       |                                     |
| accentSweepDepthOct  | 5.6056   | 5.6056   | 0.000       |                                     |
| accentVcaDepth       | 2.3943   | 2.3943   | 0.000       |                                     |
| vcfDecayMinSec       | 0.063658 | 0.063658 | 0.000       |                                     |
| vcfDecayMaxSec       | 1.0691   | 1.0691   | 0.000       |                                     |
| accentDecaySec       | 0.084449 | 0.084449 | 0.000       |                                     |
| vegDecaySec          | 1.009    | 1.009    | 0.000       | AT BOUND (model may lack structure) |
| vcaResTapRatio       | 1.6824   | 1.6824   | 0.000       |                                     |

Timing: note-on offset 0.00 ms (relative to each clip's measured note-on), gate length 1300.0 ms.

## Applying the result

Paste `synth_parameters.txt` into `acidus::SynthParameters` (src/core/SynthEngine.hpp) or rerun with `--apply`.

Audio: `renders/` (hardware / before / after for the 40 worst samples).
