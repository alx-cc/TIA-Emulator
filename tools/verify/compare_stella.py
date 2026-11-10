#!/usr/bin/env python3
"""Compare our output with Stella's, per single-voice setting. Report only.

Stella models the TIA circuit independently of Ron Fries' code. Its counters
start in a different state, so the outputs are not expected to line up tick
for tick. Instead, for each AUDC x AUDF setting this compares the repeating
pattern: same period, and the same pattern from some starting point (or the
same pattern inverted, which sounds identical after the DC-blocking cap).

Always exits 0: differences are for a person to look at, and the core is never
changed to match Stella.
"""

import csv
import sys
from collections import defaultdict

from check_periods import repeats
from sweep import SETTLE
import tia_model


def find_period(data, first, end, hint):
    """Smallest period among divisors of small multiples of hint, or None."""
    candidates = set()
    for m in range(1, 5):
        n = hint * m
        candidates.update(d for d in range(1, n + 1) if n % d == 0)
    for p in sorted(candidates):
        if p * 2 <= end - first and repeats(data, first, p, end - first - p):
            return p
    return None


def bits(data, first, period):
    return "".join("0" if data[i * 2:i * 2 + 2] == b"\0\0" else "1"
                   for i in range(first, first + period))


def main():
    if len(sys.argv) != 4:
        sys.exit("usage: compare_stella.py <ours.raw> <stella.raw> <sweep.csv>")
    ours = open(sys.argv[1], "rb").read()
    stella = open(sys.argv[2], "rb").read()

    results = defaultdict(lambda: defaultdict(list))  # verdict -> audc -> [voice:audf]
    count = 0
    for row in csv.DictReader(open(sys.argv[3])):
        start, length, voice, audc, audf = (int(row[k]) for k in
            ("start", "length", "voice", "audc", "audf"))
        count += 1
        first, end = start + SETTLE, start + length
        hint = tia_model.expected_period(audc, audf)
        p_ours = find_period(ours, first, end, hint)
        p_stella = find_period(stella, first, end, hint)

        if p_ours is None or p_stella is None or p_ours != p_stella:
            verdict = f"different period (ours {p_ours}, Stella {p_stella})"
        else:
            a, b = bits(ours, first, p_ours), bits(stella, first, p_stella)
            if b in a + a:
                verdict = "same"
            elif b.translate(str.maketrans("01", "10")) in a + a:
                verdict = "same, inverted"
            else:
                verdict = "same period, different pattern"
        results[verdict][audc].append(f"{voice}:{audf}")

    print(f"Stella comparison, {count} single-voice settings (report only):")
    for verdict in sorted(results, key=lambda v: (v != "same", v != "same, inverted", v)):
        by_audc = results[verdict]
        n = sum(len(v) for v in by_audc.values())
        print(f"  {verdict}: {n}")
        if verdict not in ("same", "same, inverted"):
            for audc in sorted(by_audc):
                cases = by_audc[audc]
                shown = ", ".join(cases[:8]) + (" ..." if len(cases) > 8 else "")
                print(f"    AUDC {audc:2}: {len(cases)} settings (voice:AUDF {shown})")
        else:
            print(f"    AUDC {', '.join(str(a) for a in sorted(by_audc))}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
