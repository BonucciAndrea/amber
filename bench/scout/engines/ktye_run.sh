#!/bin/sh
# bench/scout/engines/ktye_run.sh - argv shim for the ktye/k scout adapter.
# Run:  sh bench/scout/engines/ktye_run.sh <ktye-binary> bench/scout/engines/ktye.k <op> <N> <runs> <warmup>
#
# ktye/k treats every command-line argument as a file to load (a .k file is
# executed, any other file is bound to a variable named after it) and has no
# argv accessor, so the protocol arguments are passed the one way it accepts:
# as an -e expression evaluated after the script is loaded.  <ktye-binary> is
# ktye-scout: ktye's kv.c built with one extra native, now[], as the clock.
k="$1"; script="$2"; op="$3"; n="${4:-10000000}"; runs="${5:-5}"; warm="${6:-2}"
case "$op" in *[!a-z0-9_]*) echo "ERROR bad op" >&2; exit 2;; esac
exec "$k" "$script" -e "main[\"$op\";$n;$runs;$warm]"
