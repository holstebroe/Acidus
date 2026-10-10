# Acidus vs. hardware TB-303 calibration report (20261010-102230)

44 reference samples, 19 free parameters. Search: 205 evaluations in 10.1 min, 0 CMA-ES restarts, stopped because: no progress for 600 s.

## Match quality (mean over samples)

All errors are dB; lower is better. `harm` = full-note harmonic levels (k^-0.5 weighted RMS), `inter` = energy between harmonics, `env` = RMS envelope, `stft` = 1/3-octave levels over time, `peak` = shape (height+width+implicit frequency) of the resonant-peak window on Resonance=max samples only -- weighted more heavily than `harm` so it can't be traded away.

| metric                                         | before | after |
|------------------------------------------------|--------|-------|
| objective (incl. priors)                       | 3.18   | 3.18  |
| weighted error                                 | 3.18   | 3.18  |
| harmonic error (dB)                            | 1.89   | 1.89  |
| resonant-peak shape error (dB)                 | 3.17   | 3.17  |
| inter-harmonic error (dB)                      | 0.67   | 0.67  |
| envelope error (dB)                            | 3.32   | 3.32  |
| spectrogram error (dB)                         | 8.73   | 8.73  |
| aligned waveform error (10*sqrt(residual/ref)) | 0.00   | 0.00  |
| resonant-peak sweep track error (semitones)    | 2.71   | 2.71  |
| harmonics-over-time error (dB)                 | 5.76   | 5.76  |
| mean |harmonic error| (dB)                     | 2.24   | 2.24  |
| note level error, RMS over notes (dB)          | 1.71   | 1.71  |
| harmonics within 1 dB (%)                      | 34.57  | 34.57 |
| harmonics within 3 dB (%)                      | 75.29  | 75.29 |
| harmonics within 6 dB (%)                      | 93.15  | 93.15 |

Recording gain solved as -11.2 dB (before: -11.2 dB).

## Per set

Mean error of each set's notes, and the RMS of their level errors (the recordings are not normalised, so absolute note levels are compared under one global gain).

| set | notes | before | after | level before (dB) | level after (dB) | harm | env | stft |
|-----|-------|--------|-------|-------------------|------------------|------|-----|------|
| P01 | 4     | 3.06   | 3.06  | 0.3               | 0.3              | 1.3  | 3.4 | 10.1 |
| P02 | 4     | 3.33   | 3.33  | 0.4               | 0.4              | 1.1  | 2.2 | 15.2 |
| P03 | 4     | 5.10   | 5.10  | 5.1               | 5.1              | 2.2  | 4.0 | 17.6 |
| P04 | 4     | 2.59   | 2.59  | 1.0               | 1.0              | 1.6  | 4.0 | 5.8  |
| P05 | 4     | 2.76   | 2.76  | 0.5               | 0.5              | 2.6  | 2.4 | 5.4  |
| P06 | 4     | 2.51   | 2.51  | 1.2               | 1.2              | 2.0  | 2.5 | 5.7  |
| P07 | 4     | 3.42   | 3.42  | 0.5               | 0.5              | 1.1  | 3.5 | 5.5  |
| P08 | 4     | 3.21   | 3.21  | 0.9               | 0.9              | 1.1  | 6.7 | 8.2  |
| P09 | 4     | 3.07   | 3.07  | 1.3               | 1.3              | 3.2  | 2.6 | 7.3  |
| P10 | 4     | 2.72   | 2.72  | 0.5               | 0.5              | 1.8  | 2.3 | 6.7  |
| P11 | 2     | 3.46   | 3.46  | 1.0               | 1.0              | 2.8  | 3.6 | 9.0  |
| P12 | 2     | 2.90   | 2.90  | 0.1               | 0.1              | 2.6  | 2.3 | 8.1  |

## Per sample (sorted by remaining error, worst first)

