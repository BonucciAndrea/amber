#!/usr/bin/env bash
# =============================================================================
# One-command setup for Amber.
#
#     ./install.sh              # or:  bash install.sh   (if the +x bit was lost)
#
# What it does:
#   1. checks you have a C compiler, and tells you exactly how to get one if not
#   2. makes every script in the tree executable
#   3. builds the interpreter (AMBER_NATIVE=1 ./install.sh for a tuned build)
#   4. runs the self-test
#   5. adds the Amber shell block to the rc file YOUR shell actually reads
#
# Installs NOTHING system-wide, needs no root, and never touches your other
# k/q installs (kdb+, kona, ngn/k, ...). Everything lives in this folder;
# deleting the folder uninstalls Amber completely.
#
# Options:
#   --no-alias    build and test, but do not touch any shell rc file
#   --no-test     skip the self-test
#   -y, --yes     never prompt (assumed when stdin is not a terminal)
#   -h, --help    this message
#
# Amber - GNU AGPLv3 - see LICENSE and NOTICE.
# =============================================================================
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
here="$(am_scriptdir "$0")"
cd "$here"

DO_ALIAS=1; DO_TEST=1
for a in "$@"; do
  case "$a" in
    --no-alias) DO_ALIAS=0 ;;
    --no-test)  DO_TEST=0 ;;
    -y|--yes)   : ;;   # accepted for symmetry with amber-ai; this script never prompts
    -h|--help)  sed -n '2,25p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
    *) echo "install.sh: unknown option '$a' (try --help)" >&2; exit 2 ;;
  esac
done

if [ -t 1 ] && [ -z "${NO_COLOR:-}" ]; then
  B=$'\033[1m'; G=$'\033[32m'; Y=$'\033[33m'; R=$'\033[31m'; D=$'\033[2m'; Z=$'\033[0m'
else B=""; G=""; Y=""; R=""; D=""; Z=""; fi
step(){ printf '%s==>%s %s\n' "$B" "$Z" "$*"; }
ok(){   printf '    %s+%s %s\n' "$G" "$Z" "$*"; }
warn(){ printf '    %s!%s %s\n' "$Y" "$Z" "$*"; }
die(){  printf '%serror:%s %s\n' "$R" "$Z" "$*" >&2; exit 1; }
note(){ printf '    %s%s%s\n' "$D" "$*" "$Z"; }

# ---------------------------------------------------------------------------
# 0. platform + toolchain
# ---------------------------------------------------------------------------
step "checking your toolchain"

UNAME=$(uname -s 2>/dev/null || echo unknown)
IS_WSL=0
case "$(uname -r 2>/dev/null)" in *[Mm]icrosoft*|*WSL*) IS_WSL=1 ;; esac
case "$UNAME" in
  Darwin) PLATFORM="macOS" ;;
  Linux)  PLATFORM=$([ "$IS_WSL" = 1 ] && echo "WSL2" || echo "Linux") ;;
  *)      PLATFORM="$UNAME" ;;
esac
ok "platform: $PLATFORM ($(uname -m 2>/dev/null || echo '?'))"

CC_FOUND=""
for c in "${CC:-}" cc gcc clang; do
  [ -n "$c" ] || continue
  command -v "$c" >/dev/null 2>&1 && { CC_FOUND="$c"; break; }
done

if [ -z "$CC_FOUND" ]; then
  case "$PLATFORM" in
    macOS)
      die "no C compiler found.

Install Apple's Command Line Tools (this is the only step that needs a GUI
prompt; it takes a few minutes and needs no Apple ID):

    xcode-select --install

Then re-run:  ./install.sh" ;;
    WSL2|Linux)
      if command -v apt-get >/dev/null 2>&1; then
        die "no C compiler found.

    sudo apt-get update && sudo apt-get install -y build-essential

Then re-run:  ./install.sh"
      elif command -v dnf >/dev/null 2>&1; then
        die "no C compiler found.

    sudo dnf install -y gcc make

Then re-run:  ./install.sh"
      elif command -v yum >/dev/null 2>&1; then
        die "no C compiler found.

    sudo yum groupinstall -y \"Development Tools\"

