# Acidus vs. hardware TB-303 calibration report (20261010-123927)

61 reference samples, 43 free parameters. Search: 751 evaluations in 60.4 min, 0 CMA-ES restarts, stopped because: time cap reached.

## Match quality (mean over samples)

All errors are dB; lower is better. `harm` = full-note harmonic levels (k^-0.5 weighted RMS), `inter` = energy between harmonics, `env` = RMS envelope, `stft` = 1/3-octave levels over time, `peak` = shape (height+width+implicit frequency) of the resonant-peak window on Resonance=max samples only -- weighted more heavily than `harm` so it can't be traded away.

| metric                                         | before | after |
|------------------------------------------------|--------|-------|
| objective (incl. priors)                       | 2.30   | 1.81  |
| weighted error                                 | 2.29   | 1.80  |
| harmonic error (dB)                            | 1.89   | 1.57  |
| resonant-peak shape error (dB)                 | 2.00   | 0.86  |
| inter-harmonic error (dB)                      | 0.67   | 0.53  |
| envelope error (dB)                            | 3.50   | 3.40  |
| spectrogram error (dB)                         | 7.34   | 6.44  |
| aligned waveform error (10*sqrt(residual/ref)) | 0.00   | 0.00  |
| resonant-peak sweep track error (semitones)    | 2.46   | 2.13  |
| harmonics-over-time error (dB)                 | 5.86   | 4.62  |
| mean |harmonic error| (dB)                     | 2.26   | 1.84  |
| note level error, RMS over notes (dB)          | 2.03   | 2.83  |
| harmonics within 1 dB (%)                      | 30.21  | 33.70 |
| harmonics within 3 dB (%)                      | 74.22  | 80.26 |
| harmonics within 6 dB (%)                      | 94.92  | 98.98 |

Recording gain solved as -12.1 dB (before: -10.3 dB).

## Per set

Mean error of each set's notes, and the RMS of their level errors (the recordings are not normalised, so absolute note levels are compared under one global gain).

| set | notes | before | after | level before (dB) | level after (dB) | harm | env | stft |
|-----|-------|--------|-------|-------------------|------------------|------|-----|------|
| 1A  | 9     | 2.19   | 1.81  | 2.6               | 3.8              | 1.9  | 3.5 | 6.6  |
| 1B  | 5     | 2.75   | 1.98  | 1.9               | 2.3              | 1.4  | 3.7 | 6.9  |
| 1C  | 3     | 2.12   | 1.66  | 2.3               | 2.0              | 1.8  | 4.0 | 5.6  |
| 1D  | 4     | 2.15   | 1.91  | 0.7               | 2.6              | 1.3  | 2.6 | 6.2  |
| 1E  | 2     | 0.93   | 1.08  | 2.2               | 4.5              | 2.5  | 3.6 | 4.9  |
| 1R  | 1     | 2.98   | 1.74  | 3.0               | 2.9              | 1.2  | 3.2 | 7.4  |
| 2A  | 16    | 2.08   | 1.91  | 2.4               | 3.2              | 1.4  | 3.9 | 6.9  |
| 2B  | 14    | 2.69   | 1.79  | 1.2               | 1.9              | 1.6  | 2.5 | 6.2  |
| 2C  | 6     | 2.27   | 1.63  | 1.6               | 1.6              | 1.4  | 3.9 | 6.1  |
| 2R  | 1     | 1.98   | 1.52  | 2.9               | 2.8              | 1.4  | 3.2 | 7.1  |

## Per sample (sorted by remaining error, worst first)

