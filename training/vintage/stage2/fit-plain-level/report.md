# Acidus vs. hardware TB-303 calibration report (20261010-125014)

61 reference samples, 5 free parameters. Search: 129 evaluations in 10.1 min, 0 CMA-ES restarts, stopped because: time cap reached.

## Match quality (mean over samples)

All errors are dB; lower is better. `harm` = full-note harmonic levels (k^-0.5 weighted RMS), `inter` = energy between harmonics, `env` = RMS envelope, `stft` = 1/3-octave levels over time, `peak` = shape (height+width+implicit frequency) of the resonant-peak window on Resonance=max samples only -- weighted more heavily than `harm` so it can't be traded away.

| metric                                         | before | after |
|------------------------------------------------|--------|-------|
| objective (incl. priors)                       | 1.84   | 1.83  |
| weighted error                                 | 1.84   | 1.83  |
| harmonic error (dB)                            | 1.58   | 1.60  |
| resonant-peak shape error (dB)                 | 0.86   | 0.86  |
| inter-harmonic error (dB)                      | 0.52   | 0.54  |
| envelope error (dB)                            | 3.75   | 3.66  |
| spectrogram error (dB)                         | 6.58   | 6.56  |
| aligned waveform error (10*sqrt(residual/ref)) | 0.00   | 0.00  |
| resonant-peak sweep track error (semitones)    | 2.14   | 2.14  |
| harmonics-over-time error (dB)                 | 4.79   | 4.75  |
| mean |harmonic error| (dB)                     | 1.85   | 1.92  |
| note level error, RMS over notes (dB)          | 2.82   | 2.64  |
| harmonics within 1 dB (%)                      | 33.65  | 32.84 |
| harmonics within 3 dB (%)                      | 80.10  | 78.30 |
| harmonics within 6 dB (%)                      | 98.91  | 98.91 |

Recording gain solved as -11.9 dB (before: -12.1 dB).

## Per set

Mean error of each set's notes, and the RMS of their level errors (the recordings are not normalised, so absolute note levels are compared under one global gain).

| set | notes | before | after | level before (dB) | level after (dB) | harm | env | stft |
|-----|-------|--------|-------|-------------------|------------------|------|-----|------|
| 1A  | 9     | 1.87   | 1.86  | 3.8               | 3.6              | 1.9  | 3.8 | 6.7  |
| 1B  | 5     | 2.02   | 2.03  | 2.2               | 2.3              | 1.4  | 4.1 | 7.1  |
| 1C  | 3     | 1.67   | 1.65  | 2.0               | 1.9              | 1.8  | 3.9 | 5.7  |
| 1D  | 4     | 1.94   | 1.94  | 2.6               | 1.9              | 1.4  | 2.8 | 6.2  |
| 1E  | 2     | 1.10   | 1.07  | 4.5               | 3.9              | 2.5  | 3.3 | 4.8  |
| 1R  | 1     | 1.79   | 1.78  | 2.9               | 2.8              | 1.2  | 3.4 | 7.7  |
| 2A  | 16    | 1.95   | 1.94  | 3.2               | 3.0              | 1.5  | 4.2 | 7.0  |
| 2B  | 14    | 1.85   | 1.86  | 1.9               | 2.0              | 1.6  | 3.0 | 6.4  |
| 2C  | 6     | 1.63   | 1.61  | 1.6               | 1.5              | 1.4  | 3.9 | 6.2  |
| 2R  | 1     | 1.55   | 1.54  | 2.8               | 2.7              | 1.4  | 3.4 | 7.1  |

## Per sample (sorted by remaining error, worst first)

