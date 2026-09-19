#!/usr/bin/env python3
"""Spectral diagnostic tool for comparing Acidus renders against a reference
(Open303, another Acidus render, or eventually a real TB-303 recording).

No third-party dependencies (stdlib only), so it runs anywhere this repo's
DSP code can be built. It answers the two questions that matter most when a
render "doesn't sound like a TB-303":

  1. Is the spectral energy distribution (bass vs. mid vs. high) in the right
     ballpark, and does the strongest spectral peak actually track the
     played note's pitch (as it should), or sit at a fixed, input-independent
     frequency (a strong sign of filter self-oscillation/instability)?
  2. How does that compare to a reference render at matching knob settings?

Usage:
    python3 tools/analyze_audio.py render.wav
    python3 tools/analyze_audio.py ours.wav reference.wav
    python3 tools/analyze_audio.py ours.wav --times 0.05,0.3,0.6

Supports 16-bit and 24-bit PCM mono or stereo WAV (stereo is downmixed to
mono by averaging channels).
"""
import sys
import struct
import math
import cmath
import argparse


def read_wav(path):
    """Reads a PCM WAV file (16 or 24-bit, mono or stereo) into a list of
    floats in [-1, 1], downmixing to mono if needed. Returns (samples, sample_rate)."""
    with open(path, 'rb') as f:
        data = f.read()
    if data[0:4] != b'RIFF' or data[8:12] != b'WAVE':
        raise ValueError(f"{path}: not a RIFF/WAVE file")
    pos = 12
    channels = bits = sr = None
    pcm_data = None
    while pos < len(data):
        chunk_id = data[pos:pos + 4]
        chunk_size = struct.unpack('<I', data[pos + 4:pos + 8])[0]
        body = data[pos + 8:pos + 8 + chunk_size]
        if chunk_id == b'fmt ':
            audio_format, channels, sr, byte_rate, block_align, bits = struct.unpack('<HHIIHH', body[:16])
        elif chunk_id == b'data':
            pcm_data = body
        pos += 8 + chunk_size + (chunk_size & 1)
    if pcm_data is None or sr is None:
        raise ValueError(f"{path}: missing fmt/data chunk")

    if bits == 16:
        n_frames = len(pcm_data) // (2 * channels)
        raw = struct.unpack(f'<{n_frames * channels}h', pcm_data[:n_frames * channels * 2])
        scale = 32768.0
    elif bits == 24:
        n_frames = len(pcm_data) // (3 * channels)
        raw = [0] * (n_frames * channels)
        for i in range(n_frames * channels):
            b0, b1, b2 = pcm_data[3 * i], pcm_data[3 * i + 1], pcm_data[3 * i + 2]
            val = b0 | (b1 << 8) | (b2 << 16)
            if val & 0x800000:
                val -= 0x1000000
            raw[i] = val
        scale = 8388608.0
    else:
        raise ValueError(f"{path}: unsupported bit depth {bits} (only 16/24-bit PCM supported)")

    if channels == 1:
        samples = [v / scale for v in raw]
    else:
        samples = [
            sum(raw[i * channels + c] for c in range(channels)) / channels / scale
            for i in range(n_frames)
        ]
    return samples, sr


def next_pow2(n):
    p = 1
    while p < n:
        p *= 2
    return p


