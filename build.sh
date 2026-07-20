#!/usr/bin/env bash
# Build the Amber interpreter (./amber) from the C sources in this folder.
# Portable flags (no -march=native, warnings silenced) so the binary runs on
# any x86-64 Linux with a C compiler. Never installs anything system-wide.
set -e
cd "$(dirname "$(readlink -f "$0")")"
CC="${CC:-}"
if [ -z "$CC" ]; then
  for c in cc gcc clang; do command -v "$c" >/dev/null 2>&1 && { CC="$c"; break; }; done
fi
if [ -z "$CC" ]; then
  echo "No C compiler found. On Ubuntu:  sudo apt-get install build-essential" >&2
  exit 1
fi
echo "amber: compiling with $CC (portable -O2) ..."
mkdir -p o
F="-fsigned-char -fno-math-errno -fno-signed-zeros -fno-stack-protector -fomit-frame-pointer -w -O2"
for f in *.c; do "$CC" $F -o "o/${f%.c}.o" -c "$f"; done
"$CC" $F -o amber o/*.o -lm -ldl
echo "amber: built ./amber"
