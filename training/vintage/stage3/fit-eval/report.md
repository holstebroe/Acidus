# Acidus vs. hardware TB-303 calibration report (20261010-094320)

97 reference samples, 53 free parameters. Search: 0 evaluations in 0.0 min, 0 CMA-ES restarts, stopped because: evaluate-only.

## Match quality (mean over samples)

All errors are dB; lower is better. `harm` = full-note harmonic levels (k^-0.5 weighted RMS), `inter` = energy between harmonics, `env` = RMS envelope, `stft` = 1/3-octave levels over time, `peak` = shape (height+width+implicit frequency) of the resonant-peak window on Resonance=max samples only -- weighted more heavily than `harm` so it can't be traded away.

| metric                                         | before | after |
|------------------------------------------------|--------|-------|
| objective (incl. priors)                       | 2.65   | 2.65  |
| weighted error                                 | 2.64   | 2.64  |
| harmonic error (dB)                            | 2.52   | 2.52  |
| resonant-peak shape error (dB)                 | 2.64   | 2.64  |
| inter-harmonic error (dB)                      | 1.05   | 1.05  |
| envelope error (dB)                            | 4.77   | 4.77  |
| spectrogram error (dB)                         | 8.86   | 8.86  |
| aligned waveform error (10*sqrt(residual/ref)) | 0.00   | 0.00  |
| resonant-peak sweep track error (semitones)    | 2.75   | 2.75  |
| harmonics-over-time error (dB)                 | 7.31   | 7.31  |
| mean |harmonic error| (dB)                     | 2.83   | 2.83  |
| note level error, RMS over notes (dB)          | 4.40   | 4.40  |
| harmonics within 1 dB (%)                      | 29.32  | 29.32 |
| harmonics within 3 dB (%)                      | 64.54  | 64.54 |
| harmonics within 6 dB (%)                      | 89.03  | 89.03 |

Recording gain solved as -9.8 dB (before: -9.8 dB).

## Per set

Mean error of each set's notes, and the RMS of their level errors (the recordings are not normalised, so absolute note levels are compared under one global gain).

| set | notes | before | after | level before (dB) | level after (dB) | harm | env  | stft |
|-----|-------|--------|-------|-------------------|------------------|------|------|------|
| 1A  | 9     | 2.41   | 2.41  | 4.7               | 4.7              | 2.4  | 4.5  | 7.1  |
| 1B  | 5     | 3.31   | 3.31  | 2.3               | 2.3              | 2.1  | 3.7  | 8.9  |
| 1C  | 3     | 2.97   | 2.97  | 1.9               | 1.9              | 2.4  | 3.0  | 9.7  |
| 1D  | 4     | 2.88   | 2.88  | 1.8               | 1.8              | 2.6  | 3.2  | 7.9  |
| 1E  | 2     | 1.23   | 1.23  | 4.0               | 4.0              | 2.6  | 3.4  | 6.2  |
| 1R  | 1     | 3.02   | 3.02  | 2.4               | 2.4              | 1.2  | 3.1  | 8.2  |
| 2A  | 16    | 2.12   | 2.12  | 3.4               | 3.4              | 1.9  | 4.3  | 7.3  |
| 2B  | 14    | 2.73   | 2.73  | 2.0               | 2.0              | 2.5  | 3.2  | 7.5  |
| 2C  | 6     | 2.80   | 2.80  | 1.4               | 1.4              | 1.6  | 2.9  | 10.6 |
| 2R  | 1     | 2.58   | 2.58  | 2.3               | 2.3              | 1.4  | 3.1  | 7.6  |
| 3A  | 15    | 2.53   | 2.53  | 1.6               | 1.6              | 1.6  | 2.7  | 8.4  |
| 3B  | 5     | 3.81   | 3.81  | 5.9               | 5.9              | 4.7  | 5.4  | 8.2  |
| 3C  | 6     | 3.46   | 3.46  | 12.6              | 12.6             | 7.0  | 12.3 | 14.3 |
| 3D  | 9     | 2.30   | 2.30  | 4.3               | 4.3              | 2.4  | 10.3 | 13.1 |
| 3R  | 1     | 1.47   | 1.47  | 2.4               | 2.4              | 1.1  | 4.1  | 6.2  |

## Per sample (sorted by remaining error, worst first)

