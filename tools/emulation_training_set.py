#!/usr/bin/env python3
"""Write the staged training set for calibrating against a software 303.

The set is explained in docs/EMULATION_TRAINING_PLAN.md. For a source id
<src> this writes:

    <out>/<src>-stage0.mid ... -stage4.mid
                         one MIDI file per stage: import it into the DAW,
                         MIDI-learn the knobs, render the whole stage as one WAV
    <out>/<src>_sheet.csv        every single note of stages 0-3, for reading
    <out>/<src>_sequences.csv    stage 4 (slides, accent runs, retriggers)

Then fit the rendered stage with tools/fit_stage.py.

The MIDI files run at 125 BPM, 480 PPQ, so one tick is exactly 1 ms. Accent
is velocity 127, normal 100 (Burette's convention); a slide is the next
note-on 10 ms before the previous note-off. Knobs are sent as Control Changes
300 ms before each note (or sequence), in silence. The default map is
Acidus's own (Cutoff 71, Resonance 72, Env Mod 73, Decay 74, Accent 22,
Waveform 23): MIDI-learn the emulation's knobs to those, and the same files
drive both plugins. A marker names each set and a text event at every note
records its knobs, so the MIDI file alone describes the stage.

CC is 7-bit, so 25 % cannot be sent exactly (31.75 / 127): every knob is
snapped to the nearest CC value and the text events record the exact
position that value sets (25 % -> 32 -> 25.197 %). Check that the emulation's
MIDI learn maps CC 0..127 linearly onto the whole knob travel.

    python3 tools/emulation_training_set.py <src> --out test/resources/<src>
"""
import argparse
import csv
import struct
from pathlib import Path

NOTE_ON_MS = 500           # first note-on in every file
SLOT_MS = 3000             # note-on to next note-on, minimum
TAIL_MS = 1700             # silence after the gate before the next note-on
GATE_MS = 1300             # standard gate (the dinsync set: 1330 ms)
OVERLAP_MS = 10            # slide: next note-on this long before the note-off
STEP_MS = 120              # one 16th at 125 BPM
VEL_ACCENT, VEL_NORMAL = 127, 100

BASE = dict(note="C2", waveform="saw", accent="no", cutoff=50, resonance=0,
            envmod=0, decay=0, accent_knob=0, gate_ms=GATE_MS)

NOTE_NAMES = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]


def midi_of(name):
    """C2 = MIDI 36, the convention of the calibration tools."""
    pitch, octave = name[:-1], int(name[-1])
    return 12 * (octave + 1) + NOTE_NAMES.index(pitch)


def row(purpose, **kw):
    r = dict(BASE)
    r.update(kw)
    r["purpose"] = purpose
    return r


def grid(purpose, fixed, **axes):
    """All combinations of the axes (first axis outermost), on top of fixed."""
    rows = [dict(fixed)]
    for key, values in axes.items():
        rows = [dict(r, **{key: v}) for r in rows for v in values]
    return [row(purpose, **r) for r in rows]