| sample | before | after | harm | peak | inter | env | stft | wave | harm ±3dB | level before | level after | tuning (c) | drift (c) |
|--------|--------|-------|------|------|-------|-----|------|------|-----------|--------------|-------------|------------|-----------|
| P03-1  | 8.58   | 8.58  | 4.0  | 7.9  | 0.0   | 8.2 | 28.2 | 0.0  | 0%        | +10.0        | +10.0       | +0.0       | 0.00      |
| P03-2  | 6.09   | 6.09  | 1.9  | 6.1  | 1.4   | 2.3 | 28.7 | 0.0  | 77%       | -1.2         | -1.2        | +0.0       | 0.00      |
| P07-4  | 5.49   | 5.49  | 0.8  | 12.2 | 1.9   | 2.6 | 6.6  | 0.0  | 96%       | -0.6         | -0.6        | +0.0       | 0.00      |
| P02-1  | 4.33   | 4.33  | 1.1  | 2.8  | 0.0   | 2.3 | 28.5 | 0.0  | 100%      | +0.2         | +0.2        | +0.0       | 0.00      |
| P02-2  | 4.30   | 4.30  | 0.8  | 2.9  | 0.9   | 2.2 | 20.8 | 0.0  | 100%      | -0.0         | -0.0        | +0.0       | 0.00      |
| P01-1  | 3.84   | 3.84  | 1.1  | 4.0  | 0.0   | 7.1 | 8.1  | 0.0  | 95%       | +0.2         | +0.2        | +0.0       | 0.00      |
| P09-3  | 3.78   | 3.78  | 5.1  | 4.1  | 2.3   | 2.8 | 8.1  | 0.0  | 26%       | +1.9         | +1.9        | +0.0       | 0.00      |
| P09-4  | 3.62   | 3.62  | 5.6  | 4.2  | 1.8   | 3.0 | 6.9  | 0.0  | 38%       | +1.7         | +1.7        | +0.0       | 0.00      |
| P11-1  | 3.60   | 3.60  | 3.1  | 1.9  | 0.0   | 5.1 | 8.5  | 0.0  | 24%       | -1.0         | -1.0        | +0.0       | 0.00      |
| P07-3  | 3.55   | 3.55  | 1.3  | 5.0  | 0.0   | 5.1 | 5.3  | 0.0  | 93%       | -0.3         | -0.3        | +0.0       | 0.00      |
| P08-1  | 3.47   | 3.47  | 1.1  | 2.6  | 0.6   | 8.4 | 12.4 | 0.0  | 94%       | -1.5         | -1.5        | +0.0       | 0.00      |
| P11-2  | 3.32   | 3.32  | 2.6  | 1.9  | 0.0   | 2.2 | 9.5  | 0.0  | 24%       | -1.0         | -1.0        | +0.0       | 0.00      |
| P05-4  | 3.29   | 3.29  | 4.2  | 2.5  | 2.4   | 4.6 | 5.2  | 0.0  | 38%       | -0.0         | -0.0        | +0.0       | 0.00      |
| P01-2  | 3.23   | 3.23  | 1.4  | 3.3  | 0.2   | 2.2 | 17.6 | 0.0  | 86%       | -0.1         | -0.1        | +0.0       | 0.00      |
| P10-1  | 3.19   | 3.19  | 1.1  | 3.3  | 0.0   | 1.7 | 7.4  | 0.0  | 94%       | -0.1         | -0.1        | +0.0       | 0.00      |
| P08-2  | 3.18   | 3.18  | 1.1  | 2.3  | 1.6   | 7.9 | 9.0  | 0.0  | 89%       | -0.2         | -0.2        | +0.0       | 0.00      |
| P08-4  | 3.09   | 3.09  | 1.4  | 2.7  | 1.4   | 8.1 | 5.1  | 0.0  | 98%       | +1.1         | +1.1        | +0.0       | 0.00      |
| P08-3  | 3.08   | 3.08  | 0.7  | 2.5  | 0.8   | 2.5 | 6.4  | 0.0  | 100%      | -0.1         | -0.1        | +0.0       | 0.00      |
| P04-1  | 3.07   | 3.07  | 1.3  | 3.0  | 2.7   | 6.3 | 4.6  | 0.0  | 90%       | +1.0         | +1.0        | +0.0       | 0.00      |
| P01-3  | 3.05   | 3.05  | 1.3  | 3.4  | 0.1   | 2.0 | 8.0  | 0.0  | 91%       | +0.3         | +0.3        | +0.0       | 0.00      |
| P10-4  | 3.02   | 3.02  | 3.6  | 4.1  | 2.4   | 3.5 | 4.3  | 0.0  | 62%       | +0.1         | +0.1        | +0.0       | 0.00      |
| P10-2  | 2.94   | 2.94  | 1.4  | 1.3  | 0.0   | 1.8 | 9.3  | 0.0  | 95%       | -0.7         | -0.7        | +0.0       | 0.00      |
| P12-1  | 2.91   | 2.91  | 2.5  | 2.2  | 0.0   | 2.3 | 8.6  | 0.0  | 27%       | +0.1         | +0.1        | +0.0       | 0.00      |
| P03-3  | 2.90   | 2.90  | 1.1  | 3.4  | 0.0   | 2.2 | 6.9  | 0.0  | 97%       | -0.3         | -0.3        | +0.0       | 0.00      |
| P12-2  | 2.89   | 2.89  | 2.7  | 2.2  | 0.1   | 2.3 | 7.6  | 0.0  | 27%       | +0.1         | +0.1        | +0.0       | 0.00      |
| P03-4  | 2.81   | 2.81  | 1.8  | 1.7  | 0.6   | 3.4 | 6.5  | 0.0  | 64%       | -0.9         | -0.9        | +0.0       | 0.00      |
| P05-1  | 2.72   | 2.72  | 2.6  | 2.5  | 0.9   | 1.8 | 4.4  | 0.0  | 32%       | +0.7         | +0.7        | +0.0       | 0.00      |
| P06-1  | 2.70   | 2.70  | 2.2  | 2.4  | 0.2   | 1.6 | 5.5  | 0.0  | 52%       | +0.7         | +0.7        | +0.0       | 0.00      |
| P06-2  | 2.70   | 2.70  | 1.9  | 2.4  | 0.0   | 1.2 | 7.4  | 0.0  | 80%       | +0.2         | +0.2        | +0.0       | 0.00      |
| P09-1  | 2.59   | 2.59  | 1.5  | 2.5  | 0.0   | 2.3 | 7.6  | 0.0  | 92%       | +0.6         | +0.6        | +0.0       | 0.00      |
| P04-2  | 2.59   | 2.59  | 3.2  | 2.7  | 0.6   | 2.2 | 6.5  | 0.0  | 42%       | -1.1         | -1.1        | +0.0       | 0.00      |
| P05-2  | 2.58   | 2.58  | 2.0  | 2.4  | 0.3   | 1.6 | 5.5  | 0.0  | 73%       | +0.2         | +0.2        | +0.0       | 0.00      |
| P07-1  | 2.48   | 2.48  | 1.7  | 2.5  | 0.1   | 2.3 | 6.1  | 0.0  | 89%       | +0.6         | +0.6        | +0.0       | 0.00      |
| P06-3  | 2.45   | 2.45  | 1.9  | 2.7  | 0.5   | 2.0 | 5.7  | 0.0  | 88%       | +1.8         | +1.8        | +0.0       | 0.00      |
| P05-3  | 2.44   | 2.44  | 1.6  | 2.9  | 1.3   | 1.5 | 6.5  | 0.0  | 91%       | +0.6         | +0.6        | +0.0       | 0.00      |
| P02-3  | 2.38   | 2.38  | 1.6  | 2.5  | 0.9   | 2.0 | 4.7  | 0.0  | 91%       | +0.6         | +0.6        | +0.0       | 0.00      |
| P04-3  | 2.36   | 2.36  | 1.0  | 2.6  | 0.6   | 3.2 | 6.8  | 0.0  | 95%       | +0.9         | +0.9        | +0.0       | 0.00      |
| P04-4  | 2.35   | 2.35  | 1.0  | 2.6  | 0.5   | 4.4 | 5.4  | 0.0  | 95%       | +0.9         | +0.9        | +0.0       | 0.00      |
| P09-2  | 2.30   | 2.30  | 0.7  | 2.6  | 0.0   | 2.3 | 6.7  | 0.0  | 100%      | +0.4         | +0.4        | +0.0       | 0.00      |
| P02-4  | 2.30   | 2.30  | 0.8  | 2.6  | 0.0   | 2.4 | 6.9  | 0.0  | 100%      | +0.4         | +0.4        | +0.0       | 0.00      |
| P06-4  | 2.20   | 2.20  | 2.0  | 2.7  | 1.1   | 5.1 | 4.0  | 0.0  | 74%       | +1.5         | +1.5        | +0.0       | 0.00      |
| P07-2  | 2.16   | 2.16  | 0.8  | 2.6  | 0.7   | 3.9 | 3.8  | 0.0  | 100%      | +0.4         | +0.4        | +0.0       | 0.00      |
| P01-4  | 2.12   | 2.12  | 1.5  | 2.6  | 0.2   | 2.1 | 6.6  | 0.0  | 96%       | -0.4         | -0.4        | +0.0       | 0.00      |
| P10-3  | 1.71   | 1.71  | 1.2  | 2.1  | 0.3   | 2.1 | 5.7  | 0.0  | 98%       | -0.6         | -0.6        | +0.0       | 0.00      |

