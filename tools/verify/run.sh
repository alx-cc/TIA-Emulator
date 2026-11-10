#!/bin/sh
# Build the host programs and run every verification check.
# Output goes to build-host/verify/ (raw samples, sweep WAV, script).
# Exits non-zero if a pass/fail check fails; the Stella comparison only reports.
set -e
cd "$(dirname "$0")/../.."
cmake -S host -B build-host -G Ninja -DCMAKE_BUILD_TYPE=Release >/dev/null
cmake --build build-host >/dev/null

out=build-host/verify
mkdir -p $out
python3 tools/verify/sweep.py $out/sweep
build-host/tia_dump $out/sweep.txt $out/ours.raw $out/ours.wav
build-host/tia_dump_ref $out/sweep.txt $out/ref.raw
build-host/tia_dump_stella $out/sweep.txt $out/stella.raw

status=0
echo "== 1. Exact match with MAME"
python3 tools/verify/compare.py $out/ours.raw $out/ref.raw || status=1
echo "== 2. Repeat length and levels for every setting"
python3 tools/verify/check_periods.py $out/ours.raw $out/sweep.csv || status=1
echo "== 2b. Stolberg's frequency tables"
python3 tools/verify/check_stolberg.py $out/ours.raw $out/sweep.csv || status=1
echo "== 3. Stella"
python3 tools/verify/compare_stella.py $out/ours.raw $out/stella.raw $out/sweep.csv
exit $status