def note_sets():
    """Ordered (stage, set, purpose, rows). Later stages never repeat a note of
    an earlier stage, so each stage is a separate render batch."""
    s = []
    common = dict(cutoff=25, envmod=50, decay=50)
    # --- Stage 0: separability probes (2x2 corners) -------------------------
    # The resonant peak is tracked, so these sit where it is clear: Resonance
    # 75-100 %, Cutoff 50-75 %, and C1 / C2, whose dense harmonics let the
    # peak be placed between them.
    tr = dict(cutoff=75, resonance=75)
    s.append((0, "P01", "pitch x cutoff: key tracking of the settled peak",
              grid("P01", dict(resonance=100), note=["C1", "C2"], cutoff=[50, 100])))
    s.append((0, "P02", "pitch x envmod: sweep depth in octaves",
              grid("P02", dict(tr, decay=50), note=["C1", "C2"], envmod=[25, 100])))
    s.append((0, "P03", "pitch x accent: accent sweep and level",
              grid("P03", dict(tr, resonance=100, envmod=0, decay=50, accent_knob=100),
                   note=["C1", "C2"], accent=["no", "yes"])))
    s.append((0, "P04", "decay x accent: Decay is shorted on accented steps",
              grid("P04", dict(tr, envmod=75, accent_knob=100), accent=["no", "yes"], decay=[0, 100])))
    s.append((0, "P05", "envmod x decay: depth from Env Mod only, tau from Decay only",
              grid("P05", dict(tr, gate_ms=2500), envmod=[50, 75], decay=[25, 75])))
    s.append((0, "P06", "cutoff x decay: tau does not depend on Cutoff",
              grid("P06", dict(resonance=75, envmod=50, gate_ms=2500), cutoff=[75, 100], decay=[25, 75])))
    s.append((0, "P07", "resonance x envmod: depth does not depend on Resonance",
              grid("P07", dict(cutoff=75, decay=50), resonance=[75, 100], envmod=[25, 100])))
    s.append((0, "P08", "gate x accent: nothing looks ahead to the gate; release shape",
              grid("P08", dict(tr, envmod=100, decay=75, accent_knob=100),
                   gate_ms=[150, 1300], accent=["no", "yes"])))
    s.append((0, "P09", "waveform x envmod: same sweep for saw and square",
              grid("P09", dict(tr, decay=50), waveform=["saw", "square"], envmod=[25, 100])))
    s.append((0, "P10", "accent x cutoff: the accent sweep adds in octaves",
              grid("P10", dict(resonance=100, envmod=0, decay=50, accent_knob=100),
                   cutoff=[50, 100], accent=["no", "yes"])))
    s.append((0, "P11", "Accent knob on an unaccented step does nothing",
              grid("P11", dict(common, resonance=75), accent_knob=[0, 100])))
    s.append((0, "P12", "determinism: the same note twice",
              [row("P12", cutoff=50, resonance=75, envmod=50, decay=50)] * 2))
    # --- Stage 1: the parameters you hear first ------------------------------
    s.append((1, "1A", "static filter, coarse: Cutoff x Resonance at 0/50/100",
              grid("1A", {}, cutoff=[0, 50, 100], resonance=[0, 50, 100])))
    s.append((1, "1B", "Env Mod depth: 0/50/100 at Cutoff 50, 100 at Cutoff 0 and 100",
              grid("1B", dict(resonance=75, decay=50), envmod=[0, 50, 100])
              + grid("1B", dict(resonance=75, decay=50, envmod=100), cutoff=[0, 100])))
    s.append((1, "1C", "Decay range: 0/50/100, long gate",
              grid("1C", dict(cutoff=25, resonance=75, envmod=100, gate_ms=2500), decay=[0, 50, 100])))
    s.append((1, "1D", "accent amount: knob 100 at Resonance 0/100, with unaccented twins",
              grid("1D", dict(common, accent_knob=100), resonance=[0, 100], accent=["no", "yes"])))
    s.append((1, "1E", "square level and the long VEG decay",
              [row("1E", waveform="square"), row("1E", gate_ms=4000)]))
    # --- Stage 2: knob laws (the curve between the end points) ---------------
    s.append((2, "2A", "static filter, rest of the 5x5 Cutoff x Resonance grid",
              [r for r in grid("2A", {}, cutoff=[0, 25, 50, 75, 100], resonance=[0, 25, 50, 75, 100])
               if not (r["cutoff"] in (0, 50, 100) and r["resonance"] in (0, 50, 100))]))
    s.append((2, "2B", "Env Mod law: fine at Cutoff 50, coarse at Cutoff 0 and 100",
              grid("2B", dict(cutoff=50, resonance=75, decay=50), envmod=[25, 60, 70, 75, 80, 90])
              + grid("2B", dict(resonance=75, decay=50), cutoff=[0, 100], envmod=[0, 25, 50, 75])))
    s.append((2, "2C", "Decay law, long gate",
              grid("2C", dict(cutoff=25, resonance=75, envmod=100, gate_ms=2500), decay=[10, 25, 40, 60, 75, 90])))
    # --- Stage 3: detail ---------------------------------------------------------
    s.append((3, "3A", "accent: Accent knob x Resonance (the C13 network), one square",
              [r for r in grid("3A", dict(common, accent="yes"), resonance=[0, 50, 100],
                               accent_knob=[0, 25, 50, 75, 100])
               if not (r["accent_knob"] == 100 and r["resonance"] in (0, 100))]
              + [row("3A", resonance=50, accent_knob=100, **common),
                 row("3A", waveform="square", cutoff=50, resonance=75, envmod=50, decay=50,
                     accent="yes", accent_knob=100)]))
    s.append((3, "3B", "square through the filter",
              [r for r in grid("3B", dict(waveform="square"), cutoff=[0, 50, 100], resonance=[0, 100])
               if not (r["cutoff"] == 50 and r["resonance"] == 0)]))
    s.append((3, "3C", "pitch: octave scale, square duty law, low-end coupling",
              grid("3C", dict(cutoff=100), waveform=["saw", "square"], note=["C1", "C3", "C4"])))
    s.append((3, "3D", "VCA: short gates and release, unaccented and accented; accented long gate",
              grid("3D", dict(accent_knob=100), accent=["no", "yes"], gate_ms=[30, 60, 120, 250])
              + [row("3D", accent="yes", accent_knob=100, gate_ms=4000)]))
    # --- Anchor: one identical note per stage checks the gain chain ----------
    for stage in (1, 2, 3):
        s.append((stage, f"{stage}R", "anchor: same note in every stage, render it last",
                  [row(f"{stage}R", cutoff=50, resonance=50)]))
    return s


