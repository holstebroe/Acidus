#!/usr/bin/env python3
"""Fit Acidus' DSP parameters to hardware TB-303 reference samples.

Reads the reference notes in test/resources (file-name grammar documented in
test/resources/reference_resources.md), renders the same notes through the
plugin's own SynthEngine (via the acidus_calibration_render shared library,
built from src/calibration/CalibrationRender.cpp), and runs a time-capped
CMA-ES search over every model constant plus the *hardware knob positions*
until the rendered notes match the references as closely as possible.

What is fitted
--------------
* Model constants: every float in acidus::SynthParameters that shapes the
  sound (filter ladder, coupling networks, envelopes, knob laws, VCA, ...).
  See MODEL_PARAMS below for bounds; --fix / --only restrict the set.
* Knob positions: we don't know exactly where the hardware's knob end stops
  sit relative to the plugin's knob law, so every (knob, digit) pair used in
  the file names -- e.g. "cutoff@0", "cutoff@5", "cutoff@1" -- is a free
  parameter shared by all samples that use it, with a weak prior pulling it
  toward its nominal position (0 / 0.5 / 1).
* Tuning: measured per sample from the harmonic peak positions of the full
  note's DFT (sub-cent accuracy) and applied directly -- a derivative-free
  search can't beat an explicit measurement for this.
* Note timing: one global note-on offset and gate length (all samples come
  from the same sequencer at 142 BPM).
* Recording gain: solved in closed form for every candidate (one global
  gain, so relative levels between samples still have to match).

How a match is scored (all errors are in dB, lower is better)
-------------------------------------------------------------
* harm  -- level of every harmonic (energy within +-f0/4 of k*f0) in the
           full-note, non-windowed DFT, up to --fmax. Weighted by k^-0.5 so
           the low harmonics that carry most of the sound count more, while
           the upper harmonics still matter.
* inter -- energy *between* harmonics. Catches spurious (e.g. aliasing)
           peaks that the hardware doesn't have.
* env   -- short-time RMS envelope (window = 2 pitch periods, 1 ms hop).
* stft  -- 1/3-octave band levels over time (23 ms frames): filter sweeps.
The hardware drifts slightly in pitch/level over a note, so every feature is
deliberately insensitive to exact waveform phase.

Output (in --out, default calibration_results/<timestamp>/)
------------------------------------------------------------
report.md                 before/after statistics, per-sample ranking,
                          suspicious-label analysis, fitted parameter list
result.json               everything machine-readable
synth_parameters.txt      C++ lines to paste into SynthEngine.hpp
renders/*.wav             reference / before / after audio for listening
--apply                   writes the fitted defaults into src/core/SynthEngine.hpp

Usage
-----
    python3 tools/calibrate_reference.py                       # 20 min cap
    python3 tools/calibrate_reference.py --max-minutes 60 --patience-minutes 8
    python3 tools/calibrate_reference.py --only filter,knobs   # subset
    python3 tools/calibrate_reference.py --evaluate-only       # just score the current code

Requires numpy. The shared library is built automatically (cmake) if missing.
"""

import argparse
import ctypes
import datetime
import json
import math
import os
import re
import struct
import subprocess
import sys
import time
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

try:
    import numpy as np
except ImportError:  # pragma: no cover
    sys.exit("calibrate_reference.py needs numpy: pip install numpy")

REPO = Path(__file__).resolve().parents[1]

# ---------------------------------------------------------------------------
# Parameter space
# ---------------------------------------------------------------------------

# name -> (lo, hi, log-scaled, group). Names are SynthParameters fields.
MODEL_PARAMS = {
    # Oscillator
    "oscCouplingHz":            (15.0, 120.0, True, "osc"),
    "oscSawLpfHz":              (4000.0, 40000.0, True, "osc"),
    "oscSawShape":              (-0.4, 0.4, False, "osc"),
    # Filter
    "filterFeedbackGain":       (6.0, 22.0, False, "filter"),
    "filterFeedbackHeadroomHz": (0.0, 20000.0, False, "filter"),
    "filterResonanceSkew":      (0.05, 8.0, False, "filter"),
    "filterMaxResonanceOutputGain": (0.5, 6.0, True, "filter"),
    "resCouplingHz":            (40.0, 400.0, True, "filter"),
    "filterResCouplingTrackHz": (0.0, 400.0, False, "filter"),
    "filterCapScale1":          (0.2, 4.0, True, "filter"),
    "filterCapScale2":          (0.2, 4.0, True, "filter"),
    "filterCapScale3":          (0.2, 4.0, True, "filter"),
    "filterCapScale4":          (0.2, 4.0, True, "filter"),
    "filterLadderInputScale":   (0.01, 0.4, True, "filter"),
    "filterInputCouplingHz":    (3.0, 60.0, True, "filter"),
    "filterOutputCouplingHz":   (6000.0, 40000.0, True, "filter"),
    "filterPostHpHz":           (5.0, 60.0, True, "filter"),
    "filterNotchHz":            (2.0, 25.0, True, "filter"),
    "filterNotchBandwidthHz":   (1.0, 15.0, True, "filter"),
    "filterAllpassHz":          (4.0, 50.0, True, "filter"),
    # Knob laws / CV summing
    "cutoffBaseHz":             (50.0, 800.0, True, "cv"),
    "cutoffSpanOct":            (1.5, 7.0, False, "cv"),
    "cutoffTaperExp":           (0.5, 3.5, False, "cv"),
    "envModOffsetOct":          (0.0, 2.5, False, "cv"),
    "envModDepthOct":           (0.5, 7.0, False, "cv"),
    "accentSweepDepthOct":      (0.0, 5.0, False, "cv"),
    "accentVcaDepth":           (0.0, 2.5, False, "cv"),
    # Envelopes / VCA
    "vcfAttackMs":              (0.3, 20.0, True, "env"),
    "vcaAttackMs":              (0.3, 20.0, True, "env"),
    "vcfDecayMinSec":           (0.05, 0.8, True, "env"),
    "vcfDecayMaxSec":           (0.8, 6.0, True, "env"),
    "accentDecaySec":           (0.05, 0.8, True, "env"),
    "vegDecaySec":              (0.5, 10.0, True, "env"),
    "vcaGateOffMs":             (0.3, 20.0, True, "env"),
    "vcaGateOffAccentMs":       (0.3, 80.0, True, "env"),
    "vcaGainSaturationDrive":   (0.1, 10.0, True, "env"),
}

# Plugin-side front-panel parameters that the knob positions map onto.
KNOBS = {"c": "cutoff", "r": "resonance", "e": "envMod", "d": "decay", "a": "accent"}
KNOB_PRIOR_SPAN = 0.30   # knob positions may move this far from nominal
TIMING_PARAMS = ("onsetMs", "gateMs")


def digit_to_position(d):
    """reference_resources.md: 0 = minimum, 1 = maximum, 5 = halfway;
    other digits are reserved for intermediate positions (read as d/10)."""
    return {0: 0.0, 1: 1.0}.get(d, d / 10.0)


# ---------------------------------------------------------------------------
# Reference parsing / loading
# ---------------------------------------------------------------------------

NAME_RE = re.compile(
    r"^(?P<src>303)_(?P<wave>saw|square)-(?P<note>[A-G]#?[0-9])"
    r"t(?P<tune>-?[0-9]+)c(?P<c>[0-9])r(?P<r>[0-9])e(?P<e>[0-9])d(?P<d>[0-9])a(?P<a>[0-9])$")
NOTE_SEMITONES = {"C": 0, "D": 2, "E": 4, "F": 5, "G": 7, "A": 9, "B": 11}


def note_to_midi(note):
    semis = NOTE_SEMITONES[note[0]] + (1 if "#" in note else 0)
    return 12 * (int(note[-1]) + 1) + semis


