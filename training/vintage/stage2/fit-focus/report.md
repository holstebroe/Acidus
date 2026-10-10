# Acidus vs. hardware TB-303 calibration report (20261010-135212)

61 reference samples, 43 free parameters. Search: 2945 evaluations in 42.3 min, 3 CMA-ES restarts, stopped because: time cap reached.

## Match quality (mean over samples)

All errors are dB; lower is better. `harm` = full-note harmonic levels (k^-0.5 weighted RMS), `inter` = energy between harmonics, `env` = RMS envelope, `stft` = 1/3-octave levels over time, `peak` = shape (height+width+implicit frequency) of the resonant-peak window on Resonance=max samples only -- weighted more heavily than `harm` so it can't be traded away.

| metric                                         | before | after |
|------------------------------------------------|--------|-------|
| objective (incl. priors)                       | 2.30   | 1.94  |
| weighted error                                 | 2.29   | 1.93  |
| harmonic error (dB)                            | 1.89   | 1.87  |
| resonant-peak shape error (dB)                 | 2.00   | 1.18  |
| inter-harmonic error (dB)                      | 0.67   | 0.73  |
| envelope error (dB)                            | 3.50   | 3.56  |
| spectrogram error (dB)                         | 7.34   | 6.60  |
| aligned waveform error (10*sqrt(residual/ref)) | 0.00   | 0.00  |
| resonant-peak sweep track error (semitones)    | 2.46   | 2.01  |
| harmonics-over-time error (dB)                 | 5.86   | 4.87  |
| mean |harmonic error| (dB)                     | 2.26   | 2.34  |
| note level error, RMS over notes (dB)          | 2.03   | 2.42  |
| harmonics within 1 dB (%)                      | 30.21  | 23.21 |
| harmonics within 3 dB (%)                      | 74.22  | 68.99 |
| harmonics within 6 dB (%)                      | 94.92  | 98.22 |

Recording gain solved as -11.1 dB (before: -10.3 dB).

## Per set

Mean error of each set's notes, and the RMS of their level errors (the recordings are not normalised, so absolute note levels are compared under one global gain).

| set | notes | before | after | level before (dB) | level after (dB) | harm | env | stft |
|-----|-------|--------|-------|-------------------|------------------|------|-----|------|
| 1A  | 9     | 2.19   | 2.01  | 2.6               | 3.5              | 2.1  | 3.8 | 6.7  |
| 1B  | 5     | 2.75   | 2.00  | 1.9               | 2.2              | 1.6  | 4.1 | 6.9  |
| 1C  | 3     | 2.12   | 1.78  | 2.3               | 2.1              | 2.0  | 3.8 | 5.6  |
| 1D  | 4     | 2.15   | 2.18  | 0.7               | 1.5              | 1.7  | 2.9 | 6.8  |
| 1E  | 2     | 0.93   | 1.02  | 2.2               | 2.9              | 2.6  | 2.5 | 4.8  |
| 1R  | 1     | 2.98   | 1.94  | 3.0               | 2.2              | 1.5  | 3.1 | 7.4  |
| 2A  | 16    | 2.08   | 1.91  | 2.4               | 2.6              | 1.6  | 4.0 | 6.8  |
| 2B  | 14    | 2.69   | 2.01  | 1.2               | 1.8              | 2.2  | 3.0 | 6.6  |
| 2C  | 6     | 2.27   | 1.88  | 1.6               | 1.6              | 1.6  | 3.8 | 6.6  |
| 2R  | 1     | 1.98   | 1.49  | 2.9               | 2.2              | 1.8  | 3.0 | 7.0  |

## Per sample (sorted by remaining error, worst first)

