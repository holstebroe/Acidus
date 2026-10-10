# Acidus vs. hardware TB-303 calibration report (20261010-094215)

61 reference samples, 41 free parameters. Search: 0 evaluations in 0.0 min, 0 CMA-ES restarts, stopped because: evaluate-only.

## Match quality (mean over samples)

All errors are dB; lower is better. `harm` = full-note harmonic levels (k^-0.5 weighted RMS), `inter` = energy between harmonics, `env` = RMS envelope, `stft` = 1/3-octave levels over time, `peak` = shape (height+width+implicit frequency) of the resonant-peak window on Resonance=max samples only -- weighted more heavily than `harm` so it can't be traded away.

| metric                                         | before | after |
|------------------------------------------------|--------|-------|
| objective (incl. priors)                       | 2.65   | 2.65  |
| weighted error                                 | 2.64   | 2.64  |
| harmonic error (dB)                            | 2.30   | 2.30  |
| resonant-peak shape error (dB)                 | 2.46   | 2.46  |
| inter-harmonic error (dB)                      | 0.75   | 0.75  |
| envelope error (dB)                            | 3.94   | 3.94  |
| spectrogram error (dB)                         | 8.38   | 8.38  |
| aligned waveform error (10*sqrt(residual/ref)) | 0.00   | 0.00  |
| resonant-peak sweep track error (semitones)    | 2.65   | 2.65  |
| harmonics-over-time error (dB)                 | 6.99   | 6.99  |
| mean |harmonic error| (dB)                     | 2.49   | 2.49  |
| note level error, RMS over notes (dB)          | 3.46   | 3.46  |
| harmonics within 1 dB (%)                      | 29.93  | 29.93 |
| harmonics within 3 dB (%)                      | 67.13  | 67.13 |
| harmonics within 6 dB (%)                      | 92.71  | 92.71 |

Recording gain solved as -8.8 dB (before: -8.8 dB).

## Per set

Mean error of each set's notes, and the RMS of their level errors (the recordings are not normalised, so absolute note levels are compared under one global gain).

| set | notes | before | after | level before (dB) | level after (dB) | harm | env | stft |
|-----|-------|--------|-------|-------------------|------------------|------|-----|------|
| 1A  | 9     | 2.51   | 2.51  | 5.1               | 5.1              | 2.6  | 4.8 | 7.5  |
| 1B  | 5     | 3.42   | 3.42  | 2.8               | 2.8              | 2.3  | 4.1 | 9.4  |
| 1C  | 3     | 2.94   | 2.94  | 1.0               | 1.0              | 2.3  | 2.4 | 10.0 |
| 1D  | 4     | 2.91   | 2.91  | 2.4               | 2.4              | 2.5  | 3.3 | 8.3  |
| 1E  | 2     | 1.37   | 1.37  | 4.9               | 4.9              | 2.7  | 4.3 | 6.8  |
| 1R  | 1     | 3.17   | 3.17  | 3.3               | 3.3              | 1.5  | 3.9 | 8.7  |
| 2A  | 16    | 2.23   | 2.23  | 4.0               | 4.0              | 2.0  | 4.7 | 7.8  |
| 2B  | 14    | 2.85   | 2.85  | 2.6               | 2.6              | 2.7  | 3.5 | 8.0  |
| 2C  | 6     | 2.81   | 2.81  | 0.5               | 0.5              | 1.6  | 2.6 | 11.1 |
| 2R  | 1     | 2.71   | 2.71  | 3.3               | 3.3              | 1.5  | 3.9 | 8.2  |

## Per sample (sorted by remaining error, worst first)