Then re-run:  ./install.sh"
      elif command -v pacman >/dev/null 2>&1; then
        die "no C compiler found.

    sudo pacman -S --needed base-devel

Then re-run:  ./install.sh"
      elif command -v apk >/dev/null 2>&1; then
        die "no C compiler found.

    sudo apk add build-base

Then re-run:  ./install.sh"
      else
        die "no C compiler found. Install gcc or clang with your package manager, then re-run ./install.sh"
      fi ;;
    *)
      die "no C compiler found. Install gcc or clang, then re-run ./install.sh" ;;
  esac
fi
ok "compiler: $CC_FOUND ($("$CC_FOUND" --version 2>/dev/null | head -1))"

# `make` is NOT required (build.sh calls the compiler directly) -- say so, so
# nobody goes hunting for it after reading a generic C-project README.
command -v make >/dev/null 2>&1 || note "make is not installed and is not needed: build.sh drives the compiler directly"

# python3 is a TEST dependency only.
if command -v python3 >/dev/null 2>&1; then
  ok "python3: $(python3 --version 2>&1)  (used by the test suite only)"
else
  warn "python3 not found -- the fuzz and REPL-terminal test suites will be skipped"
fi

# ---------------------------------------------------------------------------
# 1. permissions
# ---------------------------------------------------------------------------
step "fixing permissions"
# A git clone can arrive with mode 644 on every script (the index records the
# bit, and a zip/copy or a Windows checkout can drop it), in which case `./a`
# answers the very first line of the README with "Permission denied".
FIXED=0
for f in a build.sh install.sh demo.sh tests/*.sh tests/*.py; do
  [ -f "$f" ] || continue
  [ -x "$f" ] || { chmod +x "$f" 2>/dev/null && FIXED=$((FIXED+1)); }
done
[ "$FIXED" -gt 0 ] && ok "made $FIXED script(s) executable" || ok "all scripts already executable"

# ---------------------------------------------------------------------------
# 2. build
# ---------------------------------------------------------------------------
step "building amber"
if [ -n "${AMBER_NATIVE:-}" ]; then
  note "AMBER_NATIVE=1 -- machine-tuned build (the binary will only run on this CPU family)"
fi
bash build.sh || die "build failed.

The compiler's own error is above. If it mentions a missing header, install
the toolchain for your platform (see --help), then re-run ./install.sh"
ok "built $here/amber  ($(./amber --version 2>/dev/null))"

# ---------------------------------------------------------------------------
# 3. self-test
# ---------------------------------------------------------------------------
if [ "$DO_TEST" = 1 ]; then
  step "running the self-test"
  if ./amber test.k 2>/dev/null | tail -3 | grep -q "0 failures"; then
    ok "test.k: 0 failures"
  else
    warn "test.k did not report 0 failures -- run  ./amber test.k  to see why"
  fi
fi

# ---------------------------------------------------------------------------
# 4. shell integration
# ---------------------------------------------------------------------------
# Pick the rc file the user's LOGIN shell actually reads, not the one this
# script happens to be running under: `bash install.sh` from a zsh session used
# to append to ~/.bashrc, which zsh never sources, so the alias silently never
# appeared.
pick_rc() {
  sh_name=$(basename "${SHELL:-/bin/sh}")
  case "$sh_name" in
    zsh)  printf '%s\n' "${ZDOTDIR:-$HOME}/.zshrc" ;;
    bash)
      # macOS Terminal starts LOGIN shells, which read .bash_profile and NOT
      # .bashrc; most Linux terminals do the opposite.
      if [ "$PLATFORM" = "macOS" ] && [ -f "$HOME/.bash_profile" ]; then
        printf '%s\n' "$HOME/.bash_profile"
      else
        printf '%s\n' "$HOME/.bashrc"
      fi ;;
    ksh)  printf '%s\n' "$HOME/.kshrc" ;;
    fish) printf '%s\n' "$HOME/.config/fish/config.fish" ;;
    *)    printf '%s\n' "$HOME/.profile" ;;
  esac
}

MARK_BEGIN="# >>> amber >>>"
MARK_END="# <<< amber <<<"

if [ "$DO_ALIAS" = 1 ]; then
  step "shell integration"
  rc="$(pick_rc)"
  sh_name=$(basename "${SHELL:-/bin/sh}")

  if [ "$sh_name" = "fish" ]; then
    warn "fish detected -- fish syntax differs; add this to $rc by hand:"
    note "set -gx AMBER_HOME \"$here\""
    note "alias amber='env AMBER_NATIVE=1 \"$here/a\"'"
  else
    mkdir -p "$(dirname "$rc")" 2>/dev/null || true
    if grep -qF "$MARK_BEGIN" "$rc" 2>/dev/null; then
      # Replace the existing block rather than appending a second copy.
      tmp="$rc.amber.$$"
      awk -v b="$MARK_BEGIN" -v e="$MARK_END" '
        $0==b {skip=1} skip==0 {print} $0==e {skip=0}' "$rc" > "$tmp" && mv "$tmp" "$rc"
      ok "replaced the existing Amber block in $rc"
    else
      ok "adding the Amber block to $rc"
    fi

    {
      printf '\n%s\n' "$MARK_BEGIN"
      cat <<EOF
# === Amber - native engine configuration ===================================
# Managed by install.sh. Edit freely; re-running install.sh replaces this block.
#
# AMBER_HOME is the Amber checkout itself: Amber is one self-contained folder
# with no bin/ directory and nothing installed system-wide.
export AMBER_HOME="$here"

# amber  -> the full REPL: loads repl.k, the q/kdb+ vocabulary and the stdlib.
# AMBER_NATIVE is read by build.sh, which ./a re-runs whenever the sources are
# newer than the binary -- so the first run after a git pull rebuilds with
# -march=native (or -mcpu=native on Apple Silicon / aarch64).
alias amber='AMBER_NATIVE=1 "\$AMBER_HOME/a"'

# amberx -> the bare interpreter for scripts and pipes: no REPL, no stdlib.
alias amberx='"\$AMBER_HOME/amber"'

# Pin the vector engine to your physical cores; omit to use every core.
# alias amber='AMBER_NATIVE=1 AMBER_THREADS=8 "\$AMBER_HOME/a"'

# amber-ai -> the same REPL with the local AI co-pilot pointed at your model
# server and given a longer answer budget. Requires the amber-ai extension;
# without it these variables are simply ignored. lib/ai.k is loaded for you by
# repl.k via lib/ext.k -- do NOT pass it as a script argument.
alias amber-ai='AMBER_NATIVE=1 AMBER_AI=1 AMBER_AI_URL="http://127.0.0.1:11434/api/generate" AMBER_AI_TIMEOUT_MS=10000 "\$AMBER_HOME/a"'

# Aliases do not exist in non-interactive shells. For scripts, cron and CI,
# put the launcher on PATH instead of adding the repo root to PATH (which would
# also expose install.sh and demo.sh as commands):
#     mkdir -p ~/.local/bin && ln -sf "\$AMBER_HOME/a" ~/.local/bin/amber
EOF
      printf '%s\n' "$MARK_END"
    } >> "$rc"

    ok "wrote the Amber block to $rc"
    note "shell detected: $sh_name    rc file: $rc"
  fi
fi

# ---------------------------------------------------------------------------
# 5. done
# ---------------------------------------------------------------------------
echo
printf '%sAmber is ready.%s\n' "$B" "$Z"
echo
echo "start it now:"
echo "    $here/a"
if [ "$DO_ALIAS" = 1 ] && [ "$(basename "${SHELL:-sh}")" != "fish" ]; then
echo
echo "or open a new terminal (or: source $(pick_rc)) and just type:"
echo "    amber"
fi
echo
echo "The REPL has native line editing, history (~/.amber_history) and Tab"
echo "completion built in. Do NOT start it under rlwrap: Amber handles the"
echo "terminal itself. If you do anyway, it detects rlwrap and stands down"
echo "quietly rather than fighting it for the cursor."
echo
echo "Optional local AI co-pilot (separate repository, installs into this one):"
echo "    git clone https://github.com/bonucciandrea/amber-ai.git"
echo "    cd amber-ai && ./install.sh $here"
