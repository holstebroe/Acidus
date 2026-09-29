#!/usr/bin/env python3
"""Build the machine-readable manifest and the markdown description of the
dinsync.info TB-303 reference recordings in test/resources/x0x-reference.

Source: http://www.dinsync.info/2010/02/tb-303-reference-recordings-for-x0xb0x.html
(the author's chart is copied in "x0x-reference knob positions.txt").

Every SET-<row><col>.wav file holds 16 notes: 4 knob *positions* x 4 notes.
Within a position the notes are always

    1: saw, unaccented    2: saw, accented
    3: square, unaccented 4: square, accented

all on the lowest C of the keypad with no transpose (C2, ~65.4 Hz). The
chart gives the five knob percentages (Cutoff, Resonance, Env Mod, Decay,
Accent) for each position. Levels are deliberately *not* normalised, within
or across sets, so absolute level differences are part of the reference.

This script parses the chart (so the chart stays the single source of truth
for the knob settings), measures every note (note-on, gate-off, pitch,
level), and writes

    test/resources/x0x-reference/x0x_reference_manifest.json
    test/resources/x0x-reference/x0x_reference.md

The manifest is what tools/calibrate_reference.py --manifest reads.

Usage:
    python3 tools/x0x_reference_manifest.py            # rewrite both files
    python3 tools/x0x_reference_manifest.py --check    # fail if they are stale
"""

import argparse
import json
import math
import re
import sys
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))
from calibrate_reference import read_wav, estimate_f0, midi_to_hz  # noqa: E402

REPO = Path(__file__).resolve().parents[1]
REF_DIR = REPO / "test" / "resources" / "x0x-reference"
CHART = REF_DIR / "x0x-reference knob positions.txt"
MANIFEST = REF_DIR / "x0x_reference_manifest.json"
MARKDOWN = REF_DIR / "x0x_reference.md"

KNOBS = ("cutoff", "resonance", "envMod", "decay", "accent")
KNOB_TITLES = {"cutoff": "Cutoff", "resonance": "Reso", "envMod": "Env Mod", "decay": "Decay", "accent": "Accent"}
NOTE_KINDS = (("saw", False), ("saw", True), ("square", False), ("square", True))
NOTE_NAME, NOTE_MIDI = "C2", 36          # lowest C of the keypad, no transpose (65.4 Hz)

# Recording layout (identical in every file, verified by measure_note()).
SLOT_SEC = 3.0                            # one note every 3 s
CLIP_PRE_SEC = 0.050                      # clip starts this long before the measured note-on
CLIP_LEN_SEC = 1.75                       # ... and lasts this long (gate ~1.33 s + release)


def parse_chart(path):
    """-> {set_name: [[pct x5] for position 1..4]}"""
    sets, cur = {}, None
    for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
        m = re.match(r"\s*SET-([A-E][1-5])\.wav", line)
        if m:
            cur = m.group(1)
            sets[cur] = []
            continue
        m = re.match(r"\s*position\s+([1-4])\s+" + r"\s+".join([r"(\d+)%"] * 5), line)
        if m and cur:
            pos = int(m.group(1))
            if pos != len(sets[cur]) + 1:
                raise ValueError(f"SET-{cur}: positions out of order")
            sets[cur].append([int(g) for g in m.groups()[1:]])
    for s, rows in sets.items():
        if len(rows) != 4:
            raise ValueError(f"SET-{s}: expected 4 positions, found {len(rows)}")
    if len(sets) != 25:
        raise ValueError(f"expected 25 sets in the chart, found {len(sets)}")
    return sets


def rms_env(x, win):
    c = np.concatenate([[0.0], np.cumsum(x * x)])
    e = np.zeros(len(x))
    h = win // 2
    idx = np.arange(len(x))
    lo, hi = np.clip(idx - h, 0, len(x)), np.clip(idx + h, 0, len(x))
    e = np.sqrt((c[hi] - c[lo]) / np.maximum(1, hi - lo))
    return e


