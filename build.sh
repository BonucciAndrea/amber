#!/usr/bin/env bash
# Build the Amber interpreter (./amber) from the C sources in this folder.
# Portable flags (no -march=native, warnings silenced) so the binary runs on
# any x86-64 / arm64 host with a C compiler. Never installs anything system-wide.
set -e
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
cd "$(am_scriptdir "$0")"
CC="${CC:-}"
if [ -z "$CC" ]; then
  for c in cc gcc clang; do command -v "$c" >/dev/null 2>&1 && { CC="$c"; break; }; done
fi
if [ -z "$CC" ]; then
  echo "No C compiler found. On Ubuntu:  sudo apt-get install build-essential" >&2
  exit 1
fi
# ---- options --------------------------------------------------------------
# ./build.sh              build ./amber only (the default; unchanged)
# ./build.sh --shared     ALSO build libamber.so -- the dynamic C API seam that
#                         every out-of-tree satellite (python-amber, amber-arrow,
#                         amber-jupyter, vscode-amber's LSP, the Grafana backend,
#                         amber-flame) links against.  See src/ext.h section 6.
# ./build.sh --shared-only  build ONLY libamber.so and skip the executable.
# AMBER_SHARED=1 is equivalent to --shared, for callers that cannot pass a flag.
#
# The shared library is a SEPARATE object set (-fPIC -DAMBER_SHARED), never a relink of
# o/*.o: position-independent code and the global-dynamic TLS model the shared
# build needs (see the AM_TLS_IE note in src/a.h) both change code generation, so
# sharing objects between the two would silently pessimise ./amber -- which is
# the one thing this repository will not trade away.
WANT_SHARED=${AMBER_SHARED:+1}
WANT_EXE=1
for arg in "$@"; do
  case "$arg" in
    --shared)      WANT_SHARED=1 ;;
    --shared-only) WANT_SHARED=1; WANT_EXE=0 ;;
    -h|--help)
      sed -n '/^# ---- options/,/^WANT_SHARED/p' "$0" | sed 's/^# \{0,1\}//;$d'
      exit 0 ;;
    *) echo "amber: unknown build option '$arg' (try --help)" >&2; exit 2 ;;
  esac
done

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
# AMBER_NATIVE=1 asks for a machine-specific build. The tuning flag is NOT
# portable: -march=native is x86 syntax that Apple clang REJECTS outright on
# Apple Silicon ("the clang compiler does not support '-march=native'"), where
# the equivalent is -mcpu=native. GCC on aarch64 accepts -mcpu=native too. So
# probe rather than assume -- otherwise `AMBER_NATIVE=1 ./build.sh` in CI, which
# is exactly what the documented macOS build line does, fails on every arm64
# runner. If neither flag is accepted the build still succeeds, just portable.
if [ -n "${AMBER_NATIVE:-}" ]; then
  NATFLAG=""
  for cand in -march=native -mcpu=native; do
    if printf 'int main(){return 0;}' | "$CC" $cand -x c - -o .natcheck 2>/dev/null; then
      NATFLAG="$cand"; break
    fi
  done
  rm -f .natcheck
  if [ -n "$NATFLAG" ]; then
    # -DAMBER_BUILD_NATIVE makes the REPL's [native]/[portable] tag AUTHORITATIVE.
    # Inferring it from the SIMD backend name was wrong on arm64 (NEON is the
    # baseline, so a portable build reported "native") -- see sb_build_kind().
    F="$F $NATFLAG -funroll-loops -DAMBER_BUILD_NATIVE=1"; MODE="native $NATFLAG -O3$LTOTAG$OMPTAG"
  else
    MODE="portable -O3$LTOTAG$OMPTAG (no native tuning flag accepted by $CC)"
  fi
else
  MODE="portable -O3$LTOTAG$OMPTAG"
fi

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

