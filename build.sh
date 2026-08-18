#!/usr/bin/env bash
# Build the Amber interpreter (./amber) from the C sources in this folder.
# Portable flags (no -march=native, warnings silenced) so the binary runs on
# any x86-64 / arm64 host with a C compiler. Never installs anything system-wide.
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
# Portable -O3 (+ LTO where supported) by default. Set AMBER_NATIVE=1 for a faster
# machine-specific build (adds -march=native -funroll-loops; the binary then only runs
# on this CPU family).
F="-Isrc -fsigned-char -fno-math-errno -fno-signed-zeros -fno-stack-protector -fomit-frame-pointer -w -O3 -pthread"
LTOTAG=""
if printf 'int main(){return 0;}' | "$CC" -flto -x c - -o .ltocheck 2>/dev/null; then F="$F -flto"; LTOTAG=" -flto"; fi
rm -f .ltocheck
# OpenMP drives the `omp simd` / `omp parallel for reduction` hints on the
# reduction kernels in src/3.c. Probed rather than assumed: where it is absent
# the AMSIMD/AMPAR macros expand to nothing and the hand-unrolled four-way
# accumulators still carry the vectorisation, so the build never depends on it.
OMPTAG=""
if printf '#include <omp.h>\nint main(){return omp_get_max_threads();}' | "$CC" -fopenmp -x c - -o .ompcheck 2>/dev/null; then F="$F -fopenmp"; OMPTAG=" +openmp"; fi
rm -f .ompcheck
if [ -n "${AMBER_NATIVE:-}" ]; then F="$F -march=native -funroll-loops"; MODE="native -O3$LTOTAG$OMPTAG"; else MODE="portable -O3$LTOTAG$OMPTAG"; fi

# ---- extensions -------------------------------------------------------------
# ext/ is empty in a stock checkout. An out-of-tree package (for example the
# separate `amber-ai` repository) drops its .c files there and re-runs this
# script; they are compiled with the same flags and linked into the same binary,
# and they plug themselves in through the hooks in src/ext.h. Nothing in src/
# is ever patched, so pulling a new Amber release cannot conflict with them.
EXT=""
if [ -d ext ]; then
  for f in ext/*.c; do [ -e "$f" ] && EXT="$EXT $f"; done
fi
EXTTAG=""
[ -n "$EXT" ] && EXTTAG=" + extensions:$(for f in $EXT; do printf ' %s' "$(basename "$f" .c)"; done)"

echo "amber: compiling with $CC ($MODE)$EXTTAG ..."
mkdir -p o
# Drop object files whose source no longer exists (an uninstalled extension),
# otherwise a stale o/*.o would keep being linked in forever.
for o in o/*.o; do
  [ -e "$o" ] || continue
  b="$(basename "$o" .o)"
  [ -e "src/$b.c" ] || [ -e "ext/$b.c" ] || { echo "amber: dropping stale $o"; rm -f "$o"; }
done
for f in src/*.c $EXT; do "$CC" $F -o "o/$(basename "${f%.c}").o" -c "$f"; done
# link: -ldl exists on Linux; on macOS dlopen lives in libSystem, so fall back without it
"$CC" $F -o amber o/*.o -lm -ldl 2>/dev/null || "$CC" $F -o amber o/*.o -lm
echo "amber: built ./amber"