| sample | before | after | harm | peak | inter | env | stft | wave | harm ±3dB | level before | level after | tuning (c) | drift (c) |
|--------|--------|-------|------|------|-------|-----|------|------|-----------|--------------|-------------|------------|-----------|
| 1D-4   | 3.39   | 3.46  | 1.1  | 3.5  | 2.2   | 1.6 | 7.1  | 0.0  | 97%       | -0.3         | -0.6        | +0.0       | 0.00      |
| 2A-14  | 3.19   | 3.09  | 1.1  | 4.7  | 0.5   | 1.6 | 5.5  | 0.0  | 96%       | +1.0         | +1.4        | +0.0       | 0.00      |
| 2A-7   | 3.06   | 3.02  | 1.4  | 2.2  | 0.0   | 1.5 | 7.6  | 0.0  | 50%       | +0.8         | -0.1        | +0.0       | 0.00      |
| 2A-2   | 2.34   | 2.89  | 0.7  | 1.7  | 0.0   | 1.5 | 8.6  | 0.0  | 94%       | +0.2         | +0.1        | +0.0       | 0.00      |
| 1B-5   | 3.39   | 2.85  | 2.3  | 0.5  | 0.9   | 4.9 | 6.3  | 0.0  | 83%       | +1.9         | +3.6        | +0.0       | 0.00      |
| 2A-6   | 2.76   | 2.83  | 1.0  | 1.1  | 0.4   | 7.0 | 8.4  | 0.0  | 97%       | +2.0         | +1.5        | +0.0       | 0.00      |
| 2A-5   | 2.79   | 2.75  | 1.1  | 0.9  | 0.0   | 2.5 | 7.7  | 0.0  | 91%       | +2.0         | +2.0        | +0.0       | 0.00      |
| 2A-9   | 3.22   | 2.65  | 1.0  | 0.9  | 0.0   | 2.4 | 7.6  | 0.0  | 96%       | +2.5         | +2.0        | +0.0       | 0.00      |
| 1A-3   | 3.20   | 2.62  | 2.0  | 1.3  | 0.0   | 1.5 | 7.1  | 0.0  | 41%       | -0.7         | -1.2        | +0.0       | 0.00      |
| 2B-14  | 3.89   | 2.58  | 3.0  | 0.4  | 1.2   | 3.2 | 7.6  | 0.0  | 81%       | +1.7         | +3.5        | +0.0       | 0.00      |
| 2B-7   | 2.85   | 2.54  | 0.7  | 1.0  | 0.0   | 1.1 | 7.9  | 0.0  | 100%      | -0.2         | -0.3        | +0.0       | 0.00      |
| 1A-6   | 3.72   | 2.47  | 1.0  | 2.1  | 0.1   | 1.5 | 7.4  | 0.0  | 96%       | +1.6         | +0.9        | +0.0       | 0.00      |
| 1A-9   | 1.84   | 2.44  | 2.6  | 3.7  | 2.1   | 2.5 | 3.9  | 0.0  | 40%       | +0.1         | +2.2        | +0.0       | 0.00      |
| 2B-8   | 2.97   | 2.34  | 0.6  | 1.0  | 0.1   | 1.4 | 6.9  | 0.0  | 100%      | -0.3         | -0.4        | +0.0       | 0.00      |
| 1B-4   | 2.54   | 2.11  | 1.6  | 0.6  | 0.9   | 7.2 | 7.4  | 0.0  | 88%       | -2.8         | -2.8        | +0.0       | 0.00      |
| 1B-1   | 2.70   | 2.08  | 1.1  | 0.6  | 0.0   | 2.4 | 7.3  | 0.0  | 100%      | +1.9         | +1.6        | +0.0       | 0.00      |
| 1D-3   | 3.00   | 2.01  | 0.7  | 1.3  | 0.9   | 1.3 | 4.9  | 0.0  | 100%      | +0.3         | -0.5        | +0.0       | 0.00      |
| 2B-10  | 2.03   | 1.99  | 2.8  | 0.6  | 2.6   | 2.4 | 6.6  | 0.0  | 23%       | -1.3         | -1.5        | +0.0       | 0.00      |
| 2B-6   | 3.19   | 1.99  | 1.0  | 0.7  | 0.3   | 1.9 | 6.4  | 0.0  | 100%      | +0.8         | +0.9        | +0.0       | 0.00      |
| 2B-1   | 2.87   | 1.97  | 0.8  | 0.6  | 0.0   | 2.3 | 8.1  | 0.0  | 100%      | +1.8         | +1.5        | +0.0       | 0.00      |
| 1C-3   | 2.40   | 1.96  | 3.2  | 0.3  | 0.0   | 2.4 | 6.2  | 0.0  | 37%       | -3.4         | -2.5        | +0.0       | 0.00      |
| 1A-7   | 1.53   | 1.91  | 3.4  | -    | 1.3   | 8.4 | 8.1  | 0.0  | 92%       | +5.4         | +8.3        | +0.0       | 0.00      |
| 2C-5   | 3.05   | 1.84  | 1.5  | 0.3  | 0.3   | 3.2 | 6.9  | 0.0  | 88%       | -1.4         | -1.1        | +0.0       | 0.00      |
| 2B-13  | 2.23   | 1.82  | 3.1  | 0.4  | 1.4   | 6.2 | 4.3  | 0.0  | 40%       | +1.5         | +3.3        | +0.0       | 0.00      |
| 2C-1   | 1.57   | 1.77  | 1.2  | 0.7  | 0.1   | 3.7 | 7.6  | 0.0  | 62%       | -1.9         | -2.4        | +0.0       | 0.00      |
| 1R-1   | 2.98   | 1.74  | 1.2  | 0.5  | 0.0   | 3.2 | 7.4  | 0.0  | 91%       | +3.0         | +2.9        | +0.0       | 0.00      |
| 1C-1   | 1.68   | 1.74  | 1.2  | 0.4  | 1.6   | 4.8 | 7.1  | 0.0  | 90%       | -1.8         | -2.2        | +0.0       | 0.00      |
| 2C-6   | 2.94   | 1.69  | 2.2  | 0.2  | 0.2   | 2.8 | 4.5  | 0.0  | 74%       | -2.3         | -1.7        | +0.0       | 0.00      |
| 2A-10  | 1.35   | 1.64  | 2.6  | -    | 1.4   | 9.3 | 5.3  | 0.0  | 94%       | +4.9         | +7.7        | +0.0       | 0.00      |
| 2B-5   | 3.27   | 1.64  | 1.3  | 0.4  | 1.0   | 1.9 | 6.6  | 0.0  | 99%       | +0.7         | +0.7        | +0.0       | 0.00      |
| 2A-13  | 2.70   | 1.61  | 1.0  | 0.4  | 0.1   | 2.0 | 6.9  | 0.0  | 99%       | +1.7         | +2.0        | +0.0       | 0.00      |
| 1A-8   | 2.92   | 1.60  | 1.8  | 0.4  | 1.0   | 3.3 | 5.1  | 0.0  | 93%       | +2.5         | +3.5        | +0.0       | 0.00      |
| 1A-2   | 1.69   | 1.56  | 1.3  | 0.9  | 0.0   | 1.2 | 8.5  | 0.0  | 61%       | -0.1         | +0.5        | +0.0       | 0.00      |
| 2C-4   | 2.45   | 1.55  | 1.3  | 0.3  | 0.0   | 4.0 | 6.8  | 0.0  | 89%       | -1.1         | -1.1        | +0.0       | 0.00      |
| 2A-12  | 2.55   | 1.55  | 1.5  | 0.4  | 0.2   | 3.1 | 6.9  | 0.0  | 94%       | +2.8         | +3.1        | +0.0       | 0.00      |
| 2B-4   | 3.14   | 1.54  | 1.8  | 0.4  | 1.4   | 2.1 | 5.9  | 0.0  | 59%       | +1.0         | +0.9        | +0.0       | 0.00      |
| 2C-2   | 1.80   | 1.54  | 1.2  | 0.3  | 1.5   | 5.2 | 5.7  | 0.0  | 92%       | -1.5         | -1.8        | +0.0       | 0.00      |
| 2B-2   | 2.46   | 1.52  | 0.7  | 0.2  | 0.2   | 2.2 | 6.1  | 0.0  | 100%      | +1.5         | +1.2        | +0.0       | 0.00      |
| 2R-1   | 1.98   | 1.52  | 1.4  | 0.5  | 0.0   | 3.2 | 7.1  | 0.0  | 92%       | +2.9         | +2.8        | +0.0       | 0.00      |
| 1B-2   | 2.72   | 1.50  | 0.7  | 0.3  | 0.0   | 2.3 | 7.4  | 0.0  | 100%      | +1.6         | +1.3        | +0.0       | 0.00      |
| 2A-16  | 2.27   | 1.46  | 1.9  | 0.4  | 0.6   | 3.2 | 6.1  | 0.0  | 94%       | +1.7         | +3.2        | +0.0       | 0.00      |
| 1A-5   | 2.58   | 1.39  | 1.2  | 0.5  | 0.0   | 4.4 | 6.0  | 0.0  | 91%       | +3.0         | +2.9        | +0.0       | 0.00      |
| 2B-11  | 2.08   | 1.38  | 2.3  | 0.3  | 0.8   | 3.0 | 6.2  | 0.0  | 69%       | +1.4         | +3.1        | +0.0       | 0.00      |
| 2C-3   | 1.82   | 1.35  | 1.3  | 0.3  | 0.0   | 4.7 | 5.4  | 0.0  | 91%       | -1.3         | -1.5        | +0.0       | 0.00      |
| 1B-3   | 2.42   | 1.35  | 1.0  | 0.3  | 0.0   | 1.9 | 6.1  | 0.0  | 99%       | +0.2         | +0.3        | +0.0       | 0.00      |
| 1A-4   | 1.08   | 1.33  | 2.0  | -    | 0.0   | 5.8 | 6.8  | 0.0  | 57%       | +3.2         | +5.8        | +0.0       | 0.00      |
| 2A-15  | 1.35   | 1.32  | 2.2  | -    | 1.4   | 7.3 | 4.1  | 0.0  | 92%       | +3.6         | +4.6        | +0.0       | 0.00      |
| 2B-9   | 1.92   | 1.31  | 0.7  | 0.7  | 0.1   | 1.3 | 6.5  | 0.0  | 100%      | -0.4         | -0.7        | +0.0       | 0.00      |
| 1C-2   | 2.28   | 1.28  | 1.2  | 0.3  | 1.4   | 4.8 | 3.7  | 0.0  | 94%       | -1.0         | -1.1        | +0.0       | 0.00      |
| 2A-1   | 1.36   | 1.28  | 1.2  | -    | 0.0   | 7.2 | 6.3  | 0.0  | 75%       | -0.7         | +0.9        | +0.0       | 0.00      |
| 2B-3   | 2.81   | 1.27  | 1.1  | 0.3  | 1.5   | 2.7 | 3.5  | 0.0  | 100%      | +1.1         | +1.0        | +0.0       | 0.00      |
| 2B-12  | 2.00   | 1.22  | 2.3  | 0.4  | 1.8   | 3.0 | 4.0  | 0.0  | 63%       | +1.4         | +3.2        | +0.0       | 0.00      |
| 2A-11  | 1.15   | 1.17  | 1.8  | -    | 0.2   | 4.2 | 6.6  | 0.0  | 93%       | +3.5         | +4.1        | +0.0       | 0.00      |
| 1E-2   | 0.88   | 1.15  | 2.0  | -    | 0.0   | 4.3 | 6.2  | 0.0  | 56%       | +3.0         | +5.7        | +0.0       | 0.00      |
| 2A-8   | 1.14   | 1.14  | 1.7  | -    | 0.0   | 3.6 | 7.4  | 0.0  | 72%       | +3.2         | +3.8        | +0.0       | 0.00      |
| 1D-2   | 1.02   | 1.09  | 1.7  | -    | 0.0   | 4.5 | 5.6  | 0.0  | 67%       | +1.0         | +3.5        | +0.0       | 0.00      |
| 2A-4   | 1.12   | 1.09  | 1.4  | -    | 0.0   | 3.0 | 7.7  | 0.0  | 69%       | +1.5         | +2.6        | +0.0       | 0.00      |
| 1D-1   | 1.20   | 1.08  | 1.9  | -    | 0.0   | 2.9 | 7.0  | 0.0  | 41%       | +0.8         | +3.6        | +0.0       | 0.00      |
| 2A-3   | 0.98   | 1.05  | 1.3  | -    | 0.0   | 3.3 | 7.2  | 0.0  | 62%       | +0.5         | +3.3        | +0.0       | 0.00      |
| 1E-1   | 0.99   | 1.02  | 2.9  | -    | 0.6   | 2.8 | 3.7  | 0.0  | 45%       | -0.2         | +2.7        | +0.0       | 0.00      |
| 1A-1   | 1.16   | 0.99  | 1.5  | -    | 0.0   | 3.0 | 6.4  | 0.0  | 52%       | -2.3         | +1.0        | +0.0       | 0.00      |