| sample | before | after | harm | peak | inter | env  | stft | wave | harm ±3dB | level before | level after | tuning (c) | drift (c) |
|--------|--------|-------|------|------|-------|------|------|------|-----------|--------------|-------------|------------|-----------|
| 2B-14  | 5.33   | 5.33  | 9.7  | 2.8  | 6.8   | 4.9  | 8.9  | 0.0  | 41%       | +4.2         | +4.2        | +0.0       | 0.00      |
| 1B-5   | 4.94   | 4.94  | 5.3  | 2.9  | 2.9   | 5.6  | 7.6  | 0.0  | 23%       | +4.3         | +4.3        | +0.0       | 0.00      |
| 1D-3   | 4.73   | 4.73  | 3.2  | 6.4  | 0.4   | 3.0  | 8.5  | 0.0  | 27%       | +1.2         | +1.2        | +0.0       | 0.00      |
| 1D-4   | 4.45   | 4.45  | 1.9  | 4.1  | 0.6   | 3.0  | 11.3 | 0.0  | 71%       | +0.4         | +0.4        | +0.0       | 0.00      |
| 1C-3   | 3.79   | 3.79  | 4.4  | 2.9  | 0.0   | 1.5  | 7.5  | 0.0  | 44%       | -1.6         | -1.6        | +0.0       | 0.00      |
| 1A-8   | 3.61   | 3.61  | 3.5  | 2.7  | 2.7   | 5.5  | 6.9  | 0.0  | 84%       | +5.8         | +5.8        | +0.0       | 0.00      |
| 2C-6   | 3.60   | 3.60  | 3.9  | 2.8  | 0.5   | 2.5  | 9.4  | 0.0  | 42%       | -0.7         | -0.7        | +0.0       | 0.00      |
| 1A-3   | 3.44   | 3.44  | 2.3  | 3.3  | 0.2   | 2.2  | 7.4  | 0.0  | 28%       | -0.1         | -0.1        | +0.0       | 0.00      |
| 2A-14  | 3.44   | 3.44  | 1.7  | 4.7  | 0.7   | 2.9  | 6.9  | 0.0  | 82%       | +2.5         | +2.5        | +0.0       | 0.00      |
| 1B-4   | 3.37   | 3.37  | 1.4  | 2.8  | 0.0   | 4.3  | 10.8 | 0.0  | 92%       | -2.0         | -2.0        | +0.0       | 0.00      |
| 1A-6   | 3.29   | 3.29  | 1.5  | 3.4  | 0.4   | 3.4  | 9.2  | 0.0  | 92%       | +2.7         | +2.7        | +0.0       | 0.00      |
| 2C-5   | 3.25   | 3.25  | 2.0  | 2.8  | 0.0   | 2.2  | 12.0 | 0.0  | 87%       | -0.0         | -0.0        | +0.0       | 0.00      |
| 1R-1   | 3.17   | 3.17  | 1.5  | 1.4  | 0.2   | 3.9  | 8.7  | 0.0  | 87%       | +3.3         | +3.3        | +0.0       | 0.00      |
| 2B-8   | 3.16   | 3.16  | 3.1  | 3.1  | 0.4   | 2.4  | 7.2  | 0.0  | 22%       | -0.6         | -0.6        | +0.0       | 0.00      |
| 2A-7   | 3.10   | 3.10  | 1.4  | 2.0  | 0.0   | 2.4  | 8.7  | 0.0  | 59%       | +1.6         | +1.6        | +0.0       | 0.00      |
| 1B-3   | 3.04   | 3.04  | 1.2  | 2.3  | 0.0   | 3.3  | 9.4  | 0.0  | 94%       | +1.6         | +1.6        | +0.0       | 0.00      |
| 1B-2   | 3.03   | 3.03  | 2.0  | 2.2  | 0.3   | 3.8  | 10.0 | 0.0  | 66%       | +2.5         | +2.5        | +0.0       | 0.00      |
| 2A-12  | 2.95   | 2.95  | 2.9  | 2.3  | 0.3   | 4.9  | 8.5  | 0.0  | 17%       | +4.6         | +4.6        | +0.0       | 0.00      |
| 2B-1   | 2.93   | 2.93  | 1.4  | 2.1  | 0.2   | 3.7  | 10.0 | 0.0  | 91%       | +2.6         | +2.6        | +0.0       | 0.00      |
| 2A-16  | 2.90   | 2.90  | 3.0  | 2.7  | 1.5   | 4.5  | 7.5  | 0.0  | 83%       | +4.1         | +4.1        | +0.0       | 0.00      |
| 2B-13  | 2.89   | 2.89  | 3.7  | 2.7  | 1.7   | 6.6  | 6.1  | 0.0  | 40%       | +4.1         | +4.1        | +0.0       | 0.00      |
| 2B-7   | 2.87   | 2.87  | 2.4  | 2.4  | 0.0   | 2.5  | 7.3  | 0.0  | 20%       | -0.5         | -0.5        | +0.0       | 0.00      |
| 1A-7   | 2.85   | 2.85  | 5.5  | -    | 3.7   | 11.8 | 10.9 | 0.0  | 0%        | +11.8        | +11.8       | +0.0       | 0.00      |
| 2C-4   | 2.80   | 2.80  | 1.1  | 2.7  | 0.0   | 2.4  | 11.8 | 0.0  | 97%       | +0.1         | +0.1        | +0.0       | 0.00      |
| 2B-3   | 2.80   | 2.80  | 1.3  | 2.1  | 1.2   | 3.4  | 7.1  | 0.0  | 94%       | +2.2         | +2.2        | +0.0       | 0.00      |
| 1C-2   | 2.79   | 2.79  | 1.9  | 2.7  | 0.0   | 3.4  | 10.7 | 0.0  | 77%       | +0.1         | +0.1        | +0.0       | 0.00      |
| 1B-1   | 2.74   | 2.74  | 1.7  | 2.1  | 0.1   | 3.6  | 9.0  | 0.0  | 90%       | +2.7         | +2.7        | +0.0       | 0.00      |
| 2B-2   | 2.71   | 2.71  | 1.7  | 2.2  | 0.4   | 3.4  | 8.9  | 0.0  | 91%       | +2.4         | +2.4        | +0.0       | 0.00      |
| 2R-1   | 2.71   | 2.71  | 1.5  | 1.4  | 0.3   | 3.9  | 8.2  | 0.0  | 88%       | +3.3         | +3.3        | +0.0       | 0.00      |
| 2A-5   | 2.69   | 2.69  | 1.5  | 0.8  | 0.0   | 2.4  | 7.6  | 0.0  | 58%       | +1.2         | +1.2        | +0.0       | 0.00      |
| 2B-9   | 2.68   | 2.68  | 2.7  | 2.4  | 0.0   | 2.2  | 8.7  | 0.0  | 23%       | -0.6         | -0.6        | +0.0       | 0.00      |
| 1A-5   | 2.68   | 2.68  | 1.5  | 1.4  | 0.1   | 4.8  | 7.5  | 0.0  | 87%       | +3.4         | +3.4        | +0.0       | 0.00      |
| 2A-13  | 2.66   | 2.66  | 1.9  | 2.5  | 0.9   | 3.4  | 8.3  | 0.0  | 85%       | +3.2         | +3.2        | +0.0       | 0.00      |
| 2B-11  | 2.65   | 2.65  | 3.1  | 2.7  | 1.6   | 3.9  | 7.3  | 0.0  | 82%       | +3.9         | +3.9        | +0.0       | 0.00      |
| 2A-9   | 2.65   | 2.65  | 1.5  | 2.0  | 0.6   | 3.4  | 9.1  | 0.0  | 87%       | +3.1         | +3.1        | +0.0       | 0.00      |
| 2A-6   | 2.56   | 2.56  | 0.9  | 1.3  | 0.5   | 6.8  | 9.0  | 0.0  | 97%       | +1.9         | +1.9        | +0.0       | 0.00      |
| 2B-12  | 2.55   | 2.55  | 3.0  | 2.7  | 2.6   | 3.8  | 5.6  | 0.0  | 80%       | +3.9         | +3.9        | +0.0       | 0.00      |
| 2B-4   | 2.47   | 2.47  | 1.8  | 2.1  | 0.7   | 3.3  | 8.5  | 0.0  | 88%       | +2.1         | +2.1        | +0.0       | 0.00      |
| 2C-2   | 2.45   | 2.45  | 0.9  | 2.2  | 0.0   | 3.5  | 10.9 | 0.0  | 90%       | -0.4         | -0.4        | +0.0       | 0.00      |
| 2C-3   | 2.44   | 2.44  | 0.9  | 2.5  | 0.0   | 2.8  | 10.8 | 0.0  | 100%      | -0.2         | -0.2        | +0.0       | 0.00      |
| 2B-5   | 2.37   | 2.37  | 1.4  | 2.1  | 0.4   | 3.3  | 8.7  | 0.0  | 96%       | +1.9         | +1.9        | +0.0       | 0.00      |
| 2B-6   | 2.34   | 2.34  | 1.3  | 1.7  | 0.1   | 3.4  | 8.8  | 0.0  | 95%       | +2.1         | +2.1        | +0.0       | 0.00      |
| 1A-9   | 2.29   | 2.29  | 2.6  | 2.6  | 1.7   | 2.9  | 4.8  | 0.0  | 78%       | +2.0         | +2.0        | +0.0       | 0.00      |
| 2C-1   | 2.28   | 2.28  | 0.6  | 1.5  | 0.3   | 2.0  | 11.8 | 0.0  | 92%       | -0.9         | -0.9        | +0.0       | 0.00      |
| 2A-2   | 2.28   | 2.28  | 0.7  | 1.8  | 0.0   | 2.4  | 8.1  | 0.0  | 88%       | -0.3         | -0.3        | +0.0       | 0.00      |
| 1C-1   | 2.24   | 2.24  | 0.4  | 1.8  | 0.4   | 2.3  | 11.9 | 0.0  | 100%      | -0.8         | -0.8        | +0.0       | 0.00      |
| 2B-10  | 2.22   | 2.22  | 1.8  | 1.7  | 0.0   | 2.4  | 8.7  | 0.0  | 75%       | -1.0         | -1.0        | +0.0       | 0.00      |
| 2A-15  | 2.18   | 2.18  | 4.4  | -    | 3.7   | 9.2  | 7.5  | 0.0  | 85%       | +8.1         | +8.1        | +0.0       | 0.00      |
| 2A-10  | 1.89   | 1.89  | 3.0  | -    | 2.1   | 9.6  | 7.0  | 0.0  | 87%       | +8.3         | +8.3        | +0.0       | 0.00      |
| 1A-2   | 1.86   | 1.86  | 2.0  | 1.7  | 0.0   | 2.8  | 7.6  | 0.0  | 21%       | -1.5         | -1.5        | +0.0       | 0.00      |
| 1E-1   | 1.76   | 1.76  | 4.0  | -    | 2.5   | 5.8  | 7.0  | 0.0  | 27%       | +5.7         | +5.7        | +0.0       | 0.00      |
| 2A-11  | 1.58   | 1.58  | 2.5  | -    | 1.3   | 5.9  | 8.2  | 0.0  | 87%       | +5.8         | +5.8        | +0.0       | 0.00      |
| 2A-1   | 1.44   | 1.44  | 2.5  | -    | 0.0   | 7.7  | 5.4  | 0.0  | 29%       | -2.8         | -2.8        | +0.0       | 0.00      |
| 1A-1   | 1.40   | 1.40  | 2.8  | -    | 0.0   | 5.6  | 6.3  | 0.0  | 8%        | -4.7         | -4.7        | +0.0       | 0.00      |
| 1D-1   | 1.28   | 1.28  | 2.9  | -    | 0.0   | 2.6  | 7.5  | 0.0  | 24%       | +0.6         | +0.6        | +0.0       | 0.00      |
| 2A-8   | 1.23   | 1.23  | 1.6  | -    | 0.1   | 4.0  | 8.1  | 0.0  | 78%       | +3.5         | +3.5        | +0.0       | 0.00      |
| 1D-2   | 1.19   | 1.19  | 2.0  | -    | 0.8   | 4.7  | 5.7  | 0.0  | 74%       | +4.6         | +4.6        | +0.0       | 0.00      |
| 1A-4   | 1.18   | 1.18  | 1.5  | -    | 0.0   | 4.3  | 7.3  | 0.0  | 76%       | +4.1         | +4.1        | +0.0       | 0.00      |
| 2A-4   | 1.07   | 1.07  | 1.7  | -    | 0.0   | 2.6  | 7.3  | 0.0  | 47%       | +0.3         | +0.3        | +0.0       | 0.00      |
| 2A-3   | 1.02   | 1.02  | 1.8  | -    | 0.0   | 2.4  | 6.8  | 0.0  | 45%       | -0.7         | -0.7        | +0.0       | 0.00      |
| 1E-2   | 0.98   | 0.98  | 1.5  | -    | 0.0   | 2.8  | 6.6  | 0.0  | 78%       | +3.9         | +3.9        | +0.0       | 0.00      |

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
| 2A-2   | 131          | 131              | 131             | +0.5           | -1.0               | -1.0              |
| 2A-5   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 2A-6   | 196          | 196              | 196             | -2.3           | -1.5               | -1.5              |
| 2A-7   | 196          | 262              | 262             | +1.3           | +0.8               | +0.8              |
| 2A-9   | 65           | 392              | 392             | +0.0           | -4.3               | -4.3              |
| 2A-12  | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 2A-13  | 65           | 720              | 720             | +0.0           | -7.3               | -7.3              |
| 2A-14  | 1439         | 785              | 785             | -18.3          | -0.4               | -0.4              |
| 2A-16  | 65           | 1504             | 1504            | +0.0           | -12.2              | -12.2             |
| 2B-1   | 65           | 392              | 392             | +0.0           | -4.4               | -4.4              |
| 2B-2   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 2B-3   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 2B-4   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 2B-5   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 2B-6   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 2B-7   | 131          | 196              | 196             | -1.0           | -2.6               | -2.6              |
| 2B-8   | 131          | 196              | 196             | -0.9           | -2.7               | -2.7              |
| 2B-9   | 65           | 131              | 131             | +0.0           | -1.3               | -1.3              |
| 2B-10  | 65           | 10138            | 10138           | +0.0           | -137.9             | -137.9            |
| 2B-11  | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 2B-12  | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 2B-13  | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 2B-14  | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 2C-1   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 2C-2   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 2C-3   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 2C-4   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 2C-5   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 2C-6   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 2R-1   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |

Height is the resonant peak's level above the fundamental (gain-independent). This is the number that answers "is the acid squelch there": before/after collapsing to something much smaller than hardware's means the peak got flattened away, not just shifted.

`tuning` is measured from the harmonic peaks relative to equal temperament; `drift` is the std-dev of short-time pitch over the note (hardware fluctuation).

### Worst remaining harmonics / spurious high-frequency content

- **1A-1** worst: k10 654Hz ref -48.4 / sim -55.7, k9 589Hz ref -45.3 / sim -51.8, k13 850Hz ref -56.6 / sim -65.8, k17 1112Hz ref -65.4 / sim -76.5. Model excess above 4 kHz: none > 3 dB.
- **1A-2** worst: k9 589Hz ref -39.0 / sim -43.9, k10 654Hz ref -42.3 / sim -47.8, k11 720Hz ref -45.5 / sim -51.4, k21 1374Hz ref -68.4 / sim -77.5. Model excess above 4 kHz: none > 3 dB.
- **1A-3** worst: k13 850Hz ref -45.1 / sim -52.3, k14 916Hz ref -47.4 / sim -55.2, k12 785Hz ref -43.0 / sim -49.2, k23 1504Hz ref -65.8 / sim -75.5. Model excess above 4 kHz: none > 3 dB.
- **1A-4** worst: k1 65Hz ref 0.0 / sim 4.4, k38 2485Hz ref -76.1 / sim -80.0, k2 131Hz ref -4.7 / sim -0.8, k36 2355Hz ref -74.1 / sim -77.8. Model excess above 4 kHz: none > 3 dB.
- **1A-5** worst: k5 327Hz ref -8.9 / sim -4.2, k6 392Hz ref -12.3 / sim -7.7, k3 196Hz ref -5.4 / sim -1.3, k2 131Hz ref -4.0 / sim -0.0. Model excess above 4 kHz: none > 3 dB.
- **1A-6** worst: k7 458Hz ref -7.6 / sim -0.2, k6 392Hz ref -4.6 / sim 0.9, k8 523Hz ref -9.8 / sim -5.3, k3 196Hz ref -6.2 / sim -3.1. Model excess above 4 kHz: none > 3 dB.
- **1A-7** worst: k3 196Hz ref -8.4 / sim 6.3, k4 262Hz ref -10.7 / sim 3.3, k2 131Hz ref -4.9 / sim 9.0, k5 327Hz ref -12.5 / sim 0.5. Model excess above 4 kHz: harmonic 4251Hz +3.6dB, harmonic 4055Hz +3.6dB, harmonic 4121Hz +3.6dB.
- **1A-8** worst: k5 327Hz ref -11.9 / sim -3.6, k6 392Hz ref -13.0 / sim -4.8, k7 458Hz ref -13.8 / sim -5.7, k4 262Hz ref -10.4 / sim -2.3. Model excess above 4 kHz: none > 3 dB.
- **1A-9** worst: k22 1439Hz ref -15.4 / sim -10.2, k21 1374Hz ref -16.3 / sim -11.1, k20 1308Hz ref -17.0 / sim -11.9, k23 1504Hz ref -13.9 / sim -8.9. Model excess above 4 kHz: harmonic 5690Hz +3.2dB.
- **1B-1** worst: k6 392Hz ref -8.2 / sim -3.1, k7 458Hz ref -9.6 / sim -4.8, k8 523Hz ref -11.0 / sim -7.1, k3 196Hz ref -6.4 / sim -2.8. Model excess above 4 kHz: none > 3 dB.
- **1B-2** worst: k6 392Hz ref -9.5 / sim -4.5, k5 327Hz ref -8.1 / sim -3.2, k7 458Hz ref -10.6 / sim -6.2, k8 523Hz ref -11.6 / sim -7.8. Model excess above 4 kHz: none > 3 dB.
- **1B-3** worst: k4 262Hz ref -8.8 / sim -5.2, k5 327Hz ref -10.4 / sim -6.9, k6 392Hz ref -11.6 / sim -8.3, k3 196Hz ref -6.5 / sim -3.4. Model excess above 4 kHz: none > 3 dB.
- **1B-4** worst: k13 850Hz ref -17.6 / sim -23.7, k15 981Hz ref -18.4 / sim -29.5, k14 916Hz ref -18.1 / sim -26.5, k12 785Hz ref -16.9 / sim -21.3. Model excess above 4 kHz: none > 3 dB.
- **1B-5** worst: k9 589Hz ref -14.8 / sim -6.9, k8 523Hz ref -14.4 / sim -6.7, k168 10988Hz ref -49.7 / sim -42.2, k167 10923Hz ref -49.6 / sim -42.1. Model excess above 4 kHz: harmonic 15763Hz +12.2dB, harmonic 15828Hz +12.2dB, harmonic 15894Hz +12.1dB, inter-harmonic 15796Hz +11.0dB.
- **1C-1** worst: k8 523Hz ref -18.2 / sim -21.1, k1 65Hz ref 0.0 / sim -1.5, k2 131Hz ref -6.0 / sim -5.0, k7 458Hz ref -17.4 / sim -18.3. Model excess above 4 kHz: none > 3 dB.
- **1C-2** worst: k46 3009Hz ref -42.0 / sim -58.7, k51 3336Hz ref -47.9 / sim -62.8, k52 3401Hz ref -48.8 / sim -63.6, k50 3270Hz ref -46.9 / sim -62.0. Model excess above 4 kHz: none > 3 dB.
- **1C-3** worst: k27 1766Hz ref -18.9 / sim -32.2, k28 1831Hz ref -19.1 / sim -33.9, k29 1897Hz ref -19.4 / sim -35.5, k30 1962Hz ref -19.6 / sim -37.0. Model excess above 4 kHz: none > 3 dB.
- **1D-1** worst: k18 1177Hz ref -51.2 / sim -58.4, k17 1112Hz ref -49.4 / sim -56.3, k19 1243Hz ref -52.9 / sim -60.3, k20 1308Hz ref -54.6 / sim -62.2. Model excess above 4 kHz: none > 3 dB.
- **1D-2** worst: k3 196Hz ref -9.1 / sim -2.0, k4 262Hz ref -11.8 / sim -5.1, k2 131Hz ref -4.9 / sim 1.1, k5 327Hz ref -13.9 / sim -8.1. Model excess above 4 kHz: none > 3 dB.
- **1D-3** worst: k25 1635Hz ref -50.7 / sim -58.9, k24 1570Hz ref -49.3 / sim -57.5, k26 1701Hz ref -52.1 / sim -60.3, k27 1766Hz ref -53.5 / sim -61.6. Model excess above 4 kHz: none > 3 dB.
- **1D-4** worst: k43 2812Hz ref -55.8 / sim -62.3, k44 2878Hz ref -57.0 / sim -63.0, k42 2747Hz ref -54.8 / sim -60.7, k45 2943Hz ref -57.9 / sim -63.2. Model excess above 4 kHz: none > 3 dB.
- **1E-1** worst: k29 1897Hz ref -78.0 / sim -66.4, k27 1766Hz ref -74.6 / sim -64.1, k15 981Hz ref -47.8 / sim -64.5, k8 523Hz ref -36.6 / sim -26.9. Model excess above 4 kHz: none > 3 dB.
- **1E-2** worst: k1 65Hz ref 0.0 / sim 4.3, k2 131Hz ref -4.7 / sim -0.9, k37 2420Hz ref -75.6 / sim -79.3, k36 2355Hz ref -74.6 / sim -78.2. Model excess above 4 kHz: none > 3 dB.
- **1R-1** worst: k5 327Hz ref -8.9 / sim -4.3, k6 392Hz ref -12.2 / sim -7.7, k3 196Hz ref -5.4 / sim -1.3, k2 131Hz ref -4.0 / sim -0.1. Model excess above 4 kHz: none > 3 dB.
- **2A-1** worst: k20 1308Hz ref -68.5 / sim -79.3, k13 850Hz ref -54.2 / sim -61.8, k21 1374Hz ref -70.2 / sim -81.4, k12 785Hz ref -51.6 / sim -58.6. Model excess above 4 kHz: none > 3 dB.
- **2A-2** worst: k7 458Hz ref -26.7 / sim -31.2, k6 392Hz ref -21.7 / sim -25.5, k12 785Hz ref -47.5 / sim -51.6, k11 720Hz ref -44.9 / sim -48.3. Model excess above 4 kHz: none > 3 dB.
- **2A-3** worst: k20 1308Hz ref -65.9 / sim -71.9, k22 1439Hz ref -69.2 / sim -75.7, k21 1374Hz ref -67.6 / sim -73.9, k19 1243Hz ref -64.2 / sim -69.8. Model excess above 4 kHz: none > 3 dB.
- **2A-4** worst: k20 1308Hz ref -62.5 / sim -67.6, k23 1504Hz ref -67.3 / sim -73.3, k21 1374Hz ref -64.0 / sim -69.6, k22 1439Hz ref -65.8 / sim -71.4. Model excess above 4 kHz: none > 3 dB.
- **2A-5** worst: k29 1897Hz ref -74.3 / sim -79.2, k28 1831Hz ref -73.0 / sim -77.8, k27 1766Hz ref -71.5 / sim -76.2, k26 1701Hz ref -70.1 / sim -74.7. Model excess above 4 kHz: none > 3 dB.
- **2A-6** worst: k4 262Hz ref -7.5 / sim -3.4, k5 327Hz ref -11.4 / sim -9.3, k3 196Hz ref -2.3 / sim -0.2, k2 131Hz ref -2.1 / sim -0.3. Model excess above 4 kHz: none > 3 dB.
- **2A-7** worst: k4 262Hz ref -4.4 / sim 0.5, k32 2093Hz ref -75.6 / sim -79.4, k31 2028Hz ref -74.3 / sim -78.1, k28 1831Hz ref -70.3 / sim -73.8. Model excess above 4 kHz: none > 3 dB.
- **2A-8** worst: k2 131Hz ref -3.9 / sim 0.3, k3 196Hz ref -6.0 / sim -2.2, k42 2747Hz ref -76.4 / sim -79.9, k4 262Hz ref -8.8 / sim -5.3. Model excess above 4 kHz: none > 3 dB.
- **2A-9** worst: k6 392Hz ref -8.9 / sim -2.7, k7 458Hz ref -11.7 / sim -6.6, k5 327Hz ref -5.6 / sim -1.5, k3 196Hz ref -5.8 / sim -2.0. Model excess above 4 kHz: none > 3 dB.
- **2A-10** worst: k2 131Hz ref -4.9 / sim 4.8, k3 196Hz ref -8.3 / sim 0.6, k1 65Hz ref 0.0 / sim 8.0, k4 262Hz ref -10.7 / sim -3.2. Model excess above 4 kHz: none > 3 dB.
- **2A-11** worst: k3 196Hz ref -7.6 / sim -0.0, k4 262Hz ref -9.3 / sim -1.8, k5 327Hz ref -10.3 / sim -3.5, k2 131Hz ref -4.7 / sim 2.0. Model excess above 4 kHz: none > 3 dB.
- **2A-12** worst: k4 262Hz ref -9.2 / sim -2.7, k5 327Hz ref -9.9 / sim -3.6, k3 196Hz ref -7.6 / sim -1.4, k6 392Hz ref -10.0 / sim -4.3. Model excess above 4 kHz: none > 3 dB.
- **2A-13** worst: k11 720Hz ref -11.1 / sim -5.8, k5 327Hz ref -10.4 / sim -5.4, k4 262Hz ref -9.5 / sim -4.5, k12 785Hz ref -12.8 / sim -8.0. Model excess above 4 kHz: none > 3 dB.
- **2A-14** worst: k12 785Hz ref -7.8 / sim -0.2, k13 850Hz ref -9.5 / sim -2.7, k14 916Hz ref -10.9 / sim -6.8, k5 327Hz ref -10.8 / sim -7.0. Model excess above 4 kHz: harmonic 4906Hz +3.3dB, harmonic 4840Hz +3.3dB, harmonic 4775Hz +3.3dB.
- **2A-15** worst: k5 327Hz ref -11.9 / sim -1.2, k4 262Hz ref -10.4 / sim 0.3, k6 392Hz ref -13.0 / sim -2.5, k7 458Hz ref -13.8 / sim -3.6. Model excess above 4 kHz: none > 3 dB.
- **2A-16** worst: k5 327Hz ref -12.0 / sim -5.5, k6 392Hz ref -13.2 / sim -6.7, k7 458Hz ref -14.0 / sim -7.7, k4 262Hz ref -10.4 / sim -4.2. Model excess above 4 kHz: none > 3 dB.
- **2B-1** worst: k6 392Hz ref -8.6 / sim -3.3, k7 458Hz ref -9.9 / sim -5.0, k5 327Hz ref -7.0 / sim -3.0, k8 523Hz ref -11.1 / sim -7.2. Model excess above 4 kHz: none > 3 dB.
- **2B-2** worst: k5 327Hz ref -8.6 / sim -3.9, k6 392Hz ref -9.9 / sim -5.5, k7 458Hz ref -11.0 / sim -7.1, k4 262Hz ref -6.6 / sim -3.0. Model excess above 4 kHz: none > 3 dB.
- **2B-3** worst: k5 327Hz ref -9.1 / sim -5.0, k4 262Hz ref -7.3 / sim -3.4, k6 392Hz ref -10.4 / sim -6.6, k7 458Hz ref -11.4 / sim -8.0. Model excess above 4 kHz: none > 3 dB.
- **2B-4** worst: k5 327Hz ref -9.3 / sim -5.4, k4 262Hz ref -7.6 / sim -3.7, k6 392Hz ref -10.6 / sim -7.0, k7 458Hz ref -11.6 / sim -8.3. Model excess above 4 kHz: harmonic 6017Hz +3.2dB, harmonic 5887Hz +3.2dB, harmonic 5952Hz +3.2dB.
- **2B-5** worst: k4 262Hz ref -7.9 / sim -4.2, k5 327Hz ref -9.6 / sim -5.9, k6 392Hz ref -10.8 / sim -7.4, k7 458Hz ref -11.8 / sim -8.7. Model excess above 4 kHz: none > 3 dB.
- **2B-6** worst: k4 262Hz ref -7.8 / sim -4.1, k5 327Hz ref -9.4 / sim -5.8, k6 392Hz ref -10.7 / sim -7.2, k7 458Hz ref -11.6 / sim -8.5. Model excess above 4 kHz: none > 3 dB.
- **2B-7** worst: k14 916Hz ref -47.2 / sim -53.2, k15 981Hz ref -49.9 / sim -55.8, k13 850Hz ref -44.5 / sim -50.4, k16 1046Hz ref -52.6 / sim -58.3. Model excess above 4 kHz: none > 3 dB.
- **2B-8** worst: k6 392Hz ref -13.6 / sim -21.1, k15 981Hz ref -48.0 / sim -55.4, k16 1046Hz ref -50.5 / sim -57.9, k14 916Hz ref -45.4 / sim -52.8. Model excess above 4 kHz: none > 3 dB.
- **2B-9** worst: k7 458Hz ref -13.4 / sim -23.3, k13 850Hz ref -37.0 / sim -46.7, k6 392Hz ref -11.0 / sim -17.4, k14 916Hz ref -39.5 / sim -49.6. Model excess above 4 kHz: none > 3 dB.
- **2B-10** worst: k10 654Hz ref -15.1 / sim -23.0, k9 589Hz ref -14.0 / sim -19.0, k23 1504Hz ref -43.9 / sim -56.3, k24 1570Hz ref -45.4 / sim -58.0. Model excess above 4 kHz: none > 3 dB.
- **2B-11** worst: k5 327Hz ref -12.0 / sim -5.8, k6 392Hz ref -13.2 / sim -6.9, k7 458Hz ref -14.1 / sim -8.0, k4 262Hz ref -10.4 / sim -4.4. Model excess above 4 kHz: none > 3 dB.
- **2B-12** worst: k5 327Hz ref -12.0 / sim -5.8, k6 392Hz ref -13.2 / sim -6.9, k7 458Hz ref -14.1 / sim -8.0, k4 262Hz ref -10.5 / sim -4.4. Model excess above 4 kHz: none > 3 dB.
- **2B-13** worst: k5 327Hz ref -12.0 / sim -5.6, k6 392Hz ref -13.2 / sim -6.8, k7 458Hz ref -14.1 / sim -7.8, k4 262Hz ref -10.5 / sim -4.2. Model excess above 4 kHz: harmonic 7326Hz +4.9dB, harmonic 7260Hz +4.9dB, harmonic 7129Hz +4.8dB.
- **2B-14** worst: k12 785Hz ref -16.1 / sim -8.5, k11 720Hz ref -15.9 / sim -8.5, k10 654Hz ref -15.6 / sim -8.5, k13 850Hz ref -16.0 / sim -8.9. Model excess above 4 kHz: harmonic 9157Hz +18.6dB, harmonic 9092Hz +18.6dB, harmonic 9222Hz +18.6dB, inter-harmonic 8928Hz +15.8dB.
- **2C-1** worst: k10 654Hz ref -18.2 / sim -21.3, k11 720Hz ref -19.4 / sim -23.4, k9 589Hz ref -17.3 / sim -19.2, k1 65Hz ref 0.0 / sim -1.5. Model excess above 4 kHz: none > 3 dB.
- **2C-2** worst: k45 2943Hz ref -47.6 / sim -64.4, k46 3009Hz ref -48.8 / sim -65.3, k44 2878Hz ref -46.2 / sim -63.5, k47 3074Hz ref -49.9 / sim -66.2. Model excess above 4 kHz: none > 3 dB.
- **2C-3** worst: k17 1112Hz ref -20.0 / sim -23.0, k18 1177Hz ref -21.0 / sim -23.9, k16 1046Hz ref -19.5 / sim -21.9, k19 1243Hz ref -21.5 / sim -25.1. Model excess above 4 kHz: none > 3 dB.
- **2C-4** worst: k21 1374Hz ref -19.9 / sim -24.6, k20 1308Hz ref -19.6 / sim -23.1, k22 1439Hz ref -20.7 / sim -26.2, k25 1635Hz ref -21.3 / sim -32.2. Model excess above 4 kHz: none > 3 dB.
- **2C-5** worst: k26 1701Hz ref -20.6 / sim -32.1, k27 1766Hz ref -21.0 / sim -33.9, k25 1635Hz ref -20.2 / sim -30.1, k23 1504Hz ref -19.7 / sim -26.1. Model excess above 4 kHz: none > 3 dB.
- **2C-6** worst: k30 1962Hz ref -20.6 / sim -37.3, k26 1701Hz ref -19.4 / sim -30.7, k29 1897Hz ref -20.3 / sim -35.8, k25 1635Hz ref -19.1 / sim -28.8. Model excess above 4 kHz: none > 3 dB.
- **2R-1** worst: k5 327Hz ref -8.9 / sim -4.3, k6 392Hz ref -12.1 / sim -7.7, k3 196Hz ref -5.4 / sim -1.3, k2 131Hz ref -3.9 / sim -0.1. Model excess above 4 kHz: none > 3 dB.

