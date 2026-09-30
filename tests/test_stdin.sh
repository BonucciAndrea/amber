#!/usr/bin/env bash
# tests/test_stdin.sh - regression tests for reading a program from a pipe.
# GNU AGPLv3 - see LICENSE and NOTICE.
#
# With stdin not a terminal, rep() in src/m.c reads it in blocks and runs each
# complete line. A line split across two reads must be run whole: the part
# already read has to be kept until the rest arrives.
set -u
cd "$(dirname "$0")/.." || exit 1
AMBER=${AMBER:-./amber}
[ -x "$AMBER" ] || { echo "test_stdin: $AMBER not built"; exit 1; }

fail=0
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

# 1. many short lines, so that block boundaries fall inside lines: every line
#    must run, in order, with its own result
for i in $(seq 1 300); do echo "$i+1000"; done > "$tmp/lines.k"
seq 1001 1300 > "$tmp/want"
if AMBER_DIAG=0 "$AMBER" < "$tmp/lines.k" > "$tmp/got" 2>&1 && cmp -s "$tmp/got" "$tmp/want"
then echo "  PASS short lines across read boundaries"
else echo "  FAIL short lines across read boundaries"; diff "$tmp/want" "$tmp/got" | head -5; fail=1; fi

exit $fail
