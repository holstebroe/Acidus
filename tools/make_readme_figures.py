#!/usr/bin/env python3
"""Render the README's hardware-vs-Acidus figures.

Every figure compares a dinsync.info x0x reference note with the same note
rendered through the plugin's own SynthEngine (the x0x profile, the note's
chart knob positions, the measured tuning and gate), under one recording
gain for all notes -- the same comparison the calibrator scores.

    python3 tools/make_readme_figures.py                # writes docs/images/
    python3 tools/make_readme_figures.py --out /tmp/fig --gain-db -8.4

Outputs:
  sweep_tracks.png   resonant-peak frequency over time for four squelchy notes
  squelch_spectrum.gif  slow-motion spectrum of one accented squelch
  squelch_waveform.png  the first cycles of that squelch, overlaid
"""

import argparse
import json
import sys
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))
from calibrate_reference import (DEFAULT_MANIFEST, REPO, Renderer, find_library,  # noqa: E402
                                 load_manifest, midi_to_hz)
from sweep_track import track  # noqa: E402

# Validated categorical pair (dataviz reference palette, light surface).
HARDWARE = "#2a78d6"
ACIDUS = "#eb6834"
SURFACE = "#fcfcfb"
INK = "#0b0b0b"
INK_2 = "#52514e"
GRID = "#e4e3df"

SWEEP_NOTES = ["E2-p3-saw-acc", "D3-p4-saw-acc", "C2-p3-saw-acc", "D1-p2-saw-acc"]
SQUELCH_NOTE = "E2-p3-saw-acc"


def knob_label(ref):
    names = (("cutoff", "Cutoff"), ("resonance", "Res"), ("envMod", "Env Mod"), ("decay", "Decay"), ("accent", "Accent"))
    parts = [f"{n} {int(round(ref.nominal[k] * 100))}" for k, n in names]
    return " · ".join(parts) + (" · accented" if ref.accent_step else "")


def render_pair(r, ref, gain_db):
    """Hardware clip and the Acidus render, aligned, Acidus scaled by the global gain."""
    on = int(round(ref.timing_hint[0] * ref.sr / 1000.0))
    v = r.defaults.copy()
    for k, val in ref.nominal.items():
        v[r.index[k]] = val
    v[r.index["tuningCents"]] = ref.fixed_tune_cents
    y, _ = r.render(v, ref.waveform, ref.midi, ref.accent_step, ref.sr,
                    int(round(ref.timing_hint[1] * ref.sr / 1000.0)), ref.n - on)
    y = np.concatenate([np.zeros(on), y])[:ref.n] * 10 ** (gain_db / 20.0)
    return ref.x, y, on


def save_png(fig, path, width_px):
    """Supersampled PNG: draw at 3x the final 2x-density size, Lanczos down."""
    from PIL import Image
    w_in = fig.get_figwidth()
    fig.savefig(path, dpi=3 * 2 * width_px / w_in / 2, facecolor=SURFACE)
    img = Image.open(path).convert("RGB")
    h = round(img.height * (2 * width_px) / img.width)
    img.resize((2 * width_px, h), Image.LANCZOS).save(path, optimize=True)


def style(ax):
    ax.set_facecolor(SURFACE)
    for s in ("top", "right"):
        ax.spines[s].set_visible(False)
    for s in ("left", "bottom"):
        ax.spines[s].set_color(GRID)
    ax.tick_params(colors=INK_2, labelsize=8)
    ax.grid(True, which="major", color=GRID, lw=0.6)
    ax.xaxis.label.set_color(INK_2)
    ax.yaxis.label.set_color(INK_2)


