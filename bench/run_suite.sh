#!/usr/bin/env bash
# bench/run_suite.sh - re-measure BENCHMARKS.md section 2: Amber, numpy/pandas, Polars, DuckDB.
# Run from anywhere:  ./bench/run_suite.sh
#   CORE=3            core every engine is pinned to (taskset; skipped if missing)
#   PY=python3        interpreter with numpy/pandas/polars/duckdb (missing ones are skipped)
#   OUT=dir           where the raw RESULT files go (default: a fresh temp dir)
# One engine per process, one after another, all single-threaded.
set -euo pipefail
cd "$(dirname "$0")/.."
CORE=${CORE:-3}
PY=${PY:-python3}
OUT=${OUT:-$(mktemp -d)}
mkdir -p "$OUT"
export AMBER_THREADS=1 POLARS_MAX_THREADS=1 OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 MKL_NUM_THREADS=1
PIN=""; command -v taskset >/dev/null && PIN="taskset -c $CORE"

[ -x ./amber ] || { echo "no ./amber here; build it first (./build.sh)" >&2; exit 1; }
echo "amber  -> $OUT/amber.txt" >&2
$PIN ./amber bench/suite.k < /dev/null > "$OUT/amber.txt"

: > "$OUT/python.txt"
if "$PY" -c 'import numpy' 2>/dev/null; then
  for e in numpy polars duckdb; do
    echo "$e -> $OUT/python.txt" >&2
    $PIN "$PY" bench/suite.py "$e" >> "$OUT/python.txt" || echo "$e failed, skipped" >&2
  done
else
  echo "no numpy for $PY: Amber only" >&2
fi

"$PY" bench/render.py "$OUT/amber.txt" "$OUT/python.txt"