| sample | before | after | harm | peak | inter | env | stft | wave | harm ±3dB | level before | level after | tuning (c) | drift (c) |
|--------|--------|-------|------|------|-------|-----|------|------|-----------|--------------|-------------|------------|-----------|
| 1D-4   | 3.47   | 3.52  | 1.1  | 3.6  | 2.1   | 1.9 | 7.3  | 0.0  | 97%       | -0.6         | -0.4        | +0.0       | 0.00      |
| 2A-14  | 3.13   | 3.18  | 1.2  | 4.7  | 0.5   | 2.3 | 5.6  | 0.0  | 97%       | +1.4         | +1.6        | +0.0       | 0.00      |
| 2A-7   | 3.09   | 3.09  | 1.3  | 2.2  | 0.1   | 2.2 | 8.0  | 0.0  | 76%       | -0.1         | +0.4        | +0.0       | 0.00      |
| 2A-2   | 2.96   | 2.96  | 0.7  | 1.7  | 0.0   | 2.2 | 8.8  | 0.0  | 94%       | +0.1         | +0.3        | +0.0       | 0.00      |
| 1B-5   | 2.87   | 2.89  | 2.4  | 0.5  | 0.7   | 5.1 | 6.6  | 0.0  | 80%       | +3.6         | +3.7        | +0.0       | 0.00      |
| 2A-6   | 2.84   | 2.87  | 1.1  | 1.1  | 0.4   | 7.3 | 8.6  | 0.0  | 97%       | +1.5         | +1.6        | +0.0       | 0.00      |
| 2A-5   | 2.79   | 2.77  | 1.2  | 0.9  | 0.0   | 2.8 | 7.6  | 0.0  | 82%       | +2.0         | +1.9        | +0.0       | 0.00      |
| 2A-9   | 2.69   | 2.71  | 1.0  | 0.9  | 0.0   | 2.8 | 7.7  | 0.0  | 96%       | +2.0         | +2.2        | +0.0       | 0.00      |
| 1A-3   | 2.70   | 2.69  | 1.9  | 1.3  | 0.0   | 2.2 | 7.4  | 0.0  | 41%       | -1.2         | -0.7        | +0.0       | 0.00      |
| 2B-14  | 2.62   | 2.64  | 3.1  | 0.4  | 1.2   | 3.7 | 7.8  | 0.0  | 81%       | +3.5         | +3.6        | +0.0       | 0.00      |
| 2B-7   | 2.64   | 2.63  | 0.7  | 1.0  | 0.0   | 2.2 | 8.0  | 0.0  | 100%      | -0.3         | -0.1        | +0.0       | 0.00      |
| 1A-6   | 2.55   | 2.58  | 1.0  | 2.1  | 0.2   | 2.4 | 7.8  | 0.0  | 96%       | +0.9         | +1.3        | +0.0       | 0.00      |
| 1A-9   | 2.49   | 2.50  | 2.7  | 3.7  | 2.1   | 2.9 | 4.2  | 0.0  | 37%       | +2.2         | +2.3        | +0.0       | 0.00      |
| 2B-8   | 2.42   | 2.42  | 0.6  | 1.0  | 0.1   | 2.3 | 7.2  | 0.0  | 100%      | -0.4         | -0.3        | +0.0       | 0.00      |
| 1B-1   | 2.12   | 2.15  | 1.2  | 0.6  | 0.0   | 2.8 | 7.7  | 0.0  | 100%      | +1.6         | +1.8        | +0.0       | 0.00      |
| 1B-4   | 2.15   | 2.12  | 1.6  | 0.6  | 0.9   | 7.2 | 7.5  | 0.0  | 89%       | -2.8         | -2.6        | +0.0       | 0.00      |
| 1D-3   | 2.07   | 2.07  | 0.7  | 1.2  | 0.9   | 2.2 | 5.2  | 0.0  | 100%      | -0.5         | -0.1        | +0.0       | 0.00      |
| 2B-6   | 2.04   | 2.05  | 1.0  | 0.7  | 0.3   | 2.5 | 6.6  | 0.0  | 100%      | +0.9         | +1.0        | +0.0       | 0.00      |
| 2B-10  | 2.04   | 2.05  | 2.8  | 0.6  | 2.7   | 2.8 | 6.7  | 0.0  | 25%       | -1.5         | -1.3        | +0.0       | 0.00      |
| 2B-1   | 2.02   | 2.04  | 0.9  | 0.6  | 0.0   | 2.9 | 8.3  | 0.0  | 100%      | +1.5         | +1.7        | +0.0       | 0.00      |
| 1C-3   | 1.96   | 1.93  | 3.1  | 0.3  | 0.0   | 2.4 | 6.0  | 0.0  | 37%       | -2.5         | -2.4        | +0.0       | 0.00      |
| 2B-13  | 1.87   | 1.89  | 3.1  | 0.4  | 1.4   | 6.4 | 4.8  | 0.0  | 40%       | +3.3         | +3.4        | +0.0       | 0.00      |
| 1A-7   | 1.92   | 1.88  | 3.3  | -    | 1.2   | 8.2 | 8.1  | 0.0  | 86%       | +8.3         | +8.0        | +0.0       | 0.00      |
| 2C-5   | 1.86   | 1.85  | 1.4  | 0.3  | 0.3   | 3.2 | 7.0  | 0.0  | 88%       | -1.1         | -0.9        | +0.0       | 0.00      |
| 1R-1   | 1.79   | 1.78  | 1.2  | 0.5  | 0.0   | 3.4 | 7.7  | 0.0  | 94%       | +2.9         | +2.8        | +0.0       | 0.00      |
| 1C-1   | 1.75   | 1.73  | 1.1  | 0.5  | 1.6   | 4.7 | 7.1  | 0.0  | 90%       | -2.2         | -2.1        | +0.0       | 0.00      |
| 2C-1   | 1.74   | 1.73  | 1.2  | 0.7  | 0.1   | 3.6 | 7.5  | 0.0  | 69%       | -2.3         | -2.2        | +0.0       | 0.00      |
| 2B-5   | 1.71   | 1.72  | 1.3  | 0.4  | 1.0   | 2.5 | 6.8  | 0.0  | 98%       | +0.7         | +0.9        | +0.0       | 0.00      |
| 1A-2   | 1.70   | 1.69  | 1.4  | 0.9  | 0.0   | 2.2 | 8.5  | 0.0  | 57%       | +0.5         | +0.3        | +0.0       | 0.00      |
| 2C-6   | 1.70   | 1.69  | 2.2  | 0.2  | 0.2   | 2.8 | 4.6  | 0.0  | 74%       | -1.7         | -1.5        | +0.0       | 0.00      |
| 2A-13  | 1.67   | 1.69  | 1.1  | 0.4  | 0.1   | 2.7 | 7.1  | 0.0  | 99%       | +2.0         | +2.1        | +0.0       | 0.00      |
| 1A-8   | 1.62   | 1.63  | 1.8  | 0.4  | 1.1   | 3.6 | 5.1  | 0.0  | 93%       | +3.5         | +3.6        | +0.0       | 0.00      |
| 2A-10  | 1.66   | 1.61  | 2.6  | -    | 1.2   | 8.9 | 5.3  | 0.0  | 71%       | +7.7         | +7.0        | +0.0       | 0.00      |
| 2A-12  | 1.61   | 1.61  | 1.5  | 0.4  | 0.3   | 3.6 | 7.2  | 0.0  | 94%       | +3.1         | +3.0        | +0.0       | 0.00      |
| 2B-4   | 1.59   | 1.61  | 1.9  | 0.4  | 1.4   | 2.5 | 6.2  | 0.0  | 51%       | +0.9         | +1.1        | +0.0       | 0.00      |
| 2B-2   | 1.57   | 1.59  | 0.8  | 0.2  | 0.3   | 2.6 | 6.3  | 0.0  | 100%      | +1.2         | +1.3        | +0.0       | 0.00      |
| 1B-2   | 1.55   | 1.57  | 0.7  | 0.3  | 0.0   | 2.9 | 7.7  | 0.0  | 100%      | +1.3         | +1.4        | +0.0       | 0.00      |
| 2C-4   | 1.57   | 1.55  | 1.2  | 0.3  | 0.0   | 4.0 | 6.8  | 0.0  | 90%       | -1.1         | -0.9        | +0.0       | 0.00      |
| 2A-16  | 1.52   | 1.55  | 1.9  | 0.4  | 0.7   | 3.7 | 6.6  | 0.0  | 93%       | +3.2         | +3.3        | +0.0       | 0.00      |
| 2R-1   | 1.55   | 1.54  | 1.4  | 0.5  | 0.0   | 3.4 | 7.1  | 0.0  | 96%       | +2.8         | +2.7        | +0.0       | 0.00      |
| 2C-2   | 1.56   | 1.54  | 1.2  | 0.3  | 1.6   | 5.1 | 5.8  | 0.0  | 92%       | -1.8         | -1.7        | +0.0       | 0.00      |
| 2B-11  | 1.42   | 1.44  | 2.4  | 0.3  | 0.9   | 3.4 | 6.3  | 0.0  | 58%       | +3.1         | +3.2        | +0.0       | 0.00      |
| 1A-5   | 1.42   | 1.41  | 1.1  | 0.5  | 0.0   | 4.5 | 6.2  | 0.0  | 93%       | +2.9         | +2.8        | +0.0       | 0.00      |
| 1B-3   | 1.39   | 1.40  | 1.0  | 0.3  | 0.0   | 2.4 | 6.3  | 0.0  | 100%      | +0.3         | +0.5        | +0.0       | 0.00      |
| 2B-9   | 1.39   | 1.40  | 0.7  | 0.7  | 0.1   | 2.2 | 6.8  | 0.0  | 100%      | -0.7         | -0.5        | +0.0       | 0.00      |
| 2A-15  | 1.35   | 1.34  | 2.2  | -    | 1.3   | 7.3 | 4.5  | 0.0  | 92%       | +4.6         | +4.5        | +0.0       | 0.00      |
| 2C-3   | 1.35   | 1.33  | 1.2  | 0.2  | 0.0   | 4.6 | 5.3  | 0.0  | 91%       | -1.5         | -1.4        | +0.0       | 0.00      |
| 2B-3   | 1.30   | 1.32  | 1.1  | 0.3  | 1.6   | 3.0 | 3.7  | 0.0  | 100%      | +1.0         | +1.1        | +0.0       | 0.00      |
| 2A-1   | 1.31   | 1.30  | 1.4  | -    | 0.0   | 7.4 | 6.0  | 0.0  | 64%       | +0.9         | +0.3        | +0.0       | 0.00      |
| 1C-2   | 1.30   | 1.28  | 1.2  | 0.3  | 1.4   | 4.8 | 3.9  | 0.0  | 95%       | -1.1         | -1.0        | +0.0       | 0.00      |
| 1A-4   | 1.35   | 1.28  | 2.2  | -    | 0.0   | 5.1 | 6.6  | 0.0  | 40%       | +5.8         | +4.9        | +0.0       | 0.00      |
| 2B-12  | 1.23   | 1.26  | 2.4  | 0.4  | 1.9   | 3.3 | 4.1  | 0.0  | 56%       | +3.2         | +3.2        | +0.0       | 0.00      |
| 2A-11  | 1.21   | 1.18  | 1.8  | -    | 0.1   | 4.2 | 6.8  | 0.0  | 93%       | +4.1         | +3.9        | +0.0       | 0.00      |
| 2A-8   | 1.17   | 1.14  | 1.7  | -    | 0.0   | 3.5 | 7.4  | 0.0  | 65%       | +3.8         | +3.4        | +0.0       | 0.00      |
| 1D-1   | 1.12   | 1.09  | 2.1  | -    | 0.0   | 2.7 | 6.6  | 0.0  | 22%       | +3.6         | +2.6        | +0.0       | 0.00      |
| 2A-4   | 1.13   | 1.09  | 1.5  | -    | 0.0   | 3.2 | 7.4  | 0.0  | 59%       | +2.5         | +2.0        | +0.0       | 0.00      |
| 1E-2   | 1.15   | 1.08  | 2.1  | -    | 0.0   | 3.4 | 5.9  | 0.0  | 41%       | +5.7         | +4.7        | +0.0       | 0.00      |
| 1D-2   | 1.09   | 1.07  | 1.6  | -    | 0.0   | 4.4 | 5.7  | 0.0  | 67%       | +3.5         | +2.7        | +0.0       | 0.00      |
| 1A-1   | 1.05   | 1.06  | 1.9  | -    | 0.0   | 3.4 | 6.0  | 0.0  | 40%       | +1.0         | -0.2        | +0.0       | 0.00      |
| 1E-1   | 1.05   | 1.05  | 2.9  | -    | 0.7   | 3.2 | 3.6  | 0.0  | 48%       | +2.7         | +2.7        | +0.0       | 0.00      |
| 2A-3   | 1.10   | 1.02  | 1.6  | -    | 0.0   | 2.8 | 6.7  | 0.0  | 52%       | +3.3         | +2.1        | +0.0       | 0.00      |