## Suspicious samples

- **2C-4**: spectrum is within 1.2 dB of 2C-5 although the decay label differs -- one of the two labels is probably wrong
- **2B-11**: spectrum is within 1.2 dB of 2B-12 although the envMod label differs -- one of the two labels is probably wrong
- **2B-3**: spectrum is within 1.5 dB of 2B-4 although the envMod label differs -- one of the two labels is probably wrong
- **2A-6**: spectrum is within 1.0 dB of 2B-7 although the cutoff/decay label differs -- one of the two labels is probably wrong
- **1D-1**: spectrum is within 1.0 dB of 1E-2 although the cutoff/envMod/decay/accent label differs -- one of the two labels is probably wrong
- **1A-4**: spectrum is within 1.2 dB of 1D-1 although the cutoff/envMod/decay/accent label differs -- one of the two labels is probably wrong
- **1A-3**: spectrum is within 1.4 dB of 2B-7 although the resonance/decay label differs -- one of the two labels is probably wrong
- **2B-14**: error 5.33 is 2.0x the median
- **1B-5**: error 4.94 is 1.8x the median
- **1D-3**: error 4.73 is 1.8x the median
- **1D-4**: error 4.45 is 1.7x the median

## Fitted knob positions

| knob (label)     | nominal | fitted | sensitivity |
|------------------|---------|--------|-------------|
| accent (x0)      | 0.00    | 0.000  | 0.000       |
| accent (x100)    | 1.00    | 1.000  | 0.000       |
| cutoff (x0)      | 0.00    | 0.000  | 0.000       |
| cutoff (x100)    | 1.00    | 1.000  | 0.000       |
| cutoff (x25)     | 0.25    | 0.252  | 0.000       |
| cutoff (x50)     | 0.50    | 0.504  | 0.000       |
| cutoff (x74)     | 0.75    | 0.748  | 0.000       |
| decay (x0)       | 0.00    | 0.000  | 0.000       |
| decay (x10)      | 0.10    | 0.102  | 0.000       |
| decay (x100)     | 1.00    | 1.000  | 0.000       |
| decay (x25)      | 0.25    | 0.252  | 0.000       |
| decay (x40)      | 0.40    | 0.402  | 0.000       |
| decay (x50)      | 0.50    | 0.504  | 0.000       |
| decay (x59)      | 0.60    | 0.598  | 0.000       |
| decay (x74)      | 0.75    | 0.748  | 0.000       |
| decay (x89)      | 0.90    | 0.898  | 0.000       |
| envMod (x0)      | 0.00    | 0.000  | 0.000       |
| envMod (x100)    | 1.00    | 1.000  | 0.000       |
| envMod (x25)     | 0.25    | 0.252  | 0.000       |
| envMod (x50)     | 0.50    | 0.504  | 0.000       |
| envMod (x59)     | 0.60    | 0.598  | 0.000       |
| envMod (x70)     | 0.70    | 0.701  | 0.000       |
| envMod (x74)     | 0.75    | 0.748  | 0.000       |
| envMod (x80)     | 0.80    | 0.803  | 0.000       |
| envMod (x89)     | 0.90    | 0.898  | 0.000       |
| resonance (x0)   | 0.00    | 0.000  | 0.000       |
| resonance (x100) | 1.00    | 1.000  | 0.000       |
| resonance (x25)  | 0.25    | 0.252  | 0.000       |
| resonance (x50)  | 0.50    | 0.504  | 0.000       |
| resonance (x74)  | 0.75    | 0.748  | 0.000       |