| sample | before | after | harm | peak | inter | env | stft | wave | harm ±3dB | level before | level after | tuning (c) | drift (c) |
|--------|--------|-------|------|------|-------|-----|------|------|-----------|--------------|-------------|------------|-----------|
| 2A-14  | 3.19   | 3.88  | 1.3  | 7.8  | 0.7   | 2.4 | 5.8  | 0.0  | 96%       | +1.0         | +1.8        | +0.0       | 0.00      |
| 1D-4   | 3.39   | 3.78  | 1.0  | 3.4  | 0.6   | 2.4 | 9.3  | 0.0  | 94%       | -0.3         | -0.3        | +0.0       | 0.00      |
| 1A-3   | 3.20   | 3.26  | 2.1  | 3.1  | 0.0   | 2.0 | 7.1  | 0.0  | 34%       | -0.7         | -0.3        | +0.0       | 0.00      |
| 1A-9   | 1.84   | 3.12  | 3.1  | 5.2  | 2.2   | 2.9 | 4.8  | 0.0  | 17%       | +0.1         | +2.0        | +0.0       | 0.00      |
| 2A-7   | 3.06   | 3.07  | 1.7  | 2.5  | 0.0   | 2.1 | 7.2  | 0.0  | 44%       | +0.8         | +0.7        | +0.0       | 0.00      |
| 2B-14  | 3.89   | 2.83  | 5.5  | 0.9  | 3.2   | 3.7 | 7.6  | 0.0  | 80%       | +1.7         | +3.2        | +0.0       | 0.00      |
| 2A-5   | 2.79   | 2.77  | 1.6  | 0.9  | 0.0   | 2.5 | 7.2  | 0.0  | 48%       | +2.0         | +1.1        | +0.0       | 0.00      |
| 1D-3   | 3.00   | 2.76  | 1.6  | 3.4  | 0.2   | 2.4 | 5.5  | 0.0  | 71%       | +0.3         | +0.4        | +0.0       | 0.00      |
| 2A-2   | 2.34   | 2.75  | 0.6  | 1.6  | 0.0   | 2.1 | 8.5  | 0.0  | 100%      | +0.2         | +0.1        | +0.0       | 0.00      |
| 2B-7   | 2.85   | 2.70  | 1.2  | 1.4  | 0.0   | 2.0 | 7.8  | 0.0  | 80%       | -0.2         | -0.2        | +0.0       | 0.00      |
| 1B-5   | 3.39   | 2.60  | 2.8  | 1.0  | 1.5   | 5.1 | 5.7  | 0.0  | 74%       | +1.9         | +3.4        | +0.0       | 0.00      |
| 2B-8   | 2.97   | 2.55  | 1.3  | 1.5  | 0.2   | 2.1 | 7.1  | 0.0  | 75%       | -0.3         | -0.3        | +0.0       | 0.00      |
| 1B-4   | 2.54   | 2.48  | 2.2  | 0.9  | 2.2   | 7.2 | 7.6  | 0.0  | 76%       | -2.8         | -2.7        | +0.0       | 0.00      |
| 1A-6   | 3.72   | 2.47  | 1.4  | 1.9  | 0.0   | 2.7 | 7.4  | 0.0  | 81%       | +1.6         | +1.7        | +0.0       | 0.00      |
| 2C-5   | 3.05   | 2.39  | 2.4  | 0.6  | 1.6   | 3.1 | 7.8  | 0.0  | 35%       | -1.4         | -1.0        | +0.0       | 0.00      |
| 2A-6   | 2.76   | 2.33  | 0.6  | 1.1  | 0.2   | 7.2 | 7.9  | 0.0  | 100%      | +2.0         | +1.5        | +0.0       | 0.00      |
| 2C-6   | 2.94   | 2.26  | 2.4  | 0.6  | 1.5   | 2.8 | 5.8  | 0.0  | 72%       | -2.3         | -1.6        | +0.0       | 0.00      |
| 2B-6   | 3.19   | 2.23  | 2.0  | 0.4  | 1.6   | 2.6 | 6.8  | 0.0  | 57%       | +0.8         | +1.0        | +0.0       | 0.00      |
| 2B-13  | 2.23   | 2.10  | 3.7  | 0.9  | 1.7   | 6.3 | 5.0  | 0.0  | 39%       | +1.5         | +3.0        | +0.0       | 0.00      |
| 1A-7   | 1.53   | 1.98  | 3.3  | -    | 1.7   | 8.7 | 8.6  | 0.0  | 91%       | +5.4         | +8.5        | +0.0       | 0.00      |
| 2A-9   | 3.22   | 1.97  | 1.3  | 0.6  | 0.2   | 2.9 | 7.5  | 0.0  | 98%       | +2.5         | +2.2        | +0.0       | 0.00      |
| 2B-1   | 2.87   | 1.96  | 1.1  | 0.6  | 0.0   | 3.0 | 8.3  | 0.0  | 100%      | +1.8         | +1.7        | +0.0       | 0.00      |
| 2B-11  | 2.08   | 1.95  | 2.9  | 0.8  | 0.8   | 3.2 | 6.7  | 0.0  | 31%       | +1.4         | +2.9        | +0.0       | 0.00      |
| 1R-1   | 2.98   | 1.94  | 1.5  | 0.5  | 0.0   | 3.1 | 7.4  | 0.0  | 83%       | +3.0         | +2.2        | +0.0       | 0.00      |
| 1C-3   | 2.40   | 1.91  | 2.6  | 0.8  | 0.4   | 2.1 | 5.5  | 0.0  | 74%       | -3.4         | -2.5        | +0.0       | 0.00      |
| 2B-10  | 2.03   | 1.91  | 2.7  | 0.4  | 2.5   | 2.8 | 6.8  | 0.0  | 25%       | -1.3         | -1.3        | +0.0       | 0.00      |
| 2B-5   | 3.27   | 1.90  | 2.1  | 0.4  | 1.8   | 2.6 | 7.0  | 0.0  | 44%       | +0.7         | +0.9        | +0.0       | 0.00      |
| 2A-16  | 2.27   | 1.85  | 2.2  | 0.9  | 0.6   | 3.6 | 6.6  | 0.0  | 92%       | +1.7         | +3.0        | +0.0       | 0.00      |
| 2C-4   | 2.45   | 1.85  | 1.4  | 0.6  | 0.9   | 3.9 | 7.2  | 0.0  | 92%       | -1.1         | -1.0        | +0.0       | 0.00      |
| 2A-13  | 2.70   | 1.84  | 1.3  | 0.8  | 0.3   | 2.8 | 7.3  | 0.0  | 96%       | +1.7         | +2.1        | +0.0       | 0.00      |
| 2B-12  | 2.00   | 1.80  | 2.7  | 0.9  | 1.8   | 3.1 | 4.8  | 0.0  | 34%       | +1.4         | +2.9        | +0.0       | 0.00      |
| 1B-1   | 2.70   | 1.75  | 0.9  | 0.6  | 0.0   | 2.9 | 7.2  | 0.0  | 100%      | +1.9         | +1.8        | +0.0       | 0.00      |
| 2A-12  | 2.55   | 1.75  | 1.4  | 0.6  | 0.3   | 3.3 | 7.2  | 0.0  | 97%       | +2.8         | +2.6        | +0.0       | 0.00      |
| 1C-1   | 1.68   | 1.74  | 1.5  | 0.4  | 2.3   | 4.6 | 7.1  | 0.0  | 90%       | -1.8         | -2.3        | +0.0       | 0.00      |
| 2B-4   | 3.14   | 1.73  | 2.2  | 0.5  | 1.8   | 2.6 | 6.4  | 0.0  | 34%       | +1.0         | +1.1        | +0.0       | 0.00      |
| 2C-1   | 1.57   | 1.71  | 1.2  | 0.5  | 0.3   | 3.7 | 7.6  | 0.0  | 69%       | -1.9         | -2.4        | +0.0       | 0.00      |
| 1A-5   | 2.58   | 1.70  | 1.3  | 0.5  | 0.0   | 4.3 | 5.8  | 0.0  | 96%       | +3.0         | +2.2        | +0.0       | 0.00      |
| 1B-2   | 2.72   | 1.69  | 1.3  | 0.6  | 0.0   | 3.0 | 7.9  | 0.0  | 100%      | +1.6         | +1.5        | +0.0       | 0.00      |
| 1C-2   | 2.28   | 1.69  | 2.0  | 0.6  | 3.0   | 4.7 | 4.2  | 0.0  | 32%       | -1.0         | -1.1        | +0.0       | 0.00      |
| 2B-2   | 2.46   | 1.64  | 0.9  | 0.5  | 0.0   | 2.7 | 6.7  | 0.0  | 100%      | +1.5         | +1.4        | +0.0       | 0.00      |
| 1A-8   | 2.92   | 1.60  | 1.5  | 0.9  | 1.0   | 3.2 | 5.3  | 0.0  | 94%       | +2.5         | +2.9        | +0.0       | 0.00      |
| 2C-2   | 1.80   | 1.59  | 1.2  | 0.3  | 2.2   | 5.1 | 5.6  | 0.0  | 86%       | -1.5         | -1.8        | +0.0       | 0.00      |
| 1A-2   | 1.69   | 1.59  | 1.6  | 0.9  | 0.0   | 2.2 | 8.3  | 0.0  | 39%       | -0.1         | -0.5        | +0.0       | 0.00      |
| 2A-10  | 1.35   | 1.58  | 2.5  | -    | 1.3   | 8.9 | 5.3  | 0.0  | 94%       | +4.9         | +6.8        | +0.0       | 0.00      |
| 2R-1   | 1.98   | 1.49  | 1.8  | 0.5  | 0.0   | 3.0 | 7.0  | 0.0  | 48%       | +2.9         | +2.2        | +0.0       | 0.00      |
| 2C-3   | 1.82   | 1.47  | 1.2  | 0.4  | 0.4   | 4.5 | 5.5  | 0.0  | 91%       | -1.3         | -1.5        | +0.0       | 0.00      |
| 1B-3   | 2.42   | 1.46  | 1.0  | 0.4  | 0.2   | 2.4 | 6.3  | 0.0  | 99%       | +0.2         | +0.4        | +0.0       | 0.00      |
| 2B-9   | 1.92   | 1.42  | 1.0  | 0.7  | 0.0   | 2.0 | 7.2  | 0.0  | 87%       | -0.4         | -0.5        | +0.0       | 0.00      |
| 2B-3   | 2.81   | 1.39  | 1.0  | 0.5  | 1.5   | 3.0 | 4.3  | 0.0  | 100%      | +1.1         | +1.2        | +0.0       | 0.00      |
| 2A-1   | 1.36   | 1.33  | 1.7  | -    | 0.0   | 7.5 | 5.6  | 0.0  | 57%       | -0.7         | -1.1        | +0.0       | 0.00      |
| 1A-4   | 1.08   | 1.26  | 2.4  | -    | 0.0   | 4.4 | 6.7  | 0.0  | 33%       | +3.2         | +4.0        | +0.0       | 0.00      |
| 2A-15  | 1.35   | 1.22  | 1.8  | -    | 1.2   | 7.0 | 4.0  | 0.0  | 92%       | +3.6         | +3.7        | +0.0       | 0.00      |
| 1D-1   | 1.20   | 1.15  | 2.6  | -    | 0.0   | 2.4 | 6.7  | 0.0  | 22%       | +0.8         | +1.7        | +0.0       | 0.00      |
| 2A-8   | 1.14   | 1.12  | 2.0  | -    | 0.0   | 2.8 | 7.2  | 0.0  | 46%       | +3.2         | +2.4        | +0.0       | 0.00      |
| 1A-1   | 1.16   | 1.11  | 2.1  | -    | 0.0   | 3.8 | 5.9  | 0.0  | 36%       | -2.3         | -1.6        | +0.0       | 0.00      |
| 2A-11  | 1.15   | 1.10  | 1.7  | -    | 0.1   | 3.6 | 6.7  | 0.0  | 94%       | +3.5         | +3.0        | +0.0       | 0.00      |
| 2A-4   | 1.12   | 1.08  | 1.8  | -    | 0.0   | 2.7 | 7.2  | 0.0  | 44%       | +1.5         | +0.7        | +0.0       | 0.00      |
| 1E-2   | 0.88   | 1.07  | 2.3  | -    | 0.0   | 2.8 | 5.9  | 0.0  | 32%       | +3.0         | +3.9        | +0.0       | 0.00      |
| 1D-2   | 1.02   | 1.04  | 1.4  | -    | 0.0   | 4.5 | 5.7  | 0.0  | 74%       | +1.0         | +2.4        | +0.0       | 0.00      |
| 2A-3   | 0.98   | 1.00  | 1.9  | -    | 0.0   | 2.1 | 6.6  | 0.0  | 41%       | +0.5         | +0.9        | +0.0       | 0.00      |
| 1E-1   | 0.99   | 0.96  | 3.0  | -    | 0.4   | 2.2 | 3.7  | 0.0  | 48%       | -0.2         | +0.9        | +0.0       | 0.00      |