| sample | before | after | harm | peak | inter | env  | stft | wave | harm ±3dB | level before | level after | tuning (c) | drift (c) |
|--------|--------|-------|------|------|-------|------|------|------|-----------|--------------|-------------|------------|-----------|
| 3B-5   | 6.04   | 6.04  | 6.4  | 10.9 | 4.8   | 3.9  | 8.3  | 0.0  | 19%       | +3.5         | +3.5        | +0.0       | 0.00      |
| 3A-15  | 5.53   | 5.53  | 4.3  | 4.1  | 2.4   | 4.3  | 10.4 | 0.0  | 30%       | +2.6         | +2.6        | +0.0       | 0.00      |
| 3B-3   | 5.22   | 5.22  | 4.4  | 6.5  | 1.9   | 3.2  | 9.5  | 0.0  | 20%       | +2.6         | +2.6        | +0.0       | 0.00      |
| 2B-14  | 5.08   | 5.08  | 8.9  | 2.8  | 6.2   | 4.1  | 8.6  | 0.0  | 64%       | +3.3         | +3.3        | +0.0       | 0.00      |
| 1B-5   | 4.71   | 4.71  | 4.5  | 2.9  | 2.5   | 5.0  | 7.3  | 0.0  | 38%       | +3.3         | +3.3        | +0.0       | 0.00      |
| 1D-3   | 4.67   | 4.67  | 3.4  | 6.4  | 0.2   | 2.5  | 8.0  | 0.0  | 31%       | +0.3         | +0.3        | +0.0       | 0.00      |
| 1D-4   | 4.38   | 4.38  | 2.1  | 4.1  | 0.5   | 2.4  | 10.8 | 0.0  | 70%       | -0.5         | -0.5        | +0.0       | 0.00      |
| 3C-1   | 4.34   | 4.34  | 5.1  | -    | 2.9   | 9.8  | 32.8 | 0.0  | 21%       | +9.7         | +9.7        | +0.0       | 0.00      |
| 3D-5   | 4.22   | 4.22  | 1.8  | -    | 2.8   | 21.5 | 26.2 | 0.0  | 72%       | +2.8         | +2.8        | +0.0       | 0.00      |
| 3C-4   | 4.14   | 4.14  | 9.8  | -    | 8.7   | 12.0 | 15.9 | 0.0  | 10%       | +12.1        | +12.1       | +0.0       | 0.00      |
| 1C-3   | 3.88   | 3.88  | 4.6  | 2.9  | 0.0   | 2.0  | 7.6  | 0.0  | 43%       | -2.6         | -2.6        | +0.0       | 0.00      |
| 3A-13  | 3.75   | 3.75  | 1.4  | 2.9  | 0.2   | 2.2  | 10.6 | 0.0  | 94%       | +1.7         | +1.7        | +0.0       | 0.00      |
| 3A-12  | 3.66   | 3.66  | 1.3  | 2.9  | 0.2   | 2.5  | 10.7 | 0.0  | 90%       | +2.3         | +2.3        | +0.0       | 0.00      |
| 2C-6   | 3.57   | 3.57  | 4.0  | 2.8  | 0.2   | 2.4  | 9.0  | 0.0  | 40%       | -1.7         | -1.7        | +0.0       | 0.00      |
| 3B-4   | 3.57   | 3.57  | 8.7  | -    | 7.2   | 11.7 | 11.7 | 0.0  | 12%       | +11.7        | +11.7       | +0.0       | 0.00      |
| 3D-6   | 3.47   | 3.47  | 2.5  | -    | 1.0   | 16.8 | 21.1 | 0.0  | 86%       | +5.5         | +5.5        | +0.0       | 0.00      |
| 1B-4   | 3.45   | 3.45  | 1.6  | 2.8  | 0.0   | 5.0  | 10.7 | 0.0  | 91%       | -2.9         | -2.9        | +0.0       | 0.00      |
| 1A-3   | 3.44   | 3.44  | 2.4  | 3.3  | 0.0   | 2.3  | 7.2  | 0.0  | 28%       | -1.1         | -1.1        | +0.0       | 0.00      |
| 3C-6   | 3.43   | 3.43  | 8.1  | -    | 7.0   | 13.5 | 9.6  | 0.0  | 17%       | +13.6        | +13.6       | +0.0       | 0.00      |
| 1A-8   | 3.40   | 3.40  | 3.0  | 2.7  | 2.2   | 4.7  | 6.3  | 0.0  | 86%       | +4.8         | +4.8        | +0.0       | 0.00      |
| 3A-11  | 3.39   | 3.39  | 1.3  | 2.4  | 0.2   | 2.3  | 10.3 | 0.0  | 92%       | +1.9         | +1.9        | +0.0       | 0.00      |
| 3C-5   | 3.37   | 3.37  | 8.0  | -    | 7.1   | 13.7 | 8.8  | 0.0  | 12%       | +13.8        | +13.8       | +0.0       | 0.00      |
| 2A-14  | 3.27   | 3.27  | 1.3  | 4.7  | 0.4   | 2.3  | 6.4  | 0.0  | 96%       | +1.5         | +1.5        | +0.0       | 0.00      |
| 2C-5   | 3.23   | 3.23  | 2.0  | 2.8  | 0.0   | 2.3  | 11.4 | 0.0  | 86%       | -1.0         | -1.0        | +0.0       | 0.00      |
| 2B-8   | 3.22   | 3.22  | 3.5  | 3.1  | 0.2   | 2.8  | 6.9  | 0.0  | 22%       | -1.5         | -1.5        | +0.0       | 0.00      |
| 3A-10  | 3.22   | 3.22  | 1.1  | 2.1  | 0.0   | 2.2  | 10.0 | 0.0  | 86%       | +1.1         | +1.1        | +0.0       | 0.00      |
| 1A-6   | 3.14   | 3.14  | 1.2  | 3.4  | 0.2   | 2.7  | 8.7  | 0.0  | 94%       | +1.7         | +1.7        | +0.0       | 0.00      |
| 2A-7   | 3.08   | 3.08  | 1.6  | 2.0  | 0.0   | 2.1  | 8.2  | 0.0  | 47%       | +0.6         | +0.6        | +0.0       | 0.00      |
| 1R-1   | 3.02   | 3.02  | 1.2  | 1.4  | 0.0   | 3.1  | 8.2  | 0.0  | 94%       | +2.4         | +2.4        | +0.0       | 0.00      |
| 3B-2   | 3.00   | 3.00  | 1.9  | 4.4  | 0.9   | 2.8  | 5.9  | 0.0  | 44%       | +0.4         | +0.4        | +0.0       | 0.00      |
| 2B-7   | 2.95   | 2.95  | 2.8  | 2.4  | 0.0   | 3.0  | 7.0  | 0.0  | 20%       | -1.4         | -1.4        | +0.0       | 0.00      |
| 1B-2   | 2.94   | 2.94  | 2.2  | 2.2  | 0.1   | 3.0  | 9.5  | 0.0  | 45%       | +1.5         | +1.5        | +0.0       | 0.00      |
| 1B-3   | 2.89   | 2.89  | 0.8  | 2.3  | 0.0   | 2.7  | 8.9  | 0.0  | 100%      | +0.6         | +0.6        | +0.0       | 0.00      |
| 3C-2   | 2.81   | 2.81  | 5.4  | -    | 5.9   | 12.5 | 8.9  | 0.0  | 0%        | +12.5        | +12.5       | +0.0       | 0.00      |
| 2B-1   | 2.79   | 2.79  | 1.2  | 2.1  | 0.0   | 3.0  | 9.4  | 0.0  | 96%       | +1.7         | +1.7        | +0.0       | 0.00      |
| 2C-4   | 2.79   | 2.79  | 1.1  | 2.7  | 0.0   | 2.8  | 11.3 | 0.0  | 96%       | -0.9         | -0.9        | +0.0       | 0.00      |
| 3A-9   | 2.77   | 2.77  | 2.5  | 2.3  | 0.4   | 3.9  | 6.5  | 0.0  | 43%       | +0.6         | +0.6        | +0.0       | 0.00      |
| 1C-2   | 2.77   | 2.77  | 1.9  | 2.7  | 0.0   | 3.8  | 10.1 | 0.0  | 77%       | -0.9         | -0.9        | +0.0       | 0.00      |
| 3D-1   | 2.73   | 2.73  | 2.4  | -    | 1.4   | 11.7 | 17.0 | 0.0  | 21%       | +1.3         | +1.3        | +0.0       | 0.00      |
| 2A-12  | 2.73   | 2.73  | 2.2  | 2.3  | 0.2   | 4.1  | 7.9  | 0.0  | 85%       | +3.6         | +3.6        | +0.0       | 0.00      |
| 3A-14  | 2.72   | 2.72  | 3.4  | 1.4  | 0.2   | 2.2  | 7.5  | 0.0  | 22%       | +0.2         | +0.2        | +0.0       | 0.00      |
| 2B-9   | 2.70   | 2.70  | 2.8  | 2.4  | 0.0   | 2.6  | 8.4  | 0.0  | 23%       | -1.6         | -1.6        | +0.0       | 0.00      |
| 3C-3   | 2.70   | 2.70  | 5.8  | -    | 0.0   | 12.6 | 9.5  | 0.0  | 0%        | +13.2        | +13.2       | +0.0       | 0.00      |
| 2A-16  | 2.69   | 2.69  | 2.4  | 2.7  | 1.1   | 3.6  | 7.0  | 0.0  | 84%       | +3.1         | +3.1        | +0.0       | 0.00      |
| 2A-5   | 2.68   | 2.68  | 1.8  | 0.8  | 0.0   | 2.2  | 7.1  | 0.0  | 42%       | +0.2         | +0.2        | +0.0       | 0.00      |
| 2B-13  | 2.65   | 2.65  | 2.9  | 2.7  | 1.3   | 6.2  | 5.4  | 0.0  | 50%       | +3.1         | +3.1        | +0.0       | 0.00      |
| 2B-3   | 2.63   | 2.63  | 1.0  | 2.1  | 0.8   | 2.9  | 6.4  | 0.0  | 99%       | +1.2         | +1.2        | +0.0       | 0.00      |
| 2B-2   | 2.62   | 2.62  | 1.8  | 2.2  | 0.1   | 2.7  | 8.4  | 0.0  | 73%       | +1.4         | +1.4        | +0.0       | 0.00      |
| 1A-7   | 2.60   | 2.60  | 4.9  | -    | 3.3   | 10.9 | 10.2 | 0.0  | 84%       | +10.8        | +10.8       | +0.0       | 0.00      |
| 2R-1   | 2.58   | 2.58  | 1.4  | 1.4  | 0.1   | 3.1  | 7.6  | 0.0  | 83%       | +2.3         | +2.3        | +0.0       | 0.00      |
| 1B-1   | 2.54   | 2.54  | 1.1  | 2.1  | 0.0   | 2.8  | 8.4  | 0.0  | 96%       | +1.7         | +1.7        | +0.0       | 0.00      |
| 1A-5   | 2.52   | 2.52  | 1.1  | 1.4  | 0.0   | 4.4  | 6.8  | 0.0  | 93%       | +2.4         | +2.4        | +0.0       | 0.00      |
| 2A-13  | 2.49   | 2.49  | 1.5  | 2.5  | 0.6   | 2.8  | 7.8  | 0.0  | 89%       | +2.2         | +2.2        | +0.0       | 0.00      |
| 2A-9   | 2.49   | 2.49  | 1.1  | 2.0  | 0.3   | 2.8  | 8.5  | 0.0  | 93%       | +2.1         | +2.1        | +0.0       | 0.00      |
| 2C-2   | 2.46   | 2.46  | 1.0  | 2.2  | 0.0   | 4.0  | 10.4 | 0.0  | 90%       | -1.4         | -1.4        | +0.0       | 0.00      |
| 2C-3   | 2.45   | 2.45  | 1.0  | 2.5  | 0.0   | 3.3  | 10.2 | 0.0  | 97%       | -1.2         | -1.2        | +0.0       | 0.00      |
| 2B-11  | 2.43   | 2.43  | 2.4  | 2.7  | 1.2   | 3.2  | 6.7  | 0.0  | 84%       | +2.9         | +2.9        | +0.0       | 0.00      |
| 2A-6   | 2.43   | 2.43  | 0.5  | 1.3  | 0.3   | 6.6  | 8.4  | 0.0  | 97%       | +0.9         | +0.9        | +0.0       | 0.00      |
| 3A-8   | 2.33   | 2.33  | 1.1  | 1.9  | 0.8   | 1.9  | 7.5  | 0.0  | 96%       | +1.3         | +1.3        | +0.0       | 0.00      |
| 2B-12  | 2.32   | 2.32  | 2.4  | 2.7  | 2.2   | 3.1  | 4.9  | 0.0  | 82%       | +2.9         | +2.9        | +0.0       | 0.00      |
| 2C-1   | 2.31   | 2.31  | 0.8  | 1.5  | 0.1   | 2.5  | 11.3 | 0.0  | 92%       | -1.9         | -1.9        | +0.0       | 0.00      |
| 2A-2   | 2.30   | 2.30  | 0.8  | 1.8  | 0.0   | 2.8  | 7.7  | 0.0  | 88%       | -1.3         | -1.3        | +0.0       | 0.00      |
| 1C-1   | 2.28   | 2.28  | 0.6  | 1.8  | 0.3   | 3.1  | 11.3 | 0.0  | 100%      | -1.8         | -1.8        | +0.0       | 0.00      |
| 3A-6   | 2.27   | 2.27  | 1.0  | 1.7  | 0.2   | 1.6  | 9.6  | 0.0  | 95%       | +0.9         | +0.9        | +0.0       | 0.00      |
| 2B-10  | 2.26   | 2.26  | 1.9  | 1.7  | 0.0   | 3.0  | 8.5  | 0.0  | 74%       | -2.0         | -2.0        | +0.0       | 0.00      |
| 3A-7   | 2.26   | 2.26  | 0.9  | 1.7  | 0.6   | 1.8  | 9.0  | 0.0  | 96%       | +1.2         | +1.2        | +0.0       | 0.00      |
| 2B-4   | 2.25   | 2.25  | 1.1  | 2.1  | 0.4   | 2.6  | 7.9  | 0.0  | 100%      | +1.1         | +1.1        | +0.0       | 0.00      |
| 3D-2   | 2.23   | 2.23  | 2.7  | -    | 0.5   | 10.3 | 11.9 | 0.0  | 18%       | +3.0         | +3.0        | +0.0       | 0.00      |
| 3A-5   | 2.19   | 2.19  | 1.2  | 1.6  | 0.0   | 1.5  | 9.4  | 0.0  | 100%      | +0.3         | +0.3        | +0.0       | 0.00      |
| 2B-5   | 2.18   | 2.18  | 0.8  | 2.1  | 0.1   | 2.7  | 8.2  | 0.0  | 100%      | +0.9         | +0.9        | +0.0       | 0.00      |
| 2B-6   | 2.17   | 2.17  | 0.9  | 1.7  | 0.0   | 2.7  | 8.3  | 0.0  | 100%      | +1.1         | +1.1        | +0.0       | 0.00      |
| 1A-9   | 2.10   | 2.10  | 2.0  | 2.6  | 1.3   | 2.5  | 4.2  | 0.0  | 77%       | +1.0         | +1.0        | +0.0       | 0.00      |
| 3D-7   | 1.97   | 1.97  | 2.5  | -    | 1.0   | 7.0  | 12.1 | 0.0  | 89%       | +5.7         | +5.7        | +0.0       | 0.00      |
| 2A-15  | 1.95   | 1.95  | 3.8  | -    | 3.3   | 8.5  | 6.7  | 0.0  | 87%       | +7.2         | +7.2        | +0.0       | 0.00      |
| 1A-2   | 1.93   | 1.93  | 2.2  | 1.7  | 0.0   | 3.5  | 7.5  | 0.0  | 14%       | -2.5         | -2.5        | +0.0       | 0.00      |
| 3D-3   | 1.89   | 1.89  | 2.4  | -    | 1.3   | 8.9  | 9.2  | 0.0  | 28%       | +3.3         | +3.3        | +0.0       | 0.00      |
| 2A-10  | 1.70   | 1.70  | 2.5  | -    | 1.8   | 8.9  | 6.3  | 0.0  | 90%       | +7.3         | +7.3        | +0.0       | 0.00      |
| 3D-8   | 1.62   | 1.62  | 2.5  | -    | 2.6   | 7.8  | 6.1  | 0.0  | 83%       | +5.9         | +5.9        | +0.0       | 0.00      |
| 1E-1   | 1.56   | 1.56  | 3.6  | -    | 2.1   | 4.9  | 6.3  | 0.0  | 30%       | +4.8         | +4.8        | +0.0       | 0.00      |
| 2A-1   | 1.51   | 1.51  | 2.7  | -    | 0.0   | 8.0  | 5.5  | 0.0  | 21%       | -3.8         | -3.8        | +0.0       | 0.00      |
| 1A-1   | 1.51   | 1.51  | 3.0  | -    | 0.0   | 6.3  | 6.5  | 0.0  | 8%        | -5.7         | -5.7        | +0.0       | 0.00      |
| 3R-1   | 1.47   | 1.47  | 1.1  | 1.4  | 0.0   | 4.1  | 6.2  | 0.0  | 93%       | +2.4         | +2.4        | +0.0       | 0.00      |
| 2A-11  | 1.39   | 1.39  | 2.1  | -    | 0.9   | 5.1  | 7.7  | 0.0  | 89%       | +4.8         | +4.8        | +0.0       | 0.00      |
| 3D-4   | 1.35   | 1.35  | 1.6  | -    | 0.6   | 5.1  | 8.4  | 0.0  | 73%       | +3.3         | +3.3        | +0.0       | 0.00      |
| 1D-1   | 1.33   | 1.33  | 3.1  | -    | 0.0   | 3.1  | 7.3  | 0.0  | 22%       | -0.4         | -0.4        | +0.0       | 0.00      |
| 3D-9   | 1.26   | 1.26  | 2.9  | -    | 2.1   | 3.4  | 5.6  | 0.0  | 83%       | +5.7         | +5.7        | +0.0       | 0.00      |
| 3B-1   | 1.25   | 1.25  | 2.3  | -    | 0.0   | 5.2  | 5.7  | 0.0  | 36%       | -4.1         | -4.1        | +0.0       | 0.00      |
| 1D-2   | 1.14   | 1.14  | 1.8  | -    | 0.5   | 4.9  | 5.6  | 0.0  | 81%       | +3.6         | +3.6        | +0.0       | 0.00      |
| 2A-8   | 1.12   | 1.12  | 1.5  | -    | 0.0   | 3.3  | 7.6  | 0.0  | 70%       | +2.6         | +2.6        | +0.0       | 0.00      |
| 2A-3   | 1.10   | 1.10  | 2.0  | -    | 0.0   | 3.0  | 6.6  | 0.0  | 38%       | -1.7         | -1.7        | +0.0       | 0.00      |
| 1A-4   | 1.09   | 1.09  | 1.6  | -    | 0.0   | 3.5  | 6.9  | 0.0  | 62%       | +3.1         | +3.1        | +0.0       | 0.00      |
| 2A-4   | 1.08   | 1.08  | 1.9  | -    | 0.0   | 2.7  | 7.0  | 0.0  | 38%       | -0.7         | -0.7        | +0.0       | 0.00      |
| 3A-2   | 1.03   | 1.03  | 1.3  | -    | 0.0   | 3.3  | 6.9  | 0.0  | 90%       | -0.6         | -0.6        | +0.0       | 0.00      |
| 3A-1   | 1.01   | 1.01  | 1.5  | -    | 0.0   | 3.1  | 6.5  | 0.0  | 58%       | -1.6         | -1.6        | +0.0       | 0.00      |
| 3A-4   | 0.96   | 0.96  | 1.4  | -    | 1.3   | 4.1  | 4.3  | 0.0  | 87%       | +3.0         | +3.0        | +0.0       | 0.00      |
| 3A-3   | 0.94   | 0.94  | 0.9  | -    | 0.3   | 3.3  | 6.4  | 0.0  | 96%       | +0.9         | +0.9        | +0.0       | 0.00      |
| 1E-2   | 0.90   | 0.90  | 1.5  | -    | 0.0   | 2.0  | 6.2  | 0.0  | 63%       | +3.0         | +3.0        | +0.0       | 0.00      |

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
| 3A-5   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 3A-6   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 3A-7   | 65           | 9942             | 9942            | +0.0           | -139.6             | -139.6            |
| 3A-8   | 65           | 11642            | 11642           | +0.0           | -133.4             | -133.4            |
| 3A-9   | 65           | 14586            | 14586           | +0.0           | -131.7             | -131.7            |
| 3A-10  | 196          | 196              | 196             | -1.5           | +0.4               | +0.4              |
| 3A-11  | 196          | 196              | 196             | -1.6           | +0.1               | +0.1              |
| 3A-12  | 196          | 523              | 523             | -2.1           | -7.5               | -7.5              |
| 3A-13  | 720          | 654              | 654             | -11.7          | -6.1               | -6.1              |
| 3A-14  | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 3A-15  | 65           | 1635             | 1635            | +0.0           | -19.5              | -19.5             |
| 3B-2   | 196          | 196              | 196             | +0.1           | -0.1               | -0.1              |
| 3B-3   | 327          | 458              | 458             | -1.7           | -3.2               | -3.2              |
| 3B-5   | 1701         | 1766             | 1766            | -7.3           | -4.0               | -4.0              |
| 3R-1   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |

Height is the resonant peak's level above the fundamental (gain-independent). This is the number that answers "is the acid squelch there": before/after collapsing to something much smaller than hardware's means the peak got flattened away, not just shifted.

`tuning` is measured from the harmonic peaks relative to equal temperament; `drift` is the std-dev of short-time pitch over the note (hardware fluctuation).

### Worst remaining harmonics / spurious high-frequency content

- **1A-1** worst: k10 654Hz ref -48.4 / sim -56.7, k9 589Hz ref -45.3 / sim -52.8, k8 523Hz ref -41.9 / sim -48.6, k13 850Hz ref -56.6 / sim -66.8. Model excess above 4 kHz: none > 3 dB.
- **1A-2** worst: k9 589Hz ref -39.0 / sim -44.9, k10 654Hz ref -42.3 / sim -48.8, k5 327Hz ref -19.6 / sim -24.3, k7 458Hz ref -30.9 / sim -35.7. Model excess above 4 kHz: none > 3 dB.
- **1A-3** worst: k13 850Hz ref -45.1 / sim -53.3, k14 916Hz ref -47.4 / sim -56.1, k11 720Hz ref -40.5 / sim -46.7, k12 785Hz ref -43.0 / sim -50.1. Model excess above 4 kHz: none > 3 dB.
- **1A-4** worst: k36 2355Hz ref -74.1 / sim -78.8, k37 2420Hz ref -75.2 / sim -79.8, k35 2289Hz ref -73.2 / sim -77.6, k34 2224Hz ref -72.1 / sim -76.5. Model excess above 4 kHz: none > 3 dB.
- **1A-5** worst: k5 327Hz ref -8.9 / sim -5.2, k6 392Hz ref -12.3 / sim -8.6, k3 196Hz ref -5.4 / sim -2.3, k2 131Hz ref -4.0 / sim -1.0. Model excess above 4 kHz: none > 3 dB.
- **1A-6** worst: k7 458Hz ref -7.6 / sim -1.2, k6 392Hz ref -4.6 / sim -0.1, k8 523Hz ref -9.8 / sim -6.3, k3 196Hz ref -6.2 / sim -4.0. Model excess above 4 kHz: none > 3 dB.
- **1A-7** worst: k3 196Hz ref -8.4 / sim 5.3, k4 262Hz ref -10.7 / sim 2.3, k2 131Hz ref -4.9 / sim 8.0, k5 327Hz ref -12.5 / sim -0.5. Model excess above 4 kHz: none > 3 dB.
- **1A-8** worst: k5 327Hz ref -11.9 / sim -4.6, k6 392Hz ref -13.0 / sim -5.8, k7 458Hz ref -13.8 / sim -6.7, k4 262Hz ref -10.4 / sim -3.2. Model excess above 4 kHz: none > 3 dB.
- **1A-9** worst: k22 1439Hz ref -15.4 / sim -11.1, k21 1374Hz ref -16.3 / sim -12.1, k20 1308Hz ref -17.0 / sim -12.8, k23 1504Hz ref -13.9 / sim -9.9. Model excess above 4 kHz: none > 3 dB.
- **1B-1** worst: k6 392Hz ref -8.2 / sim -4.1, k7 458Hz ref -9.6 / sim -5.8, k8 523Hz ref -11.0 / sim -8.1, k3 196Hz ref -6.4 / sim -3.8. Model excess above 4 kHz: none > 3 dB.
- **1B-2** worst: k35 2289Hz ref -53.8 / sim -58.3, k34 2224Hz ref -52.7 / sim -57.2, k36 2355Hz ref -54.9 / sim -59.4, k33 2158Hz ref -51.6 / sim -56.0. Model excess above 4 kHz: none > 3 dB.
- **1B-3** worst: k4 262Hz ref -8.8 / sim -6.2, k5 327Hz ref -10.4 / sim -7.9, k6 392Hz ref -11.6 / sim -9.3, k3 196Hz ref -6.5 / sim -4.4. Model excess above 4 kHz: none > 3 dB.
- **1B-4** worst: k12 785Hz ref -16.9 / sim -22.3, k13 850Hz ref -17.6 / sim -24.7, k15 981Hz ref -18.4 / sim -30.5, k14 916Hz ref -18.1 / sim -27.5. Model excess above 4 kHz: none > 3 dB.
- **1B-5** worst: k9 589Hz ref -14.8 / sim -7.9, k8 523Hz ref -14.4 / sim -7.7, k168 10988Hz ref -49.7 / sim -43.2, k167 10923Hz ref -49.6 / sim -43.1. Model excess above 4 kHz: harmonic 15763Hz +11.2dB, harmonic 15828Hz +11.2dB, harmonic 15894Hz +11.2dB, inter-harmonic 15796Hz +10.0dB.
- **1C-1** worst: k1 65Hz ref 0.0 / sim -2.5, k7 458Hz ref -17.4 / sim -19.3, k8 523Hz ref -18.2 / sim -22.1, k6 392Hz ref -15.9 / sim -17.6. Model excess above 4 kHz: none > 3 dB.
- **1C-2** worst: k46 3009Hz ref -42.0 / sim -59.7, k51 3336Hz ref -47.9 / sim -63.8, k52 3401Hz ref -48.8 / sim -64.6, k50 3270Hz ref -46.9 / sim -63.0. Model excess above 4 kHz: none > 3 dB.
- **1C-3** worst: k27 1766Hz ref -18.9 / sim -33.2, k28 1831Hz ref -19.1 / sim -34.9, k29 1897Hz ref -19.4 / sim -36.5, k26 1701Hz ref -18.6 / sim -31.3. Model excess above 4 kHz: none > 3 dB.
- **1D-1** worst: k15 981Hz ref -45.5 / sim -52.9, k16 1046Hz ref -47.5 / sim -55.1, k14 916Hz ref -43.5 / sim -50.5, k17 1112Hz ref -49.4 / sim -57.3. Model excess above 4 kHz: none > 3 dB.
- **1D-2** worst: k3 196Hz ref -9.1 / sim -3.0, k4 262Hz ref -11.8 / sim -6.1, k2 131Hz ref -4.9 / sim 0.1, k5 327Hz ref -13.9 / sim -9.1. Model excess above 4 kHz: none > 3 dB.
- **1D-3** worst: k24 1570Hz ref -49.3 / sim -58.5, k25 1635Hz ref -50.7 / sim -59.9, k23 1504Hz ref -48.0 / sim -56.9, k26 1701Hz ref -52.1 / sim -61.2. Model excess above 4 kHz: none > 3 dB.
- **1D-4** worst: k43 2812Hz ref -55.8 / sim -63.3, k44 2878Hz ref -57.0 / sim -64.0, k42 2747Hz ref -54.8 / sim -61.7, k45 2943Hz ref -57.9 / sim -64.2. Model excess above 4 kHz: none > 3 dB.
- **1E-1** worst: k29 1897Hz ref -78.0 / sim -67.4, k15 981Hz ref -47.8 / sim -65.5, k27 1766Hz ref -74.6 / sim -65.1, k13 850Hz ref -42.6 / sim -52.0. Model excess above 4 kHz: none > 3 dB.
- **1E-2** worst: k36 2355Hz ref -74.6 / sim -79.2, k35 2289Hz ref -73.6 / sim -78.0, k37 2420Hz ref -75.6 / sim -80.2, k34 2224Hz ref -72.5 / sim -76.8. Model excess above 4 kHz: none > 3 dB.
- **1R-1** worst: k5 327Hz ref -8.9 / sim -5.2, k6 392Hz ref -12.2 / sim -8.6, k3 196Hz ref -5.4 / sim -2.3, k2 131Hz ref -4.0 / sim -1.0. Model excess above 4 kHz: none > 3 dB.
- **2A-1** worst: k13 850Hz ref -54.2 / sim -62.7, k20 1308Hz ref -68.5 / sim -80.3, k12 785Hz ref -51.6 / sim -59.6, k11 720Hz ref -48.9 / sim -56.3. Model excess above 4 kHz: none > 3 dB.
- **2A-2** worst: k7 458Hz ref -26.7 / sim -32.2, k6 392Hz ref -21.7 / sim -26.5, k12 785Hz ref -47.5 / sim -52.6, k2 131Hz ref 0.0 / sim -2.2. Model excess above 4 kHz: none > 3 dB.
- **2A-3** worst: k17 1112Hz ref -60.4 / sim -66.4, k19 1243Hz ref -64.2 / sim -70.8, k18 1177Hz ref -62.4 / sim -68.7, k20 1308Hz ref -65.9 / sim -72.8. Model excess above 4 kHz: none > 3 dB.
- **2A-4** worst: k18 1177Hz ref -58.8 / sim -64.4, k20 1308Hz ref -62.5 / sim -68.6, k17 1112Hz ref -56.9 / sim -62.1, k19 1243Hz ref -60.6 / sim -66.5. Model excess above 4 kHz: none > 3 dB.
- **2A-5** worst: k29 1897Hz ref -74.3 / sim -80.2, k28 1831Hz ref -73.0 / sim -78.8, k27 1766Hz ref -71.5 / sim -77.2, k25 1635Hz ref -68.7 / sim -74.0. Model excess above 4 kHz: none > 3 dB.
- **2A-6** worst: k4 262Hz ref -7.5 / sim -4.4, k5 327Hz ref -11.4 / sim -10.2, k3 196Hz ref -2.3 / sim -1.2, k7 458Hz ref -20.8 / sim -21.9. Model excess above 4 kHz: none > 3 dB.
- **2A-7** worst: k31 2028Hz ref -74.3 / sim -79.0, k28 1831Hz ref -70.3 / sim -74.7, k30 1962Hz ref -73.2 / sim -77.6, k32 2093Hz ref -75.6 / sim -80.4. Model excess above 4 kHz: none > 3 dB.
- **2A-8** worst: k41 2682Hz ref -75.6 / sim -79.9, k40 2616Hz ref -74.7 / sim -78.9, k39 2551Hz ref -73.8 / sim -77.9, k38 2485Hz ref -72.8 / sim -76.8. Model excess above 4 kHz: none > 3 dB.
- **2A-9** worst: k6 392Hz ref -8.9 / sim -3.7, k7 458Hz ref -11.7 / sim -7.6, k5 327Hz ref -5.6 / sim -2.5, k3 196Hz ref -5.8 / sim -2.9. Model excess above 4 kHz: none > 3 dB.
- **2A-10** worst: k2 131Hz ref -4.9 / sim 3.8, k3 196Hz ref -8.3 / sim -0.3, k1 65Hz ref 0.0 / sim 7.1, k4 262Hz ref -10.7 / sim -4.2. Model excess above 4 kHz: none > 3 dB.
- **2A-11** worst: k3 196Hz ref -7.6 / sim -1.0, k4 262Hz ref -9.3 / sim -2.7, k5 327Hz ref -10.3 / sim -4.5, k2 131Hz ref -4.7 / sim 1.1. Model excess above 4 kHz: none > 3 dB.
- **2A-12** worst: k4 262Hz ref -9.2 / sim -3.7, k5 327Hz ref -9.9 / sim -4.6, k3 196Hz ref -7.6 / sim -2.4, k6 392Hz ref -10.0 / sim -5.3. Model excess above 4 kHz: none > 3 dB.
- **2A-13** worst: k11 720Hz ref -11.1 / sim -6.8, k5 327Hz ref -10.4 / sim -6.4, k4 262Hz ref -9.5 / sim -5.5, k12 785Hz ref -12.8 / sim -9.0. Model excess above 4 kHz: none > 3 dB.
- **2A-14** worst: k12 785Hz ref -7.8 / sim -1.2, k13 850Hz ref -9.5 / sim -3.6, k14 916Hz ref -10.9 / sim -7.7, k5 327Hz ref -10.8 / sim -8.0. Model excess above 4 kHz: none > 3 dB.
- **2A-15** worst: k5 327Hz ref -11.9 / sim -2.2, k4 262Hz ref -10.4 / sim -0.7, k6 392Hz ref -13.0 / sim -3.5, k7 458Hz ref -13.8 / sim -4.6. Model excess above 4 kHz: none > 3 dB.
- **2A-16** worst: k5 327Hz ref -12.0 / sim -6.5, k6 392Hz ref -13.2 / sim -7.7, k7 458Hz ref -14.0 / sim -8.6, k4 262Hz ref -10.4 / sim -5.1. Model excess above 4 kHz: none > 3 dB.
- **2B-1** worst: k6 392Hz ref -8.6 / sim -4.3, k7 458Hz ref -9.9 / sim -6.0, k5 327Hz ref -7.0 / sim -4.0, k8 523Hz ref -11.1 / sim -8.2. Model excess above 4 kHz: none > 3 dB.
- **2B-2** worst: k5 327Hz ref -8.6 / sim -4.9, k43 2812Hz ref -57.1 / sim -60.8, k42 2747Hz ref -56.2 / sim -59.8, k44 2878Hz ref -58.0 / sim -61.7. Model excess above 4 kHz: none > 3 dB.
- **2B-3** worst: k5 327Hz ref -9.1 / sim -6.0, k4 262Hz ref -7.3 / sim -4.4, k6 392Hz ref -10.4 / sim -7.6, k7 458Hz ref -11.4 / sim -9.0. Model excess above 4 kHz: none > 3 dB.
- **2B-4** worst: k5 327Hz ref -9.3 / sim -6.4, k4 262Hz ref -7.6 / sim -4.7, k6 392Hz ref -10.6 / sim -8.0, k7 458Hz ref -11.6 / sim -9.3. Model excess above 4 kHz: none > 3 dB.
- **2B-5** worst: k4 262Hz ref -7.9 / sim -5.2, k5 327Hz ref -9.6 / sim -6.9, k6 392Hz ref -10.8 / sim -8.4, k7 458Hz ref -11.8 / sim -9.7. Model excess above 4 kHz: none > 3 dB.
- **2B-6** worst: k4 262Hz ref -7.8 / sim -5.1, k5 327Hz ref -9.4 / sim -6.8, k6 392Hz ref -10.7 / sim -8.2, k7 458Hz ref -11.6 / sim -9.5. Model excess above 4 kHz: none > 3 dB.
- **2B-7** worst: k14 916Hz ref -47.2 / sim -54.2, k15 981Hz ref -49.9 / sim -56.8, k13 850Hz ref -44.5 / sim -51.4, k16 1046Hz ref -52.6 / sim -59.3. Model excess above 4 kHz: none > 3 dB.
- **2B-8** worst: k6 392Hz ref -13.6 / sim -22.1, k15 981Hz ref -48.0 / sim -56.4, k16 1046Hz ref -50.5 / sim -58.9, k14 916Hz ref -45.4 / sim -53.8. Model excess above 4 kHz: none > 3 dB.
- **2B-9** worst: k7 458Hz ref -13.4 / sim -24.3, k6 392Hz ref -11.0 / sim -18.3, k13 850Hz ref -37.0 / sim -47.7, k14 916Hz ref -39.5 / sim -50.5. Model excess above 4 kHz: none > 3 dB.
- **2B-10** worst: k10 654Hz ref -15.1 / sim -24.0, k9 589Hz ref -14.0 / sim -20.0, k23 1504Hz ref -43.9 / sim -57.3, k24 1570Hz ref -45.4 / sim -59.0. Model excess above 4 kHz: none > 3 dB.
- **2B-11** worst: k5 327Hz ref -12.0 / sim -6.7, k6 392Hz ref -13.2 / sim -7.9, k7 458Hz ref -14.1 / sim -8.9, k4 262Hz ref -10.4 / sim -5.3. Model excess above 4 kHz: none > 3 dB.
- **2B-12** worst: k5 327Hz ref -12.0 / sim -6.7, k6 392Hz ref -13.2 / sim -7.9, k7 458Hz ref -14.1 / sim -8.9, k4 262Hz ref -10.5 / sim -5.3. Model excess above 4 kHz: none > 3 dB.
- **2B-13** worst: k5 327Hz ref -12.0 / sim -6.6, k6 392Hz ref -13.2 / sim -7.8, k7 458Hz ref -14.1 / sim -8.8, k4 262Hz ref -10.5 / sim -5.2. Model excess above 4 kHz: harmonic 7326Hz +3.9dB, harmonic 7260Hz +3.9dB, harmonic 7129Hz +3.9dB.
- **2B-14** worst: k12 785Hz ref -16.1 / sim -9.5, k11 720Hz ref -15.9 / sim -9.5, k10 654Hz ref -15.6 / sim -9.5, k13 850Hz ref -16.0 / sim -9.9. Model excess above 4 kHz: harmonic 9157Hz +17.6dB, harmonic 9092Hz +17.6dB, harmonic 9222Hz +17.6dB, inter-harmonic 8928Hz +14.8dB.
- **2C-1** worst: k10 654Hz ref -18.2 / sim -22.3, k9 589Hz ref -17.3 / sim -20.2, k1 65Hz ref 0.0 / sim -2.5, k8 523Hz ref -16.7 / sim -19.0. Model excess above 4 kHz: none > 3 dB.
- **2C-2** worst: k45 2943Hz ref -47.6 / sim -65.4, k46 3009Hz ref -48.8 / sim -66.2, k44 2878Hz ref -46.2 / sim -64.5, k47 3074Hz ref -49.9 / sim -67.1. Model excess above 4 kHz: none > 3 dB.
- **2C-3** worst: k16 1046Hz ref -19.5 / sim -22.8, k17 1112Hz ref -20.0 / sim -24.0, k18 1177Hz ref -21.0 / sim -24.9, k1 65Hz ref 0.0 / sim -2.4. Model excess above 4 kHz: none > 3 dB.
- **2C-4** worst: k20 1308Hz ref -19.6 / sim -24.1, k21 1374Hz ref -19.9 / sim -25.5, k19 1243Hz ref -19.5 / sim -22.9, k22 1439Hz ref -20.7 / sim -27.2. Model excess above 4 kHz: none > 3 dB.
- **2C-5** worst: k26 1701Hz ref -20.6 / sim -33.0, k22 1439Hz ref -19.2 / sim -25.2, k27 1766Hz ref -21.0 / sim -34.9, k25 1635Hz ref -20.2 / sim -31.1. Model excess above 4 kHz: none > 3 dB.
- **2C-6** worst: k25 1635Hz ref -19.1 / sim -29.8, k30 1962Hz ref -20.6 / sim -38.3, k26 1701Hz ref -19.4 / sim -31.7, k29 1897Hz ref -20.3 / sim -36.8. Model excess above 4 kHz: none > 3 dB.
- **2R-1** worst: k5 327Hz ref -8.9 / sim -5.2, k6 392Hz ref -12.1 / sim -8.7, k44 2878Hz ref -76.4 / sim -79.8, k43 2812Hz ref -75.5 / sim -78.8. Model excess above 4 kHz: none > 3 dB.
- **3A-1** worst: k10 654Hz ref -40.9 / sim -44.7, k11 720Hz ref -43.6 / sim -47.8, k12 785Hz ref -46.1 / sim -50.7, k13 850Hz ref -48.5 / sim -53.4. Model excess above 4 kHz: none > 3 dB.
- **3A-2** worst: k7 458Hz ref -28.0 / sim -31.9, k8 523Hz ref -31.0 / sim -35.6, k9 589Hz ref -33.7 / sim -39.1, k10 654Hz ref -36.4 / sim -42.3. Model excess above 4 kHz: none > 3 dB.
- **3A-3** worst: k8 523Hz ref -25.4 / sim -29.5, k7 458Hz ref -23.1 / sim -26.3, k9 589Hz ref -27.5 / sim -32.5, k10 654Hz ref -29.6 / sim -35.2. Model excess above 4 kHz: none > 3 dB.
- **3A-4** worst: k3 196Hz ref -10.1 / sim -5.2, k2 131Hz ref -5.5 / sim -1.2, k4 262Hz ref -13.3 / sim -9.1, k12 785Hz ref -27.3 / sim -30.4. Model excess above 4 kHz: none > 3 dB.
- **3A-5** worst: k3 196Hz ref -8.3 / sim -5.5, k9 589Hz ref -29.0 / sim -31.8, k10 654Hz ref -32.8 / sim -35.6, k14 916Hz ref -43.8 / sim -47.9. Model excess above 4 kHz: none > 3 dB.
- **3A-6** worst: k3 196Hz ref -7.8 / sim -4.5, k4 262Hz ref -11.6 / sim -8.4, k13 850Hz ref -36.7 / sim -42.5, k5 327Hz ref -14.1 / sim -12.2. Model excess above 4 kHz: none > 3 dB.
- **3A-7** worst: k4 262Hz ref -10.5 / sim -6.9, k3 196Hz ref -7.3 / sim -3.9, k5 327Hz ref -12.5 / sim -9.7, k6 392Hz ref -14.1 / sim -12.6. Model excess above 4 kHz: none > 3 dB.
- **3A-8** worst: k4 262Hz ref -9.9 / sim -6.4, k5 327Hz ref -11.6 / sim -8.3, k3 196Hz ref -7.2 / sim -4.0, k6 392Hz ref -12.8 / sim -10.1. Model excess above 4 kHz: none > 3 dB.
- **3A-9** worst: k31 2028Hz ref -36.2 / sim -44.8, k32 2093Hz ref -37.6 / sim -45.7, k30 1962Hz ref -34.7 / sim -43.9, k33 2158Hz ref -38.9 / sim -46.6. Model excess above 4 kHz: none > 3 dB.
- **3A-10** worst: k12 785Hz ref -29.4 / sim -35.2, k4 262Hz ref -6.0 / sim -2.0, k15 981Hz ref -40.3 / sim -44.3, k16 1046Hz ref -43.4 / sim -46.9. Model excess above 4 kHz: none > 3 dB.
- **3A-11** worst: k4 262Hz ref -6.0 / sim -1.3, k5 327Hz ref -8.5 / sim -4.4, k6 392Hz ref -10.4 / sim -6.7, k19 1243Hz ref -46.7 / sim -51.0. Model excess above 4 kHz: none > 3 dB.
- **3A-12** worst: k4 262Hz ref -6.0 / sim -1.2, k7 458Hz ref -10.5 / sim -5.8, k6 392Hz ref -9.1 / sim -4.8, k5 327Hz ref -7.9 / sim -3.6. Model excess above 4 kHz: none > 3 dB.
- **3A-13** worst: k4 262Hz ref -6.8 / sim -2.7, k5 327Hz ref -8.1 / sim -4.7, k6 392Hz ref -8.8 / sim -5.6, k8 523Hz ref -9.5 / sim -6.7. Model excess above 4 kHz: none > 3 dB.
- **3A-14** worst: k37 2420Hz ref -71.4 / sim -79.8, k36 2355Hz ref -70.4 / sim -78.7, k35 2289Hz ref -69.3 / sim -77.5, k34 2224Hz ref -68.3 / sim -76.3. Model excess above 4 kHz: none > 3 dB.
- **3A-15** worst: k6 392Hz ref -22.5 / sim -10.5, k8 523Hz ref -22.7 / sim -11.2, k10 654Hz ref -22.6 / sim -11.6, k12 785Hz ref -22.5 / sim -12.1. Model excess above 4 kHz: inter-harmonic 4415Hz +8.2dB, inter-harmonic 4284Hz +8.0dB, inter-harmonic 4153Hz +7.9dB, harmonic 4121Hz +6.4dB.
- **3B-1** worst: k9 589Hz ref -46.2 / sim -54.9, k17 1112Hz ref -69.1 / sim -87.6, k15 981Hz ref -63.8 / sim -87.5, k11 720Hz ref -52.8 / sim -65.2. Model excess above 4 kHz: none > 3 dB.
- **3B-2** worst: k24 1570Hz ref -71.6 / sim -79.4, k13 850Hz ref -47.5 / sim -62.6, k15 981Hz ref -52.8 / sim -75.6, k5 327Hz ref -8.8 / sim -14.7. Model excess above 4 kHz: none > 3 dB.
- **3B-3** worst: k31 2028Hz ref -72.0 / sim -58.3, k29 1897Hz ref -69.1 / sim -55.7, k33 2158Hz ref -74.0 / sim -61.0, k6 392Hz ref -15.3 / sim -2.5. Model excess above 4 kHz: none > 3 dB.
- **3B-4** worst: k4 262Hz ref -22.9 / sim -3.7, k6 392Hz ref -24.5 / sim -5.9, k15 981Hz ref -26.3 / sim -43.6, k8 523Hz ref -25.3 / sim -8.4. Model excess above 4 kHz: harmonic 4840Hz +13.0dB, harmonic 4971Hz +12.9dB, harmonic 4709Hz +12.8dB, inter-harmonic 4153Hz +10.0dB.
- **3B-5** worst: k15 981Hz ref -20.7 / sim -44.2, k27 1766Hz ref -15.7 / sim -2.4, k30 1962Hz ref -12.7 / sim -42.0, k87 5690Hz ref -69.0 / sim -57.0. Model excess above 4 kHz: inter-harmonic 4546Hz +14.9dB, inter-harmonic 4415Hz +14.7dB, inter-harmonic 5134Hz +14.5dB, harmonic 5690Hz +12.0dB.
- **3C-1** worst: k6 196Hz ref -10.9 / sim 2.7, k5 164Hz ref -9.3 / sim 4.2, k7 229Hz ref -12.2 / sim 1.3, k8 262Hz ref -13.2 / sim -0.2. Model excess above 4 kHz: harmonic 4121Hz +5.3dB, harmonic 4153Hz +5.3dB, harmonic 4186Hz +5.3dB.
- **3C-2** worst: k2 262Hz ref -5.7 / sim 7.4, k1 131Hz ref 0.0 / sim 13.1, k3 392Hz ref -8.9 / sim 2.0, k4 523Hz ref -11.2 / sim -2.6. Model excess above 4 kHz: harmonic 4971Hz +4.8dB, harmonic 4840Hz +4.8dB, harmonic 5102Hz +4.8dB, inter-harmonic 4121Hz +4.0dB.
- **3C-3** worst: k1 262Hz ref 0.0 / sim 14.4, k2 523Hz ref -5.4 / sim 4.5, k3 785Hz ref -9.5 / sim -2.7, k15 3924Hz ref -50.0 / sim -44.0. Model excess above 4 kHz: harmonic 4186Hz +6.0dB, harmonic 4448Hz +5.9dB, harmonic 4709Hz +5.8dB.
- **3C-4** worst: k5 164Hz ref -27.0 / sim 4.1, k10 327Hz ref -29.0 / sim 0.0, k12 392Hz ref -27.3 / sim -3.0, k17 556Hz ref -31.5 / sim -9.2. Model excess above 4 kHz: harmonic 4906Hz +14.6dB, harmonic 4611Hz +14.6dB, harmonic 4251Hz +14.5dB, inter-harmonic 4039Hz +11.6dB.
- **3C-5** worst: k15 1962Hz ref -54.9 / sim -29.4, k17 2224Hz ref -56.3 / sim -32.9, k13 1701Hz ref -41.1 / sim -25.6, k19 2485Hz ref -51.5 / sim -36.2. Model excess above 4 kHz: harmonic 4186Hz +13.8dB, harmonic 4448Hz +12.8dB, inter-harmonic 4121Hz +12.5dB, harmonic 4709Hz +11.7dB.
- **3C-6** worst: k7 1831Hz ref -42.9 / sim -22.3, k12 3140Hz ref -54.6 / sim -39.0, k14 3663Hz ref -58.4 / sim -42.9, k19 4971Hz ref -78.1 / sim -63.4. Model excess above 4 kHz: harmonic 6802Hz +15.9dB, harmonic 4971Hz +14.7dB, harmonic 6279Hz +13.6dB, inter-harmonic 4317Hz +10.0dB.
- **3D-1** worst: k8 523Hz ref -20.1 / sim -25.3, k9 589Hz ref -22.7 / sim -28.2, k7 458Hz ref -17.5 / sim -22.4, k10 654Hz ref -25.2 / sim -30.9. Model excess above 4 kHz: none > 3 dB.
- **3D-2** worst: k21 1374Hz ref -47.9 / sim -53.4, k23 1504Hz ref -50.9 / sim -56.5, k22 1439Hz ref -49.4 / sim -55.0, k20 1308Hz ref -46.3 / sim -51.7. Model excess above 4 kHz: none > 3 dB.
- **3D-3** worst: k40 2616Hz ref -73.3 / sim -79.3, k39 2551Hz ref -72.4 / sim -78.3, k38 2485Hz ref -71.5 / sim -77.3, k41 2682Hz ref -74.3 / sim -80.4. Model excess above 4 kHz: none > 3 dB.
- **3D-4** worst: k1 65Hz ref 0.0 / sim 3.7, k2 131Hz ref -4.8 / sim -1.2, k29 1897Hz ref -64.0 / sim -67.5, k26 1701Hz ref -60.2 / sim -63.6. Model excess above 4 kHz: none > 3 dB.
- **3D-5** worst: k4 262Hz ref -10.8 / sim -4.9, k3 196Hz ref -8.4 / sim -2.6, k5 327Hz ref -12.6 / sim -7.3, k2 131Hz ref -5.2 / sim -0.4. Model excess above 4 kHz: none > 3 dB.
- **3D-6** worst: k3 196Hz ref -8.2 / sim 0.2, k4 262Hz ref -10.7 / sim -2.4, k2 131Hz ref -4.9 / sim 2.5, k5 327Hz ref -12.4 / sim -5.0. Model excess above 4 kHz: none > 3 dB.
- **3D-7** worst: k3 196Hz ref -8.1 / sim 0.2, k4 262Hz ref -10.5 / sim -2.6, k2 131Hz ref -4.6 / sim 2.8, k5 327Hz ref -12.2 / sim -5.3. Model excess above 4 kHz: none > 3 dB.
- **3D-8** worst: k3 196Hz ref -8.3 / sim 0.0, k4 262Hz ref -10.7 / sim -2.8, k2 131Hz ref -4.9 / sim 2.8, k5 327Hz ref -12.6 / sim -5.6. Model excess above 4 kHz: none > 3 dB.
- **3D-9** worst: k3 196Hz ref -8.6 / sim -0.7, k4 262Hz ref -11.2 / sim -3.6, k2 131Hz ref -5.1 / sim 2.3, k5 327Hz ref -13.4 / sim -6.4. Model excess above 4 kHz: inter-harmonic 4219Hz +3.8dB, inter-harmonic 4284Hz +3.8dB, inter-harmonic 4088Hz +3.8dB, harmonic 4251Hz +3.1dB.
- **3R-1** worst: k5 327Hz ref -8.9 / sim -5.2, k6 392Hz ref -12.3 / sim -8.6, k3 196Hz ref -5.4 / sim -2.3, k2 131Hz ref -4.0 / sim -1.0. Model excess above 4 kHz: none > 3 dB.

