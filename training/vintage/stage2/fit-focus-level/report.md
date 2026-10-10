# Acidus vs. hardware TB-303 calibration report (20261010-140314)

61 reference samples, 7 free parameters. Search: 127 evaluations in 10.3 min, 0 CMA-ES restarts, stopped because: time cap reached.

## Match quality (mean over samples)

All errors are dB; lower is better. `harm` = full-note harmonic levels (k^-0.5 weighted RMS), `inter` = energy between harmonics, `env` = RMS envelope, `stft` = 1/3-octave levels over time, `peak` = shape (height+width+implicit frequency) of the resonant-peak window on Resonance=max samples only -- weighted more heavily than `harm` so it can't be traded away.

| metric                                         | before | after |
|------------------------------------------------|--------|-------|
| objective (incl. priors)                       | 1.93   | 1.89  |
| weighted error                                 | 1.93   | 1.89  |
| harmonic error (dB)                            | 1.87   | 1.87  |
| resonant-peak shape error (dB)                 | 1.18   | 1.17  |
| inter-harmonic error (dB)                      | 0.72   | 0.72  |
| envelope error (dB)                            | 3.59   | 3.23  |
| spectrogram error (dB)                         | 6.59   | 6.45  |
| aligned waveform error (10*sqrt(residual/ref)) | 0.00   | 0.00  |
| resonant-peak sweep track error (semitones)    | 2.02   | 2.01  |
| harmonics-over-time error (dB)                 | 4.86   | 4.68  |
| mean |harmonic error| (dB)                     | 2.35   | 2.36  |
| note level error, RMS over notes (dB)          | 2.42   | 2.39  |
| harmonics within 1 dB (%)                      | 23.19  | 22.70 |
| harmonics within 3 dB (%)                      | 68.30  | 67.63 |
| harmonics within 6 dB (%)                      | 98.21  | 98.34 |

Recording gain solved as -11.1 dB (before: -11.1 dB).

## Per set

Mean error of each set's notes, and the RMS of their level errors (the recordings are not normalised, so absolute note levels are compared under one global gain).

| set | notes | before | after | level before (dB) | level after (dB) | harm | env | stft |
|-----|-------|--------|-------|-------------------|------------------|------|-----|------|
| 1A  | 9     | 2.02   | 1.94  | 3.5               | 3.3              | 2.1  | 3.2 | 6.5  |
| 1B  | 5     | 2.00   | 1.95  | 2.2               | 2.2              | 1.7  | 3.9 | 6.6  |
| 1C  | 3     | 1.82   | 1.78  | 2.1               | 2.1              | 2.1  | 3.9 | 5.6  |
| 1D  | 4     | 2.17   | 2.12  | 1.5               | 1.3              | 1.6  | 2.5 | 6.8  |
| 1E  | 2     | 1.01   | 1.05  | 2.9               | 3.4              | 2.6  | 2.9 | 4.9  |
| 1R  | 1     | 2.02   | 1.90  | 2.2               | 2.4              | 1.5  | 2.8 | 7.1  |
| 2A  | 16    | 1.92   | 1.86  | 2.6               | 2.6              | 1.6  | 3.6 | 6.5  |
| 2B  | 14    | 2.01   | 1.97  | 1.8               | 1.9              | 2.2  | 2.6 | 6.4  |
| 2C  | 6     | 1.88   | 1.88  | 1.6               | 1.6              | 1.7  | 3.9 | 6.6  |
| 2R  | 1     | 1.48   | 1.48  | 2.2               | 2.3              | 1.8  | 2.8 | 7.0  |

## Per sample (sorted by remaining error, worst first)