### Resonant peak (Resonance=max samples)

| sample | peak Hz (hw) | peak Hz (before) | peak Hz (after) | height dB (hw) | height dB (before) | height dB (after) |
|--------|--------------|------------------|-----------------|----------------|--------------------|-------------------|
| 1A-2   | 131          | 65               | 65              | -1.5           | +0.0               | +0.0              |
| 1A-3   | 196          | 196              | 196             | -0.1           | +0.1               | -2.0              |
| 1A-5   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1A-6   | 327          | 458              | 392             | -1.9           | -0.8               | -3.5              |
| 1A-8   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1A-9   | 1701         | 1766             | 1766            | -3.9           | -6.1               | -10.7             |
| 1B-1   | 65           | 392              | 65              | +0.0           | -4.6               | +0.0              |
| 1B-2   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1B-3   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1B-4   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1B-5   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1C-1   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1C-2   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1C-3   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1D-3   | 589          | 262              | 65              | -10.3          | -2.1               | +0.0              |
| 1D-4   | 981          | 15436            | 14847           | -6.4           | -138.3             | -136.0            |
| 1R-1   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 2A-2   | 131          | 131              | 65              | +0.5           | -1.1               | +0.0              |
| 2A-5   | 65           | 14128            | 65              | +0.0           | -141.9             | +0.0              |
| 2A-6   | 196          | 196              | 196             | -2.3           | -1.8               | -3.2              |
| 2A-7   | 196          | 262              | 262             | +1.3           | +0.4               | -3.4              |
| 2A-9   | 65           | 392              | 327             | +0.0           | -4.5               | -5.3              |
| 2A-12  | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 2A-13  | 65           | 720              | 720             | +0.0           | -7.7               | -9.8              |
| 2A-14  | 1439         | 850              | 785             | -18.3          | -2.6               | -7.0              |
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
| 2B-10  | 65           | 15632            | 14128           | +0.0           | -137.4             | -140.0            |
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