## Suspicious samples

- **3A-14**: spectrum is within 1.1 dB of 3R-1 although the cutoff/envMod/decay/accent label differs -- one of the two labels is probably wrong
- **3A-10**: spectrum is within 1.2 dB of 3A-11 although the accent label differs -- one of the two labels is probably wrong
- **2R-1**: spectrum is within 1.5 dB of 3A-14 although the cutoff/envMod/decay/accent label differs -- one of the two labels is probably wrong
- **2C-4**: spectrum is within 1.2 dB of 2C-5 although the decay label differs -- one of the two labels is probably wrong
- **2B-11**: spectrum is within 1.2 dB of 2B-12 although the envMod label differs -- one of the two labels is probably wrong
- **2B-3**: spectrum is within 1.5 dB of 2B-4 although the envMod label differs -- one of the two labels is probably wrong
- **2A-8**: spectrum is within 1.2 dB of 3D-3 although the resonance/accent label differs -- one of the two labels is probably wrong
- **2A-6**: spectrum is within 1.0 dB of 2B-7 although the cutoff/decay label differs -- one of the two labels is probably wrong
- **1R-1**: spectrum is within 1.2 dB of 3A-14 although the cutoff/envMod/decay/accent label differs -- one of the two labels is probably wrong
- **1D-2**: spectrum is within 0.3 dB of 3D-9 although the cutoff/envMod/decay label differs -- one of the two labels is probably wrong
- **1D-1**: spectrum is within 1.0 dB of 1E-2 although the cutoff/envMod/decay/accent label differs -- one of the two labels is probably wrong
- **1A-5**: spectrum is within 1.1 dB of 3A-14 although the cutoff/envMod/decay/accent label differs -- one of the two labels is probably wrong
- **1A-4**: spectrum is within 1.2 dB of 1D-1 although the cutoff/envMod/decay/accent label differs -- one of the two labels is probably wrong
- **1A-3**: spectrum is within 1.4 dB of 2B-7 although the resonance/decay label differs -- one of the two labels is probably wrong
- **3B-5**: error 6.04 is 2.3x the median
- **3A-15**: error 5.53 is 2.1x the median
- **3B-3**: error 5.22 is 2.0x the median
- **2B-14**: error 5.08 is 2.0x the median
- **1B-5**: error 4.71 is 1.8x the median
- **1D-3**: error 4.67 is 1.8x the median
- **1D-4**: error 4.38 is 1.7x the median
- **3C-1**: error 4.34 is 1.7x the median
- **3D-5**: error 4.22 is 1.6x the median
- **3C-4**: error 4.14 is 1.6x the median