| sample | before | after | harm | peak | inter | env | stft | wave | harm ±3dB | level before | level after | tuning (c) | drift (c) |
|--------|--------|-------|------|------|-------|-----|------|------|-----------|--------------|-------------|------------|-----------|
| 2A-14  | 3.84   | 3.81  | 1.3  | 7.7  | 0.7   | 2.0 | 5.9  | 0.0  | 94%       | +1.8         | +1.8        | +0.0       | 0.00      |
| 1D-4   | 3.77   | 3.70  | 0.8  | 3.6  | 0.1   | 2.1 | 9.0  | 0.0  | 100%      | -0.3         | -1.2        | +0.0       | 0.00      |
| 1A-9   | 3.13   | 3.19  | 3.4  | 5.2  | 2.5   | 2.7 | 4.9  | 0.0  | 15%       | +2.0         | +2.5        | +0.0       | 0.00      |
| 1A-3   | 3.29   | 3.15  | 2.2  | 3.1  | 0.0   | 1.3 | 6.6  | 0.0  | 31%       | -0.3         | -0.8        | +0.0       | 0.00      |
| 2A-7   | 3.08   | 2.98  | 1.8  | 2.5  | 0.0   | 1.5 | 6.7  | 0.0  | 44%       | +0.7         | +0.3        | +0.0       | 0.00      |
| 2B-14  | 2.83   | 2.82  | 5.6  | 0.9  | 3.3   | 3.3 | 7.5  | 0.0  | 80%       | +3.2         | +3.4        | +0.0       | 0.00      |
| 2A-5   | 2.77   | 2.76  | 1.6  | 0.9  | 0.0   | 2.1 | 7.4  | 0.0  | 52%       | +1.1         | +1.4        | +0.0       | 0.00      |
| 2A-2   | 2.77   | 2.67  | 0.7  | 1.6  | 0.0   | 1.5 | 8.1  | 0.0  | 100%      | +0.1         | +0.0        | +0.0       | 0.00      |
| 2B-7   | 2.70   | 2.63  | 1.2  | 1.4  | 0.0   | 1.0 | 7.8  | 0.0  | 80%       | -0.2         | -0.3        | +0.0       | 0.00      |
| 1D-3   | 2.74   | 2.62  | 1.6  | 3.2  | 0.2   | 1.6 | 5.3  | 0.0  | 69%       | +0.4         | +0.0        | +0.0       | 0.00      |
| 1B-5   | 2.60   | 2.57  | 3.0  | 1.0  | 1.6   | 4.9 | 5.2  | 0.0  | 68%       | +3.4         | +3.5        | +0.0       | 0.00      |
| 1B-4   | 2.48   | 2.47  | 2.3  | 0.9  | 2.2   | 7.1 | 7.6  | 0.0  | 76%       | -2.7         | -2.7        | +0.0       | 0.00      |
| 2B-8   | 2.57   | 2.47  | 1.4  | 1.5  | 0.1   | 1.3 | 6.9  | 0.0  | 72%       | -0.3         | -0.4        | +0.0       | 0.00      |
| 2C-5   | 2.39   | 2.38  | 2.5  | 0.6  | 1.6   | 3.1 | 7.6  | 0.0  | 31%       | -1.0         | -1.0        | +0.0       | 0.00      |
| 1A-6   | 2.48   | 2.37  | 1.4  | 1.8  | 0.0   | 2.0 | 6.9  | 0.0  | 77%       | +1.7         | +1.5        | +0.0       | 0.00      |
| 2A-6   | 2.34   | 2.29  | 0.6  | 1.1  | 0.2   | 7.0 | 7.6  | 0.0  | 100%      | +1.5         | +1.5        | +0.0       | 0.00      |
| 2C-6   | 2.25   | 2.26  | 2.4  | 0.6  | 1.6   | 2.7 | 5.9  | 0.0  | 71%       | -1.6         | -1.6        | +0.0       | 0.00      |
| 2B-6   | 2.24   | 2.18  | 2.1  | 0.4  | 1.6   | 2.1 | 6.7  | 0.0  | 55%       | +1.0         | +1.0        | +0.0       | 0.00      |
| 2B-13  | 2.12   | 2.07  | 3.8  | 0.9  | 1.8   | 6.2 | 4.5  | 0.0  | 38%       | +3.0         | +3.2        | +0.0       | 0.00      |
| 2B-11  | 1.94   | 1.95  | 3.0  | 0.8  | 0.8   | 2.9 | 6.8  | 0.0  | 28%       | +2.9         | +3.0        | +0.0       | 0.00      |
| 2A-9   | 1.96   | 1.94  | 1.3  | 0.6  | 0.2   | 2.5 | 7.5  | 0.0  | 98%       | +2.2         | +2.2        | +0.0       | 0.00      |
| 1C-3   | 1.90   | 1.92  | 2.5  | 0.8  | 0.4   | 2.1 | 5.8  | 0.0  | 74%       | -2.5         | -2.4        | +0.0       | 0.00      |
| 2B-1   | 1.95   | 1.90  | 1.1  | 0.6  | 0.0   | 2.5 | 8.0  | 0.0  | 100%      | +1.7         | +1.7        | +0.0       | 0.00      |
| 1R-1   | 2.02   | 1.90  | 1.5  | 0.5  | 0.0   | 2.8 | 7.1  | 0.0  | 74%       | +2.2         | +2.4        | +0.0       | 0.00      |
| 2C-4   | 1.85   | 1.85  | 1.4  | 0.6  | 1.0   | 3.9 | 7.2  | 0.0  | 93%       | -1.0         | -1.0        | +0.0       | 0.00      |
| 2B-10  | 1.91   | 1.85  | 2.7  | 0.3  | 2.6   | 2.3 | 6.7  | 0.0  | 25%       | -1.3         | -1.4        | +0.0       | 0.00      |
| 2B-5   | 1.90   | 1.84  | 2.2  | 0.4  | 1.9   | 2.2 | 6.8  | 0.0  | 30%       | +0.9         | +0.9        | +0.0       | 0.00      |
| 2B-12  | 1.78   | 1.82  | 2.8  | 0.9  | 1.8   | 3.0 | 5.0  | 0.0  | 33%       | +2.9         | +3.0        | +0.0       | 0.00      |
| 2A-16  | 1.86   | 1.80  | 2.3  | 0.9  | 0.6   | 3.2 | 6.2  | 0.0  | 92%       | +3.0         | +3.1        | +0.0       | 0.00      |
| 1A-7   | 1.97   | 1.79  | 3.0  | -    | 1.2   | 7.5 | 8.1  | 0.0  | 92%       | +8.5         | +7.4        | +0.0       | 0.00      |
| 2A-13  | 1.82   | 1.78  | 1.3  | 0.8  | 0.3   | 2.2 | 7.2  | 0.0  | 96%       | +2.1         | +2.2        | +0.0       | 0.00      |
| 1C-1   | 1.86   | 1.75  | 1.6  | 0.4  | 2.4   | 4.7 | 7.0  | 0.0  | 90%       | -2.3         | -2.4        | +0.0       | 0.00      |
| 1A-5   | 1.72   | 1.74  | 1.3  | 0.5  | 0.0   | 4.2 | 5.4  | 0.0  | 89%       | +2.2         | +2.4        | +0.0       | 0.00      |
| 2C-1   | 1.70   | 1.71  | 1.2  | 0.4  | 0.3   | 3.7 | 7.7  | 0.0  | 69%       | -2.4         | -2.5        | +0.0       | 0.00      |
| 1B-1   | 1.75   | 1.70  | 0.8  | 0.6  | 0.0   | 2.6 | 6.8  | 0.0  | 100%      | +1.8         | +1.8        | +0.0       | 0.00      |
| 1C-2   | 1.70   | 1.69  | 2.0  | 0.6  | 3.1   | 4.7 | 4.1  | 0.0  | 31%       | -1.1         | -1.1        | +0.0       | 0.00      |
| 2A-12  | 1.73   | 1.67  | 1.3  | 0.6  | 0.1   | 2.7 | 6.8  | 0.0  | 97%       | +2.6         | +2.6        | +0.0       | 0.00      |
| 2B-4   | 1.77   | 1.66  | 2.2  | 0.5  | 1.8   | 2.3 | 5.9  | 0.0  | 30%       | +1.1         | +1.1        | +0.0       | 0.00      |
| 1B-2   | 1.69   | 1.63  | 1.3  | 0.6  | 0.0   | 2.6 | 7.6  | 0.0  | 100%      | +1.5         | +1.5        | +0.0       | 0.00      |
| 2C-2   | 1.61   | 1.60  | 1.3  | 0.4  | 2.3   | 5.2 | 5.5  | 0.0  | 88%       | -1.8         | -1.9        | +0.0       | 0.00      |
| 2B-2   | 1.64   | 1.60  | 0.9  | 0.5  | 0.0   | 2.4 | 6.7  | 0.0  | 100%      | +1.4         | +1.4        | +0.0       | 0.00      |
| 1A-8   | 1.58   | 1.53  | 1.4  | 0.9  | 0.8   | 2.7 | 5.4  | 0.0  | 95%       | +2.9         | +2.7        | +0.0       | 0.00      |
| 2A-10  | 1.60   | 1.53  | 2.6  | -    | 1.2   | 8.7 | 4.8  | 0.0  | 60%       | +6.8         | +6.5        | +0.0       | 0.00      |
| 2C-3   | 1.44   | 1.50  | 1.2  | 0.5  | 0.4   | 4.6 | 5.7  | 0.0  | 91%       | -1.4         | -1.5        | +0.0       | 0.00      |
| 2R-1   | 1.48   | 1.48  | 1.8  | 0.5  | 0.0   | 2.8 | 7.0  | 0.0  | 46%       | +2.2         | +2.3        | +0.0       | 0.00      |
| 1A-2   | 1.60   | 1.41  | 1.6  | 0.9  | 0.0   | 1.1 | 8.3  | 0.0  | 43%       | -0.5         | -0.2        | +0.0       | 0.00      |
| 1B-3   | 1.49   | 1.39  | 1.0  | 0.4  | 0.2   | 2.1 | 5.9  | 0.0  | 99%       | +0.4         | +0.4        | +0.0       | 0.00      |
| 2B-9   | 1.43   | 1.38  | 1.0  | 0.7  | 0.0   | 1.3 | 6.9  | 0.0  | 87%       | -0.5         | -0.6        | +0.0       | 0.00      |
| 2B-3   | 1.39   | 1.35  | 1.0  | 0.5  | 1.6   | 2.9 | 4.2  | 0.0  | 100%      | +1.2         | +1.2        | +0.0       | 0.00      |
| 2A-1   | 1.35   | 1.28  | 1.6  | -    | 0.0   | 7.1 | 5.7  | 0.0  | 61%       | -1.1         | -0.4        | +0.0       | 0.00      |
| 1A-4   | 1.26   | 1.25  | 2.4  | -    | 0.0   | 4.4 | 6.5  | 0.0  | 36%       | +4.0         | +4.4        | +0.0       | 0.00      |
| 1D-1   | 1.15   | 1.12  | 2.6  | -    | 0.0   | 1.8 | 6.9  | 0.0  | 24%       | +1.7         | +2.1        | +0.0       | 0.00      |
| 1E-2   | 1.07   | 1.11  | 2.3  | -    | 0.0   | 3.2 | 6.1  | 0.0  | 34%       | +3.9         | +4.2        | +0.0       | 0.00      |
| 2A-8   | 1.12   | 1.09  | 2.0  | -    | 0.0   | 2.6 | 7.1  | 0.0  | 48%       | +2.4         | +2.6        | +0.0       | 0.00      |
| 2A-4   | 1.08   | 1.06  | 1.7  | -    | 0.0   | 2.3 | 7.4  | 0.0  | 47%       | +0.7         | +1.3        | +0.0       | 0.00      |
| 2A-15  | 1.23   | 1.06  | 1.5  | -    | 0.9   | 6.6 | 3.1  | 0.0  | 94%       | +3.7         | +3.1        | +0.0       | 0.00      |
| 1D-2   | 1.03   | 1.04  | 1.4  | -    | 0.0   | 4.3 | 5.9  | 0.0  | 74%       | +2.4         | +1.1        | +0.0       | 0.00      |
| 2A-11  | 1.11   | 1.04  | 1.8  | -    | 0.1   | 3.1 | 6.2  | 0.0  | 77%       | +3.0         | +2.9        | +0.0       | 0.00      |
| 1A-1   | 1.12   | 1.01  | 1.9  | -    | 0.0   | 2.9 | 5.9  | 0.0  | 40%       | -1.6         | -0.7        | +0.0       | 0.00      |
| 1E-1   | 0.96   | 0.99  | 2.9  | -    | 0.6   | 2.6 | 3.7  | 0.0  | 42%       | +0.9         | +2.4        | +0.0       | 0.00      |
| 2A-3   | 1.01   | 0.97  | 1.8  | -    | 0.0   | 1.9 | 6.6  | 0.0  | 45%       | +0.9         | +1.6        | +0.0       | 0.00      |

