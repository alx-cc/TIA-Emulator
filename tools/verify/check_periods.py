#!/usr/bin/env python3
"""Check each single-voice segment's repeat length and level. Pass/fail.

For every AUDC x AUDF segment listed in the sweep CSV, after skipping the
first ticks (the previous setting's divider finishing), the output must:
  - repeat with exactly the expected period from tia_model (it repeats with
    that period, and with no shorter one),
  - only take the values 0 and AUDV << 10 (constant AUDV << 10 for AUDC 0, 11).
"""

import csv
import struct
import sys

import tia_model
from sweep import SETTLE


def prime_factors(n):
    out, p = set(), 2
    while p * p <= n:
        while n % p == 0:
            out.add(p)
            n //= p
        p += 1
    if n > 1:
        out.add(n)
    return out


def repeats(data, start, period, length):
    """True if data[i] == data[i + period] for start <= i < start + length."""
    a = data[start * 2:(start + length) * 2]
    b = data[(start + period) * 2:(start + period + length) * 2]
    return len(a) == len(b) == length * 2 and a == b


def smallest_period(data, start, end):
    """Brute force, only used to report what was found when a check fails."""
    for p in range(1, (end - start) // 2 + 1):
        if repeats(data, start, p, end - start - p):
            return p
    return None


def main():
    if len(sys.argv) != 3:
        sys.exit("usage: check_periods.py <out.raw> <sweep.csv>")
    data = open(sys.argv[1], "rb").read()
    failures = 0
    count = 0

    for row in csv.DictReader(open(sys.argv[2])):
        start, length, voice, audc, audf, audv = (int(row[k]) for k in
            ("start", "length", "voice", "audc", "audf", "audv"))
        count += 1
        first = start + SETTLE
        end = start + length
        expected = tia_model.expected_period(audc, audf)
        level = audv << 10
        problems = []

        values = set(struct.unpack(f"<{end - first}h", data[first * 2:end * 2]))
        allowed = {level} if expected == 1 else {0, level}
        if not values <= allowed:
            problems.append(f"values {sorted(values)} not in {sorted(allowed)}")

        window = end - first - expected
        if not repeats(data, first, expected, window):
            problems.append(f"does not repeat every {expected}")
        else:
            for p in prime_factors(expected):
                if repeats(data, first, expected // p, end - first - expected // p):
                    problems.append(f"also repeats every {expected // p} (shorter than {expected})")
                    break

        if problems:
            failures += 1
            found = smallest_period(data, first, end)
            print(f"FAIL voice {voice} AUDC {audc:2} AUDF {audf:2}: "
                  + "; ".join(problems) + f" (found period {found})")

    if failures:
        print(f"FAIL: {failures} of {count} settings")
        return 1
    print(f"PASS: all {count} settings repeat with the expected period and levels")
    return 0


if __name__ == "__main__":
    sys.exit(main())