def sweep_figure(r, refs, gain_db, path):
    import matplotlib.pyplot as plt
    from matplotlib.ticker import FuncFormatter, NullFormatter
    fig, axs = plt.subplots(2, 2, figsize=(9.6, 5.6), facecolor=SURFACE)
    for ax, nid in zip(axs.flat, SWEEP_NOTES):
        ref = refs[nid]
        x, y, on = render_pair(r, ref, gain_db)
        f0 = midi_to_hz(ref.midi) * 2 ** (ref.fixed_tune_cents / 1200.0)
        th, ta = track(x, ref.sr, on, f0), track(y, ref.sr, on, f0)
        # The first analysis window straddles note-on, where the tracker has no
        # stable peak yet: start the plot at 20 ms.
        th, ta = th[th[:, 0] >= 20], ta[ta[:, 0] >= 20]
        style(ax)
        ax.semilogy(th[:, 0], th[:, 1], color=HARDWARE, lw=2.0, solid_capstyle="round")
        ax.semilogy(ta[:, 0], ta[:, 1], color=ACIDUS, lw=2.0, solid_capstyle="round", dashes=(4, 2))
        ax.set_xlim(0, 500)
        lo = min(th[th[:, 0] <= 500, 1].min(), ta[ta[:, 0] <= 500, 1].min()) * 0.85
        hi = max(th[th[:, 0] <= 500, 1].max(), ta[ta[:, 0] <= 500, 1].max()) * 1.15
        ax.set_ylim(lo, hi)
        ticks = [t for t in (100, 200, 300, 500, 1000, 2000, 3000, 5000, 10000, 20000) if lo <= t <= hi]
        ax.set_yticks(ticks)
        ax.yaxis.set_minor_formatter(NullFormatter())
        ax.yaxis.set_major_formatter(FuncFormatter(lambda v, _: f"{v / 1000:g}k" if v >= 1000 else f"{v:g}"))
        ax.set_title(f"{nid}\n{knob_label(ref)}", fontsize=8.5, color=INK, loc="left")
        ax.set_xlabel("ms after note-on", fontsize=8)
        ax.set_ylabel("resonant peak (Hz)", fontsize=8)
    handles = [plt.Line2D([], [], color=HARDWARE, lw=2.0), plt.Line2D([], [], color=ACIDUS, lw=2.0, dashes=(4, 2))]
    fig.suptitle("Where the resonance is, every 5 ms: hardware vs Acidus", y=0.985, fontsize=11, color=INK)
    fig.legend(handles, ["TB-303 (hardware recording)", "Acidus"], loc="upper center", ncol=2, frameon=False,
               fontsize=9, labelcolor=INK, bbox_to_anchor=(0.5, 0.95))
    fig.tight_layout(rect=(0, 0, 1, 0.92))
    save_png(fig, path, 960)
    plt.close(fig)


def spectrum_frames(x, sr, on, t_ms, win_ms=30.0):
    n = int(sr * win_ms / 1000.0)
    w = np.hanning(n)
    i = on + int(sr * t_ms / 1000.0)
    seg = x[i:i + n]
    if len(seg) < n:
        seg = np.pad(seg, (0, n - len(seg)))
    nfft = 8192
    s = np.abs(np.fft.rfft(seg * w, nfft)) / (w.sum() / 2)
    f = np.fft.rfftfreq(nfft, 1 / sr)
    # 1/6-octave smoothing on a log grid: shows the filter's shape and the
    # resonant peak rather than individual harmonics (which a 30 ms window
    # cannot resolve at C2 anyway).
    grid = np.geomspace(50.0, 16000.0, 400)
    p = s ** 2
    df = f[1]
    sm = np.array([p[(f >= min(g * 2 ** (-1 / 12), g - df)) & (f <= max(g * 2 ** (1 / 12), g + df))].mean()
                   for g in grid])
    return grid, 10 * np.log10(sm + 1e-18)


def squelch_gif(r, refs, gain_db, path, t_end_ms=600, step_ms=10, slow=10):
    import matplotlib.pyplot as plt
    ref = refs[SQUELCH_NOTE]
    x, y, on = render_pair(r, ref, gain_db)
    f0 = midi_to_hz(ref.midi) * 2 ** (ref.fixed_tune_cents / 1200.0)
    times = np.arange(0, t_end_ms, step_ms)
    fig, ax = plt.subplots(figsize=(6.4, 3.4), facecolor=SURFACE)
    style(ax)
    ax.set_xscale("log")
    ax.set_xlim(50, 12000)
    ax.set_ylim(-90, -10)
    from matplotlib.ticker import FuncFormatter, NullFormatter
    ax.set_xticks([100, 200, 500, 1000, 2000, 5000, 10000])
    ax.xaxis.set_minor_formatter(NullFormatter())
    ax.xaxis.set_major_formatter(FuncFormatter(lambda v, _: f"{v / 1000:g}k" if v >= 1000 else f"{v:g}"))
    ax.set_xlabel("frequency (Hz)", fontsize=8)
    ax.set_ylabel("level (dB)", fontsize=8)
    lh, = ax.plot([], [], color=HARDWARE, lw=3.0, solid_capstyle="round")
    la, = ax.plot([], [], color=ACIDUS, lw=1.6, solid_capstyle="round")
    ax.legend([lh, la], ["TB-303 (hardware recording)", "Acidus"], loc="upper right", frameon=False, fontsize=8,
              labelcolor=INK)
    ax.set_title(f"{SQUELCH_NOTE}: {knob_label(ref)}", fontsize=8.5, color=INK, loc="left")
    clock = ax.text(0.02, 0.06, "", transform=ax.transAxes, fontsize=9, color=INK_2)

    def frame(k):
        t = times[k]
        f, sh = spectrum_frames(x, ref.sr, on, t)
        _, sa = spectrum_frames(y, ref.sr, on, t)
        lh.set_data(f, sh)
        la.set_data(f, sa)
        clock.set_text(f"{t:3.0f} ms after note-on   ·   {slow}x slow motion")

    # Supersample every frame (draw at 3x, Lanczos down to 1x) so the lines stay
    # smooth after the GIF's 256-colour quantisation, which would otherwise
    # stair-step the anti-aliased edges.
    from PIL import Image
    fig.tight_layout()
    out_w, out_h = int(fig.get_figwidth() * 100), int(fig.get_figheight() * 100)
    fig.set_dpi(300)
    frames = []
    for k in range(len(times)):
        frame(k)
        fig.canvas.draw()
        img = Image.frombuffer("RGBA", fig.canvas.get_width_height(), fig.canvas.buffer_rgba()).convert("RGB")
        img = img.resize((out_w, out_h), Image.LANCZOS)
        frames.append(img.quantize(colors=256, method=Image.MEDIANCUT, dither=Image.Dither.NONE))
    frames[0].save(path, save_all=True, append_images=frames[1:], duration=step_ms * slow, loop=0,
                   optimize=True, disposal=1)
    plt.close(fig)


