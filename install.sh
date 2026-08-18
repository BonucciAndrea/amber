#!/usr/bin/env bash
# One-command setup for Amber.  Run with:   ./install.sh     (or: bash install.sh)
# - makes the launchers executable
# - builds the interpreter
# - runs the self-test
# - adds an  `a`  alias to your shell so you can start Amber by typing:  a
#
# Installs NOTHING system-wide, needs no root, and never touches your other
# k/q installs (kdb+, kona, ngn/k, ...).  Everything lives in this folder.
set -e
here="$(cd "$(dirname "$(readlink -f "$0")")" && pwd)"
cd "$here"
chmod +x a build.sh install.sh 2>/dev/null || true
[ -f tests/test_repl_term.py ] && chmod +x tests/test_repl_term.py 2>/dev/null || true
[ -f tests/test_ext_seam.sh ] && chmod +x tests/test_ext_seam.sh 2>/dev/null || true

echo "==> building amber ..."
bash build.sh

echo "==> running self-test ..."
./amber test.k | tail -3

rc="$HOME/.bashrc"; [ -n "${ZSH_VERSION:-}" ] && rc="$HOME/.zshrc"
line="alias a='$here/a'"
if ! grep -qxF "$line" "$rc" 2>/dev/null; then
  printf '\n# Amber\n%s\n' "$line" >> "$rc"
  echo "==> added alias to $rc"
else
  echo "==> alias already in $rc"
fi

echo
echo "Done.  Start Amber now with:   $here/a"
echo "Or open a new terminal (or 'source $rc') and just type:   a"
echo
echo "The REPL has native line editing, history (~/.amber_history) and Tab"
echo "completion built in.  Do NOT start it under rlwrap: Amber handles the"
echo "terminal itself, and rlwrap would only print a warning and fight it for"
echo "the cursor.  AMBER_NO_EDIT=1 turns the editor off if you ever need to."
echo
echo "Optional add-ons live in ext/ and lib/ and are installed by their own"
echo "repositories -- e.g. a local, offline AI co-pilot:"
echo "    git clone https://github.com/bonucciandrea/amber-ai.git"
echo "    cd amber-ai && ./install.sh $here"