- **1A-1** worst: k13 850Hz ref -56.6 / sim -62.8, k17 1112Hz ref -65.4 / sim -73.3, k14 916Hz ref -59.1 / sim -65.6, k21 1374Hz ref -72.7 / sim -82.0. Model excess above 4 kHz: none > 3 dB.
- **1A-2** worst: k21 1374Hz ref -68.4 / sim -74.3, k25 1635Hz ref -74.7 / sim -81.5, k19 1243Hz ref -64.8 / sim -70.2, k17 1112Hz ref -60.8 / sim -65.8. Model excess above 4 kHz: none > 3 dB.
- **1A-3** worst: k14 916Hz ref -47.4 / sim -53.6, k13 850Hz ref -45.1 / sim -50.7, k23 1504Hz ref -65.8 / sim -73.5, k26 1701Hz ref -70.5 / sim -78.7. Model excess above 4 kHz: none > 3 dB.
- **1A-4** worst: k31 2028Hz ref -68.8 / sim -74.7, k30 1962Hz ref -67.6 / sim -73.5, k32 2093Hz ref -69.9 / sim -75.9, k29 1897Hz ref -66.4 / sim -72.2. Model excess above 4 kHz: none > 3 dB.
- **1A-5** worst: k5 327Hz ref -8.9 / sim -5.7, k6 392Hz ref -12.3 / sim -9.2, k43 2812Hz ref -76.8 / sim -79.5, k42 2747Hz ref -76.0 / sim -78.6. Model excess above 4 kHz: none > 3 dB.
- **1A-6** worst: k28 1831Hz ref -55.8 / sim -59.3, k29 1897Hz ref -57.2 / sim -60.7, k31 2028Hz ref -59.7 / sim -63.2, k27 1766Hz ref -54.4 / sim -57.8. Model excess above 4 kHz: none > 3 dB.
- **1A-7** worst: k3 196Hz ref -8.4 / sim 2.2, k2 131Hz ref -4.9 / sim 5.5, k4 262Hz ref -10.7 / sim -1.1, k5 327Hz ref -12.5 / sim -4.1. Model excess above 4 kHz: none > 3 dB.
- **1A-8** worst: k4 262Hz ref -10.4 / sim -6.3, k3 196Hz ref -8.2 / sim -4.1, k5 327Hz ref -11.9 / sim -8.0, k2 131Hz ref -4.9 / sim -1.2. Model excess above 4 kHz: none > 3 dB.
- **1A-9** worst: k87 5690Hz ref -62.2 / sim -57.0, k99 6475Hz ref -67.0 / sim -61.9, k88 5756Hz ref -62.4 / sim -57.3, k112 7326Hz ref -72.5 / sim -67.5. Model excess above 4 kHz: harmonic 5690Hz +5.2dB, harmonic 6475Hz +5.0dB, harmonic 5756Hz +5.0dB, inter-harmonic 4088Hz +3.5dB.
- **1B-1** worst: k6 392Hz ref -8.2 / sim -5.3, k7 458Hz ref -9.6 / sim -7.3, k3 196Hz ref -6.4 / sim -4.2, k2 131Hz ref -4.4 / sim -2.1. Model excess above 4 kHz: none > 3 dB.
- **1B-2** worst: k38 2485Hz ref -57.1 / sim -59.8, k39 2551Hz ref -58.2 / sim -60.9, k37 2420Hz ref -56.0 / sim -58.7, k40 2616Hz ref -59.2 / sim -61.9. Model excess above 4 kHz: none > 3 dB.
- **1B-3** worst: k24 1570Hz ref -21.0 / sim -24.2, k23 1504Hz ref -20.8 / sim -23.6, k30 1962Hz ref -23.3 / sim -26.4, k21 1374Hz ref -20.0 / sim -22.7. Model excess above 4 kHz: none > 3 dB.
- **1B-4** worst: k41 2682Hz ref -42.8 / sim -37.7, k42 2747Hz ref -43.8 / sim -38.9, k40 2616Hz ref -41.6 / sim -36.7, k43 2812Hz ref -44.9 / sim -40.0. Model excess above 4 kHz: none > 3 dB.
- **1B-5** worst: k10 654Hz ref -14.9 / sim -10.3, k4 262Hz ref -10.4 / sim -5.8, k11 720Hz ref -14.8 / sim -10.2, k168 10988Hz ref -49.7 / sim -45.2. Model excess above 4 kHz: harmonic 14586Hz +5.7dB, harmonic 14782Hz +5.7dB, harmonic 14520Hz +5.7dB, inter-harmonic 14226Hz +5.5dB.
- **1C-1** worst: k7 458Hz ref -17.4 / sim -21.3, k6 392Hz ref -15.9 / sim -18.7, k2 131Hz ref -6.0 / sim -8.4, k5 327Hz ref -14.8 / sim -17.0. Model excess above 4 kHz: inter-harmonic 4088Hz +5.0dB, inter-harmonic 4153Hz +4.1dB, inter-harmonic 4219Hz +3.2dB.
- **1C-2** worst: k105 6868Hz ref -77.6 / sim -74.0, k106 6933Hz ref -78.0 / sim -74.5, k104 6802Hz ref -77.2 / sim -73.6, k48 3140Hz ref -44.7 / sim -41.2. Model excess above 4 kHz: inter-harmonic 4088Hz +4.1dB, inter-harmonic 6181Hz +3.8dB, inter-harmonic 6246Hz +3.8dB, harmonic 6868Hz +3.6dB.
- **1C-3** worst: k37 2420Hz ref -21.4 / sim -27.6, k36 2355Hz ref -21.1 / sim -27.3, k38 2485Hz ref -21.7 / sim -28.0, k35 2289Hz ref -20.8 / sim -27.0. Model excess above 4 kHz: none > 3 dB.
- **1D-1** worst: k28 1831Hz ref -66.1 / sim -72.4, k29 1897Hz ref -67.3 / sim -73.7, k27 1766Hz ref -64.8 / sim -70.9, k30 1962Hz ref -68.5 / sim -75.0. Model excess above 4 kHz: none > 3 dB.
- **1D-2** worst: k10 654Hz ref -20.6 / sim -25.2, k11 720Hz ref -21.7 / sim -26.8, k9 589Hz ref -19.4 / sim -23.3, k12 785Hz ref -22.7 / sim -28.2. Model excess above 4 kHz: none > 3 dB.
- **1D-3** worst: k29 1897Hz ref -56.3 / sim -61.1, k28 1831Hz ref -54.9 / sim -59.7, k30 1962Hz ref -57.8 / sim -62.5, k27 1766Hz ref -53.5 / sim -58.2. Model excess above 4 kHz: none > 3 dB.
- **1D-4** worst: k70 4578Hz ref -79.6 / sim -76.1, k69 4513Hz ref -79.0 / sim -75.6, k68 4448Hz ref -78.3 / sim -75.0, k67 4382Hz ref -77.7 / sim -74.5. Model excess above 4 kHz: harmonic 4578Hz +3.5dB, harmonic 4513Hz +3.5dB, harmonic 4644Hz +3.4dB.
- **1E-1** worst: k11 720Hz ref -37.1 / sim -48.2, k13 850Hz ref -42.6 / sim -58.4, k15 981Hz ref -47.8 / sim -70.3, k28 1831Hz ref -69.8 / sim -82.6. Model excess above 4 kHz: none > 3 dB.
- **1E-2** worst: k29 1897Hz ref -66.8 / sim -72.6, k30 1962Hz ref -68.1 / sim -73.9, k31 2028Hz ref -69.2 / sim -75.3, k28 1831Hz ref -65.6 / sim -71.2. Model excess above 4 kHz: none > 3 dB.
- **1R-1** worst: k43 2812Hz ref -76.3 / sim -79.5, k41 2682Hz ref -74.5 / sim -77.7, k5 327Hz ref -8.9 / sim -5.8, k42 2747Hz ref -75.5 / sim -78.6. Model excess above 4 kHz: none > 3 dB.
- **2A-1** worst: k20 1308Hz ref -68.5 / sim -75.9, k21 1374Hz ref -70.2 / sim -77.9, k19 1243Hz ref -66.9 / sim -73.8, k18 1177Hz ref -65.4 / sim -71.6. Model excess above 4 kHz: none > 3 dB.
- **2A-2** worst: k5 327Hz ref -18.7 / sim -16.4, k7 458Hz ref -26.7 / sim -28.9, k12 785Hz ref -47.5 / sim -49.5, k13 850Hz ref -50.0 / sim -52.7. Model excess above 4 kHz: none > 3 dB.
- **2A-3** worst: k19 1243Hz ref -64.2 / sim -70.2, k18 1177Hz ref -62.4 / sim -68.1, k20 1308Hz ref -65.9 / sim -72.3, k22 1439Hz ref -69.2 / sim -76.0. Model excess above 4 kHz: none > 3 dB.
- **2A-4** worst: k20 1308Hz ref -62.5 / sim -67.9, k19 1243Hz ref -60.6 / sim -65.8, k23 1504Hz ref -67.3 / sim -73.5, k21 1374Hz ref -64.0 / sim -69.8. Model excess above 4 kHz: none > 3 dB.
- **2A-5** worst: k29 1897Hz ref -74.3 / sim -79.5, k28 1831Hz ref -73.0 / sim -78.1, k27 1766Hz ref -71.5 / sim -76.5, k26 1701Hz ref -70.1 / sim -75.0. Model excess above 4 kHz: none > 3 dB.
- **2A-6** worst: k4 262Hz ref -7.5 / sim -4.6, k1 65Hz ref 0.0 / sim 1.8, k5 327Hz ref -11.4 / sim -10.2, k2 131Hz ref -2.1 / sim -0.9. Model excess above 4 kHz: none > 3 dB.
- **2A-7** worst: k31 2028Hz ref -74.3 / sim -79.1, k28 1831Hz ref -70.3 / sim -74.9, k21 1374Hz ref -58.7 / sim -63.3, k27 1766Hz ref -68.9 / sim -73.4. Model excess above 4 kHz: none > 3 dB.
- **2A-8** worst: k35 2289Hz ref -69.8 / sim -74.8, k36 2355Hz ref -70.8 / sim -75.9, k34 2224Hz ref -68.7 / sim -73.7, k37 2420Hz ref -71.8 / sim -76.9. Model excess above 4 kHz: none > 3 dB.
- **2A-9** worst: k6 392Hz ref -8.9 / sim -5.1, k25 1635Hz ref -53.8 / sim -56.5, k24 1570Hz ref -52.2 / sim -54.9, k23 1504Hz ref -50.6 / sim -53.2. Model excess above 4 kHz: none > 3 dB.
- **2A-10** worst: k2 131Hz ref -4.9 / sim 2.9, k1 65Hz ref 0.0 / sim 7.2, k3 196Hz ref -8.3 / sim -2.0, k4 262Hz ref -10.7 / sim -6.4. Model excess above 4 kHz: none > 3 dB.
- **2A-11** worst: k3 196Hz ref -7.6 / sim -3.7, k4 262Hz ref -9.3 / sim -5.7, k2 131Hz ref -4.7 / sim -1.1, k5 327Hz ref -10.3 / sim -7.2. Model excess above 4 kHz: none > 3 dB.
- **2A-12** worst: k3 196Hz ref -7.6 / sim -4.2, k4 262Hz ref -9.2 / sim -6.0, k2 131Hz ref -4.6 / sim -1.6, k5 327Hz ref -9.9 / sim -7.1. Model excess above 4 kHz: none > 3 dB.
- **2A-13** worst: k11 720Hz ref -11.1 / sim -7.8, k3 196Hz ref -7.8 / sim -4.7, k4 262Hz ref -9.5 / sim -6.5, k5 327Hz ref -10.4 / sim -7.7. Model excess above 4 kHz: none > 3 dB.
- **2A-14** worst: k4 262Hz ref -9.7 / sim -6.4, k3 196Hz ref -7.9 / sim -4.6, k5 327Hz ref -10.8 / sim -7.7, k12 785Hz ref -7.8 / sim -4.8. Model excess above 4 kHz: none > 3 dB.
- **2A-15** worst: k4 262Hz ref -10.4 / sim -5.6, k3 196Hz ref -8.2 / sim -3.4, k5 327Hz ref -11.9 / sim -7.3, k2 131Hz ref -4.9 / sim -0.5. Model excess above 4 kHz: none > 3 dB.
- **2A-16** worst: k4 262Hz ref -10.4 / sim -6.1, k3 196Hz ref -8.2 / sim -3.9, k5 327Hz ref -12.0 / sim -7.8, k6 392Hz ref -13.2 / sim -9.2. Model excess above 4 kHz: none > 3 dB.
- **2B-1** worst: k6 392Hz ref -8.6 / sim -5.8, k28 1831Hz ref -52.5 / sim -54.8, k29 1897Hz ref -53.9 / sim -56.2, k27 1766Hz ref -51.1 / sim -53.4. Model excess above 4 kHz: none > 3 dB.
- **2B-2** worst: k5 327Hz ref -8.6 / sim -6.5, k2 131Hz ref -4.2 / sim -2.2, k4 262Hz ref -6.6 / sim -4.7, k3 196Hz ref -5.8 / sim -4.0. Model excess above 4 kHz: none > 3 dB.
- **2B-3** worst: k2 131Hz ref -4.2 / sim -2.3, k86 5625Hz ref -79.3 / sim -77.4, k87 5690Hz ref -79.7 / sim -77.8, k29 1897Hz ref -34.8 / sim -32.9. Model excess above 4 kHz: none > 3 dB.
- **2B-4** worst: k32 2093Hz ref -37.2 / sim -31.6, k33 2158Hz ref -38.7 / sim -33.1, k31 2028Hz ref -35.6 / sim -30.2, k34 2224Hz ref -40.1 / sim -34.7. Model excess above 4 kHz: harmonic 4251Hz +3.7dB, harmonic 4317Hz +3.7dB, harmonic 4382Hz +3.7dB.
- **2B-5** worst: k39 2551Hz ref -39.8 / sim -34.5, k38 2485Hz ref -38.5 / sim -33.2, k40 2616Hz ref -41.0 / sim -35.8, k37 2420Hz ref -37.2 / sim -32.1. Model excess above 4 kHz: harmonic 5036Hz +3.6dB, harmonic 4971Hz +3.6dB, harmonic 5102Hz +3.5dB.
- **2B-6** worst: k51 3336Hz ref -41.5 / sim -36.7, k50 3270Hz ref -40.5 / sim -35.7, k52 3401Hz ref -42.4 / sim -37.8, k71 4644Hz ref -56.6 / sim -52.0. Model excess above 4 kHz: harmonic 4644Hz +4.5dB, harmonic 5102Hz +4.5dB, harmonic 4840Hz +4.3dB, inter-harmonic 4088Hz +4.3dB.
- **2B-7** worst: k14 916Hz ref -47.2 / sim -50.8, k15 981Hz ref -49.9 / sim -53.5, k13 850Hz ref -44.5 / sim -48.0, k16 1046Hz ref -52.6 / sim -55.9. Model excess above 4 kHz: none > 3 dB.
- **2B-8** worst: k15 981Hz ref -48.0 / sim -51.9, k16 1046Hz ref -50.5 / sim -54.4, k14 916Hz ref -45.4 / sim -49.2, k17 1112Hz ref -53.0 / sim -56.7. Model excess above 4 kHz: none > 3 dB.
- **2B-9** worst: k21 1374Hz ref -53.8 / sim -57.0, k20 1308Hz ref -51.9 / sim -55.0, k19 1243Hz ref -49.9 / sim -53.0, k18 1177Hz ref -47.8 / sim -50.9. Model excess above 4 kHz: none > 3 dB.
- **2B-10** worst: k19 1243Hz ref -37.0 / sim -29.3, k20 1308Hz ref -38.9 / sim -31.4, k18 1177Hz ref -34.7 / sim -27.3, k21 1374Hz ref -40.7 / sim -33.8. Model excess above 4 kHz: none > 3 dB.
- **2B-11** worst: k99 6475Hz ref -63.6 / sim -59.4, k98 6410Hz ref -63.2 / sim -59.0, k101 6606Hz ref -64.5 / sim -60.3, k100 6541Hz ref -64.0 / sim -59.9. Model excess above 4 kHz: harmonic 6475Hz +4.2dB, harmonic 6410Hz +4.2dB, harmonic 6606Hz +4.2dB.
- **2B-12** worst: k4 262Hz ref -10.5 / sim -6.3, k3 196Hz ref -8.2 / sim -4.1, k105 6868Hz ref -64.7 / sim -60.6, k106 6933Hz ref -65.1 / sim -61.0. Model excess above 4 kHz: harmonic 6868Hz +4.0dB, harmonic 6933Hz +4.0dB, harmonic 6802Hz +4.0dB.
- **2B-13** worst: k127 8307Hz ref -69.0 / sim -62.8, k126 8241Hz ref -68.6 / sim -62.5, k125 8176Hz ref -68.2 / sim -62.1, k124 8110Hz ref -67.9 / sim -61.8. Model excess above 4 kHz: harmonic 8372Hz +6.2dB, harmonic 8437Hz +6.2dB, harmonic 8307Hz +6.2dB, inter-harmonic 4611Hz +6.1dB.
- **2B-14** worst: k4 262Hz ref -10.4 / sim -6.0, k3 196Hz ref -8.2 / sim -3.8, k5 327Hz ref -12.0 / sim -7.6, k6 392Hz ref -13.1 / sim -9.0. Model excess above 4 kHz: harmonic 8045Hz +11.7dB, harmonic 8110Hz +11.7dB, harmonic 7980Hz +11.7dB, inter-harmonic 7816Hz +10.1dB.
- **2C-1** worst: k9 589Hz ref -17.3 / sim -21.7, k7 458Hz ref -15.6 / sim -19.3, k8 523Hz ref -16.7 / sim -20.2, k10 654Hz ref -18.2 / sim -23.8. Model excess above 4 kHz: none > 3 dB.
- **2C-2** worst: k11 720Hz ref -17.8 / sim -21.7, k12 785Hz ref -18.8 / sim -22.9, k49 3205Hz ref -51.9 / sim -48.7, k9 589Hz ref -16.6 / sim -19.8. Model excess above 4 kHz: inter-harmonic 4088Hz +3.6dB.
- **2C-3** worst: k14 916Hz ref -18.6 / sim -22.3, k13 850Hz ref -17.9 / sim -21.4, k15 981Hz ref -19.4 / sim -23.0, k16 1046Hz ref -19.5 / sim -23.4. Model excess above 4 kHz: none > 3 dB.
- **2C-4** worst: k20 1308Hz ref -19.6 / sim -23.5, k21 1374Hz ref -19.9 / sim -24.0, k18 1177Hz ref -19.0 / sim -22.4, k19 1243Hz ref -19.5 / sim -22.9. Model excess above 4 kHz: none > 3 dB.
- **2C-5** worst: k48 3140Hz ref -40.4 / sim -35.5, k49 3205Hz ref -41.5 / sim -36.5, k50 3270Hz ref -42.4 / sim -37.6, k47 3074Hz ref -39.3 / sim -34.6. Model excess above 4 kHz: harmonic 4055Hz +3.8dB, harmonic 4121Hz +3.8dB, harmonic 4186Hz +3.6dB.
- **2C-6** worst: k33 2158Hz ref -21.5 / sim -26.4, k34 2224Hz ref -21.8 / sim -26.8, k32 2093Hz ref -21.2 / sim -26.1, k31 2028Hz ref -20.9 / sim -25.8. Model excess above 4 kHz: none > 3 dB.
- **2R-1** worst: k43 2812Hz ref -75.5 / sim -79.6, k42 2747Hz ref -74.6 / sim -78.6, k40 2616Hz ref -72.8 / sim -76.7, k41 2682Hz ref -73.8 / sim -77.7. Model excess above 4 kHz: none > 3 dB.

