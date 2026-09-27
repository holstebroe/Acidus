# Reference resources

The `test/resources` folder contains reference samples from external Roland
TB-303 sources. These samples are intended for calibrating the plugin's
parameters to match the hardware reference as closely as possible.

The samples are currently all single notes played at 142 BPM. The file name
encodes the approximate parameters used to record the source sample.

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