### Resonant peak (Resonance=max samples)

| sample | peak Hz (hw) | peak Hz (before) | peak Hz (after) | height dB (hw) | height dB (before) | height dB (after) |
|--------|--------------|------------------|-----------------|----------------|--------------------|-------------------|
| 1A-2   | 131          | 65               | 65              | -1.5           | +0.0               | +0.0              |
| 1A-3   | 196          | 196              | 196             | -0.1           | -0.8               | -0.8              |
| 1A-5   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1A-6   | 327          | 392              | 392             | -1.9           | -2.8               | -2.8              |
| 1A-8   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1A-9   | 1701         | 1766             | 1766            | -3.9           | -7.6               | -7.6              |
| 1B-1   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1B-2   | 65           | 14782            | 14978           | +0.0           | -138.5             | -140.2            |
| 1B-3   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1B-4   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1B-5   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1C-1   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1C-2   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1C-3   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 1D-3   | 589          | 13474            | 11969           | -10.3          | -142.0             | -140.6            |
| 1D-4   | 981          | 14978            | 14978           | -6.4           | -131.1             | -131.8            |
| 1R-1   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 2A-2   | 131          | 196              | 196             | +0.5           | -3.5               | -3.5              |
| 2A-5   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 2A-6   | 196          | 65               | 65              | -2.3           | +0.0               | +0.0              |
| 2A-7   | 196          | 262              | 262             | +1.3           | -1.1               | -1.1              |
| 2A-9   | 65           | 392              | 392             | +0.0           | -6.6               | -6.6              |
| 2A-12  | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 2A-13  | 65           | 720              | 720             | +0.0           | -10.1              | -10.1             |
| 2A-14  | 1439         | 785              | 785             | -18.3          | -3.7               | -3.7              |
| 2A-16  | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 2B-1   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 2B-2   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 2B-3   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 2B-4   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 2B-5   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 2B-6   | 65           | 65               | 65              | +0.0           | +0.0               | +0.0              |
| 2B-7   | 131          | 65               | 65              | -1.0           | +0.0               | +0.0              |
| 2B-8   | 131          | 65               | 65              | -0.9           | +0.0               | +0.0              |
| 2B-9   | 65           | 65               | 11446           | +0.0           | +0.0               | -142.5            |
| 2B-10  | 65           | 14782            | 14782           | +0.0           | -135.9             | -136.1            |
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

