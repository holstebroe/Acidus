#!/usr/bin/env python3
"""Build a calibration reference manifest for a NEW source from a CSV knob sheet.

tools/calibrate_reference.py --manifest <manifest.json> reads a manifest: one
"clip" per note (file, sample range, knobs, waveform, accent, tuning, note-on,
gate length). Writing that JSON by hand is error-prone, so this script
generates it from a plain CSV next to your WAV files. See
docs/CALIBRATION_COOKBOOK.md for the whole workflow.

CSV columns (header row required; order free; '#' starts a comment line):

    file        WAV file, relative to the CSV                        (required)
    note        C2, A#1, ...  (the note as played on the TB-303 keypad) (required)
    waveform    saw | square                                           (required)
    accent      yes | no   -- was this step an accented step?          (required)
    cutoff, resonance, envmod, decay
                knob positions in PERCENT of travel, 0-100             (required)
    accent_knob Accent knob, percent (default 0; only matters if accent = yes)
    note_on_ms  time of the note-on in the file. Blank = detect it from the
                audio (only sensible for files that hold a single note)
    gate_ms     gate length (note-on to gate-off). Blank = --gate-ms
    tune_cents  the source's pitch offset from equal temperament. Blank =
                measured from the note, see --tune-cents
    set         free text group name for per-group tables in the report
                (default: the file name without extension)
    id          clip id (default <set>-<row number in that set>)

Usage:
    python3 tools/make_reference_manifest.py my-source/notes.csv \\
        --description "Hardware TB-303 from <who>, recorded <when>" \\
        --gate-ms 1300
    # -> my-source/notes_manifest.json

    python3 tools/calibrate_reference.py --manifest my-source/notes_manifest.json --evaluate-only

Every clip starts 50 ms before the note-on and ends after the gate plus
--tail-ms of release (clipped to the file and to the next note-on in the same
file), so several notes per file are fine as long as note_on_ms is given.
"""

import argparse
import csv
import json
import math
import sys
from pathlib import Path
from types import SimpleNamespace

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))
from calibrate_reference import (KNOBS, detect_timing, estimate_f0, midi_to_hz,  # noqa: E402
                                 note_to_midi, read_wav)

PRE_MS = 50.0
KNOB_COLUMNS = {"cutoff": "cutoff", "resonance": "resonance", "envMod": "envmod", "decay": "decay"}


