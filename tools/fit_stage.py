#!/usr/bin/env python3
"""Fit one rendered stage of the emulation training set, then listen.

The loop (docs/EMULATION_TRAINING_PLAN.md):

  1. tools/emulation_training_set.py <src> writes <src>-stage<N>.mid
  2. import the MIDI into the DAW, MIDI-learn the emulation's knobs, render
     the whole stage as one WAV
  3. python3 tools/fit_stage.py <src> <N> --wav <render.wav>
  4. listen to <dir>/stage<N>/ab.wav (source note, then Acidus, note by note)
     or put <dir>/stage<N>/acidus.wav next to the render in the DAW
  5. optional: import calibrations/<src>-stage<N>.json into the plugin (right-
     click the logo plate), trim by ear, export it as
     calibrations/<src>-stage<N>-trimmed.json
  6. happy: render stage N+1 and repeat; it starts from the newest
     calibrations/<src>-stage<N>*.json, so the trimmed export if there is one

The MIDI file alone describes the stage: notes, velocity (accent), gate, and a
text event per note with the knob positions. This script aligns the MIDI to
the WAV (and checks the tempo), writes the stage manifest, fits with the
stage's parameters on this stage plus every earlier stage already in <dir>,
saves calibrations/<src>-stage<N>.json and renders the A/B files.

Stage 0 (the separability probes) writes a probe report and then fits the
same scale constants as stage 1 to its 44 notes (--probes-only skips the
fit). Its notes stay out of later stages' fits unless --with-probes. Stage 4 (sequences)
cannot be fitted yet.

    python3 tools/fit_stage.py <src> 1 --wav ~/renders/stage1.wav
    python3 tools/fit_stage.py <src> 1 --rehearse calibrations/acidvoice.json   # dry run: Acidus as the source
"""
import argparse
import csv
import datetime
import json
import shutil
import struct
import subprocess
import sys
import warnings
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
    # Stage 0's probes sit at mid-to-high Cutoff and Resonance, so they pin
    # the same scale constants as stage 1, less well at low Cutoff.
    0: "cutoffBaseHz,cutoffSpanOct,filterFeedbackGain,filterResonanceLimit,envModScaleC0Slope,"
       "envModScaleC1Slope,envModOffset,vcfDecayMinSec,vcfDecayMaxSec,accentDecaySec,accentSweepDepthOct,"
       "accentVcaDepth,vegDecaySec,vcaResTapRatio,oscSquareLevel,timing",
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