### Resonant peak (Resonance=max samples)

| sample | peak Hz (hw) | peak Hz (before) | peak Hz (after) | height dB (hw) | height dB (before) | height dB (after) |
|--------|--------------|------------------|-----------------|----------------|--------------------|-------------------|
| 1A-2   | 131          | 65               | 65              | -1.5           | +0.0               | +0.0              |
| 1A-3   | 196          | 196              | 196             | -0.1           | +0.1               | -0.8              |
| 1A-5   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1A-6   | 327          | 458              | 392             | -1.9           | -0.8               | -2.8              |
| 1A-8   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1A-9   | 1701         | 1766             | 1766            | -3.9           | -6.1               | -7.6              |
| 1B-1   | 65           | 392              | 65              | +0.0           | -4.6               | +0.0              |
| 1B-2   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1B-3   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1B-4   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1B-5   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1C-1   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1C-2   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1C-3   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1D-3   | 589          | 262              | 654             | -10.3          | -2.1               | -12.1             |
| 1D-4   | 981          | 15436            | 1046            | -6.4           | -138.3             | -9.5              |
| 1R-1   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 2A-2   | 131          | 131              | 196             | +0.5           | -1.1               | -3.5              |
| 2A-5   | 65           | 14128            | 65              | +0.0           | -141.9             | +0.0              |
| 2A-6   | 196          | 196              | 65              | -2.3           | -1.8               | +0.0              |
| 2A-7   | 196          | 262              | 262             | +1.3           | +0.4               | -1.1              |
| 2A-9   | 65           | 392              | 392             | +0.0           | -4.5               | -6.6              |
| 2A-12  | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 2A-13  | 65           | 720              | 720             | +0.0           | -7.7               | -10.1             |
| 2A-14  | 1439         | 850              | 785             | -18.3          | -2.6               | -3.7              |
| 2A-16  | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 2B-1   | 65           | 392              | 65              | +0.0           | -4.8               | +0.0              |
| 2B-2   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 2B-3   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 2B-4   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 2B-5   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 2B-6   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 2B-7   | 131          | 196              | 65              | -1.0           | -2.7               | +0.0              |
| 2B-8   | 131          | 196              | 65              | -0.9           | -2.9               | +0.0              |
| 2B-9   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 2B-10  | 65           | 15632            | 65              | +0.0           | -137.4             | +0.0              |
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

