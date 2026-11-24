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

# 4. with stdout closed, printing a result is 'io on stderr, and the session goes on
#    (the failed write freed nothing it could: a crash)
printf '1\n2\n' | AMBER_DIAG=0 "$AMBER" >&- 2> "$tmp/got"; rc=$?
printf "'io\n'io\n" > "$tmp/closed.want"
if [ $rc = 0 ] && cmp -s "$tmp/got" "$tmp/closed.want"
then echo "  PASS stdout closed"
else echo "  FAIL stdout closed (exit $rc)"; fail=1; fi

# 5. under \d a name the namespace lacks is read from the root, and an
#    assignment stays in the namespace (#131)
printf 'r:42\n\\d .foo\nr+1\nr:7\nr\n. `r\n\\d .\nr\n.foo.r\n. `r\n' > "$tmp/ns.k"
printf '43\n7\n7\n42\n7\n42\n' > "$tmp/ns.want"
check "\\d reads fall back to the root" "$tmp/ns.k" "$tmp/ns.want"

# 6. repl.k runs the one-letter commands, \v lands in the scroll-back like the
#    rest, and qSQL and the library work inside a namespace (#131)
printf 'aa:1 2 3\n\\v\n\\d .foo\nt:([]s:`a`b`a;v:10 20 30)\nexec v from select sum v by s from t\nsum 1 2\n' \
  | AMBER_DIAG=0 "$AMBER" repl.k > "$tmp/repl.got" 2>&1
ok=1
grep -q '| aa ' "$tmp/repl.got" || ok=0
grep -q '40 20$' "$tmp/repl.got" || ok=0
grep -q '> 3$' "$tmp/repl.got" || ok=0
grep -q 'error' "$tmp/repl.got" && ok=0
if [ $ok = 1 ]; then echo "  PASS repl.k backslash commands and \\d"
else echo "  FAIL repl.k backslash commands and \\d"; head -c 2000 "$tmp/repl.got" | cut -c1-120; fail=1; fi

exit $fail