Cutoff law with the fitted end stops folded in (so the plugin's knob range equals the hardware's): `cutoffBaseHz = 212.2`, `cutoffSpanOct = 3.321`, `cutoffTaperExp = 1.429`.

## Fitted model parameters

`sensitivity` = objective increase for a 5% (of range) nudge. Near-zero means the current references don't constrain that parameter -- its value is not evidence of anything. `AT BOUND` means the optimizer pushed it to the edge of its plausible range, usually a sign that it is compensating for something the model does not implement.

| parameter              | before    | after     | sensitivity |  |
|------------------------|-----------|-----------|-------------|--|
| filterFeedbackGain     | 17.42     | 17.42     | 0.000       |  |
| filterResonanceLimit   | 0.91245   | 0.91245   | 0.000       |  |
| filterResonanceSkew    | -0.8718   | -0.8718   | 0.000       |  |
| resCouplingHz          | 104.29    | 104.29    | 0.000       |  |
| filterCapScale1        | 1.2895    | 1.2895    | 0.000       |  |
| filterCapScale2        | 0.69764   | 0.69764   | 0.000       |  |
| filterCapScale3        | 0.91275   | 0.91275   | 0.000       |  |
| filterCapScale4        | 1.0631    | 1.0631    | 0.000       |  |
| filterLadderInputScale | 0.035224  | 0.035224  | 0.000       |  |
| filterInputCouplingHz  | 6.0165    | 6.0165    | 0.000       |  |
| filterOutputCouplingHz | 20000     | 20000     | 0.000       |  |
| filterPostHpHz         | 198.89    | 198.89    | 0.000       |  |
| filterNotchHz          | 7.5164    | 7.5164    | 0.000       |  |
| filterNotchBandwidthHz | 4.7       | 4.7       | 0.000       |  |
| filterAllpassHz        | 14.008    | 14.008    | 0.000       |  |
| cutoffBaseHz           | 212.18    | 212.18    | 0.000       |  |
| cutoffSpanOct          | 3.3213    | 3.3213    | 0.000       |  |
| cutoffTaperExp         | 1.4289    | 1.4289    | 0.000       |  |
| cutoffMaxHz            | 23723     | 23723     | 0.000       |  |
| envModScaleC0          | 0.76145   | 0.76145   | 0.000       |  |
| envModScaleC0Slope     | 2.8142    | 2.8142    | 0.000       |  |
| envModScaleC1          | 0.77794   | 0.77794   | 0.000       |  |
| envModScaleC1Slope     | 4.4796    | 4.4796    | 0.000       |  |
| envModOffset           | 0.34908   | 0.34908   | 0.000       |  |
| envModOffsetCutSlope   | -0.023862 | -0.023862 | 0.000       |  |
| envModTaperExp         | 2         | 2         | 0.000       |  |
| envModTaperMid         | 0.68612   | 0.68612   | 0.000       |  |
| envModTaperWidth       | 0.12193   | 0.12193   | 0.000       |  |
| accentSweepDepthOct    | 7.9269    | 7.9269    | 0.000       |  |
| accentVcaDepth         | 2.2843    | 2.2843    | 0.000       |  |
| accentChargeBaseSec    | 0.030579  | 0.030579  | 0.000       |  |
| accentChargePotSec     | 0.044021  | 0.044021  | 0.000       |  |
| accentMixSec           | 0.14498   | 0.14498   | 0.000       |  |
| accentDiodeDrop        | 0.30033   | 0.30033   | 0.000       |  |
| vcfDecayMinSec         | 0.056061  | 0.056061  | 0.000       |  |
| vcfDecayMaxSec         | 1.1404    | 1.1404    | 0.000       |  |
| vcfDecayTaper          | 17.982    | 17.982    | 0.000       |  |
| vegDecaySec            | 1.0379    | 1.0379    | 0.000       |  |
| vcaResTapRatio         | 1.0503    | 1.0503    | 0.000       |  |

Timing: note-on offset 0.00 ms (relative to each clip's measured note-on), gate length 1300.0 ms.

## Applying the result

Paste `synth_parameters.txt` into `acidus::SynthParameters` (src/core/SynthEngine.hpp) or rerun with `--apply`.

Audio: `renders/` (hardware / before / after for the 40 worst samples).