### Resonant peak (Resonance=max samples)

| sample | peak Hz (hw) | peak Hz (before) | peak Hz (after) | height dB (hw) | height dB (before) | height dB (after) |
|--------|--------------|------------------|-----------------|----------------|--------------------|-------------------|
| 1A-2   | 131          | 65               | 65              | -1.5           | +0.0               | +0.0              |
| 1A-3   | 196          | 196              | 196             | -0.1           | -2.0               | -2.0              |
| 1A-5   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1A-6   | 327          | 392              | 15174           | -1.9           | -3.5               | -134.7            |
| 1A-8   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1A-9   | 1701         | 1766             | 1766            | -3.9           | -10.7              | -10.7             |
| 1B-1   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1B-2   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1B-3   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1B-4   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1B-5   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1C-1   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1C-2   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1C-3   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1D-3   | 589          | 11446            | 65              | -10.3          | -147.0             | +0.0              |
| 1D-4   | 981          | 14782            | 1046            | -6.4           | -134.4             | -13.3             |
| 1R-1   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 2A-2   | 131          | 65               | 65              | +0.5           | +0.0               | +0.0              |
| 2A-5   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 2A-6   | 196          | 196              | 196             | -2.3           | -3.2               | -3.2              |
| 2A-7   | 196          | 262              | 262             | +1.3           | -3.4               | -3.4              |
| 2A-9   | 65           | 327              | 327             | +0.0           | -5.3               | -5.3              |
| 2A-12  | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 2A-13  | 65           | 720              | 720             | +0.0           | -9.8               | -9.8              |
| 2A-14  | 1439         | 785              | 785             | -18.3          | -7.0               | -7.0              |
| 2A-16  | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 2B-1   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 2B-2   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 2B-3   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 2B-4   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 2B-5   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 2B-6   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 2B-7   | 131          | 65               | 65              | -1.0           | +0.0               | +0.0              |
| 2B-8   | 131          | 65               | 65              | -0.9           | +0.0               | +0.0              |
| 2B-9   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 2B-10  | 65           | 14782            | 65              | +0.0           | -141.0             | +0.0              |
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