- **1A-1** worst: k21 1374Hz ref -72.7 / sim -78.1, k20 1308Hz ref -71.0 / sim -76.2, k22 1439Hz ref -74.4 / sim -79.5, k18 1177Hz ref -67.5 / sim -72.5. Model excess above 4 kHz: none > 3 dB.
- **1A-2** worst: k25 1635Hz ref -74.7 / sim -79.5, k23 1504Hz ref -71.7 / sim -76.2, k22 1439Hz ref -70.1 / sim -74.3, k24 1570Hz ref -73.3 / sim -77.9. Model excess above 4 kHz: none > 3 dB.
- **1A-3** worst: k23 1504Hz ref -65.8 / sim -72.6, k26 1701Hz ref -70.5 / sim -77.5, k14 916Hz ref -47.4 / sim -52.9, k20 1308Hz ref -60.5 / sim -67.0. Model excess above 4 kHz: none > 3 dB.
- **1A-4** worst: k1 65Hz ref 0.0 / sim 6.7, k2 131Hz ref -4.7 / sim -0.2, k36 2355Hz ref -74.1 / sim -78.4, k37 2420Hz ref -75.2 / sim -79.4. Model excess above 4 kHz: none > 3 dB.
- **1A-5** worst: k6 392Hz ref -12.3 / sim -8.9, k5 327Hz ref -8.9 / sim -5.6, k2 131Hz ref -4.0 / sim -0.8, k1 65Hz ref 0.0 / sim 3.1. Model excess above 4 kHz: none > 3 dB.
- **1A-6** worst: k7 458Hz ref -7.6 / sim -3.4, k5 327Hz ref -1.9 / sim -5.0, k6 392Hz ref -4.6 / sim -1.8, k32 2093Hz ref -61.0 / sim -63.4. Model excess above 4 kHz: none > 3 dB.
- **1A-7** worst: k2 131Hz ref -4.9 / sim 5.2, k3 196Hz ref -8.4 / sim 1.3, k4 262Hz ref -10.7 / sim -2.3, k1 65Hz ref 0.0 / sim 8.0. Model excess above 4 kHz: none > 3 dB.
- **1A-8** worst: k3 196Hz ref -8.2 / sim -3.8, k4 262Hz ref -10.4 / sim -6.0, k2 131Hz ref -4.9 / sim -0.6, k5 327Hz ref -11.9 / sim -7.8. Model excess above 4 kHz: none > 3 dB.
- **1A-9** worst: k87 5690Hz ref -62.2 / sim -57.4, k88 5756Hz ref -62.4 / sim -57.7, k99 6475Hz ref -67.0 / sim -62.5, k112 7326Hz ref -72.5 / sim -68.1. Model excess above 4 kHz: harmonic 5690Hz +4.8dB, harmonic 5756Hz +4.7dB, harmonic 6475Hz +4.5dB, inter-harmonic 4088Hz +3.5dB.
- **1B-1** worst: k6 392Hz ref -8.2 / sim -5.5, k48 3140Hz ref -78.0 / sim -75.4, k49 3205Hz ref -78.8 / sim -76.3, k47 3074Hz ref -77.1 / sim -74.6. Model excess above 4 kHz: none > 3 dB.
- **1B-2** worst: k5 327Hz ref -8.1 / sim -6.0, k2 131Hz ref -4.3 / sim -2.6, k6 392Hz ref -9.5 / sim -7.9, k3 196Hz ref -6.1 / sim -4.6. Model excess above 4 kHz: none > 3 dB.
- **1B-3** worst: k24 1570Hz ref -21.0 / sim -24.1, k23 1504Hz ref -20.8 / sim -23.7, k30 1962Hz ref -23.3 / sim -26.5, k31 2028Hz ref -24.1 / sim -26.9. Model excess above 4 kHz: none > 3 dB.
- **1B-4** worst: k15 981Hz ref -18.4 / sim -23.5, k14 916Hz ref -18.1 / sim -22.8, k13 850Hz ref -17.6 / sim -22.2, k12 785Hz ref -16.9 / sim -21.4. Model excess above 4 kHz: none > 3 dB.
- **1B-5** worst: k10 654Hz ref -14.9 / sim -9.7, k9 589Hz ref -14.8 / sim -9.8, k8 523Hz ref -14.4 / sim -9.8, k4 262Hz ref -10.4 / sim -5.9. Model excess above 4 kHz: harmonic 4840Hz +3.4dB, harmonic 4775Hz +3.4dB, harmonic 4906Hz +3.3dB.
- **1C-1** worst: k7 458Hz ref -17.4 / sim -21.2, k6 392Hz ref -15.9 / sim -18.6, k2 131Hz ref -6.0 / sim -8.5, k5 327Hz ref -14.8 / sim -17.2. Model excess above 4 kHz: none > 3 dB.
- **1C-2** worst: k16 1046Hz ref -18.8 / sim -22.4, k17 1112Hz ref -19.4 / sim -23.0, k18 1177Hz ref -20.2 / sim -23.6, k15 981Hz ref -18.5 / sim -21.9. Model excess above 4 kHz: none > 3 dB.
- **1C-3** worst: k38 2485Hz ref -21.7 / sim -28.5, k39 2551Hz ref -22.3 / sim -29.0, k37 2420Hz ref -21.4 / sim -28.0, k40 2616Hz ref -23.0 / sim -29.6. Model excess above 4 kHz: none > 3 dB.
- **1D-1** worst: k1 65Hz ref 0.0 / sim 4.1, k36 2355Hz ref -75.1 / sim -79.1, k35 2289Hz ref -74.0 / sim -78.0, k37 2420Hz ref -76.0 / sim -80.1. Model excess above 4 kHz: none > 3 dB.
- **1D-2** worst: k9 589Hz ref -19.4 / sim -25.5, k8 523Hz ref -18.2 / sim -23.5, k10 654Hz ref -20.6 / sim -27.3, k11 720Hz ref -21.7 / sim -28.9. Model excess above 4 kHz: none > 3 dB.
- **1D-3** worst: k33 2158Hz ref -61.5 / sim -63.9, k32 2093Hz ref -60.2 / sim -62.5, k12 785Hz ref -19.9 / sim -17.7, k13 850Hz ref -25.3 / sim -23.1. Model excess above 4 kHz: none > 3 dB.
- **1D-4** worst: k17 1112Hz ref -16.3 / sim -12.9, k50 3270Hz ref -64.3 / sim -67.4, k35 2289Hz ref -48.6 / sim -45.6, k36 2355Hz ref -49.8 / sim -46.8. Model excess above 4 kHz: none > 3 dB.
- **1E-1** worst: k13 850Hz ref -42.6 / sim -57.1, k15 981Hz ref -47.8 / sim -68.2, k11 720Hz ref -37.1 / sim -47.1, k28 1831Hz ref -69.8 / sim -79.6. Model excess above 4 kHz: none > 3 dB.
- **1E-2** worst: k1 65Hz ref 0.0 / sim 6.6, k36 2355Hz ref -74.6 / sim -79.1, k37 2420Hz ref -75.6 / sim -80.2, k35 2289Hz ref -73.6 / sim -78.0. Model excess above 4 kHz: none > 3 dB.
- **1R-1** worst: k5 327Hz ref -8.9 / sim -5.6, k6 392Hz ref -12.2 / sim -8.9, k2 131Hz ref -4.0 / sim -0.9, k1 65Hz ref 0.0 / sim 3.1. Model excess above 4 kHz: none > 3 dB.
- **2A-1** worst: k21 1374Hz ref -70.2 / sim -75.3, k20 1308Hz ref -68.5 / sim -73.4, k22 1439Hz ref -72.1 / sim -77.0, k19 1243Hz ref -66.9 / sim -71.4. Model excess above 4 kHz: none > 3 dB.
- **2A-2** worst: k5 327Hz ref -18.7 / sim -15.3, k4 262Hz ref -11.3 / sim -9.2, k2 131Hz ref 0.0 / sim -1.9, k3 196Hz ref -4.8 / sim -3.1. Model excess above 4 kHz: none > 3 dB.
- **2A-3** worst: k24 1570Hz ref -72.3 / sim -76.1, k25 1635Hz ref -73.7 / sim -77.5, k26 1701Hz ref -75.1 / sim -78.9, k1 65Hz ref 0.0 / sim 3.7. Model excess above 4 kHz: none > 3 dB.
- **2A-4** worst: k29 1897Hz ref -75.4 / sim -80.0, k27 1766Hz ref -72.9 / sim -77.3, k28 1831Hz ref -74.4 / sim -78.7, k26 1701Hz ref -71.6 / sim -75.8. Model excess above 4 kHz: none > 3 dB.
- **2A-5** worst: k31 2028Hz ref -76.6 / sim -79.9, k4 262Hz ref -10.8 / sim -7.6, k30 1962Hz ref -75.5 / sim -78.6, k29 1897Hz ref -74.3 / sim -77.2. Model excess above 4 kHz: none > 3 dB.
- **2A-6** worst: k4 262Hz ref -7.5 / sim -3.7, k5 327Hz ref -11.4 / sim -9.2, k6 392Hz ref -16.2 / sim -14.2, k30 1962Hz ref -78.6 / sim -76.6. Model excess above 4 kHz: none > 3 dB.
- **2A-7** worst: k3 196Hz ref 0.0 / sim -3.9, k20 1308Hz ref -56.7 / sim -60.4, k21 1374Hz ref -58.7 / sim -62.3, k31 2028Hz ref -74.3 / sim -77.9. Model excess above 4 kHz: none > 3 dB.
- **2A-8** worst: k1 65Hz ref 0.0 / sim 4.2, k2 131Hz ref -3.9 / sim 0.1, k41 2682Hz ref -75.6 / sim -79.4, k40 2616Hz ref -74.7 / sim -78.4. Model excess above 4 kHz: none > 3 dB.
- **2A-9** worst: k6 392Hz ref -8.9 / sim -4.5, k7 458Hz ref -11.7 / sim -8.5, k2 131Hz ref -4.1 / sim -1.9, k1 65Hz ref 0.0 / sim 2.1. Model excess above 4 kHz: none > 3 dB.
- **2A-10** worst: k1 65Hz ref 0.0 / sim 8.3, k2 131Hz ref -4.9 / sim 3.3, k3 196Hz ref -8.3 / sim -2.0, k4 262Hz ref -10.7 / sim -6.6. Model excess above 4 kHz: none > 3 dB.
- **2A-11** worst: k2 131Hz ref -4.7 / sim 0.1, k3 196Hz ref -7.6 / sim -2.9, k1 65Hz ref 0.0 / sim 4.3, k4 262Hz ref -9.3 / sim -5.1. Model excess above 4 kHz: none > 3 dB.
- **2A-12** worst: k3 196Hz ref -7.6 / sim -3.9, k2 131Hz ref -4.6 / sim -1.0, k4 262Hz ref -9.2 / sim -5.8, k1 65Hz ref 0.0 / sim 3.3. Model excess above 4 kHz: none > 3 dB.
- **2A-13** worst: k11 720Hz ref -11.1 / sim -7.9, k3 196Hz ref -7.8 / sim -5.1, k12 785Hz ref -12.8 / sim -10.3, k2 131Hz ref -4.7 / sim -2.2. Model excess above 4 kHz: none > 3 dB.
- **2A-14** worst: k12 785Hz ref -7.8 / sim -2.1, k13 850Hz ref -9.5 / sim -6.2, k10 654Hz ref -4.9 / sim -8.0, k73 4775Hz ref -78.9 / sim -76.5. Model excess above 4 kHz: none > 3 dB.
- **2A-15** worst: k3 196Hz ref -8.2 / sim -2.8, k4 262Hz ref -10.4 / sim -5.0, k2 131Hz ref -4.9 / sim 0.3, k5 327Hz ref -11.9 / sim -6.8. Model excess above 4 kHz: none > 3 dB.
- **2A-16** worst: k3 196Hz ref -8.2 / sim -4.0, k4 262Hz ref -10.4 / sim -6.3, k2 131Hz ref -4.9 / sim -0.9, k5 327Hz ref -12.0 / sim -8.1. Model excess above 4 kHz: none > 3 dB.
- **2B-1** worst: k6 392Hz ref -8.6 / sim -6.0, k7 458Hz ref -9.9 / sim -7.8, k2 131Hz ref -4.3 / sim -2.5, k50 3270Hz ref -76.7 / sim -75.2. Model excess above 4 kHz: none > 3 dB.
- **2B-2** worst: k2 131Hz ref -4.2 / sim -2.6, k5 327Hz ref -8.6 / sim -7.0, k14 916Hz ref -15.8 / sim -17.4, k15 981Hz ref -16.8 / sim -18.3. Model excess above 4 kHz: none > 3 dB.
- **2B-3** worst: k86 5625Hz ref -79.3 / sim -77.2, k87 5690Hz ref -79.7 / sim -77.7, k85 5560Hz ref -78.8 / sim -76.8, k29 1897Hz ref -34.8 / sim -32.8. Model excess above 4 kHz: none > 3 dB.
- **2B-4** worst: k32 2093Hz ref -37.2 / sim -32.6, k31 2028Hz ref -35.6 / sim -31.0, k30 1962Hz ref -34.0 / sim -29.6, k33 2158Hz ref -38.7 / sim -34.3. Model excess above 4 kHz: harmonic 4251Hz +3.1dB, harmonic 5102Hz +3.1dB, harmonic 4317Hz +3.1dB.
- **2B-5** worst: k37 2420Hz ref -37.2 / sim -34.2, k36 2355Hz ref -35.8 / sim -32.8, k21 1374Hz ref -19.2 / sim -22.1, k38 2485Hz ref -38.5 / sim -35.7. Model excess above 4 kHz: none > 3 dB.
- **2B-6** worst: k23 1504Hz ref -19.9 / sim -22.7, k26 1701Hz ref -21.4 / sim -24.0, k20 1308Hz ref -18.8 / sim -21.3, k24 1570Hz ref -20.6 / sim -23.1. Model excess above 4 kHz: none > 3 dB.
- **2B-7** worst: k14 916Hz ref -47.2 / sim -49.7, k15 981Hz ref -49.9 / sim -52.4, k13 850Hz ref -44.5 / sim -46.8, k16 1046Hz ref -52.6 / sim -54.8. Model excess above 4 kHz: none > 3 dB.
- **2B-8** worst: k15 981Hz ref -48.0 / sim -50.1, k16 1046Hz ref -50.5 / sim -52.7, k14 916Hz ref -45.4 / sim -47.5, k17 1112Hz ref -53.0 / sim -55.0. Model excess above 4 kHz: none > 3 dB.
- **2B-9** worst: k10 654Hz ref -27.6 / sim -24.9, k9 589Hz ref -22.8 / sim -20.4, k11 720Hz ref -31.2 / sim -29.3, k37 2420Hz ref -77.8 / sim -76.4. Model excess above 4 kHz: none > 3 dB.
- **2B-10** worst: k19 1243Hz ref -37.0 / sim -29.2, k20 1308Hz ref -38.9 / sim -31.3, k18 1177Hz ref -34.7 / sim -27.2, k17 1112Hz ref -32.5 / sim -25.5. Model excess above 4 kHz: none > 3 dB.
- **2B-11** worst: k3 196Hz ref -8.2 / sim -4.1, k4 262Hz ref -10.4 / sim -6.4, k2 131Hz ref -4.9 / sim -1.0, k5 327Hz ref -12.0 / sim -8.2. Model excess above 4 kHz: harmonic 6214Hz +3.2dB, harmonic 6017Hz +3.2dB, harmonic 6148Hz +3.2dB.
- **2B-12** worst: k3 196Hz ref -8.2 / sim -4.1, k4 262Hz ref -10.5 / sim -6.4, k2 131Hz ref -4.9 / sim -1.0, k5 327Hz ref -12.0 / sim -8.2. Model excess above 4 kHz: harmonic 6475Hz +3.3dB, harmonic 6868Hz +3.3dB, harmonic 6606Hz +3.3dB.
- **2B-13** worst: k127 8307Hz ref -69.0 / sim -63.8, k126 8241Hz ref -68.6 / sim -63.5, k125 8176Hz ref -68.2 / sim -63.2, k124 8110Hz ref -67.9 / sim -62.8. Model excess above 4 kHz: harmonic 8372Hz +5.2dB, harmonic 8307Hz +5.1dB, harmonic 8437Hz +5.1dB, inter-harmonic 4480Hz +4.7dB.
- **2B-14** worst: k13 850Hz ref -16.0 / sim -11.4, k3 196Hz ref -8.2 / sim -3.8, k4 262Hz ref -10.4 / sim -6.1, k14 916Hz ref -15.8 / sim -11.5. Model excess above 4 kHz: harmonic 7652Hz +5.3dB, harmonic 7718Hz +5.3dB, harmonic 7587Hz +5.3dB.
- **2C-1** worst: k9 589Hz ref -17.3 / sim -21.8, k7 458Hz ref -15.6 / sim -19.5, k8 523Hz ref -16.7 / sim -20.3, k10 654Hz ref -18.2 / sim -23.9. Model excess above 4 kHz: none > 3 dB.
- **2C-2** worst: k11 720Hz ref -17.8 / sim -22.1, k12 785Hz ref -18.8 / sim -23.0, k9 589Hz ref -16.6 / sim -19.9, k10 654Hz ref -17.5 / sim -20.7. Model excess above 4 kHz: none > 3 dB.
- **2C-3** worst: k14 916Hz ref -18.6 / sim -22.5, k13 850Hz ref -17.9 / sim -21.7, k11 720Hz ref -16.8 / sim -20.2, k15 981Hz ref -19.4 / sim -22.9. Model excess above 4 kHz: none > 3 dB.
- **2C-4** worst: k20 1308Hz ref -19.6 / sim -23.6, k21 1374Hz ref -19.9 / sim -24.0, k19 1243Hz ref -19.5 / sim -23.1, k18 1177Hz ref -19.0 / sim -22.6. Model excess above 4 kHz: none > 3 dB.
- **2C-5** worst: k28 1831Hz ref -21.3 / sim -25.5, k27 1766Hz ref -21.0 / sim -25.2, k25 1635Hz ref -20.2 / sim -24.4, k26 1701Hz ref -20.6 / sim -24.8. Model excess above 4 kHz: none > 3 dB.
- **2C-6** worst: k33 2158Hz ref -21.5 / sim -26.6, k34 2224Hz ref -21.8 / sim -26.9, k32 2093Hz ref -21.2 / sim -26.2, k31 2028Hz ref -20.9 / sim -25.9. Model excess above 4 kHz: none > 3 dB.
- **2R-1** worst: k5 327Hz ref -8.9 / sim -5.6, k6 392Hz ref -12.1 / sim -8.9, k2 131Hz ref -3.9 / sim -0.9, k1 65Hz ref 0.0 / sim 3.0. Model excess above 4 kHz: none > 3 dB.

