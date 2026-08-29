#!/usr/bin/env bash
# tests/test_comments.sh - regression tests for Amber's comment lexer.
# GNU AGPLv3 - see LICENSE and NOTICE.
#
# Guards docs/MISSING.md 14 bug 1: a bare "/" line that opens a block comment
# with no closing "\" line used to silently truncate the rest of the file and
# exit 0 (data loss with no diagnostic). It must now raise a clean parse error.
# Properly-closed block comments and trailing "/ ..." line comments must still
# work exactly as before.
set -u
cd "$(dirname "$0")/.." || exit 1
AMBER=${AMBER:-./amber}
[ -x "$AMBER" ] || { echo "test_comments: $AMBER not built"; exit 1; }

fail=0
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

check() { # name  expected-rc  expected-substr  file
  local name=$1 xrc=$2 want=$3 f=$4
  local out rc
  out=$("$AMBER" "$f" 2>&1); rc=$?
  if [ "$xrc" = "nonzero" ]; then [ "$rc" -ne 0 ] || { echo "  FAIL $name: expected nonzero exit, got 0"; fail=1; return; }
  else [ "$rc" -eq "$xrc" ] || { echo "  FAIL $name: expected exit $xrc, got $rc"; fail=1; return; }; fi
  if [ -n "$want" ]; then case "$out" in *"$want"*) ;; *) echo "  FAIL $name: output lacked '$want'"; echo "    got: $out"; fail=1; return;; esac; fi
  echo "  PASS $name"
}

# 1. unterminated block comment -> loud parse error, NOT a silent exit-0 truncation
printf '1+1\n/\n2+2\n' > "$tmp/unterminated.k"
check unterminated_block_errors nonzero "" "$tmp/unterminated.k"

# 2. properly-closed block comment ( / ... \ ) still parses; code after it runs
printf '`0:"before"\n/\nthis whole block\nis a comment\n\\\n`0:"after"\n' > "$tmp/closed.k"
check closed_block_runs 0 "after" "$tmp/closed.k"

# 3. trailing line comment ("/ text" or "expr / text") is unaffected
printf '`0:$3+4 / seven\n`0:"tail"\n' > "$tmp/trailing.k"
check trailing_line_comment 0 "tail" "$tmp/trailing.k"

# 4. bare "/" as the LAST line with a real trailing newline but no closer also errors
printf '`0:"x"\n/\n' > "$tmp/lastline.k"
check bare_slash_last_line nonzero "" "$tmp/lastline.k"

if [ "$fail" = 0 ]; then echo "test_comments: ALL PASSED"; else echo "test_comments: FAILURES"; fi
exit $fail
