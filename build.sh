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
# Portable -O3 (+ LTO where supported) by default. Set AMBER_NATIVE=1 for a faster
# machine-specific build (adds -march=native -funroll-loops; the binary then only runs
# on this CPU family).
F="-fsigned-char -fno-math-errno -fno-signed-zeros -fno-stack-protector -fomit-frame-pointer -w -O3 -pthread"
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
echo "amber: compiling with $CC ($MODE) ..."
mkdir -p o
for f in src/*.c; do "$CC" $F -o "o/$(basename "${f%.c}").o" -c "$f"; done
# link: -ldl exists on Linux; on macOS dlopen lives in libSystem, so fall back without it
"$CC" $F -o amber o/*.o -lm -ldl 2>/dev/null || "$CC" $F -o amber o/*.o -lm
echo "amber: built ./amber"