def measure_note(x, sr, slot_index):
    """Measure one note inside its 3 s slot. Sample positions are absolute
    (file) sample indices."""
    s0 = int(round(slot_index * SLOT_SEC * sr))
    s1 = min(len(x), int(round((slot_index + 1) * SLOT_SEC * sr)))
    seg = x[s0:s1]
    noise = float(np.std(seg[int(0.2 * sr): int(1.0 * sr)]))
    # First sound: first sample 30x (+30 dB) above the noise floor's RMS.
    # The floor (-87 dBFS) has occasional isolated spikes up to ~10x.
    first = int(np.argmax(np.abs(seg) > 30 * noise))
    # Envelope over one pitch period, so the saw/square edges don't ripple it.
    period = int(round(sr / midi_to_hz(NOTE_MIDI)))
    envp = rms_env(seg, period)
    envdb = 20 * np.log10(envp + 1e-12)
    # Gate-off: steepest fall of that envelope 1.2-1.45 s after the first
    # sound (every gate is ~1.3 s; a wider window can lock onto a fast
    # filter decay instead).
    h = period // 2
    d = np.full(len(envdb), np.inf)
    d[h:-h] = envdb[2 * h:] - envdb[:-2 * h]
    lo, hi = first + int(1.2 * sr), first + int(1.45 * sr)
    off = lo + int(np.argmin(d[lo:hi]))
    body = seg[first + int(0.05 * sr): off - int(0.02 * sr)]
    f0 = estimate_f0(body, sr, midi_to_hz(NOTE_MIDI))
    above = np.where(envp[off:] > 10 * noise)[0]
    tail = off + (int(above[-1]) if len(above) else 0)
    return {
        "first_sound_sample": s0 + first,
        "gate_off_sample": s0 + off,
        "tail_end_sample": s0 + tail,
        "f0_hz": float(f0),
        "tune_cents": float(1200 * math.log2(f0 / midi_to_hz(NOTE_MIDI))),
        "peak_dbfs": float(20 * np.log10(np.abs(seg).max() + 1e-12)),
        "rms_dbfs": float(20 * np.log10(np.sqrt(np.mean(body ** 2)) + 1e-12)),
        "noise_dbfs": float(20 * np.log10(noise + 1e-12)),
    }


