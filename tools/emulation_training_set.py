#!/usr/bin/env python3
"""Write the reduced training set for calibrating against a software 303.

The set is explained in docs/EMULATION_TRAINING_PLAN.md. This script writes,
for a source id <src>:

    <out>/<src>_notes.csv       knob sheet for tools/make_reference_manifest.py:
                                single notes of stage 0 (probes P01-P12) and
                                stages 1-3 (sets 1A-1E, 2A-2C, 3A-3D, anchors nR)
    <out>/<src>_sequences.csv   stage 4: slides, accent runs, retriggers (Q1-Q6);
                                the calibrator does not fit multi-note clips yet
    <out>/midi/<src>-<set>.mid  one MIDI file per WAV to bounce, if --midi

Stages are cumulative: render a stage, fit and validate it (see the plan),
then rerun with a higher --through so the sheet also lists the next stage.

The MIDI files run at 125 BPM, 480 PPQ, so one tick is exactly 1 ms and every
note_on_ms in the sheets is a whole tick. Accent is velocity 127, normal 100
(Burette's convention); a slide is the next note-on 10 ms before the previous
note-off. By default knob positions are NOT in the MIDI: set them per note
from the sheet (plugin parameter automation at the exact value, changed in
the silence between notes). With --cc the MIDI also sets them by Control
Change, 300 ms before each note (or sequence). `--cc default` uses Acidus's
CC map (Cutoff 71, Resonance 72, Env Mod 73, Decay 74, Accent 22, Waveform
23): MIDI-learn the emulation's knobs to those, and the same files drive both.

CC is 7-bit, so 25 % cannot be sent exactly (31.75 / 127). In --cc mode every
knob is snapped to the nearest CC value and the sheet records the exact
position that CC gives (25 % -> 32 -> 25.197 %), so the fit sees the knob the
emulation really got. Check that the emulation maps CC 0..127 linearly onto
the full knob travel before trusting this.

    python3 tools/emulation_training_set.py <src> --out test/resources/<src> --midi --cc default --through 1
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
    s.append((0, "P01", "pitch x cutoff: key tracking of the settled peak",
              grid("P01", dict(resonance=100), note=["C1", "C3"], cutoff=[50, 100])))
    s.append((0, "P02", "pitch x envmod: sweep depth in octaves",
              grid("P02", dict(cutoff=25, resonance=75, decay=50), note=["C1", "C3"], envmod=[25, 100])))
    s.append((0, "P03", "pitch x accent: accent sweep and level",
              grid("P03", dict(common, resonance=100, accent_knob=100), note=["C1", "C3"], accent=["no", "yes"])))
    s.append((0, "P04", "decay x accent: Decay is shorted on accented steps",
              grid("P04", dict(cutoff=25, resonance=75, envmod=75, accent_knob=100),
                   accent=["no", "yes"], decay=[0, 100])))
    s.append((0, "P05", "envmod x decay: depth from Env Mod only, tau from Decay only",
              grid("P05", dict(cutoff=25, resonance=75), envmod=[50, 100], decay=[25, 75])))
    s.append((0, "P06", "cutoff x decay: tau does not depend on Cutoff",
              grid("P06", dict(resonance=75, envmod=100), cutoff=[25, 75], decay=[25, 75])))
    s.append((0, "P07", "resonance x envmod: depth does not depend on Resonance",
              grid("P07", dict(cutoff=25, decay=50), resonance=[50, 100], envmod=[25, 100])))
    s.append((0, "P08", "gate x accent: nothing looks ahead to the gate; release shape",
              grid("P08", dict(cutoff=25, resonance=75, envmod=100, decay=75, accent_knob=100),
                   gate_ms=[150, 1300], accent=["no", "yes"])))
    s.append((0, "P09", "waveform x envmod: same sweep for saw and square",
              grid("P09", dict(cutoff=25, resonance=75, decay=50), waveform=["saw", "square"], envmod=[25, 100])))
    s.append((0, "P10", "accent x cutoff x envmod: accent sweep adds in octaves",
              grid("P10", dict(resonance=100, decay=50, accent_knob=100),
                   cutoff=[25, 75], envmod=[0, 100], accent=["no", "yes"])))
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


def layout(rows):
    """note_on_ms per row: fixed grid, slot grows with the gate."""
    t = NOTE_ON_MS
    for r in rows:
        r["note_on_ms"] = t
        t += max(SLOT_MS, r["gate_ms"] + TAIL_MS)
    return t


def write_midi(path, events, end_ms, controls=()):
    """Type-0 SMF, 125 BPM, 480 PPQ (1 tick = 1 ms). events: (midi, on, off, vel);
    controls: (tick, data) Control Changes."""
    msgs = [(t, -1, data) for t, data in controls]
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


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("source_id")
    ap.add_argument("--out", default=".", help="output folder")
    ap.add_argument("--midi", action="store_true", help="also write one MIDI file per WAV")
    ap.add_argument("--cc", type=parse_cc, default={},
                    help="set knobs by MIDI CC: 'default' (Acidus's CC map) or a comma list "
                         "control=CC number, controls cutoff, resonance, envmod, decay, accent_knob, waveform")
    ap.add_argument("--through", type=int, default=4,
                    help="write stages 0..N only (default 4, all), so the sheet names only rendered files")
    args = ap.parse_args()
    out = Path(args.out)
    out.mkdir(parents=True, exist_ok=True)
    src = args.source_id
    if args.midi:
        (out / "midi").mkdir(exist_ok=True)

    cols = ["file", "note", "waveform", "accent", "cutoff", "resonance", "envmod", "decay",
            "accent_knob", "note_on_ms", "gate_ms", "tune_cents", "set", "stage", "purpose"]
    n_notes = 0
    with open(out / f"{src}_notes.csv", "w", newline="") as f:
        f.write(f"# Reduced training set for a software 303, stages 0-{args.through}; docs/EMULATION_TRAINING_PLAN.md.\n"
                "# One WAV per set; knobs are percent of travel; render at 125 BPM from the MIDI files.\n"
                "# tune_cents: leave blank for an exact emulation or pass --tune-cents 0.\n")
        w = csv.DictWriter(f, fieldnames=cols, extrasaction="ignore")
        w.writeheader()
        for stage, name, purpose, rows in note_sets():
            if stage > args.through:
                continue
            rows = [snap(dict(r), args.cc) for r in rows]
            end = layout(rows)
            fname = f"{src}-{name}.wav"
            for r in rows:
                w.writerow(dict(r, file=fname, set=name, stage=stage, purpose=purpose, tune_cents=""))
            n_notes += len(rows)
            if args.midi:
                write_midi(out / "midi" / f"{src}-{name}.mid",
                           [(midi_of(r["note"]), r["note_on_ms"], r["note_on_ms"] + r["gate_ms"],
                             VEL_ACCENT if r["accent"] == "yes" else VEL_NORMAL) for r in rows], end,
                           [e for r in rows for e in cc_events(r, r["note_on_ms"], args.cc)])

    scols = ["file", "seq", "event", "note", "on_ms", "off_ms", "accent", "slide_from_previous",
             "waveform", "cutoff", "resonance", "envmod", "decay", "accent_knob", "set", "purpose"]
    n_seq = 0
    with open(out / f"{src}_sequences.csv", "w", newline="") as f:
        f.write("# Multi-note clips (slides, accent runs). Times are absolute in the file.\n"
                "# A slide is a note-on before the previous note-off; knobs hold for the whole sequence.\n")
        w = csv.DictWriter(f, fieldnames=scols)
        w.writeheader()
        for stage, name, purpose, seqs in sequence_sets():
            if stage > args.through:
                continue
            fname = f"{src}-{name}.wav"
            t, events, controls = NOTE_ON_MS, [], []
            for k, (knobs, notes) in enumerate(seqs):
                knobs = snap(dict(knobs, waveform="saw"), args.cc)
                controls += cc_events(knobs, t, args.cc)
                for i, (n, on, off, acc) in enumerate(notes):
                    slid = i > 0 and notes[i - 1][2] > on
                    w.writerow(dict(file=fname, seq=f"{name}-{k + 1}", event=i + 1, note=n,
                                    on_ms=t + on, off_ms=t + off, accent="yes" if acc else "no",
                                    slide_from_previous="yes" if slid else "no",
                                    set=name, purpose=purpose, **knobs))
                    events.append((midi_of(n), t + on, t + off, VEL_ACCENT if acc else VEL_NORMAL))
                t += max(SLOT_MS, max(off for _, _, off, _ in notes) + TAIL_MS)
                n_seq += 1
            if args.midi:
                write_midi(out / "midi" / f"{src}-{name}.mid", events, t, controls)

    print(f"{n_notes} single notes in {out / (src + '_notes.csv')}")
    print(f"{n_seq} sequences in {out / (src + '_sequences.csv')}")


if __name__ == "__main__":
    main()