- **1A-1** worst: k17 1112Hz ref -65.4 / sim -72.1, k21 1374Hz ref -72.7 / sim -80.1, k20 1308Hz ref -71.0 / sim -78.4, k18 1177Hz ref -67.5 / sim -74.4. Model excess above 4 kHz: none > 3 dB.
- **1A-2** worst: k21 1374Hz ref -68.4 / sim -74.0, k25 1635Hz ref -74.7 / sim -81.1, k19 1243Hz ref -64.8 / sim -70.0, k17 1112Hz ref -60.8 / sim -65.5. Model excess above 4 kHz: none > 3 dB.
- **1A-3** worst: k14 916Hz ref -47.4 / sim -54.1, k13 850Hz ref -45.1 / sim -51.1, k23 1504Hz ref -65.8 / sim -74.0, k26 1701Hz ref -70.5 / sim -79.0. Model excess above 4 kHz: none > 3 dB.
- **1A-4** worst: k31 2028Hz ref -68.8 / sim -74.7, k32 2093Hz ref -69.9 / sim -75.9, k33 2158Hz ref -71.0 / sim -77.2, k30 1962Hz ref -67.6 / sim -73.4. Model excess above 4 kHz: none > 3 dB.
- **1A-5** worst: k5 327Hz ref -8.9 / sim -5.6, k6 392Hz ref -12.3 / sim -9.1, k43 2812Hz ref -76.8 / sim -80.2, k42 2747Hz ref -76.0 / sim -79.2. Model excess above 4 kHz: none > 3 dB.
- **1A-6** worst: k28 1831Hz ref -55.8 / sim -59.5, k29 1897Hz ref -57.2 / sim -60.9, k31 2028Hz ref -59.7 / sim -63.4, k32 2093Hz ref -61.0 / sim -64.6. Model excess above 4 kHz: none > 3 dB.
- **1A-7** worst: k3 196Hz ref -8.4 / sim 1.2, k2 131Hz ref -4.9 / sim 4.5, k4 262Hz ref -10.7 / sim -2.1, k5 327Hz ref -12.5 / sim -5.2. Model excess above 4 kHz: none > 3 dB.
- **1A-8** worst: k4 262Hz ref -10.4 / sim -6.5, k3 196Hz ref -8.2 / sim -4.3, k5 327Hz ref -11.9 / sim -8.2, k2 131Hz ref -4.9 / sim -1.4. Model excess above 4 kHz: none > 3 dB.
- **1A-9** worst: k87 5690Hz ref -62.2 / sim -56.4, k99 6475Hz ref -67.0 / sim -61.4, k88 5756Hz ref -62.4 / sim -56.8, k112 7326Hz ref -72.5 / sim -67.0. Model excess above 4 kHz: harmonic 5690Hz +5.7dB, harmonic 6475Hz +5.6dB, harmonic 5756Hz +5.5dB, inter-harmonic 4088Hz +4.1dB.
- **1B-1** worst: k6 392Hz ref -8.2 / sim -5.3, k7 458Hz ref -9.6 / sim -7.3, k3 196Hz ref -6.4 / sim -4.2, k2 131Hz ref -4.4 / sim -2.2. Model excess above 4 kHz: none > 3 dB.
- **1B-2** worst: k38 2485Hz ref -57.1 / sim -59.8, k39 2551Hz ref -58.2 / sim -60.9, k37 2420Hz ref -56.0 / sim -58.7, k40 2616Hz ref -59.2 / sim -61.9. Model excess above 4 kHz: none > 3 dB.
- **1B-3** worst: k24 1570Hz ref -21.0 / sim -24.0, k30 1962Hz ref -23.3 / sim -26.3, k23 1504Hz ref -20.8 / sim -23.5, k21 1374Hz ref -20.0 / sim -22.6. Model excess above 4 kHz: none > 3 dB.
- **1B-4** worst: k41 2682Hz ref -42.8 / sim -37.6, k42 2747Hz ref -43.8 / sim -38.7, k40 2616Hz ref -41.6 / sim -36.5, k43 2812Hz ref -44.9 / sim -39.9. Model excess above 4 kHz: none > 3 dB.
- **1B-5** worst: k168 10988Hz ref -49.7 / sim -44.9, k167 10923Hz ref -49.6 / sim -44.8, k10 654Hz ref -14.9 / sim -10.2, k166 10858Hz ref -49.4 / sim -44.7. Model excess above 4 kHz: harmonic 14586Hz +6.0dB, harmonic 14782Hz +6.0dB, harmonic 14520Hz +6.0dB, inter-harmonic 14226Hz +5.8dB.
- **1C-1** worst: k7 458Hz ref -17.4 / sim -21.3, k6 392Hz ref -15.9 / sim -18.7, k2 131Hz ref -6.0 / sim -8.5, k1 65Hz ref 0.0 / sim -2.3. Model excess above 4 kHz: inter-harmonic 4088Hz +5.1dB, inter-harmonic 4153Hz +4.2dB, inter-harmonic 4219Hz +3.4dB.
- **1C-2** worst: k105 6868Hz ref -77.6 / sim -73.9, k106 6933Hz ref -78.0 / sim -74.3, k48 3140Hz ref -44.7 / sim -41.0, k104 6802Hz ref -77.2 / sim -73.5. Model excess above 4 kHz: inter-harmonic 4088Hz +4.2dB, inter-harmonic 6181Hz +3.9dB, inter-harmonic 6246Hz +3.9dB, harmonic 6868Hz +3.7dB.
- **1C-3** worst: k37 2420Hz ref -21.4 / sim -27.5, k36 2355Hz ref -21.1 / sim -27.2, k38 2485Hz ref -21.7 / sim -27.8, k35 2289Hz ref -20.8 / sim -26.9. Model excess above 4 kHz: none > 3 dB.
- **1D-1** worst: k28 1831Hz ref -66.1 / sim -72.3, k29 1897Hz ref -67.3 / sim -73.5, k30 1962Hz ref -68.5 / sim -74.8, k31 2028Hz ref -69.7 / sim -76.1. Model excess above 4 kHz: none > 3 dB.
- **1D-2** worst: k9 589Hz ref -19.4 / sim -25.1, k10 654Hz ref -20.6 / sim -27.0, k8 523Hz ref -18.2 / sim -22.9, k11 720Hz ref -21.7 / sim -28.7. Model excess above 4 kHz: none > 3 dB.
- **1D-3** worst: k29 1897Hz ref -56.3 / sim -61.3, k28 1831Hz ref -54.9 / sim -59.8, k30 1962Hz ref -57.8 / sim -62.6, k27 1766Hz ref -53.5 / sim -58.3. Model excess above 4 kHz: none > 3 dB.
- **1D-4** worst: k69 4513Hz ref -79.0 / sim -76.6, k70 4578Hz ref -79.6 / sim -77.2, k46 3009Hz ref -58.8 / sim -61.1, k68 4448Hz ref -78.3 / sim -76.1. Model excess above 4 kHz: none > 3 dB.
- **1E-1** worst: k13 850Hz ref -42.6 / sim -57.1, k15 981Hz ref -47.8 / sim -69.9, k11 720Hz ref -37.1 / sim -46.9, k28 1831Hz ref -69.8 / sim -81.6. Model excess above 4 kHz: none > 3 dB.
- **1E-2** worst: k30 1962Hz ref -68.1 / sim -73.8, k31 2028Hz ref -69.2 / sim -75.1, k32 2093Hz ref -70.4 / sim -76.3, k29 1897Hz ref -66.8 / sim -72.4. Model excess above 4 kHz: none > 3 dB.
- **1R-1** worst: k42 2747Hz ref -75.5 / sim -79.2, k43 2812Hz ref -76.3 / sim -80.2, k41 2682Hz ref -74.5 / sim -78.2, k40 2616Hz ref -73.6 / sim -77.2. Model excess above 4 kHz: none > 3 dB.
- **2A-1** worst: k21 1374Hz ref -70.2 / sim -77.2, k20 1308Hz ref -68.5 / sim -75.2, k19 1243Hz ref -66.9 / sim -73.1, k18 1177Hz ref -65.4 / sim -71.0. Model excess above 4 kHz: none > 3 dB.
- **2A-2** worst: k7 458Hz ref -26.7 / sim -29.0, k5 327Hz ref -18.7 / sim -16.5, k12 785Hz ref -47.5 / sim -49.6, k2 131Hz ref 0.0 / sim -1.7. Model excess above 4 kHz: none > 3 dB.
- **2A-3** worst: k22 1439Hz ref -69.2 / sim -75.1, k21 1374Hz ref -67.6 / sim -73.4, k20 1308Hz ref -65.9 / sim -71.5, k24 1570Hz ref -72.3 / sim -78.5. Model excess above 4 kHz: none > 3 dB.
- **2A-4** worst: k23 1504Hz ref -67.3 / sim -73.1, k21 1374Hz ref -64.0 / sim -69.4, k22 1439Hz ref -65.8 / sim -71.2, k25 1635Hz ref -70.3 / sim -76.4. Model excess above 4 kHz: none > 3 dB.
- **2A-5** worst: k29 1897Hz ref -74.3 / sim -79.3, k28 1831Hz ref -73.0 / sim -77.9, k27 1766Hz ref -71.5 / sim -76.4, k26 1701Hz ref -70.1 / sim -74.9. Model excess above 4 kHz: none > 3 dB.
- **2A-6** worst: k4 262Hz ref -7.5 / sim -4.6, k1 65Hz ref 0.0 / sim 1.7, k5 327Hz ref -11.4 / sim -10.2, k2 131Hz ref -2.1 / sim -0.9. Model excess above 4 kHz: none > 3 dB.
- **2A-7** worst: k31 2028Hz ref -74.3 / sim -79.6, k28 1831Hz ref -70.3 / sim -75.4, k27 1766Hz ref -68.9 / sim -73.9, k24 1570Hz ref -64.2 / sim -69.1. Model excess above 4 kHz: none > 3 dB.
- **2A-8** worst: k34 2224Hz ref -68.7 / sim -73.7, k35 2289Hz ref -69.8 / sim -74.9, k36 2355Hz ref -70.8 / sim -76.1, k37 2420Hz ref -71.8 / sim -77.1. Model excess above 4 kHz: none > 3 dB.
- **2A-9** worst: k6 392Hz ref -8.9 / sim -5.1, k25 1635Hz ref -53.8 / sim -56.6, k24 1570Hz ref -52.2 / sim -55.0, k23 1504Hz ref -50.6 / sim -53.3. Model excess above 4 kHz: none > 3 dB.
- **2A-10** worst: k2 131Hz ref -4.9 / sim 2.6, k1 65Hz ref 0.0 / sim 6.9, k3 196Hz ref -8.3 / sim -2.3, k4 262Hz ref -10.7 / sim -6.7. Model excess above 4 kHz: none > 3 dB.
- **2A-11** worst: k3 196Hz ref -7.6 / sim -3.8, k64 4186Hz ref -76.2 / sim -79.7, k4 262Hz ref -9.3 / sim -5.7, k2 131Hz ref -4.7 / sim -1.1. Model excess above 4 kHz: none > 3 dB.
- **2A-12** worst: k3 196Hz ref -7.6 / sim -4.2, k4 262Hz ref -9.2 / sim -6.0, k2 131Hz ref -4.6 / sim -1.6, k5 327Hz ref -9.9 / sim -7.1. Model excess above 4 kHz: none > 3 dB.
- **2A-13** worst: k11 720Hz ref -11.1 / sim -7.8, k3 196Hz ref -7.8 / sim -4.6, k4 262Hz ref -9.5 / sim -6.4, k5 327Hz ref -10.4 / sim -7.6. Model excess above 4 kHz: none > 3 dB.
- **2A-14** worst: k4 262Hz ref -9.7 / sim -6.4, k3 196Hz ref -7.9 / sim -4.5, k5 327Hz ref -10.8 / sim -7.6, k12 785Hz ref -7.8 / sim -4.8. Model excess above 4 kHz: none > 3 dB.
- **2A-15** worst: k4 262Hz ref -10.4 / sim -6.2, k3 196Hz ref -8.2 / sim -4.0, k5 327Hz ref -11.9 / sim -7.9, k2 131Hz ref -4.9 / sim -1.1. Model excess above 4 kHz: none > 3 dB.
- **2A-16** worst: k4 262Hz ref -10.4 / sim -6.0, k3 196Hz ref -8.2 / sim -3.8, k5 327Hz ref -12.0 / sim -7.7, k6 392Hz ref -13.2 / sim -9.1. Model excess above 4 kHz: none > 3 dB.
- **2B-1** worst: k6 392Hz ref -8.6 / sim -5.8, k28 1831Hz ref -52.5 / sim -54.8, k29 1897Hz ref -53.9 / sim -56.2, k27 1766Hz ref -51.1 / sim -53.4. Model excess above 4 kHz: none > 3 dB.
- **2B-2** worst: k5 327Hz ref -8.6 / sim -6.5, k2 131Hz ref -4.2 / sim -2.2, k4 262Hz ref -6.6 / sim -4.7, k3 196Hz ref -5.8 / sim -4.0. Model excess above 4 kHz: none > 3 dB.
- **2B-3** worst: k86 5625Hz ref -79.3 / sim -77.3, k87 5690Hz ref -79.7 / sim -77.7, k29 1897Hz ref -34.8 / sim -32.8, k85 5560Hz ref -78.8 / sim -76.8. Model excess above 4 kHz: none > 3 dB.
- **2B-4** worst: k32 2093Hz ref -37.2 / sim -31.5, k33 2158Hz ref -38.7 / sim -33.0, k31 2028Hz ref -35.6 / sim -30.1, k34 2224Hz ref -40.1 / sim -34.6. Model excess above 4 kHz: harmonic 4251Hz +3.8dB, harmonic 4317Hz +3.8dB, harmonic 4382Hz +3.8dB.
- **2B-5** worst: k39 2551Hz ref -39.8 / sim -34.4, k38 2485Hz ref -38.5 / sim -33.1, k40 2616Hz ref -41.0 / sim -35.7, k37 2420Hz ref -37.2 / sim -31.9. Model excess above 4 kHz: harmonic 5036Hz +3.7dB, harmonic 4971Hz +3.7dB, harmonic 5102Hz +3.7dB.
- **2B-6** worst: k51 3336Hz ref -41.5 / sim -36.6, k50 3270Hz ref -40.5 / sim -35.6, k52 3401Hz ref -42.4 / sim -37.6, k71 4644Hz ref -56.6 / sim -51.9. Model excess above 4 kHz: harmonic 4644Hz +4.7dB, harmonic 5102Hz +4.6dB, harmonic 4840Hz +4.5dB, inter-harmonic 4088Hz +4.5dB.
- **2B-7** worst: k14 916Hz ref -47.2 / sim -50.9, k15 981Hz ref -49.9 / sim -53.5, k13 850Hz ref -44.5 / sim -48.0, k16 1046Hz ref -52.6 / sim -56.0. Model excess above 4 kHz: none > 3 dB.
- **2B-8** worst: k15 981Hz ref -48.0 / sim -51.9, k16 1046Hz ref -50.5 / sim -54.4, k14 916Hz ref -45.4 / sim -49.2, k17 1112Hz ref -53.0 / sim -56.7. Model excess above 4 kHz: none > 3 dB.
- **2B-9** worst: k21 1374Hz ref -53.8 / sim -57.0, k20 1308Hz ref -51.9 / sim -55.1, k19 1243Hz ref -49.9 / sim -53.0, k18 1177Hz ref -47.8 / sim -50.9. Model excess above 4 kHz: none > 3 dB.
- **2B-10** worst: k19 1243Hz ref -37.0 / sim -29.2, k20 1308Hz ref -38.9 / sim -31.3, k18 1177Hz ref -34.7 / sim -27.3, k21 1374Hz ref -40.7 / sim -33.7. Model excess above 4 kHz: none > 3 dB.
- **2B-11** worst: k99 6475Hz ref -63.6 / sim -59.3, k98 6410Hz ref -63.2 / sim -58.9, k101 6606Hz ref -64.5 / sim -60.2, k100 6541Hz ref -64.0 / sim -59.7. Model excess above 4 kHz: harmonic 6475Hz +4.3dB, harmonic 6410Hz +4.3dB, harmonic 6606Hz +4.3dB.
- **2B-12** worst: k4 262Hz ref -10.5 / sim -6.2, k3 196Hz ref -8.2 / sim -4.0, k105 6868Hz ref -64.7 / sim -60.5, k106 6933Hz ref -65.1 / sim -60.9. Model excess above 4 kHz: harmonic 6868Hz +4.2dB, harmonic 6933Hz +4.2dB, harmonic 6802Hz +4.2dB.
- **2B-13** worst: k127 8307Hz ref -69.0 / sim -62.6, k126 8241Hz ref -68.6 / sim -62.3, k125 8176Hz ref -68.2 / sim -62.0, k124 8110Hz ref -67.9 / sim -61.6. Model excess above 4 kHz: harmonic 8372Hz +6.4dB, harmonic 8437Hz +6.3dB, harmonic 8307Hz +6.3dB, inter-harmonic 4611Hz +6.3dB.
- **2B-14** worst: k4 262Hz ref -10.4 / sim -5.8, k3 196Hz ref -8.2 / sim -3.7, k5 327Hz ref -12.0 / sim -7.5, k6 392Hz ref -13.1 / sim -8.9. Model excess above 4 kHz: harmonic 8045Hz +11.9dB, harmonic 8110Hz +11.9dB, harmonic 7980Hz +11.9dB, inter-harmonic 7816Hz +10.3dB.
- **2C-1** worst: k9 589Hz ref -17.3 / sim -21.6, k7 458Hz ref -15.6 / sim -19.3, k8 523Hz ref -16.7 / sim -20.2, k10 654Hz ref -18.2 / sim -23.7. Model excess above 4 kHz: none > 3 dB.
- **2C-2** worst: k11 720Hz ref -17.8 / sim -21.7, k12 785Hz ref -18.8 / sim -22.9, k49 3205Hz ref -51.9 / sim -48.5, k48 3140Hz ref -50.9 / sim -47.6. Model excess above 4 kHz: inter-harmonic 4088Hz +3.7dB, inter-harmonic 4153Hz +3.0dB.
- **2C-3** worst: k14 916Hz ref -18.6 / sim -22.2, k13 850Hz ref -17.9 / sim -21.4, k15 981Hz ref -19.4 / sim -22.9, k16 1046Hz ref -19.5 / sim -23.3. Model excess above 4 kHz: none > 3 dB.
- **2C-4** worst: k21 1374Hz ref -19.9 / sim -23.9, k20 1308Hz ref -19.6 / sim -23.4, k18 1177Hz ref -19.0 / sim -22.3, k19 1243Hz ref -19.5 / sim -22.9. Model excess above 4 kHz: none > 3 dB.
- **2C-5** worst: k48 3140Hz ref -40.4 / sim -35.3, k49 3205Hz ref -41.5 / sim -36.4, k50 3270Hz ref -42.4 / sim -37.5, k47 3074Hz ref -39.3 / sim -34.4. Model excess above 4 kHz: harmonic 4055Hz +4.0dB, harmonic 4121Hz +3.9dB, harmonic 4186Hz +3.8dB.
- **2C-6** worst: k33 2158Hz ref -21.5 / sim -26.3, k34 2224Hz ref -21.8 / sim -26.6, k32 2093Hz ref -21.2 / sim -26.0, k31 2028Hz ref -20.9 / sim -25.6. Model excess above 4 kHz: none > 3 dB.
- **2R-1** worst: k41 2682Hz ref -73.8 / sim -78.2, k39 2551Hz ref -71.9 / sim -76.2, k40 2616Hz ref -72.8 / sim -77.2, k42 2747Hz ref -74.6 / sim -79.2. Model excess above 4 kHz: none > 3 dB.