## Suspicious samples

- **2C-4**: spectrum is within 1.2 dB of 2C-5 although the decay label differs -- one of the two labels is probably wrong
- **2B-11**: spectrum is within 1.2 dB of 2B-12 although the envMod label differs -- one of the two labels is probably wrong
- **2B-3**: spectrum is within 1.5 dB of 2B-4 although the envMod label differs -- one of the two labels is probably wrong
- **2A-6**: spectrum is within 1.0 dB of 2B-7 although the cutoff/decay label differs -- one of the two labels is probably wrong
- **1D-1**: spectrum is within 1.0 dB of 1E-2 although the cutoff/envMod/decay/accent label differs -- one of the two labels is probably wrong
- **1A-4**: spectrum is within 1.2 dB of 1D-1 although the cutoff/envMod/decay/accent label differs -- one of the two labels is probably wrong
- **1A-3**: spectrum is within 1.4 dB of 2B-7 although the resonance/decay label differs -- one of the two labels is probably wrong
- **1D-4**: error 3.46 is 2.2x the median
- **2A-14**: error 3.09 is 1.9x the median
- **2A-7**: error 3.02 is 1.9x the median
- **2A-2**: error 2.89 is 1.8x the median
- **1B-5**: error 2.85 is 1.8x the median
- **2A-6**: error 2.83 is 1.8x the median
- **2A-5**: error 2.75 is 1.7x the median
- **2A-9**: error 2.65 is 1.7x the median
- **1A-3**: error 2.62 is 1.6x the median
- **2B-14**: error 2.58 is 1.6x the median
- **2B-7**: error 2.54 is 1.6x the median
- **1A-6**: error 2.47 is 1.5x the median
- **1A-9**: error 2.44 is 1.5x the median

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

