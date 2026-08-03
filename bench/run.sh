#!/usr/bin/env bash
# Cross-engine benchmark. Runs every engine that is installed; skips the rest.
#   ./run.sh                 # auto-detect amber, growler/k, q, python (numpy/pandas[/duckdb/polars])
#   K=/path/to/growler ./run.sh    # point at a specific growler/k binary
set -u
cd "$(dirname "$0")/.."
OUT=$(mktemp -d); trap 'rm -rf "$OUT"' EXIT
say(){ printf '  %-14s %s\n' "$1" "$2"; }
echo "detecting engines..."
# Amber (build if needed)
[ -x ./amber ] || bash build.sh >/dev/null 2>&1 || true
if [ -x ./amber ]; then ./amber bench/bench.k 2>/dev/null | grep -E '^(SANITY|TIME)' > "$OUT/amber.txt"; say amber "ok"; else say amber "MISSING"; fi
# Python engines
for e in numpy_pandas duckdb polars; do
  python3 bench/bench.py "$e" 2>/dev/null | grep -E '^(SANITY|TIME)' > "$OUT/$e.txt"
  if [ -s "$OUT/$e.txt" ]; then say "$e" "ok"; else rm -f "$OUT/$e.txt"; say "$e" "skip (not installed)"; fi
done
# growler/k
KBIN="${K:-}"; [ -z "$KBIN" ] && for c in growler k ngn ok; do command -v "$c" >/dev/null 2>&1 && { KBIN=$(command -v "$c"); break; }; done
if [ -n "$KBIN" ]; then "$KBIN" bench/bench_k.k 2>/dev/null | grep -E '^(SANITY|TIME)' > "$OUT/growler_k.txt"
  [ -s "$OUT/growler_k.txt" ] && say "growler/k" "ok ($KBIN)" || { rm -f "$OUT/growler_k.txt"; say "growler/k" "ran but no output (check dialect/timer in bench_k.k)"; }
else say "growler/k" "skip (set K=/path/to/growler)"; fi
# kdb+/q
if command -v q >/dev/null 2>&1; then q bench/bench_q.q -q 2>/dev/null | grep -E '^(SANITY|TIME)' > "$OUT/q.txt"; [ -s "$OUT/q.txt" ] && say q "ok" || { rm -f "$OUT/q.txt"; say q "ran but no output"; }
else say q "skip (not installed)"; fi
echo; python3 bench/compare.py "$OUT"