def build_manifest():
    chart = parse_chart(CHART)
    clips, files = [], {}
    for set_name in sorted(chart):
        fname = f"SET-{set_name}.wav"
        x, sr = read_wav(REF_DIR / fname)
        files[fname] = {"sample_rate": sr, "frames": len(x), "channels_used": "mean of L/R"}
        for pos in range(1, 5):
            pct = chart[set_name][pos - 1]
            for j, (wave, accent) in enumerate(NOTE_KINDS):
                n = (pos - 1) * 4 + j + 1              # 1-based note number in the set
                m = measure_note(x, sr, n - 1)
                clips.append({
                    "id": f"{set_name}-p{pos}-{wave}{'-acc' if accent else ''}",
                    "file": fname,
                    "set": set_name,
                    "position": pos,
                    "note_number": n,
                    "note": NOTE_NAME,
                    "midi": NOTE_MIDI,
                    "waveform": wave,
                    "accent": accent,
                    "knobs_pct": dict(zip(KNOBS, pct)),
                    "knobs": {k: v / 100.0 for k, v in zip(KNOBS, pct)},
                    "slot_start_sample": int(round((n - 1) * SLOT_SEC * sr)),
                    "slot_end_sample": min(len(x), int(round(n * SLOT_SEC * sr))),
                    **{k: (round(v, 3) if isinstance(v, float) else v) for k, v in m.items()},
                })
    # Note-on. The notes sit on a fixed 3 s grid (first-sound times vary by
    # only ~1-2 ms per note kind), but an unaccented note becomes audible
    # ~4.5 ms after an accented one: at the moment the accented notes start,
    # the unaccented ones show only a small click, and their VCA rises later.
    # So the gate opens at the same grid position for every note, and that
    # position is where the accented notes start sounding.
    sr = next(iter(files.values()))["sample_rate"]
    grid = float(np.median([c["first_sound_sample"] - c["slot_start_sample"] for c in clips if c["accent"]]))
    for c in clips:
        on = c["slot_start_sample"] + int(round(grid))
        c["note_on_sample"] = on
        c["first_sound_delay_ms"] = round((c["first_sound_sample"] - on) * 1000.0 / sr, 2)
        c["gate_ms"] = round((c["gate_off_sample"] - on) * 1000.0 / sr, 1)
        c["clip_start_sample"] = on - int(round(CLIP_PRE_SEC * sr))
        c["clip_end_sample"] = min(files[c["file"]]["frames"], c["clip_start_sample"] + int(round(CLIP_LEN_SEC * sr)))
    # One unit, one Tune setting, one key: the pitch is the same for every
    # note. Per-note f0 fits are biased by a strong, sweeping resonant peak
    # (up to -25 c at Reso/Env Mod 100 %), so the tuning to render with is
    # the median of the "clean" notes (Resonance and Env Mod <= 50 %). Per
    # file those agree within ~0.3 c and the file medians within +-0.4 c --
    # well inside what the comparison can resolve -- so one global value is
    # used (and lets the calibrator render identical settings only once).
    def clean(c):
        return c["knobs_pct"]["resonance"] <= 50 and c["knobs_pct"]["envMod"] <= 50
    render_tune = round(float(np.median([c["tune_cents"] for c in clips if clean(c)])), 2)
    for fname in files:
        tunes = [c["tune_cents"] for c in clips if c["file"] == fname and clean(c)]
        files[fname]["tune_cents_clean_median"] = round(float(np.median(tunes)), 2) if tunes else None
    for c in clips:
        c["render_tune_cents"] = render_tune
    # Notes recorded more than once (same knobs, waveform and accent) --
    # e.g. "all 50 %" is position 2 of every C set. Their spread shows how
    # repeatable the (hand-set, approximate) knob positions are.
    groups = {}
    for c in clips:
        key = (tuple(c["knobs_pct"][k] for k in KNOBS), c["waveform"], c["accent"])
        groups.setdefault(key, []).append(c["id"])
    dups = []
    for key, ids in groups.items():
        if len(ids) > 1:
            lv = [next(c["rms_dbfs"] for c in clips if c["id"] == i) for i in ids]
            dups.append({"knobs_pct": dict(zip(KNOBS, key[0])), "waveform": key[1], "accent": key[2],
                         "ids": ids, "rms_dbfs_spread": round(max(lv) - min(lv), 2)})
    return {
        "description": "TB-303 reference recordings for x0xb0x builders (dinsync.info, 2010). "
                       "One hardware TB-303, 25 sets x 16 notes. Levels are NOT normalised "
                       "(within or across sets): compare absolute levels with one global gain.",
        "source_url": "http://www.dinsync.info/2010/02/tb-303-reference-recordings-for-x0xb0x.html",
        "chart": CHART.name,
        "generator": "tools/x0x_reference_manifest.py",
        "knob_order": list(KNOBS),
        "knob_positions_are_approximate": True,
        "note_layout": [{"in_position": j + 1, "waveform": w, "accent": a} for j, (w, a) in enumerate(NOTE_KINDS)],
        "timing": {"slot_sec": SLOT_SEC, "note_on_in_slot_sec": round(grid / sr, 5),
                   "clip_pre_sec": CLIP_PRE_SEC, "clip_len_sec": CLIP_LEN_SEC},
        "sample_positions": "absolute frame indices into the file; *_end_sample is exclusive",
        "files": files,
        "duplicates": dups,
        "clips": clips,
    }


def fmt_time(samples, sr):
    return f"{samples / sr:.3f}"


