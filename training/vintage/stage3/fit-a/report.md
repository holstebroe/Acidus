# Acidus vs. hardware TB-303 calibration report (20261010-150807)

97 reference samples, 21 free parameters. Search: 267 evaluations in 31.5 min, 0 CMA-ES restarts, stopped because: time cap reached.

## Match quality (mean over samples)

All errors are dB; lower is better. `harm` = full-note harmonic levels (k^-0.5 weighted RMS), `inter` = energy between harmonics, `env` = RMS envelope, `stft` = 1/3-octave levels over time, `peak` = shape (height+width+implicit frequency) of the resonant-peak window on Resonance=max samples only -- weighted more heavily than `harm` so it can't be traded away.

| metric                                         | before | after |
|------------------------------------------------|--------|-------|
| objective (incl. priors)                       | 1.98   | 1.93  |
| weighted error                                 | 1.98   | 1.93  |
| harmonic error (dB)                            | 1.98   | 2.08  |
| resonant-peak shape error (dB)                 | 1.21   | 1.03  |
| inter-harmonic error (dB)                      | 0.75   | 0.97  |
| envelope error (dB)                            | 4.46   | 4.17  |
| spectrogram error (dB)                         | 7.45   | 7.26  |
| aligned waveform error (10*sqrt(residual/ref)) | 0.00   | 0.00  |
| resonant-peak sweep track error (semitones)    | 2.11   | 2.13  |
| harmonics-over-time error (dB)                 | 5.80   | 5.48  |
| mean |harmonic error| (dB)                     | 2.20   | 2.30  |
| note level error, RMS over notes (dB)          | 3.56   | 3.50  |
| harmonics within 1 dB (%)                      | 33.30  | 32.43 |
| harmonics within 3 dB (%)                      | 73.46  | 72.78 |
| harmonics within 6 dB (%)                      | 95.33  | 95.32 |

Recording gain solved as -11.3 dB (before: -11.8 dB).

## Per set

Mean error of each set's notes, and the RMS of their level errors (the recordings are not normalised, so absolute note levels are compared under one global gain).

| set | notes | before | after | level before (dB) | level after (dB) | harm | env | stft |
|-----|-------|--------|-------|-------------------|------------------|------|-----|------|
| 1A  | 9     | 1.89   | 1.90  | 4.0               | 4.1              | 2.1  | 4.0 | 6.8  |
| 1B  | 5     | 2.04   | 2.13  | 2.4               | 2.8              | 1.6  | 4.2 | 7.2  |
| 1C  | 3     | 1.63   | 1.82  | 1.8               | 1.7              | 1.9  | 4.1 | 6.1  |
| 1D  | 4     | 1.96   | 1.86  | 2.7               | 2.0              | 1.4  | 2.4 | 6.3  |
| 1E  | 2     | 1.12   | 1.04  | 4.7               | 3.9              | 2.3  | 3.3 | 5.0  |
| 1R  | 1     | 1.83   | 1.87  | 3.1               | 3.1              | 1.3  | 3.7 | 7.7  |
| 2A  | 16    | 1.98   | 1.97  | 3.5               | 3.5              | 1.6  | 4.3 | 7.1  |
| 2B  | 14    | 1.89   | 1.95  | 2.1               | 2.7              | 2.0  | 2.9 | 6.6  |
| 2C  | 6     | 1.60   | 1.47  | 1.4               | 1.4              | 1.2  | 4.0 | 5.9  |
| 2R  | 1     | 1.59   | 1.53  | 3.1               | 3.1              | 1.4  | 3.5 | 7.5  |
| 3A  | 15    | 1.94   | 1.86  | 2.8               | 2.1              | 1.7  | 2.2 | 6.4  |
| 3B  | 5     | 2.86   | 2.39  | 3.3               | 3.8              | 3.3  | 3.9 | 5.2  |
| 3C  | 6     | 2.23   | 2.26  | 7.1               | 6.9              | 4.9  | 6.6 | 11.0 |
| 3D  | 9     | 2.30   | 2.13  | 4.7               | 4.5              | 2.6  | 9.1 | 11.5 |
| 3R  | 1     | 1.18   | 1.15  | 3.2               | 3.1              | 1.3  | 4.3 | 5.6  |

## Per sample (sorted by remaining error, worst first)