## Suspicious samples

- **2C-4**: spectrum is within 1.2 dB of 2C-5 although the decay label differs -- one of the two labels is probably wrong
- **2B-11**: spectrum is within 1.2 dB of 2B-12 although the envMod label differs -- one of the two labels is probably wrong
- **2B-3**: spectrum is within 1.5 dB of 2B-4 although the envMod label differs -- one of the two labels is probably wrong
- **2A-6**: spectrum is within 1.0 dB of 2B-7 although the cutoff/decay label differs -- one of the two labels is probably wrong
- **1D-1**: spectrum is within 1.0 dB of 1E-2 although the cutoff/envMod/decay/accent label differs -- one of the two labels is probably wrong
- **1A-4**: spectrum is within 1.2 dB of 1D-1 although the cutoff/envMod/decay/accent label differs -- one of the two labels is probably wrong
- **1A-3**: spectrum is within 1.4 dB of 2B-7 although the resonance/decay label differs -- one of the two labels is probably wrong
- **2A-14**: error 3.81 is 2.1x the median
- **1D-4**: error 3.70 is 2.1x the median
- **1A-9**: error 3.19 is 1.8x the median
- **1A-3**: error 3.15 is 1.8x the median
- **2A-7**: error 2.98 is 1.7x the median
- **2B-14**: error 2.82 is 1.6x the median
- **2A-5**: error 2.76 is 1.6x the median
- **2A-2**: error 2.67 is 1.5x the median

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

