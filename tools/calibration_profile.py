#!/usr/bin/env python3
"""Per-source calibration profiles for Acidus.

A profile (calibrations/<name>.json) holds every SynthParameters calibration
constant fitted to one hardware reference source, plus where it came from.
The model is the same for every source; only these constants differ (e.g.
the unit's cutoff trim), so switching sources means swapping the defaults
in src/core/SynthEngine.hpp -- or, in tools/calibrate_reference.py, passing
--calibration calibrations/<name>.json to score/fit from a profile without
touching the header.

Usage:
    python3 tools/calibration_profile.py list
    python3 tools/calibration_profile.py show x0x
    python3 tools/calibration_profile.py diff acidvoice x0x
    python3 tools/calibration_profile.py apply acidvoice      # write into SynthEngine.hpp
    python3 tools/calibration_profile.py capture NAME --source "..." [--notes "..."]
        # snapshot the current SynthEngine.hpp defaults as a profile

Front-panel settings (cutoff, resonance, ...) and the master volume/drive/
tuning are not part of a profile.
"""

import argparse
import datetime
import json
import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
PROFILE_DIR = REPO / "calibrations"
HEADER = REPO / "src" / "core" / "SynthEngine.hpp"

# SynthParameters fields that are user controls, not calibration.
NOT_CALIBRATION = {"cutoff", "resonance", "envMod", "decay", "accent", "masterVolume", "drive", "tuningCents"}

FIELD_RE = re.compile(r"^\s*float\s+(\w+)\{([-+0-9.eE]+)f?\};", re.M)


def header_values(text=None):
    text = text if text is not None else HEADER.read_text()
    body = text[text.index("struct SynthParameters"):]
    body = body[: body.index("\n};") + 3]
    return {k: float(v) for k, v in FIELD_RE.findall(body) if k not in NOT_CALIBRATION}


def fmt_float(v):
    if v == 0:
        return "0.0f"
    s = f"{v:.6g}"
    if "e" in s:
        s = f"{v:.6f}".rstrip("0")
    if "." not in s:
        s += ".0"
    return s + "f"


def write_header(values):
    text = HEADER.read_text()
    known = header_values(text)
    missing = sorted(set(values) - set(known))
    if missing:
        sys.exit(f"profile has fields SynthParameters doesn't: {', '.join(missing)}")
    for name, v in values.items():
        pat = re.compile(r"(float\s+%s\{)[^}]*(\};)" % re.escape(name))
        text = pat.sub(lambda m: m.group(1) + fmt_float(v) + m.group(2), text, count=1)
    HEADER.write_text(text)
    unset = sorted(set(known) - set(values))
    return unset


def load(name):
    p = Path(name)
    if not p.suffix:
        p = PROFILE_DIR / f"{name}.json"
    return json.loads(p.read_text(encoding="utf-8")), p


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    sub.add_parser("list")
    s = sub.add_parser("show"); s.add_argument("name")
    s = sub.add_parser("diff"); s.add_argument("a"); s.add_argument("b")
    s = sub.add_parser("apply"); s.add_argument("name")
    s = sub.add_parser("capture")
    s.add_argument("name"); s.add_argument("--source", required=True); s.add_argument("--notes", default="")
    args = ap.parse_args()

    if args.cmd == "list":
        cur = header_values()
        for p in sorted(PROFILE_DIR.glob("*.json")):
            prof = json.loads(p.read_text(encoding="utf-8"))
            active = all(abs(cur.get(k, float("nan")) - v) <= 1e-6 * max(1.0, abs(v))
                         for k, v in prof["parameters"].items())
            print(f"{p.stem:12s} {'(active) ' if active else '         '}{prof.get('source', '')}")
    elif args.cmd == "show":
        prof, _ = load(args.name)
        print(json.dumps({k: v for k, v in prof.items() if k != "parameters"}, indent=1))
        for k, v in prof["parameters"].items():
            print(f"  {k:26s} {v:.6g}")
    elif args.cmd == "diff":
        a, _ = load(args.a)
        b, _ = load(args.b)
        pa, pb = a["parameters"], b["parameters"]
        print(f"{'parameter':26s} {args.a:>12s} {args.b:>12s}")
        for k in sorted(set(pa) | set(pb)):
            va, vb = pa.get(k), pb.get(k)
            if va is None or vb is None or abs(va - vb) > 1e-6 * max(1.0, abs(va)):
                f = lambda v: "-" if v is None else f"{v:.6g}"
                print(f"{k:26s} {f(va):>12s} {f(vb):>12s}")
    elif args.cmd == "apply":
        prof, p = load(args.name)
        unset = write_header(prof["parameters"])
        print(f"applied {p.relative_to(REPO)} to {HEADER.relative_to(REPO)}")
        if unset:
            print("not in the profile (left as is): " + ", ".join(unset))
    elif args.cmd == "capture":
        PROFILE_DIR.mkdir(exist_ok=True)
        prof = {
            "source": args.source,
            "captured": datetime.date.today().isoformat(),
            "notes": args.notes,
            "parameters": header_values(),
        }
        out = PROFILE_DIR / f"{args.name}.json"
        out.write_text(json.dumps(prof, indent=1) + "\n", encoding="utf-8")
        print(f"wrote {out.relative_to(REPO)} ({len(prof['parameters'])} parameters)")


if __name__ == "__main__":
    main()
