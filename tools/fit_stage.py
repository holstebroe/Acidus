#!/usr/bin/env python3
"""Fit one rendered stage of the emulation training set, then listen.

The loop (docs/EMULATION_TRAINING_PLAN.md):

  1. tools/emulation_training_set.py <src> writes <src>-stage<N>.mid
  2. import the MIDI into the DAW, MIDI-learn the emulation's knobs, render
     the whole stage as one WAV
  3. python3 tools/fit_stage.py <src> <N> --wav <render.wav>
  4. listen to <dir>/stage<N>/ab.wav (source note, then Acidus, note by note)
     or put <dir>/stage<N>/acidus.wav next to the render in the DAW
  5. happy: render stage N+1 and repeat; it starts from this stage's profile

The MIDI file alone describes the stage: notes, velocity (accent), gate, and a
text event per note with the knob positions. This script aligns the MIDI to
the WAV (and checks the tempo), writes the stage manifest, fits with the
stage's parameters on this stage plus every earlier stage already in <dir>,
saves calibrations/<src>-stage<N>.json and renders the A/B files.

Stage 0 (the separability probes) is not fitted: it writes a probe report.
Its notes stay out of later fits unless --with-probes. Stage 4 (sequences)
cannot be fitted yet.

    python3 tools/fit_stage.py <src> 1 --wav ~/renders/stage1.wav
    python3 tools/fit_stage.py <src> 1 --rehearse calibrations/acidvoice.json   # dry run: Acidus as the source
"""
import argparse
import csv
import datetime
import json
import shutil
import subprocess
import sys
from pathlib import Path
from types import SimpleNamespace

import numpy as np

TOOLS = Path(__file__).resolve().parent
REPO = TOOLS.parent
sys.path.insert(0, str(TOOLS))
import calibrate_reference as cr  # noqa: E402

# What each stage fits (calibrate_reference.py --only). Stage 1 keeps the
# knob-law shapes: three points per knob can't pin them down.
STAGE_ONLY = {
    1: "cutoffBaseHz,cutoffSpanOct,filterFeedbackGain,filterResonanceLimit,envModScaleC0Slope,"
       "envModScaleC1Slope,envModOffset,vcfDecayMinSec,vcfDecayMaxSec,accentDecaySec,accentSweepDepthOct,"
       "accentVcaDepth,vegDecaySec,vcaResTapRatio,oscSquareLevel,timing",
    2: "cv,filter,vcfDecayMinSec,vcfDecayMaxSec,vcfDecayTaper,vegDecaySec,vcaResTapRatio,timing",
    3: "osc,filter,cv,env,timing",
}
KNOB_KEYS = {"cutoff": "cutoff", "resonance": "resonance", "envmod": "envMod", "decay": "decay",
             "accent_knob": "accent"}
NOTE_NAMES = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]
TAIL_MS = 400.0
ACCENT_VELOCITY = 102


def fail(msg):
    sys.exit(f"error: {msg}")


# ---------------------------------------------------------------------------
# MIDI
# ---------------------------------------------------------------------------

