#!/usr/bin/env python3
"""Compare two raw outputs from tia_dump sample for sample. Pass/fail."""

import struct
import sys


def main():
    if len(sys.argv) != 3:
        sys.exit("usage: compare.py <ours.raw> <reference.raw>")
    ours = open(sys.argv[1], "rb").read()
    ref = open(sys.argv[2], "rb").read()

    if ours == ref:
        print(f"PASS: {len(ours) // 2} samples identical")
        return 0

    n = min(len(ours), len(ref)) // 2
    a = struct.unpack(f"<{n}h", ours[: n * 2])
    b = struct.unpack(f"<{n}h", ref[: n * 2])
    diffs = [i for i in range(n) if a[i] != b[i]]
    if len(ours) != len(ref):
        print(f"FAIL: lengths differ ({len(ours) // 2} vs {len(ref) // 2} samples)")
    if diffs:
        i = diffs[0]
        print(f"FAIL: {len(diffs)} samples differ; first at tick {i}: ours {a[i]}, reference {b[i]}")
    return 1


if __name__ == "__main__":
    sys.exit(main())