def fail(msg):
    sys.exit(f"error: {msg}")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("csv")
    ap.add_argument("--out", default=None, help="manifest path (default <csv>_manifest.json next to the CSV)")
    ap.add_argument("--description", default="")
    ap.add_argument("--source-url", default="")
    ap.add_argument("--gate-ms", type=float, default=None, help="gate length for rows with no gate_ms")
    ap.add_argument("--tail-ms", type=float, default=400.0, help="release kept after the gate (default 400)")
    ap.add_argument("--tune-cents", type=float, default=None,
                    help="one tuning offset for every clip (default: the median of the per-note measurements "
                    "on notes with Resonance and Env Mod <= 50 %%, which are the ones the harmonic fit "
                    "can be trusted on; rows with a tune_cents column keep theirs)")
    args = ap.parse_args()

    csv_path = Path(args.csv)
    rows = [r for r in csv.DictReader(l for l in csv_path.read_text(encoding="utf-8-sig").splitlines()
                                      if l.strip() and not l.lstrip().startswith("#"))]
    if not rows:
        fail("no rows in the CSV")
    rows = [{(k or "").strip().lower(): (v or "").strip() for k, v in r.items()} for r in rows]
    for need in ("file", "note", "waveform", "accent", "cutoff", "resonance", "envmod", "decay"):
        if need not in rows[0]:
            fail(f"missing column '{need}'")

    by_file = {}
    for n, r in enumerate(rows, start=1):
        by_file.setdefault(r.get("file", ""), []).append(n)
    for f, lines in by_file.items():
        if len(lines) > 1 and any(not rows[n - 1].get("note_on_ms") for n in lines):
            fail(f"{f} holds {len(lines)} notes (rows {', '.join(map(str, lines))}): every row needs note_on_ms")

    audio, clips, files, per_set = {}, [], {}, {}
    for n, r in enumerate(rows, start=1):   # n = data row number (after the header), for messages
        where = f"row {n} ({r['file']})"
        wav = csv_path.parent / r["file"]
        if not wav.exists():
            fail(f"{where}: file not found")
        if r["file"] not in audio:
            audio[r["file"]] = read_wav(wav)
        x, sr = audio[r["file"]]
        files[r["file"]] = {"sample_rate": sr, "frames": len(x), "channels_used": "mean of L/R"}
        try:
            midi = note_to_midi(r["note"])
        except (KeyError, ValueError):
            fail(f"{where}: bad note '{r['note']}' (use e.g. C2, A#1)")
        if r["waveform"] not in ("saw", "square"):
            fail(f"{where}: waveform must be saw or square")
        if r["accent"].lower() not in ("yes", "no"):
            fail(f"{where}: accent must be yes or no")
        pct = {}
        for k, col in KNOB_COLUMNS.items():
            pct[k] = float(r[col])
        pct["accent"] = float(r.get("accent_knob") or 0)
        if any(not 0 <= v <= 100 for v in pct.values()):
            fail(f"{where}: knob values are percent, 0-100")
        gate_ms = float(r["gate_ms"]) if r.get("gate_ms") else args.gate_ms
        if r.get("note_on_ms"):
            on = int(round(float(r["note_on_ms"]) * sr / 1000.0))
        else:
            d_on, d_gate = detect_timing(SimpleNamespace(x=x, sr=sr, f0=midi_to_hz(midi)))
            on = int(round(d_on * sr / 1000.0))
            gate_ms = gate_ms or d_gate
        if not gate_ms:
            fail(f"{where}: no gate_ms and no --gate-ms")
        grp = r.get("set") or Path(r["file"]).stem
        per_set[grp] = per_set.get(grp, 0) + 1
        clips.append({
            "id": r.get("id") or f"{grp}-{per_set[grp]}",
            "file": r["file"], "set": grp, "position": 1 + (per_set[grp] - 1) // 4, "note_number": per_set[grp],
            "note": r["note"], "midi": midi, "waveform": r["waveform"], "accent": r["accent"].lower() == "yes",
            "knobs_pct": {k: pct[k] for k in KNOBS.values()},
            "knobs": {k: pct[k] / 100.0 for k in KNOBS.values()},
            "note_on_sample": on, "gate_ms": round(gate_ms, 1),
            "_tune": float(r["tune_cents"]) if r.get("tune_cents") else None,
        })

    # Clip windows: from 50 ms before the note-on to gate + release, but never
    # into the next note of the same file.
    for c in clips:
        sr = files[c["file"]]["sample_rate"]
        a = max(0, c["note_on_sample"] - int(round(PRE_MS * sr / 1000.0)))
        b = c["note_on_sample"] + int(round((c["gate_ms"] + args.tail_ms) * sr / 1000.0))
        later = [o["note_on_sample"] for o in clips if o["file"] == c["file"] and o["note_on_sample"] > c["note_on_sample"]]
        if later:
            b = min(b, min(later) - int(round(PRE_MS * sr / 1000.0)))
        c["clip_start_sample"], c["clip_end_sample"] = a, min(b, files[c["file"]]["frames"])
        if c["clip_end_sample"] - c["clip_start_sample"] < 0.5 * c["gate_ms"] * sr / 1000.0:
            fail(f"{c['id']}: clip is shorter than half the gate: notes too close together or note_on_ms/gate_ms wrong")

    # Tuning: the notes' own pitch, as the harmonic comb measures it.
    def measured(c):
        x, sr = audio[c["file"]]
        seg = x[c["note_on_sample"] + int(0.05 * sr): c["note_on_sample"] + int(c["gate_ms"] * sr / 1000.0) - int(0.02 * sr)]
        f_guess = midi_to_hz(c["midi"])
        return 1200 * math.log2(estimate_f0(seg, sr, f_guess) / f_guess)

    clean = [c for c in clips if c["knobs_pct"]["resonance"] <= 50 and c["knobs_pct"]["envMod"] <= 50]
    if args.tune_cents is not None:
        tune = args.tune_cents
    else:
        pool = clean or clips
        if not clean:
            print("warning: no note has Resonance and Env Mod <= 50 %, so tuning is measured on all notes "
                  "and will be biased by the resonant peak; pass --tune-cents or a tune_cents column",
                  file=sys.stderr)
        if any(c["_tune"] is None and c["gate_ms"] < 500 for c in pool):
            print("warning: tuning is measured on notes with a gate under 500 ms, which is too short for a "
                  "sub-cent pitch fit (and it only searches +-80 cents); give a tune_cents column or --tune-cents "
                  "from a tuner or a longer held note", file=sys.stderr)
        tune = round(float(np.median([c["_tune"] if c["_tune"] is not None else measured(c) for c in pool])), 2)
    for c in clips:
        c["render_tune_cents"] = c["_tune"] if c["_tune"] is not None else tune
        del c["_tune"]
    print(f"{len(clips)} clips from {len(files)} files, tuning {tune:+.2f} cents, "
          f"note-on / gate taken from {'the CSV' if all(r.get('note_on_ms') for r in rows) else 'CSV + detection'}")

    manifest = {
        "description": args.description or f"Reference notes from {csv_path.name}",
        "source_url": args.source_url,
        "generator": "tools/make_reference_manifest.py",
        "knob_order": list(KNOBS.values()),
        "knob_positions_are_approximate": True,
        "sample_positions": "absolute frame indices into the file; *_end_sample is exclusive",
        "files": files,
        "clips": clips,
    }
    out = Path(args.out) if args.out else csv_path.with_name(csv_path.stem + "_manifest.json")
    out.write_text(json.dumps(manifest, indent=1) + "\n", encoding="utf-8")
    print(f"wrote {out}")


if __name__ == "__main__":
    main()