def read_midi(path):
    """Notes of a type 0/1 SMF with one tempo: dicts with midi, on_ms, off_ms,
    velocity and the text event at the note-on (the knobs)."""
    d = Path(path).read_bytes()
    if d[:4] != b"MThd":
        fail(f"{path}: not a MIDI file")
    ppq = int.from_bytes(d[12:14], "big")
    pos, tracks = 14, []
    while pos + 8 <= len(d):
        size = int.from_bytes(d[pos + 4:pos + 8], "big")
        if d[pos:pos + 4] == b"MTrk":
            tracks.append(d[pos + 8:pos + 8 + size])
        pos += 8 + size
    tempo, events = 500000, []
    for trk in tracks:
        i, tick, status = 0, 0, 0

        def vlq():
            nonlocal i
            n = 0
            while True:
                b = trk[i]
                i += 1
                n = (n << 7) | (b & 0x7F)
                if b < 0x80:
                    return n
        while i < len(trk):
            tick += vlq()
            b = trk[i]
            if b == 0xFF:
                kind, i = trk[i + 1], i + 2
                n = vlq()
                body = trk[i:i + n]
                i += n
                if kind == 0x51:
                    tempo = int.from_bytes(body, "big")
                elif kind in (0x01, 0x06):
                    events.append((tick, "text" if kind == 0x01 else "marker", body.decode("latin-1")))
                continue
            if b in (0xF0, 0xF7):
                i += 1
                i += vlq()
                continue
            if b & 0x80:
                status, i = b, i + 1
            hi = status & 0xF0
            n = 1 if hi in (0xC0, 0xD0) else 2
            data = trk[i:i + n]
            i += n
            if hi == 0x90 and data[1] > 0:
                events.append((tick, "on", (data[0], data[1])))
            elif hi == 0x80 or (hi == 0x90 and data[1] == 0):
                events.append((tick, "off", data[0]))
    ms = tempo / 1000.0 / ppq
    events.sort(key=lambda e: e[0])
    notes, held, texts, markers = [], {}, {}, []
    for tick, kind, val in events:
        if kind == "text":
            texts[tick] = val
        elif kind == "marker":
            markers.append((tick * ms, val))
        elif kind == "on":
            held[val[0]] = len(notes)
            notes.append({"midi": val[0], "velocity": val[1], "on_ms": tick * ms, "off_ms": None, "tick": tick})
        elif kind == "off" and val in held:
            notes[held.pop(val)]["off_ms"] = tick * ms
    for n in notes:
        if n["off_ms"] is None:
            fail(f"{path}: note {n['midi']} at {n['on_ms']:.0f} ms has no note-off")
        n["text"] = texts.get(n["tick"])
    return notes, markers


def parse_text(text):
    """'1A-3 saw cutoff=50 resonance=0 ...' -> (id, waveform, knob percents)."""
    if not text:
        return None
    parts = text.split()
    knobs = dict(p.split("=") for p in parts[2:])
    return parts[0], parts[1], {k: float(knobs[k]) for k in KNOB_KEYS}


def note_name(m):
    return f"{NOTE_NAMES[m % 12]}{m // 12 - 1}"


def stage_notes(mid):
    notes, _ = read_midi(mid)
    out = []
    for n in notes:
        meta = parse_text(n["text"])
        if meta is None:
            fail(f"{mid}: the note at {n['on_ms']:.0f} ms has no knob text event; this needs a stage file "
                 "written by tools/emulation_training_set.py")
        if any(o["on_ms"] + o["gate_ms"] > n["on_ms"] for o in out):
            fail(f"{mid}: overlapping notes (slides); only single-note stages (0-3) can be fitted")
        cid, wave, knobs = meta
        out.append({"id": cid, "set": cid.rsplit("-", 1)[0], "midi": n["midi"], "note": note_name(n["midi"]),
                    "waveform": wave, "accent": n["velocity"] >= ACCENT_VELOCITY, "knobs": knobs,
                    "on_ms": n["on_ms"], "gate_ms": n["off_ms"] - n["on_ms"]})
    return out


# ---------------------------------------------------------------------------
# Acidus renders
# ---------------------------------------------------------------------------

class Acidus:
    def __init__(self, profile, tune_cents):
        self.r = cr.Renderer(cr.find_library(REPO / "build", False))
        self.base = self.r.defaults.copy()
        for k, v in json.loads(Path(profile).read_text(encoding="utf-8"))["parameters"].items():
            if k in self.r.index:
                self.base[self.r.index[k]] = v
        self.tune = tune_cents

    def note(self, n, sr, length_ms=None):
        v = self.base.copy()
        for col, key in KNOB_KEYS.items():
            v[self.r.index[key]] = n["knobs"][col] / 100.0
        v[self.r.index["tuningCents"]] = self.tune
        total = int(round((length_ms or n["gate_ms"] + TAIL_MS) * sr / 1000.0))
        y, ok = self.r.render(v, 0 if n["waveform"] == "saw" else 1, n["midi"], n["accent"], sr,
                              max(1, int(round(n["gate_ms"] * sr / 1000.0))), total)
        if not ok:
            fail(f"Acidus render failed for {n['id']}")
        return y

    def timeline(self, notes, sr, frames, offset_ms=0.0, gain=1.0):
        y = np.zeros(frames)
        for n in notes:
            a = int(round((n["on_ms"] + offset_ms) * sr / 1000.0))
            seg = self.note(n, sr)[: max(0, frames - a)]
            y[a:a + len(seg)] += gain * seg
        return y