## Fitted knob positions

| knob (label)     | nominal | fitted | sensitivity |
|------------------|---------|--------|-------------|
| accent (x0)      | 0.00    | 0.000  | 0.000       |
| accent (x100)    | 1.00    | 1.000  | 0.000       |
| accent (x25)     | 0.25    | 0.252  | 0.000       |
| accent (x50)     | 0.50    | 0.504  | 0.000       |
| accent (x74)     | 0.75    | 0.748  | 0.000       |
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

| parameter              | before    | after     | sensitivity |                                     |
|------------------------|-----------|-----------|-------------|-------------------------------------|
| oscCouplingHz          | 47.24     | 47.24     | 0.000       |                                     |
| oscSawLpfHz            | 40000     | 40000     | 0.000       | AT BOUND (model may lack structure) |
| oscSawShape            | 0         | 0         | 0.000       |                                     |
| oscSquareDutyDepth     | 0.12      | 0.12      | 0.000       |                                     |
| oscSquareLevel         | 0.70553   | 0.70553   | 0.000       |                                     |
| filterFeedbackGain     | 17.42     | 17.42     | 0.000       |                                     |
| filterResonanceLimit   | 0.91245   | 0.91245   | 0.000       |                                     |
| filterResonanceSkew    | -0.8718   | -0.8718   | 0.000       |                                     |
| resCouplingHz          | 104.29    | 104.29    | 0.000       |                                     |
| filterCapScale1        | 1.2895    | 1.2895    | 0.000       |                                     |
| filterCapScale2        | 0.69764   | 0.69764   | 0.000       |                                     |
| filterCapScale3        | 0.91275   | 0.91275   | 0.000       |                                     |
| filterCapScale4        | 1.0631    | 1.0631    | 0.000       |                                     |
| filterLadderInputScale | 0.035224  | 0.035224  | 0.000       |                                     |
| filterInputCouplingHz  | 6.0165    | 6.0165    | 0.000       |                                     |
| filterOutputCouplingHz | 20000     | 20000     | 0.000       |                                     |
| filterPostHpHz         | 198.89    | 198.89    | 0.000       |                                     |
| filterNotchHz          | 7.5164    | 7.5164    | 0.000       |                                     |
| filterNotchBandwidthHz | 4.7       | 4.7       | 0.000       |                                     |
| filterAllpassHz        | 14.008    | 14.008    | 0.000       |                                     |
| cutoffBaseHz           | 212.18    | 212.18    | 0.000       |                                     |
| cutoffSpanOct          | 3.3213    | 3.3213    | 0.000       |                                     |
| cutoffTaperExp         | 1.4289    | 1.4289    | 0.000       |                                     |
| cutoffMaxHz            | 23723     | 23723     | 0.000       |                                     |
| envModScaleC0          | 0.76145   | 0.76145   | 0.000       |                                     |
| envModScaleC0Slope     | 2.8142    | 2.8142    | 0.000       |                                     |
| envModScaleC1          | 0.77794   | 0.77794   | 0.000       |                                     |
| envModScaleC1Slope     | 4.4796    | 4.4796    | 0.000       |                                     |
| envModOffset           | 0.34908   | 0.34908   | 0.000       |                                     |
| envModOffsetCutSlope   | -0.023862 | -0.023862 | 0.000       |                                     |
| envModTaperExp         | 2         | 2         | 0.000       |                                     |
| envModTaperMid         | 0.68612   | 0.68612   | 0.000       |                                     |
| envModTaperWidth       | 0.12193   | 0.12193   | 0.000       |                                     |
| accentSweepDepthOct    | 7.9269    | 7.9269    | 0.000       |                                     |
| accentVcaDepth         | 2.2843    | 2.2843    | 0.000       |                                     |
| accentChargeBaseSec    | 0.030579  | 0.030579  | 0.000       |                                     |
| accentChargePotSec     | 0.044021  | 0.044021  | 0.000       |                                     |
| accentMixSec           | 0.14498   | 0.14498   | 0.000       |                                     |
| accentDiodeDrop        | 0.30033   | 0.30033   | 0.000       |                                     |
| vcfAttackMs            | 0.1       | 0.1       | 0.000       |                                     |
| vcaAttackMs            | 1.3011    | 1.3011    | 0.000       |                                     |
| vcaNormalDelayMs       | 4.504     | 4.504     | 0.000       |                                     |
| vcfDecayMinSec         | 0.056061  | 0.056061  | 0.000       |                                     |
| vcfDecayMaxSec         | 1.1404    | 1.1404    | 0.000       |                                     |
| vcfDecayTaper          | 17.982    | 17.982    | 0.000       |                                     |
| accentDecaySec         | 0.075588  | 0.075588  | 0.000       |                                     |
| vegDecaySec            | 1.0379    | 1.0379    | 0.000       |                                     |
| vcaGateOffMs           | 0.81135   | 0.81135   | 0.000       |                                     |
| vcaGateOffAccentMs     | 2.8037    | 2.8037    | 0.000       |                                     |
| vcaResTapRatio         | 1.0503    | 1.0503    | 0.000       |                                     |
| vcaGainSaturationDrive | 0         | 0         | 0.000       | AT BOUND (model may lack structure) |

Timing: note-on offset 0.00 ms (relative to each clip's measured note-on), gate length 1300.0 ms.

## Applying the result

Paste `synth_parameters.txt` into `acidus::SynthParameters` (src/core/SynthEngine.hpp) or rerun with `--apply`.

Audio: `renders/` (hardware / before / after for the 40 worst samples).
