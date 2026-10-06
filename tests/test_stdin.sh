#!/usr/bin/env bash
# tests/test_stdin.sh - regression tests for reading a program from a pipe.
# GNU AGPLv3 - see LICENSE and NOTICE.
#
# With stdin not a terminal, rep() in src/m.c reads it in blocks and runs each
# complete line. A line split across two reads must be run whole: the part
# already read has to be kept until the rest arrives. And a line may be longer
# than the buffer.
set -u
cd "$(dirname "$0")/.." || exit 1
AMBER=${AMBER:-./amber}
[ -x "$AMBER" ] || { echo "test_stdin: $AMBER not built"; exit 1; }

fail=0
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

check() { # name  input-file  expected-output-file
  if AMBER_DIAG=0 "$AMBER" < "$2" > "$tmp/got" 2>&1 && cmp -s "$tmp/got" "$3"
  then echo "  PASS $1"
  else echo "  FAIL $1"; diff "$3" "$tmp/got" | head -5 | cut -c1-100; fail=1; fi
}

# 1. many short lines, so that block boundaries fall inside lines: every line
#    must run, in order, with its own result
for i in $(seq 1 300); do echo "$i+1000"; done > "$tmp/lines.k"
seq 1001 1300 > "$tmp/lines.want"
check "short lines across read boundaries" "$tmp/lines.k" "$tmp/lines.want"

# 2. lines far longer than the buffer (they used to kill the process with
#    'LONGLINE), between short ones. The length is in a comment and in blanks,
#    because literals and statement counts have limits of their own.
{ echo '1+1'
  printf '2+2 /'; head -c 300000 /dev/zero | tr '\0' a; echo
  printf '3+'; head -c 300000 /dev/zero | tr '\0' ' '; echo '3'
  echo '4+4'; } > "$tmp/long.k"
printf '2\n4\n6\n8\n' > "$tmp/long.want"
check "long lines" "$tmp/long.k" "$tmp/long.want"

# 3. a line does not depend on the line parsed before it in the same buffer.
#    Just past a lambda's [params] a - starts a number; where the last [params]
#    ended was kept from line to line, so the third line, moved to the start of
#    the buffer when it is split across two reads, read vvvv-1 as vvvv applied
#    to -1 (0N): its - is where the first line's ] ended. The layout depends on
#    rep's 256-byte buffer: if that size changes, this no longer pins the bug
{ echo '{[ab]ab}'; printf 'vvvv:5 6%232s\n' ''; echo '(vvvv-1)'; } > "$tmp/params.k"
printf '{[ab]ab}\n4 5\n' > "$tmp/params.want"
check "a line after a lambda's parameters" "$tmp/params.k" "$tmp/params.want"

exit $fail