def build_markdown(man):
    clips = man["clips"]
    sr = next(iter(man["files"].values()))["sample_rate"]
    by_set = {}
    for c in clips:
        by_set.setdefault(c["set"], []).append(c)
    tunes = [c["tune_cents"] for c in clips]
    gates = [c["gate_ms"] for c in clips]
    grid = man["timing"]["note_on_in_slot_sec"]
    delay = lambda acc: np.median([c["first_sound_delay_ms"] for c in clips if c["accent"] == acc])
    spread = lambda acc: np.std([c["first_sound_delay_ms"] for c in clips if c["accent"] == acc])
    L = []
    L.append("# x0x-reference: dinsync.info TB-303 reference recordings")
    L.append("")
    L.append("<!-- Generated by tools/x0x_reference_manifest.py from "
             "\"x0x-reference knob positions.txt\" and the WAV files. Do not edit by hand. -->")
    L.append("")
    L.append(f"Source: <{man['source_url']}> (copied into `{man['chart']}`).")
    L.append("")
    L.append("A single hardware TB-303 recorded with a systematic sweep of every front-panel "
             "knob. Knob positions are the author's approximate settings (\"these are analog "
             "knobs\"). **Levels are not normalised** -- not within a set and, as the repeated "
             "settings below show, not across sets either -- so absolute level differences "
             "(e.g. accented vs. unaccented, low vs. high cutoff) are part of the reference.")
    L.append("")
    L.append("Machine-readable version: [`x0x_reference_manifest.json`](x0x_reference_manifest.json) "
             "(read by `tools/calibrate_reference.py --manifest`).")
    L.append("")
    L.append("## File layout")
    L.append("")
    L.append(f"- 25 files `SET-A1.wav` ... `SET-E5.wav`, {sr} Hz, 16-bit stereo "
             "(L and R are identical to within -60 dB; use their mean), 49.1 s each.")
    L.append(f"- 16 notes per file, one every {SLOT_SEC:.0f} s. Note *n* (1-based) sits in the slot "
             f"`[3(n-1), 3n)` s and its note-on is at `3(n-1) + {grid:.4f}` s. The notes are on a "
             f"fixed grid: an accented note starts sounding {delay(True):+.1f} ms from that point "
             f"(std {spread(True):.1f} ms), an unaccented one {delay(False):+.1f} ms (std "
             f"{spread(False):.1f} ms) -- at the moment the accented notes start, the unaccented "
             "ones show only a faint click and their VCA opens a few ms later. The grid note-on "
             "is therefore defined as where the accented notes start sounding.")
    L.append(f"- Every note is **C2** (the keypad's lowest C, no transpose; the 303's lowest note): measured "
             f"{min(c['f0_hz'] for c in clips):.2f}-{max(c['f0_hz'] for c in clips):.2f} Hz, i.e. "
             f"{min(tunes):+.1f} to {max(tunes):+.1f} cents from 65.406 Hz (median {np.median(tunes):+.1f}). "
             "The outliers are all strong resonance / Env Mod sweeps, whose moving resonant peak "
             "biases the harmonic fit. The oscillator itself is steady: the notes with "
             "Resonance and Env Mod <= 50 % agree within ~0.3 c per file, with file medians of "
             f"{min(v for f in man['files'].values() if (v := f['tune_cents_clean_median']) is not None):+.1f} to "
             f"{max(v for f in man['files'].values() if (v := f['tune_cents_clean_median']) is not None):+.1f} c "
             "(the D/E files have no such notes). Their overall median, "
             f"{clips[0]['render_tune_cents']:+.2f} c, is the manifest's "
             "`render_tune_cents`.")
    L.append(f"- Gate (note-on to gate-off) {min(gates):.0f}-{max(gates):.0f} ms "
             f"(median {np.median(gates):.0f} ms); the release has died into the noise "
             f"(~{np.median([c['noise_dbfs'] for c in clips]):.0f} dBFS) well before the next slot.")
    L.append("- The 16 notes are 4 knob **positions** x 4 notes:")
    L.append("")
    L.append("| Note in file | Position | Waveform | Accent |")
    L.append("|---|---|---|---|")
    for n in range(1, 17):
        w, a = NOTE_KINDS[(n - 1) % 4]
        L.append(f"| {n} | {(n - 1) // 4 + 1} | {w} | {'yes' if a else 'no'} |")
    L.append("")
    L.append("## Knob settings per set")
    L.append("")
    L.append("Each row of a set is one position; the knob that the set sweeps is in **bold**. "
             "The rows (A-E) set the level of the four fixed knobs (A: 0 %, B: 25 %, C: 50 %, "
             "D: 75 %, E: 100 %); the column (1-5) says which knob is swept "
             "(1 Cutoff, 2 Resonance, 3 Env Mod, 4 Decay, 5 Accent). In row A the swept knob "
             "starts at 25 % while the others stay at 0 %; in rows B-E it sweeps 25-100 %, so "
             "one position repeats the row's base setting.")
    L.append("")
    L.append("| Set | Swept knob | Pos | Cutoff | Reso | Env Mod | Decay | Accent |")
    L.append("|---|---|---|---|---|---|---|---|")
    for s in sorted(by_set):
        swept = KNOBS[int(s[1]) - 1]
        for pos in range(1, 5):
            c = by_set[s][(pos - 1) * 4]
            cells = []
            for k in KNOBS:
                v = f"{c['knobs_pct'][k]} %"
                cells.append(f"**{v}**" if k == swept else v)
            L.append(f"| {s if pos == 1 else ''} | {KNOB_TITLES[swept] if pos == 1 else ''} | {pos} | "
                     + " | ".join(cells) + " |")
    L.append("")
    L.append("## Per-note ranges and measurements")
    L.append("")
    L.append("Times in seconds from the start of the file. *Slot* is the note's full 3 s "
             "window; *Note-on* is the grid note-on; *Sound* is when the note becomes audible "
             "(ms after note-on); *Gate-off* is measured from the audio (the steepest fall of the "
             "one-period RMS envelope); *Clip* is the "
             f"window the calibrator compares ({CLIP_PRE_SEC * 1000:.0f} ms before note-on, "
             f"{CLIP_LEN_SEC:.2f} s long). Levels in dBFS (mean of L/R): *RMS* over the "
             "sustained part (50 ms after note-on to 20 ms before gate-off), *Peak* over the slot.")
    for s in sorted(by_set):
        L.append("")
        L.append(f"### SET-{s}.wav")
        L.append("")
        L.append("| # | Pos | Wave | Acc | Cut/Res/Env/Dec/Acc % | Slot | Note-on | Sound (ms) | Gate-off | Clip | f0 Hz | RMS | Peak |")
        L.append("|---|---|---|---|---|---|---|---|---|---|---|---|---|")
        for c in by_set[s]:
            knobs = "/".join(str(c["knobs_pct"][k]) for k in KNOBS)
            L.append(
                f"| {c['note_number']} | {c['position']} | {c['waveform']} | {'yes' if c['accent'] else 'no'} | "
                f"{knobs} | {fmt_time(c['slot_start_sample'], sr)}-{fmt_time(c['slot_end_sample'], sr)} | "
                f"{fmt_time(c['note_on_sample'], sr)} | {c['first_sound_delay_ms']:+.1f} | "
                f"{fmt_time(c['gate_off_sample'], sr)} | "
                f"{fmt_time(c['clip_start_sample'], sr)}-{fmt_time(c['clip_end_sample'], sr)} | "
                f"{c['f0_hz']:.2f} | {c['rms_dbfs']:.1f} | {c['peak_dbfs']:.1f} |")
    L.append("")
    L.append("## Repeated settings")
    L.append("")
    L.append("Several knob settings were recorded in more than one set (the chart's rows B-E "
             "repeat the base setting in every column). Their level spread shows how well the "
             "hand-set knobs and the recording level repeat -- a small spread means one "
             "global recording gain is valid for all 25 files.")
    L.append("")
    L.append("| Cut/Res/Env/Dec/Acc % | Wave | Acc | Takes | RMS spread (dB) |")
    L.append("|---|---|---|---|---|")
    for d in sorted(man["duplicates"], key=lambda d: (tuple(d["knobs_pct"].values()), d["waveform"], d["accent"])):
        knobs = "/".join(str(d["knobs_pct"][k]) for k in KNOBS)
        L.append(f"| {knobs} | {d['waveform']} | {'yes' if d['accent'] else 'no'} | "
                 f"{', '.join(i.split('-')[0] for i in d['ids'])} | {d['rms_dbfs_spread']:.2f} |")
    L.append("")
    return "\n".join(L)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--check", action="store_true", help="exit non-zero if the generated files are out of date")
    args = ap.parse_args()
    man = build_manifest()
    js = json.dumps(man, indent=1) + "\n"
    md = build_markdown(man)
    if args.check:
        stale = [p.name for p, t in ((MANIFEST, js), (MARKDOWN, md))
                 if not p.exists() or p.read_text(encoding="utf-8") != t]
        if stale:
            sys.exit(f"out of date: {', '.join(stale)} (run tools/x0x_reference_manifest.py)")
        print("up to date")
        return
    MANIFEST.write_text(js, encoding="utf-8")
    MARKDOWN.write_text(md, encoding="utf-8")
    print(f"wrote {MANIFEST.relative_to(REPO)} ({len(man['clips'])} clips) and {MARKDOWN.relative_to(REPO)}")


if __name__ == "__main__":
    main()
