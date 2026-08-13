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
ASAN=0; QUICK=0; TSAN=0
for a in "$@"; do case "$a" in --asan) ASAN=1;; --tsan) TSAN=1;; --quick) QUICK=1;; esac; done

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

SUITES="test.k test-fin.k test-ext.k tests/test_matrix.k tests/test_qsql.k tests/test_sort_window.k examples/peach_verify.k"
for s in $SUITES; do say "$s"; run_k ./amber "$s" native; done

# peach is the thread-pool primitive: run its verifier again with the pool
# forced to several lanes so CI actually exercises concurrent workers (the
# default may pick 1 lane on a single-core runner). Same 0-failures gate.
say "examples/peach_verify.k (AMBER_THREADS=4)"; ( export AMBER_THREADS=4; run_k ./amber examples/peach_verify.k "threads=4" )

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
  # UBSan's `object-size` sub-check is disabled: Amber deliberately stores every
  # array's header (refcount, length, type, bucket) at NEGATIVE offsets from the
  # payload pointer (see the _r/_n/_T/_A macros in a.h), so __builtin_object_size
  # sees "too small" an object and false-positives on those perfectly valid
  # header loads. This is intrinsic to the tagged/arena representation and
  # predates -- and is unrelated to -- the peach thread pool; every other UBSan
  # check (signed overflow, alignment, null, bounds, VLA, ...) stays on, and the
  # whole suite (peach included) is clean under AMBER_THREADS with it.
  SAN="-fsanitize=address,undefined -fno-sanitize=object-size"
  for f in src/*.c; do
    $CC -fsigned-char -g -O1 -w -pthread $SAN \
        -fno-omit-frame-pointer -c "$f" -o "o/san/$(basename "${f%.c}").o" || exit 1
  done
  $CC -fsigned-char -g -O1 -w -pthread $SAN \
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

if [ "$TSAN" = 1 ]; then
  # ThreadSanitizer: the real race detector for peach's thread pool. `address`
  # and `thread` instrumentation are mutually exclusive (the compiler rejects
  # combining them), so TSan is its own build, separate from --asan. Every suite
  # is run with the pool forced to 4 lanes so concurrent workers -- the lock-free
  # per-thread allocator (bkt[]), the thread-local PRNG, the atomic refcounts and
  # the pool's own sync -- are all actually exercised under the detector.
  say "sanitizer build (ThreadSanitizer)"
  mkdir -p o/tsan
  CC="${CC:-cc}"
  for f in src/*.c; do
    $CC -fsigned-char -g -O1 -w -pthread -fsanitize=thread \
        -fno-omit-frame-pointer -c "$f" -o "o/tsan/$(basename "${f%.c}").o" || exit 1
  done
  $CC -fsigned-char -g -O1 -w -pthread -fsanitize=thread \
      -o o/tsan/amber o/tsan/*.o -lm -ldl || exit 1
  export TSAN_OPTIONS="halt_on_error=0 report_signal_unsafe=0 exitcode=99"
  for s in $SUITES; do
    say "$s (tsan, AMBER_THREADS=4)"
    out=$(AMBER_THREADS=4 o/tsan/amber "$s" 2>&1); rc=$?
    echo "$out" | grep -vE '^(simd|par):' | tail -6
    if echo "$out" | grep -Eq 'ThreadSanitizer: (data race|lock-order|deadlock)'; then
      echo "  -> FAIL (ThreadSanitizer diagnostics above)"; fail=1
    elif echo "$out" | grep -Eq '0 failures'; then echo "  -> PASS (0 races)"
    else echo "  -> FAIL rc=$rc"; fail=1; fi
  done
fi

say "result"
[ "$fail" = 0 ] && echo "ALL SUITES PASSED" || echo "SOME SUITES FAILED"
exit $fail