## Suspicious samples

- **2C-4**: spectrum is within 1.2 dB of 2C-5 although the decay label differs -- one of the two labels is probably wrong
- **2B-11**: spectrum is within 1.2 dB of 2B-12 although the envMod label differs -- one of the two labels is probably wrong
- **2B-3**: spectrum is within 1.5 dB of 2B-4 although the envMod label differs -- one of the two labels is probably wrong
- **2A-6**: spectrum is within 1.0 dB of 2B-7 although the cutoff/decay label differs -- one of the two labels is probably wrong
- **1D-1**: spectrum is within 1.0 dB of 1E-2 although the cutoff/envMod/decay/accent label differs -- one of the two labels is probably wrong
- **1A-4**: spectrum is within 1.2 dB of 1D-1 although the cutoff/envMod/decay/accent label differs -- one of the two labels is probably wrong
- **1A-3**: spectrum is within 1.4 dB of 2B-7 although the resonance/decay label differs -- one of the two labels is probably wrong
- **2A-14**: error 3.88 is 2.2x the median
- **1D-4**: error 3.78 is 2.1x the median
- **1A-3**: error 3.26 is 1.8x the median
- **1A-9**: error 3.12 is 1.7x the median
- **2A-7**: error 3.07 is 1.7x the median
- **2B-14**: error 2.83 is 1.6x the median
- **2A-5**: error 2.77 is 1.5x the median
- **1D-3**: error 2.76 is 1.5x the median
- **2A-2**: error 2.75 is 1.5x the median
- **2B-7**: error 2.70 is 1.5x the median

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

