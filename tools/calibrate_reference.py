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
* wave  -- (optional, --w-wave, off by default) phase-aligned waveform
           shape: the render is shifted by a sub-sample lag within +-1/2
           pitch period (the hardware's oscillator phase at note-on is
           arbitrary) to best match the reference over the sustained part,
           then compared in 20 ms frames, each with its own least-squares
           gain, so level is left to `env` and only the normalised PCM shape
           (and how it changes over the note) is scored. Reported as
           10*sqrt(residual energy / reference energy): 0 = identical,
           3.2 = 10 % residual energy, 10 = no better than silence.
The other features are deliberately insensitive to exact waveform phase.
harm / inter / stft are clamped at the recording's own noise floor (the
inter-harmonic level beside each harmonic; the quietest frame of each band):
at low cutoff the hardware's upper bands are hiss, and a model that rolls off
cleanly below it must not be scored as missing harmonics. `inter` therefore
only counts content the model adds on top of the hardware's noise.

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
DEFAULT_MANIFEST = REPO / "test" / "resources" / "x0x-reference" / "x0x_reference_manifest.json"

# ---------------------------------------------------------------------------
# Parameter space
# ---------------------------------------------------------------------------

# name -> (lo, hi, log-scaled, group). Names are SynthParameters fields.
MODEL_PARAMS = {
    # Oscillator
    "oscCouplingHz":            (15.0, 60.0, True, "osc"),     # ref: none in hardware; Open303 44.5 (TB303_REFERENCE.md §11.3)
    "oscSawLpfHz":              (4000.0, 40000.0, True, "osc"),
    "oscSawShape":              (-0.4, 0.4, False, "osc"),
    # Square duty = 0.45 + depth * exp(-f / 180 Hz): 0.62 at C2 with the
    # antto depth 0.25; the x0x set's C2 squares null their 15th harmonic,
    # i.e. duty ~0.533 (depth ~0.12). Level: saw/square ratio (§9).
    "oscSquareDutyDepth":       (0.0, 0.35, False, "osc"),
    "oscSquareLevel":           (0.3, 1.2, True, "osc"),
    # Filter
    # Upper bound extends past the ladder's own self-oscillation ceiling
    # (Filter.hpp's kLadderCriticalGain_ = 17): with the resonance-dependent
    # output-gain stage removed (see Filter.hpp/.cpp, 2026-09), the resonant
    # peak's whole level now has to come from this approaching that ceiling,
    # the same way the real feedback loop does it (TB303_EMULATION_
    # REFERENCE.md Sec59/60) -- a render that actually goes unstable is
    # already rejected by Problem._job's isfinite/amplitude check, so the
    # bound itself doesn't need to pre-guess where that line is.
    "filterFeedbackGain":       (6.0, 30.0, False, "filter"),
    "filterResonanceSkew":      (-6.0, 8.0, False, "filter"),   # < 0: resonance builds late in the travel (x0x fit hit 0.05)
    "resCouplingHz":            (60.0, 160.0, True, "filter"),  # in-loop HP; antto 70-140, Open303 150 (§11.3)
    "filterCapScale1":          (0.2, 4.0, True, "filter"),
    "filterCapScale2":          (0.2, 4.0, True, "filter"),
    "filterCapScale3":          (0.2, 4.0, True, "filter"),
    "filterCapScale4":          (0.2, 4.0, True, "filter"),
    "filterLadderInputScale":   (0.01, 0.4, True, "filter"),
    "filterInputCouplingHz":    (3.0, 60.0, True, "filter"),
    "filterOutputCouplingHz":   (6000.0, 40000.0, True, "filter"),
    "filterPostHpHz":           (5.0, 200.0, True, "filter"),  # also stands in for the VCA-input coupling (10 nF into BA662, tens-hundreds of Hz, §15.4)
    "filterNotchHz":            (2.0, 25.0, True, "filter"),
    "filterNotchBandwidthHz":   (1.0, 15.0, True, "filter"),
    "filterAllpassHz":          (4.0, 50.0, True, "filter"),
    # Knob laws / CV summing
    "cutoffBaseHz":             (50.0, 800.0, True, "cv"),
    "cutoffSpanOct":            (1.5, 7.0, False, "cv"),
    "cutoffTaperExp":           (0.5, 3.5, False, "cv"),
    "envModScaleC0":            (0.3, 1.5, False, "cv"),
    "envModScaleC0Slope":       (2.0, 6.0, False, "cv"),
    "envModScaleC1":            (0.3, 1.5, False, "cv"),
    "envModScaleC1Slope":       (2.0, 6.0, False, "cv"),
    "envModOffset":             (0.1, 0.5, False, "cv"),
    "envModOffsetCutSlope":     (-0.2, 0.3, False, "cv"),
    "accentSweepDepthOct":      (0.0, 9.0, False, "cv"),
    "accentVcaDepth":           (0.0, 6.0, False, "cv"),
    "accentChargeBaseSec":      (0.035, 0.065, True, "cv"),   # R46 x C13 +-30 %
    "accentChargePotSec":       (0.030, 0.070, True, "cv"),   # VR4b x C13
    "accentMixSec":             (0.07, 0.15, True, "cv"),     # R_mix x C13 (100k +-30 %)
    # Envelopes / VCA
    "vcfAttackMs":              (0.02, 1.0, True, "env"),
    "vcaAttackMs":              (0.3, 8.0, True, "env"),     # VCA onset: few ms, R134/C41 2.2 ms (§15.2)
    "vcfDecayMinSec":           (0.055, 0.10, True, "env"),   # tau, R136 x C62 (+-20 % caps)
    "vcfDecayMaxSec":           (0.85, 1.35, True, "env"),    # tau, (R136 + VR6) x C62
    "accentDecaySec":           (0.055, 0.10, True, "env"),
    "vegDecaySec":              (1.0, 6.0, True, "env"),      # R123 x C42 = 1.5 s (§15.1); hardware samples look flatter
    "vcaGateOffMs":             (0.3, 20.0, True, "env"),
    "vcaGateOffAccentMs":       (0.3, 80.0, True, "env"),
    "vcaResTapRatio":           (0.0, 3.0, False, "env"),     # filter->VCA taps, §12: 100k/220k = 0.45 or reversed 2.2
    "vcaGainSaturationDrive":   (0.0, 10.0, False, "env"),   # 0 = linear control law (default)
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
    """One reference note.

    labels:  knob -> label of the hardware knob position it was recorded at
             (the file-name digit, or "x<percent>" for manifest clips). Every
             (knob, label) pair is one fitted knob position shared by all
             notes that use it.
    nominal: knob -> the label's nominal position (0..1).
    fixed_tune_cents: render/analysis tuning given by the source (manifest);
             None = measure it from the note.
    timing_hint: (note-on ms, gate ms) given by the source; None = detect."""

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
        self.labels = {KNOBS[k]: int(m[k]) for k in KNOBS}
        self.nominal = {k: digit_to_position(d) for k, d in self.labels.items()}
        self.fixed_tune_cents = None
        self.timing_hint = None
        self.group = None
        # The file name doesn't say whether the step was accented; the Accent
        # knob only does anything on an accented step, so by default a
        # non-minimum Accent knob implies the step was recorded with accent.
        if accent_mode == "knob":
            self.accent_step = self.labels["accent"] != 0
        else:
            self.accent_step = accent_mode == "all"
        x, sr = read_wav(self.path)
        tail = max(8, len(x) // 100)
        self.x = x - x[-tail:].mean()
        self.sr = sr
        self.n = len(self.x)

    @classmethod
    def from_clip(cls, clip, x, sr, analysis_ms=None):
        """A note from a reference manifest (see tools/x0x_reference_manifest.py):
        the clip window of a longer recording, with knobs, accent, tuning and
        timing given by the manifest."""
        self = cls.__new__(cls)
        self.path = None
        self.name = clip["id"]
        self.waveform = 0 if clip["waveform"] == "saw" else 1
        self.note = clip["note"]
        self.midi = int(clip["midi"])
        self.fixed_tune_cents = float(clip["render_tune_cents"])
        self.label_tune_cents = self.fixed_tune_cents
        pct = clip["knobs_pct"]
        self.labels = {k: f"x{int(pct[k])}" for k in KNOBS.values()}
        self.nominal = {k: float(clip["knobs"][k]) for k in KNOBS.values()}
        self.accent_step = bool(clip["accent"])
        self.group = clip.get("set")
        a, b = int(clip["clip_start_sample"]), int(clip["clip_end_sample"])
        if analysis_ms:
            b = min(b, int(clip["note_on_sample"]) + int(round(analysis_ms * sr / 1000.0)))
        seg = x[a:b]
        # The recordings sit on a clean, DC-free floor: remove the level of
        # the silence before the note, not of the (release) tail.
        pre = seg[: max(8, int(clip["note_on_sample"]) - a)]
        self.x = seg - pre.mean()
        self.sr = sr
        self.n = len(self.x)
        self.timing_hint = ((int(clip["note_on_sample"]) - a) * 1000.0 / sr, float(clip["gate_ms"]))
        return self


def load_manifest(path, includes, excludes, analysis_ms=None, rotate=None):
    """Reference notes from a manifest JSON (tools/x0x_reference_manifest.py).
    includes/excludes are substrings matched against the clip ids, e.g.
    "A1-" (a set), "-p2-" (a position), "square", "-acc".
    rotate: None, or keep one note per (set, position), cycling through the note
    kinds (saw / saw-acc / square / square-acc) from one position and set to
    the next -- a quarter of the notes that still covers every knob setting
    and every note kind evenly (a fast search subset). The value (0-3) is an
    offset that selects one of the four disjoint quarters."""
    path = Path(path)
    man = json.loads(path.read_text(encoding="utf-8"))
    clips = man["clips"]
    if rotate is not None:
        sets = sorted({c["set"] for c in clips})
        clips = [c for c in clips
                 if (c["note_number"] - 1) % 4 == (c["position"] - 1 + sets.index(c["set"]) + int(rotate)) % 4]
    clips = [c for c in clips if not any(e in c["id"] for e in excludes)]
    if includes:
        clips = [c for c in clips if any(e in c["id"] for e in includes)]
    audio, refs = {}, []
    for c in clips:
        if c["file"] not in audio:
            audio[c["file"]] = read_wav(path.parent / c["file"])
        x, sr = audio[c["file"]]
        refs.append(Reference.from_clip(c, x, sr, analysis_ms))
    return refs


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
        "y": x,
        "harm": db(harm * norm),
        "inter": db(inter * norm),
        "env": db(env),
        "stft": db(bands / spec.st_frame ** 2),
    }


FLOORS = {"harm": 80.0, "inter": 80.0, "env": 50.0, "stft": 70.0}


def noise_floors(rf):
    """The recording's own noise floor, per feature bin (dB). Above a few kHz
    at low cutoff the hardware notes are hiss, not harmonics (harmonic and
    inter-harmonic levels agree within ~1 dB), so a model that rolls off
    cleanly below that must not be scored as a harmonic deficit.
      harm:  the reference's inter-harmonic level on either side (same
             bandwidth, f0/2), +3 dB
      inter: the reference's own inter-harmonic level (only content the
             model adds on top of the hardware's noise counts)
      stft:  the quietest frame of each band (the silence around the note),
             +3 dB"""
    if "floors" in rf:
        return rf["floors"]
    inter = rf["inter"]
    nb = np.concatenate([[inter[0]], inter])
    na = np.concatenate([inter, [inter[-1]]])
    harm_fl = 10 * np.log10(0.5 * (10 ** (nb / 10) + 10 ** (na / 10))) + 3.0
    stft_fl = rf["stft"].min(axis=0) + 3.0
    rf["floors"] = {"harm": harm_fl, "inter": inter.copy(), "stft": stft_fl[None, :]}
    return rf["floors"]


def feature_errors(rf, sf, gain_db, spec):
    """Per-feature error vectors (dB), both sides clamped at a floor relative
    to the reference's own maximum so the fit doesn't chase the noise floor,
    and (harm / inter / stft) at the recording's measured noise floor."""
    nf = noise_floors(rf)
    out = {}
    for key in ("harm", "inter", "env", "stft"):
        r = rf[key]
        s = sf[key] + gain_db
        fl = r.max() - FLOORS[key] if key != "inter" else rf["harm"].max() - FLOORS[key]
        if key in nf:
            fl = np.maximum(fl, nf[key])
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


def wave_error(ref, y):
    """Phase-aligned, per-frame-normalised waveform error (see docstring)."""
    x, sr = ref.x, ref.sr
    lo, hi = ref.wave_lo, ref.wave_hi
    if hi - lo < 64:
        return 0.0
    P = sr / ref.f0
    maxlag = int(math.ceil(P / 2)) + 1
    seg = x[lo:hi]
    # Integer lag by direct correlation over +-P/2, then parabolic refinement.
    best, lags = None, np.arange(-maxlag, maxlag + 1)
    c = np.array([np.dot(seg, y[lo - l:hi - l]) if lo - l >= 0 and hi - l <= len(y) else -np.inf for l in lags])
    i = int(np.argmax(c))
    frac = 0.0
    if 0 < i < len(c) - 1 and np.isfinite(c[i - 1]) and np.isfinite(c[i + 1]):
        den = c[i - 1] - 2 * c[i] + c[i + 1]
        if den < 0:
            frac = 0.5 * (c[i - 1] - c[i + 1]) / den
    lag = lags[i] + frac
    # Fractional shift of the render by `lag` samples (FFT phase ramp).
    n = len(y)
    Y = np.fft.rfft(y)
    k = np.fft.rfftfreq(n)
    ys = np.fft.irfft(Y * np.exp(-2j * np.pi * k * lag), n)[lo:hi]
    fl = max(64, int(round(0.020 * sr)))
    num = den = 0.0
    for a in range(0, len(seg) - fl + 1, fl):
        r, m = seg[a:a + fl], ys[a:a + fl]
        mm = float(np.dot(m, m))
        g = float(np.dot(r, m)) / mm if mm > 0 else 0.0
        num += float(np.sum((r - g * m) ** 2))
        den += float(np.dot(r, r))
    return 10.0 * math.sqrt(num / den) if den > 0 else 0.0


def local_bump(h, smooth_half):
    """h minus a locally-smoothed baseline (edge-padded moving average).
    At high cutoff the resonant hump is often a modest *local* bump on an
    already-bright, barely-attenuated spectrum -- the fundamental (or a low
    harmonic) can still be the single loudest bin in absolute dB. A plain
    argmax(h) then locks onto the fundamental and reports a perfect,
    trivial "peak match" while missing the actual resonance entirely.
    Detrending against a wider local average finds the bump regardless of
    the passband's absolute level."""
    n = len(h)
    if n < 2 * smooth_half + 1:
        return h - float(np.mean(h))
    kernel = np.ones(2 * smooth_half + 1) / (2 * smooth_half + 1)
    baseline = np.convolve(np.pad(h, smooth_half, mode="edge"), kernel, mode="valid")
    return h - baseline


def peak_window(feat, spec, has_peak, window_hz=400.0):
    """A window of harmonics around the reference's own resonant peak,
    expressed relative to the fundamental (gain-independent). Comparing a
    *shape* over this fixed window -- not just the single peak bin -- also
    penalizes a peak at the wrong frequency or the wrong width, not only
    the wrong height, without needing separate frequency/bandwidth logic:
    a shifted or narrower/wider model peak shows up as a mismatched curve
    across the same harmonic indices as the reference's peak.
    A Gaussian-ish weight centred on the peak keeps the harmonics right at
    the top of the peak (the part that actually reads as resonance) more
    important than the window's edges."""
    if not has_peak:
        return 0, 0, 0, np.zeros(0), np.zeros(0)
    h = feat["harm"]
    half = max(2, int(round(window_hz / spec.f0)))
    k = int(np.argmax(local_bump(h, smooth_half=3 * half)))
    lo, hi = max(0, k - half), min(len(h), k + half + 1)
    rel = h[lo:hi] - h[0]
    idx = np.arange(lo, hi)
    weight = np.exp(-0.5 * ((idx - k) / max(1.0, half / 2.0)) ** 2)
    return k, lo, hi, rel, weight


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
        self.w = {"harm": args.w_harm, "inter": args.w_inter, "env": args.w_env, "stft": args.w_stft,
                  "peak": args.w_peak, "wave": args.w_wave}
        self.pool = ThreadPoolExecutor(max_workers=args.workers)

        # Per-reference analysis
        for ref in refs:
            if ref.fixed_tune_cents is not None:
                ref.tune_cents = ref.fixed_tune_cents
                ref.f0 = midi_to_hz(ref.midi) * 2 ** (ref.tune_cents / 1200.0)
                ref.drift_cents = 0.0
            else:
                f_guess = midi_to_hz(ref.midi) * 2 ** (ref.label_tune_cents / 1200.0)
                ref.f0 = estimate_f0(ref.x, ref.sr, f_guess)
                ref.tune_cents = 1200 * math.log2(ref.f0 / midi_to_hz(ref.midi))
                ref.drift_cents = pitch_drift_cents(ref.x, ref.sr, ref.f0)
            ref.spec = FeatureSpec(ref, ref.f0, args.fmax, 1 << int(math.ceil(math.log2(ref.n * 4))))
            ref.feat = compute_features(ref.x, ref.spec)
            ref.onset_ms, ref.gate_ms = ref.timing_hint or detect_timing(ref)
            # Waveform window: sustained part, 15 ms after onset to 5 ms
            # before the gate closes (skips the VCA attack and release).
            ref.wave_lo = int((ref.onset_ms + 15.0) * ref.sr / 1000.0)
            ref.wave_hi = int((ref.onset_ms + ref.gate_ms - 5.0) * ref.sr / 1000.0)
            ref.has_peak = ref.nominal["resonance"] >= 0.5
            ref.peak_k, ref.peak_lo, ref.peak_hi, ref.peak_ref_rel, ref.peak_weight = \
                peak_window(ref.feat, ref.spec, has_peak=ref.has_peak)

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
        self.knob_nominal = {}
        for ref in refs:
            for knob in KNOBS.values():
                self.knob_nominal[(knob, ref.labels[knob])] = ref.nominal[knob]
        used = sorted(self.knob_nominal, key=lambda kl: (kl[0], str(kl[1])))
        if not (only and "knobs" not in only) and "knobs" not in fixed:
            for knob, label in used:
                nom = self.knob_nominal[(knob, label)]
                lo, hi = max(0.0, nom - KNOB_PRIOR_SPAN), min(1.0, nom + KNOB_PRIOR_SPAN)
                p = Param(f"{knob}@{label}", lo, hi, False, nom, "knob", "knobs", target=knob, nominal=nom)
                p.label = label
                self.params.append(p)
        self.knob_digits = used
        self.capscale_idx = [i for i, p in enumerate(self.params)
                             if p.kind == "model" and p.target in
                             ("filterCapScale1", "filterCapScale2", "filterCapScale3", "filterCapScale4")]
        onset0 = max(0.0, float(np.median([r.onset_ms for r in refs])))
        # Manifest clips carry their own measured note-on; the fitted onset is
        # then a shift relative to it (a clip-dependent start), not a global
        # position in the file.
        self.relative_onset = all(r.timing_hint is not None for r in refs)
        if self.relative_onset:
            onset0 = 0.0
        gate0 = float(np.median([r.gate_ms for r in refs if not r.accent_step] or [r.gate_ms for r in refs]))
        self.timing_init = {"onsetMs": onset0, "gateMs": gate0}
        if not (only and "timing" not in only) and "timing" not in fixed:
            # Never below 0: a negative onset starts the render mid-attack, and
            # the resulting step at sample 0 adds a broadband -6 dB/oct
            # "harmonic" tail the hardware (trimmed ~1 ms before its onset)
            # doesn't have -- the optimizer will happily use it to fake a
            # faster VCA attack.
            if self.relative_onset:
                # Shift of the note-on against the manifest's (grid) note-on.
                self.params.append(Param("onsetMs", -4.0, 4.0, False, 0.0, "timing", "timing"))
            else:
                self.params.append(Param("onsetMs", 0.0, 8.0, False, max(0.0, onset0), "timing", "timing"))
            self.params.append(Param("gateMs", max(10.0, gate0 - 30), gate0 + 30, False, gate0, "timing", "timing"))
        self.u0 = np.array([p.to_unit(p.init) for p in self.params])

    # -- mapping ------------------------------------------------------------
    def decode(self, u):
        base = self.r.defaults.copy()
        knobs = dict(self.knob_nominal)
        timing = dict(self.timing_init)
        for p, ui in zip(self.params, u):
            v = p.to_real(ui)
            if p.kind == "model":
                base[self.r.index[p.target]] = v
            elif p.kind == "knob":
                knobs[(p.target, p.label)] = v
            else:
                timing[p.key] = v
        return base, knobs, timing

    def sample_values(self, base, knobs, ref, knob_override=None):
        v = base.copy()
        for knob in KNOBS.values():
            pos = knobs[(knob, ref.labels[knob])]
            if knob_override and knob in knob_override:
                pos = knob_override[knob]
            v[self.r.index[knob]] = pos
        v[self.r.index["tuningCents"]] = ref.tune_cents
        return v

    def render_ref(self, values, ref, timing):
        sr = ref.sr
        onset_ms = timing["onsetMs"] + (ref.onset_ms if self.relative_onset else 0.0)
        onset = int(round(onset_ms * sr / 1000.0))
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

    def _job_key(self, u, ref):
        """Everything a render + feature extraction depends on: notes recorded
        with the same settings (e.g. the repeated base setting of each x0x
        set) are rendered once per candidate."""
        base, knobs, timing = self.decode(u)
        v = self.sample_values(base, knobs, ref)
        onset = timing["onsetMs"] + (ref.onset_ms if self.relative_onset else 0.0)
        return (v.tobytes(), ref.waveform, ref.midi, ref.accent_step, round(onset * ref.sr / 1000.0),
                round(timing["gateMs"] * ref.sr / 1000.0), ref.n, ref.sr, ref.f0, ref.spec.nfft)

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
        sim_rel = sf["harm"][ref.peak_lo:ref.peak_hi] - sf["harm"][0] if ref.has_peak else np.zeros(0)
        comp = {
            "harm": weighted_rms(e["harm"], ref.spec.harm_w),
            "inter": weighted_rms(e["inter"], ref.spec.inter_w),
            "env": weighted_rms(e["env"]),
            "stft": weighted_rms(e["stft"]),
            "peak": weighted_rms(sim_rel - ref.peak_ref_rel, ref.peak_weight),
            "wave": wave_error(ref, sf["y"]) if self.w["wave"] > 0 else 0.0,
        }
        cost = sum(self.w[k] * comp[k] for k in comp) / sum(self.w.values())
        return cost, comp

    def prior(self, u):
        pen = 0.0
        for p, ui in zip(self.params, u):
            if p.kind == "knob":
                pen += self.args.knob_prior * ((p.to_real(ui) - p.nominal) / 0.1) ** 2
        # The real ladder's four pole frequencies are documented as *mildly*
        # spread, not formant-like (TB303_RESEARCH_COMPENDIUM.md Sec6). An
        # unconstrained per-stage search can otherwise fit a wildly uneven
        # spread that cancels errors at the sampled harmonics but rings/
        # ripples between them -- exactly the "jagged, not clean" rolloff
        # difference from hardware. Penalize spread around the *shared*
        # scale (so an overall cutoff-law shift stays free) unless the data
        # earns it.
        if self.capscale_idx and self.args.capscale_prior > 0:
            logs = np.array([math.log(self.params[i].to_real(u[i])) for i in self.capscale_idx])
            pen += self.args.capscale_prior * float(np.sum((logs - logs.mean()) ** 2))
        return pen

    def evaluate_batch(self, U, gain=None):
        """Returns (costs, details) for a list of unit-space candidates."""
        jobs = [(ci, ri) for ci in range(len(U)) for ri in range(len(self.refs))]
        keys = [self._job_key(U[ci], self.refs[ri]) for ci, ri in jobs]
        first = {}
        for j, k in enumerate(keys):
            first.setdefault(k, j)
        uniq = list(first.values())
        res = dict(zip(uniq, self.pool.map(lambda j: self._job(U[jobs[j][0]], self.refs[jobs[j][1]]), uniq)))
        feats = [res[first[k]] for k in keys]
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
    fl = np.maximum(rh.max() - FLOORS["harm"], noise_floors(ref.feat)["harm"])
    valid = rh > fl
    e = np.maximum(sh, fl) - np.maximum(rh, fl)
    ev = e[valid]
    order = np.argsort(-np.abs(e) * valid)
    worst = [{"k": int(k + 1), "hz": round(float(spec.harm_freqs[k]), 1),
              "ref_db": round(float(rh[k] - rh.max()), 1), "sim_db": round(float(sh[k] - rh.max()), 1),
              "err_db": round(float(e[k]), 1)} for k in order[:6]]
    # Content the model adds that the hardware doesn't have, above 4 kHz.
    ri, si = ref.feat["inter"], sf["inter"] + gain
    fl_i = np.maximum(rh.max() - FLOORS["inter"], noise_floors(ref.feat)["inter"])
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
    peak_info = None
    if ref.has_peak:
        half = max(2, int(round(400.0 / spec.f0)))
        k_sim = int(np.argmax(local_bump(sh, smooth_half=3 * half)))
        peak_info = {
            "ref_freq_hz": round(float(spec.harm_freqs[ref.peak_k]), 1),
            "sim_freq_hz": round(float(spec.harm_freqs[k_sim]), 1),
            "freq_ratio_semitones": round(12 * math.log2(spec.harm_freqs[k_sim] / spec.harm_freqs[ref.peak_k]), 2),
            "ref_height_db": round(float(rh[ref.peak_k] - rh[0]), 1),
            "sim_height_db": round(float(sh[k_sim] - sh[0]), 1),
            "shape_error_db": round(comp["peak"], 2) if comp else None,
        }
    # Overall level of the note (sum of the harmonic energies) vs. hardware:
    # the x0x references are not normalised, so this is meaningful per note.
    level_db = float(10 * np.log10(np.sum(10 ** (sh / 10)) / np.sum(10 ** (rh / 10))))
    return {
        "cost": cost,
        "components": comp,
        "level_db": level_db,
        "harm_within_1db": float(np.mean(np.abs(ev) <= 1)) * 100,
        "harm_within_3db": float(np.mean(np.abs(ev) <= 3)) * 100,
        "harm_within_6db": float(np.mean(np.abs(ev) <= 6)) * 100,
        "harm_mean_abs_db": float(np.mean(np.abs(ev))),
        "harm_mean_abs_db_first10": float(np.mean(np.abs(e[:10]))),
        "worst_harmonics": worst,
        "spurious_hf": sorted(excess, key=lambda d: -d["excess_db"])[:4],
        "peak": peak_info,
    }


def summarize(problem, u, gain=None):
    costs, det = problem.evaluate_batch([u], gain)
    d = det[0]
    per = [sample_stats(problem, ref, sf, d["gain_db"]) for ref, sf in zip(problem.refs, d["feats"])]
    agg = {k: float(np.mean([p[k] for p in per if not p.get("failed")]))
           for k in ("cost", "harm_within_1db", "harm_within_3db", "harm_within_6db", "harm_mean_abs_db")}
    agg["level_rms_db"] = float(np.sqrt(np.mean([p["level_db"] ** 2 for p in per if not p.get("failed")])))
    for comp in ("harm", "inter", "env", "stft", "wave"):
        agg[comp] = float(np.mean([p["components"][comp] for p in per if not p.get("failed")]))
    peak_costs = [p["components"]["peak"] for p, ref in zip(per, problem.refs) if ref.has_peak and not p.get("failed")]
    agg["peak"] = float(np.mean(peak_costs)) if peak_costs else 0.0
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
        start_pos = np.array([knobs[(k, ref.labels[k])] for k in names])

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
            diff = [k for k in KNOBS.values() if a.labels[k] != b.labels[k]]
            if not diff:
                continue
            ha, hb = a.feat["harm"] - a.feat["harm"].max(), b.feat["harm"] - b.feat["harm"].max()
            k = min(len(ha), len(hb))
            m = (ha[:k] > -60) | (hb[:k] > -60)
            d = weighted_rms((ha[:k] - hb[:k])[m], a.spec.harm_w[:k][m])
            if d < tol_db:
                out.append({"a": a.name, "b": b.name, "knobs": diff, "spectral_distance_db": d})
    return out


def absorbed_cutoff_law(base_vals, idx, knobs, nominal):
    """Re-express fitted cutoff knob positions as a new cutoff law so the
    plugin's knob 0/0.5/1 lands where the hardware's min/half/max do."""
    at = {nominal[kl]: v for kl, v in knobs.items() if kl[0] == "cutoff"}
    p0, p1 = at.get(0.0), at.get(1.0)
    if p0 is None or p1 is None:
        return None
    e = base_vals[idx["cutoffTaperExp"]]
    span = base_vals[idx["cutoffSpanOct"]]
    basehz = base_vals[idx["cutoffBaseHz"]]
    a0, a1 = p0 ** e, p1 ** e
    new = {"cutoffBaseHz": basehz * 2 ** (span * a0), "cutoffSpanOct": span * (a1 - a0), "cutoffTaperExp": e}
    p5 = at.get(0.5)
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


def maybe_plot(problem, before, after, out_dir, shown=None):
    try:
        import matplotlib
        matplotlib.use("Agg")
        import matplotlib.pyplot as plt
    except ImportError:
        return False
    for ri, ref in enumerate(problem.refs):
        if shown is not None and ref.name not in shown:
            continue
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
    ap.add_argument("--manifest", nargs="?", const=str(DEFAULT_MANIFEST), default=None,
                    help="use the notes of a reference manifest instead of --refs (default with no value: "
                    "the dinsync.info x0x set, test/resources/x0x-reference/x0x_reference_manifest.json); "
                    "--include/--exclude then match clip ids such as 'A1-p3-saw-acc'")
    ap.add_argument("--analysis-ms", type=float, default=None,
                    help="manifest only: compare just the first N ms after note-on (faster search; "
                    "anything after the window, e.g. the gate-off release, is then unconstrained)")
    ap.add_argument("--rotate", nargs="?", type=int, const=0, default=None,
                    help="manifest only: one note per set/position, rotating through the four note kinds "
                    "(a quarter of the notes; fast search subset). An optional offset 0-3 picks one of "
                    "the four disjoint quarters")
    ap.add_argument("--max-renders", type=int, default=40,
                    help="write audio/plots for this many samples (the worst-fitting ones)")
    ap.add_argument("--no-sensitivity", action="store_true",
                    help="skip the per-parameter sensitivity scan (2 evaluations per free parameter)")
    ap.add_argument("--build-dir", default=str(REPO / "build"))
    ap.add_argument("--calibration", default=None,
                    help="start from a calibration profile (calibrations/<name>.json) instead of the "
                    "compiled-in SynthParameters defaults")
    ap.add_argument("--rebuild", action="store_true", help="rebuild the render library first")
    ap.add_argument("--out", default=None, help="output folder (default calibration_results/<timestamp>)")
    ap.add_argument("--max-minutes", type=float, default=20.0, help="time cap for the main fit")
    ap.add_argument("--patience-minutes", type=float, default=4.0, help="stop after this long without progress")
    ap.add_argument("--refit-seconds", type=float, default=None,
                    help="per-sample budget for the wrong-label check (0 disables; default 30, "
                    "0 with --manifest)")
    ap.add_argument("--sigma", type=float, default=0.12, help="initial CMA-ES step (unit-cube scale)")
    ap.add_argument("--seed", type=int, default=1)
    ap.add_argument("--workers", type=int, default=os.cpu_count() or 4)
    ap.add_argument("--fmax", type=float, default=16000.0, help="highest frequency compared (Hz)")
    ap.add_argument("--only", default="", help="comma list of groups/params to fit "
                    "(groups: osc, filter, cv, env, knobs, timing)")
    ap.add_argument("--fix", default="", help="comma list of groups/params to keep at their current value")
    ap.add_argument("--include", default="",
                    help="comma list of substrings; if given, only matching reference files are used")
    ap.add_argument("--exclude", default="",
                    help="comma list of substrings; matching reference files are left out of the fit")
    ap.add_argument("--accent", choices=("knob", "all", "none"), default="knob",
                    help="which samples were recorded on an accented step (default: Accent knob > 0)")
    ap.add_argument("--knob-prior", type=float, default=0.05,
                    help="cost per (0.1 knob travel)^2 of moving a knob position off nominal")
    ap.add_argument("--capscale-prior", type=float, default=0.05,
                    help="cost per unit of log-variance across the 4 ladder pole scales; "
                    "discourages formant-like spread the data doesn't clearly require (0 disables)")
    ap.add_argument("--w-harm", type=float, default=1.0)
    ap.add_argument("--w-inter", type=float, default=0.25)
    ap.add_argument("--w-env", type=float, default=0.5)
    ap.add_argument("--w-stft", type=float, default=0.5)
    ap.add_argument("--w-peak", type=float, default=2.0,
                    help="weight on the resonant-peak shape (height+width+implicit frequency) at "
                    "Resonance=max samples; deliberately > w-harm so a broadband harmonic fit can't "
                    "trade the peak away (see 2026-09 peak-vs-broadband tradeoff)")
    ap.add_argument("--w-wave", type=float, default=0.0,
                    help="weight on the phase-aligned, per-frame-normalised waveform comparison (0 = off)")
    ap.add_argument("--evaluate-only", action="store_true", help="score the current code, no fitting")
    ap.add_argument("--apply", action="store_true", help="write fitted defaults into src/core/SynthEngine.hpp")
    args = ap.parse_args()

    excludes = [e for e in args.exclude.split(",") if e]
    includes = [e for e in args.include.split(",") if e]
    if args.refit_seconds is None:
        args.refit_seconds = 0.0 if args.manifest else 30.0
    refs = []
    if args.manifest:
        refs = load_manifest(args.manifest, includes, excludes, args.analysis_ms, args.rotate)
    else:
        ref_paths = sorted(Path(args.refs).glob("*.wav"))
        ref_paths = [p for p in ref_paths if not any(e in p.name for e in excludes)]
        if includes:
            ref_paths = [p for p in ref_paths if any(e in p.name for e in includes)]
        for p in ref_paths:
            try:
                refs.append(Reference(p, args.accent))
            except ValueError as e:
                print(f"skipping {p.name}: {e}")
    if not refs:
        sys.exit(f"no usable reference samples in {args.manifest or args.refs}")

    renderer = Renderer(find_library(Path(args.build_dir), args.rebuild))
    if args.calibration:
        # Start from a per-source profile (tools/calibration_profile.py)
        # instead of the compiled-in SynthParameters defaults.
        prof = json.loads(Path(args.calibration).read_text(encoding="utf-8"))
        unknown = [k for k in prof["parameters"] if k not in renderer.index]
        if unknown:
            sys.exit(f"{args.calibration}: unknown parameters {', '.join(unknown)} (rebuild the render library?)")
        for k, v in prof["parameters"].items():
            renderer.defaults[renderer.index[k]] = v
    t0 = time.time()
    problem = Problem(refs, renderer, args)
    print(f"{len(refs)} reference samples, {len(problem.params)} free parameters, {args.workers} workers")
    for ref in (refs if len(refs) <= 40 else []):
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
    sens = {} if args.no_sensitivity else sensitivity(problem, u_best, after["aggregate"]["objective"])
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

    model_values = {p.target: float(base[idx[p.target]]) for p in problem.params if p.kind == "model"}
    absorbed = absorbed_cutoff_law(base, idx, knobs, problem.knob_nominal)

    # Outlier analysis.
    costs = np.array([s["cost"] for s in after["samples"]])
    med = float(np.median(costs))
    ranking = sorted(zip([r.name for r in problem.refs], costs), key=lambda t: -t[1])

    # Audio / plots for listening: the worst --max-renders samples.
    shown = {name for name, _ in ranking[:args.max_renders]}
    for ref in problem.refs:
        if ref.name not in shown:
            continue
        write_wav(out_dir / "renders" / f"{ref.name}_hardware.wav", ref.x, ref.sr)
        for lab, (b, k, t, g) in (("before", (base0, knobs0, timing0, before["aggregate"]["gain_db"])),
                                  ("after", (base, knobs, timing, after["aggregate"]["gain_db"]))):
            y, _ = problem.render_ref(problem.sample_values(b, k, ref), ref, t)
            write_wav(out_dir / "renders" / f"{ref.name}_{lab}.wav", y * 10 ** (g / 20), ref.sr)
    plotted = maybe_plot(problem, before, after, out_dir, shown)
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
                        "accent_step": r.accent_step, "knob_labels": r.labels, "set": r.group}
                       for r in problem.refs],
        "before": {"aggregate": before["aggregate"], "samples": before["samples"]},
        "after": {"aggregate": after["aggregate"], "samples": after["samples"]},
        "model_parameters": {k: {"before": float(base0[idx[k]]), "after": v, "sensitivity": sens.get(k)}
                             for k, v in model_values.items()},
        "knob_positions": {f"{k}@{d}": {"nominal": problem.knob_nominal[(k, d)], "fitted": knobs[(k, d)],
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
    lines.append("// Fitted hardware knob positions (plugin knob value at each labeled position):")
    for k, d in problem.knob_digits:
        lines.append(f"//   {k:10s} {str(d):5s}: {knobs[(k, d)]:.3f}  (nominal {problem.knob_nominal[(k, d)]:.2f})")
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
              "`inter` = energy between harmonics, `env` = RMS envelope, `stft` = 1/3-octave levels over time, "
              "`peak` = shape (height+width+implicit frequency) of the resonant-peak window on "
              "Resonance=max samples only -- weighted more heavily than `harm` so it can't be traded away.")
    md.append("")
    rows = []
    for key, lab in (("objective", "objective (incl. priors)"), ("cost", "weighted error"),
                     ("harm", "harmonic error (dB)"), ("peak", "resonant-peak shape error (dB)"),
                     ("inter", "inter-harmonic error (dB)"),
                     ("env", "envelope error (dB)"), ("stft", "spectrogram error (dB)"),
                     ("wave", "aligned waveform error (10*sqrt(residual/ref))"),
                     ("harm_mean_abs_db", "mean |harmonic error| (dB)"),
                     ("level_rms_db", "note level error, RMS over notes (dB)"),
                     ("harm_within_1db", "harmonics within 1 dB (%)"),
                     ("harm_within_3db", "harmonics within 3 dB (%)"),
                     ("harm_within_6db", "harmonics within 6 dB (%)")):
        rows.append([lab, f"{B[key]:.2f}", f"{A[key]:.2f}"])
    md.append(fmt_table(rows, ["metric", "before", "after"]))
    md.append("")
    md.append(f"Recording gain solved as {A['gain_db']:+.1f} dB (before: {B['gain_db']:+.1f} dB).")
    md.append("")
    groups = sorted({r.group for r in problem.refs if r.group})
    if groups:
        md.append("## Per set")
        md.append("")
        md.append("Mean error of each set's notes, and the RMS of their level errors (the "
                  "recordings are not normalised, so absolute note levels are compared under one "
                  "global gain).")
        md.append("")
        rows = []
        for g in groups:
            ii = [i for i, r in enumerate(problem.refs) if r.group == g]
            sb = [before["samples"][i] for i in ii]
            sa = [after["samples"][i] for i in ii]
            ok = lambda ss: [x for x in ss if not x.get("failed")]
            rows.append([g, len(ii), f"{np.mean([x['cost'] for x in sb]):.2f}",
                         f"{np.mean([x['cost'] for x in sa]):.2f}",
                         f"{np.sqrt(np.mean([x['level_db'] ** 2 for x in ok(sb)])):.1f}",
                         f"{np.sqrt(np.mean([x['level_db'] ** 2 for x in ok(sa)])):.1f}",
                         f"{np.mean([x['components']['harm'] for x in ok(sa)]):.1f}",
                         f"{np.mean([x['components']['env'] for x in ok(sa)]):.1f}",
                         f"{np.mean([x['components']['stft'] for x in ok(sa)]):.1f}"])
        md.append(fmt_table(rows, ["set", "notes", "before", "after", "level before (dB)",
                                   "level after (dB)", "harm", "env", "stft"]))
        md.append("")
    md.append("## Per sample (sorted by remaining error, worst first)")
    md.append("")
    rows = []
    for name, c in ranking:
        i = [r.name for r in problem.refs].index(name)
        sb, sa, ref = before["samples"][i], after["samples"][i], problem.refs[i]
        rows.append([name, f"{sb['cost']:.2f}", f"{sa['cost']:.2f}",
                     f"{sa['components']['harm']:.1f}",
                     f"{sa['components']['peak']:.1f}" if ref.has_peak else "-",
                     f"{sa['components']['inter']:.1f}",
                     f"{sa['components']['env']:.1f}", f"{sa['components']['stft']:.1f}",
                     f"{sa['components']['wave']:.1f}",
                     f"{sa['harm_within_3db']:.0f}%", f"{sb['level_db']:+.1f}", f"{sa['level_db']:+.1f}",
                     f"{ref.tune_cents:+.1f}", f"{ref.drift_cents:.2f}"])
    md.append(fmt_table(rows, ["sample", "before", "after", "harm", "peak", "inter", "env", "stft", "wave",
                               "harm ±3dB", "level before", "level after", "tuning (c)", "drift (c)"]))
    md.append("")
    md.append("### Resonant peak (Resonance=max samples)")
    md.append("")
    peak_rows = []
    for ref in problem.refs:
        if not ref.has_peak:
            continue
        i = [r.name for r in problem.refs].index(ref.name)
        pb, pa = before["samples"][i].get("peak"), after["samples"][i].get("peak")
        if pb is None or pa is None:
            continue
        peak_rows.append([ref.name, f"{pb['ref_freq_hz']:.0f}", f"{pb['sim_freq_hz']:.0f}", f"{pa['sim_freq_hz']:.0f}",
                          f"{pb['ref_height_db']:+.1f}", f"{pb['sim_height_db']:+.1f}", f"{pa['sim_height_db']:+.1f}"])
    md.append(fmt_table(peak_rows, ["sample", "peak Hz (hw)", "peak Hz (before)", "peak Hz (after)",
                                    "height dB (hw)", "height dB (before)", "height dB (after)"]))
    md.append("")
    md.append("Height is the resonant peak's level above the fundamental (gain-independent). This is the "
              "number that answers \"is the acid squelch there\": before/after collapsing to something much "
              "smaller than hardware's means the peak got flattened away, not just shifted.")
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
    rows = [[f"{k} ({d})", f"{problem.knob_nominal[(k, d)]:.2f}",
             f"{knobs[(k, d)]:.3f}", f"{sens.get(f'{k}@{d}', 0):.3f}"] for k, d in problem.knob_digits]
    md.append(fmt_table(rows, ["knob (label)", "nominal", "fitted", "sensitivity"]))
    if absorbed:
        md.append("")
        md.append("Cutoff law with the fitted end stops folded in (so the plugin's knob range equals the "
                  "hardware's): " + ", ".join(f"`{k} = {v:.4g}`" for k, v in absorbed.items()) + ".")
    md.append("")
    md.append("## Fitted model parameters")
    md.append("")
    md.append("`sensitivity` = objective increase for a 5% (of range) nudge. Near-zero means the current "
              "references don't constrain that parameter -- its value is not evidence of anything. "
              "`AT BOUND` means the optimizer pushed it to the edge of its plausible range, usually a sign "
              "that it is compensating for something the model does not implement.")
    md.append("")
    rows = []
    for k, v in model_values.items():
        s = sens.get(k, 0.0)
        par = next(p for p in problem.params if p.target == k and p.kind == "model")
        u = par.to_unit(v)
        note = "unconstrained" if sens and s < 0.005 else ""
        if u < 0.02 or u > 0.98:
            note = (note + " " if note else "") + "AT BOUND (model may lack structure)"
        rows.append([k, f"{base0[idx[k]]:.5g}", f"{v:.5g}", f"{s:.3f}", note])
    md.append(fmt_table(rows, ["parameter", "before", "after", "sensitivity", ""]))
    md.append("")
    md.append(f"Timing: note-on offset {timing['onsetMs']:.2f} ms"
              + (" (relative to each clip's measured note-on)" if problem.relative_onset else "")
              + f", gate length {timing['gateMs']:.1f} ms.")
    md.append("")
    md.append("## Applying the result")
    md.append("")
    if applied:
        md.append(f"`--apply` wrote {len(applied)} defaults into `src/core/SynthEngine.hpp`.")
    else:
        md.append("Paste `synth_parameters.txt` into `acidus::SynthParameters` (src/core/SynthEngine.hpp) "
                  "or rerun with `--apply`.")
    md.append("")
    md.append(f"Audio: `renders/` (hardware / before / after for the {len(shown)} worst samples)."
              + (" Plots: `<sample>.png`." if plotted else ""))
    (out_dir / "report.md").write_text("\n".join(md) + "\n")
    # The per-sample sections of a large (manifest) run are too long for a console.
    cut = next((i for i, l in enumerate(md) if l.startswith("## Per sample")), len(md))
    print("\n".join(md if len(problem.refs) <= 40 else md[:cut]))
    print(f"\nWrote {out_dir}")


if __name__ == "__main__":
    main()