def sound_starts(x, sr, rel_db=-50.0, quiet_ms=200):
    """Start times (ms) of the sounds in x: 1 ms frames above rel_db of the
    peak (and the noise floor), after at least quiet_ms of frames below it. The stage files leave
    1.5 s or more of silence between notes, so each note is one sound."""
    hop = max(1, sr // 1000)
    n = len(x) // hop
    if n == 0:
        return np.zeros(0)
    frames = np.abs(x[:n * hop]).reshape(n, hop).max(axis=1)
    # Above rel_db of the peak, and well above the source's own noise: the
    # quietest fifth of the frames is silence between notes.
    loud = frames > max(frames.max() * 10 ** (rel_db / 20.0), 6.0 * np.percentile(frames, 20))
    starts, quiet = [], quiet_ms
    for i, is_loud in enumerate(loud):
        if is_loud and quiet >= quiet_ms:
            starts.append(i * hop * 1000.0 / sr)
        quiet = 0 if is_loud else quiet + 1
    return np.array(starts)


def align(x, sr, notes, acidus):
    """Offset (ms) of the WAV against the MIDI. Where the clip sits in the
    project, and where the render starts, don't matter: the notes are found
    in the audio first. Then each note's onset in the WAV is compared with its
    onset in an Acidus render of the same note, so a source whose unaccented
    notes open late isn't shifted for it."""
    midi_on = np.array([n["on_ms"] for n in notes])
    starts = sound_starts(x, sr)
    if len(starts) == 0:
        fail("the WAV is silent")
    if len(starts) == len(notes):
        # One sound per note: their spacing gives the tempo the DAW played at.
        slope, coarse = np.polyfit(midi_on, starts, 1)
        if abs(slope - 1.0) > 0.002:
            fail(f"the notes are spaced as if played at {125.0 / slope:.2f} BPM: set the project tempo to "
                 "125 BPM (the MIDI file's tempo) and render again")
    else:
        print(f"note: found {len(starts)} sounds for {len(notes)} notes (effects or noise in the render?); "
              "aligning on the first note")
        coarse = starts[0] - midi_on[0]
    offs = []
    for n in notes:
        ref = acidus.note(dict(n, gate_ms=min(n["gate_ms"], 250.0)), sr, length_ms=300.0)
        o_ref = onset_ms(ref, sr, 0.0, 300.0)
        at = n["on_ms"] + coarse
        o_src = onset_ms(x, sr, at - 150.0, at + 300.0)
        if o_ref is not None and o_src is not None:
            offs.append(coarse + o_src - 150.0 - o_ref)
    if len(offs) < 3:
        fail("could not find the notes in the WAV: is it the render of this stage's MIDI file?")
    offs = np.array(offs)
    k = max(1, len(offs) // 5)
    drift = float(np.median(offs[-k:]) - np.median(offs[:k]))
    if abs(drift) > 5.0:
        fail(f"the notes drift {drift:+.0f} ms from the start to the end of the WAV: render at 125 BPM "
             "(the MIDI file's tempo), at the WAV's own sample rate")
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


def stft_rms(fa, fb, t1=800.0, top_db=50.0, floor=None):
    """RMS dB difference of two notes' 1/3-octave spectrograms over the first
    t1 ms, on the cells within top_db of the loudest and, given the source's
    noise floor per band, at least 10 dB above it (noise cells differ at
    random)."""
    a, b = _cut(_window(fa, 0, t1, "stft"), _window(fb, 0, t1, "stft"))
    m = (a > a.max() - top_db) & (b > b.max() - top_db)
    if floor is not None:
        m &= (a > floor[None, :] + 10.0) & (b > floor[None, :] + 10.0)
    return float(np.sqrt(np.mean((a[m] - b[m]) ** 2))) if m.any() else float("nan")


def noise_floor(x, sr, notes, offset):
    """The source's noise per 1/3-octave band (dB, as in the spectrogram
    features), from the silences between notes: from 1.2 s after a gate ends
    to 250 ms before the next note."""
    rows = []
    for n, m in zip(notes[:-1], notes[1:]):
        a = max(0, int((n["on_ms"] + n["gate_ms"] + 1200.0 + offset) * sr / 1000.0))
        b = min(len(x), int((m["on_ms"] - 250.0 + offset) * sr / 1000.0))
        if b - a >= 4096:
            spec = cr.FeatureSpec(SimpleNamespace(sr=sr, n=b - a), 65.4, 16000.0, 1 << 12)
            rows.append(cr.compute_features(x[a:b], spec)["stft"])
    if not rows:
        return None
    return cr.db(np.mean(10.0 ** (np.concatenate(rows) / 10.0), axis=0))


def repeat_diagnosis(x, sr, n1, n2, offset, floor):
    """What differs between two renders of the same note: level, pitch, and
    the bands where the spectrogram differs most."""
    out = []
    segs = []
    for n in (n1, n2):
        a = int((n["on_ms"] + offset) * sr / 1000.0)
        segs.append(x[a:a + int((n["gate_ms"] + TAIL_MS) * sr / 1000.0)])
    f0 = cr.midi_to_hz(n1["midi"])
    spec = cr.FeatureSpec(SimpleNamespace(sr=sr, n=min(map(len, segs))), f0, 16000.0, 1 << 16)
    fa, fb = (cr.compute_features(s_[:spec.n], spec) for s_ in segs)
    out.append(f"peak level {np.max(fb['env']) - np.max(fa['env']):+.2f} dB")
    steady = [s_[int(0.4 * sr):int(min(1.2, n1['gate_ms'] / 1000.0 - 0.05) * sr)] for s_ in segs]
    # Where the resonant peak sits: early (the envelope sweep) and settled.
    tracks = [peak_track(s_, sr, f0, n1["gate_ms"]) for s_ in segs]
    for t0, t1, label in ((20, 200, "early"), (400, n1["gate_ms"] - 50, "settled")):
        a_, b_ = (t[int(t0 / 10):int(t1 / 10)] for t in tracks)
        k = min(len(a_), len(b_))
        d = 12.0 * (b_[:k] - a_[:k])
        d = d[np.isfinite(d)]
        if len(d) >= 3:
            out.append(f"resonant peak {label} {float(np.median(d)):+.2f} st")
    if min(map(len, steady)) > sr // 10:
        p1, p2 = (cr.estimate_f0(s_, sr, f0) for s_ in steady)
        out.append(f"pitch {1200 * np.log2(p2 / p1):+.1f} cents")
    a, b = _cut(fa["stft"], fb["stft"])
    m = (a > a.max() - 50.0) & (b > b.max() - 50.0)
    if floor is not None:
        m &= (a > floor[None, :] + 10.0) & (b > floor[None, :] + 10.0)
    fb_hz = sr / spec.st_frame
    centres = np.sqrt(spec.st_edges[:-1] * spec.st_edges[1:]) * fb_hz
    d = np.where(m, np.abs(a - b), np.nan)
    with np.errstate(all="ignore"), warnings.catch_warnings():
        warnings.simplefilter("ignore", RuntimeWarning)
        per_band = np.nanmean(d, axis=0)
        per_frame = np.nanmean(d, axis=1)
    # When in the note: a difference that fades within the first few hundred
    # ms is a knob still moving (parameter smoothing), one that lasts is not.
    hop = spec.st_hop * 1000.0 / sr
    spans = []
    for t0, t1 in ((0, 150), (150, 500), (500, n1["gate_ms"])):
        v = per_frame[int(t0 / hop):int(t1 / hop)]
        v = v[np.isfinite(v)]
        if len(v):
            spans.append(f"{t0:.0f}-{t1:.0f} ms {float(np.mean(v)):.1f} dB")
    if spans:
        out.append("difference over the note: " + ", ".join(spans))
    worst = [i for i in np.argsort(np.nan_to_num(per_band, nan=-1.0))[::-1][:3] if np.isfinite(per_band[i])]
    if worst:
        out.append("most different around " + ", ".join(f"{centres[i]:.0f} Hz ({per_band[i]:.1f} dB)" for i in worst))
    return "; ".join(out)


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
    floor = noise_floor(x, sr, notes, offset)
    peak_db = 20 * np.log10(max(1e-12, float(np.max(np.abs(x)))))
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
    # P12 (the same note twice) is the source's repeatability: P08 and P11
    # compare two notes that should be identical too, so only what they
    # differ by beyond it counts.
    repeat = stft_rms(S["P12"][0], S["P12"][1], 1300.0, floor=floor) if "P12" in S else float("nan")
    base = repeat if np.isfinite(repeat) else 0.0
    if "P08" in S:   # (150 ms, no), (150, acc), (1300, no), (1300, acc): the first 140 ms must match
        v = max(stft_rms(S["P08"][0], S["P08"][2], 140.0, floor=floor),
                stft_rms(S["P08"][1], S["P08"][3], 140.0, floor=floor))
        add("P08", "gate length changes the note before gate-off", v, 0.0, max(0.0, v - base), "dB", "< 1 dB",
            v - base < 1.0, "the source looks ahead to the gate (or isn't one-shot): keep gates the same everywhere")
    # The square's missing even harmonics make its peak track ~1 st noisier.
    pair("P09", "waveform x Env Mod, peak track", "add square notes to 2B", limit=2.0)
    pair("P10", "accent sweep vs Cutoff, peak track", "repeat 3A at Cutoff 75")
    if "P11" in S:
        v = stft_rms(S["P11"][0], S["P11"][1], 1300.0, floor=floor)
        add("P11", "Accent knob on an unaccented note", v, 0.0, max(0.0, v - base), "dB", "< 0.5 dB",
            v - base < 0.5, "the knob acts without accent: add unaccented notes to the 3A Accent knob sweep")
    if "P12" in S:
        add("P12", "the same note twice (repeatability)", repeat, 0.0, repeat, "dB", "< 0.5 dB", repeat < 0.5,
            "the source is not repeatable: turn off noise / drift / analog variation, see below")
    notes_p12 = [n for n in notes if n["set"] == "P12"]

    lines = ["# Stage 0: separability probes", "",
             f"WAV offset against the MIDI: {offset:+.1f} ms. `source` and `Acidus` are the interaction",
             "measured in each; `left over` is what the source has that Acidus's structure doesn't",
             "(peak tracks: median over frames, semitones). P04, P08, P11, P12 should be zero in the source;",
             "P08 and P11 count only what exceeds P12, the source's own note-to-note difference.", "",
             "| probe | what | source | Acidus | left over | limit | verdict | if it interacts |",
             "|---|---|---|---|---|---|---|---|"]
    lines += [f"| {' | '.join(r)} |" for r in rows]
    lines += [""]
    if floor is not None:
        lines.append(f"Source noise floor between notes: loudest band {float(np.max(floor)) - peak_db:+.0f} dB "
                     "against the peak sample (spectrogram cells within 10 dB of it are not compared).")
    if len(notes_p12) == 2:
        lines.append(f"P12, second note against the first: {repeat_diagnosis(x, sr, notes_p12[0], notes_p12[1], offset, floor)}.")
        if np.isfinite(repeat) and repeat >= 0.5:
            lines += ["The fit needs a repeatable source: two identical notes should match to well under",
                      "0.5 dB. A level difference points at drift or random variation; a pitch",
                      "difference at oscillator drift; differences only in the top bands at noise or",
                      "random phase. A resonant peak that moves between the two notes, with level and",
                      "pitch equal, is a filter that varies per note (an \"analog\" or component-",
                      "tolerance option): turn it off. A difference near the filter's frequency that fades over the",
                      "note is a knob still moving when the note starts: the first P12 note follows",
                      "a knob change, the second does not. Check that the emulation (or the DAW's",
                      "MIDI learn) does not smooth CC changes, or regenerate the MIDI files (the",
                      "knob CCs now come 1.2 s before each note) and render again. P08 and P11",
                      "only count what they exceed P12 by, so they are unreliable until P12 passes."]
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
    for k in ([stage] if alone else range(0 if (with_probes or stage == 0) else 1, stage + 1)):
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


# Filter constants that only act in one coupling-network mode
# (filterCouplingNetwork, src/core/Filter.cpp): freeing them in the other
# mode only lets them wander.
OPEN303_NETWORK_ONLY = ("filterInputCouplingHz", "resCouplingHz", "filterNotchHz",
                        "filterNotchBandwidthHz", "filterAllpassHz")
STINCHCOMBE_NETWORK_ONLY = ("filterNetworkTimeScale",)
NEVER_FITTED = {
    "filterLadderTopology": "a switch (ladder orientation): try both by hand",
    "filterCouplingNetwork": "a switch (Open303 or Stinchcombe coupling network): pick it with --start",
    "vcoOctaveScale": "measure it from the 3C pitches with a tuner",
}


def stage_parameters(stage, start):
    """(tuned, inactive, kept) for a stage starting from a profile: the
    constants the fit varies, the ones the stage would free but that do
    nothing in the profile's coupling-network mode, and the rest."""
    params = json.loads(Path(start).read_text(encoding="utf-8")).get("parameters", {})
    network = params.get("filterCouplingNetwork", 0.0) >= 0.5
    inactive = STINCHCOMBE_NETWORK_ONLY if not network else OPEN303_NETWORK_ONLY
    only = set(STAGE_ONLY[stage].split(","))
    selected = [k for k, v in cr.MODEL_PARAMS.items() if k in only or v[3] in only]
    tuned = [k for k in selected if k not in inactive]
    off = [k for k in selected if k in inactive]
    kept = [k for k in cr.MODEL_PARAMS if k not in selected] + list(NEVER_FITTED)
    return tuned, off, kept


def print_parameters(stage, start):
    tuned, off, kept = stage_parameters(stage, start)
    print(f"stage {stage}, starting from {start}:")
    print(f"\n  tuned ({len(tuned)}):")
    for k in tuned:
        print(f"    {k:26s} group {cr.MODEL_PARAMS[k][3]}")
    if off:
        print(f"\n  not tuned, no effect in this profile's coupling-network mode ({len(off)}):")
        for k in off:
            print(f"    {k}")
    print(f"\n  kept at the starting profile's value ({len(kept)}):")
    for k in kept:
        print(f"    {k:26s} " + (NEVER_FITTED[k] if k in NEVER_FITTED else f"group {cr.MODEL_PARAMS[k][3]}"))
    print("\n  also fitted: a note-on shift and gate offset (timing); one overall level gain is solved directly.")
    print("  never fitted: the panel knobs (exact in an emulation, --fix knobs), Volume, Drive, Tuning.")


def default_start(src, stage):
    """The profile the next round starts from: the newest of the previous
    stage's fit and any trimmed version of it exported from the plugin
    (calibrations/<src>-stage<N-1>*.json, e.g. <src>-stage1-trimmed.json),
    else an earlier stage's, else the x0x profile."""
    for k in range(stage - 1, -1, -1):
        found = sorted((REPO / "calibrations").glob(f"{src}-stage{k}*.json"), key=lambda p: p.stat().st_mtime)
        if found:
            return found[-1]
    return REPO / "calibrations" / "x0x.json"


def write_float_wav(path, x, sr):
    """Mono 32-bit float WAV (no rounding of the source's samples)."""
    data = np.asarray(x, dtype="<f4").tobytes()
    with open(path, "wb") as f:
        f.write(b"RIFF" + struct.pack("<I", 36 + len(data)) + b"WAVE")
        f.write(b"fmt " + struct.pack("<IHHIIHH", 16, 3, 1, sr, sr * 4, 4, 32))
        f.write(b"data" + struct.pack("<I", len(data)) + data)


def check_channels(path, channel):
    """A stereo render whose channels differ is mixed to mono by the tools,
    and if the two sides' phase relation changes from note to note (a chorus,
    stereo width, separate oscillators per side) the mix changes too. Report
    how different the sides are; with --channel keep one side only."""
    xs, sr = cr.read_wav(path, mix=False)
    if xs.ndim == 1 or xs.shape[1] < 2:
        if channel != "mix":
            print(f"note: the render is mono, --channel {channel} has nothing to pick")
        return
    left, right = xs[:, 0], xs[:, 1]
    mid = float(np.sqrt(np.mean((left + right) ** 2)))
    side = float(np.sqrt(np.mean((left - right) ** 2)))
    ratio = 20 * np.log10(max(side, 1e-12) / max(mid, 1e-12))
    print(f"stereo check: left - right is {ratio:.1f} dB below left + right"
          + (" (the channels are identical)" if ratio < -80 else ""))
    if ratio > -40 and channel == "mix":
        print("warning: the left and right channels differ. The tools mix them to mono, so a stereo effect "
              "(chorus, width, unison, separate oscillators per side) in the emulation changes the notes from "
              "one to the next. Turn it off and render again, or analyse one side with --channel left")
    if channel in ("left", "right"):
        write_float_wav(path, left if channel == "left" else right, sr)
        print(f"using the {channel} channel only")


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
    ap.add_argument("--start", help="profile to start from, e.g. one exported from the plugin (default: the "
                    "newest calibrations/<src>-stage<N-1>*.json, so a trimmed export of the previous stage wins; "
                    "else calibrations/x0x.json)")
    ap.add_argument("--channel", choices=("mix", "left", "right"), default="mix",
                    help="which channel of a stereo render to analyse (default: the mix of both)")
    ap.add_argument("--tune-cents", type=float, default=0.0, help="the source's tuning offset (default 0)")
    ap.add_argument("--alone", action="store_true", help="fit this stage's notes only, not earlier stages too")
    ap.add_argument("--list-params", action="store_true",
                    help="print which constants this stage tunes and which it keeps, then stop")
    ap.add_argument("--probes-only", action="store_true",
                    help="stage 0: write the probe report, don't fit")
    ap.add_argument("--with-probes", action="store_true",
                    help="also fit the stage 0 probe notes (44 more notes: slower, little new information)")
    ap.add_argument("--tag", help="name this run: it writes fit-<tag>/, ab-<tag>.wav, acidus-<tag>.wav and "
                    "calibrations/<src>-stage<N>-<tag>.json instead of the plain names, so a side experiment "
                    "(e.g. --include P07-4 to fit one note) leaves the full fit alone")
    ap.add_argument("--network", choices=("open303", "stinchcombe"),
                    help="force the filter's coupling-network model in the starting profile (default: as the "
                    "profile has it)")
    ap.add_argument("--minutes", type=float, default=30.0, help="fit time cap (default 30)")
    ap.add_argument("--patience", type=float, default=None,
                    help="stop after this many minutes without progress (default: a third of --minutes)")
    ap.add_argument("--evaluate-only", action="store_true", help="score the start profile, no fit")
    args, extra = ap.parse_known_args()

    src, stage = args.source_id, args.stage
    base = Path(args.dir) if args.dir else REPO / "test" / "resources" / src
    if args.list_params:
        if stage == 4:
            fail("stage 4 is not fitted")
        print_parameters(stage, Path(args.start) if args.start else default_start(src, stage))
        return
    mid = Path(args.midi) if args.midi else base / f"{src}-stage{stage}.mid"
    if not mid.exists():
        fail(f"{mid} not found: run tools/emulation_training_set.py {src} --out {base}")
    if stage == 4:
        fail("stage 4 (slides, accent runs) can't be fitted yet: the calibrator renders one note per clip "
             "and the slide time is fixed. Use it for listening only.")
    if bool(args.wav) == bool(args.rehearse):
        fail("give exactly one of --wav and --rehearse")
    start = Path(args.start) if args.start else default_start(src, stage)
    notes = stage_notes(mid)
    work = base / f"stage{stage}"
    work.mkdir(parents=True, exist_ok=True)
    suffix = f"-{args.tag}" if args.tag else ""
    if args.network:
        prof = json.loads(start.read_text(encoding="utf-8"))
        prof.setdefault("parameters", {})["filterCouplingNetwork"] = 1.0 if args.network == "stinchcombe" else 0.0
        start = work / f"start{suffix}.json"
        start.write_text(json.dumps(prof, indent=1) + "\n", encoding="utf-8")
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
    check_channels(work / wav_name, args.channel)
    x, sr = cr.read_wav(work / wav_name)
    peak = float(np.max(np.abs(x)))
    if peak >= 0.999:
        fail(f"{wav_name} clips (peak {peak:.3f}): lower the output level and render again")
    if peak < 1e-3:
        fail(f"{wav_name} is silent")

    acidus = Acidus(start, args.tune_cents)
    offset = align(x, sr, notes, acidus)
    print(f"WAV offset against the MIDI: {offset:+.1f} ms, peak {20 * np.log10(peak):.1f} dBFS")
    if peak > 10 ** (-3.0 / 20.0):
        print("warning: the render peaks above -3 dBFS. If the emulation or the DAW limits or clips anywhere, "
              "the loudest notes are bent; lower the output so the peak sits near -12 dBFS (and keep that "
              "level for every later stage)")
    write_manifest(work, src, stage, notes, wav_name, offset, args.tune_cents)

    if stage == 0:
        sim = acidus.timeline([n for n in notes if n["set"].startswith("P")], sr, len(x))
        probe_report(x, sim, sr, notes, offset, work / "probes.md")
        print(f"\nwrote {work / 'probes.md'}")
        if args.probes_only:
            return

    manifest, n_clips = merged_manifest(base, src, stage, args.alone, args.with_probes)
    fit_dir = work / f"fit{suffix}"
    cmd = [sys.executable, str(TOOLS / "calibrate_reference.py"), "--manifest", str(manifest),
           "--calibration", str(start), "--fix", ",".join(["knobs"] + list(stage_parameters(stage, start)[1])),
           "--w-sweep", "1", "--only", STAGE_ONLY[stage],
           "--max-minutes", str(args.minutes), "--out", str(fit_dir), "--no-sensitivity",
           "--patience-minutes", str(args.patience if args.patience is not None else args.minutes / 3.0)]
    if args.evaluate_only:
        cmd.append("--evaluate-only")
    first = stage if args.alone else (0 if (args.with_probes or stage == 0) else 1)
    print(f"fitting {n_clips} notes (stages {first}-{stage}) for up to {args.minutes:g} min ...", flush=True)
    subprocess.run(cmd + extra, check=True)

    result = json.loads((fit_dir / "result.json").read_text())
    ckpt = fit_dir / "checkpoint.json"
    prof = json.loads((ckpt if ckpt.exists() and not args.evaluate_only else start).read_text())
    prof.pop("checkpoint", None)
    prof.update({"name": f"{src}-stage{stage}{suffix}", "source": f"{src}, stage {stage} of docs/EMULATION_TRAINING_PLAN.md (tools/fit_stage.py)",
                 "fitted": datetime.date.today().isoformat(), "reference_set": str(manifest.relative_to(REPO))
                 if manifest.is_relative_to(REPO) else str(manifest), "started_from": str(start)})
    out_prof = REPO / "calibrations" / f"{src}-stage{stage}{suffix}.json"
    if not args.evaluate_only:
        out_prof.write_text(json.dumps(prof, indent=1) + "\n")

    after = Acidus(out_prof if not args.evaluate_only else start, args.tune_cents)
    gain = 10 ** (result["after"]["aggregate"]["gain_db"] / 20.0)
    # --include narrows the fit to some notes; the listening files follow it.
    inc = [e for i, a in enumerate(extra) if a == "--include" and i + 1 < len(extra) for e in extra[i + 1].split(",") if e]
    inc += [e for a in extra if a.startswith("--include=") for e in a.split("=", 1)[1].split(",") if e]
    heard = [n for n in notes if any(e in n["id"] for e in inc)] if inc else notes
    write_ab(work / f"ab{suffix}.wav", x, sr, heard, offset, after, gain)
    cr.write_wav(work / f"acidus{suffix}.wav", after.timeline(heard, sr, len(x), offset, gain), sr)

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
    print(f"listen   {work / f'ab{suffix}.wav'}  (source, then Acidus, note by note)")
    print(f"         {work / f'acidus{suffix}.wav'}  (Acidus on the render's timeline, for the DAW)")
    if stage < 3:
        print(f"next     render {src}-stage{stage + 1}.mid, then tools/fit_stage.py {src} {stage + 1} --wav <render>")


if __name__ == "__main__":
    main()