- **1A-1** worst: k17 1112Hz ref -65.4 / sim -71.9, k21 1374Hz ref -72.7 / sim -80.4, k20 1308Hz ref -71.0 / sim -78.4, k18 1177Hz ref -67.5 / sim -74.2. Model excess above 4 kHz: none > 3 dB.
- **1A-2** worst: k25 1635Hz ref -74.7 / sim -80.0, k21 1374Hz ref -68.4 / sim -72.8, k23 1504Hz ref -71.7 / sim -76.6, k22 1439Hz ref -70.1 / sim -74.7. Model excess above 4 kHz: none > 3 dB.
- **1A-3** worst: k23 1504Hz ref -65.8 / sim -72.3, k26 1701Hz ref -70.5 / sim -77.4, k20 1308Hz ref -60.5 / sim -66.7, k15 981Hz ref -49.8 / sim -55.3. Model excess above 4 kHz: none > 3 dB.
- **1A-4** worst: k1 65Hz ref 0.0 / sim 5.8, k36 2355Hz ref -74.1 / sim -79.6, k35 2289Hz ref -73.2 / sim -78.4, k34 2224Hz ref -72.1 / sim -77.3. Model excess above 4 kHz: none > 3 dB.
- **1A-5** worst: k6 392Hz ref -12.3 / sim -9.0, k5 327Hz ref -8.9 / sim -5.7, k2 131Hz ref -4.0 / sim -1.0, k1 65Hz ref 0.0 / sim 3.0. Model excess above 4 kHz: none > 3 dB.
- **1A-6** worst: k7 458Hz ref -7.6 / sim -3.1, k6 392Hz ref -4.6 / sim -1.4, k5 327Hz ref -1.9 / sim -4.7, k8 523Hz ref -9.8 / sim -7.7. Model excess above 4 kHz: none > 3 dB.
- **1A-7** worst: k2 131Hz ref -4.9 / sim 4.8, k3 196Hz ref -8.4 / sim 1.0, k4 262Hz ref -10.7 / sim -2.6, k1 65Hz ref 0.0 / sim 7.7. Model excess above 4 kHz: none > 3 dB.
- **1A-8** worst: k3 196Hz ref -8.2 / sim -3.7, k4 262Hz ref -10.4 / sim -6.0, k2 131Hz ref -4.9 / sim -0.6, k5 327Hz ref -11.9 / sim -7.7. Model excess above 4 kHz: none > 3 dB.
- **1A-9** worst: k87 5690Hz ref -62.2 / sim -57.3, k88 5756Hz ref -62.4 / sim -57.7, k99 6475Hz ref -67.0 / sim -62.4, k112 7326Hz ref -72.5 / sim -68.0. Model excess above 4 kHz: harmonic 5690Hz +4.9dB, harmonic 5756Hz +4.7dB, harmonic 6475Hz +4.5dB, inter-harmonic 4088Hz +3.5dB.
- **1B-1** worst: k6 392Hz ref -8.2 / sim -5.4, k48 3140Hz ref -78.0 / sim -75.3, k49 3205Hz ref -78.8 / sim -76.2, k47 3074Hz ref -77.1 / sim -74.5. Model excess above 4 kHz: none > 3 dB.
- **1B-2** worst: k5 327Hz ref -8.1 / sim -5.9, k2 131Hz ref -4.3 / sim -2.4, k6 392Hz ref -9.5 / sim -7.8, k3 196Hz ref -6.1 / sim -4.5. Model excess above 4 kHz: none > 3 dB.
- **1B-3** worst: k24 1570Hz ref -21.0 / sim -24.0, k30 1962Hz ref -23.3 / sim -26.4, k23 1504Hz ref -20.8 / sim -23.6, k31 2028Hz ref -24.1 / sim -26.8. Model excess above 4 kHz: none > 3 dB.
- **1B-4** worst: k15 981Hz ref -18.4 / sim -23.4, k14 916Hz ref -18.1 / sim -22.7, k13 850Hz ref -17.6 / sim -22.1, k16 1046Hz ref -19.2 / sim -24.0. Model excess above 4 kHz: none > 3 dB.
- **1B-5** worst: k10 654Hz ref -14.9 / sim -9.7, k9 589Hz ref -14.8 / sim -9.7, k8 523Hz ref -14.4 / sim -9.7, k4 262Hz ref -10.4 / sim -5.8. Model excess above 4 kHz: harmonic 4840Hz +3.5dB, harmonic 4775Hz +3.5dB, harmonic 4906Hz +3.4dB.
- **1C-1** worst: k7 458Hz ref -17.4 / sim -21.1, k6 392Hz ref -15.9 / sim -18.5, k5 327Hz ref -14.8 / sim -17.1, k2 131Hz ref -6.0 / sim -8.3. Model excess above 4 kHz: none > 3 dB.
- **1C-2** worst: k16 1046Hz ref -18.8 / sim -22.3, k17 1112Hz ref -19.4 / sim -22.9, k18 1177Hz ref -20.2 / sim -23.5, k15 981Hz ref -18.5 / sim -21.7. Model excess above 4 kHz: none > 3 dB.
- **1C-3** worst: k38 2485Hz ref -21.7 / sim -28.4, k39 2551Hz ref -22.3 / sim -28.9, k37 2420Hz ref -21.4 / sim -27.9, k40 2616Hz ref -23.0 / sim -29.5. Model excess above 4 kHz: none > 3 dB.
- **1D-1** worst: k35 2289Hz ref -74.0 / sim -79.0, k36 2355Hz ref -75.1 / sim -80.1, k34 2224Hz ref -73.0 / sim -77.8, k33 2158Hz ref -71.9 / sim -76.7. Model excess above 4 kHz: none > 3 dB.
- **1D-2** worst: k8 523Hz ref -18.2 / sim -23.8, k9 589Hz ref -19.4 / sim -25.8, k10 654Hz ref -20.6 / sim -27.7, k7 458Hz ref -16.9 / sim -21.3. Model excess above 4 kHz: none > 3 dB.
- **1D-3** worst: k12 785Hz ref -19.9 / sim -17.4, k13 850Hz ref -25.3 / sim -22.8, k33 2158Hz ref -61.5 / sim -63.6, k32 2093Hz ref -60.2 / sim -62.2. Model excess above 4 kHz: none > 3 dB.
- **1D-4** worst: k17 1112Hz ref -16.3 / sim -13.0, k50 3270Hz ref -64.3 / sim -67.3, k49 3205Hz ref -63.2 / sim -66.0, k35 2289Hz ref -48.6 / sim -45.8. Model excess above 4 kHz: none > 3 dB.
- **1E-1** worst: k13 850Hz ref -42.6 / sim -57.4, k11 720Hz ref -37.1 / sim -47.2, k15 981Hz ref -47.8 / sim -70.5, k28 1831Hz ref -69.8 / sim -82.1. Model excess above 4 kHz: none > 3 dB.
- **1E-2** worst: k1 65Hz ref 0.0 / sim 5.6, k36 2355Hz ref -74.6 / sim -80.0, k35 2289Hz ref -73.6 / sim -78.8, k34 2224Hz ref -72.5 / sim -77.7. Model excess above 4 kHz: none > 3 dB.
- **1R-1** worst: k5 327Hz ref -8.9 / sim -5.7, k6 392Hz ref -12.2 / sim -9.0, k2 131Hz ref -4.0 / sim -1.0, k1 65Hz ref 0.0 / sim 2.9. Model excess above 4 kHz: none > 3 dB.
- **2A-1** worst: k21 1374Hz ref -70.2 / sim -76.3, k20 1308Hz ref -68.5 / sim -74.3, k19 1243Hz ref -66.9 / sim -72.2, k18 1177Hz ref -65.4 / sim -70.1. Model excess above 4 kHz: none > 3 dB.
- **2A-2** worst: k5 327Hz ref -18.7 / sim -15.1, k4 262Hz ref -11.3 / sim -9.0, k3 196Hz ref -4.8 / sim -2.9, k2 131Hz ref 0.0 / sim -1.8. Model excess above 4 kHz: none > 3 dB.
- **2A-3** worst: k24 1570Hz ref -72.3 / sim -77.9, k25 1635Hz ref -73.7 / sim -79.6, k23 1504Hz ref -70.8 / sim -76.3, k22 1439Hz ref -69.2 / sim -74.4. Model excess above 4 kHz: none > 3 dB.
- **2A-4** worst: k26 1701Hz ref -71.6 / sim -76.6, k28 1831Hz ref -74.4 / sim -79.6, k27 1766Hz ref -72.9 / sim -78.1, k25 1635Hz ref -70.3 / sim -74.9. Model excess above 4 kHz: none > 3 dB.
- **2A-5** worst: k30 1962Hz ref -75.5 / sim -78.9, k31 2028Hz ref -76.6 / sim -80.4, k29 1897Hz ref -74.3 / sim -77.6, k28 1831Hz ref -73.0 / sim -76.1. Model excess above 4 kHz: none > 3 dB.
- **2A-6** worst: k4 262Hz ref -7.5 / sim -3.5, k5 327Hz ref -11.4 / sim -9.0, k6 392Hz ref -16.2 / sim -14.0, k10 654Hz ref -35.7 / sim -33.6. Model excess above 4 kHz: none > 3 dB.
- **2A-7** worst: k3 196Hz ref 0.0 / sim -3.4, k20 1308Hz ref -56.7 / sim -60.0, k4 262Hz ref -4.4 / sim -1.1, k21 1374Hz ref -58.7 / sim -61.9. Model excess above 4 kHz: none > 3 dB.
- **2A-8** worst: k41 2682Hz ref -75.6 / sim -79.9, k40 2616Hz ref -74.7 / sim -78.8, k39 2551Hz ref -73.8 / sim -77.8, k38 2485Hz ref -72.8 / sim -76.8. Model excess above 4 kHz: none > 3 dB.
- **2A-9** worst: k6 392Hz ref -8.9 / sim -4.4, k7 458Hz ref -11.7 / sim -8.3, k2 131Hz ref -4.1 / sim -1.8, k1 65Hz ref 0.0 / sim 2.2. Model excess above 4 kHz: none > 3 dB.
- **2A-10** worst: k1 65Hz ref 0.0 / sim 7.6, k2 131Hz ref -4.9 / sim 2.6, k3 196Hz ref -8.3 / sim -2.7, k56 3663Hz ref -75.6 / sim -79.1. Model excess above 4 kHz: none > 3 dB.
- **2A-11** worst: k2 131Hz ref -4.7 / sim -0.2, k3 196Hz ref -7.6 / sim -3.2, k1 65Hz ref 0.0 / sim 4.1, k4 262Hz ref -9.3 / sim -5.3. Model excess above 4 kHz: none > 3 dB.
- **2A-12** worst: k3 196Hz ref -7.6 / sim -3.9, k2 131Hz ref -4.6 / sim -1.1, k4 262Hz ref -9.2 / sim -5.8, k1 65Hz ref 0.0 / sim 3.3. Model excess above 4 kHz: none > 3 dB.
- **2A-13** worst: k11 720Hz ref -11.1 / sim -7.8, k3 196Hz ref -7.8 / sim -5.0, k12 785Hz ref -12.8 / sim -10.1, k2 131Hz ref -4.7 / sim -2.1. Model excess above 4 kHz: none > 3 dB.
- **2A-14** worst: k12 785Hz ref -7.8 / sim -1.9, k13 850Hz ref -9.5 / sim -5.9, k10 654Hz ref -4.9 / sim -7.8, k74 4840Hz ref -79.4 / sim -76.7. Model excess above 4 kHz: none > 3 dB.
- **2A-15** worst: k3 196Hz ref -8.2 / sim -2.8, k4 262Hz ref -10.4 / sim -5.1, k2 131Hz ref -4.9 / sim 0.3, k5 327Hz ref -11.9 / sim -6.9. Model excess above 4 kHz: none > 3 dB.
- **2A-16** worst: k3 196Hz ref -8.2 / sim -3.9, k4 262Hz ref -10.4 / sim -6.2, k2 131Hz ref -4.9 / sim -0.8, k5 327Hz ref -12.0 / sim -8.0. Model excess above 4 kHz: none > 3 dB.
- **2B-1** worst: k6 392Hz ref -8.6 / sim -5.9, k7 458Hz ref -9.9 / sim -7.7, k2 131Hz ref -4.3 / sim -2.4, k1 65Hz ref 0.0 / sim 1.7. Model excess above 4 kHz: none > 3 dB.
- **2B-2** worst: k2 131Hz ref -4.2 / sim -2.4, k5 327Hz ref -8.6 / sim -6.8, k4 262Hz ref -6.6 / sim -5.0, k73 4775Hz ref -79.1 / sim -77.5. Model excess above 4 kHz: none > 3 dB.
- **2B-3** worst: k86 5625Hz ref -79.3 / sim -77.1, k87 5690Hz ref -79.7 / sim -77.6, k85 5560Hz ref -78.8 / sim -76.7, k29 1897Hz ref -34.8 / sim -32.7. Model excess above 4 kHz: none > 3 dB.
- **2B-4** worst: k32 2093Hz ref -37.2 / sim -32.5, k31 2028Hz ref -35.6 / sim -30.9, k30 1962Hz ref -34.0 / sim -29.5, k33 2158Hz ref -38.7 / sim -34.2. Model excess above 4 kHz: harmonic 4251Hz +3.2dB, harmonic 5102Hz +3.2dB, harmonic 5036Hz +3.2dB.
- **2B-5** worst: k37 2420Hz ref -37.2 / sim -34.1, k36 2355Hz ref -35.8 / sim -32.7, k38 2485Hz ref -38.5 / sim -35.6, k21 1374Hz ref -19.2 / sim -22.0. Model excess above 4 kHz: none > 3 dB.
- **2B-6** worst: k23 1504Hz ref -19.9 / sim -22.6, k26 1701Hz ref -21.4 / sim -23.9, k27 1766Hz ref -21.7 / sim -24.3, k24 1570Hz ref -20.6 / sim -23.0. Model excess above 4 kHz: none > 3 dB.
- **2B-7** worst: k14 916Hz ref -47.2 / sim -49.5, k15 981Hz ref -49.9 / sim -52.2, k13 850Hz ref -44.5 / sim -46.7, k16 1046Hz ref -52.6 / sim -54.7. Model excess above 4 kHz: none > 3 dB.
- **2B-8** worst: k15 981Hz ref -48.0 / sim -50.0, k16 1046Hz ref -50.5 / sim -52.5, k14 916Hz ref -45.4 / sim -47.3, k17 1112Hz ref -53.0 / sim -54.9. Model excess above 4 kHz: none > 3 dB.
- **2B-9** worst: k10 654Hz ref -27.6 / sim -24.8, k9 589Hz ref -22.8 / sim -20.2, k11 720Hz ref -31.2 / sim -29.1, k37 2420Hz ref -77.8 / sim -76.3. Model excess above 4 kHz: none > 3 dB.
- **2B-10** worst: k19 1243Hz ref -37.0 / sim -29.0, k20 1308Hz ref -38.9 / sim -31.2, k18 1177Hz ref -34.7 / sim -27.1, k17 1112Hz ref -32.5 / sim -25.4. Model excess above 4 kHz: none > 3 dB.
- **2B-11** worst: k3 196Hz ref -8.2 / sim -4.0, k4 262Hz ref -10.4 / sim -6.3, k2 131Hz ref -4.9 / sim -0.9, k5 327Hz ref -12.0 / sim -8.1. Model excess above 4 kHz: harmonic 6214Hz +3.3dB, harmonic 6017Hz +3.3dB, harmonic 6148Hz +3.3dB.
- **2B-12** worst: k3 196Hz ref -8.2 / sim -4.0, k4 262Hz ref -10.5 / sim -6.3, k2 131Hz ref -4.9 / sim -0.9, k5 327Hz ref -12.0 / sim -8.1. Model excess above 4 kHz: harmonic 6868Hz +3.3dB, harmonic 6475Hz +3.3dB, harmonic 6606Hz +3.3dB.
- **2B-13** worst: k127 8307Hz ref -69.0 / sim -63.8, k126 8241Hz ref -68.6 / sim -63.4, k125 8176Hz ref -68.2 / sim -63.1, k124 8110Hz ref -67.9 / sim -62.7. Model excess above 4 kHz: harmonic 8372Hz +5.2dB, harmonic 8307Hz +5.2dB, harmonic 8437Hz +5.2dB, inter-harmonic 4480Hz +4.8dB.
- **2B-14** worst: k13 850Hz ref -16.0 / sim -11.3, k3 196Hz ref -8.2 / sim -3.7, k4 262Hz ref -10.4 / sim -6.0, k14 916Hz ref -15.8 / sim -11.4. Model excess above 4 kHz: harmonic 7652Hz +5.4dB, harmonic 7718Hz +5.4dB, harmonic 7587Hz +5.4dB.
- **2C-1** worst: k9 589Hz ref -17.3 / sim -21.6, k7 458Hz ref -15.6 / sim -19.4, k8 523Hz ref -16.7 / sim -20.2, k10 654Hz ref -18.2 / sim -23.8. Model excess above 4 kHz: none > 3 dB.
- **2C-2** worst: k11 720Hz ref -17.8 / sim -21.9, k12 785Hz ref -18.8 / sim -22.8, k9 589Hz ref -16.6 / sim -19.8, k10 654Hz ref -17.5 / sim -20.6. Model excess above 4 kHz: none > 3 dB.
- **2C-3** worst: k14 916Hz ref -18.6 / sim -22.4, k13 850Hz ref -17.9 / sim -21.6, k15 981Hz ref -19.4 / sim -22.8, k11 720Hz ref -16.8 / sim -20.1. Model excess above 4 kHz: none > 3 dB.
- **2C-4** worst: k20 1308Hz ref -19.6 / sim -23.4, k21 1374Hz ref -19.9 / sim -23.9, k19 1243Hz ref -19.5 / sim -23.0, k18 1177Hz ref -19.0 / sim -22.5. Model excess above 4 kHz: none > 3 dB.
- **2C-5** worst: k28 1831Hz ref -21.3 / sim -25.4, k27 1766Hz ref -21.0 / sim -25.1, k25 1635Hz ref -20.2 / sim -24.3, k26 1701Hz ref -20.6 / sim -24.7. Model excess above 4 kHz: none > 3 dB.
- **2C-6** worst: k33 2158Hz ref -21.5 / sim -26.5, k34 2224Hz ref -21.8 / sim -26.8, k32 2093Hz ref -21.2 / sim -26.1, k31 2028Hz ref -20.9 / sim -25.8. Model excess above 4 kHz: none > 3 dB.
- **2R-1** worst: k5 327Hz ref -8.9 / sim -5.7, k6 392Hz ref -12.1 / sim -9.0, k44 2878Hz ref -76.4 / sim -79.4, k43 2812Hz ref -75.5 / sim -78.4. Model excess above 4 kHz: none > 3 dB.