| sample | before | after | harm | peak | inter | env  | stft | wave | harm ±3dB | level before | level after | tuning (c) | drift (c) |
|--------|--------|-------|------|------|-------|------|------|------|-----------|--------------|-------------|------------|-----------|
| 3A-13  | 4.84   | 4.40  | 3.3  | 5.2  | 3.6   | 1.4  | 7.3  | 0.0  | 16%       | +1.6         | +1.2        | +0.0       | 0.00      |
| 3B-3   | 3.98   | 4.12  | 2.5  | 3.9  | 0.9   | 3.8  | 5.7  | 0.0  | 42%       | -2.7         | -3.3        | +0.0       | 0.00      |
| 3D-5   | 4.06   | 3.71  | 1.9  | -    | 1.3   | 18.7 | 23.2 | 0.0  | 66%       | -0.2         | +2.1        | +0.0       | 0.00      |
| 3C-1   | 3.70   | 3.67  | 3.2  | -    | 1.1   | 7.8  | 31.1 | 0.0  | 87%       | +7.6         | +7.6        | +0.0       | 0.00      |
| 3A-15  | 3.35   | 3.45  | 2.9  | 1.7  | 0.6   | 1.9  | 4.0  | 0.0  | 56%       | -1.5         | -2.2        | +0.0       | 0.00      |
| 1B-5   | 2.93   | 3.32  | 3.5  | 0.5  | 1.7   | 5.6  | 6.9  | 0.0  | 36%       | +3.9         | +5.0        | +0.0       | 0.00      |
| 1D-4   | 3.51   | 3.21  | 1.0  | 2.9  | 2.1   | 1.4  | 7.3  | 0.0  | 96%       | -0.3         | -0.6        | +0.0       | 0.00      |
| 2A-14  | 3.17   | 3.13  | 1.4  | 4.2  | 0.7   | 2.3  | 6.4  | 0.0  | 93%       | +1.7         | +2.3        | +0.0       | 0.00      |
| 2B-14  | 2.69   | 3.05  | 4.3  | 0.4  | 2.0   | 4.5  | 8.2  | 0.0  | 51%       | +3.8         | +4.9        | +0.0       | 0.00      |
| 2A-7   | 3.09   | 3.04  | 1.3  | 2.2  | 0.2   | 1.7  | 7.7  | 0.0  | 59%       | +0.2         | +0.0        | +0.0       | 0.00      |
| 3A-12  | 3.14   | 3.03  | 2.6  | 1.2  | 2.9   | 1.7  | 8.1  | 0.0  | 40%       | +1.9         | +1.4        | +0.0       | 0.00      |
| 3B-5   | 5.44   | 2.97  | 4.5  | 3.8  | 4.4   | 1.9  | 5.6  | 0.0  | 30%       | +0.3         | +0.7        | +0.0       | 0.00      |
| 3D-6   | 3.35   | 2.95  | 2.1  | -    | 0.0   | 14.0 | 18.7 | 0.0  | 73%       | +3.0         | +3.3        | +0.0       | 0.00      |
| 2A-2   | 2.97   | 2.89  | 0.7  | 1.7  | 0.0   | 1.6  | 8.6  | 0.0  | 94%       | +0.4         | +0.0        | +0.0       | 0.00      |
| 2A-6   | 2.89   | 2.87  | 1.1  | 1.1  | 0.5   | 7.1  | 8.4  | 0.0  | 97%       | +1.8         | +1.5        | +0.0       | 0.00      |
| 1A-9   | 2.56   | 2.83  | 3.7  | 3.7  | 2.9   | 3.6  | 5.0  | 0.0  | 13%       | +2.5         | +3.7        | +0.0       | 0.00      |
| 3A-11  | 2.83   | 2.79  | 2.1  | 1.0  | 3.0   | 1.7  | 7.4  | 0.0  | 84%       | +1.4         | +0.9        | +0.0       | 0.00      |
| 2A-5   | 2.82   | 2.76  | 1.0  | 0.9  | 0.0   | 2.6  | 8.0  | 0.0  | 97%       | +2.3         | +2.0        | +0.0       | 0.00      |
| 2A-9   | 2.73   | 2.73  | 1.1  | 0.9  | 0.0   | 2.8  | 8.1  | 0.0  | 96%       | +2.3         | +2.4        | +0.0       | 0.00      |
| 3A-10  | 2.60   | 2.67  | 1.5  | 1.1  | 2.2   | 1.6  | 6.9  | 0.0  | 89%       | +0.4         | +0.0        | +0.0       | 0.00      |
| 1A-3   | 2.69   | 2.62  | 2.0  | 1.2  | 0.0   | 1.8  | 7.1  | 0.0  | 41%       | -0.9         | -1.3        | +0.0       | 0.00      |
| 2B-7   | 2.63   | 2.59  | 0.7  | 1.0  | 0.3   | 1.4  | 8.2  | 0.0  | 100%      | +0.0         | -0.3        | +0.0       | 0.00      |
| 1A-6   | 2.57   | 2.57  | 1.1  | 2.1  | 0.4   | 2.3  | 7.7  | 0.0  | 96%       | +1.2         | +1.4        | +0.0       | 0.00      |
| 3D-1   | 2.76   | 2.56  | 3.0  | -    | 3.3   | 10.5 | 13.8 | 0.0  | 40%       | +3.4         | +5.1        | +0.0       | 0.00      |
| 3C-5   | 2.00   | 2.44  | 8.6  | -    | 1.4   | 5.5  | 6.9  | 0.0  | 44%       | +5.9         | +5.3        | +0.0       | 0.00      |
| 2B-8   | 2.41   | 2.34  | 0.7  | 1.0  | 0.6   | 1.4  | 6.9  | 0.0  | 100%      | -0.1         | -0.5        | +0.0       | 0.00      |
| 1C-3   | 1.90   | 2.25  | 2.6  | 0.3  | 0.0   | 2.3  | 7.6  | 0.0  | 65%       | -2.2         | -1.6        | +0.0       | 0.00      |
| 3B-2   | 2.18   | 2.24  | 2.6  | 2.7  | 0.0   | 4.6  | 5.1  | 0.0  | 24%       | -4.3         | -5.3        | +0.0       | 0.00      |
| 1B-4   | 2.11   | 2.23  | 1.5  | 0.6  | 1.5   | 8.0  | 8.1  | 0.0  | 90%       | -2.5         | -2.5        | +0.0       | 0.00      |
| 2B-13  | 1.93   | 2.21  | 4.2  | 0.4  | 2.2   | 6.9  | 5.0  | 0.0  | 28%       | +3.6         | +4.7        | +0.0       | 0.00      |
| 3D-3   | 1.90   | 2.18  | 4.4  | -    | 7.2   | 7.2  | 7.6  | 0.0  | 47%       | +6.0         | +5.4        | +0.0       | 0.00      |
| 1B-1   | 2.18   | 2.15  | 1.4  | 0.6  | 0.0   | 3.0  | 7.5  | 0.0  | 98%       | +1.9         | +2.1        | +0.0       | 0.00      |
| 2B-6   | 2.06   | 2.13  | 1.0  | 0.8  | 0.8   | 2.1  | 6.8  | 0.0  | 100%      | +1.2         | +1.4        | +0.0       | 0.00      |
| 1D-3   | 2.08   | 2.06  | 0.8  | 1.5  | 1.3   | 1.4  | 5.2  | 0.0  | 100%      | -0.2         | -0.3        | +0.0       | 0.00      |
| 2B-1   | 2.07   | 2.05  | 1.0  | 0.5  | 0.0   | 3.0  | 8.4  | 0.0  | 100%      | +1.8         | +2.0        | +0.0       | 0.00      |
| 2B-10  | 2.05   | 2.04  | 3.0  | 0.5  | 3.3   | 2.7  | 6.7  | 0.0  | 25%       | -1.2         | -1.4        | +0.0       | 0.00      |
| 1A-7   | 1.96   | 1.97  | 3.4  | -    | 1.3   | 8.9  | 8.4  | 0.0  | 92%       | +8.6         | +8.7        | +0.0       | 0.00      |
| 3C-6   | 2.05   | 1.96  | 6.6  | -    | 1.6   | 3.8  | 6.8  | 0.0  | 27%       | +4.1         | +3.5        | +0.0       | 0.00      |
| 1C-1   | 1.72   | 1.94  | 1.9  | 0.4  | 2.7   | 5.0  | 7.3  | 0.0  | 80%       | -2.0         | -2.4        | +0.0       | 0.00      |
| 3C-4   | 2.07   | 1.93  | 5.1  | -    | 2.7   | 4.7  | 8.1  | 0.0  | 46%       | +5.0         | +4.3        | +0.0       | 0.00      |
| 3D-2   | 2.31   | 1.93  | 2.8  | -    | 1.0   | 8.8  | 9.1  | 0.0  | 18%       | +5.4         | +5.3        | +0.0       | 0.00      |
| 3C-2   | 1.88   | 1.87  | 3.2  | -    | 2.8   | 9.6  | 6.1  | 0.0  | 93%       | +9.4         | +9.5        | +0.0       | 0.00      |
| 1R-1   | 1.83   | 1.87  | 1.3  | 0.5  | 0.0   | 3.7  | 7.7  | 0.0  | 91%       | +3.1         | +3.1        | +0.0       | 0.00      |
| 1A-8   | 1.67   | 1.85  | 2.2  | 0.4  | 1.3   | 4.4  | 6.0  | 0.0  | 88%       | +3.8         | +4.7        | +0.0       | 0.00      |
| 2A-16  | 1.59   | 1.82  | 2.8  | 0.4  | 0.9   | 4.7  | 6.8  | 0.0  | 84%       | +3.5         | +4.5        | +0.0       | 0.00      |
| 2A-13  | 1.71   | 1.78  | 1.3  | 0.4  | 0.2   | 2.9  | 7.5  | 0.0  | 93%       | +2.3         | +2.8        | +0.0       | 0.00      |
| 2B-5   | 1.74   | 1.77  | 1.6  | 0.5  | 1.6   | 2.1  | 6.8  | 0.0  | 93%       | +1.0         | +1.2        | +0.0       | 0.00      |
| 2A-12  | 1.67   | 1.75  | 2.0  | 0.4  | 0.4   | 4.1  | 7.3  | 0.0  | 92%       | +3.4         | +3.7        | +0.0       | 0.00      |
| 2B-11  | 1.49   | 1.74  | 3.4  | 0.4  | 1.1   | 4.2  | 7.2  | 0.0  | 24%       | +3.4         | +4.5        | +0.0       | 0.00      |
| 2C-1   | 1.70   | 1.74  | 1.2  | 0.4  | 0.3   | 4.0  | 7.9  | 0.0  | 69%       | -2.1         | -2.4        | +0.0       | 0.00      |
| 3C-3   | 1.70   | 1.69  | 2.9  | -    | 0.0   | 8.4  | 6.8  | 0.0  | 92%       | +8.8         | +8.8        | +0.0       | 0.00      |
| 3D-8   | 1.67   | 1.68  | 2.8  | -    | 0.1   | 7.4  | 7.9  | 0.0  | 52%       | +5.0         | +4.2        | +0.0       | 0.00      |
| 3D-7   | 1.95   | 1.68  | 2.4  | -    | 0.0   | 5.7  | 10.5 | 0.0  | 71%       | +4.2         | +3.8        | +0.0       | 0.00      |
| 2A-10  | 1.69   | 1.63  | 2.6  | -    | 1.3   | 9.3  | 5.2  | 0.0  | 94%       | +8.0         | +7.6        | +0.0       | 0.00      |
| 2B-4   | 1.64   | 1.62  | 2.2  | 0.4  | 2.1   | 2.2  | 6.0  | 0.0  | 39%       | +1.2         | +1.4        | +0.0       | 0.00      |
| 2B-12  | 1.31   | 1.61  | 3.4  | 0.4  | 2.3   | 4.1  | 5.3  | 0.0  | 27%       | +3.4         | +4.5        | +0.0       | 0.00      |
| 2C-2   | 1.52   | 1.61  | 1.0  | 0.3  | 2.3   | 5.4  | 5.9  | 0.0  | 96%       | -1.5         | -1.8        | +0.0       | 0.00      |
| 2B-2   | 1.62   | 1.60  | 1.0  | 0.2  | 0.7   | 2.5  | 6.4  | 0.0  | 100%      | +1.5         | +1.6        | +0.0       | 0.00      |
| 1A-2   | 1.71   | 1.55  | 1.1  | 0.9  | 0.1   | 1.6  | 8.7  | 0.0  | 79%       | +0.8         | +0.3        | +0.0       | 0.00      |
| 3A-9   | 1.47   | 1.54  | 2.5  | 0.4  | 0.1   | 3.6  | 4.2  | 0.0  | 43%       | +1.4         | +0.9        | +0.0       | 0.00      |
| 2R-1   | 1.59   | 1.53  | 1.4  | 0.5  | 0.0   | 3.5  | 7.5  | 0.0  | 92%       | +3.1         | +3.1        | +0.0       | 0.00      |
| 2C-5   | 1.83   | 1.52  | 1.2  | 0.3  | 0.7   | 3.3  | 6.0  | 0.0  | 93%       | -0.8         | -0.5        | +0.0       | 0.00      |
| 1B-2   | 1.60   | 1.51  | 0.9  | 0.3  | 0.2   | 2.9  | 7.6  | 0.0  | 100%      | +1.6         | +1.7        | +0.0       | 0.00      |
| 3B-4   | 1.68   | 1.51  | 4.5  | -    | 1.1   | 4.5  | 4.7  | 0.0  | 42%       | +4.8         | +4.1        | +0.0       | 0.00      |
| 1B-3   | 1.40   | 1.43  | 0.8  | 0.3  | 0.0   | 1.8  | 6.2  | 0.0  | 100%      | +0.6         | +0.8        | +0.0       | 0.00      |
| 1A-5   | 1.46   | 1.43  | 1.3  | 0.5  | 0.0   | 4.7  | 6.1  | 0.0  | 91%       | +3.2         | +3.1        | +0.0       | 0.00      |
| 2A-15  | 1.38   | 1.42  | 2.4  | -    | 1.5   | 7.7  | 4.4  | 0.0  | 90%       | +4.9         | +5.4        | +0.0       | 0.00      |
| 2C-3   | 1.31   | 1.39  | 1.1  | 0.3  | 0.0   | 4.8  | 5.9  | 0.0  | 95%       | -1.2         | -1.3        | +0.0       | 0.00      |
| 2C-4   | 1.54   | 1.39  | 1.0  | 0.3  | 0.2   | 4.1  | 6.6  | 0.0  | 97%       | -0.8         | -0.7        | +0.0       | 0.00      |
| 3D-4   | 1.53   | 1.38  | 1.9  | -    | 1.8   | 5.3  | 7.3  | 0.0  | 91%       | +6.2         | +5.5        | +0.0       | 0.00      |
| 3A-7   | 1.48   | 1.37  | 0.9  | 0.8  | 0.2   | 2.2  | 7.7  | 0.0  | 98%       | +3.0         | +2.4        | +0.0       | 0.00      |
| 1A-4   | 1.38   | 1.32  | 2.1  | -    | 0.0   | 5.5  | 6.8  | 0.0  | 48%       | +6.1         | +5.4        | +0.0       | 0.00      |
| 3A-6   | 1.49   | 1.32  | 1.1  | 0.7  | 0.0   | 2.4  | 8.0  | 0.0  | 98%       | +2.8         | +2.2        | +0.0       | 0.00      |
| 1C-2   | 1.27   | 1.28  | 1.1  | 0.3  | 2.0   | 4.9  | 3.4  | 0.0  | 100%      | -0.8         | -0.8        | +0.0       | 0.00      |
| 2B-9   | 1.40   | 1.28  | 0.7  | 0.7  | 0.3   | 1.4  | 6.3  | 0.0  | 97%       | -0.4         | -0.7        | +0.0       | 0.00      |
| 2B-3   | 1.35   | 1.27  | 1.4  | 0.4  | 2.3   | 2.7  | 3.7  | 0.0  | 99%       | +1.3         | +1.4        | +0.0       | 0.00      |
| 2A-1   | 1.32   | 1.25  | 1.3  | -    | 0.0   | 7.3  | 5.8  | 0.0  | 68%       | +1.2         | +0.5        | +0.0       | 0.00      |
| 3A-5   | 1.32   | 1.25  | 1.1  | 0.7  | 0.4   | 2.4  | 7.6  | 0.0  | 92%       | +2.1         | +1.6        | +0.0       | 0.00      |
| 3A-8   | 1.26   | 1.25  | 0.9  | 0.7  | 0.5   | 2.0  | 6.1  | 0.0  | 100%      | +2.5         | +1.9        | +0.0       | 0.00      |
| 3A-14  | 1.38   | 1.25  | 0.5  | 0.7  | 1.1   | 1.8  | 5.6  | 0.0  | 100%      | +1.7         | +1.4        | +0.0       | 0.00      |
| 2A-11  | 1.24   | 1.23  | 1.9  | -    | 0.1   | 4.8  | 6.8  | 0.0  | 93%       | +4.4         | +4.5        | +0.0       | 0.00      |
| 1E-2   | 1.17   | 1.18  | 2.1  | -    | 0.0   | 4.4  | 6.3  | 0.0  | 49%       | +6.0         | +5.3        | +0.0       | 0.00      |
| 2A-8   | 1.20   | 1.17  | 1.6  | -    | 0.0   | 3.8  | 7.6  | 0.0  | 74%       | +4.1         | +3.8        | +0.0       | 0.00      |
| 2C-6   | 1.67   | 1.17  | 1.7  | 0.2  | 0.6   | 2.8  | 3.0  | 0.0  | 83%       | -1.4         | -0.9        | +0.0       | 0.00      |
| 3R-1   | 1.18   | 1.15  | 1.3  | 0.5  | 0.0   | 4.3  | 5.6  | 0.0  | 91%       | +3.2         | +3.1        | +0.0       | 0.00      |
| 3B-1   | 1.05   | 1.14  | 2.4  | -    | 0.0   | 4.5  | 5.0  | 0.0  | 36%       | -2.1         | -3.9        | +0.0       | 0.00      |
| 3D-9   | 1.17   | 1.10  | 2.2  | -    | 0.0   | 4.0  | 5.4  | 0.0  | 77%       | +5.4         | +4.7        | +0.0       | 0.00      |
| 2A-4   | 1.15   | 1.09  | 1.3  | -    | 0.0   | 3.1  | 7.9  | 0.0  | 69%       | +2.8         | +2.3        | +0.0       | 0.00      |
| 1D-2   | 1.11   | 1.08  | 1.7  | -    | 0.0   | 4.2  | 6.0  | 0.0  | 67%       | +3.8         | +2.8        | +0.0       | 0.00      |
| 1D-1   | 1.13   | 1.08  | 2.1  | -    | 0.0   | 2.5  | 6.7  | 0.0  | 24%       | +3.9         | +2.9        | +0.0       | 0.00      |
| 2A-3   | 1.12   | 1.03  | 1.5  | -    | 0.0   | 2.9  | 6.8  | 0.0  | 59%       | +3.6         | +2.6        | +0.0       | 0.00      |
| 1A-1   | 1.05   | 1.00  | 1.8  | -    | 0.0   | 3.1  | 5.9  | 0.0  | 44%       | +1.3         | +0.1        | +0.0       | 0.00      |
| 3A-2   | 1.04   | 0.93  | 1.4  | -    | 0.0   | 2.5  | 6.4  | 0.0  | 95%       | +4.3         | +3.0        | +0.0       | 0.00      |
| 3A-4   | 0.95   | 0.92  | 1.6  | -    | 0.3   | 3.2  | 4.9  | 0.0  | 73%       | +5.1         | +3.9        | +0.0       | 0.00      |
| 3A-3   | 1.01   | 0.90  | 1.3  | -    | 0.2   | 2.2  | 6.3  | 0.0  | 92%       | +5.0         | +3.6        | +0.0       | 0.00      |
| 1E-1   | 1.07   | 0.90  | 2.5  | -    | 0.3   | 2.2  | 3.8  | 0.0  | 55%       | +3.0         | +1.6        | +0.0       | 0.00      |
| 3A-1   | 0.90   | 0.81  | 1.1  | -    | 0.1   | 2.2  | 5.7  | 0.0  | 97%       | +3.2         | +2.0        | +0.0       | 0.00      |

