#!/usr/bin/env bash
# demo.sh  -  one-command Amber Mega Demo runner.
#
# Builds Amber with maximum optimization, runs the two showcase scripts in
# demo/, prints a couple of \ast / \disasm samples straight from the real
# compiler/VM, and tells you how to open the interactive web notebook.
#
# Works on Linux and macOS (build.sh already falls back off -ldl on macOS;
# this script only uses portable POSIX shell + coreutils).
#
#   ./demo.sh            portable -O3 build (safe to redistribute)
#   AMBER_NATIVE=1 ./demo.sh   -march=native build (fastest on THIS machine)
set -e
cd "$(dirname "$(readlink -f "$0" 2>/dev/null || echo "$0")")"

BOLD=$(tput bold 2>/dev/null || true)
DIM=$(tput dim 2>/dev/null || true)
CYAN=$(tput setaf 6 2>/dev/null || true)
RESET=$(tput sgr0 2>/dev/null || true)

hr() { echo; echo "${CYAN}== $1 $(printf '=%.0s' $(seq 1 $((60 - ${#1}))))${RESET}"; }

hr "1. build (maximum optimization)"
if [ -n "${AMBER_NATIVE:-}" ]; then
  echo "AMBER_NATIVE=1 -> machine-specific -O3 -march=native build"
else
  echo "portable -O3 (+LTO where supported) build -- set AMBER_NATIVE=1 for -march=native"
fi
./build.sh
echo

hr "2. HFT financial tick demo (demo/hft_demo.k)"
echo "${DIM}(edit the N: line in demo/hft_demo.k to change the row count --"
echo " the default is 5,000,000 trades / 10,000,000 quotes)${RESET}"
echo
./amber demo/hft_demo.k

hr "3. engine speed & parallelism showcase (demo/bench_showcase.k)"
echo "${DIM}(default N: 10,000,000 elements per round; set AMBER_THREADS=N to"
echo " change the peach worker count, default: online CPU count)${RESET}"
echo
./amber demo/bench_showcase.k

hr "4. \\ast -- the real parser's AST, with hooks/forks/projections labeled"
echo '\ast (+;-)@3        / a 2-verb train is an explicit Hook'      | ./amber
echo
echo '\ast (*;+;%)[2;3;4] / a 3-verb train is an explicit Fork'      | ./amber
echo

hr "5. \\disasm -- real compiled VM bytecode, not a toy"
echo '\disasm {[a;b]a+b*2}' | ./amber

hr "6. try the interactive web notebook"
cat <<'EOF'
Amber also ships a zero-install, in-browser K notebook -- open it directly
in any modern browser, no server or build step needed:

    notebooks/Amber-Notebook-Studio.html   (gallery-card UI, richer charts)
    notebooks/Amber-Notebook-Classic.html  (sidebar-list UI)
    notebooks/Amber-Notebook-Console.html  (minimal single-pane console)

On Linux:   xdg-open notebooks/Amber-Notebook-Studio.html
On macOS:   open notebooks/Amber-Notebook-Studio.html

These notebooks are a standalone JS re-implementation of the K language for
zero-install browser use, so they run independently of the ./amber binary
you just built -- see amber-notepad (a separate project) for a notebook
backed by the real compiled engine via WebAssembly.
EOF

hr "done"
echo "Mega Demo complete. See README.md's Quick Showcase section for more."