def onset_ms(x, sr, start_ms, stop_ms):
    """First sample above 5 % of the window's peak, ms from the window start."""
    a, b = max(0, int(start_ms * sr / 1000.0)), int(stop_ms * sr / 1000.0)
    w = np.abs(x[a:b])
    if len(w) == 0 or w.max() <= 0:
        return None
    return float(np.argmax(w > 0.05 * w.max())) * 1000.0 / sr + (a * 1000.0 / sr - start_ms)


def align(x, sr, notes, acidus):
    """Offset (ms) of the WAV against the MIDI, from each note's onset in the
    WAV minus its onset in an Acidus render of the same note, so a source
    whose unaccented notes open late isn't shifted for it."""
    offs = []
    for n in notes:
        ref = acidus.note(dict(n, gate_ms=min(n["gate_ms"], 250.0)), sr, length_ms=300.0)
        o_ref = onset_ms(ref, sr, 0.0, 300.0)
        o_src = onset_ms(x, sr, n["on_ms"] - 150.0, n["on_ms"] + 300.0)
        if o_ref is not None and o_src is not None:
            offs.append(o_src - 150.0 - o_ref)
    if len(offs) < 3:
        fail("could not find the notes in the WAV: is it the render of this stage's MIDI file?")
    offs = np.array(offs)
    k = max(1, len(offs) // 5)
    drift = float(np.median(offs[-k:]) - np.median(offs[:k]))
    if abs(drift) > 5.0:
        fail(f"the notes drift {drift:+.0f} ms from the start to the end of the WAV: render at 125 BPM "
             "(the MIDI file's tempo) at the WAV's own sample rate, from the start of the project")
    return float(np.median(offs))


# ---------------------------------------------------------------------------
# Stage 0: separability probes
# ---------------------------------------------------------------------------

def peak_track(seg, sr, f0, gate_ms):
    """Resonant-peak frequency (log2 Hz) every 10 ms, NaN where no clear peak.
    30 ms frames, the saw's -6 dB/oct tilt removed and the spectrum smoothed
    over one harmonic spacing (as calibrate_reference.sweep_track); a frame
    counts if the peak stands 6 dB above the level an octave below it, and
    only while the gate is open."""
    frame, hop, nfft = int(0.030 * sr), int(0.010 * sr), 1 << 14
    fr = np.fft.rfftfreq(nfft, 1.0 / sr)
    tilt = 20 * np.log10(np.maximum(fr, 1.0) / f0)
    k = max(3, int(round(f0 / (sr / nfft))))
    kern = np.ones(k) / k
    lo, hi = int(np.searchsorted(fr, max(100.0, 2.5 * f0))), int(np.searchsorted(fr, min(16000.0, 0.45 * sr)))
    win = np.hanning(frame)
    n = max(0, int((gate_ms - 20.0) / 10.0))
    out = np.full(n, np.nan)
    for j in range(n):
        x = seg[j * hop:j * hop + frame]
        if len(x) < frame:
            break
        X = 20 * np.log10(np.abs(np.fft.rfft(x * win, nfft)) + 1e-9) + tilt
        Xs = np.convolve(X[:hi + k], kern, "same")
        i = lo + int(np.argmax(Xs[lo:hi]))
        if 0 < i < len(Xs) - 1:
            a, b, c = Xs[i - 1], Xs[i], Xs[i + 1]
            d = 0.5 * (a - c) / (a - 2 * b + c) if a - 2 * b + c < 0 else 0.0
            f = fr[i] + d * (fr[1] - fr[0])
        else:
            f = fr[i]
        below = Xs[int(np.searchsorted(fr, f / 2))]
        if f > 0 and Xs[i] - below >= 6.0:
            out[j] = np.log2(f)
    return out


def probe_features(x, sr, n, on_ms):
    a = int(round(on_ms * sr / 1000.0))
    seg = x[a:a + int(round((n["gate_ms"] + TAIL_MS) * sr / 1000.0))]
    f0 = cr.midi_to_hz(n["midi"])
    spec = cr.FeatureSpec(SimpleNamespace(sr=sr, n=len(seg)), f0, 16000.0, 1 << 16)
    f = cr.compute_features(seg, spec)
    return {"track": peak_track(seg, sr, f0, n["gate_ms"]), "stft": f["stft"], "env": f["env"],
            "gate_ms": n["gate_ms"], "stft_hop_ms": spec.st_hop * 1000.0 / sr}


def _window(f, t0, t1, key):
    v = f[key]
    hop = 10.0 if key == "track" else f["stft_hop_ms"]
    return v[int(t0 / hop):int(t1 / hop)]


def track_dev(diff, t0=40.0):
    """Typical size (semitones) of a framewise track difference: the median
    of |diff| over the frames where it exists, after the first t0 ms."""
    d = diff[int(t0 / 10.0):]
    d = d[np.isfinite(d)]
    return float(np.median(np.abs(12.0 * d))) if len(d) >= 5 else float("nan")


def _tr(g, t1=800.0):
    return _window(g, 0, t1, "track")


def _cut(*v):
    n = min(map(len, v))
    return [x[:n] for x in v]


def track_interaction(f, t1=800.0):
    """Framewise (d - c) - (b - a) for corners a, b, c, d = (A0 B0), (A0 B1), (A1 B0), (A1 B1)."""
    a, b, c, d = _cut(*(_tr(g, t1) for g in f))
    return (d - c) - (b - a)


def stft_rms(fa, fb, t1=800.0, top_db=50.0):
    a, b = _cut(_window(fa, 0, t1, "stft"), _window(fb, 0, t1, "stft"))
    m = (a > a.max() - top_db) & (b > b.max() - top_db)
    return float(np.sqrt(np.mean((a[m] - b[m]) ** 2))) if m.any() else float("nan")


def decay_fit(f):
    """tau (s) and depth (oct) of the peak track, B + A exp(-t/tau)."""
    tr = f["track"]
    t = np.arange(len(tr)) * 0.010
    m = np.isfinite(tr) & (t > 0.03)
    if m.sum() < 10:
        return float("nan"), float("nan")
    best = (np.inf, np.nan, np.nan)
    for tau in np.geomspace(0.02, 3.0, 120):
        X = np.stack([np.exp(-t[m] / tau), np.ones(m.sum())], axis=1)
        coef, *_ = np.linalg.lstsq(X, tr[m], rcond=None)
        r = float(np.sum((X @ coef - tr[m]) ** 2))
        if r < best[0]:
            best = (r, tau, coef[0])
    return best[1], best[2]


def probe_report(x, sim, sr, notes, offset, out_path):
    """Each probe measures an interaction in the source and in Acidus (the
    starting profile, same notes). Acidus has the 303's structure, so what it
    shares with the source (ladder nonlinearity, coupling filters, the accent
    switch) is not a problem; what is left over is an interaction the model
    can't produce, and that pair needs crossing in the training set."""
    S, A = {}, {}
    for n in notes:
        if n["set"].startswith("P"):
            S.setdefault(n["set"], []).append(probe_features(x, sr, n, n["on_ms"] + offset))
            A.setdefault(n["set"], []).append(probe_features(sim, sr, n, n["on_ms"]))
    rows = []

    def add(pid, what, src, acid, value, unit, limit, ok, action):
        fmt = (lambda v: f"{v:.2f}" if np.isfinite(v) else "n/a")
        verdict = "inconclusive" if not np.isfinite(value) else ("ok" if ok else "INTERACTS")
        rows.append((pid, what, fmt(src), fmt(acid), f"{fmt(value)} {unit}", limit, verdict,
                     "" if verdict == "ok" else action))

    def pair(pid, what, action, limit=1.0):
        if pid in S:
            i_s, i_a = track_interaction(S[pid]), track_interaction(A[pid])
            i_s, i_a = _cut(i_s, i_a)
            v = track_dev(i_s - i_a)
            add(pid, what, track_dev(i_s), track_dev(i_a), v, "st", f"< {limit:g} st", v < limit, action)

    if "P01" in S:   # corners: (C1, cut 50), (C1, cut 100), (C2, cut 50), (C2, cut 100)
        def shift(f):
            sh = [np.nanmedian(_window(g, 400, 1200, "track")) if np.isfinite(_window(g, 400, 1200, "track")).any()
                  else np.nan for g in f]
            return 12.0 * np.nanmean([sh[2] - sh[0], sh[3] - sh[1]])
        s_, a_ = shift(S["P01"]), shift(A["P01"])
        add("P01", "settled peak shift C1 -> C2 (key tracking)", s_, a_, abs(s_ - a_), "st", "< 1 st",
            abs(s_ - a_) < 1.0, "key tracking: Acidus has no constant for it; keep later stages at C2 and note the gap")
    pair("P02", "pitch x Env Mod, peak track", "repeat 2B at C1")
    pair("P03", "pitch x accent, peak track", "repeat 3A at C1")
    if "P03" in S:   # (C1 no), (C1 acc), (C2 no), (C2 acc)
        def lev(f):
            lv = [float(np.max(g["env"])) for g in f]
            return (lv[3] - lv[2]) - (lv[1] - lv[0])
        s_, a_ = lev(S["P03"]), lev(A["P03"])
        add("P03", "pitch x accent, level", s_, a_, abs(s_ - a_), "dB", "< 1 dB", abs(s_ - a_) < 1.0,
            "repeat 3A at C1")
    if "P04" in S:   # (no, d0), (no, d100), (acc, d0), (acc, d100): accented pair must match
        def acc(f):
            return track_dev(np.subtract(*_cut(_tr(f[3]), _tr(f[2]))))
        s_, a_ = acc(S["P04"]), acc(A["P04"])
        add("P04", "Decay knob on accented notes", s_, a_, s_, "st", "< 1 st", s_ < 1.0,
            "Decay acts on accents: add accented notes to the 2C Decay sweep")
    for pid, what, action in (("P05", "Decay tau vs Env Mod", "cross 2B and 2C (Env Mod x Decay)"),
                              ("P06", "Decay tau vs Cutoff", "repeat 2C at Cutoff 100")):
        if pid in S:   # (A0, d25), (A0, d75), (A1, d25), (A1, d75): tau should follow Decay only
            def ratio(f):
                t = [decay_fit(g)[0] for g in f]
                if not all(0.03 < v < 0.8 * g["gate_ms"] / 1000.0 and np.isfinite(g["track"]).mean() >= 0.5
                           for v, g in zip(t, f)):
                    return float("nan")   # tau outside what the gate shows, or the peak mostly lost
                return 100.0 * max(abs(t[2] / t[0] - 1), abs(t[3] / t[1] - 1))
            s_, a_ = ratio(S[pid]), ratio(A[pid])
            add(pid, what + ", tau change %", s_, a_, abs(s_ - a_), "%", "< 15 %", abs(s_ - a_) < 15.0, action)
    pair("P07", "Resonance x Env Mod, peak track", "repeat 2B at Resonance 100")
    if "P08" in S:   # (150 ms, no), (150, acc), (1300, no), (1300, acc): the first 140 ms must match
        v = max(stft_rms(S["P08"][0], S["P08"][2], 140.0), stft_rms(S["P08"][1], S["P08"][3], 140.0))
        add("P08", "gate length changes the note before gate-off", v, 0.0, v, "dB", "< 1 dB", v < 1.0,
            "the source looks ahead to the gate (or isn't one-shot): keep gates the same everywhere")
    # The square's missing even harmonics make its peak track ~1 st noisier.
    pair("P09", "waveform x Env Mod, peak track", "add square notes to 2B", limit=2.0)
    pair("P10", "accent sweep vs Cutoff, peak track", "repeat 3A at Cutoff 75")
    for pid, what, lim, action in (
            ("P11", "Accent knob on an unaccented note", 0.5,
             "the knob acts without accent: add unaccented notes to the 3A Accent knob sweep"),
            ("P12", "the same note twice", 0.1,
             "the source is not repeatable: turn off drift / analog variation / random phase")):
        if pid in S:
            v = stft_rms(S[pid][0], S[pid][1], 1300.0)
            add(pid, what, v, 0.0, v, "dB", f"< {lim:g} dB", v < lim, action)

    lines = ["# Stage 0: separability probes", "",
             f"WAV offset against the MIDI: {offset:+.1f} ms. `source` and `Acidus` are the interaction",
             "measured in each; `left over` is what the source has that Acidus's structure doesn't",
             "(peak tracks: median over frames, semitones). P04, P08, P11, P12 must be zero in the source.", "",
             "| probe | what | source | Acidus | left over | limit | verdict | if it interacts |",
             "|---|---|---|---|---|---|---|---|"]
    lines += [f"| {' | '.join(r)} |" for r in rows]
    lines += ["", "\"inconclusive\": no clear resonant peak in one of the corners, or (P05, P06) a Decay",
              "time constant longer than the gate or a peak lost for most of the note. Nothing",
              "contradicts the 303 structure there; keep the plan as it is.",
              "What to add when a pair interacts: docs/EMULATION_TRAINING_PLAN.md, section 3."]
    out_path.write_text("\n".join(lines) + "\n")
    print("\n".join(lines))


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def write_manifest(work, src, stage, notes, wav_name, offset, tune):
    sheet = work / f"{src}-stage{stage}.csv"
    with open(sheet, "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["file", "note", "waveform", "accent", "cutoff", "resonance", "envmod", "decay",
                    "accent_knob", "note_on_ms", "gate_ms", "tune_cents", "set", "id"])
        for n in notes:
            k = n["knobs"]
            w.writerow([wav_name, n["note"], n["waveform"], "yes" if n["accent"] else "no", k["cutoff"],
                        k["resonance"], k["envmod"], k["decay"], k["accent_knob"],
                        round(n["on_ms"] + offset, 3), round(n["gate_ms"], 3), tune, n["set"], n["id"]])
    subprocess.run([sys.executable, str(TOOLS / "make_reference_manifest.py"), str(sheet),
                    "--description", f"{src} stage {stage}, rendered from {src}-stage{stage}.mid"], check=True)
    return work / f"{src}-stage{stage}_manifest.json"