def sequence_sets():
    """Ordered (set, purpose, [(seq knobs, [(note, on_ms, off_ms, accent)])])."""
    clean = dict(cutoff=100, resonance=0, envmod=0, decay=0, accent_knob=0)

    def slide(a, b, hold=600):
        return [(a, 0, hold + OVERLAP_MS, False), (b, hold, 2 * hold, False)]

    def run(notes, accents, step=STEP_MS, gate=60, slid=False):
        ev = []
        for i, (n, acc) in enumerate(zip(notes, accents)):
            on = i * step
            last = i == len(notes) - 1
            off = on + (600 if last and slid else step + OVERLAP_MS if slid else gate)
            ev.append((n, on, off, acc))
        return ev

    s = []
    s.append(("Q1", "slide pitch law: interval and direction (RC vs constant rate)",
              [(clean, slide(a, b)) for a, b in
               [("C2", "C#2"), ("C2", "D#2"), ("C2", "G2"), ("C2", "C3"), ("C3", "C2"), ("C1", "C3")]]))
    s.append(("Q2", "slide x filter: same glide under a resonant, swept filter",
              [(dict(cutoff=25, resonance=75, envmod=50, decay=50, accent_knob=0), slide("C2", "C3"))]))
    s.append(("Q3", "slide chain: a glide that starts mid-glide",
              [(clean, run(["C2", "D#2", "G2", "C3"], [False] * 4, slid=True))]))
    env = dict(cutoff=25, resonance=75, envmod=100, decay=75, accent_knob=100)
    s.append(("Q4", "slide x envelopes: no retrigger; accent switched mid-envelope",
              [(env, [("C2", 0, 370, a), ("C3", 360, 1300, b)])
               for a, b in [(False, False), (False, True), (True, False)]]))
    acc = dict(cutoff=25, resonance=100, envmod=50, decay=50, accent_knob=100)
    four = ["C2"] * 4
    s.append(("Q5", "accent stacking on C13: runs of accents and spacing",
              [(acc, run(four, [True] * 4)),
               (dict(acc, resonance=0), run(four, [True] * 4)),
               (acc, run(four, [True] * 4, step=2 * STEP_MS)),
               (acc, run(four, [False, True, False, True]))]))
    s.append(("Q6", "MEG retrigger from a partly charged state",
              [(dict(cutoff=25, resonance=75, envmod=100, decay=100, accent_knob=0),
                run(four, [False] * 4))]))
    return [(4,) + x for x in s]