Cutoff law with the fitted end stops folded in (so the plugin's knob range equals the hardware's): `cutoffBaseHz = 259.3`, `cutoffSpanOct = 3.298`, `cutoffTaperExp = 1.691`.

## Fitted model parameters

`sensitivity` = objective increase for a 5% (of range) nudge. Near-zero means the current references don't constrain that parameter -- its value is not evidence of anything. `AT BOUND` means the optimizer pushed it to the edge of its plausible range, usually a sign that it is compensating for something the model does not implement.

| parameter           | before  | after   | sensitivity |  |
|---------------------|---------|---------|-------------|--|
| oscSquareLevel      | 0.38901 | 0.44509 | 0.000       |  |
| accentVcaDepth      | 1.97    | 1.5019  | 0.000       |  |
| vcaResTapRatio      | 1.6209  | 1.7669  | 0.000       |  |
| vcaCutoffLevelDb    | -1.4606 | -2.0731 | 0.000       |  |
| vcaCutoffLevelResDb | 0.51894 | 0.81166 | 0.000       |  |

Timing: note-on offset -0.74 ms (relative to each clip's measured note-on), gate length 1311.1 ms.

## Applying the result

Paste `synth_parameters.txt` into `acidus::SynthParameters` (src/core/SynthEngine.hpp) or rerun with `--apply`.

Audio: `renders/` (hardware / before / after for the 40 worst samples).
