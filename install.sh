#!/usr/bin/env bash
# One-time setup for Amber.  Run with:   bash install.sh
# - makes the launchers executable
# - builds the interpreter
# - adds an  `a`  alias to your shell so you can start Amber by typing:  a
# Installs NOTHING system-wide and never touches your other k/q installs (kdb+, kona, ...).
set -e
here="$(cd "$(dirname "$(readlink -f "$0")")" && pwd)"
cd "$here"
chmod +x a build.sh
echo "==> building amber ..."
bash build.sh
echo "==> running self-test ..."
./amber test.k | tail -3
rc="$HOME/.bashrc"; [ -n "$ZSH_VERSION" ] && rc="$HOME/.zshrc"
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