KNOB_COLS = ("cutoff", "resonance", "envmod", "decay", "accent_knob")
CC_LEAD_MS = 300


# Acidus's own CC map (AcidusClap.hpp): MIDI-learn the emulation to the same
# numbers and one set of MIDI files drives both plugins.
DEFAULT_CC = "cutoff=71,resonance=72,envmod=73,decay=74,accent_knob=22,waveform=23"


def parse_cc(text):
    if text == "default":
        text = DEFAULT_CC
    if text == "none":
        return {}
    cc = {}
    for item in filter(None, text.split(",")):
        key, num = item.split("=")
        if key not in KNOB_COLS + ("waveform",):
            raise SystemExit(f"--cc: unknown control {key!r}")
        cc[key] = int(num)
    return cc


def snap(knobs, cc):
    """Knob percents as the CC values the MIDI sends will set them."""
    return {k: (round(round(v * 1.27) / 1.27, 3) if k in cc else v) if k in KNOB_COLS else v
            for k, v in knobs.items()}


def cc_events(knobs, at_ms, cc):
    """(tick, data) Control Changes that set the knobs at at_ms."""
    ev = []
    for key, num in cc.items():
        val = (127 if knobs["waveform"] == "square" else 0) if key == "waveform" else round(knobs[key] * 1.27)
        ev.append((max(0, at_ms - CC_LEAD_MS), bytes([0xB0, num, val])))
    return ev


def write_midi(path, events, end_ms, controls=(), texts=()):
    """Type-0 SMF, 125 BPM, 480 PPQ (1 tick = 1 ms). events: (midi, on, off, vel);
    controls: (tick, data) Control Changes; texts: (tick, meta type, text)."""
    msgs = [(t, -2, bytes([0xFF, kind, len(txt)]) + txt.encode()) for t, kind, txt in texts]
    msgs += [(t, -1, data) for t, data in controls]
    for m, on, off, vel in events:
        msgs.append((on, 1, bytes([0x90, m, vel])))
        msgs.append((off, 0, bytes([0x80, m, 0])))       # note-off sorts first at equal ticks
    msgs.sort(key=lambda x: (x[0], x[1]))

    def vlq(n):
        out = [n & 0x7F]
        while n > 0x7F:
            n >>= 7
            out.insert(0, (n & 0x7F) | 0x80)
        return bytes(out)

    trk = bytearray(vlq(0) + b"\xff\x51\x03" + (480000).to_bytes(3, "big"))   # 480000 us / quarter
    trk += vlq(0) + b"\xff\x58\x04\x04\x02\x18\x08"
    now = 0
    for t, _, data in msgs:
        trk += vlq(t - now) + data
        now = t
    trk += vlq(max(0, end_ms - now)) + b"\xff\x2f\x00"
    path.write_bytes(b"MThd" + struct.pack(">IHHH", 6, 0, 1, 480) + b"MTrk" + struct.pack(">I", len(trk)) + trk)


