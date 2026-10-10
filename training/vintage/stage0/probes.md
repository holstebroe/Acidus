# Stage 0: separability probes

WAV offset against the MIDI: +3.9 ms. `source` and `Acidus` are the interaction
measured in each; `left over` is what the source has that Acidus's structure doesn't
(peak tracks: median over frames, semitones). P04, P08, P11, P12 should be zero in the source;
P08 and P11 count only what exceeds P12, the source's own note-to-note difference.

| probe | what | source | Acidus | left over | limit | verdict | if it interacts |
|---|---|---|---|---|---|---|---|
| P01 | settled peak shift C1 -> C2 (key tracking) | 0.11 | -0.57 | 0.68 st | < 1 st | ok |  |
| P02 | pitch x Env Mod, peak track | 0.64 | 0.26 | 0.79 st | < 1 st | ok |  |
| P03 | pitch x accent, peak track | 2.28 | 0.50 | 2.18 st | < 1 st | INTERACTS | repeat 3A at C1 |
| P03 | pitch x accent, level | -4.78 | 1.31 | 6.09 dB | < 1 dB | INTERACTS | repeat 3A at C1 |
| P04 | Decay knob on accented notes | 0.33 | 0.00 | 0.33 st | < 1 st | ok |  |
| P05 | Decay tau vs Env Mod, tau change % | 11.87 | n/a | n/a % | < 15 % | inconclusive | cross 2B and 2C (Env Mod x Decay) |
| P06 | Decay tau vs Cutoff, tau change % | 15.50 | 18.34 | 2.84 % | < 15 % | ok |  |
| P07 | Resonance x Env Mod, peak track | 0.63 | 0.24 | 0.60 st | < 1 st | ok |  |
| P08 | gate length changes the note before gate-off | 6.23 | 0.00 | 2.09 dB | < 1 dB | INTERACTS | the source looks ahead to the gate (or isn't one-shot): keep gates the same everywhere |
| P09 | waveform x Env Mod, peak track | 1.30 | 0.99 | 0.89 st | < 2 st | ok |  |
| P10 | accent sweep vs Cutoff, peak track | 0.85 | 0.53 | 1.39 st | < 1 st | INTERACTS | repeat 3A at Cutoff 75 |
| P11 | Accent knob on an unaccented note | 4.17 | 0.00 | 0.04 dB | < 0.5 dB | ok |  |
| P12 | the same note twice (repeatability) | 4.14 | 0.00 | 4.14 dB | < 0.5 dB | INTERACTS | the source is not repeatable: turn off noise / drift / analog variation, see below |

Source noise floor between notes: loudest band -172 dB against the peak sample (spectrogram cells within 10 dB of it are not compared).
P12, second note against the first: peak level +0.00 dB; resonant peak early +0.02 st; resonant peak settled +0.28 st; pitch -0.0 cents; difference over the note: 0-150 ms 3.5 dB, 150-500 ms 3.5 dB, 500-1300 ms 3.3 dB; most different around 193 Hz (5.3 dB), 236 Hz (4.0 dB), 448 Hz (3.7 dB).
The fit needs a repeatable source: two identical notes should match to well under
0.5 dB. A level difference points at drift or random variation; a pitch
difference at oscillator drift; differences only in the top bands at noise or
random phase. A resonant peak that moves between the two notes, with level and
pitch equal, is a filter that varies per note (an "analog" or component-
tolerance option): turn it off. A difference near the filter's frequency that fades over the
note is a knob still moving when the note starts: the first P12 note follows
a knob change, the second does not. Check that the emulation (or the DAW's
MIDI learn) does not smooth CC changes, or regenerate the MIDI files (the
knob CCs now come 1.2 s before each note) and render again. P08 and P11
only count what they exceed P12 by, so they are unreliable until P12 passes.

"inconclusive": no clear resonant peak in one of the corners, or (P05, P06) a Decay
time constant longer than the gate or a peak lost for most of the note. Nothing
contradicts the 303 structure there; keep the plan as it is.
What to add when a pair interacts: docs/EMULATION_TRAINING_PLAN.md, section 3.