def merged_manifest(base, src, stage, alone, with_probes):
    files, clips = {}, []
    for k in ([stage] if alone else range(0 if with_probes else 1, stage + 1)):
        m = base / f"stage{k}" / f"{src}-stage{k}_manifest.json"
        if not m.exists():
            continue
        man = json.loads(m.read_text())
        for f, info in man["files"].items():
            files[f"stage{k}/{f}"] = info
        for c in man["clips"]:
            clips.append(dict(c, file=f"stage{k}/{c['file']}"))
    out = base / f"fit-stage{stage}_manifest.json"
    out.write_text(json.dumps({"description": f"{src} stage {stage}" + ("" if alone else " and earlier stages"),
                               "generator": "tools/fit_stage.py", "files": files, "clips": clips}, indent=1) + "\n")
    return out, len(clips)


def write_ab(path, x, sr, notes, offset, acidus, gain):
    """Source note, 300 ms gap, Acidus note, 700 ms gap; one shared scale."""
    pre = int(0.05 * sr)
    gap, pause = np.zeros(int(0.3 * sr)), np.zeros(int(0.7 * sr))
    parts = []
    for n in notes:
        a = int(round((n["on_ms"] + offset) * sr / 1000.0)) - pre
        src = x[max(0, a):a + pre + int(round((n["gate_ms"] + TAIL_MS) * sr / 1000.0))]
        sim = np.concatenate([np.zeros(pre), gain * acidus.note(n, sr)])[:len(src)]
        parts += [src, gap, sim, pause]
    y = np.concatenate(parts)
    cr.write_wav(path, y * (0.9 / max(1e-9, np.max(np.abs(y)))), sr)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("source_id")
    ap.add_argument("stage", type=int, choices=range(5))
    ap.add_argument("--wav", help="the DAW render of the whole stage")
    ap.add_argument("--rehearse", metavar="PROFILE",
                    help="dry run: render the stage through Acidus with this profile instead of --wav")
    ap.add_argument("--dir", help="working folder (default test/resources/<src>)")
    ap.add_argument("--midi", help="stage MIDI file (default <dir>/<src>-stage<N>.mid)")
    ap.add_argument("--start", help="profile to start from (default: the previous stage's fit, "
                    "else calibrations/x0x.json)")
    ap.add_argument("--tune-cents", type=float, default=0.0, help="the source's tuning offset (default 0)")
    ap.add_argument("--alone", action="store_true", help="fit this stage's notes only, not earlier stages too")
    ap.add_argument("--with-probes", action="store_true",
                    help="also fit the stage 0 probe notes (48 more notes: slower, little new information)")
    ap.add_argument("--minutes", type=float, default=30.0, help="fit time cap (default 30)")
    ap.add_argument("--patience", type=float, default=None,
                    help="stop after this many minutes without progress (default: a third of --minutes)")
    ap.add_argument("--evaluate-only", action="store_true", help="score the start profile, no fit")
    args, extra = ap.parse_known_args()

    src, stage = args.source_id, args.stage
    base = Path(args.dir) if args.dir else REPO / "test" / "resources" / src
    mid = Path(args.midi) if args.midi else base / f"{src}-stage{stage}.mid"
    if not mid.exists():
        fail(f"{mid} not found: run tools/emulation_training_set.py {src} --out {base}")
    if stage == 4:
        fail("stage 4 (slides, accent runs) can't be fitted yet: the calibrator renders one note per clip "
             "and the slide time is fixed. Use it for listening only.")
    if bool(args.wav) == bool(args.rehearse):
        fail("give exactly one of --wav and --rehearse")
    prev = [REPO / "calibrations" / f"{src}-stage{k}.json" for k in range(stage - 1, 0, -1)]
    start = Path(args.start) if args.start else next((p for p in prev if p.exists()),
                                                     REPO / "calibrations" / "x0x.json")
    notes = stage_notes(mid)
    work = base / f"stage{stage}"
    work.mkdir(parents=True, exist_ok=True)
    wav_name = f"{src}-stage{stage}.wav"
    print(f"stage {stage}: {len(notes)} notes from {mid.name}; starting profile {start}")

    if args.rehearse:
        sr = 48000
        rehearse = Acidus(args.rehearse, args.tune_cents)
        frames = int((notes[-1]["on_ms"] + notes[-1]["gate_ms"] + 1700.0) * sr / 1000.0)
        cr.write_wav(work / wav_name, 0.1 * rehearse.timeline(notes, sr, frames), sr)
        print(f"rehearsal: rendered the stage through Acidus with {args.rehearse}")
    elif Path(args.wav).resolve() != (work / wav_name).resolve():
        shutil.copyfile(args.wav, work / wav_name)
    x, sr = cr.read_wav(work / wav_name)
    peak = float(np.max(np.abs(x)))
    if peak >= 0.999:
        fail(f"{wav_name} clips (peak {peak:.3f}): lower the output level and render again")
    if peak < 1e-3:
        fail(f"{wav_name} is silent")

    acidus = Acidus(start, args.tune_cents)
    offset = align(x, sr, notes, acidus)
    print(f"WAV offset against the MIDI: {offset:+.1f} ms, peak {20 * np.log10(peak):.1f} dBFS")
    write_manifest(work, src, stage, notes, wav_name, offset, args.tune_cents)

    if stage == 0:
        sim = acidus.timeline([n for n in notes if n["set"].startswith("P")], sr, len(x))
        probe_report(x, sim, sr, notes, offset, work / "probes.md")
        print(f"\nwrote {work / 'probes.md'}. Next: render stage 1 and run "
              f"tools/fit_stage.py {src} 1 --wav <render>")
        return

    manifest, n_clips = merged_manifest(base, src, stage, args.alone, args.with_probes)
    fit_dir = work / "fit"
    cmd = [sys.executable, str(TOOLS / "calibrate_reference.py"), "--manifest", str(manifest),
           "--calibration", str(start), "--fix", "knobs", "--w-sweep", "1", "--only", STAGE_ONLY[stage],
           "--max-minutes", str(args.minutes), "--out", str(fit_dir), "--no-sensitivity",
           "--patience-minutes", str(args.patience if args.patience is not None else args.minutes / 3.0)]
    if args.evaluate_only:
        cmd.append("--evaluate-only")
    first = stage if args.alone else (0 if args.with_probes else 1)
    print(f"fitting {n_clips} notes (stages {first}-{stage}) for up to {args.minutes:g} min ...", flush=True)
    subprocess.run(cmd + extra, check=True)

    result = json.loads((fit_dir / "result.json").read_text())
    ckpt = fit_dir / "checkpoint.json"
    prof = json.loads((ckpt if ckpt.exists() and not args.evaluate_only else start).read_text())
    prof.pop("checkpoint", None)
    prof.update({"source": f"{src}, stage {stage} of docs/EMULATION_TRAINING_PLAN.md (tools/fit_stage.py)",
                 "fitted": datetime.date.today().isoformat(), "reference_set": str(manifest.relative_to(REPO))
                 if manifest.is_relative_to(REPO) else str(manifest), "started_from": str(start)})
    out_prof = REPO / "calibrations" / f"{src}-stage{stage}.json"
    if not args.evaluate_only:
        out_prof.write_text(json.dumps(prof, indent=1) + "\n")

    after = Acidus(out_prof if not args.evaluate_only else start, args.tune_cents)
    gain = 10 ** (result["after"]["aggregate"]["gain_db"] / 20.0)
    write_ab(work / "ab.wav", x, sr, notes, offset, after, gain)
    cr.write_wav(work / "acidus.wav", after.timeline(notes, sr, len(x), offset, gain), sr)

    per_set = {}
    for ref, b, a in zip(result["references"], result["before"]["samples"], result["after"]["samples"]):
        per_set.setdefault(ref["set"], []).append((b["cost"], a["cost"]))
    print("\nweighted error per set, before -> after:")
    for s, v in per_set.items():
        print(f"  {s:5s} {np.mean([b for b, _ in v]):5.2f} -> {np.mean([a for _, a in v]):5.2f}")
    b, a = result["before"]["aggregate"], result["after"]["aggregate"]
    print(f"  all   {b['cost']:5.2f} -> {a['cost']:5.2f}   (harmonics within 3 dB "
          f"{b['harm_within_3db']:.0f} % -> {a['harm_within_3db']:.0f} %)")
    print(f"\nprofile  {out_prof if not args.evaluate_only else '(evaluate-only: none written)'}")
    print(f"report   {fit_dir / 'report.md'}")
    print(f"listen   {work / 'ab.wav'}  (source, then Acidus, note by note)")
    print(f"         {work / 'acidus.wav'}  (Acidus on the render's timeline, for the DAW)")
    if stage < 3:
        print(f"next     render {src}-stage{stage + 1}.mid, then tools/fit_stage.py {src} {stage + 1} --wav <render>")


if __name__ == "__main__":
    main()
