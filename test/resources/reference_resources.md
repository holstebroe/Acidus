# Reference resources

The `test/resources` folder contains reference samples from external Roland
TB-303 sources. These samples are intended for calibrating the plugin's
parameters to match the hardware reference as closely as possible.

The samples are currently all single notes played at 142 BPM. The file name
encodes the approximate parameters used to record the source sample.

The `x0x-reference/` subfolder holds a second, much larger set (the
dinsync.info recordings: 25 files x 16 notes, one systematic knob sweep).
Those files carry several notes each and follow their own layout, described
in [`x0x-reference/x0x_reference.md`](x0x-reference/x0x_reference.md); the
file-name grammar below applies only to the single-note files in this
folder.

## File name format

Example: `303_saw-A2t-42c0r0e0d1a0.wav`

| Token   | Meaning                                          |
|---------|---------------------------------------------------|
| `303`   | Source id (hardware Roland TB-303)                 |
| `saw`   | Oscillator waveform mode                           |
| `A2`    | Note played (A2)                                   |
| `t-42`  | Tuning, 42 cents flat (measured, see below)         |
| `c0`    | Cutoff knob at minimum position                     |
| `r0`    | Resonance knob at minimum position                  |
| `e0`    | Env Mod knob at minimum position                    |
| `d1`    | Decay knob at maximum position                      |
| `a0`    | Accent knob at minimum position                     |

### Grammar

The name (excluding extension) follows this grammar, given in ABNF
(RFC 5234). Terminals are case-sensitive.

```abnf
filename      = source-id "_" waveform "-" note tuning
                cutoff resonance env-mod decay accent ".wav"

source-id     = "303"

waveform      = "saw" / "square"

note          = note-letter [ sharp ] octave
note-letter   = "A" / "B" / "C" / "D" / "E" / "F" / "G"
sharp         = "#"
octave        = DIGIT                ; 0-9

tuning        = "t" [ "-" ] cents
cents         = 1*DIGIT              ; absolute offset in cents;
                                      ; a leading "-" means flat, no sign means sharp/in-tune

cutoff        = "c" knob-position
resonance     = "r" knob-position
env-mod       = "e" knob-position
decay         = "d" knob-position
accent        = "a" knob-position

knob-position = DIGIT                ; see "Knob position encoding" below

DIGIT         = %x30-39               ; "0"-"9"
```

### Field reference

- **source-id** — identifier of the hardware unit the sample was recorded
  from. Currently always `303`.
- **waveform** — oscillator mode: `saw` or `square`.
- **note** — the note played, e.g. `A2` or `A#2`.
- **tuning** — the source's tuning offset from equal temperament, in cents.
  `t-42` means 42 cents flat; a value with no leading `-` means sharp (or
  in tune, at `t0`). The values are measured from each recording (mean of
  a harmonic-comb f0 fit and a time-domain period estimate, which agree
  within ~2 cents), not the nominal Tune setting: most samples sit at
  -40..-43 cents, but `A1 c0` and `D2 c0` are ~-50 cents, i.e. ~8 cents
  flatter than the same notes recorded at `c1`, so they were most likely
  recorded in a different sitting.
- **cutoff**, **resonance**, **env-mod**, **decay**, **accent** — the
  corresponding front-panel knob position, encoded as described below.

### Knob position encoding

Each knob field is a single digit giving the knob's position as a fraction
of its full travel:

| Digit | Position                          |
|-------|------------------------------------|
| `0`   | Minimum                            |
| `1`   | Maximum                            |
| `5`   | Halfway between minimum and maximum |

Only `0`, `1` and `5` are used by the samples currently in this folder;
other digits are reserved for intermediate positions that may be added
later.

## Label corrections (2026-09-27)

Five files were renamed after their audio contradicted the knob label. Each
correction was checked three ways: the time course of the resonant peak, a
per-sample free-knob fit (`tools/calibrate_reference.py` label check), and
the match error with the corrected label.

| Original name | New name | Evidence |
|---|---|---|
| `303_saw-A1t-43c1r1e0d1a0` | `303_saw-A1t-43c1r1e0d0a0` | Resonant peak glides 3050 → 1930 Hz within 140 ms: the 68 ms MEG of Decay min, not the 1.07 s of Decay max (the `t-40` A2 note at the same label only moves 3330 → 3000 Hz). Match error 9.1 → 5.9. |
| `303_saw-D2t-43c1r1e0d1a0` | `303_saw-D2t-43c1r1e0d0a0` | Same glide, 3050 → 1900 Hz. Match error 8.3 → 3.6; peak-shape error 11 → 1.3 dB. |
| `303_saw-A1t-51c0r1e0d1a0` | `303_saw-A1t-51c0r1e0d0a0` | Peak falls ≈ 320 → 215 Hz over the note, the Env Mod law's 0.74 oct for a fully decayed MEG at Cutoff min; the `t-41` c0r1 notes stay put. Match error 6.1 → 3.7. |
| `303_saw-D2t-50c0r1e0d1a0` | `303_saw-D2t-50c0r1e0d0a0` | Same, ≈ 350 → 215 Hz. Match error 4.7 → 2.5. |
| `303_saw-D3t-41c1r1e0d1a1` | `303_saw-D3t-41c1r1e0d1a0` | Not an accented step: level is flat and the peak sits at 3.0–3.15 kHz like the unaccented A2 c1r1 note, where the accented D2 c1r1 note is +7.8 dB louder and peaks at 6.2 kHz. The Accent knob does nothing on an unaccented step, so `a0` describes it exactly. Match error 10.8 → 3.0. |

So all the `t-43` and `t-50/-51` notes (the other sittings) were most likely
recorded with Decay at minimum. An accented step with Accent at 0 would
sound the same (it also shorts the Decay pot), so either reading fits.
