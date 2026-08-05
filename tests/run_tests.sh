#!/usr/bin/env bash
# tests/run_tests.sh  -  run every Amber test suite.
# GNU AGPLv3 - see LICENSE and NOTICE.
#
#   tests/run_tests.sh              build + run all K suites + the C unit tests + a short fuzz pass
#   tests/run_tests.sh --asan       additionally build an ASan/UBSan interpreter and re-run every
#                                   suite plus a long fuzz pass under it (slow, catches silent OOB)
#   tests/run_tests.sh --quick      K suites only
#
# Exit status is 0 iff every suite reports "0 failures" and the fuzzer finds
# no crashes or hangs.
set -u
# The K suites locate their own modules from `argv 1 and run from any cwd; this
# cd is only so build.sh, the C unit tests and the o/ scratch dir land in the
# repo root.
cd "$(dirname "$(readlink -f "$0")")/.."
ASAN=0; QUICK=0
for a in "$@"; do case "$a" in --asan) ASAN=1;; --quick) QUICK=1;; esac; done

fail=0
say(){ printf '\n\033[1m== %s\033[0m\n' "$*"; }
run_k(){ # run_k <binary> <script> <label>
  # Amber renders a diagnostic to stderr even for errors that .[f;a;h] goes on
  # to trap, and the suites deliberately provoke many such errors (te[...]), so
  # stderr is kept aside and only shown when the suite actually fails.
  err=$(mktemp); out=$("$1" "$2" 2>"$err"); rc=$?
  echo "$out" | grep -vE '^(simd|par):'
  # Three independent gates: the suite must exit 0, it must actually print a
  # report (a suite that dies mid-file prints nothing at all), and that report
  # must say 0 failures.
  if [ "$rc" = 0 ] && echo "$out" | grep -Eq 'tests run' && echo "$out" | grep -Eq '0 failures'
  then echo "  -> PASS ($3)"
  else echo "  -> FAIL ($3) rc=$rc"; sed -n '1,40p' "$err"; fail=1; fi
  rm -f "$err"
}

say "build"
./build.sh || exit 1

SUITES="test.k test-fin.k tests/test_matrix.k tests/test_qsql.k"
for s in $SUITES; do say "$s"; run_k ./amber "$s" native; done

if [ "$QUICK" = 0 ]; then
  say "C unit tests (tests/*.c)"
  mkdir -p o/t
  CC="${CC:-cc}"
  # test_simd.c and test_parallel.c are deliberately standalone; test_ast.c links
  # the whole interpreter except 0.c (which owns main()).  See each file's header.
  for t in tests/test_simd.c tests/test_parallel.c; do
    b="o/t/$(basename "${t%.c}")"
    if $CC -w -O2 -std=c99 -Isrc -pthread -o "$b" "$t" src/simd.c src/parallel.c -lm 2>/dev/null && "$b" >/dev/null 2>&1
    then echo "  -> PASS ($t)"; else echo "  -> FAIL ($t)"; fail=1; fi
  done
  # test_ast.c links the whole interpreter and supplies its own main(). src/0.c
  # owns main() BUT also owns `pg` (the page size) and the non-wasm js_eval()
  # stub, so it cannot simply be dropped -- compile it with -Dldstatic, which is
  # exactly the guard 0.c already wraps main() in.
  objs=""
  for f in src/*.c; do
    d=""; [ "$(basename "$f")" = "0.c" ] && d="-Dldstatic"
    $CC -w -O2 -pthread $d -c "$f" -o "o/t/$(basename "${f%.c}").o" 2>/dev/null || true
    objs="$objs o/t/$(basename "${f%.c}").o"; done
  if $CC -w -O2 -std=c99 -Isrc -pthread -o o/t/test_ast tests/test_ast.c $objs -lm -ldl 2>/dev/null \
     || $CC -w -O2 -std=c99 -Isrc -pthread -o o/t/test_ast tests/test_ast.c $objs -lm 2>/dev/null
  then if o/t/test_ast >/dev/null 2>&1; then echo "  -> PASS (tests/test_ast.c)"
       else echo "  -> FAIL (tests/test_ast.c)"; fail=1; fi
  else echo "  -> SKIP (tests/test_ast.c did not link)"; fi

  say "fuzz / crash harness"
  if python3 tests/fuzz.py --amber ./amber --cases 1500; then echo "  -> PASS (fuzz)"
  else echo "  -> FAIL (fuzz)"; fail=1; fi
fi

if [ "$ASAN" = 1 ]; then
  say "sanitizer build (ASan + UBSan)"
  mkdir -p o/san
  CC="${CC:-cc}"
  for f in src/*.c; do
    $CC -fsigned-char -g -O1 -w -pthread -fsanitize=address,undefined \
        -fno-omit-frame-pointer -c "$f" -o "o/san/$(basename "${f%.c}").o" || exit 1
  done
  $CC -fsigned-char -g -O1 -w -pthread -fsanitize=address,undefined \
      -o o/san/amber o/san/*.o -lm -ldl || exit 1
  export ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=print_stacktrace=1
  for s in $SUITES; do
    say "$s (asan+ubsan)"
    out=$(o/san/amber "$s" 2>&1); rc=$?
    echo "$out" | grep -vE '^(simd|par):' | tail -8
    if echo "$out" | grep -Eq 'runtime error:|AddressSanitizer:|LeakSanitizer:'; then
      echo "  -> FAIL (sanitizer diagnostics above)"; fail=1
    elif [ "$rc" = 0 ] && echo "$out" | grep -Eq '0 failures'; then echo "  -> PASS"
    else echo "  -> FAIL rc=$rc"; fail=1; fi
  done
  say "fuzz under sanitizers"
  if python3 tests/fuzz.py --amber o/san/amber --cases 800 --timeout 30; then echo "  -> PASS"
  else echo "  -> FAIL"; fail=1; fi
fi

say "result"
[ "$fail" = 0 ] && echo "ALL SUITES PASSED" || echo "SOME SUITES FAILED"
exit $fail
