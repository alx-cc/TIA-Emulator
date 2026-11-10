#!/usr/bin/env python3
"""Check the output against Eckhard Stolberg's published tables (1997).

1. Frequencies (pass/fail): for every AUDC x AUDF in stolberg_ntsc.csv, the
   measured repeat length must give his listed NTSC frequency. He uses a
   31440 Hz base clock and one decimal, so the check uses that clock and
   allows 0.1 Hz for his rounding.
2. Waveforms (report only): for the modes where he lists the output pattern,
   compare our pattern at AUDF 0 with his, allowing any starting point. He
   worked from Ron Fries' original TIASOUND, so a difference may come from a
   later change to the emulation rather than a bug.

Note: his tables were calculated from Fries' TIASOUND, the code MAME's file
descends from, so they are not independent of the emulation. They confirm
the frequency of every setting from a published source.
"""

import csv
import os
import sys

from check_periods import prime_factors, repeats
from sweep import SETTLE
import tia_model

STOLBERG_RATE = 31440
TOLERANCE_HZ = 0.1

POLY5 = "0010110011111000110111010100001"
DIV31 = "1111111111111000000000000000000"
# AUDC: (pattern, ticks per bit at AUDF 0)
PATTERNS = {
    1: ("001010000111011", 1),
    4: ("01", 1), 5: ("01", 1),
    6: (DIV31, 1), 10: (DIV31, 1),
    7: (POLY5, 1), 9: (POLY5, 1),
    12: ("10", 3), 13: ("10", 3),
    14: (DIV31, 3), 15: (POLY5, 3),
}


def measured_period(data, start, length, audc, audf):
    """The expected period if the segment repeats with it and no shorter one."""
    first = start + SETTLE
    expected = tia_model.expected_period(audc, audf)
    if not repeats(data, first, expected, length - SETTLE - expected):
        return None
    for p in prime_factors(expected):
        if repeats(data, first, expected // p, length - SETTLE - expected // p):
            return None
    return expected


def main():
    if len(sys.argv) != 3:
        sys.exit("usage: check_stolberg.py <out.raw> <sweep.csv>")
    data = open(sys.argv[1], "rb").read()
    segments = {}
    for row in csv.DictReader(open(sys.argv[2])):
        if row["voice"] == "0":
            segments[int(row["audc"]), int(row["audf"])] = (int(row["start"]), int(row["length"]))

    table = os.path.join(os.path.dirname(os.path.abspath(__file__)), "stolberg_ntsc.csv")
    rows = list(csv.DictReader(line for line in open(table) if not line.startswith("#")))

    failures = 0
    for row in rows:
        audc, audf, listed = int(row["audc"]), int(row["audf"]), float(row["ntsc_hz"])
        start, length = segments[audc, audf]
        period = measured_period(data, start, length, audc, audf)
        if period is None:
            failures += 1
            print(f"FAIL AUDC {audc:2} AUDF {audf:2}: no clean period found")
            continue
        ours = STOLBERG_RATE / period
        if abs(ours - listed) > TOLERANCE_HZ:
            failures += 1
            print(f"FAIL AUDC {audc:2} AUDF {audf:2}: {ours:.2f} Hz, Stolberg lists {listed}")
    if failures:
        print(f"FAIL: {failures} of {len(rows)} frequencies differ from Stolberg's tables")
    else:
        print(f"PASS: all {len(rows)} frequencies match Stolberg's tables")

    print("Waveforms at AUDF 0 (report only):")
    for audc, (pattern, step) in sorted(PATTERNS.items()):
        start, _ = segments[audc, 0]
        first = start + SETTLE
        bits = "".join("1" if data[(first + i * step) * 2:(first + i * step) * 2 + 2] != b"\0\0" else "0"
                       for i in range(len(pattern)))
        inverted = bits.translate(str.maketrans("01", "10"))
        if bits in pattern + pattern:
            verdict = "same"
        elif inverted in pattern + pattern:
            # Same wave upside down: sounds identical after the DC-blocking
            # cap. For toggling modes it only depends on the starting state.
            verdict = "same, inverted"
        else:
            verdict = "DIFFERENT"
        print(f"  AUDC {audc:2}: {verdict}" + ("" if verdict != "DIFFERENT" else
              f"\n    ours     {bits}\n    Stolberg {pattern} (any rotation)"))

    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