## Suspicious samples

- **2C-4**: spectrum is within 1.2 dB of 2C-5 although the decay label differs -- one of the two labels is probably wrong
- **2B-11**: spectrum is within 1.2 dB of 2B-12 although the envMod label differs -- one of the two labels is probably wrong
- **2B-3**: spectrum is within 1.5 dB of 2B-4 although the envMod label differs -- one of the two labels is probably wrong
- **2A-6**: spectrum is within 1.0 dB of 2B-7 although the cutoff/decay label differs -- one of the two labels is probably wrong
- **1D-1**: spectrum is within 1.0 dB of 1E-2 although the cutoff/envMod/decay/accent label differs -- one of the two labels is probably wrong
- **1A-4**: spectrum is within 1.2 dB of 1D-1 although the cutoff/envMod/decay/accent label differs -- one of the two labels is probably wrong
- **1A-3**: spectrum is within 1.4 dB of 2B-7 although the resonance/decay label differs -- one of the two labels is probably wrong
- **1D-4**: error 3.52 is 2.1x the median
- **2A-14**: error 3.18 is 1.9x the median
- **2A-7**: error 3.09 is 1.8x the median
- **2A-2**: error 2.96 is 1.8x the median
- **1B-5**: error 2.89 is 1.7x the median
- **2A-6**: error 2.87 is 1.7x the median
- **2A-5**: error 2.77 is 1.6x the median
- **2A-9**: error 2.71 is 1.6x the median
- **1A-3**: error 2.69 is 1.6x the median
- **2B-14**: error 2.64 is 1.6x the median
- **2B-7**: error 2.63 is 1.6x the median
- **1A-6**: error 2.58 is 1.5x the median

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

| parameter           | before  | after   | sensitivity |  |
|---------------------|---------|---------|-------------|--|
| oscSquareLevel      | 0.38901 | 0.43479 | 0.000       |  |
| accentVcaDepth      | 2.3943  | 2.3057  | 0.000       |  |
| vcaResTapRatio      | 1.6824  | 1.9512  | 0.000       |  |
| vcaCutoffLevelDb    | -1.9418 | -1.652  | 0.000       |  |
| vcaCutoffLevelResDb | 1.2048  | 1.0799  | 0.000       |  |

Timing: note-on offset 0.00 ms (relative to each clip's measured note-on), gate length 1300.0 ms.

## Applying the result

Paste `synth_parameters.txt` into `acidus::SynthParameters` (src/core/SynthEngine.hpp) or rerun with `--apply`.

Audio: `renders/` (hardware / before / after for the 40 worst samples).