def note_text(name, r):
    """The text event that makes a stage MIDI file self-describing: the fitter
    (tools/fit_stage.py) reads the knobs from it, whatever the CC map."""
    knobs = " ".join(f"{k}={r[k]:g}" for k in KNOB_COLS)
    return f"{name} {r['waveform']} {knobs}"


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("source_id")
    ap.add_argument("--out", default=".", help="output folder")
    ap.add_argument("--cc", type=parse_cc, default=parse_cc("default"),
                    help="knob CCs: 'default' (Acidus's map, for MIDI learn; the default), 'none' "
                         "(set knobs by automation from the sheet), or a comma list control=CC number "
                         "with controls cutoff, resonance, envmod, decay, accent_knob, waveform")
    args = ap.parse_args()
    out = Path(args.out)
    out.mkdir(parents=True, exist_ok=True)
    src = args.source_id

    cols = ["file", "note", "waveform", "accent", "cutoff", "resonance", "envmod", "decay",
            "accent_knob", "note_on_ms", "gate_ms", "set", "stage", "purpose"]
    stages = {}
    for stage, name, purpose, rows in note_sets():
        stages.setdefault(stage, []).append((name, purpose, rows))
    with open(out / f"{src}_sheet.csv", "w", newline="") as f:
        f.write("# Reduced training set for a software 303 (docs/EMULATION_TRAINING_PLAN.md).\n"
                "# One MIDI file and one WAV per stage; knobs are percent of travel. For reading:\n"
                "# tools/fit_stage.py takes the knobs from the MIDI file itself.\n")
        w = csv.DictWriter(f, fieldnames=cols, extrasaction="ignore")
        w.writeheader()
        for stage, sets in sorted(stages.items()):
            t, events, controls, texts = NOTE_ON_MS, [], [], []
            for name, purpose, rows in sets:
                rows = [snap(dict(r), args.cc) for r in rows]
                texts.append((max(0, t - CC_LEAD_MS - 100), 0x06, f"{name}: {purpose}"))
                for i, r in enumerate(rows):
                    r["note_on_ms"] = t
                    t += max(SLOT_MS, r["gate_ms"] + TAIL_MS)
                    w.writerow(dict(r, file=f"{src}-stage{stage}.wav", set=name, stage=stage, purpose=purpose))
                    events.append((midi_of(r["note"]), r["note_on_ms"], r["note_on_ms"] + r["gate_ms"],
                                   VEL_ACCENT if r["accent"] == "yes" else VEL_NORMAL))
                    controls += cc_events(r, r["note_on_ms"], args.cc)
                    texts.append((r["note_on_ms"], 0x01, note_text(f"{name}-{i + 1}", r)))
            write_midi(out / f"{src}-stage{stage}.mid", events, t, controls, texts)
            print(f"stage {stage}: {len(events)} notes, {t / 60000:.1f} min -> {out / f'{src}-stage{stage}.mid'}")

    scols = ["file", "seq", "event", "note", "on_ms", "off_ms", "accent", "slide_from_previous",
             "waveform", "cutoff", "resonance", "envmod", "decay", "accent_knob", "set", "purpose"]
    with open(out / f"{src}_sequences.csv", "w", newline="") as f:
        f.write("# Stage 4: multi-note clips (slides, accent runs). Times are absolute in the file.\n"
                "# A slide is a note-on before the previous note-off; knobs hold for the whole sequence.\n")
        w = csv.DictWriter(f, fieldnames=scols)
        w.writeheader()
        t, events, controls, texts, n_seq = NOTE_ON_MS, [], [], [], 0
        for stage, name, purpose, seqs in sequence_sets():
            texts.append((max(0, t - CC_LEAD_MS - 100), 0x06, f"{name}: {purpose}"))
            for k, (knobs, notes) in enumerate(seqs):
                knobs = snap(dict(knobs, waveform="saw"), args.cc)
                controls += cc_events(knobs, t, args.cc)
                texts.append((t, 0x01, note_text(f"{name}-{k + 1}", knobs)))
                for i, (n, on, off, acc) in enumerate(notes):
                    slid = i > 0 and notes[i - 1][2] > on
                    w.writerow(dict(file=f"{src}-stage4.wav", seq=f"{name}-{k + 1}", event=i + 1, note=n,
                                    on_ms=t + on, off_ms=t + off, accent="yes" if acc else "no",
                                    slide_from_previous="yes" if slid else "no",
                                    set=name, purpose=purpose, **knobs))
                    events.append((midi_of(n), t + on, t + off, VEL_ACCENT if acc else VEL_NORMAL))
                t += max(SLOT_MS, max(off for _, _, off, _ in notes) + TAIL_MS)
                n_seq += 1
        write_midi(out / f"{src}-stage4.mid", events, t, controls, texts)
        print(f"stage 4: {n_seq} sequences, {t / 60000:.1f} min -> {out / f'{src}-stage4.mid'}")


if __name__ == "__main__":
    main()