### Resonant peak (Resonance=max samples)

| sample | peak Hz (hw) | peak Hz (before) | peak Hz (after) | height dB (hw) | height dB (before) | height dB (after) |
|--------|--------------|------------------|-----------------|----------------|--------------------|-------------------|
| 1A-2   | 131          | 65               | 65              | -1.5           | +0.0               | +0.0              |
| 1A-3   | 196          | 196              | 196             | -0.1           | -0.8               | -0.8              |
| 1A-5   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1A-6   | 327          | 392              | 15632           | -1.9           | -2.8               | -134.6            |
| 1A-8   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1A-9   | 1701         | 1766             | 1766            | -3.9           | -7.6               | -7.5              |
| 1B-1   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1B-2   | 65           | 14782            | 65              | +0.0           | -138.5             | +0.0              |
| 1B-3   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1B-4   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1B-5   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1C-1   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1C-2   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1C-3   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1D-3   | 589          | 13474            | 654             | -10.3          | -142.0             | -12.0             |
| 1D-4   | 981          | 14978            | 981             | -6.4           | -131.1             | -10.2             |
| 1R-1   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 2A-2   | 131          | 196              | 196             | +0.5           | -3.5               | -3.5              |
| 2A-5   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 2A-6   | 196          | 65               | 65              | -2.3           | +0.0               | +0.0              |
| 2A-7   | 196          | 262              | 262             | +1.3           | -1.1               | -1.0              |
| 2A-9   | 65           | 392              | 392             | +0.0           | -6.6               | -6.6              |
| 2A-12  | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 2A-13  | 65           | 720              | 720             | +0.0           | -10.1              | -10.0             |
| 2A-14  | 1439         | 785              | 785             | -18.3          | -3.7               | -3.6              |
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
| 2B-10  | 65           | 14782            | 65              | +0.0           | -135.9             | +0.0              |
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
| 3A-6   | 65           | 9418             | 10858           | +0.0           | -141.8             | -143.4            |
| 3A-7   | 65           | 9811             | 11642           | +0.0           | -133.3             | -144.7            |
| 3A-8   | 65           | 12623            | 12558           | +0.0           | -136.0             | -141.7            |
| 3A-9   | 65           | 15436            | 15370           | +0.0           | -136.1             | -145.0            |
| 3A-10  | 196          | 10465            | 196             | -1.5           | -138.3             | -1.6              |
| 3A-11  | 196          | 10465            | 196             | -1.6           | -136.5             | -2.1              |
| 3A-12  | 196          | 11969            | 65              | -2.1           | -135.1             | +0.0              |
| 3A-13  | 720          | 14978            | 850             | -11.7          | -141.3             | -11.2             |
| 3A-14  | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 3A-15  | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 3B-2   | 196          | 196              | 196             | +0.1           | -1.0               | -0.9              |
| 3B-3   | 327          | 458              | 13016           | -1.7           | -6.5               | -140.6            |
| 3B-5   | 1701         | 1766             | 1701            | -7.3           | -7.2               | -9.0              |
| 3R-1   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |

Height is the resonant peak's level above the fundamental (gain-independent). This is the number that answers "is the acid squelch there": before/after collapsing to something much smaller than hardware's means the peak got flattened away, not just shifted.

`tuning` is measured from the harmonic peaks relative to equal temperament; `drift` is the std-dev of short-time pitch over the note (hardware fluctuation).

### Worst remaining harmonics / spurious high-frequency content

- **1A-1** worst: k21 1374Hz ref -72.7 / sim -79.5, k17 1112Hz ref -65.4 / sim -71.4, k20 1308Hz ref -71.0 / sim -77.7, k18 1177Hz ref -67.5 / sim -73.6. Model excess above 4 kHz: none > 3 dB.
- **1A-2** worst: k21 1374Hz ref -68.4 / sim -71.7, k19 1243Hz ref -64.8 / sim -68.1, k22 1439Hz ref -70.1 / sim -73.3, k18 1177Hz ref -62.8 / sim -66.0. Model excess above 4 kHz: none > 3 dB.
- **1A-3** worst: k23 1504Hz ref -65.8 / sim -71.8, k26 1701Hz ref -70.5 / sim -76.2, k20 1308Hz ref -60.5 / sim -66.5, k15 981Hz ref -49.8 / sim -55.4. Model excess above 4 kHz: none > 3 dB.
- **1A-4** worst: k1 65Hz ref 0.0 / sim 6.2, k37 2420Hz ref -75.2 / sim -79.8, k36 2355Hz ref -74.1 / sim -78.7, k34 2224Hz ref -72.1 / sim -76.6. Model excess above 4 kHz: none > 3 dB.
- **1A-5** worst: k6 392Hz ref -12.3 / sim -8.7, k5 327Hz ref -8.9 / sim -5.3, k2 131Hz ref -4.0 / sim -0.6, k1 65Hz ref 0.0 / sim 3.3. Model excess above 4 kHz: none > 3 dB.
- **1A-6** worst: k7 458Hz ref -7.6 / sim -3.0, k6 392Hz ref -4.6 / sim -1.3, k5 327Hz ref -1.9 / sim -4.5, k14 916Hz ref -31.2 / sim -28.6. Model excess above 4 kHz: none > 3 dB.
- **1A-7** worst: k2 131Hz ref -4.9 / sim 5.5, k3 196Hz ref -8.4 / sim 1.6, k4 262Hz ref -10.7 / sim -1.9, k1 65Hz ref 0.0 / sim 8.3. Model excess above 4 kHz: none > 3 dB.
- **1A-8** worst: k3 196Hz ref -8.2 / sim -2.6, k4 262Hz ref -10.4 / sim -4.9, k2 131Hz ref -4.9 / sim 0.5, k5 327Hz ref -11.9 / sim -6.6. Model excess above 4 kHz: none > 3 dB.
- **1A-9** worst: k87 5690Hz ref -62.2 / sim -56.0, k88 5756Hz ref -62.4 / sim -56.3, k99 6475Hz ref -67.0 / sim -61.1, k112 7326Hz ref -72.5 / sim -66.7. Model excess above 4 kHz: harmonic 5690Hz +6.2dB, harmonic 5756Hz +6.0dB, inter-harmonic 4088Hz +5.9dB, harmonic 6475Hz +5.8dB.
- **1B-1** worst: k6 392Hz ref -8.2 / sim -5.0, k49 3205Hz ref -78.8 / sim -75.9, k7 458Hz ref -9.6 / sim -6.7, k48 3140Hz ref -78.0 / sim -75.1. Model excess above 4 kHz: none > 3 dB.
- **1B-2** worst: k5 327Hz ref -8.1 / sim -5.6, k2 131Hz ref -4.3 / sim -2.1, k6 392Hz ref -9.5 / sim -7.6, k3 196Hz ref -6.1 / sim -4.2. Model excess above 4 kHz: none > 3 dB.
- **1B-3** worst: k29 1897Hz ref -22.7 / sim -25.1, k24 1570Hz ref -21.0 / sim -23.2, k30 1962Hz ref -23.3 / sim -25.5, k28 1831Hz ref -22.6 / sim -24.6. Model excess above 4 kHz: none > 3 dB.
- **1B-4** worst: k15 981Hz ref -18.4 / sim -22.9, k16 1046Hz ref -19.2 / sim -23.3, k14 916Hz ref -18.1 / sim -22.2, k12 785Hz ref -16.9 / sim -20.9. Model excess above 4 kHz: none > 3 dB.
- **1B-5** worst: k10 654Hz ref -14.9 / sim -8.4, k9 589Hz ref -14.8 / sim -8.4, k8 523Hz ref -14.4 / sim -8.5, k3 196Hz ref -8.2 / sim -2.3. Model excess above 4 kHz: harmonic 4840Hz +5.0dB, harmonic 4775Hz +5.0dB, harmonic 4906Hz +4.9dB, inter-harmonic 10563Hz +3.9dB.
- **1C-1** worst: k40 2616Hz ref -50.0 / sim -44.8, k7 458Hz ref -17.4 / sim -20.9, k2 131Hz ref -6.0 / sim -8.5, k6 392Hz ref -15.9 / sim -18.4. Model excess above 4 kHz: none > 3 dB.
- **1C-2** worst: k16 1046Hz ref -18.8 / sim -21.7, k17 1112Hz ref -19.4 / sim -22.4, k19 1243Hz ref -20.2 / sim -23.2, k14 916Hz ref -17.8 / sim -20.6. Model excess above 4 kHz: none > 3 dB.
- **1C-3** worst: k39 2551Hz ref -22.3 / sim -28.1, k38 2485Hz ref -21.7 / sim -27.6, k40 2616Hz ref -23.0 / sim -28.8, k37 2420Hz ref -21.4 / sim -27.1. Model excess above 4 kHz: none > 3 dB.
- **1D-1** worst: k36 2355Hz ref -75.1 / sim -80.0, k35 2289Hz ref -74.0 / sim -78.8, k34 2224Hz ref -73.0 / sim -77.7, k33 2158Hz ref -71.9 / sim -76.6. Model excess above 4 kHz: none > 3 dB.
- **1D-2** worst: k8 523Hz ref -18.2 / sim -23.9, k9 589Hz ref -19.4 / sim -26.0, k10 654Hz ref -20.6 / sim -27.9, k7 458Hz ref -16.9 / sim -21.4. Model excess above 4 kHz: none > 3 dB.
- **1D-3** worst: k13 850Hz ref -25.3 / sim -22.4, k12 785Hz ref -19.9 / sim -17.1, k14 916Hz ref -29.6 / sim -27.3, k33 2158Hz ref -61.5 / sim -63.7. Model excess above 4 kHz: none > 3 dB.
- **1D-4** worst: k47 3074Hz ref -60.3 / sim -65.6, k48 3140Hz ref -61.9 / sim -66.6, k46 3009Hz ref -58.8 / sim -62.3, k49 3205Hz ref -63.2 / sim -66.2. Model excess above 4 kHz: none > 3 dB.
- **1E-1** worst: k15 981Hz ref -47.8 / sim -59.1, k13 850Hz ref -42.6 / sim -51.7, k17 1112Hz ref -52.7 / sim -67.7, k19 1243Hz ref -57.4 / sim -77.8. Model excess above 4 kHz: none > 3 dB.
- **1E-2** worst: k1 65Hz ref 0.0 / sim 6.1, k36 2355Hz ref -74.6 / sim -79.7, k35 2289Hz ref -73.6 / sim -78.5, k34 2224Hz ref -72.5 / sim -77.3. Model excess above 4 kHz: none > 3 dB.
- **1R-1** worst: k5 327Hz ref -8.9 / sim -5.3, k6 392Hz ref -12.2 / sim -8.7, k2 131Hz ref -4.0 / sim -0.6, k1 65Hz ref 0.0 / sim 3.3. Model excess above 4 kHz: none > 3 dB.
- **2A-1** worst: k21 1374Hz ref -70.2 / sim -75.7, k20 1308Hz ref -68.5 / sim -73.7, k19 1243Hz ref -66.9 / sim -71.8, k22 1439Hz ref -72.1 / sim -77.5. Model excess above 4 kHz: none > 3 dB.
- **2A-2** worst: k5 327Hz ref -18.7 / sim -15.3, k4 262Hz ref -11.3 / sim -9.3, k2 131Hz ref 0.0 / sim -2.0, k3 196Hz ref -4.8 / sim -3.2. Model excess above 4 kHz: none > 3 dB.
- **2A-3** worst: k25 1635Hz ref -73.7 / sim -79.3, k24 1570Hz ref -72.3 / sim -77.6, k23 1504Hz ref -70.8 / sim -75.9, k26 1701Hz ref -75.1 / sim -80.9. Model excess above 4 kHz: none > 3 dB.
- **2A-4** worst: k29 1897Hz ref -75.4 / sim -79.4, k26 1701Hz ref -71.6 / sim -75.5, k27 1766Hz ref -72.9 / sim -76.8, k24 1570Hz ref -68.7 / sim -72.5. Model excess above 4 kHz: none > 3 dB.
- **2A-5** worst: k4 262Hz ref -10.8 / sim -7.6, k3 196Hz ref -5.6 / sim -3.0, k1 65Hz ref 0.0 / sim 2.3, k26 1701Hz ref -70.1 / sim -72.4. Model excess above 4 kHz: none > 3 dB.
- **2A-6** worst: k4 262Hz ref -7.5 / sim -3.6, k5 327Hz ref -11.4 / sim -9.1, k10 654Hz ref -35.7 / sim -33.5, k9 589Hz ref -31.5 / sim -29.3. Model excess above 4 kHz: none > 3 dB.
- **2A-7** worst: k3 196Hz ref 0.0 / sim -3.8, k20 1308Hz ref -56.7 / sim -60.1, k31 2028Hz ref -74.3 / sim -77.6, k21 1374Hz ref -58.7 / sim -62.0. Model excess above 4 kHz: none > 3 dB.
- **2A-8** worst: k1 65Hz ref 0.0 / sim 4.2, k2 131Hz ref -3.9 / sim 0.1, k42 2747Hz ref -76.4 / sim -80.0, k40 2616Hz ref -74.7 / sim -78.2. Model excess above 4 kHz: none > 3 dB.
- **2A-9** worst: k6 392Hz ref -8.9 / sim -4.2, k7 458Hz ref -11.7 / sim -8.2, k2 131Hz ref -4.1 / sim -1.5, k1 65Hz ref 0.0 / sim 2.4. Model excess above 4 kHz: none > 3 dB.
- **2A-10** worst: k1 65Hz ref 0.0 / sim 8.2, k2 131Hz ref -4.9 / sim 3.2, k3 196Hz ref -8.3 / sim -2.1, k4 262Hz ref -10.7 / sim -6.7. Model excess above 4 kHz: none > 3 dB.
- **2A-11** worst: k2 131Hz ref -4.7 / sim 0.4, k3 196Hz ref -7.6 / sim -2.5, k1 65Hz ref 0.0 / sim 4.7, k4 262Hz ref -9.3 / sim -4.7. Model excess above 4 kHz: none > 3 dB.
- **2A-12** worst: k3 196Hz ref -7.6 / sim -3.2, k2 131Hz ref -4.6 / sim -0.4, k4 262Hz ref -9.2 / sim -5.1, k1 65Hz ref 0.0 / sim 3.9. Model excess above 4 kHz: none > 3 dB.
- **2A-13** worst: k11 720Hz ref -11.1 / sim -7.1, k3 196Hz ref -7.8 / sim -4.3, k12 785Hz ref -12.8 / sim -9.5, k2 131Hz ref -4.7 / sim -1.4. Model excess above 4 kHz: none > 3 dB.
- **2A-14** worst: k12 785Hz ref -7.8 / sim -1.2, k13 850Hz ref -9.5 / sim -5.3, k74 4840Hz ref -79.4 / sim -76.3, k73 4775Hz ref -78.9 / sim -75.7. Model excess above 4 kHz: harmonic 4906Hz +3.2dB, harmonic 4840Hz +3.2dB, harmonic 4775Hz +3.2dB.
- **2A-15** worst: k3 196Hz ref -8.2 / sim -1.9, k4 262Hz ref -10.4 / sim -4.2, k2 131Hz ref -4.9 / sim 1.2, k5 327Hz ref -11.9 / sim -6.0. Model excess above 4 kHz: none > 3 dB.
- **2A-16** worst: k3 196Hz ref -8.2 / sim -2.7, k4 262Hz ref -10.4 / sim -5.0, k2 131Hz ref -4.9 / sim 0.4, k5 327Hz ref -12.0 / sim -6.7. Model excess above 4 kHz: none > 3 dB.
- **2B-1** worst: k6 392Hz ref -8.6 / sim -5.6, k7 458Hz ref -9.9 / sim -7.5, k2 131Hz ref -4.3 / sim -2.1, k5 327Hz ref -7.0 / sim -4.9. Model excess above 4 kHz: none > 3 dB.
- **2B-2** worst: k2 131Hz ref -4.2 / sim -2.1, k74 4840Hz ref -79.6 / sim -77.6, k73 4775Hz ref -79.1 / sim -77.1, k22 1439Hz ref -30.4 / sim -28.4. Model excess above 4 kHz: none > 3 dB.
- **2B-3** worst: k29 1897Hz ref -34.8 / sim -31.8, k28 1831Hz ref -32.9 / sim -29.9, k30 1962Hz ref -36.6 / sim -33.8, k86 5625Hz ref -79.3 / sim -76.5. Model excess above 4 kHz: none > 3 dB.
- **2B-4** worst: k32 2093Hz ref -37.2 / sim -31.5, k31 2028Hz ref -35.6 / sim -30.0, k33 2158Hz ref -38.7 / sim -33.2, k30 1962Hz ref -34.0 / sim -28.6. Model excess above 4 kHz: harmonic 6017Hz +3.9dB, harmonic 5102Hz +3.9dB, harmonic 5036Hz +3.9dB.
- **2B-5** worst: k37 2420Hz ref -37.2 / sim -33.2, k36 2355Hz ref -35.8 / sim -31.8, k38 2485Hz ref -38.5 / sim -34.7, k35 2289Hz ref -34.2 / sim -30.6. Model excess above 4 kHz: none > 3 dB.
- **2B-6** worst: k78 5102Hz ref -60.3 / sim -57.8, k71 4644Hz ref -56.6 / sim -54.2, k74 4840Hz ref -58.1 / sim -55.8, k75 4906Hz ref -58.6 / sim -56.3. Model excess above 4 kHz: none > 3 dB.
- **2B-7** worst: k14 916Hz ref -47.2 / sim -49.8, k15 981Hz ref -49.9 / sim -52.4, k13 850Hz ref -44.5 / sim -46.9, k16 1046Hz ref -52.6 / sim -54.9. Model excess above 4 kHz: none > 3 dB.
- **2B-8** worst: k15 981Hz ref -48.0 / sim -50.2, k16 1046Hz ref -50.5 / sim -52.7, k14 916Hz ref -45.4 / sim -47.5, k17 1112Hz ref -53.0 / sim -55.0. Model excess above 4 kHz: none > 3 dB.
- **2B-9** worst: k10 654Hz ref -27.6 / sim -24.6, k9 589Hz ref -22.8 / sim -20.1, k11 720Hz ref -31.2 / sim -28.9, k12 785Hz ref -34.2 / sim -32.7. Model excess above 4 kHz: none > 3 dB.
- **2B-10** worst: k19 1243Hz ref -37.0 / sim -28.3, k20 1308Hz ref -38.9 / sim -30.3, k18 1177Hz ref -34.7 / sim -26.5, k21 1374Hz ref -40.7 / sim -32.7. Model excess above 4 kHz: harmonic 4055Hz +3.0dB.
- **2B-11** worst: k3 196Hz ref -8.2 / sim -2.7, k4 262Hz ref -10.4 / sim -5.0, k2 131Hz ref -4.9 / sim 0.4, k5 327Hz ref -12.0 / sim -6.8. Model excess above 4 kHz: harmonic 6214Hz +4.4dB, harmonic 6017Hz +4.4dB, harmonic 6148Hz +4.4dB.
- **2B-12** worst: k3 196Hz ref -8.2 / sim -2.7, k4 262Hz ref -10.5 / sim -5.0, k2 131Hz ref -4.9 / sim 0.4, k5 327Hz ref -12.0 / sim -6.8. Model excess above 4 kHz: harmonic 6475Hz +4.5dB, harmonic 6606Hz +4.5dB, harmonic 6868Hz +4.5dB.
- **2B-13** worst: k127 8307Hz ref -69.0 / sim -62.5, k125 8176Hz ref -68.2 / sim -61.8, k126 8241Hz ref -68.6 / sim -62.2, k124 8110Hz ref -67.9 / sim -61.5. Model excess above 4 kHz: inter-harmonic 4480Hz +7.0dB, inter-harmonic 4546Hz +7.0dB, inter-harmonic 4611Hz +6.8dB, harmonic 8372Hz +6.5dB.
- **2B-14** worst: k13 850Hz ref -16.0 / sim -10.0, k3 196Hz ref -8.2 / sim -2.4, k4 262Hz ref -10.4 / sim -4.7, k12 785Hz ref -16.1 / sim -10.3. Model excess above 4 kHz: harmonic 7652Hz +7.1dB, harmonic 7587Hz +7.1dB, harmonic 7718Hz +7.1dB, inter-harmonic 6835Hz +4.7dB.
- **2C-1** worst: k9 589Hz ref -17.3 / sim -21.6, k7 458Hz ref -15.6 / sim -19.1, k10 654Hz ref -18.2 / sim -23.5, k8 523Hz ref -16.7 / sim -19.8. Model excess above 4 kHz: none > 3 dB.
- **2C-2** worst: k11 720Hz ref -17.8 / sim -21.6, k12 785Hz ref -18.8 / sim -22.1, k10 654Hz ref -17.5 / sim -20.3, k9 589Hz ref -16.6 / sim -19.2. Model excess above 4 kHz: none > 3 dB.
- **2C-3** worst: k13 850Hz ref -17.9 / sim -21.2, k16 1046Hz ref -19.5 / sim -23.2, k17 1112Hz ref -20.0 / sim -23.8, k14 916Hz ref -18.6 / sim -21.6. Model excess above 4 kHz: none > 3 dB.
- **2C-4** worst: k21 1374Hz ref -19.9 / sim -23.3, k20 1308Hz ref -19.6 / sim -22.8, k22 1439Hz ref -20.7 / sim -23.6, k18 1177Hz ref -19.0 / sim -21.9. Model excess above 4 kHz: none > 3 dB.
- **2C-5** worst: k28 1831Hz ref -21.3 / sim -24.6, k25 1635Hz ref -20.2 / sim -23.5, k27 1766Hz ref -21.0 / sim -24.3, k26 1701Hz ref -20.6 / sim -23.9. Model excess above 4 kHz: none > 3 dB.
- **2C-6** worst: k33 2158Hz ref -21.5 / sim -25.6, k34 2224Hz ref -21.8 / sim -26.0, k32 2093Hz ref -21.2 / sim -25.3, k31 2028Hz ref -20.9 / sim -24.9. Model excess above 4 kHz: none > 3 dB.
- **2R-1** worst: k5 327Hz ref -8.9 / sim -5.4, k6 392Hz ref -12.1 / sim -8.7, k2 131Hz ref -3.9 / sim -0.6, k1 65Hz ref 0.0 / sim 3.3. Model excess above 4 kHz: none > 3 dB.
- **3A-1** worst: k24 1570Hz ref -68.3 / sim -71.4, k29 1897Hz ref -75.0 / sim -78.9, k23 1504Hz ref -66.9 / sim -69.8, k28 1831Hz ref -73.7 / sim -77.4. Model excess above 4 kHz: none > 3 dB.
- **3A-2** worst: k1 65Hz ref 0.0 / sim 3.2, k10 654Hz ref -36.4 / sim -39.6, k9 589Hz ref -33.7 / sim -36.7, k11 720Hz ref -39.0 / sim -42.3. Model excess above 4 kHz: none > 3 dB.
- **3A-3** worst: k1 65Hz ref 0.0 / sim 4.0, k7 458Hz ref -23.1 / sim -26.9, k8 523Hz ref -25.4 / sim -30.0, k2 131Hz ref -6.2 / sim -3.1. Model excess above 4 kHz: none > 3 dB.
- **3A-4** worst: k9 589Hz ref -22.7 / sim -28.3, k8 523Hz ref -21.1 / sim -25.8, k10 654Hz ref -24.2 / sim -30.5, k1 65Hz ref 0.0 / sim 4.3. Model excess above 4 kHz: none > 3 dB.
- **3A-5** worst: k11 720Hz ref -36.2 / sim -32.4, k10 654Hz ref -32.8 / sim -29.2, k12 785Hz ref -39.1 / sim -35.5, k13 850Hz ref -41.5 / sim -38.5. Model excess above 4 kHz: none > 3 dB.
- **3A-6** worst: k3 196Hz ref -7.8 / sim -4.8, k4 262Hz ref -11.6 / sim -8.8, k1 65Hz ref 0.0 / sim 2.6, k5 327Hz ref -14.1 / sim -11.7. Model excess above 4 kHz: none > 3 dB.
- **3A-7** worst: k3 196Hz ref -7.3 / sim -4.3, k1 65Hz ref 0.0 / sim 2.8, k4 262Hz ref -10.5 / sim -7.7, k5 327Hz ref -12.5 / sim -10.2. Model excess above 4 kHz: none > 3 dB.
- **3A-8** worst: k3 196Hz ref -7.2 / sim -4.7, k1 65Hz ref 0.0 / sim 2.4, k4 262Hz ref -9.9 / sim -7.7, k5 327Hz ref -11.6 / sim -9.8. Model excess above 4 kHz: none > 3 dB.
- **3A-9** worst: k31 2028Hz ref -36.2 / sim -46.7, k32 2093Hz ref -37.6 / sim -47.6, k30 1962Hz ref -34.7 / sim -45.8, k33 2158Hz ref -38.9 / sim -48.4. Model excess above 4 kHz: none > 3 dB.
- **3A-10** worst: k12 785Hz ref -29.4 / sim -22.3, k15 981Hz ref -40.3 / sim -34.9, k16 1046Hz ref -43.4 / sim -38.4, k17 1112Hz ref -45.4 / sim -41.5. Model excess above 4 kHz: none > 3 dB.
- **3A-11** worst: k12 785Hz ref -29.0 / sim -19.0, k14 916Hz ref -36.1 / sim -27.8, k15 981Hz ref -39.5 / sim -32.1, k16 1046Hz ref -41.9 / sim -35.7. Model excess above 4 kHz: none > 3 dB.
- **3A-12** worst: k15 981Hz ref -35.9 / sim -28.5, k16 1046Hz ref -38.5 / sim -32.2, k17 1112Hz ref -41.0 / sim -35.3, k18 1177Hz ref -43.4 / sim -38.0. Model excess above 4 kHz: none > 3 dB.
- **3A-13** worst: k13 850Hz ref -22.0 / sim -9.7, k14 916Hz ref -26.3 / sim -16.7, k15 981Hz ref -29.5 / sim -21.9, k16 1046Hz ref -32.7 / sim -25.9. Model excess above 4 kHz: none > 3 dB.
- **3A-14** worst: k3 196Hz ref -6.5 / sim -4.7, k1 65Hz ref 0.0 / sim 1.7, k4 262Hz ref -9.1 / sim -7.9, k2 131Hz ref -2.6 / sim -1.7. Model excess above 4 kHz: none > 3 dB.
- **3A-15** worst: k59 3859Hz ref -57.1 / sim -70.9, k61 3990Hz ref -58.4 / sim -68.5, k57 3728Hz ref -55.7 / sim -71.7, k55 3597Hz ref -54.4 / sim -67.4. Model excess above 4 kHz: none > 3 dB.
- **3B-1** worst: k17 1112Hz ref -69.1 / sim -83.5, k9 589Hz ref -46.2 / sim -53.5, k15 981Hz ref -63.8 / sim -77.0, k11 720Hz ref -52.8 / sim -61.7. Model excess above 4 kHz: none > 3 dB.
- **3B-2** worst: k2 131Hz ref -14.1 / sim -22.9, k5 327Hz ref -8.8 / sim -15.7, k13 850Hz ref -47.5 / sim -58.0, k15 981Hz ref -52.8 / sim -65.9. Model excess above 4 kHz: none > 3 dB.
- **3B-3** worst: k5 327Hz ref -1.7 / sim -9.5, k35 2289Hz ref -76.1 / sim -68.4, k37 2420Hz ref -77.9 / sim -70.3, k33 2158Hz ref -74.0 / sim -66.7. Model excess above 4 kHz: none > 3 dB.
- **3B-4** worst: k17 1112Hz ref -29.4 / sim -45.5, k19 1243Hz ref -32.6 / sim -58.6, k21 1374Hz ref -36.0 / sim -51.3, k36 2355Hz ref -48.6 / sim -62.6. Model excess above 4 kHz: harmonic 5036Hz +3.5dB, harmonic 4906Hz +3.4dB, harmonic 5167Hz +3.3dB.
- **3B-5** worst: k19 1243Hz ref -22.6 / sim -49.4, k17 1112Hz ref -21.8 / sim -33.0, k98 6410Hz ref -74.1 / sim -63.4, k96 6279Hz ref -72.8 / sim -62.4. Model excess above 4 kHz: inter-harmonic 4480Hz +15.8dB, inter-harmonic 4611Hz +15.6dB, inter-harmonic 4742Hz +15.4dB, harmonic 6410Hz +10.7dB.
- **3C-1** worst: k4 131Hz ref -7.5 / sim 3.0, k5 164Hz ref -9.3 / sim 1.0, k6 196Hz ref -10.9 / sim -0.9, k3 98Hz ref -5.1 / sim 4.7. Model excess above 4 kHz: none > 3 dB.
- **3C-2** worst: k1 131Hz ref 0.0 / sim 10.5, k2 262Hz ref -5.7 / sim 3.1, k3 392Hz ref -8.9 / sim -2.8, k4 523Hz ref -11.2 / sim -7.6. Model excess above 4 kHz: none > 3 dB.
- **3C-3** worst: k1 262Hz ref 0.0 / sim 10.0, k2 523Hz ref -5.4 / sim -0.6, k15 3924Hz ref -50.0 / sim -48.5, k16 4186Hz ref -52.2 / sim -50.7. Model excess above 4 kHz: none > 3 dB.
- **3C-4** worst: k5 164Hz ref -27.0 / sim -3.8, k10 327Hz ref -29.0 / sim -10.7, k24 785Hz ref -28.6 / sim -49.6, k12 392Hz ref -27.3 / sim -13.2. Model excess above 4 kHz: harmonic 5167Hz +4.9dB, harmonic 4742Hz +4.8dB, harmonic 5102Hz +4.7dB.
- **3C-5** worst: k10 1308Hz ref -25.3 / sim -49.7, k8 1046Hz ref -22.4 / sim -46.9, k12 1570Hz ref -28.7 / sim -52.4, k14 1831Hz ref -32.3 / sim -54.8. Model excess above 4 kHz: none > 3 dB.
- **3C-6** worst: k4 1046Hz ref -14.9 / sim -29.7, k6 1570Hz ref -21.6 / sim -35.2, k23 6017Hz ref -67.1 / sim -82.4, k25 6541Hz ref -68.4 / sim -80.9. Model excess above 4 kHz: harmonic 6802Hz +6.0dB, harmonic 6279Hz +4.0dB.
- **3D-1** worst: k52 3401Hz ref -78.6 / sim -71.0, k53 3466Hz ref -79.2 / sim -72.0, k49 3205Hz ref -76.4 / sim -69.6, k50 3270Hz ref -77.1 / sim -71.4. Model excess above 4 kHz: harmonic 4055Hz +5.6dB, inter-harmonic 4088Hz +4.9dB, harmonic 4251Hz +4.8dB, inter-harmonic 4153Hz +4.5dB.
- **3D-2** worst: k1 65Hz ref 0.0 / sim 6.3, k20 1308Hz ref -46.3 / sim -51.7, k24 1570Hz ref -52.4 / sim -58.1, k27 1766Hz ref -56.4 / sim -62.0. Model excess above 4 kHz: none > 3 dB.
- **3D-3** worst: k47 3074Hz ref -79.3 / sim -67.5, k46 3009Hz ref -78.5 / sim -67.0, k45 2943Hz ref -77.7 / sim -66.5, k44 2878Hz ref -76.9 / sim -66.2. Model excess above 4 kHz: harmonic 4055Hz +7.8dB, harmonic 4121Hz +7.6dB, inter-harmonic 4088Hz +7.6dB, inter-harmonic 4153Hz +7.4dB.
- **3D-4** worst: k1 65Hz ref 0.0 / sim 6.4, k2 131Hz ref -4.8 / sim -0.2, k20 1308Hz ref -51.3 / sim -54.4, k17 1112Hz ref -46.0 / sim -49.1. Model excess above 4 kHz: none > 3 dB.
- **3D-5** worst: k8 523Hz ref -16.2 / sim -21.1, k9 589Hz ref -17.3 / sim -22.5, k2 131Hz ref -5.2 / sim -0.8, k10 654Hz ref -18.2 / sim -24.3. Model excess above 4 kHz: inter-harmonic 8078Hz +4.1dB, inter-harmonic 8012Hz +4.1dB, inter-harmonic 8143Hz +4.0dB, harmonic 8699Hz +3.8dB.
- **3D-6** worst: k10 654Hz ref -18.1 / sim -24.1, k11 720Hz ref -19.1 / sim -25.6, k12 785Hz ref -20.0 / sim -27.2, k9 589Hz ref -17.1 / sim -22.3. Model excess above 4 kHz: none > 3 dB.
- **3D-7** worst: k11 720Hz ref -19.2 / sim -26.1, k10 654Hz ref -18.2 / sim -24.4, k12 785Hz ref -20.2 / sim -27.7, k13 850Hz ref -21.2 / sim -29.2. Model excess above 4 kHz: none > 3 dB.
- **3D-8** worst: k14 916Hz ref -23.0 / sim -31.0, k15 981Hz ref -24.0 / sim -32.4, k13 850Hz ref -22.1 / sim -29.6, k16 1046Hz ref -24.9 / sim -33.7. Model excess above 4 kHz: none > 3 dB.
- **3D-9** worst: k11 720Hz ref -21.5 / sim -27.3, k2 131Hz ref -5.1 / sim 0.3, k12 785Hz ref -22.6 / sim -28.9, k10 654Hz ref -20.4 / sim -25.6. Model excess above 4 kHz: none > 3 dB.
- **3R-1** worst: k6 392Hz ref -12.3 / sim -8.7, k5 327Hz ref -8.9 / sim -5.3, k2 131Hz ref -4.0 / sim -0.6, k1 65Hz ref 0.0 / sim 3.3. Model excess above 4 kHz: none > 3 dB.

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
- **3A-13**: error 4.40 is 2.5x the median
- **3B-3**: error 4.12 is 2.4x the median
- **3D-5**: error 3.71 is 2.1x the median
- **3C-1**: error 3.67 is 2.1x the median
- **3A-15**: error 3.45 is 2.0x the median
- **1B-5**: error 3.32 is 1.9x the median
- **1D-4**: error 3.21 is 1.8x the median
- **2A-14**: error 3.13 is 1.8x the median
- **2B-14**: error 3.05 is 1.8x the median
- **2A-7**: error 3.04 is 1.7x the median
- **3A-12**: error 3.03 is 1.7x the median
- **3B-5**: error 2.97 is 1.7x the median
- **3D-6**: error 2.95 is 1.7x the median
- **2A-2**: error 2.89 is 1.7x the median
- **2A-6**: error 2.87 is 1.6x the median
- **1A-9**: error 2.83 is 1.6x the median
- **3A-11**: error 2.79 is 1.6x the median
- **2A-5**: error 2.76 is 1.6x the median
- **2A-9**: error 2.73 is 1.6x the median
- **3A-10**: error 2.67 is 1.5x the median
- **1A-3**: error 2.62 is 1.5x the median

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