def waveform_figure(r, refs, gain_db, path, t0_ms=20, cycles=4):
    import matplotlib.pyplot as plt
    ref = refs[SQUELCH_NOTE]
    x, y, on = render_pair(r, ref, gain_db)
    f0 = midi_to_hz(ref.midi) * 2 ** (ref.fixed_tune_cents / 1200.0)
    i0 = on + int(ref.sr * t0_ms / 1000.0)
    n = int(ref.sr * cycles / f0)
    # align the render to the recording by cross-correlation within half a cycle
    span = int(ref.sr / f0 / 2)
    xs = x[i0:i0 + n]
    best = max(range(-span, span + 1), key=lambda d: float(np.dot(xs, y[i0 + d:i0 + d + n])))
    ys = y[i0 + best:i0 + best + n]
    t = np.arange(n) / ref.sr * 1000.0 + t0_ms
    fig, ax = plt.subplots(figsize=(7.2, 2.8), facecolor=SURFACE)
    style(ax)
    ax.plot(t, xs, color=HARDWARE, lw=1.8)
    ax.plot(t, ys, color=ACIDUS, lw=1.5, dashes=(4, 2))
    ax.set_xlabel("ms after note-on", fontsize=8)
    ax.set_ylabel("amplitude", fontsize=8)
    ax.legend(["TB-303 (hardware recording)", "Acidus"], loc="lower right", frameon=False, fontsize=8, labelcolor=INK)
    ax.set_title(f"{SQUELCH_NOTE}: the waveform {t0_ms} ms into the accent", fontsize=9, color=INK, loc="left")
    fig.tight_layout()
    save_png(fig, path, 720)
    plt.close(fig)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--out", default=str(REPO / "docs" / "images"))
    ap.add_argument("--calibration", default=str(REPO / "calibrations" / "x0x.json"))
    ap.add_argument("--gain-db", type=float, default=-8.4,
                    help="global recording gain, as solved by calibrate_reference.py --manifest --evaluate-only")
    ap.add_argument("--build-dir", default=str(REPO / "build"))
    args = ap.parse_args()

    import matplotlib
    matplotlib.use("Agg")

    r = Renderer(find_library(Path(args.build_dir), False))
    for k, v in json.loads(Path(args.calibration).read_text())["parameters"].items():
        r.defaults[r.index[k]] = v
    ids = sorted(set(SWEEP_NOTES + [SQUELCH_NOTE]))
    refs = {ref.name: ref for ref in load_manifest(str(DEFAULT_MANIFEST), ids, [])}

    out = Path(args.out)
    out.mkdir(parents=True, exist_ok=True)
    sweep_figure(r, refs, args.gain_db, out / "sweep_tracks.png")
    waveform_figure(r, refs, args.gain_db, out / "squelch_waveform.png")
    squelch_gif(r, refs, args.gain_db, out / "squelch_spectrum.gif")
    for p in ("sweep_tracks.png", "squelch_waveform.png", "squelch_spectrum.gif"):
        print(f"{out / p}  ({(out / p).stat().st_size / 1024:.0f} KB)")


if __name__ == "__main__":
    main()