### Resonant peak (Resonance=max samples)

| sample | peak Hz (hw) | peak Hz (before) | peak Hz (after) | height dB (hw) | height dB (before) | height dB (after) |
|--------|--------------|------------------|-----------------|----------------|--------------------|-------------------|
| P01-1  | 327          | 425              | 425             | -4.2           | -0.7               | -0.7              |
| P01-2  | 1668         | 1733             | 1733            | -6.6           | -5.2               | -5.2              |
| P01-3  | 327          | 458              | 458             | -1.9           | -0.9               | -0.9              |
| P01-4  | 1701         | 1701             | 1701            | -3.9           | -3.6               | -3.6              |
| P02-1  | 33           | 33               | 33              | +0.0           | +0.0               | +0.0              |
| P02-2  | 33           | 33               | 33              | +0.0           | +0.0               | +0.0              |
| P02-3  | 65           | 785              | 785             | +0.0           | -8.9               | -8.9              |
| P02-4  | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| P03-1  | 1472         | 1177             | 1177            | -10.6          | -10.5              | -10.5             |
| P03-2  | 2616         | 2682             | 2682            | -12.5          | -15.5              | -15.5             |
| P03-3  | 1374         | 1243             | 1243            | -13.0          | -9.5               | -9.5              |
| P03-4  | 2616         | 2682             | 2682            | -11.5          | -13.3              | -13.3             |
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
| P07-1  | 65           | 785              | 785             | +0.0           | -8.9               | -8.9              |
| P07-2  | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| P07-3  | 1504         | 1308             | 1308            | -13.2          | -10.5              | -10.5             |
| P07-4  | 7783         | 7195             | 7195            | -26.2          | -29.6              | -29.6             |
| P08-1  | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| P08-2  | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| P08-3  | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| P08-4  | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| P09-1  | 65           | 785              | 785             | +0.0           | -8.9               | -8.9              |
| P09-2  | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| P09-3  | 65           | 916              | 916             | +0.0           | -10.9              | -10.9             |
| P09-4  | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| P10-1  | 654          | 458              | 458             | -9.5           | -0.8               | -0.8              |
| P10-2  | 1439         | 1374             | 1374            | -8.8           | -9.3               | -9.3              |
| P10-3  | 1701         | 1766             | 1766            | -4.9           | -6.8               | -6.8              |
| P10-4  | 5756         | 5821             | 5821            | -16.9          | -18.9              | -18.9             |
| P11-1  | 65           | 196              | 196             | +0.0           | -2.3               | -2.3              |
| P11-2  | 65           | 196              | 196             | +0.0           | -2.3               | -2.3              |
| P12-1  | 65           | 327              | 327             | +0.0           | -4.2               | -4.2              |
| P12-2  | 65           | 327              | 327             | +0.0           | -4.2               | -4.2              |

