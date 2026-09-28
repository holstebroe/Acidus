#!/usr/bin/env python3
"""Track the resonant peak over time: hardware reference vs. Acidus.

For each x0x reference note (ids as in test/resources/x0x-reference/
x0x_reference_manifest.json, e.g. E3-p1-saw, E3-p1-saw-acc) this prints the
resonant-peak frequency every few ms after note-on, for the hardware and
for Acidus rendered with the same knobs, and optionally plots both tracks.

The peak is found per frame (Hann window, default 30 ms): the saw's
-6 dB/oct tilt is removed and the spectrum smoothed over one harmonic
spacing, so the maximum above 150 Hz is the resonance, not a harmonic. The
settled value of an accented note (the MEG and the accent sweep decay
within ~0.5 s) is the cutoff with no envelope: the sweep's floor.

Usage:
    python3 tools/sweep_track.py E3-p1-saw E3-p1-saw-acc
    python3 tools/sweep_track.py E3-p1 --calibration calibrations/acidvoice.json
    python3 tools/sweep_track.py E3-p1-saw --set envModScaleC1Slope=2.0 --set cutoffBaseHz=250
    python3 tools/sweep_track.py E3-p --plot sweep.png            # all E3 positions

An id without a note kind (E3-p1) selects all notes of that position;
"E3-p" selects all positions of E3.
"""

import argparse
import json
import sys
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))
from calibrate_reference import (DEFAULT_MANIFEST, REPO, Renderer, find_library,  # noqa: E402
                                 load_manifest, midi_to_hz)

TIMES_MS = [0, 10, 20, 30, 50, 80, 120, 200, 300, 500, 800, 1200]


def track(x, sr, on, f0, win_ms=30.0, hop_ms=5.0, t_end=1.3, fmin=150.0):
    """-> array of (ms after note-on, peak Hz, peak height dB)."""
    w, hop, nfft = int(win_ms * sr / 1000), int(hop_ms * sr / 1000), 1 << 14
    win = np.hanning(w)
    f = np.fft.rfftfreq(nfft, 1 / sr)
    tilt = 20 * np.log10(np.maximum(f, 1) / f0)
    sm = max(3, int(round(f0 / (sr / nfft))))
    ker = np.ones(sm) / sm
    lo, hi = np.searchsorted(f, fmin), np.searchsorted(f, min(16000, 0.45 * sr))
    b0, b1 = np.searchsorted(f, 80), np.searchsorted(f, 400)
    out = []
    for a in range(on, on + int(t_end * sr), hop):
        seg = x[a:a + w]
        if len(seg) < w:
            break
        X = np.convolve(20 * np.log10(np.abs(np.fft.rfft(seg * win, nfft)) + 1e-9) + tilt, ker, "same")
        i = lo + int(np.argmax(X[lo:hi]))
        out.append(((a - on + w / 2) * 1000 / sr, f[i], X[i] - np.median(X[b0:b1])))
    return np.array(out)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("ids", nargs="+", help="note ids or prefixes (E3-p1-saw, E3-p1, E3-p)")
    ap.add_argument("--manifest", default=str(DEFAULT_MANIFEST))
    ap.add_argument("--calibration", help="profile JSON to render with (default: compiled-in defaults)")
    ap.add_argument("--set", action="append", default=[], metavar="NAME=VALUE",
                    help="override one SynthParameters constant (repeatable)")
    ap.add_argument("--window-ms", type=float, default=30.0)
    ap.add_argument("--plot", help="write a PNG with both tracks per note")
    ap.add_argument("--build-dir", default=str(REPO / "build"))
    args = ap.parse_args()

    r = Renderer(find_library(Path(args.build_dir), False))
    if args.calibration:
        for k, v in json.loads(Path(args.calibration).read_text())["parameters"].items():
            r.defaults[r.index[k]] = v
    for s in args.set:
        k, v = s.split("=", 1)
        if k not in r.index:
            sys.exit(f"unknown parameter {k}")
        r.defaults[r.index[k]] = float(v)

    man = json.loads(Path(args.manifest).read_text())
    ids = [c["id"] for c in man["clips"]]
    ids = [i for i in ids if any(i == q or i.startswith(q + "-") or (q.endswith("-p") and i.startswith(q))
                                  for q in args.ids)]
    refs = {ref.name: ref for ref in load_manifest(args.manifest, ids, [])}
    if not ids:
        sys.exit("no matching notes")

    print("peak Hz at ms after note-on".ljust(24) + " ".join(f"{t:>6d}" for t in TIMES_MS))
    rows = []
    for nid in ids:
        ref = refs[nid]
        f0 = midi_to_hz(ref.midi) * 2 ** (ref.fixed_tune_cents / 1200.0)
        on = int(round(ref.timing_hint[0] * ref.sr / 1000.0))
        v = r.defaults.copy()
        for k, val in ref.nominal.items():
            v[r.index[k]] = val
        v[r.index["tuningCents"]] = ref.fixed_tune_cents
        y, _ = r.render(v, ref.waveform, ref.midi, ref.accent_step, ref.sr,
                        int(round(ref.timing_hint[1] * ref.sr / 1000.0)), ref.n - on)
        y = np.concatenate([np.zeros(on), y])[:ref.n]
        th = track(ref.x, ref.sr, on, f0, args.window_ms)
        ta = track(y, ref.sr, on, f0, args.window_ms)
        at = lambda tr: [tr[np.argmin(np.abs(tr[:, 0] - (t + args.window_ms / 2))), 1] for t in TIMES_MS]
        knobs = "/".join(str(int(round(ref.nominal[k] * 100))) for k in ("cutoff", "resonance", "envMod", "decay", "accent"))
        print(f"{nid} ({knobs})")
        print("  hardware".ljust(24) + " ".join(f"{x:6.0f}" for x in at(th)))
        print("  acidus".ljust(24) + " ".join(f"{x:6.0f}" for x in at(ta)))
        rows.append((nid, th, ta))

    if args.plot:
        import matplotlib
        matplotlib.use("Agg")
        import matplotlib.pyplot as plt
        n = len(rows)
        cols = 2 if n > 1 else 1
        fig, axs = plt.subplots((n + cols - 1) // cols, cols, figsize=(6.5 * cols, 3.2 * ((n + cols - 1) // cols)),
                                squeeze=False)
        for ax, (nid, th, ta) in zip(axs.flat, rows):
            ax.semilogy(th[:, 0], th[:, 1], "k-", lw=1.5, label="hardware")
            ax.semilogy(ta[:, 0], ta[:, 1], "-", color="tab:blue", lw=1.2, label="Acidus")
            ax.set_title(nid, fontsize=9)
            ax.set_xlabel("ms after note-on")
            ax.set_ylabel("resonant peak (Hz)")
            ax.grid(True, which="both", alpha=0.3)
            ax.legend(fontsize=8)
        for ax in list(axs.flat)[n:]:
            ax.axis("off")
        fig.tight_layout()
        fig.savefig(args.plot, dpi=90)
        print(f"wrote {args.plot}")


if __name__ == "__main__":
    main()
