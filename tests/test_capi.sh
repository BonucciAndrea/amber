#!/usr/bin/env bash
# tests/test_capi.sh  -  build libamber.so and exercise it from tests/test_capi.c.
# GNU AGPLv3 - see LICENSE and NOTICE.
#
#   tests/test_capi.sh            plain -O2 build, then an ASan+UBSan build
#   tests/test_capi.sh --plain    plain build only (fast)
#   tests/test_capi.sh --san      sanitizer build only
#
# The sanitizer leg is the one that matters. The C API's whole contract is an
# ownership rule stated in prose ("functions that return a value return one you
# own; functions that take one borrow it"), and prose does not compile. Running
# the same test under -fsanitize=address with detect_leaks=1 turns every
# sentence of that rule into something the build can check: a missing
# amber_release() shows up as a leak, a double release as a heap-use-after-free,
# and a wrong pointer handed out by amber_get_vector_ptr() as an out-of-bounds
# read on the very first dereference.
#
# Exit status is 0 iff every leg passed with zero sanitizer diagnostics.
set -u
am_scriptdir() {
  am__p=$1
  while [ -h "$am__p" ]; do
    am__d=$(CDPATH='' cd -- "$(dirname -- "$am__p")" && pwd -P) || return 1
    am__l=$(readlink -- "$am__p")
    case $am__l in /*) am__p=$am__l ;; *) am__p=$am__d/$am__l ;; esac
  done
  CDPATH='' cd -- "$(dirname -- "$am__p")" || return 1
  pwd -P
}
cd "$(am_scriptdir "$0")/.."
ROOT=$(pwd -P)
CC="${CC:-cc}"
DO_PLAIN=1; DO_SAN=1
for a in "$@"; do case "$a" in --plain) DO_SAN=0;; --san) DO_PLAIN=0;; esac; done
fail=0
say(){ printf '\n\033[1m== %s\033[0m\n' "$*"; }

SOEXT=so
[ "$(uname -s 2>/dev/null)" = Darwin ] && SOEXT=dylib

if [ "$DO_PLAIN" = 1 ]; then
  say "libamber.$SOEXT  (release build)"
  ./build.sh --shared || exit 1
  mkdir -p o/capi
  # -Isrc is for src/ext.h ONLY. The test deliberately includes no other core
  # header, so if this ever needs a second -I the API has grown a leak of its
  # internals and that is the bug, not the flag.
  if $CC -w -O2 -std=c99 -Isrc -o o/capi/test_capi tests/test_capi.c \
        -L"$ROOT" -lamber -lm -Wl,-rpath,"$ROOT"; then
    if o/capi/test_capi "$ROOT"; then echo "  -> PASS (release)"
    else echo "  -> FAIL (release)"; fail=1; fi
  else echo "  -> FAIL (release build did not link)"; fail=1; fi
fi

if [ "$DO_SAN" = 1 ]; then
  say "libamber.$SOEXT  (AddressSanitizer + UndefinedBehaviorSanitizer)"
  # Same reasoning as tests/run_tests.sh's --asan leg: UBSan's object-size check
  # is off because Amber stores every array header at NEGATIVE offsets from the
  # payload pointer (the _r/_n/_T/_A macros in src/a.h), which
  # __builtin_object_size cannot model and false-positives on. Every other check
  # -- alignment, null, signed overflow, bounds, VLA, shift, return -- stays on.
  SAN="-fsanitize=address,undefined -fno-sanitize=object-size"
  mkdir -p o/capisan
  for f in src/*.c ext/*.c; do
    [ -e "$f" ] || continue
    $CC -fsigned-char -g -O1 -w -pthread -fPIC -Dshared $SAN -fno-omit-frame-pointer \
        -c "$f" -o "o/capisan/$(basename "${f%.c}").o" || exit 1
  done
  # No export map on the sanitizer build: ASan needs its own interceptor symbols
  # visible across the boundary, and hiding the engine's internals is a hygiene
  # property of the shipped artefact, not something under test here.
  SOFLAGS="-shared"
  [ "$SOEXT" = dylib ] && SOFLAGS="-dynamiclib -install_name @rpath/libamber_san.dylib"
  $CC -fsigned-char -g -O1 -w -pthread -fPIC $SAN $SOFLAGS \
      -o "o/capisan/libamber_san.$SOEXT" o/capisan/*.o -lm -ldl 2>/dev/null \
   || $CC -fsigned-char -g -O1 -w -pthread -fPIC $SAN $SOFLAGS \
      -o "o/capisan/libamber_san.$SOEXT" o/capisan/*.o -lm || exit 1

  if $CC -w -g -O1 -std=c99 -Isrc $SAN -o o/capisan/test_capi tests/test_capi.c \
        -L"$ROOT/o/capisan" -lamber_san -lm -Wl,-rpath,"$ROOT/o/capisan"; then
    export ASAN_OPTIONS="detect_leaks=1:detect_stack_use_after_return=1"
    export UBSAN_OPTIONS="print_stacktrace=1:halt_on_error=0"
    # Belt and braces. Amber reserves its object heap (mmap) and its interned
    # symbol table for process lifetime by design and has no teardown path --
    # see amber_shutdown() in src/ext.c, which says so -- and on some platforms
    # LeakSanitizer reports exactly that as a leak. It does NOT on the reference
    # build (glibc/x86-64: this test reports zero leaks with suppressions
    # disabled entirely, which is the state it should be kept in), so these four
    # frames are here to keep a platform difference from turning into a red CI
    # light, not to hide anything. A leak the API itself caused is not in this
    # list and will still fail the run.
    cat > o/capisan/lsan.supp <<'SUPP'
leak:mm
leak:us
leak:kinit
leak:ki
SUPP
    export LSAN_OPTIONS="suppressions=$ROOT/o/capisan/lsan.supp:print_suppressions=0"
    out=$(o/capisan/test_capi "$ROOT" 2>&1); rc=$?
    echo "$out" | tail -25
    if echo "$out" | grep -Eq 'runtime error:|AddressSanitizer:|LeakSanitizer:|UndefinedBehaviorSanitizer:'; then
      echo "  -> FAIL (sanitizer diagnostics above)"; fail=1
    elif [ "$rc" = 0 ]; then echo "  -> PASS (0 sanitizer diagnostics)"
    else echo "  -> FAIL rc=$rc"; fail=1; fi
  else echo "  -> FAIL (sanitizer build did not link)"; fail=1; fi
fi

say "result"
[ "$fail" = 0 ] && echo "C API: ALL LEGS PASSED" || echo "C API: SOME LEGS FAILED"
exit $fail