def midi_to_hz(m):
    return 440.0 * 2.0 ** ((m - 69) / 12.0)


def read_wav(path):
    data = Path(path).read_bytes()
    if data[0:4] != b"RIFF" or data[8:12] != b"WAVE":
        raise ValueError(f"{path}: not a RIFF/WAVE file")
    pos, fmt, pcm = 12, None, None
    while pos + 8 <= len(data):
        cid = data[pos:pos + 4]
        size = struct.unpack("<I", data[pos + 4:pos + 8])[0]
        body = data[pos + 8:pos + 8 + size]
        if cid == b"fmt ":
            fmt = struct.unpack("<HHIIHH", body[:16])
            if fmt[0] == 0xFFFE and len(body) >= 26:
                fmt = (struct.unpack("<H", body[24:26])[0],) + fmt[1:]
        elif cid == b"data":
            pcm = body
        pos += 8 + size + (size & 1)
    if fmt is None or pcm is None:
        raise ValueError(f"{path}: missing fmt/data chunk")
    tag, ch, sr, _, _, bits = fmt
    if tag == 3 and bits == 32:
        x = np.frombuffer(pcm, dtype="<f4").astype(np.float64)
    elif bits == 16:
        x = np.frombuffer(pcm[:len(pcm) // 2 * 2], dtype="<i2") / 32768.0
    elif bits == 24:
        b = np.frombuffer(pcm[:len(pcm) // 3 * 3], dtype=np.uint8).reshape(-1, 3).astype(np.int32)
        v = b[:, 0] | (b[:, 1] << 8) | (b[:, 2] << 16)
        x = np.where(v >= 1 << 23, v - (1 << 24), v) / float(1 << 23)
    elif bits == 32:
        x = np.frombuffer(pcm, dtype="<i4") / 2147483648.0
    else:
        raise ValueError(f"{path}: unsupported WAV format tag={tag} bits={bits}")
    x = np.asarray(x, dtype=np.float64)
    if ch > 1:
        x = x[: len(x) // ch * ch].reshape(-1, ch).mean(axis=1)
    return x, sr


def write_wav(path, x, sr):
    x = np.clip(np.asarray(x, dtype=np.float64), -1.0, 1.0)
    pcm = (x * 32767.0).astype("<i2").tobytes()
    with open(path, "wb") as f:
        f.write(b"RIFF" + struct.pack("<I", 36 + len(pcm)) + b"WAVE")
        f.write(b"fmt " + struct.pack("<IHHIIHH", 16, 1, 1, sr, sr * 2, 2, 16))
        f.write(b"data" + struct.pack("<I", len(pcm)) + pcm)


class Reference:
    def __init__(self, path, accent_mode):
        self.path = Path(path)
        self.name = self.path.stem
        m = NAME_RE.match(self.name)
        if not m:
            raise ValueError(f"{self.path.name}: file name does not follow reference_resources.md grammar")
        self.waveform = 0 if m["wave"] == "saw" else 1
        self.note = m["note"]
        self.midi = note_to_midi(m["note"])
        self.label_tune_cents = float(m["tune"])
        self.digits = {KNOBS[k]: int(m[k]) for k in KNOBS}
        # The file name doesn't say whether the step was accented; the Accent
        # knob only does anything on an accented step, so by default a
        # non-minimum Accent knob implies the step was recorded with accent.
        if accent_mode == "knob":
            self.accent_step = self.digits["accent"] != 0
        else:
            self.accent_step = accent_mode == "all"
        x, sr = read_wav(self.path)
        tail = max(8, len(x) // 100)
        self.x = x - x[-tail:].mean()
        self.sr = sr
        self.n = len(self.x)


# ---------------------------------------------------------------------------
# Analysis
# ---------------------------------------------------------------------------

def full_spectrum(x, nfft):
    """Non-windowed DFT of the whole note (the notes start and end at zero)."""
    return np.abs(np.fft.rfft(x, nfft))


def parabolic_peak(mag, i):
    if i <= 0 or i >= len(mag) - 1:
        return float(i)
    a, b, c = np.log(mag[i - 1] + 1e-30), np.log(mag[i] + 1e-30), np.log(mag[i + 1] + 1e-30)
    den = a - 2 * b + c
    return i + (0.5 * (a - c) / den if den != 0 else 0.0)


def estimate_f0(x, sr, f_guess, nfft=1 << 18):
    """Harmonic-comb search, then weighted least squares on the parabolic-
    interpolated peaks of the first strong harmonics."""
    mag = full_spectrum(x, nfft)
    df = sr / nfft
    logmag = np.log(mag + 1e-12)
    kmax = int(min(30, 5000.0 / f_guess))
    best, best_f = -np.inf, f_guess
    for cents in np.arange(-80.0, 80.0, 0.5):
        f = f_guess * 2 ** (cents / 1200.0)
        idx = np.round(np.arange(1, kmax + 1) * f / df).astype(int)
        s = logmag[idx].sum()
        if s > best:
            best, best_f = s, f
    num = den = 0.0
    peaks = []
    for k in range(1, kmax + 1):
        c = k * best_f / df
        lo, hi = int(c - 0.15 * best_f / df), int(c + 0.15 * best_f / df) + 1
        i = lo + int(np.argmax(mag[lo:hi]))
        peaks.append((k, parabolic_peak(mag, i) * df, mag[i]))
    top = max(p[2] for p in peaks)
    for k, fk, a in peaks:
        if a > top * 10 ** (-40 / 20):
            w = a / top
            num += w * k * fk
            den += w * k * k
    return num / den if den > 0 else best_f


def pitch_drift_cents(x, sr, f0, frame=4096, hop=1024):
    """Std-dev (cents) of short-time f0 over the sustained part of the note:
    a measure of the hardware's pitch fluctuation."""
    if len(x) < frame + hop:
        return 0.0
    rms = np.array([np.sqrt(np.mean(x[i:i + frame] ** 2)) for i in range(0, len(x) - frame, hop)])
    ests = []
    for j, i in enumerate(range(0, len(x) - frame, hop)):
        if rms[j] < 0.3 * rms.max():
            continue
        seg = x[i:i + frame] * np.hanning(frame)
        mag = np.abs(np.fft.rfft(seg, 1 << 16))
        df = sr / (1 << 16)
        fs = []
        for k in range(1, 9):
            c = k * f0 / df
            lo, hi = int(c - 0.2 * f0 / df), int(c + 0.2 * f0 / df) + 1
            ii = lo + int(np.argmax(mag[lo:hi]))
            fs.append(parabolic_peak(mag, ii) * df / k)
        ests.append(np.median(fs))
    if len(ests) < 2:
        return 0.0
    return float(np.std(1200 * np.log2(np.array(ests) / f0)))


class FeatureSpec:
    """Frequency/time grids shared by a reference and its renders."""

    def __init__(self, ref, f0, fmax, nfft):
        self.sr = ref.sr
        self.n = ref.n
        self.f0 = f0
        self.nfft = nfft
        df = ref.sr / nfft
        self.K = int(min(fmax, 0.49 * ref.sr) / f0)
        k = np.arange(1, self.K + 1)
        # Harmonic bands: +-f0/4 around k*f0; inter-harmonic: the rest.
        self.h_lo = np.round((k - 0.25) * f0 / df).astype(int)
        self.h_hi = np.round((k + 0.25) * f0 / df).astype(int)
        self.i_lo = self.h_hi[:-1]
        self.i_hi = self.h_lo[1:]
        self.harm_freqs = k * f0
        self.harm_w = k ** -0.5
        self.inter_w = (k[:-1] + 0.5) ** -0.5
        # Envelope: RMS over 2 pitch periods, 1 ms hop.
        self.env_win = max(16, int(round(2 * ref.sr / f0)))
        self.env_hop = max(1, ref.sr // 1000)
        # Spectrogram bands: 1/3 octave from 80 Hz to fmax.
        self.st_frame, self.st_hop = 1024, 256
        edges = 80.0 * 2 ** (np.arange(0, 40) / 3.0)
        edges = edges[edges <= min(fmax, 0.49 * ref.sr)]
        fb = ref.sr / self.st_frame
        self.st_edges = np.unique(np.maximum(1, np.round(edges / fb).astype(int)))
        self.st_window = np.hanning(self.st_frame)


def db(p):
    return 10.0 * np.log10(np.maximum(p, 1e-20))


def compute_features(x, spec):
    x = np.asarray(x, dtype=np.float64)
    mag2 = full_spectrum(x, spec.nfft) ** 2
    cs = np.concatenate([[0.0], np.cumsum(mag2)])
    harm = cs[spec.h_hi] - cs[spec.h_lo]
    inter = cs[spec.i_hi] - cs[spec.i_lo]
    # Normalise by the analysis length so levels are comparable across notes.
    norm = 1.0 / (spec.n * spec.n)
    e2 = np.concatenate([[0.0], np.cumsum(x * x)])
    starts = np.arange(0, spec.n - spec.env_win, spec.env_hop)
    env = (e2[starts + spec.env_win] - e2[starts]) / spec.env_win
    frames = np.lib.stride_tricks.sliding_window_view(x, spec.st_frame)[::spec.st_hop]
    S = np.abs(np.fft.rfft(frames * spec.st_window, axis=1)) ** 2
    Sc = np.concatenate([np.zeros((S.shape[0], 1)), np.cumsum(S, axis=1)], axis=1)
    bands = Sc[:, spec.st_edges[1:]] - Sc[:, spec.st_edges[:-1]]
    return {
        "harm": db(harm * norm),
        "inter": db(inter * norm),
        "env": db(env),
        "stft": db(bands / spec.st_frame ** 2),
    }


FLOORS = {"harm": 80.0, "inter": 80.0, "env": 50.0, "stft": 70.0}


def feature_errors(rf, sf, gain_db, spec):
    """Per-feature error vectors (dB), both sides clamped at a floor relative
    to the reference's own maximum so the fit doesn't chase the noise floor."""
    out = {}
    for key in ("harm", "inter", "env", "stft"):
        r = rf[key]
        s = sf[key] + gain_db
        fl = r.max() - FLOORS[key] if key != "inter" else rf["harm"].max() - FLOORS[key]
        rc, sc = np.maximum(r, fl), np.maximum(s, fl)
        e = sc - rc
        if key in ("env", "stft"):
            e = e[(r > fl) | (s > fl)]
        out[key] = e
    return out


def weighted_rms(e, w=None):
    if e.size == 0:
        return 0.0
    if w is None:
        return float(np.sqrt(np.mean(e * e)))
    return float(np.sqrt(np.sum(w * e * e) / np.sum(w)))


# ---------------------------------------------------------------------------
# Renderer (ctypes)
# ---------------------------------------------------------------------------

def lib_filename():
    if sys.platform == "win32":
        return "acidus_calibration_render.dll"
    if sys.platform == "darwin":
        return "acidus_calibration_render.dylib"
    return "acidus_calibration_render.so"


def build_library(build_dir):
    print(f"Building acidus_calibration_render in {build_dir} ...", flush=True)
    subprocess.run(["cmake", "-S", str(REPO), "-B", str(build_dir), "-DCMAKE_BUILD_TYPE=Release"],
                   check=True, stdout=subprocess.DEVNULL)
    subprocess.run(["cmake", "--build", str(build_dir), "--config", "Release",
                    "--target", "acidus_calibration_render", "-j"], check=True)


def find_library(build_dir, rebuild):
    if rebuild:
        build_library(build_dir)
    for cand in [build_dir / lib_filename(), build_dir / "Release" / lib_filename()]:
        if cand.exists():
            return cand
    build_library(build_dir)
    for cand in [build_dir / lib_filename(), build_dir / "Release" / lib_filename()]:
        if cand.exists():
            return cand
    sys.exit("Could not build/find the acidus_calibration_render library")


class Renderer:
    def __init__(self, lib_path):
        lib = ctypes.CDLL(str(lib_path))
        lib.acidus_calib_param_count.restype = ctypes.c_int
        lib.acidus_calib_param_name.restype = ctypes.c_char_p
        lib.acidus_calib_param_name.argtypes = [ctypes.c_int]
        lib.acidus_calib_param_default.restype = ctypes.c_double
        lib.acidus_calib_param_default.argtypes = [ctypes.c_int]
        lib.acidus_calib_render.restype = ctypes.c_int
        lib.acidus_calib_render.argtypes = [
            ctypes.POINTER(ctypes.c_double), ctypes.c_int, ctypes.c_int, ctypes.c_int,
            ctypes.c_double, ctypes.c_int, ctypes.c_int, ctypes.POINTER(ctypes.c_float)]
        self.lib = lib
        n = lib.acidus_calib_param_count()
        self.names = [lib.acidus_calib_param_name(i).decode() for i in range(n)]
        self.index = {nm: i for i, nm in enumerate(self.names)}
        self.defaults = np.array([lib.acidus_calib_param_default(i) for i in range(n)])

    def render(self, values, waveform, midi, accent, sr, gate, total):
        v = np.ascontiguousarray(values, dtype=np.float64)
        out = np.zeros(total, dtype=np.float32)
        rc = self.lib.acidus_calib_render(
            v.ctypes.data_as(ctypes.POINTER(ctypes.c_double)), int(waveform), int(midi),
            1 if accent else 0, float(sr), int(gate), int(total),
            out.ctypes.data_as(ctypes.POINTER(ctypes.c_float)))
        return out.astype(np.float64), rc == 0


# ---------------------------------------------------------------------------
# Problem definition
# ---------------------------------------------------------------------------

class Param:
    def __init__(self, key, lo, hi, log, init, kind, group, target=None, nominal=None):
        self.key, self.lo, self.hi, self.log = key, lo, hi, log
        self.init, self.kind, self.group = init, kind, group
        self.target, self.nominal = target, nominal

    def to_real(self, u):
        u = min(max(u, 0.0), 1.0)
        if self.log:
            return self.lo * (self.hi / self.lo) ** u
        return self.lo + (self.hi - self.lo) * u

    def to_unit(self, v):
        v = min(max(v, self.lo), self.hi)
        if self.log:
            return math.log(v / self.lo) / math.log(self.hi / self.lo)
        return (v - self.lo) / (self.hi - self.lo)


class Problem:
    def __init__(self, refs, renderer, args):
        self.refs = refs
        self.r = renderer
        self.args = args
        self.w = {"harm": args.w_harm, "inter": args.w_inter, "env": args.w_env, "stft": args.w_stft}
        self.pool = ThreadPoolExecutor(max_workers=args.workers)

        # Per-reference analysis
        for ref in refs:
            f_guess = midi_to_hz(ref.midi) * 2 ** (ref.label_tune_cents / 1200.0)
            ref.f0 = estimate_f0(ref.x, ref.sr, f_guess)
            ref.tune_cents = 1200 * math.log2(ref.f0 / midi_to_hz(ref.midi))
            ref.drift_cents = pitch_drift_cents(ref.x, ref.sr, ref.f0)
            ref.spec = FeatureSpec(ref, ref.f0, args.fmax, 1 << int(math.ceil(math.log2(ref.n * 4))))
            ref.feat = compute_features(ref.x, ref.spec)
            ref.onset_ms, ref.gate_ms = detect_timing(ref)

        # Parameter vector
        self.params = []
        only = set(args.only.split(",")) if args.only else None
        fixed = set(args.fix.split(",")) if args.fix else set()
        for name, (lo, hi, log, group) in MODEL_PARAMS.items():
            if name not in self.r.index:
                continue
            init = float(self.r.defaults[self.r.index[name]])
            if name in fixed or (only and group not in only and name not in only):
                continue
            lo, hi = min(lo, init), max(hi, init)
            self.params.append(Param(name, lo, hi, log, init, "model", group, target=name))
        used = sorted({(knob, ref.digits[knob]) for ref in refs for knob in KNOBS.values()})
        if not (only and "knobs" not in only) and "knobs" not in fixed:
            for knob, digit in used:
                nom = digit_to_position(digit)
                lo, hi = max(0.0, nom - KNOB_PRIOR_SPAN), min(1.0, nom + KNOB_PRIOR_SPAN)
                self.params.append(Param(f"{knob}@{digit}", lo, hi, False, nom, "knob", "knobs",
                                         target=knob, nominal=nom))
        self.knob_digits = used
        onset0 = float(np.median([r.onset_ms for r in refs]))
        gate0 = float(np.median([r.gate_ms for r in refs if not r.accent_step] or [r.gate_ms for r in refs]))
        self.timing_init = {"onsetMs": onset0, "gateMs": gate0}
        if not (only and "timing" not in only) and "timing" not in fixed:
            self.params.append(Param("onsetMs", -8.0, 8.0, False, onset0, "timing", "timing"))
            self.params.append(Param("gateMs", max(10.0, gate0 - 30), gate0 + 30, False, gate0, "timing", "timing"))
        self.u0 = np.array([p.to_unit(p.init) for p in self.params])

    # -- mapping ------------------------------------------------------------
    def decode(self, u):
        base = self.r.defaults.copy()
        knobs = {(k, d): digit_to_position(d) for k, d in self.knob_digits}
        timing = dict(self.timing_init)
        for p, ui in zip(self.params, u):
            v = p.to_real(ui)
            if p.kind == "model":
                base[self.r.index[p.target]] = v
            elif p.kind == "knob":
                knobs[(p.target, int(p.key.split("@")[1]))] = v
            else:
                timing[p.key] = v
        return base, knobs, timing

    def sample_values(self, base, knobs, ref, knob_override=None):
        v = base.copy()
        for knob in KNOBS.values():
            pos = knobs[(knob, ref.digits[knob])]
            if knob_override and knob in knob_override:
                pos = knob_override[knob]
            v[self.r.index[knob]] = pos
        v[self.r.index["tuningCents"]] = ref.tune_cents
        return v

    def render_ref(self, values, ref, timing):
        sr = ref.sr
        onset = int(round(timing["onsetMs"] * sr / 1000.0))
        gate = max(1, int(round(timing["gateMs"] * sr / 1000.0)))
        pre = max(0, -onset)
        y, ok = self.r.render(values, ref.waveform, ref.midi, ref.accent_step, sr, gate, ref.n + pre)
        if onset >= 0:
            y = np.concatenate([np.zeros(onset), y])[:ref.n]
        else:
            y = y[pre:pre + ref.n]
        return y, ok

    # -- evaluation ---------------------------------------------------------
    def _job(self, u, ref, knob_override=None):
        base, knobs, timing = self.decode(u)
        y, ok = self.render_ref(self.sample_values(base, knobs, ref, knob_override), ref, timing)
        if not ok or not np.all(np.isfinite(y)) or np.max(np.abs(y)) > 50:
            return None
        return compute_features(y, ref.spec)

    def solve_gain(self, feats, refs):
        num = den = 0.0
        for rf, sf, ref in zip([r.feat for r in refs], feats, refs):
            if sf is None:
                continue
            h = rf["harm"]
            m = h > h.max() - 40
            w = ref.spec.harm_w[m]
            num += np.sum(w * (h[m] - sf["harm"][m]))
            den += np.sum(w)
        return num / den if den > 0 else 0.0

    def sample_cost(self, ref, sf, gain):
        if sf is None:
            return 1e3, None
        e = feature_errors(ref.feat, sf, gain, ref.spec)
        comp = {
            "harm": weighted_rms(e["harm"], ref.spec.harm_w),
            "inter": weighted_rms(e["inter"], ref.spec.inter_w),
            "env": weighted_rms(e["env"]),
            "stft": weighted_rms(e["stft"]),
        }
        cost = sum(self.w[k] * comp[k] for k in comp) / sum(self.w.values())
        return cost, comp

    def prior(self, u):
        pen = 0.0
        for p, ui in zip(self.params, u):
            if p.kind == "knob":
                pen += self.args.knob_prior * ((p.to_real(ui) - p.nominal) / 0.1) ** 2
        return pen

    def evaluate_batch(self, U, gain=None):
        """Returns (costs, details) for a list of unit-space candidates."""
        jobs = [(ci, ri) for ci in range(len(U)) for ri in range(len(self.refs))]
        feats = list(self.pool.map(lambda j: self._job(U[j[0]], self.refs[j[1]]), jobs))
        nr = len(self.refs)
        costs, details = [], []
        for ci, u in enumerate(U):
            fs = feats[ci * nr:(ci + 1) * nr]
            g = self.solve_gain(fs, self.refs) if gain is None else gain
            per = [self.sample_cost(ref, sf, g) for ref, sf in zip(self.refs, fs)]
            data = float(np.mean([c for c, _ in per]))
            boundary = float(np.sum((np.asarray(u) - np.clip(u, 0, 1)) ** 2)) * 100.0
            costs.append(data + self.prior(np.clip(u, 0, 1)) + boundary)
            details.append({"gain_db": g, "data_cost": data, "per_sample": per, "feats": fs})
        return costs, details


def detect_timing(ref):
    """Rough note-on / gate-off times (ms) from the reference envelope; only
    used to initialise the fitted global timing parameters."""
    x, sr = ref.x, ref.sr
    win = max(16, int(sr / ref.f0))          # one pitch period
    e2 = np.concatenate([[0.0], np.cumsum(x * x)])
    starts = np.arange(0, len(x) - win)
    env = np.sqrt((e2[starts + win] - e2[starts]) / win)
    centre_ms = (starts + win / 2) * 1000.0 / sr
    # The level crosses half its local value when the one-period window is
    # centred on the event.
    on_idx = int(np.argmax(env > 0.5 * env.max()))
    onset_ms = float(centre_ms[on_idx])
    level = np.median(env[on_idx:on_idx + int(0.03 * sr)])
    above = np.where(env > 0.5 * min(level, env.max()))[0]
    gate_ms = float(centre_ms[above[-1]]) if len(above) else centre_ms[-1]
    # Onset is measured from the start of the VCA attack; the model needs
    # the note-on time, which is a little earlier.
    return max(-5.0, onset_ms - 1.5), gate_ms - onset_ms + 1.5


# ---------------------------------------------------------------------------
# CMA-ES (Hansen's (mu/mu_w, lambda) CMA-ES, compact form)
# ---------------------------------------------------------------------------

class CMAES:
    def __init__(self, x0, sigma, popsize=None, rng=None):
        self.n = n = len(x0)
        self.rng = rng or np.random.default_rng()
        self.lam = popsize or 4 + int(3 * math.log(n))
        self.mu = self.lam // 2
        w = np.log(self.mu + 0.5) - np.log(np.arange(1, self.mu + 1))
        self.w = w / w.sum()
        self.mueff = 1.0 / np.sum(self.w ** 2)
        self.cc = (4 + self.mueff / n) / (n + 4 + 2 * self.mueff / n)
        self.cs = (self.mueff + 2) / (n + self.mueff + 5)
        self.c1 = 2 / ((n + 1.3) ** 2 + self.mueff)
        self.cmu = min(1 - self.c1, 2 * (self.mueff - 2 + 1 / self.mueff) / ((n + 2) ** 2 + self.mueff))
        self.damps = 1 + 2 * max(0, math.sqrt((self.mueff - 1) / (n + 1)) - 1) + self.cs
        self.chiN = math.sqrt(n) * (1 - 1 / (4 * n) + 1 / (21 * n * n))
        self.m = np.array(x0, dtype=float)
        self.sigma = sigma
        self.pc = np.zeros(n)
        self.ps = np.zeros(n)
        self.B = np.eye(n)
        self.D = np.ones(n)
        self.C = np.eye(n)
        self.invsqrtC = np.eye(n)
        self.evals = 0
        self.eigen_evals = 0

    def ask(self):
        z = self.rng.standard_normal((self.lam, self.n))
        return self.m + self.sigma * (z * self.D) @ self.B.T

    def tell(self, X, f):
        X = np.asarray(X)
        self.evals += len(f)
        idx = np.argsort(f)
        xold = self.m
        sel = X[idx[:self.mu]]
        self.m = self.w @ sel
        y = (self.m - xold) / self.sigma
        self.ps = (1 - self.cs) * self.ps + math.sqrt(self.cs * (2 - self.cs) * self.mueff) * (self.invsqrtC @ y)
        hsig = (np.linalg.norm(self.ps) / math.sqrt(1 - (1 - self.cs) ** (2 * self.evals / self.lam))
                / self.chiN) < 1.4 + 2 / (self.n + 1)
        self.pc = (1 - self.cc) * self.pc + hsig * math.sqrt(self.cc * (2 - self.cc) * self.mueff) * y
        art = (sel - xold) / self.sigma
        self.C = ((1 - self.c1 - self.cmu) * self.C
                  + self.c1 * (np.outer(self.pc, self.pc) + (1 - hsig) * self.cc * (2 - self.cc) * self.C)
                  + self.cmu * (art.T * self.w) @ art)
        self.sigma *= math.exp((self.cs / self.damps) * (np.linalg.norm(self.ps) / self.chiN - 1))
        self.sigma = min(self.sigma, 1.0)
        if self.evals - self.eigen_evals > self.lam / (self.c1 + self.cmu) / self.n / 10:
            self.eigen_evals = self.evals
            self.C = np.triu(self.C) + np.triu(self.C, 1).T
            d2, self.B = np.linalg.eigh(self.C)
            self.D = np.sqrt(np.maximum(d2, 1e-20))
            self.invsqrtC = self.B @ np.diag(1 / self.D) @ self.B.T

    def condition(self):
        return (self.D.max() / self.D.min()) ** 2


def optimize(problem, u0, seconds, patience, sigma0, seed, label="fit", popsize=None, log_every=15.0,
             batch_eval=None):
    batch_eval = batch_eval or (lambda U: problem.evaluate_batch(U)[0])
    rng = np.random.default_rng(seed)
    best_u = np.array(u0, dtype=float)
    best_f = batch_eval([best_u])[0]
    start = last_improve = last_log = time.time()
    es = CMAES(best_u, sigma0, popsize, rng)
    history = [(0.0, best_f)]
    restarts, gens, evals = 0, 0, 1
    while True:
        now = time.time()
        if now - start > seconds:
            reason = "time cap reached"
            break
        if now - last_improve > patience:
            reason = f"no progress for {patience:.0f} s"
            break
        X = es.ask()
        F = batch_eval([np.asarray(x) for x in X])
        es.tell(X, F)
        gens += 1
        evals += len(F)
        i = int(np.argmin(F))
        if F[i] < best_f - max(1e-4, 1e-4 * abs(best_f)):
            last_improve = time.time()
        if F[i] < best_f:
            best_f, best_u = F[i], np.clip(X[i], 0, 1)
            history.append((time.time() - start, best_f))
        if es.sigma < 1e-3 or es.condition() > 1e12:
            restarts += 1
            lam = min(es.lam * 2, 64)
            es = CMAES(best_u, sigma0 * 0.5, lam, rng)
        if time.time() - last_log > log_every:
            last_log = time.time()
            print(f"  [{label}] {time.time() - start:6.0f}s gen {gens:5d} evals {evals:6d} "
                  f"best {best_f:8.4f} sigma {es.sigma:.4f} restarts {restarts}", flush=True)
    return best_u, best_f, {"reason": reason, "generations": gens, "evaluations": evals,
                            "restarts": restarts, "seconds": time.time() - start, "history": history}


# ---------------------------------------------------------------------------
# Reporting
# ---------------------------------------------------------------------------

def sample_stats(problem, ref, sf, gain):
    cost, comp = problem.sample_cost(ref, sf, gain)
    if sf is None:
        return {"cost": cost, "failed": True}
    spec = ref.spec
    rh, sh = ref.feat["harm"], sf["harm"] + gain
    fl = rh.max() - FLOORS["harm"]
    valid = rh > fl
    e = np.maximum(sh, fl) - np.maximum(rh, fl)
    ev = e[valid]
    order = np.argsort(-np.abs(e) * valid)
    worst = [{"k": int(k + 1), "hz": round(float(spec.harm_freqs[k]), 1),
              "ref_db": round(float(rh[k] - rh.max()), 1), "sim_db": round(float(sh[k] - rh.max()), 1),
              "err_db": round(float(e[k]), 1)} for k in order[:6]]
    # Content the model adds that the hardware doesn't have, above 4 kHz.
    ri, si = ref.feat["inter"], sf["inter"] + gain
    fl_i = rh.max() - FLOORS["inter"]
    ie = np.maximum(si, fl_i) - np.maximum(ri, fl_i)
    hf = spec.harm_freqs >= 4000
    excess = []
    for k in np.argsort(-(e * hf))[:3]:
        if e[k] > 3 and hf[k]:
            excess.append({"type": "harmonic", "hz": round(float(spec.harm_freqs[k]), 1), "excess_db": round(float(e[k]), 1)})
    for k in np.argsort(-(ie * hf[:-1]))[:3]:
        if ie[k] > 3 and hf[k]:
            excess.append({"type": "inter-harmonic", "hz": round(float(spec.harm_freqs[k] + spec.f0 / 2), 1),
                           "excess_db": round(float(ie[k]), 1)})
    return {
        "cost": cost,
        "components": comp,
        "harm_within_1db": float(np.mean(np.abs(ev) <= 1)) * 100,
        "harm_within_3db": float(np.mean(np.abs(ev) <= 3)) * 100,
        "harm_within_6db": float(np.mean(np.abs(ev) <= 6)) * 100,
        "harm_mean_abs_db": float(np.mean(np.abs(ev))),
        "harm_mean_abs_db_first10": float(np.mean(np.abs(e[:10]))),
        "worst_harmonics": worst,
        "spurious_hf": sorted(excess, key=lambda d: -d["excess_db"])[:4],
    }


def summarize(problem, u, gain=None):
    costs, det = problem.evaluate_batch([u], gain)
    d = det[0]
    per = [sample_stats(problem, ref, sf, d["gain_db"]) for ref, sf in zip(problem.refs, d["feats"])]
    agg = {k: float(np.mean([p[k] for p in per if not p.get("failed")]))
           for k in ("cost", "harm_within_1db", "harm_within_3db", "harm_within_6db", "harm_mean_abs_db")}
    for comp in ("harm", "inter", "env", "stft"):
        agg[comp] = float(np.mean([p["components"][comp] for p in per if not p.get("failed")]))
    agg["objective"] = costs[0]
    agg["gain_db"] = d["gain_db"]
    return {"aggregate": agg, "samples": per, "feats": d["feats"]}


def sensitivity(problem, u, f0, step=0.05):
    U = []
    for j in range(len(u)):
        for s in (-step, step):
            v = np.array(u)
            v[j] = min(max(v[j] + s, 0), 1)
            U.append(v)
    costs, _ = problem.evaluate_batch(U)
    out = {}
    for j, p in enumerate(problem.params):
        out[p.key] = float(max(costs[2 * j], costs[2 * j + 1]) - f0)
    return out


def refit_labels(problem, u_best, gain, seconds, seed):
    """With the global model fixed, let each sample choose its own knob
    positions over the full 0..1 range. A sample whose best free fit sits far
    from its labeled positions (and fits much better there) probably has
    wrong knob information in its file name."""
    base, knobs, timing = problem.decode(u_best)
    out = {}
    names = list(KNOBS.values())
    for ri, ref in enumerate(problem.refs):
        start_pos = np.array([knobs[(k, ref.digits[k])] for k in names])

        def batch(U, ref=ref):
            def job(v):
                ov = {k: float(min(max(x, 0), 1)) for k, x in zip(names, v)}
                vals = problem.sample_values(base, knobs, ref, ov)
                y, ok = problem.render_ref(vals, ref, timing)
                if not ok or not np.all(np.isfinite(y)):
                    return 1e3
                c, _ = problem.sample_cost(ref, compute_features(y, ref.spec), gain)
                return c + float(np.sum((np.asarray(v) - np.clip(v, 0, 1)) ** 2)) * 100
            return list(problem.pool.map(job, U))

        f_label = batch([start_pos])[0]
        # A few coarse restarts so a wrong label far away can be found.
        best_u, best_f = start_pos, f_label
        per_try = seconds / 3.0
        for t, init in enumerate([start_pos, np.full(5, 0.5), 1 - start_pos]):
            u, f, _ = optimize(problem, np.clip(init, 0, 1), per_try, per_try, 0.25, seed + ri * 7 + t,
                               label=f"refit {ref.name}", popsize=10, log_every=1e9, batch_eval=batch)
            if f < best_f:
                best_u, best_f = u, f
        out[ref.name] = {
            "labeled_positions": {k: round(float(p), 3) for k, p in zip(names, start_pos)},
            "free_fit_positions": {k: round(float(p), 3) for k, p in zip(names, best_u)},
            "cost_at_label": f_label,
            "cost_free": best_f,
            "improvement_pct": 100 * (f_label - best_f) / max(f_label, 1e-9),
            "max_knob_move": float(np.max(np.abs(best_u - start_pos))),
        }
    return out


def duplicate_labels(refs, tol_db=1.5):
    """Pairs of references on the same note/waveform whose labels differ but
    whose full-note harmonic spectra are nearly identical: at least one of
    the two labels is probably wrong (the knob wasn't actually moved)."""
    out = []
    for i in range(len(refs)):
        for j in range(i + 1, len(refs)):
            a, b = refs[i], refs[j]
            if a.midi != b.midi or a.waveform != b.waveform or a.accent_step != b.accent_step:
                continue
            diff = [k for k in KNOBS.values() if a.digits[k] != b.digits[k]]
            if not diff:
                continue
            ha, hb = a.feat["harm"] - a.feat["harm"].max(), b.feat["harm"] - b.feat["harm"].max()
            k = min(len(ha), len(hb))
            m = (ha[:k] > -60) | (hb[:k] > -60)
            d = weighted_rms((ha[:k] - hb[:k])[m], a.spec.harm_w[:k][m])
            if d < tol_db:
                out.append({"a": a.name, "b": b.name, "knobs": diff, "spectral_distance_db": d})
    return out


def absorbed_cutoff_law(base_vals, idx, knobs):
    """Re-express fitted cutoff knob positions as a new cutoff law so the
    plugin's knob 0/0.5/1 lands where the hardware's min/half/max do."""
    p0, p1 = knobs.get(("cutoff", 0)), knobs.get(("cutoff", 1))
    if p0 is None or p1 is None:
        return None
    e = base_vals[idx["cutoffTaperExp"]]
    span = base_vals[idx["cutoffSpanOct"]]
    basehz = base_vals[idx["cutoffBaseHz"]]
    a0, a1 = p0 ** e, p1 ** e
    new = {"cutoffBaseHz": basehz * 2 ** (span * a0), "cutoffSpanOct": span * (a1 - a0), "cutoffTaperExp": e}
    p5 = knobs.get(("cutoff", 5))
    if p5 is not None and a1 > a0:
        ratio = (p5 ** e - a0) / (a1 - a0)
        if 0 < ratio < 1:
            exp_mid = math.log(ratio) / math.log(0.5)
            # A degenerate mid-point (e.g. a mislabeled sample) would bend
            # the law into nonsense; keep the fitted taper then.
            if 0.3 <= exp_mid <= 4.0:
                new["cutoffTaperExp"] = exp_mid
    return new


def fmt_float(v):
    if v == 0:
        return "0.0f"
    s = f"{v:.6g}"
    if "e" in s:
        s = f"{v:.6f}".rstrip("0")
    if "." not in s:
        s += ".0"
    return s + "f"


def apply_to_header(path, values):
    text = path.read_text()
    changed = []
    for name, v in values.items():
        pat = re.compile(r"(float\s+%s\{)[^}]*(\};)" % re.escape(name))
        text, n = pat.subn(lambda m: m.group(1) + fmt_float(v) + m.group(2), text)
        if n:
            changed.append(name)
    path.write_text(text)
    return changed


def fmt_table(rows, headers):
    w = [max(len(str(h)), *(len(str(r[i])) for r in rows)) for i, h in enumerate(headers)]
    line = "| " + " | ".join(str(h).ljust(w[i]) for i, h in enumerate(headers)) + " |"
    sep = "|" + "|".join("-" * (x + 2) for x in w) + "|"
    body = ["| " + " | ".join(str(r[i]).ljust(w[i]) for i in range(len(w))) + " |" for r in rows]
    return "\n".join([line, sep] + body)


def maybe_plot(problem, before, after, out_dir):
    try:
        import matplotlib
        matplotlib.use("Agg")
        import matplotlib.pyplot as plt
    except ImportError:
        return False
    for ri, ref in enumerate(problem.refs):
        fig, ax = plt.subplots(2, 1, figsize=(11, 7))
        spec = ref.spec
        top = ref.feat["harm"].max()
        ax[0].plot(spec.harm_freqs, ref.feat["harm"] - top, "k.-", lw=1, label="hardware")
        for lab, res, col in (("before", before, "tab:red"), ("after", after, "tab:blue")):
            sf = res["feats"][ri]
            if sf is not None:
                ax[0].plot(spec.harm_freqs, sf["harm"] + res["aggregate"]["gain_db"] - top, ".-", color=col,
                           lw=0.8, ms=3, label=lab)
        ax[0].set_xscale("log")
        ax[0].set_ylim(-90, 5)
        ax[0].set_ylabel("harmonic level (dB)")
        ax[0].set_title(ref.name)
        ax[0].legend()
        t = np.arange(len(ref.feat["env"])) * spec.env_hop / ref.sr * 1000
        ax[1].plot(t, ref.feat["env"], "k", lw=1, label="hardware")
        for lab, res, col in (("before", before, "tab:red"), ("after", after, "tab:blue")):
            sf = res["feats"][ri]
            if sf is not None:
                ax[1].plot(t, sf["env"] + res["aggregate"]["gain_db"], color=col, lw=0.8, label=lab)
        ax[1].set_xlabel("ms")
        ax[1].set_ylabel("RMS (dB)")
        ax[1].set_ylim(ref.feat["env"].max() - 60, ref.feat["env"].max() + 6)
        fig.tight_layout()
        fig.savefig(out_dir / f"{ref.name}.png", dpi=90)
        plt.close(fig)
    return True


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--refs", default=str(REPO / "test" / "resources"), help="reference sample folder")
    ap.add_argument("--build-dir", default=str(REPO / "build"))
    ap.add_argument("--rebuild", action="store_true", help="rebuild the render library first")
    ap.add_argument("--out", default=None, help="output folder (default calibration_results/<timestamp>)")
    ap.add_argument("--max-minutes", type=float, default=20.0, help="time cap for the main fit")
    ap.add_argument("--patience-minutes", type=float, default=4.0, help="stop after this long without progress")
    ap.add_argument("--refit-seconds", type=float, default=30.0,
                    help="per-sample budget for the wrong-label check (0 disables)")
    ap.add_argument("--sigma", type=float, default=0.12, help="initial CMA-ES step (unit-cube scale)")
    ap.add_argument("--seed", type=int, default=1)
    ap.add_argument("--workers", type=int, default=os.cpu_count() or 4)
    ap.add_argument("--fmax", type=float, default=16000.0, help="highest frequency compared (Hz)")
    ap.add_argument("--only", default="", help="comma list of groups/params to fit "
                    "(groups: osc, filter, cv, env, knobs, timing)")
    ap.add_argument("--fix", default="", help="comma list of groups/params to keep at their current value")
    ap.add_argument("--exclude", default="",
                    help="comma list of substrings; matching reference files are left out of the fit")
    ap.add_argument("--accent", choices=("knob", "all", "none"), default="knob",
                    help="which samples were recorded on an accented step (default: Accent knob > 0)")
    ap.add_argument("--knob-prior", type=float, default=0.05,
                    help="cost per (0.1 knob travel)^2 of moving a knob position off nominal")
    ap.add_argument("--w-harm", type=float, default=1.0)
    ap.add_argument("--w-inter", type=float, default=0.25)
    ap.add_argument("--w-env", type=float, default=0.5)
    ap.add_argument("--w-stft", type=float, default=0.5)
    ap.add_argument("--evaluate-only", action="store_true", help="score the current code, no fitting")
    ap.add_argument("--apply", action="store_true", help="write fitted defaults into src/core/SynthEngine.hpp")
    args = ap.parse_args()

    ref_paths = sorted(Path(args.refs).glob("*.wav"))
    excludes = [e for e in args.exclude.split(",") if e]
    ref_paths = [p for p in ref_paths if not any(e in p.name for e in excludes)]
    refs = []
    for p in ref_paths:
        try:
            refs.append(Reference(p, args.accent))
        except ValueError as e:
            print(f"skipping {p.name}: {e}")
    if not refs:
        sys.exit(f"no usable reference samples in {args.refs}")

    renderer = Renderer(find_library(Path(args.build_dir), args.rebuild))
    t0 = time.time()
    problem = Problem(refs, renderer, args)
    print(f"{len(refs)} reference samples, {len(problem.params)} free parameters, {args.workers} workers")
    for ref in refs:
        print(f"  {ref.name}: f0 {ref.f0:.3f} Hz, tuning {ref.tune_cents:+.1f} c (label {ref.label_tune_cents:+.0f}), "
              f"pitch drift {ref.drift_cents:.2f} c, accent step {ref.accent_step}")

    tt = time.time()
    before = summarize(problem, problem.u0)
    per_eval = time.time() - tt
    print(f"Before: objective {before['aggregate']['objective']:.3f}, harmonics within 3 dB "
          f"{before['aggregate']['harm_within_3db']:.0f}%  ({per_eval * 1000:.0f} ms/eval)")

    if args.evaluate_only:
        u_best, run = problem.u0, {"reason": "evaluate-only", "generations": 0, "evaluations": 0,
                                   "restarts": 0, "seconds": 0.0, "history": []}
    else:
        print(f"Optimizing for up to {args.max_minutes:.1f} min (patience {args.patience_minutes:.1f} min) ...")
        u_best, _, run = optimize(problem, problem.u0, args.max_minutes * 60, args.patience_minutes * 60,
                                  args.sigma, args.seed)
        print(f"Stopped: {run['reason']} after {run['evaluations']} evaluations")
    after = summarize(problem, u_best)
    sens = sensitivity(problem, u_best, after["aggregate"]["objective"])
    labels = {}
    if args.refit_seconds > 0:
        print("Checking reference labels (free per-sample knob fit) ...")
        labels = refit_labels(problem, u_best, after["aggregate"]["gain_db"], args.refit_seconds, args.seed)

    write_outputs(problem, args, before, after, u_best, run, sens, labels, time.time() - t0)


def write_outputs(problem, args, before, after, u_best, run, sens, labels, elapsed):
    stamp = datetime.datetime.now().strftime("%Y%m%d-%H%M%S")
    out_dir = Path(args.out) if args.out else REPO / "calibration_results" / stamp
    (out_dir / "renders").mkdir(parents=True, exist_ok=True)
    base, knobs, timing = problem.decode(u_best)
    base0, knobs0, timing0 = problem.decode(problem.u0)
    idx = problem.r.index

    # Audio for listening.
    for ref in problem.refs:
        write_wav(out_dir / "renders" / f"{ref.name}_hardware.wav", ref.x, ref.sr)
        for lab, (b, k, t, g) in (("before", (base0, knobs0, timing0, before["aggregate"]["gain_db"])),
                                  ("after", (base, knobs, timing, after["aggregate"]["gain_db"]))):
            y, _ = problem.render_ref(problem.sample_values(b, k, ref), ref, t)
            write_wav(out_dir / "renders" / f"{ref.name}_{lab}.wav", y * 10 ** (g / 20), ref.sr)
    plotted = maybe_plot(problem, before, after, out_dir)

    model_values = {p.target: float(base[idx[p.target]]) for p in problem.params if p.kind == "model"}
    absorbed = absorbed_cutoff_law(base, idx, knobs)

    # Outlier analysis.
    costs = np.array([s["cost"] for s in after["samples"]])
    med = float(np.median(costs))
    ranking = sorted(zip([r.name for r in problem.refs], costs), key=lambda t: -t[1])
    suspects = []
    for name, c in ranking:
        lab = labels.get(name)
        why = []
        if c > 1.5 * med and len(costs) > 2:
            why.append(f"error {c:.2f} is {c / med:.1f}x the median")
        if lab and lab["improvement_pct"] > 25 and lab["max_knob_move"] > 0.2:
            moved = [f"{k} {lab['labeled_positions'][k]:.2f}->{lab['free_fit_positions'][k]:.2f}"
                     for k in KNOBS.values()
                     if abs(lab["free_fit_positions"][k] - lab["labeled_positions"][k]) > 0.2]
            why.append(f"free knob fit is {lab['improvement_pct']:.0f}% better with " + ", ".join(moved))
        if why:
            suspects.append((name, why))

    dups = duplicate_labels(problem.refs)
    for d in dups:
        why = (f"spectrum is within {d['spectral_distance_db']:.1f} dB of {d['b']} although the "
               f"{'/'.join(d['knobs'])} label differs -- one of the two labels is probably wrong")
        suspects.insert(0, (d["a"], [why]))

    result = {
        "generated": stamp,
        "elapsed_sec": elapsed,
        "run": {k: v for k, v in run.items()},
        "weights": problem.w,
        "references": [{"name": r.name, "f0_hz": r.f0, "tuning_cents": r.tune_cents,
                        "label_tuning_cents": r.label_tune_cents, "pitch_drift_cents": r.drift_cents,
                        "accent_step": r.accent_step, "digits": r.digits} for r in problem.refs],
        "before": {"aggregate": before["aggregate"], "samples": before["samples"]},
        "after": {"aggregate": after["aggregate"], "samples": after["samples"]},
        "model_parameters": {k: {"before": float(base0[idx[k]]), "after": v, "sensitivity": sens.get(k)}
                             for k, v in model_values.items()},
        "knob_positions": {f"{k}@{d}": {"nominal": digit_to_position(d), "fitted": knobs[(k, d)],
                                        "sensitivity": sens.get(f"{k}@{d}")} for k, d in problem.knob_digits},
        "timing": {k: {"before": timing0[k], "after": timing[k]} for k in TIMING_PARAMS},
        "absorbed_cutoff_law": absorbed,
        "label_check": labels,
        "suspect_samples": suspects,
        "duplicate_labels": dups,
    }
    (out_dir / "result.json").write_text(json.dumps(result, indent=2, default=float))

    # C++ lines.
    hdr_vals = dict(model_values)
    lines = ["// Paste into acidus::SynthParameters (src/core/SynthEngine.hpp), or rerun with --apply.",
             f"// Fitted {stamp} against {len(problem.refs)} hardware reference samples."]
    if absorbed:
        lines.append("// Cutoff law re-expressed so plugin knob 0/0.5/1 = hardware min/half/max:")
        hdr_vals.update(absorbed)
    for k, v in hdr_vals.items():
        lines.append(f"float {k}{{{fmt_float(v)}}};")
    lines.append("")
    lines.append("// Fitted hardware knob positions (plugin knob value at each labeled digit):")
    for k, d in problem.knob_digits:
        lines.append(f"//   {k:10s} digit {d}: {knobs[(k, d)]:.3f}  (nominal {digit_to_position(d):.2f})")
    (out_dir / "synth_parameters.txt").write_text("\n".join(lines) + "\n")

    applied = []
    if args.apply:
        applied = apply_to_header(REPO / "src" / "core" / "SynthEngine.hpp", hdr_vals)

    # Markdown report.
    B, A = before["aggregate"], after["aggregate"]
    md = [f"# Acidus vs. hardware TB-303 calibration report ({stamp})", ""]
    md.append(f"{len(problem.refs)} reference samples, {len(problem.params)} free parameters. "
              f"Search: {run['evaluations']} evaluations in {run['seconds'] / 60:.1f} min, "
              f"{run['restarts']} CMA-ES restarts, stopped because: {run['reason']}.")
    md.append("")
    md.append("## Match quality (mean over samples)")
    md.append("")
    md.append("All errors are dB; lower is better. `harm` = full-note harmonic levels (k^-0.5 weighted RMS), "
              "`inter` = energy between harmonics, `env` = RMS envelope, `stft` = 1/3-octave levels over time.")
    md.append("")
    rows = []
    for key, lab in (("objective", "objective (incl. priors)"), ("cost", "weighted error"),
                     ("harm", "harmonic error (dB)"), ("inter", "inter-harmonic error (dB)"),
                     ("env", "envelope error (dB)"), ("stft", "spectrogram error (dB)"),
                     ("harm_mean_abs_db", "mean |harmonic error| (dB)"),
                     ("harm_within_1db", "harmonics within 1 dB (%)"),
                     ("harm_within_3db", "harmonics within 3 dB (%)"),
                     ("harm_within_6db", "harmonics within 6 dB (%)")):
        rows.append([lab, f"{B[key]:.2f}", f"{A[key]:.2f}"])
    md.append(fmt_table(rows, ["metric", "before", "after"]))
    md.append("")
    md.append(f"Recording gain solved as {A['gain_db']:+.1f} dB (before: {B['gain_db']:+.1f} dB).")
    md.append("")
    md.append("## Per sample (sorted by remaining error, worst first)")
    md.append("")
    rows = []
    for name, c in ranking:
        i = [r.name for r in problem.refs].index(name)
        sb, sa, ref = before["samples"][i], after["samples"][i], problem.refs[i]
        rows.append([name, f"{sb['cost']:.2f}", f"{sa['cost']:.2f}",
                     f"{sa['components']['harm']:.1f}", f"{sa['components']['inter']:.1f}",
                     f"{sa['components']['env']:.1f}", f"{sa['components']['stft']:.1f}",
                     f"{sa['harm_within_3db']:.0f}%", f"{ref.tune_cents:+.1f}", f"{ref.drift_cents:.2f}"])
    md.append(fmt_table(rows, ["sample", "before", "after", "harm", "inter", "env", "stft",
                               "harm ±3dB", "tuning (c)", "drift (c)"]))
    md.append("")
    md.append("`tuning` is measured from the harmonic peaks relative to equal temperament; `drift` is the "
              "std-dev of short-time pitch over the note (hardware fluctuation).")
    md.append("")
    md.append("### Worst remaining harmonics / spurious high-frequency content")
    md.append("")
    for i, ref in enumerate(problem.refs):
        sa = after["samples"][i]
        w = ", ".join(f"k{h['k']} {h['hz']:.0f}Hz ref {h['ref_db']} / sim {h['sim_db']}" for h in sa["worst_harmonics"][:4])
        sp = ", ".join(f"{s['type']} {s['hz']:.0f}Hz +{s['excess_db']}dB" for s in sa["spurious_hf"]) or "none > 3 dB"
        md.append(f"- **{ref.name}** worst: {w}. Model excess above 4 kHz: {sp}.")
    md.append("")
    md.append("## Suspicious samples")
    md.append("")
    if suspects:
        for name, why in suspects:
            md.append(f"- **{name}**: " + "; ".join(why))
    else:
        md.append("No sample stands out: remaining errors are similar across samples and no free knob fit "
                  "moved far from the labeled positions.")
    if labels:
        md.append("")
        rows = []
        for name, lab in labels.items():
            rows.append([name, f"{lab['cost_at_label']:.2f}", f"{lab['cost_free']:.2f}",
                         f"{lab['improvement_pct']:.0f}%",
                         " ".join(f"{k[0]}={v:.2f}" for k, v in lab["free_fit_positions"].items())])
        md.append(fmt_table(rows, ["sample", "error at label", "error free knobs", "gain", "best free knobs"]))
    md.append("")
    md.append("## Fitted knob positions")
    md.append("")
    rows = [[f"{k} ({'min' if d == 0 else 'max' if d == 1 else d})", f"{digit_to_position(d):.2f}",
             f"{knobs[(k, d)]:.3f}", f"{sens.get(f'{k}@{d}', 0):.3f}"] for k, d in problem.knob_digits]
    md.append(fmt_table(rows, ["knob (digit)", "nominal", "fitted", "sensitivity"]))
    if absorbed:
        md.append("")
        md.append("Cutoff law with the fitted end stops folded in (so the plugin's knob range equals the "
                  "hardware's): " + ", ".join(f"`{k} = {v:.4g}`" for k, v in absorbed.items()) + ".")
    md.append("")
    md.append("## Fitted model parameters")
    md.append("")
    md.append("`sensitivity` = objective increase for a 5% (of range) nudge. Near-zero means the current "
              "references don't constrain that parameter -- its value is not evidence of anything.")
    md.append("")
    rows = []
    for k, v in model_values.items():
        s = sens.get(k, 0.0)
        rows.append([k, f"{base0[idx[k]]:.5g}", f"{v:.5g}", f"{s:.3f}",
                     "unconstrained" if s < 0.005 else ""])
    md.append(fmt_table(rows, ["parameter", "before", "after", "sensitivity", ""]))
    md.append("")
    md.append(f"Timing: note-on offset {timing['onsetMs']:.2f} ms, gate length {timing['gateMs']:.1f} ms.")
    md.append("")
    md.append("## Applying the result")
    md.append("")
    if applied:
        md.append(f"`--apply` wrote {len(applied)} defaults into `src/core/SynthEngine.hpp`.")
    else:
        md.append("Paste `synth_parameters.txt` into `acidus::SynthParameters` (src/core/SynthEngine.hpp) "
                  "or rerun with `--apply`.")
    md.append("")
    md.append(f"Audio: `renders/` (hardware / before / after per sample)."
              + (" Plots: `<sample>.png`." if plotted else ""))
    (out_dir / "report.md").write_text("\n".join(md) + "\n")
    print("\n".join(md))
    print(f"\nWrote {out_dir}")


if __name__ == "__main__":
    main()
