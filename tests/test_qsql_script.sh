#!/usr/bin/env bash
# tests/test_qsql_script.sh - regression: bare qSQL works in a loaded .k file.
# GNU AGPLv3 - see LICENSE and NOTICE.
#
# amber 2.0.0 made the C loader bsl() run each file's source through the qSQL
# rewriter (qrwf, qsql.k) so `select .. from ..` works in a script exactly as it
# does at the REPL prompt -- no sel"..." wrapper. This checks the end-to-end
# path: once the stdlib is loaded, a `\l`'d file that uses bare qSQL evaluates
# correctly, and (importantly) a plain non-qSQL script is left byte-for-byte
# unchanged so nothing regressed.
set -u
cd "$(dirname "$0")/.." || exit 1
AMBER=${AMBER:-./amber}
[ -x "$AMBER" ] || { echo "test_qsql_script: $AMBER not built"; exit 1; }
fail=0
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT

# The query file uses BARE qSQL - no sel"..." anywhere.
cat > "$tmp/queries.k" <<'K'
t:([]sym:`a`b`a`b; px:100 200 300 400; sz:10 20 30 40)
`0:"G:"
show select sum px by sym from t
`0:"W:"
show select from t where px>250
`0:"E:"
show exec sum px from t
K
# The driver loads the stdlib first, THEN loads the query file (so qrwf exists
# when bsl() rewrites it) -- exactly how a real session or launcher behaves.
cat > "$tmp/driver.k" <<K
\l amber.k
\l std.k
\l qsql.k
\l $tmp/queries.k
K

out=$("$AMBER" "$tmp/driver.k" 2>&1); rc=$?
check() { case "$out" in *"$1"*) echo "  PASS $2";; *) echo "  FAIL $2 (missing '$1')"; fail=1;; esac; }
[ "$rc" -eq 0 ] || { echo "  FAIL driver exited $rc"; echo "$out"; fail=1; }
check "400" "group-by select rewrote and ran"   # sum px for sym a = 100+300
check "600" "group-by select sym b"             # sum px for sym b = 200+400
check "300" "where-clause select rewrote and ran"
check "1000" "exec rewrote and ran"             # sum of all px

# A non-qSQL script must be evaluated verbatim (no false-positive rewrite).
cat > "$tmp/plain.k" <<'K'
from:42
select:7
`0:"PLAIN:",$from+select
K
out2=$("$AMBER" "$tmp/plain.k" 2>&1)
case "$out2" in *"PLAIN:49"*) echo "  PASS non-qSQL script untouched";; *) echo "  FAIL non-qSQL script (got: $out2)"; fail=1;; esac

if [ "$fail" = 0 ]; then echo "test_qsql_script: ALL PASSED"; else echo "test_qsql_script: FAILURES"; fi
exit $fail