Cutoff law with the fitted end stops folded in (so the plugin's knob range equals the hardware's): `cutoffBaseHz = 264.1`, `cutoffSpanOct = 3.156`, `cutoffTaperExp = 1.611`.

## Fitted model parameters

`sensitivity` = objective increase for a 5% (of range) nudge. Near-zero means the current references don't constrain that parameter -- its value is not evidence of anything. `AT BOUND` means the optimizer pushed it to the edge of its plausible range, usually a sign that it is compensating for something the model does not implement.

| parameter              | before   | after     | sensitivity |                                     |
|------------------------|----------|-----------|-------------|-------------------------------------|
| oscCouplingHz          | 47.24    | 47.516    | 0.000       |                                     |
| oscSawLpfHz            | 40000    | 27850     | 0.000       |                                     |
| oscSawShape            | 0        | -0.058705 | 0.000       |                                     |
| oscSquareDutyDepth     | 0.12     | 0.10951   | 0.000       |                                     |
| oscSquareLevel         | 0.38901  | 0.35985   | 0.000       |                                     |
| vcfAttackMs            | 0.1      | 0.10475   | 0.000       |                                     |
| vcaAttackMs            | 1.3011   | 0.91306   | 0.000       |                                     |
| vcaNormalDelayMs       | 4.504    | 4.3457    | 0.000       |                                     |
| vcfDecayMinSec         | 0.063658 | 0.062069  | 0.000       |                                     |
| vcfDecayMaxSec         | 1.0691   | 0.96118   | 0.000       |                                     |
| vcfDecayTaper          | 20.334   | 18.178    | 0.000       |                                     |
| accentDecaySec         | 0.084449 | 0.074103  | 0.000       |                                     |
| vegDecaySec            | 1.009    | 1.0711    | 0.000       |                                     |
| vcaGateOffMs           | 0.81135  | 0.50453   | 0.000       |                                     |
| vcaGateOffAccentMs     | 2.8037   | 7.215     | 0.000       |                                     |
| vcaResTapRatio         | 1.6824   | 2.0189    | 0.000       |                                     |
| vcaGainSaturationDrive | 0        | 0.0086354 | 0.000       | AT BOUND (model may lack structure) |
| vcaCutoffLevelDb       | -1.9418  | -1.5803   | 0.000       |                                     |
| vcaCutoffLevelResDb    | 1.2048   | 1.6823    | 0.000       |                                     |

Timing: note-on offset -0.74 ms (relative to each clip's measured note-on), gate length 1305.4 ms.

## Applying the result

Paste `synth_parameters.txt` into `acidus::SynthParameters` (src/core/SynthEngine.hpp) or rerun with `--apply`.

Audio: `renders/` (hardware / before / after for the 40 worst samples).