Cutoff law with the fitted end stops folded in (so the plugin's knob range equals the hardware's): `cutoffBaseHz = 264.1`, `cutoffSpanOct = 3.156`, `cutoffTaperExp = 1.611`.

## Fitted model parameters

`sensitivity` = objective increase for a 5% (of range) nudge. Near-zero means the current references don't constrain that parameter -- its value is not evidence of anything. `AT BOUND` means the optimizer pushed it to the edge of its plausible range, usually a sign that it is compensating for something the model does not implement.

| parameter              | before    | after    | sensitivity |                                     |
|------------------------|-----------|----------|-------------|-------------------------------------|
| filterFeedbackGain     | 16.393    | 16.803   | 0.000       |                                     |
| filterResonanceLimit   | 0.91309   | 0.92752  | 0.000       |                                     |
| filterResonanceSkew    | -0.8718   | -0.3083  | 0.000       |                                     |
| resCouplingHz          | 104.29    | 90.923   | 0.000       |                                     |
| filterCapScale1        | 1.2895    | 1.2101   | 0.000       |                                     |
| filterCapScale2        | 0.69764   | 0.68285  | 0.000       |                                     |
| filterCapScale3        | 0.91275   | 1.0858   | 0.000       |                                     |
| filterCapScale4        | 1.0631    | 0.95616  | 0.000       |                                     |
| filterLadderInputScale | 0.035224  | 0.027187 | 0.000       |                                     |
| filterInputCouplingHz  | 6.0165    | 4.1589   | 0.000       |                                     |
| filterOutputCouplingHz | 20000     | 16805    | 0.000       |                                     |
| filterPostHpHz         | 198.89    | 93.656   | 0.000       |                                     |
| filterNotchHz          | 7.5164    | 12.09    | 0.000       |                                     |
| filterNotchBandwidthHz | 4.7       | 5.3346   | 0.000       |                                     |
| filterAllpassHz        | 14.008    | 12.928   | 0.000       |                                     |
| cutoffBaseHz           | 222.01    | 264.11   | 0.000       |                                     |
| cutoffSpanOct          | 3.3414    | 3.1565   | 0.000       |                                     |
| cutoffTaperExp         | 1.4289    | 1.6107   | 0.000       |                                     |
| cutoffMaxHz            | 23723     | 25267    | 0.000       |                                     |
| envModScaleC0          | 0.76145   | 0.9261   | 0.000       |                                     |
| envModScaleC0Slope     | 5.1991    | 4.9925   | 0.000       |                                     |
| envModScaleC1          | 0.77794   | 0.94252  | 0.000       |                                     |
| envModScaleC1Slope     | 4.069     | 3.3817   | 0.000       |                                     |
| envModOffset           | 0.34816   | 0.38205  | 0.000       |                                     |
| envModOffsetCutSlope   | -0.023862 | 0.011168 | 0.000       |                                     |
| envModTaperExp         | 2         | 2.1102   | 0.000       |                                     |
| envModTaperMid         | 0.68612   | 0.72462  | 0.000       |                                     |
| envModTaperWidth       | 0.12193   | 0.1797   | 0.000       |                                     |
| accentSweepDepthOct    | 7.5466    | 5.6056   | 0.000       |                                     |
| accentVcaDepth         | 2.3468    | 2.3943   | 0.000       |                                     |
| accentChargeBaseSec    | 0.030579  | 0.025097 | 0.000       |                                     |
| accentChargePotSec     | 0.044021  | 0.03167  | 0.000       |                                     |
| accentMixSec           | 0.14498   | 0.10966  | 0.000       |                                     |
| accentDiodeDrop        | 0.30033   | 0.32059  | 0.000       |                                     |
| vcfDecayMinSec         | 0.061906  | 0.063658 | 0.000       |                                     |
| vcfDecayMaxSec         | 1.0862    | 1.0691   | 0.000       |                                     |
| vcfDecayTaper          | 17.982    | 20.334   | 0.000       |                                     |
| vegDecaySec            | 1         | 1.009    | 0.000       | AT BOUND (model may lack structure) |
| vcaResTapRatio         | 1.7357    | 1.6824   | 0.000       |                                     |
| vcaCutoffLevelDb       | -2.5749   | -1.9418  | 0.000       |                                     |
| vcaCutoffLevelResDb    | -0.25653  | 1.2048   | 0.000       |                                     |

Timing: note-on offset -0.17 ms (relative to each clip's measured note-on), gate length 1309.9 ms.

## Applying the result

Paste `synth_parameters.txt` into `acidus::SynthParameters` (src/core/SynthEngine.hpp) or rerun with `--apply`.

Audio: `renders/` (hardware / before / after for the 40 worst samples).