if [ "$WANT_EXE" = 1 ]; then
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
# The linker already marks ./amber executable, but ./a is a TRACKED shell script
# and a checkout (or a zip, or a tar of a checkout) can arrive at mode 644. The
# test harness then dies with "PermissionError: [Errno 13] ... /a" long after the
# build reported success, which reads as a broken engine rather than a lost bit.
# Assert both here, where the cost is one syscall; `|| true` because a read-only
# or foreign-owned tree must still be buildable.
chmod +x amber a 2>/dev/null || true
echo "amber: built ./amber"
fi

# ---- shared library -------------------------------------------------------
# libamber.so is the out-of-process seam: the same sources, compiled -fPIC with
# -DAMBER_SHARED (which drops main() -- src/0.c already guards it on exactly that
# macro, and has since long before this build mode existed) and linked behind an
# export map so the ONLY symbols that reach a host process's dynamic namespace
# are amber_* and am_ext_*.  See src/libamber.map for why that matters.
if [ -n "${WANT_SHARED:-}" ]; then
  # -soname/-install_name matter more here than they look. A satellite that
  # dlopen()s a SECOND copy of libamber.so gets a second engine: two heaps, two
  # symbol tables, two global namespaces, and values from one that are garbage
  # to the other -- with no error message, because nothing is technically wrong.
  # Recording a SONAME means the loader recognises an already-loaded libamber.so
  # and reuses it, so python-amber, libamber_arrow.so and any plugin in the same
  # process all share ONE engine, which is the only arrangement that makes sense.
  SOEXT=so; SOFLAGS="-shared -Wl,-soname,libamber.so"
  case "$(uname -s 2>/dev/null)" in
    Darwin) SOEXT=dylib; SOFLAGS="-dynamiclib -install_name @rpath/libamber.dylib" ;;
  esac
  SOFILE="libamber.$SOEXT"
  # Probe the export-map flag rather than assuming it: GNU ld and lld take
  # --version-script, Apple's ld64 takes -exported_symbols_list with a different
  # file format, and a toolchain that takes neither must still produce a working
  # (if wider-than-ideal) library rather than failing the build.
  MAPFLAG=""
  if printf 'int amber_probe(void){return 0;}' | \
     "$CC" -fPIC -shared -x c - -o .mapcheck.$SOEXT -Wl,--version-script=src/libamber.map 2>/dev/null; then
    MAPFLAG="-Wl,--version-script=src/libamber.map"
  elif [ "$SOEXT" = dylib ]; then
    printf '_amber_*\n_am_ext_*\n_am_repl_take_accepted\n' > o/libamber.syms 2>/dev/null || true
    if printf 'int amber_probe(void){return 0;}' | \
       "$CC" -fPIC -dynamiclib -x c - -o .mapcheck.$SOEXT -Wl,-exported_symbols_list,o/libamber.syms 2>/dev/null; then
      MAPFLAG="-Wl,-exported_symbols_list,o/libamber.syms"
    fi
  fi
  rm -f .mapcheck.$SOEXT
  [ -n "$MAPFLAG" ] || echo "amber: note - this linker takes no export map; libamber.$SOEXT will export its internal symbols too" >&2

  echo "amber: compiling $SOFILE with $CC (-fPIC -DAMBER_SHARED)$EXTTAG ..."
  mkdir -p o/pic
  for o in o/pic/*.o; do
    [ -e "$o" ] || continue
    b="$(basename "$o" .o)"
    [ -e "src/$b.c" ] || [ -e "ext/$b.c" ] || rm -f "$o"
  done
  for f in src/*.c $EXT; do
    "$CC" $F -fPIC -fvisibility=hidden -DAMBER_SHARED -o "o/pic/$(basename "${f%.c}").o" -c "$f"
  done
  "$CC" $F -fPIC $SOFLAGS $MAPFLAG -o "$SOFILE" o/pic/*.o -lm -ldl 2>/dev/null \
    || "$CC" $F -fPIC $SOFLAGS $MAPFLAG -o "$SOFILE" o/pic/*.o -lm
  echo "amber: built ./$SOFILE"
fi