| parameter              | before    | after     | sensitivity |                                     |
|------------------------|-----------|-----------|-------------|-------------------------------------|
| filterFeedbackGain     | 16.393    | 15.709    | 0.000       |                                     |
| filterResonanceLimit   | 0.91309   | 0.90184   | 0.000       | AT BOUND (model may lack structure) |
| filterResonanceSkew    | -0.8718   | 0.44087   | 0.000       |                                     |
| resCouplingHz          | 104.29    | 108.2     | 0.000       |                                     |
| filterCapScale1        | 1.2895    | 1.2627    | 0.000       |                                     |
| filterCapScale2        | 0.69764   | 0.63414   | 0.000       |                                     |
| filterCapScale3        | 0.91275   | 0.96187   | 0.000       |                                     |
| filterCapScale4        | 1.0631    | 0.8543    | 0.000       |                                     |
| filterLadderInputScale | 0.035224  | 0.03194   | 0.000       |                                     |
| filterInputCouplingHz  | 6.0165    | 5.1181    | 0.000       |                                     |
| filterOutputCouplingHz | 20000     | 19729     | 0.000       |                                     |
| filterPostHpHz         | 198.89    | 127.26    | 0.000       |                                     |
| filterNotchHz          | 7.5164    | 9.8276    | 0.000       |                                     |
| filterNotchBandwidthHz | 4.7       | 6.5224    | 0.000       |                                     |
| filterAllpassHz        | 14.008    | 12.659    | 0.000       |                                     |
| cutoffBaseHz           | 222.01    | 259.34    | 0.000       |                                     |
| cutoffSpanOct          | 3.3414    | 3.298     | 0.000       |                                     |
| cutoffTaperExp         | 1.4289    | 1.6914    | 0.000       |                                     |
| cutoffMaxHz            | 23723     | 26164     | 0.000       |                                     |
| envModScaleC0          | 0.76145   | 0.90838   | 0.000       |                                     |
| envModScaleC0Slope     | 5.1991    | 5.1543    | 0.000       |                                     |
| envModScaleC1          | 0.77794   | 0.90858   | 0.000       |                                     |
| envModScaleC1Slope     | 4.069     | 3.7181    | 0.000       |                                     |
| envModOffset           | 0.34816   | 0.34903   | 0.000       |                                     |
| envModOffsetCutSlope   | -0.023862 | -0.011231 | 0.000       |                                     |
| envModTaperExp         | 2         | 1.7654    | 0.000       |                                     |
| envModTaperMid         | 0.68612   | 0.73531   | 0.000       |                                     |
| envModTaperWidth       | 0.12193   | 0.1607    | 0.000       |                                     |
| accentSweepDepthOct    | 7.5466    | 6.4291    | 0.000       |                                     |
| accentVcaDepth         | 2.3468    | 1.97      | 0.000       |                                     |
| accentChargeBaseSec    | 0.030579  | 0.029491  | 0.000       |                                     |
| accentChargePotSec     | 0.044021  | 0.042243  | 0.000       |                                     |
| accentMixSec           | 0.14498   | 0.19081   | 0.000       |                                     |
| accentDiodeDrop        | 0.30033   | 0.24593   | 0.000       |                                     |
| vcfDecayMinSec         | 0.061906  | 0.058815  | 0.000       |                                     |
| vcfDecayMaxSec         | 1.0862    | 1.0743    | 0.000       |                                     |
| vcfDecayTaper          | 17.982    | 21.362    | 0.000       |                                     |
| vegDecaySec            | 1         | 1.0434    | 0.000       |                                     |
| vcaResTapRatio         | 1.7357    | 1.6209    | 0.000       |                                     |
| vcaCutoffLevelDb       | -2.5749   | -1.4606   | 0.000       |                                     |
| vcaCutoffLevelResDb    | -0.25653  | 0.51894   | 0.000       |                                     |

Timing: note-on offset -0.31 ms (relative to each clip's measured note-on), gate length 1300.4 ms.

## Applying the result

Paste `synth_parameters.txt` into `acidus::SynthParameters` (src/core/SynthEngine.hpp) or rerun with `--apply`.

Audio: `renders/` (hardware / before / after for the 40 worst samples).