Height is the resonant peak's level above the fundamental (gain-independent). This is the number that answers "is the acid squelch there": before/after collapsing to something much smaller than hardware's means the peak got flattened away, not just shifted.

`tuning` is measured from the harmonic peaks relative to equal temperament; `drift` is the std-dev of short-time pitch over the note (hardware fluctuation).

### Worst remaining harmonics / spurious high-frequency content

- **P01-1** worst: k13 425Hz ref -8.7 / sim -2.9, k14 458Hz ref -10.2 / sim -4.9, k15 490Hz ref -11.7 / sim -7.8, k12 392Hz ref -7.0 / sim -3.7. Model excess above 4 kHz: none > 3 dB.
- **P01-2** worst: k60 1962Hz ref -12.5 / sim -16.9, k145 4742Hz ref -60.5 / sim -57.0, k59 1930Hz ref -12.3 / sim -15.6, k44 1439Hz ref -18.2 / sim -14.9. Model excess above 4 kHz: harmonic 4742Hz +3.4dB, harmonic 4709Hz +3.1dB.
- **P01-3** worst: k7 458Hz ref -7.6 / sim -2.6, k6 392Hz ref -4.6 / sim -1.4, k28 1831Hz ref -56.0 / sim -59.1, k27 1766Hz ref -54.6 / sim -57.6. Model excess above 4 kHz: none > 3 dB.
- **P01-4** worst: k31 2028Hz ref -10.3 / sim -14.9, k30 1962Hz ref -9.1 / sim -13.5, k29 1897Hz ref -7.9 / sim -11.7, k32 2093Hz ref -11.8 / sim -16.3. Model excess above 4 kHz: none > 3 dB.
- **P02-1** worst: k38 1243Hz ref -20.4 / sim -23.1, k23 752Hz ref -14.4 / sim -11.8, k22 720Hz ref -14.0 / sim -11.4, k59 1930Hz ref -39.6 / sim -42.1. Model excess above 4 kHz: none > 3 dB.
- **P02-2** worst: k8 262Hz ref -11.4 / sim -8.9, k9 294Hz ref -11.5 / sim -9.2, k7 229Hz ref -10.9 / sim -8.6, k6 196Hz ref -10.0 / sim -8.1. Model excess above 4 kHz: inter-harmonic 5445Hz +3.8dB, inter-harmonic 5412Hz +3.7dB, inter-harmonic 5380Hz +3.7dB.
- **P02-3** worst: k23 1504Hz ref -23.9 / sim -29.0, k22 1439Hz ref -22.1 / sim -26.9, k24 1570Hz ref -26.0 / sim -30.8, k21 1374Hz ref -20.0 / sim -24.8. Model excess above 4 kHz: none > 3 dB.
- **P02-4** worst: k4 262Hz ref -9.0 / sim -6.4, k3 196Hz ref -7.7 / sim -5.6, k5 327Hz ref -9.1 / sim -7.2, k6 392Hz ref -10.4 / sim -8.6. Model excess above 4 kHz: none > 3 dB.
- **P03-1** worst: k22 720Hz ref -5.3 / sim 3.0, k21 687Hz ref -5.8 / sim 2.1, k473 15469Hz ref -82.9 / sim -120.9, k474 15501Hz ref -82.9 / sim -121.4. Model excess above 4 kHz: harmonic 4644Hz +6.0dB, harmonic 4677Hz +6.0dB, harmonic 4611Hz +6.0dB.
- **P03-2** worst: k25 818Hz ref -16.1 / sim -10.5, k26 850Hz ref -16.6 / sim -11.5, k47 1537Hz ref -23.5 / sim -18.6, k27 883Hz ref -17.4 / sim -12.6. Model excess above 4 kHz: harmonic 5658Hz +3.9dB, harmonic 5723Hz +3.9dB, harmonic 5625Hz +3.9dB, inter-harmonic 7015Hz +3.2dB.
- **P03-3** worst: k10 654Hz ref -6.7 / sim -10.2, k13 850Hz ref -8.7 / sim -5.4, k26 1701Hz ref -29.1 / sim -31.9, k81 5298Hz ref -79.8 / sim -77.1. Model excess above 4 kHz: none > 3 dB.
- **P03-4** worst: k13 850Hz ref -13.9 / sim -9.2, k12 785Hz ref -12.8 / sim -8.6, k92 6017Hz ref -61.5 / sim -57.6, k91 5952Hz ref -60.9 / sim -57.0. Model excess above 4 kHz: harmonic 6017Hz +3.9dB, harmonic 5952Hz +3.9dB, harmonic 6344Hz +3.8dB.
- **P04-1** worst: k5 327Hz ref -8.6 / sim -4.5, k4 262Hz ref -8.7 / sim -4.8, k60 3924Hz ref -54.7 / sim -51.1, k59 3859Hz ref -53.8 / sim -50.3. Model excess above 4 kHz: inter-harmonic 4153Hz +10.4dB, inter-harmonic 4088Hz +10.4dB, inter-harmonic 4219Hz +10.3dB, harmonic 4055Hz +3.1dB.
- **P04-2** worst: k65 4251Hz ref -41.0 / sim -34.8, k64 4186Hz ref -40.1 / sim -34.0, k63 4121Hz ref -39.3 / sim -33.2, k66 4317Hz ref -41.7 / sim -35.6. Model excess above 4 kHz: harmonic 4251Hz +6.2dB, harmonic 4186Hz +6.1dB, harmonic 4121Hz +6.1dB, inter-harmonic 4219Hz +3.4dB.
- **P04-3** worst: k5 327Hz ref -10.5 / sim -7.1, k4 262Hz ref -9.7 / sim -6.6, k6 392Hz ref -10.2 / sim -7.7, k3 196Hz ref -7.8 / sim -5.5. Model excess above 4 kHz: none > 3 dB.
- **P04-4** worst: k5 327Hz ref -10.5 / sim -7.1, k4 262Hz ref -9.7 / sim -6.6, k6 392Hz ref -10.2 / sim -7.7, k3 196Hz ref -7.8 / sim -5.5. Model excess above 4 kHz: none > 3 dB.
- **P05-1** worst: k37 2420Hz ref -40.3 / sim -46.2, k38 2485Hz ref -41.7 / sim -47.3, k36 2355Hz ref -38.9 / sim -45.2, k39 2551Hz ref -43.0 / sim -48.2. Model excess above 4 kHz: none > 3 dB.
- **P05-2** worst: k28 1831Hz ref -21.0 / sim -27.1, k27 1766Hz ref -19.8 / sim -25.3, k29 1897Hz ref -22.6 / sim -28.8, k34 2224Hz ref -30.9 / sim -36.1. Model excess above 4 kHz: none > 3 dB.
- **P05-3** worst: k5 327Hz ref -9.1 / sim -5.6, k4 262Hz ref -9.0 / sim -5.7, k56 3663Hz ref -44.3 / sim -41.1, k57 3728Hz ref -45.2 / sim -42.0. Model excess above 4 kHz: inter-harmonic 4284Hz +5.6dB, inter-harmonic 4219Hz +5.5dB, inter-harmonic 4350Hz +5.5dB.
- **P05-4** worst: k62 4055Hz ref -43.9 / sim -35.5, k61 3990Hz ref -43.1 / sim -34.7, k63 4121Hz ref -44.7 / sim -36.3, k60 3924Hz ref -42.2 / sim -33.9. Model excess above 4 kHz: inter-harmonic 4088Hz +9.2dB, inter-harmonic 4153Hz +9.1dB, inter-harmonic 4219Hz +9.0dB, harmonic 4055Hz +8.4dB.
- **P06-1** worst: k55 3597Hz ref -56.0 / sim -61.4, k56 3663Hz ref -56.7 / sim -62.2, k54 3532Hz ref -55.3 / sim -60.7, k57 3728Hz ref -57.3 / sim -62.9. Model excess above 4 kHz: none > 3 dB.
- **P06-2** worst: k28 1831Hz ref -21.2 / sim -27.1, k27 1766Hz ref -19.9 / sim -25.3, k34 2224Hz ref -31.5 / sim -36.1, k26 1701Hz ref -19.0 / sim -23.6. Model excess above 4 kHz: none > 3 dB.
- **P06-3** worst: k6 392Hz ref -13.1 / sim -8.9, k5 327Hz ref -12.0 / sim -7.8, k7 458Hz ref -14.0 / sim -9.8, k8 523Hz ref -14.7 / sim -10.6. Model excess above 4 kHz: none > 3 dB.
- **P06-4** worst: k5 327Hz ref -12.1 / sim -8.3, k6 392Hz ref -13.3 / sim -9.5, k7 458Hz ref -14.2 / sim -10.5, k4 262Hz ref -10.5 / sim -6.8. Model excess above 4 kHz: harmonic 7326Hz +3.1dB, harmonic 7522Hz +3.1dB, harmonic 7456Hz +3.1dB.
- **P07-1** worst: k21 1374Hz ref -19.8 / sim -24.8, k20 1308Hz ref -18.2 / sim -22.5, k25 1635Hz ref -27.9 / sim -32.5, k26 1701Hz ref -30.1 / sim -34.2. Model excess above 4 kHz: none > 3 dB.
- **P07-2** worst: k4 262Hz ref -9.0 / sim -6.4, k3 196Hz ref -7.7 / sim -5.6, k5 327Hz ref -9.1 / sim -7.2, k6 392Hz ref -10.5 / sim -8.6. Model excess above 4 kHz: none > 3 dB.
- **P07-3** worst: k28 1831Hz ref -29.8 / sim -34.3, k27 1766Hz ref -25.8 / sim -32.6, k13 850Hz ref -9.3 / sim -5.9, k10 654Hz ref -6.8 / sim -10.0. Model excess above 4 kHz: none > 3 dB.
- **P07-4** worst: k163 10661Hz ref -56.4 / sim -63.6, k1 65Hz ref 0.0 / sim -2.5, k5 327Hz ref -9.2 / sim -6.8, k4 262Hz ref -9.4 / sim -7.6. Model excess above 4 kHz: inter-harmonic 4284Hz +11.3dB, inter-harmonic 6639Hz +9.9dB, inter-harmonic 5265Hz +7.3dB, harmonic 5232Hz +3.2dB.
- **P08-1** worst: k42 2747Hz ref -22.4 / sim -26.2, k43 2812Hz ref -22.5 / sim -26.2, k44 2878Hz ref -22.6 / sim -26.2, k41 2682Hz ref -22.5 / sim -26.1. Model excess above 4 kHz: none > 3 dB.
- **P08-2** worst: k60 3924Hz ref -33.5 / sim -30.0, k61 3990Hz ref -33.7 / sim -30.1, k59 3859Hz ref -33.4 / sim -29.9, k62 4055Hz ref -33.8 / sim -30.3. Model excess above 4 kHz: inter-harmonic 6770Hz +4.9dB, inter-harmonic 6835Hz +4.8dB, inter-harmonic 6966Hz +4.8dB, harmonic 4055Hz +3.5dB.
- **P08-3** worst: k4 262Hz ref -9.9 / sim -8.1, k1 65Hz ref 0.0 / sim -1.7, k5 327Hz ref -10.8 / sim -9.2, k63 4121Hz ref -31.6 / sim -30.2. Model excess above 4 kHz: none > 3 dB.
- **P08-4** worst: k4 262Hz ref -9.1 / sim -5.7, k43 2812Hz ref -31.0 / sim -28.1, k3 196Hz ref -7.9 / sim -5.0, k44 2878Hz ref -31.3 / sim -28.5. Model excess above 4 kHz: inter-harmonic 9451Hz +4.8dB, inter-harmonic 9386Hz +4.8dB, inter-harmonic 9320Hz +4.7dB.
- **P09-1** worst: k21 1374Hz ref -19.9 / sim -24.8, k24 1570Hz ref -26.5 / sim -30.8, k20 1308Hz ref -18.4 / sim -22.5, k22 1439Hz ref -21.8 / sim -26.9. Model excess above 4 kHz: none > 3 dB.
- **P09-2** worst: k4 262Hz ref -9.0 / sim -6.4, k3 196Hz ref -7.7 / sim -5.6, k5 327Hz ref -9.1 / sim -7.2, k6 392Hz ref -10.4 / sim -8.6. Model excess above 4 kHz: none > 3 dB.
- **P09-3** worst: k15 981Hz ref -17.1 / sim -43.1, k31 2028Hz ref -50.6 / sim -38.2, k33 2158Hz ref -52.9 / sim -40.9, k29 1897Hz ref -47.3 / sim -35.7. Model excess above 4 kHz: harmonic 4709Hz +9.3dB, harmonic 4840Hz +8.7dB, harmonic 4578Hz +8.7dB.
- **P09-4** worst: k15 981Hz ref -19.9 / sim -46.2, k29 1897Hz ref -34.1 / sim -21.7, k33 2158Hz ref -35.7 / sim -23.5, k31 2028Hz ref -34.6 / sim -22.4. Model excess above 4 kHz: harmonic 4121Hz +9.9dB, harmonic 4971Hz +9.3dB, harmonic 5102Hz +9.3dB, inter-harmonic 6639Hz +7.6dB.
- **P10-1** worst: k7 458Hz ref -6.5 / sim -3.1, k31 2028Hz ref -55.9 / sim -59.1, k30 1962Hz ref -54.7 / sim -57.8, k29 1897Hz ref -53.4 / sim -56.4. Model excess above 4 kHz: none > 3 dB.
- **P10-2** worst: k65 4251Hz ref -64.8 / sim -69.5, k7 458Hz ref -11.0 / sim -6.9, k64 4186Hz ref -63.6 / sim -69.6, k63 4121Hz ref -63.0 / sim -67.7. Model excess above 4 kHz: none > 3 dB.
- **P10-3** worst: k26 1701Hz ref -4.9 / sim -9.6, k55 3597Hz ref -34.0 / sim -37.3, k56 3663Hz ref -35.4 / sim -38.2, k54 3532Hz ref -32.1 / sim -36.5. Model excess above 4 kHz: none > 3 dB.
- **P10-4** worst: k177 11577Hz ref -69.0 / sim -58.3, k178 11642Hz ref -67.6 / sim -58.5, k179 11708Hz ref -67.5 / sim -58.8, k25 1635Hz ref -6.4 / sim -12.6. Model excess above 4 kHz: harmonic 11512Hz +12.2dB, harmonic 11446Hz +10.9dB, harmonic 11577Hz +10.7dB, inter-harmonic 11610Hz +8.2dB.
- **P11-1** worst: k11 720Hz ref -24.1 / sim -31.7, k18 1177Hz ref -43.1 / sim -50.5, k10 654Hz ref -20.1 / sim -28.0, k19 1243Hz ref -44.9 / sim -52.6. Model excess above 4 kHz: none > 3 dB.
- **P11-2** worst: k18 1177Hz ref -42.7 / sim -50.5, k17 1112Hz ref -40.9 / sim -48.3, k16 1046Hz ref -38.9 / sim -46.0, k19 1243Hz ref -44.5 / sim -52.6. Model excess above 4 kHz: none > 3 dB.
- **P12-1** worst: k31 2028Hz ref -49.1 / sim -54.9, k32 2093Hz ref -50.3 / sim -56.2, k30 1962Hz ref -48.0 / sim -53.6, k33 2158Hz ref -51.4 / sim -57.4. Model excess above 4 kHz: none > 3 dB.
- **P12-2** worst: k33 2158Hz ref -51.1 / sim -57.4, k32 2093Hz ref -50.0 / sim -56.2, k34 2224Hz ref -52.3 / sim -58.6, k31 2028Hz ref -48.9 / sim -54.9. Model excess above 4 kHz: none > 3 dB.

## Suspicious samples

- **P11-1**: spectrum is within 0.4 dB of P11-2 although the accent label differs -- one of the two labels is probably wrong
- **P03-1**: error 8.58 is 2.9x the median
- **P03-2**: error 6.09 is 2.1x the median
- **P07-4**: error 5.49 is 1.9x the median

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
| vcaCutoffLevelDb     | 0        | 0        | 0.000       |  |
| vcaCutoffLevelResDb  | 0        | 0        | 0.000       |  |

Timing: note-on offset 0.00 ms (relative to each clip's measured note-on), gate length 1300.0 ms.

## Applying the result

Paste `synth_parameters.txt` into `acidus::SynthParameters` (src/core/SynthEngine.hpp) or rerun with `--apply`.

Audio: `renders/` (hardware / before / after for the 40 worst samples).