def fft(a):
    n = len(a)
    if n <= 1:
        return a
    even = fft(a[0::2])
    odd = fft(a[1::2])
    twiddle = [cmath.exp(-2j * math.pi * k / n) * odd[k] for k in range(n // 2)]
    return [even[k] + twiddle[k] for k in range(n // 2)] + [even[k] - twiddle[k] for k in range(n // 2)]


def spectrum(segment, sr):
    """Hann-windowed magnitude spectrum of a signal segment."""
    n = next_pow2(len(segment))
    N = len(segment)
    windowed = [
        s * (0.5 - 0.5 * math.cos(2 * math.pi * i / (N - 1)) if N > 1 else 1.0)
        for i, s in enumerate(segment)
    ]
    windowed += [0.0] * (n - len(windowed))
    X = fft([complex(s, 0.0) for s in windowed])
    mags = [abs(v) for v in X[:n // 2]]
    freqs = [i * sr / n for i in range(n // 2)]
    return freqs, mags


def band_energy(freqs, mags, lo, hi):
    return math.sqrt(sum(m * m for f, m in zip(freqs, mags) if lo <= f < hi))


def find_peak(freqs, mags, lo, hi):
    best_f, best_m = 0.0, -1.0
    for f, m in zip(freqs, mags):
        if lo <= f < hi and m > best_m:
            best_m, best_f = m, f
    return best_f, best_m


BANDS = [(20, 150), (150, 400), (400, 800), (800, 1500), (1500, 3000), (3000, 8000), (8000, 15000)]


def analyze_segment(x, sr, t, fft_len=4096, fundamental_lo=30, fundamental_hi=1200):
    """Returns a dict of diagnostics for a 4096-sample window starting at time t."""
    start = int(t * sr)
    seg_len = min(fft_len, len(x) - start)
    if start < 0 or seg_len < 512:
        return None
    seg = x[start:start + seg_len]
    seg_rms = math.sqrt(sum(s * s for s in seg) / len(seg))
    if seg_rms < 1e-5:
        return {'t': t, 'silent': True}
    freqs, mags = spectrum(seg, sr)
    total = math.sqrt(sum(m * m for m in mags)) or 1e-12
    overall_peak_f, overall_peak_m = find_peak(freqs, mags, 20, 15000)
    fundamental_f, fundamental_m = find_peak(freqs, mags, fundamental_lo, fundamental_hi)
    bands = {f"{lo}-{hi}": band_energy(freqs, mags, lo, hi) / total for lo, hi in BANDS}
    return {
        't': t, 'silent': False, 'rms': seg_rms,
        'overall_peak_hz': overall_peak_f, 'overall_peak_mag': overall_peak_m,
        'fundamental_hz': fundamental_f, 'fundamental_mag': fundamental_m,
        'bands': bands,
    }


def print_report(path, times):
    x, sr = read_wav(path)
    dur = len(x) / sr
    peak = max(abs(s) for s in x)
    rms = math.sqrt(sum(s * s for s in x) / len(x))
    print(f"\n=== {path} ===")
    print(f"duration={dur:.3f}s  sample_rate={sr}  peak={peak:.4f}  rms={rms:.4f}")
    results = []
    for t in times:
        r = analyze_segment(x, sr, t)
        if r is None:
            continue
        results.append(r)
        if r.get('silent'):
            print(f"  t={t:.3f}s  silent")
            continue
        band_str = "  ".join(f"{k}={100*v:.1f}%" for k, v in r['bands'].items())
        flag = ""
        if abs(r['overall_peak_hz'] - r['fundamental_hz']) > 20 and r['overall_peak_mag'] > r['fundamental_mag'] * 1.5:
            flag = "  <-- overall peak is NOT the fundamental (possible self-oscillation/artifact)"
        print(f"  t={t:.3f}s  rms={r['rms']:.4f}  fundamental={r['fundamental_hz']:.1f}Hz(m={r['fundamental_mag']:.2f})  "
              f"overall_peak={r['overall_peak_hz']:.1f}Hz(m={r['overall_peak_mag']:.2f}){flag}")
        print(f"           bands: {band_str}")

    nonsilent = [r for r in results if not r.get('silent')]
    if len(nonsilent) >= 3:
        peaks = [r['overall_peak_hz'] for r in nonsilent]
        if max(peaks) - min(peaks) < 5.0 and abs(peaks[0] - nonsilent[0]['fundamental_hz']) > 20:
            print(f"  ** WARNING: overall spectral peak stays fixed at ~{peaks[0]:.0f}Hz across all "
                  f"sampled times regardless of note/envelope -- this is the signature of filter "
                  f"self-oscillation, not normal filtered-oscillator behavior. **")
    return results


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('files', nargs='+', help="One WAV file to analyze, or two to compare (yours, reference).")
    ap.add_argument('--times', default="0.03,0.1,0.2,0.3,0.4,0.5,0.6,0.7,0.8,0.9",
                     help="Comma-separated times (seconds) to sample. Default covers the first ~1s.")
    args = ap.parse_args()
    times = [float(t) for t in args.times.split(',')]

    if len(args.files) == 1:
        print_report(args.files[0], times)
    elif len(args.files) == 2:
        r1 = print_report(args.files[0], times)
        r2 = print_report(args.files[1], times)
        print(f"\n=== Comparison: {args.files[0]} vs {args.files[1]} ===")
        n = min(len(r1), len(r2))
        for a, b in zip(r1[:n], r2[:n]):
            if a.get('silent') or b.get('silent'):
                continue
            print(f"  t={a['t']:.3f}s  bass% {100*a['bands']['20-150']:.1f} vs {100*b['bands']['20-150']:.1f}   "
                  f"mid(800-3k)% {100*(a['bands']['800-1500']+a['bands']['1500-3000']):.1f} vs "
                  f"{100*(b['bands']['800-1500']+b['bands']['1500-3000']):.1f}")
    else:
        ap.error("pass 1 file to analyze, or 2 files to compare")


if __name__ == '__main__':
    main()
