#!/usr/bin/env bash
# tests/test_ext_seam.sh  -  the extension seam (src/ext.h) end to end.
#
# Installs tests/ext_probe.c the way a real out-of-tree package installs
# itself -- copy a .c file into ext/, re-run ./build.sh, patch nothing -- then
# checks that the engine picked it up, and that removing it puts the engine
# back exactly as it was.
#
# GNU AGPLv3 - see LICENSE and NOTICE.
set -u
# ---- portable script-directory resolution ---------------------------------
# `readlink -f` is GNU coreutils. BSD/macOS readlink gained -f only in macOS
# 12.3 (2022), so on any older Mac every script that used it resolved to an
# empty path and cd'd to the wrong place -- or silently to $HOME. This uses
# only POSIX readlink (no -f) plus `cd -P`, which behaves identically on macOS,
# Linux, WSL2 and BusyBox, and still follows a chain of symlinks.
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
fail=0
ok(){ printf '    %-28s %s\n' "$1" "$2"; [ "$2" = PASS ] || fail=1; }

cleanup(){ rm -f ext/ext_probe.c; }
trap cleanup EXIT

# ---- 0. baseline: no extension --------------------------------------------
rm -f ext/ext_probe.c
./build.sh >/dev/null 2>&1 || { echo "    baseline build FAILED"; exit 1; }
out=$(printf '2+2\n\\\\\n' | ./a 2>&1)
case "$out" in *ext-probe*) ok "baseline_has_no_extension" FAIL;;
               *) ok "baseline_has_no_extension" PASS;; esac
# an unregistered verb must still raise 'domain (E0104), not silently yield 0.
# The literal `pr7 is echoed back inside the diagnostic, so the test keys off
# the error CODE rather than off the digit.
out=$(printf '`pr7 0\n\\\\\n' | ./a 2>&1)
case "$out" in *E0104*) ok "baseline_verb_absent" PASS;; *) ok "baseline_verb_absent" FAIL;; esac

# ---- 1. install ------------------------------------------------------------
mkdir -p ext
cp tests/ext_probe.c ext/
./build.sh >/dev/null 2>&1 || { echo "    build with extension FAILED"; exit 1; }
ok "builds_with_extension" PASS

out=$(printf '100+`pr7 0\n\\\\\n' | ./a 2>&1)
case "$out" in *107*) ok "registered_verb_works" PASS;; *) ok "registered_verb_works" FAIL;; esac

out=$(printf '\\probe\n\\\\\n' | ./a 2>&1)
case "$out" in *"probe: ok started=1"*) ok "repl_command_claimed" PASS;;
               *) ok "repl_command_claimed" FAIL;; esac

out=$(./amber --help 2>&1)
case "$out" in *"ext probe"*) ok "usage_appended" PASS;; *) ok "usage_appended" FAIL;; esac

out=$(printf '2+2\n\\\\\n' | ./a 2>&1)
case "$out" in *"[ext-probe]"*) ok "banner_tag_shown" PASS;; *) ok "banner_tag_shown" FAIL;; esac

# the engine's own suite must be unaffected by the presence of an extension
if ./amber test.k 2>/dev/null | grep -q '0 failures'; then ok "core_suite_unaffected" PASS
else ok "core_suite_unaffected" FAIL; fi

# ---- 2. uninstall ----------------------------------------------------------
rm -f ext/ext_probe.c
./build.sh >/dev/null 2>&1 || { echo "    rebuild after removal FAILED"; exit 1; }
out=$(printf '`pr7 0\n\\\\\n' | ./a 2>&1)
case "$out" in *E0104*) ok "clean_uninstall" PASS;; *) ok "clean_uninstall" FAIL;; esac

exit $fail
