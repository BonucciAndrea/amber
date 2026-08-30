<div align="center">

```
   █████╗ ███╗   ███╗██████╗ ███████╗██████╗
  ██╔══██╗████╗ ████║██╔══██╗██╔════╝██╔══██╗
  ███████║██╔████╔██║██████╔╝█████╗  ██████╔╝
  ██╔══██║██║╚██╔╝██║██╔══██╗██╔══╝  ██╔══██╗
  ██║  ██║██║ ╚═╝ ██║██████╔╝███████╗██║  ██║
  ╚═╝  ╚═╝╚═╝     ╚═╝╚═════╝ ╚══════╝╚═╝  ╚═╝
```

**A low-latency array language — columnar, vectorised, in-memory.**

![ci](https://github.com/BonucciAndrea/amber/actions/workflows/ci.yml/badge.svg)
![version](https://img.shields.io/badge/version-2.0.0-orange)
![license](https://img.shields.io/badge/license-AGPLv3-blue)
![tests](https://img.shields.io/badge/tests-742%20K--suite%20cases-brightgreen)
![build](https://img.shields.io/badge/build-C99%20·%20portable%20·%20gcc%20+%20clang-informational)

</div>

## What is Amber

Amber is a small, fast, self-contained array language with the working vocabulary of
**q/kdb+** — dictionaries, **tables & keyed tables** with `([]…)` literal syntax, the full
**join family** (left · inner · union · plus · equi · **as-of** · **window**), qSQL-style
select/by, strings, intraday **tick / OHLC** temporals, and **column attributes implemented
in C** that turn search from `O(n)` into `O(log n)` — **~1000–2000× faster** on large data.

Amber's interpreter core is built on **[ngn/k](https://codeberg.org/ngn/k)** — ngn's compact,
AGPLv3 implementation of the K array language. Amber keeps that engine's speed and small
footprint and layers a q/kdb+ vocabulary, C-level column attributes, native temporal types,
`([]…)` table syntax, a tick/HFT toolkit, and a modern REPL on top. (The attribution is
recorded in [NOTICE](NOTICE), as the AGPLv3 requires.)

```q
t:([]sym:`AAPL`MSFT`AAPL; px:187.3 411.2 187.4; sz:100 250 50)   / a table, rendered instantly
qby[t; `sym; (,`vwap)!,{wavg[x`sz;x`px]}]                        / vwap by symbol
```

<a name="whats-new"></a>
## Since 2.0.0 — on `main`, not yet tagged

REPL correctness work, all covered by `tests/test_statusbar.py`:

- **The line editor counts cells, not bytes.** `città però` used to push the input box's border
  two columns in and **Backspace deleted one *byte* of an accented letter**, leaving a broken
  sequence that was then handed to the parser. CJK and emoji broke it the other way. Layout and
  editing are now character- and cell-correct, with a built-in width table rather than
  `wcwidth()` — the latter answers per `LC_CTYPE`, so the same line would lay out differently on
  macOS and WSL.
- **Multi-line input.** A line leaving `{`, `(` or `[` open now continues on a `...>` prompt until
  the brackets balance, and is evaluated — and stored in history — as one statement. There is no
  key binding for it: Shift-Enter is indistinguishable from Enter in a terminal, so continuing on
  incomplete syntax (as python, node, ghci and q do) is the only portable mechanism.
- **The status-bar exec timer was reporting 10x.** k has no bare `.1` float literal — `.1` lexes
  as the verb `.` applied to `1` — so `.1*_0.5+10*sblast` was ten times the milliseconds. A 23 ms
  line read `230 ms`. `fmt` also forked `tput` on *every* line for the terminal size (3–6 ms each
  on macOS); it now comes from `ioctl` via `` `bi 0``. `1+1` went 62 ms → 0.053 ms.
- **Terminal robustness** — zoom/resize no longer strands a duplicate box, the box spans terminals
  wider than ~313 columns, `Ctrl-L` is byte-identical to `\clear`, and the paste path checks every
  allocation.
- **Engine** — `az()` computed `n-(I)n` to test whether a value fits in an `int`; for `0W`
  (`LLONG_MAX`) that overflows, which is undefined behaviour. Fixed.

Full detail in **[CHANGELOG.md](CHANGELOG.md)**.

<a name="whats-new-200"></a>
## What's new in 2.0.0

- **Infix notation for the two-argument library dyads.** `2 3 in 2 3 4`, `5 within 3 9`,
  `` t lj kt``, `` `sym xasc t``, `` "/" sv `a`b`c`` now work infix, exactly as in kdb+/q. The
  bracket form (`f[x;y]`) and prefix form (`f x`) still work unchanged. The infix set is a curated
  list — `in within like lj ij uj aj aj0 wj wj1 pj ej cross inter union except ss sv vs xasc xdesc`
  — plus the built-in symbol verbs; arbitrary user lambdas are **not** infix.
- **Bare qSQL now works inside a loaded `.k` script**, not just at the prompt. `select … by … from
  … where …` (and `exec`/`update`/`delete`) no longer needs a `sel"…"` wrapper in a file: the C
  loader runs every script through the same qSQL rewriter the REPL uses, once the stdlib is up.
- **Fix — `&()` (where on a literal empty generic list)** now returns `!0` correctly, instead of a
  spurious 1-element `,!0`. This had been silently affecting `ss` and any qSQL path that built a
  literal empty `()`.
- **Fix — an unterminated bare-`/` block comment** now raises a clean parse error instead of
  silently truncating the rest of the file at EOF.
- Right-to-left evaluation of a function's bracketed arguments is now pinned by regression tests, so
  a parser change can't silently flip it.

Full history in **[CHANGELOG.md](CHANGELOG.md)**.

## Table of contents

- [Quick showcase](#quick-showcase)
- [Download & install](#download--install)
- [A quick taste](#a-quick-taste)
- [REPL line editing](#repl-line-editing)
- [Architecture](#architecture) · [Extensions (`src/ext.h` + `ext/`)](#extensions)
- [Rust-style diagnostics](#rust-style-diagnostics)
- [REPL diagnostics (`\v` · `\ast` · `\trace`)](#repl-diagnostics-v--ast--trace)
- [Engine extensions: SIMD · parallel · disassembler · CSV](#engine-extensions)
- [HFT toolkit — native `aj`, arena, generators](#hft-toolkit)
- [Terminal charts (`plot` · `candle`)](#terminal-charts)
- [Grid modes (`\grid`)](#grid-modes)
- [Native temporal types](#native-temporal-types)
- [Apache Arrow C Data Interface](#apache-arrow)
- [`libamber.so` — the dynamic C API seam](#libamber)
- [The satellite ecosystem](#satellite-ecosystem)
- [Comparative benchmark query files](#comparative-benchmark-query-files)
- [Why attributes matter](#why-attributes-matter)
- [Language notes](#language-notes)
- [Finance / HFT module (`fin.k`)](#finance-module)
- [What's inside](#whats-inside) · [Roadmap](#roadmap) · [Isolation](#isolation) · [Licence](#licence)

---

<a name="quick-showcase"></a>
## Quick showcase

One command builds Amber with maximum optimization and runs the full **Mega Demo**: a
5,000,000-row HFT tick session (VWAP, a tacit 50-period EMA, a native as-of join), a
10,000,000-element vector-op benchmark (scalar vs native vs `peach`), a couple of `\ast`/
`\disasm` samples straight from the real parser/compiler, and pointers to the in-browser
notebook.

```sh
./demo.sh                    # portable -O3 build
AMBER_NATIVE=1 ./demo.sh     # -march=native build (fastest on this machine)
```

`demo/hft_demo.k` — a realistic multi-symbol tick session end to end:

```q
\l amber.k
\l fin.k
N:1000000                                                  / trade count (gentq builds N trades + 2N quotes)
gentq N                                                     / sets globals `trades` and `quotes`, realistic microstructure
qby[trades; `sym; (,`vwap)!,{round[4]wavg[x`sz;x`px]}]      / VWAP per symbol
ema[2%51; trades`px]                                        / 50-period EMA, tacit call into the C kernel
taq[trades; quotes]                                         / as-of join: every trade -> its nearest quote
```

<details>
<summary>Sample benchmark output from a real run (timings vary by machine)</summary>

`demo/hft_demo.k` prints its own summary table every run:

```
== benchmark summary =======================================
stage      ms
----------
gentq   2381.9
vwap     151.2
ema        0.1
asof     704.0
------------------------------------------------------------
total: ~3.2 s end-to-end for 500,000 trades
```

`demo/bench_showcase.k` — the same 10,000,000-element vector three ways, with a printed
speedup table:

```
op   method   ms
--------------
add  scalar   ███████████████████░  (per-element `'` each)
add  vector   ██░                   (native C kernel, auto-SIMD)   ~2-9x faster
mul  scalar   ██████████████░
mul  vector   █░                                                    ~2-9x faster
sum  serial   ░
sum  peach    ███░   (fork/IPC overhead dominates a cheap reduction --
                       peach earns its keep on per-task-heavy work, see examples/peach.k)
```

</details>

Prefer to explore interactively? Open `notebooks/Amber-Notebook-Studio.html` directly in a
browser (no build step) and hit **🚀 Load HFT Demo** in the header — it generates a tick
session, charts price & volume on a canvas, and benchmarks a naive per-symbol filter against
the vectorised `qby` call, right there in the page.

---

<a name="download--install"></a>
## Download & install

Amber compiles from source (portable **C99**, builds clean on `gcc` and `clang`) on first run —
nothing is installed system-wide. Clone the repo:

```sh
git clone https://github.com/BonucciAndrea/amber.git
cd amber
```

**Linux / WSL** — needs a C compiler (`gcc` or `clang`):

```sh
sudo apt-get update && sudo apt-get install -y build-essential   # one-time
chmod +x a build.sh install.sh                                    # restore exec bits if the copy dropped them
./a                                                               # builds, then opens the REPL
# note: do NOT install or use rlwrap for Amber -- line editing is built in
```

**macOS** (Intel or Apple Silicon) — needs Apple's `clang`:

```sh
xcode-select --install        # installs the Command Line Tools (clang); one-time
chmod +x a build.sh install.sh
./a                           # line editing / history / Tab are built in -- no rlwrap
```

That's it — `./a` compiles the interpreter (portable `-O3`, no `-march=native`) and drops you at
the prompt; it recompiles automatically whenever the C sources change, so you never run a stale
build. If `./a` prints **`Permission denied`**, the executable bit was lost in transfer — the
`chmod +x` line above fixes it (or just run `bash a`).

**Machine-tuned build.** The default build is portable C99 and always includes `-pthread` (needed
by the multithreaded vector engine) and the scalar SIMD fallback. For a build that turns on
**AVX2** (x86_64) or **NEON** (Apple Silicon / any `aarch64`) vector kernels, set `AMBER_NATIVE=1`:

```sh
AMBER_NATIVE=1 ./build.sh      # machine-tuned; check with: `simd 0
```

`` `simd 0 `` prints which backend actually got selected (`scalar` / `avx2` / `neon`) to stderr.
NEON activates unconditionally on Apple Silicon regardless of `AMBER_NATIVE`, since `aarch64`
implies it. The tuning flag is **probed, not assumed**: `-march=native` is x86 syntax that Apple
clang rejects on Apple Silicon, so `build.sh` falls back to `-mcpu=native` (aarch64) and, failing
both, to a portable build — so `AMBER_NATIVE=1 ./build.sh` succeeds on every platform rather than
breaking CI on arm64 runners.

**One command instead of all of the above:**

```sh
./install.sh                 # or: bash install.sh   (if the +x bit was lost)
AMBER_NATIVE=1 ./install.sh  # ... with a machine-tuned build
```

`install.sh` checks you have a C compiler (and prints the exact package command for your distro
if you do not), repairs the executable bit on every script, builds, runs the self-test, and writes
the shell block below into the rc file **your login shell actually reads** — `~/.zshrc` for zsh,
`~/.bash_profile` on macOS bash, `~/.bashrc` on Linux bash, `~/.profile` otherwise. Re-running it
replaces that block rather than appending a second copy.

<details>
<summary>The shell block, to add by hand</summary>

<a name="shell-configuration"></a>
```sh
# === Amber - native engine configuration ===================================
# AMBER_HOME is the Amber checkout ITSELF. Amber is one self-contained folder:
# there is no bin/ directory, nothing is copied anywhere, and deleting the
# folder uninstalls it completely.
export AMBER_HOME="$HOME/amber"

# amber  -> the full REPL: repl.k, the q/kdb+ vocabulary and the stdlib.
# AMBER_NATIVE is read by build.sh, which ./a re-runs whenever the sources are
# newer than the binary -- so the first run after a git pull rebuilds with
# -march=native (or -mcpu=native on Apple Silicon / aarch64).
alias amber='AMBER_NATIVE=1 "$AMBER_HOME/a"'

# amberx -> the bare interpreter for scripts and pipes: no REPL, no stdlib.
alias amberx='"$AMBER_HOME/amber"'

# Pin the vector engine to your physical cores; omit to use every core.
# alias amber='AMBER_NATIVE=1 AMBER_THREADS=8 "$AMBER_HOME/a"'

# amber-ai -> the same REPL with the local AI co-pilot pointed at your model
# server and given a longer answer budget. Needs the amber-ai extension;
# without it these variables are simply ignored.
alias amber-ai='AMBER_NATIVE=1 AMBER_AI=1 AMBER_AI_URL="http://127.0.0.1:11434/api/generate" AMBER_AI_TIMEOUT_MS=10000 "$AMBER_HOME/a"'

# Aliases do not exist in non-interactive shells. For scripts, cron and CI,
# symlink the launcher instead of putting the repo root on PATH (which would
# also expose install.sh and demo.sh as commands):
#     mkdir -p ~/.local/bin && ln -sf "$AMBER_HOME/a" ~/.local/bin/amber
```

</details>

Three notes on that block, because the obvious-looking variants do not work:

| pitfall | why |
|---|---|
| **`$AMBER_HOME` is the checkout, not a prefix** | Amber has no `bin/`, `lib/` or `share/` split and installs nothing outside its folder. `export PATH="$AMBER_HOME/bin:$PATH"` points at a directory that does not exist. |
| **Alias the launcher `a`, not the binary `amber`** | The bare `amber` binary is the interpreter with **no** stdlib: `amber` alone gives you a REPL where `select`, `aj` and `sum` are undefined. `./a` loads `repl.k`, which loads everything else. |
| **`AMBER_NATIVE` is a *build*-time variable** | It is read by `build.sh`, not by the interpreter. It belongs on the `a` alias — which may rebuild — and does nothing on `amberx`. There is no `AMBER_MEM_MB`: the heap is `mmap`'d with `MAP_NORESERVE` and sized lazily by the OS, so there is nothing to tune. |

The variables the engine itself reads at run time are exactly: `AMBER_THREADS` (vector-engine
lanes), `AMBER_DIAG` (rich diagnostics on/off), `AMBER_NO_EDIT` and `AMBER_RLWRAP` (line editor),
plus `AMBER_AI_*` once the [amber-ai](https://github.com/bonucciandrea/amber-ai) extension is
installed. Nothing is installed system-wide — see [Isolation](#isolation).

---

<a name="a-quick-taste"></a>
## A quick taste

> These snippets are written as you'd type them at the interactive prompt (`./a`), where a bare
> table auto-renders as a grid and qSQL sugar (`select … by … from … where …`) works directly on
> the input line. **Since 2.0.0 bare qSQL also works inside a `.k` script** run via `./amber
> file.k` — the loader runs each file through the same rewriter the REPL uses — so the `sel"…"`
> wrapper is no longer required in files (it still works). For the grid view of a bare table inside
> a script, wrap it in `show` (`show t`).

```q
/ tables are first-class and render without `show` -- at the interactive prompt
([]sym:`a`b`c; px:100 200 300)
/  sym px
/  -------
/  a   100
/  b   200
/  c   300

meta ([]sym:`a`b; px:1.5 2.5)      / column types + attributes (c | t a)

/ the join every tick shop needs — as-of (native C kernel)
trade:([]sym:`a`b`a; time:3 4 9; px:100 200 300; sz:10 20 30)
quote:([]sym:`a`a`b`a; time:1 5 2 8; bid:10 11 20 12)
aj[`sym`time; trade; quote]        / last quote at/ before each trade

/ qSQL — type select/exec/update/delete straight, no sel"…" wrapper
select last px by sym from trade   / grouped aggregate
select from trade where px>150     / filter rows

/ two-argument library dyads work infix now, identical to the bracket form
2 3 9 in 2 3 4                     / 1 1 0        (same as in[2 3 9;2 3 4])
`sym xasc trade                    / sort by sym  (same as xasc[`sym;trade])

/ 1-minute OHLCV bars (classic tickerplant query)
tb:+@[+trade; ,`time; minbar[1]@]
qby[tb; `sym`time; `o`h`l`c`v!({first x`px};{max x`px};{min x`px};{last x`px};{sum x`sz})]

/ sorted attribute => binary-search lookups
v:asc 2000000?1000000000                     / `s attribute set by asc
`at v                                        / `s
v ? 12345 67890                              / O(log n)  (see bench.k: ~1000x faster)

/ binary serialization: -8! encodes any value to bytes, -9! decodes it back
b:-8!+`a`b!(1 2 3;4 5 6)     / table -> compact byte vector
(-9!b)~+`a`b!(1 2 3;4 5 6)   / 1b  -- exact, attributes and nulls included

/ multi-core: peach runs f over items in parallel worker processes (no GIL)
peach[{avg x?1.0}; 8#1000000]                / 8 heavy tasks across AMBER_THREADS cores
```

Big tables print Q-style — the first `CROWS` rows (default 20) then `..`; set `CROWS:10`
to shorten. The cap is applied before formatting, so previewing a million-row table is
instant.

Check the interpreter version, or list every option and REPL command:

```sh
./amber --version           # amber 2.0.0
./amber --help              # options + the full \-command reference
```

Run the guided tours:

```sh
./amber examples/tour.k     # a worked example of EVERY function
./amber examples/basics.k   # a 2-minute intro
./amber examples/tick.k     # realistic trades & quotes: as-of/window joins, VWAP, OHLC
AMBER_THREADS=8 ./amber examples/peach.k   # multi-core speedup demo (serial vs peach)
./amber bench.k             # attribute speed benchmark
./amber test.k              # core suite (202); also test-fin.k (35) + test-ext.k (79)
tests/run_tests.sh          # EVERYTHING: build + all K suites + C unit tests + fuzz pass
./amber tests/test_matrix.k # the suites are self-locating - run them from any directory
tests/run_tests.sh --asan   # ... and re-run it all under AddressSanitizer + UBSan
bash bench/run.sh           # cross-engine sanity + speed (Amber vs numpy/pandas/…; see BENCHMARKS.md)
python3 bench/scout/scout.py     # the widest run: 23 ops x 14 engines (kdb+/q, PeachQ, ngn/k,
                                 #   CBQN, J, NumPy, pandas, Polars, DuckDB, a C reference)
python3 bench/scout/webgen.py --md   # regenerate the tables in docs/BENCHMARKS.md from results.json
```

---

<a name="repl-line-editing"></a>
## REPL line editing (native — do not use `rlwrap`)

Amber's REPL edits your line itself. `src/ln.c` is a single-file editor (~1,650 lines of C99 + POSIX
`termios`) in the [linenoise](https://github.com/antirez/linenoise) tradition: raw `termios`, one
visible line, ANSI refresh. No readline, no curses, no terminfo, and nothing allocated on the
keystroke path beyond the line buffer. **`rlwrap` is no longer needed, and must no longer be used.**

| key | |
|---|---|
| `←` `→`, `Ctrl-B` / `Ctrl-F` | move by character — a *character*, not a byte: `à`, `日` and `😀` step as one |
| `Ctrl-A` / `Ctrl-E`, `Home` / `End` | start / end of line |
| `↑` `↓`, `Ctrl-P` / `Ctrl-N` | history (persisted in `~/.amber_history`) |
| `Backspace`, `Del`, `Ctrl-W` / `Ctrl-U` / `Ctrl-K` | delete char / word / to start / to end |
| `Ctrl-L` | clear screen — byte-identical to `\clear` |
| `Ctrl-C` · `Ctrl-D` | abandon the line (or a continuation) · exit on an empty line |
| `PgUp` / `PgDn`, wheel | scroll the transcript while the status bar stays locked |
| `\sb` | toggle the status bar |

`Tab` is a **no-op**. Tab completion existed in 1.9.5 and was removed at 2.0.0: with k's terse
syntax it was near-useless and uncomfortable to use.

**Multi-line input.** A line that leaves a bracket open (`{`, `(`, `[`) is an incomplete statement,
so the editor keeps reading on a `...>` prompt until the brackets balance, then evaluates the
joined statement as one — and stores it in history as one entry:

```text
amber> f:{
  ...> x+1
  ...> }
amber> f 41
42
```

There is deliberately no key for this. Shift-Enter is **indistinguishable from Enter** in a
terminal (both send CR) unless you opt into an extended keyboard protocol, so continuing on
incomplete syntax — what python, node, ghci and q all do — is the only portable mechanism.
`Ctrl-C` abandons a continuation. A bracketed paste of a multi-line function is rejoined by the
same string- and comment-aware rule, so a function typed by hand and one pasted produce identical
text; a bracket inside a string (`"a{b"`) or after a comment never triggers it.

**UTF-8.** The buffer holds bytes but a terminal lays out cells, and the editor now converts
between the two: accented Latin, CJK and emoji all measure as the cells actually drawn, Backspace
removes a whole character, and horizontal scrolling never cuts a sequence in half. The width table
is built in rather than taken from `wcwidth()`, which answers per `LC_CTYPE` and would otherwise
lay the same line out differently on macOS and WSL.

**Why `rlwrap` must not be used.** rlwrap runs the wrapped program on a pty and speaks readline on
its behalf, which only works for a program that reads whole lines in *canonical* mode. Amber puts
the terminal into raw / non-canonical mode and reads single keypresses, so rlwrap is doing nothing
useful — and says so, in the middle of your session, usually right after an error:

```text
rlwrap: warning: rlwrap appears to do nothing for amber, which asks for
single keypresses all the time. Don't you need --always-readline
and possibly --no-children? (cf. the rlwrap manpage)
```

while the two editors fight over one cursor and garble the redraw. `./a` therefore **execs the
interpreter directly**. The one case where rlwrap still buys something is when the native editor
is deliberately off (a dumb terminal, an editor subshell, a screen reader):

```sh
AMBER_NO_EDIT=1 ./a          # Amber reads whole lines; ./a wraps it in: rlwrap -n -a
AMBER_NO_RLWRAP=1 AMBER_NO_EDIT=1 ./a   # ... or not even that
```

`-n` (`--no-warnings`) and `-a` (`--always-readline`) are passed unconditionally on that path, so
**no rlwrap diagnostic can reach your session on any path**. The editor also degrades to a plain
line read whenever stdin/stdout are not a terminal, so `echo '2+2' | ./a`, here-docs and CI runs
behave byte-for-byte as they always did. `tests/test_repl_term.py` asserts all of this on a real
pty, including that the terminal's `termios` is restored exactly after a normal exit *and* after
`Ctrl-C`.

---

<a name="architecture"></a>
## Architecture

```
   ./a ──► build.sh ──► cc -std=c99 src/*.c ext/*.c ──► ./amber ──► repl.k
                                        │                             │
                                        │                             └─ amber.k · fin.k · std.k
                                        │                                qsql.k · temporal.k
                                        │                                sys.k · hdb.k · ipc.k
                                        │                                lib/ext.k  (optional)
       ┌────────────────────────────────┴─────────────────────────────────┐
       │                          the interpreter                          │
       │                                                                   │
       │  a.c b.c f.c h.c i.c m.c v.c …   ngn/k evaluator core, heap, verbs │
       │  p.c ast.c vm.c                  parser · `\ast` tree · `\disasm` │
       │  3.c simd.c parallel.c peachpool.c   vector kernels · threads     │
       │  arena.c ser.c csv.c ar.c        HFT arena · -8!/-9! · CSV · Arrow│
       │  e.c diagnostic.c                the Rust-style error reporter    │
       │  inspect.c trace.c               `\v` · `\trace`                  │
       │  ln.c lnk.c                      the line editor and its `rdl` verb│
       │  ext.c                           the extension seam (src/ext.h)   │
       └───────────────────────────────────────────────────────────────────┘
```

Three properties are worth stating explicitly, because they are what the layout is *for*:

* **No optional feature is switched off.** Everything in `src/` is compiled, always. There is no
  AI code, no network code and no TLS anywhere in this repository — `grep -r socket src/` finds
  only `src/0.c`'s IPC support, the same code kdb-style `hopen` uses.
* **`src/ln.c` has no interpreter dependency.** It includes `<termios.h>` and `src/ext.h` and
  nothing else of Amber's; the interpreter-facing verb lives in the separate `src/lnk.c`. The
  editor can be lifted into another project as-is.
* **Extensions never patch `src/`.** See below.

<a name="extensions"></a>
### Extensions (`src/ext.h` + `ext/`)

`ext/` is empty in a stock checkout. An out-of-tree package installs itself by dropping `.c`
files there and re-running `./build.sh`; they are compiled with the same flags, linked into the
same binary, and plug themselves in from a constructor through the hooks in
[`src/ext.h`](src/ext.h):

| hook | what it lets an extension do |
|---|---|
| `am_ext_verb("xyz", fn)` | register a backtick verb — `` `xyz x`` — at runtime |
| `am_ext_bs` | claim a `\`-command before the "unknown `\cmd` is a shell command" fallback |
| `am_ext_hint` | offer inline ghost text in the editor (never inserted until accepted) |
| `am_ext_complete` | add Tab candidates ahead of the built-in lexical sources |
| `am_ext_startup` | run once, lazily, when the REPL first reads a line |
| `am_ext_usage` / `am_ext_banner` | append to `--help` and to the banner |

The Amber-level half goes in `lib/`; `repl.k` loads `lib/ext.k` whole-file at startup if it
exists, fully trapped, and an extension may define the optional `ext.pre` / `ext.post` /
`ext.err` / `ext.raw` / `ext.tag` hooks. `tests/ext_probe.c` is a complete worked example and
`tests/test_ext_seam.sh` installs it, checks every hook and uninstalls it again.

The reason this exists: pulling a new Amber release must never conflict with a package you
installed, and a user who installs nothing must pay nothing — every hook is a null pointer and
every call site is a predictable branch. The optional
[**amber-ai**](https://github.com/bonucciandrea/amber-ai) co-pilot (an entirely separate repo; this
engine contains **no AI code and no network code**) is installed exactly this way:

```sh
git clone https://github.com/bonucciandrea/amber-ai.git
cd amber-ai && ./install.sh /path/to/amber
```

---

## Rust-style diagnostics

By default an error prints the terse core message with a `^` caret under the failing token:

```
'length: operands have mismatched counts
 prices+sizes
       ^
```

Set **`AMBER_DIAG=1`** and the same errors are additionally rendered as a **Rust-compiler-style
report** — an `error[CODE]` line, a `-->` locator, a gutter-aligned source line, `^^^` underlines,
and a `= help:` note (all ANSI-coloured on a colour terminal):

```sh
AMBER_DIAG=1 ./amber                # interactive
AMBER_DIAG=1 ./amber myscript.k     # running a script
```

```text
error[E0104]: Vector length mismatch
  --> test.k:12:8
   |
12 |   prices + sizes
   |   ^^^^^^   ^^^^^
   |
   = help: Both vectors must have matching lengths for element-wise `+`.
```

The formatter lives in `src/diagnostic.{h,c}` (a `Span` source-tracking struct + a
`report_diagnostic()` renderer); the runtime error path (`src/e.c`) routes every parse / type /
domain error through it when `AMBER_DIAG` is set, so the flag is entirely opt-in and changes
nothing about the default output. You can exercise the formatter directly with the `` `dgn ``
self-test builtin (returns `1` when the rendered report matches its expected shape):

```q
`dgn 0        / 1  — diagnostic formatter self-test
```

Turn it on for a session and leave it: it costs one `getenv` per error and never fires on success.

<details>
<summary>Turning the report off at runtime</summary>

The diagnostic is rendered when the error is *created*, so code that catches an error with
`.[f;args;handler]` still sees it on stderr. That is switchable at runtime — useful for anything
that provokes errors on purpose (a test suite, `protect`, a retry loop):

```
prev:`diag 0        / suppress the report, returns the previous setting (1)
.[{1+`a};,0;{"caught"}]     / no output at all
`diag prev          / restore
```

The compact `'type` caret line is not suppressed — it is buffered and still handed to the trap
handler and to `` `err``, so a caught error can always be inspected. `AMBER_DIAG=0` in the
environment still works and seeds the initial value.

</details>

---

<a name="repl-diagnostics-v--ast--trace"></a>
## REPL diagnostics (`\v` · `\ast` · `\trace`)

Three zero-dependency session commands for inspecting the workspace and the evaluator itself —
none of them touch `eval`/`arena`/core REPL behaviour; they only read state and print a report.

**`\v` — a rich workspace inspector.** Every currently-defined global as an ASCII table
(Name / Type / Shape·Length / Memory), with a recursive deep-memory-footprint walker so table
and nested-list sizes are real, not a shallow guess:

```
amber> t:([]a:1 2 3;b:10 20 30)
amber> \v
+-------------+---------------+----------------+---------+
| Name        | Type          | Shape / Length | Memory  |
+-------------+---------------+----------------+---------+
| repl.prompt | Char Vector   | 7              | 64 B    |
| repl.edesc  | Table         | 41 x 12        | 1.5 KB  |
| PAL         | List          | 14             | 1.1 KB  |
| ...         | ...           | ...            | ...     |
| t           | Table         | 3 x 2          | 320 B   |
+-------------+---------------+----------------+---------+
```

`\v` lists *every* global in scope, which after `repl.k`'s modules load includes the library's own
internal state (`repl.*`, `PAL`, `GB`, `OUNI`, …) alongside your own — scan for the names you
defined, or `\d yourns` first to narrow the namespace.

<details>
<summary><code>\ast</code> — colour-coded parse tree (parse-only)</summary>

```
amber> \ast (1+2)*3-4
Root
└── Binary Op : * (Multiply)
    ├── Binary Op : + (Add)
    │   ├── Scalar : 1 (Int64)
    │   └── Scalar : 2 (Int64)
    └── Binary Op : - (Subtract)
        ├── Scalar : 3 (Int64)
        └── Scalar : 4 (Int64)
```

Every leaf carries its literal type (`Int64`, `Float64`, `Symbol`, `Char`, or a
`(TypeName Vector[len])` preview for a literal vector) instead of a generic placeholder, and
Amber's tacit forms get explicit labels — a lambda literal shows its real source text, a 2- or
3-verb train is an explicit **Hook**/**Fork**, and a curried/partial application (`1+`, `f[x;;z]`)
is an explicit **Projection** with a `Blank` node standing in for the omitted argument:

```
amber> \ast {x+1}[3]
Root
└── Apply : Apply
    ├── Lambda : {x+1} (Lambda)
    └── Scalar : 3 (Int64)

amber> \ast +/1 2 3
Root
└── Verb : +/ (Add Over/Reduce)
    └── Vector : 0x01 0x02 0x03 (Byte Vector[3])
```

(A lambda shows its *source text*, never its bytecode — `pk()` itself compiles `{...}` literals
eagerly at parse time, the one exception to "nothing is executed"; disassembling what it compiled
to is `\disasm`'s job, not `\ast`'s.) Verbs render bold cyan, adverbs bold magenta, numeric
scalars bright green, variables and symbols yellow, tree connectors dim gray; list literals,
statement blocks, and hook/fork labels get their own restrained accent colour.

</details>

<details>
<summary><code>\trace</code> — 4-phase execution profiler</summary>

**`\trace`** profiles parse → arena setup → execute → format, with a Unicode bar chart and the
arena's peak scratch usage for that one evaluation. It prints the expression's normal result
first, then the report (timings vary run to run):

```
amber> \trace (1+2)*3-4
-3
+-------------------------------------------------------+
| Parse         454ns  [■■                  ]  10.5%    |
| Arena          38ns  [                    ]   0.9%    |
| Execute       1.1us  [■■■■■               ]  24.8%    |
| Format        2.8us  [■■■■■■■■■■■■■       ]  63.8%    |
+-------------------------------------------------------+
| Total: 4.3us      Arena peak: 0 B                     |
+-------------------------------------------------------+
```

The timer prints `ns` / `us` / `ms` as appropriate, and **Arena peak** is a true high-water mark
from `arena_peak()`. Only expressions that actually reach an arena-backed kernel (`aj`, `wj`,
`` `csvr``, `\ast`) report a non-zero peak:

```
amber> \trace aj[`s`t;tr;qt]
...
| Total: 335.4us    Arena peak: 32 B                    |
+-------------------------------------------------------+
```

`\trace` runs the same `select … by … from … where …` rewrite the interactive prompt uses, so
tracing a table expression or a bare qSQL query renders and profiles correctly:

```
amber> \trace select from t where a>1
+`a`b!(2 3;20 30)
+-------------------------------------------------------+
| Parse          2us  [                    ]   0.9% |
| Arena          3us  [                    ]   1.3% |
| Execute      212us  [■■■■■■■■■■■■■■■     ]  76.7% |
| Format        58us  [■■■■                ]  21.1% |
+-------------------------------------------------------+
| Total: 276us     Arena peak: 0 B                       |
+-------------------------------------------------------+
```

</details>

---

<a name="engine-extensions"></a>
## Engine extensions: SIMD · parallel · disassembler · CSV

Four additive engine modules, each a standalone `.c`/`.h` pair that never touches core
evaluation (`a.c`'s dispatch, `b.c`'s compiler/VM, or the `+`/`*`/`+/` verb implementations).

> A fifth module, a `\hl <expr>` command that echoed one line back with ANSI syntax colour, was
> removed — it only ever colorized a line you explicitly ran, not your keystrokes as you typed
> them, which isn't what "live syntax highlighting" means. Genuine live highlighting would require
> rewriting `repl.k`'s raw-keystroke input loop, which is out of scope for now; see
> [Roadmap](#roadmap).

**SIMD vector kernels** (`src/simd.{h,c}`) — `simd_add_i64/f64`, `simd_mul_i64/f64`,
`simd_sum_i64/f64` operate on plain `int64_t*`/`double*` arrays with an AVX2 path
(`<immintrin.h>`, x86_64), a NEON path (`<arm_neon.h>`, any `aarch64` — Apple Silicon included),
and a scalar C99 fallback, selected at compile time. `simd_backend()` reports which one is active.
The arena allocator (`src/arena.c`) was bumped to genuine **32-byte alignment** via
`posix_memalign` so SIMD loads over arena-backed buffers are always aligned. Self-test + benchmark:
`` `simd 0 `` (prints backend, size, and a SIMD-vs-scalar timing comparison to stderr, returns `1`
on success):

```
$ ./amber
amber> `simd 0
simd: backend=scalar n=400009 simd_add=1.44ms scalar_add=1.98ms ok=1
1
amber> \\ (rebuilt with AMBER_NATIVE=1)
simd: backend=avx2 n=400009 simd_add=1.51ms scalar_add=1.76ms ok=1
```

**Multithreaded vector engine** (`src/parallel.{h,c}`) — `par_add_i64/f64`, `par_mul_i64/f64`,
`par_sum_i64/f64` split arrays **above 100,000 elements** (`PAR_THRESHOLD`) into one contiguous
chunk per POSIX thread (`pthread_create`/`pthread_join`), each chunk processed by the SIMD
kernels above; below the threshold it calls the SIMD kernel directly with no thread overhead.
Thread count follows the same `AMBER_THREADS` env var `peach` already uses (default: online CPU
count, capped at 64). Self-test + benchmark: `` `para 0 ``.

**Bytecode disassembler + `\disasm`** (`src/vm.{h,c}`) — Amber's interpreter (`src/b.c`) already
compiles every expression to a flat opcode array + constant pool and runs it on a real stack
VM (`cr()`/`cpl()`/`run()`) — the AST is never walked directly at eval time. Rather than bolt on
a second, disconnected VM, `vm.c` mirrors `b.c`'s real opcode table byte-for-byte and decodes
the actual compiled bytecode Amber already produces, with a self-consistency check (the decode
loop must consume exactly the bytecode length). `\disasm <expr>` compiles an expression and
prints its locals, constant pool, and instruction stream without executing it:

```
amber> \disasm (1+2)*3-4
locals (0):
constants (2):
  #0  -1
  #1  3
bytecode (6 bytes):
    0  MONAD    1
    1  CONST    #0
    2  CONSTDYAD const#1 dyad=3
    5  MONAD    0
```

(Amber's real compiler constant-folds `1+2` and `3-4` at compile time — the constant pool holds
`3` and `-1`, not the original literals — which is exactly the kind of detail a disassembler for
the *real* VM surfaces that a from-scratch reimplementation would not.)

**Native CSV parser** (`src/csv.{h,c}`) — `` `csvr "path.csv" `` parses a CSV file directly into
a genuine Amber table (the same `flp(names ! cols)` shape `([]…)` produces — verified against
`@`, `meta`, and `qwhere`), through the arena allocator, with per-column type inference (Long /
Float / Symbol), RFC-4180-subset quoted-field handling (embedded commas, `""`-escaped quotes),
and empty cells mapped to that column's null (`0N`/`0n`/`` ` ``):

```
amber> t:`csvr "trades.csv"
amber> meta t
amber> select from t where px>150
```

Self-test: `` `csv0 0 `` (round-trips a fixture CSV through the parser and checks shape/values/
nulls via `#`/`~`/`@`).

**Honest deviations, stated plainly:** (1) no `src/compiler.c` was added — `b.c` already *is*
the real compiler and VM, so a second one would be redundant/misleading; `vm.c` disassembles
the real bytecode instead. (2) the NEON path was written against the real ARM64 NEON intrinsics
and reviewed carefully, but this sandbox has no ARM cross-compiler available to actually build
and run it — it has not been executed on real Apple Silicon hardware.

---

<a name="hft-toolkit"></a>
## HFT toolkit — native `aj`, arena, generators

Amber tightens the tick/quant path:

```q
gentq 100000                       / generate a full session: sets globals `trades` and `quotes`
genopt 2000                        / generate a random option chain into global `options`
m:aj[`sym`time; trades; quotes]    / TAQ: prevailing quote for every trade (native C kernel)
select from options where abs[strike-spot]<5     / near-the-money contracts
```

* **Native `aj` kernel.** `aj`/`aj0` match each trade to its most-recent quote with a
  **branch-free `lower_bound`** binary search over each symbol group's sorted nanosecond
  timestamp slice (`src/a.c`, marshalled from `amber.k`). The pure-K reference (`ajmK`) is kept
  alongside it. Correct on 64-bit ns timestamps, empty groups, and no-match rows (→ null).
* **Zero-allocation arena.** A thread-local **16 MB bump allocator** (`src/arena.{h,c}`:
  `arena_init` / `arena_alloc` / `arena_reset` / `arena_free`) supplies transient scratch during
  evaluation and is rewound once per eval cycle, so per-tick work does not thrash the system
  `malloc`/`free` and the latency jitter they cause stays out of the hot path. Self-test:
  `` `arn 0 `` → `1`.

---

<a name="terminal-charts"></a>
## Terminal charts (`plot` · `candle`)

Pipe a query straight into a chart. `plot` renders a numeric vector as a **Braille** line
chart (2×4 dots per cell → 2× horizontal, 4× vertical resolution); `candle` renders an OHLC
table as **Unicode candlesticks** (green up / red down, box-drawing wicks, block bodies).

```
plot (14*{sin x%7}'!74;60;9)

13.99999 │     ⢀⠤⠒⠉⠉⠑⠤⡀                            ⢀⠤⠊⠉⠉⠒⠤⡀
7.000004 │  ⡠⠊          ⠈⠢⡀                     ⡠⠃          ⠈⢢
1.119252 │⠜                ⠘⢄                ⢠⠊                ⠘⢄
-6.99998 │                    ⠑⡄          ⢀⠜                      ⠣⡀
-13.9999 │                       ⠈⠒⠤⣀⣀⠤⠒⠁                            ⠉

candle bars[10; select from trades where sym=`AAPL]      / OHLC candlesticks in colour
```

See [`examples/graphs.k`](examples/graphs.k) for a chart tour.

<a name="grid-modes"></a>
## Grid modes (`\grid`)

`\grid clean|rounded|sharp|heavy` sets the table frame — `clean` (default) is a minimal dashed
header rule; `rounded`/`sharp`/`heavy` draw a full box with curved / square / thick corners:

```
\grid rounded
([]a:1 2; b:3 4)
╭───┬───╮
│ a │ b │
├───┼───┤
│ 1 │ 3 │
│ 2 │ 4 │
╰───┴───╯
[2 rows x 2 cols]
```

Borders are dimmed so colourised cells stay the focus; `\clear` clears the screen. Grids also
carry **ANSI syntax highlighting** (a 14-hue per-column palette; `COLOR:0` to disable), right-aligned
numeric/temporal columns, and a `PREC`-capped float precision (default 7 decimals).

<a name="native-temporal-types"></a>
## Native temporal types

Dates, times and timestamps are **first-class types** with literal syntax, auto-display and
type-aware arithmetic — no wrappers:

```q
2026.07.30                          / date         -> 2026.07.30
10:00:00.000 + 00:00:05.000         / time + time  -> 10:00:05.000
2026.08.15 - 2026.07.30             / date - date  -> 16   (days)
2026.07.30D09:30:00.000000000       / timestamp (ns since 2000.01.01)
year 2026.07.30                     / accessors: year month day dow / thh tmm tss
"D"$"2026.12.25"                    / string casts: "D"$ "T"$ "P"$
```

<a name="apache-arrow"></a>
## Apache Arrow C Data Interface (zero dependency)

Interop with **PyArrow / Polars / DuckDB** over the stable Arrow C ABI — no `libarrow`
linkage. Export is **zero-copy**:

```q
p:arrow.export t                    / table  -> (schemaAddr; arrayAddr)  64-bit C-ABI pointers
arrow.import p                      / (schemaAddr; arrayAddr) -> Amber table
```

<a name="libamber"></a>
## `libamber.so` — the dynamic C API seam

Amber has always had one seam for extending it **in process**: drop a `.c` file into
`ext/`, rebuild, and it plugs itself in through `src/ext.h` without a line of `src/`
being patched. There is also a matching **out-of-process** seam — the same engine,
built as a shared library, with a small documented C API on the front of it.

```bash
./build.sh                 # ./amber          (unchanged; identical binary, zero new cost)
./build.sh --shared        # ./amber  +  libamber.so
./build.sh --shared-only   # libamber.so only
AMBER_SHARED=1 ./build.sh  # same as --shared, for callers that cannot pass a flag
```

```c
#include "ext.h"                       /* section 6 -- nothing else from src/ */

amber_init("/path/to/amber");          /* boots the engine, loads the .k stdlib */
amber_value t = amber_eval_qsql("select vwap:wavg[sz;px] by sym from trades");

int type; long long n; int bits;
const void *px = amber_get_vector_ptr(amber_table_column(t, 2), &type, &n, &bits);
/* `px` IS the engine's column payload. Not a copy of it. */
```

**~60 entry points, all named `amber_*`.** Boot and evaluate (`amber_init`, `amber_eval_str`,
`amber_eval_qsql`, `amber_call`), reference counting (`amber_retain` / `amber_release`), the
zero-copy vector seam (`amber_get_vector_ptr`), tables and dictionaries, constructors for pushing
data back in, the Arrow C Data Interface, rendering, and `amber_plugin_load` for dlopening a native
plugin into a running engine.

<details>
<summary>What the shared build changes, and what it deliberately does not</summary>

**The executable is untouched.** `./build.sh` with no flags produces byte-for-byte what it produced
before, from the same objects, with the same flags. The shared library is a *separate* object set
(`-fPIC -Dshared`): position-independent code and the global-dynamic TLS model a dlopen'd library
needs both change code generation, and sharing objects between the two would silently pessimise
`./amber`.

**Only `amber_*` and `am_ext_*` are exported.** Amber's internal C is written in a terse K-derived
idiom — the engine's own globals are called `mr`, `su`, `us`, `err`, `run`, `add`, `sub`, `pk`,
`cpl`. Those are perfect inside one static binary and actively dangerous inside a library loaded
next to NumPy, libarrow and libpython. An export map (`src/libamber.map`) makes everything else
genuinely **absent** from the dynamic symbol table, so it cannot be bound to by accident and cannot
interpose on a host's symbol of the same name.

**The library records a `SONAME`.** A satellite that dlopens a *second* copy of `libamber.so` gets
a second **engine**: two heaps, two symbol tables, two global namespaces, and values from one that
are meaningless to the other — with no error, because nothing is technically wrong. The SONAME is
what lets the loader recognise an already-loaded copy and reuse it, so `python-amber`,
`libamber_arrow.so` and any plugin in one process share one engine.

**TLS drops to global-dynamic in the shared build only.** `./amber` keeps the initial-exec model on
the allocator's hot path. A dlopen'd library cannot: it is resolved out of the static TLS block the
loader sizes before `main()`, and borrowing from glibc's small surplus reserve fails
*nondeterministically* — with `cannot allocate memory in static TLS block` — depending on what else
the host imported first. See the note above `AM_TLS_IE` in `src/a.h`.

</details>

**Verification.**

```bash
tests/test_capi.sh              # release build, then ASan + UBSan
tests/run_tests.sh --asan       # the whole suite, the C API included
```

`tests/test_capi.c` is the only consumer of `libamber.so` inside this repository, and it is written
the way a satellite would write it: it includes `src/ext.h` and nothing else from `src/`, never
dereferences an `amber_value`, and links the shared library rather than the objects. **If it ever
needs a second `-I`, the API is wrong.** 81 assertions, clean under
`-fsanitize=address,undefined` with `detect_leaks=1` — because a C API whose ownership rules are
only documented is a C API whose ownership rules are wrong, and LeakSanitizer is what checks the
prose.

<a name="satellite-ecosystem"></a>
## The satellite ecosystem

Everything that consumes Amber lives **outside** this repository and reaches it through one of
three seams: `libamber.so`, the in-process `ext/` registry, or a TCP socket. Nothing below is
mentioned anywhere in `src/`, and none of it is compiled, linked or configured by this build.

| repository | seam | what it is |
|---|---|---|
| [`python-amber`](https://github.com/BonucciAndrea/python-amber) | `libamber.so` | `pip install amber`. Zero-copy NumPy views of Amber columns, pandas and Arrow bridges, dynamic dispatch (`am.gentq(10_000_000)`). |
| [`amber-arrow`](https://github.com/BonucciAndrea/amber-arrow) | `libamber.so` | `ArrowArrayStream` over an Amber table — batches that are *windows* onto one export, not slices of it. Plus `amberd`, the TCP query server, and an Arrow Flight daemon. |
| [`amber-jupyter`](https://github.com/BonucciAndrea/amber-jupyter) | `python-amber` | A Jupyter kernel. Amber cells and `%%python` cells in **one process**, so a column crosses as a pointer. |
| [`vscode-amber`](https://github.com/BonucciAndrea/vscode-amber) | `amberd` socket | Syntax highlighting, and a standalone LSP daemon with qSQL-aware completion, idiom hovers and diagnostics that never evaluate. |
| [`grafana-amber-datasource`](https://github.com/BonucciAndrea/grafana-amber-datasource) | `amberd` socket | Live dashboards. Bare qSQL panels, column-oriented on the wire. |
| [`amber-flame`](https://github.com/BonucciAndrea/amber-flame) | `python-amber` / `amberd` | A visual profiler — flamegraphs, Speedscope and Chrome tracing, built on the engine's own `\trace`. |

The engine gained **one build flag, one export map and one section of `ext.h`** for all of it.

<a name="comparative-benchmark-query-files"></a>
## Comparative benchmark query files

`bench/run_comparative.py` (see [docs/BENCHMARKS.md §5](docs/BENCHMARKS.md) for the live table,
refreshed on every push by CI) times **ten engines** on four workloads. The workloads, the data
model and the fairness rules are specified once, in **[`bench/SPEC.md`](bench/SPEC.md)**; every
engine implements that document and nothing else.

| engine | file | peer group |
|---|---|---|
| C (`gcc -O3 -march=native`) | `bench/queries/c_bench.c` | the floor everything is measured against |
| Amber (array primitives) | `bench/queries/amber_bench.k` | ngn/k · CBQN · J · Uiua |
| Amber (qSQL layer) | `bench/queries/amberq_bench.k` | DuckDB SQL |
| ngn/k | `bench/queries/k_<id>.k` | array primitives |
| CBQN | `bench/queries/bqn_<id>.bqn` | array primitives |
| J | `bench/queries/j_<id>.ijs` | array primitives |
| Uiua | `bench/queries/uiua_<id>.ua` | array primitives |
| NumPy | `bench/queries/numpy_bench.py` | array primitives |
| Julia | `bench/queries/julia_bench.jl` | scalar loops (JIT) |
| DuckDB | `bench/queries/duckdb_<id>.sql` | query layer |

Workloads: **vector arithmetic + boolean masking**, **reductions** (sum · max · dot over 10M
elements), **group-by aggregation** (100 groups over 10M rows), and an **inner join** (1M left
rows against 1,000 sparse keys).

<details>
<summary>How fairness is enforced, not just claimed</summary>

Every answer in this suite is an integer that fits in float64 exactly, and every sum is over such
integers, so the result is independent of summation order — SIMD pairwise, Kahan-compensated and
naive left-fold summation all produce the identical bit pattern. The harness therefore compares
answers **exactly** against the C reference and prints **WRONG** in place of a time for any engine
that disagrees. Each engine also emits a checksum of its *input* data, so a divergence in the
generator is caught separately (**BADDATA**). You cannot win a cell in this table by computing
something cheaper.

Two shortcuts the previous suite contained, both now removed and documented in `SPEC.md`:

* **`+/!10000000` is O(1) in Amber.** `src/3.c`'s `arf` constant-folds a sum over a *range* into
  the closed form `n(n-1)/2`. The old `vecsum` benchmark was exactly that expression, so Amber
  "won" it by never touching 10M elements while every other engine ran a real reduction. All
  data is now materialised before the clock starts.
* **A dense-key "join" is just an array index.** With right keys `0..K-1`, every array language
  answers the join with a single gather while DuckDB still builds a hash table. Right keys are
  now sparse and unsorted, forcing a genuine key lookup everywhere.

**Amber is reported twice, on purpose.** `Amber` is array-primitive code — the fair peer of
ngn/k, CBQN, J and Uiua. `Amber qSQL` routes the same workloads through the `select … by … from`
layer — the fair peer of DuckDB's SQL planner. Publishing only the faster of the two would mean
picking whichever comparison flatters Amber; the gap between the rows *is* the query layer's
overhead and is meant to be visible.

**Timing excludes startup.** Engines that can time their own kernel (Amber, C, NumPy, Julia,
DuckDB and CBQN, via `•MonoTime`) do so with a monotonic clock after warm-up passes, and the
harness uses that number directly. Engines with no usable in-language clock (ngn/k, Uiua, J) are
measured as *total process time − a measured startup baseline*. The results table labels which mode
produced each cell, so the two are never silently mixed.

</details>

The former lexer quirk that bit these files — a `.k` comment line containing *only* a bare `/`
silently truncating everything after it — **is fixed in 2.0.0**: an unterminated bare-`/` block
comment now raises a clean parse error. See [docs/MISSING.md](docs/MISSING.md) §14.

<a name="why-attributes-matter"></a>
## Why attributes matter

`bench.k` measures `?` (find) on identical data, sorted-attributed vs not:

| rows | linear scan | binary (`` `s``) | speedup |
|-----:|------------:|-----------------:|--------:|
| 100 k | 87 ms | 0.6 ms | **141×** |
| 500 k | 417 ms | 0.9 ms | **470×** |
| 2 M | 1.73 s | 1.4 ms | **1244×** |
| 5 M | 4.23 s | 1.9 ms | **2261×** |

Results are identical; only the time differs. `asc` / `xasc` set the attribute for you, and
`meta` shows it in the `a` column.

---

<a name="language-notes"></a>
## Language notes (30-second version)

Amber uses a terse array notation. A few things worth knowing:

* **Two-argument library dyads work infix *or* in brackets.** Since 2.0.0, `x in y`, `t lj kt`,
  `` `sym xasc t``, `5 within 3 9`, `` "/" sv `a`b`c`` all work infix, exactly like kdb+/q — and the
  bracket form `f[x;y]` and prefix form `f x` still work. The infix set is a curated list —
  `in within like lj ij uj aj aj0 wj wj1 pj ej cross inter union except ss sv vs xasc xdesc` — plus
  the built-in symbol verbs (``+ - * % ! & | < > = ~ , ^ # _ $ ? @ .``). Arbitrary user lambdas are
  **not** infix.
* **No `>=` / `<=`** — write `~a<b` and `~a>b`.
* **qSQL is bare — at the prompt *and* in scripts.** Type `select … by … from … where …` (also
  `exec` / `update` / `delete`) with no `sel"…"` wrapper — bare column names like `wavg[sz;px]` just
  work. Since 2.0.0 this bare form also works inside a `.k` file loaded once the stdlib is up (the
  loader runs each file through the same rewriter the REPL uses); the `sel"…"` / `exq"…"` / `upd"…"`
  / `del"…"` string forms and the functional forms `qselect`/`qby`/`qwhere` still work too.
* **`peach[f;y]` is real multi-core** — it forks `AMBER_THREADS` worker processes (default: the
  online CPU count, detected via `sysconf`; `=1` forces serial), so heavy per-item work scales
  across cores with no GIL and it won't oversubscribe a small box or leave a big one idle.
* **Grids preview Q-style** — `show t` prints the first `CROWS` rows (default 20) then `..`, with
  a dimmed `[N rows x M cols]` footer and ANSI syntax highlighting.
* **Errors show a `^` caret** under the failing token plus a descriptive message; set
  `AMBER_DIAG=1` for the full Rust-style report (see [Rust-style diagnostics](#rust-style-diagnostics)).
* **Symbols have no `_`** — use a quoted symbol `` `"a_b" ``.
* Tables: `([]col:vals;…)`; keyed tables: `([key:vals]col:vals)`. A bare table at the prompt
  auto-renders as a grid.

Full reference: **[docs/AMBER.md](docs/AMBER.md)**. Built-in help: `\` then `\q \j \z` for the Amber
vocabulary, ``\0 \+ \` \'`` for the core, `\v \ast \trace` for the session/diagnostic tools (see
[REPL diagnostics](#repl-diagnostics-v--ast--trace)), and `\disasm` for the bytecode disassembler
(see [Engine extensions](#engine-extensions)).

---

<a name="finance-module"></a>
## Finance / HFT module (`fin.k`)

Auto-loaded after `amber.k`. Generate a market session and analyse it the way an HFT desk does:

```q
gentq 100000                       / sets global `trades` and `quotes`
genopt 2000                        / sets global `options` (random option chain)
m:aj[`sym`time; trades; quotes]    / TAQ: prevailing quote for every trade
tsign m                            / Lee-Ready trade sign (+1 buy / -1 sell)
effspread m                        / effective spread = 2|px-mid|
qby[trades;`sym; `vwap!enlist {wavg[x`sz;x`px]}]   / VWAP by symbol
bars[1; trades]                    / 1-minute OHLCV bars
```

Included: `mid spread spreadbps micro imbal` (book), `vwap twap tsign signedvol effspread
notional` (trades), `ret logret rvol movavg movsum movmax movmin ema rollstd` (returns/vol),
`bars symstats` (aggregation), `bysym symrows gidx` (O(1) index), `genopt gentq` (generators).
Walkthrough: `./amber examples/hft.k`.

**Attributes.** Amber has all four kdb-style attributes in C: `` `sa`` sorted, `` `ua`` unique,
`` `pa`` parted, `` `ga`` grouped (`` `at`` reads them, `meta` shows them). Sorted/parted give
O(log n) kernel find; grouped + the group index give O(1) per-symbol slicing.

<a name="whats-inside"></a>
## What's inside

| file | |
|------|--|
| `a`, `build.sh` | launcher (build-if-stale) and portable compile (gcc / clang) |
| `src/*.c`, `src/*.h` | the interpreter — ngn/k core + Amber extensions (`src/p.c` the `([]…)` parser; `src/ar.c` Arrow; `src/arena.{h,c}` the HFT arena, 32-byte aligned; `src/diagnostic.{h,c}` the Rust-style formatter; the native `aj` kernel in `src/a.c`; `src/inspect.{h,c}` the `\v` inspector; `src/ast.{h,c}` the `\ast` visualiser; `src/trace.{h,c}` the `\trace` profiler; `src/fmtutil.{h,c}` and `src/ansi.h` shared formatting/colour helpers; `src/simd.{h,c}` AVX2/NEON/scalar kernels; `src/parallel.{h,c}` the pthreads vector engine; `src/vm.{h,c}` the bytecode disassembler behind `\disasm`; `src/csv.{h,c}` the native CSV parser behind `` `csvr``) |
| `amber.k` | the q/kdb+ vocabulary (auto-loaded) |
| `repl.k` | the REPL — banner, grid rendering, `\grid`/`\clear`, help; CRLF-safe module loader; reads its input through `` `rdl`` (the native editor) and exposes the optional `ext.*` hooks |
| `src/ln.{h,c}`, `src/lnk.c` | the native line editor (raw `termios`, history, Tab completion) and the `` `rdl`` verb that the REPL reads through — this is what replaced `rlwrap` |
| `src/ext.{h,c}`, `ext/` | **both** extension seams. Sections 1-5: the in-process registry — runtime verbs plus `\`-command / editor / startup hooks, and the (empty by default) directory `build.sh` compiles out-of-tree extensions from. Section 6: the out-of-process **dynamic C API** behind `libamber.so` (`amber_init`, `amber_eval_str`, `amber_get_vector_ptr`, …) |
| `src/libamber.map` | the linker export map for the shared build: only `amber_*` and `am_ext_*` reach a host process's dynamic namespace, so the engine's terse internals (`mr`, `run`, `add`, …) cannot collide with a host's symbols |
| `fin.k` | finance / HFT module (auto-loaded) — see `\m` help |
| `std.k` `qsql.k` `temporal.k` `sys.k` `hdb.k` `ipc.k` `tick.k` | modules (auto-loaded) |
| `examples/` | `tour.k` · `basics.k` · `tick.k` · `hft.k` · `peach.k` · `wj.k` · `graphs.k` · … |
| `test.k` `test-fin.k` `test-ext.k` | assertion suites (202 + 35 + 79) |
| `tests/harness.k` | shared assertion harness — `t` (value), `tv` (trapped expression), `te` (must-raise), `tk` (must-not-raise), `hexpect` (assertion-count guard), `hreport` |
| `tests/test_matrix.k` | **309-case combinatorial matrix**: every primitive × every element type × sizes 0 / 1 / 10 / 100 000+ (crossing the SIMD and `PAR_THRESHOLD` boundaries), asserted as invariants (shape, algebraic identity, vector-kernel-vs-scalar-reference) rather than frozen literals |
| `tests/test_qsql.k` | **117-case qSQL matrix**, written in the **bare `select … from t` syntax you actually type** (run through the same `qrw` rewrite the REPL and loader apply): the full `select`/`exec`/`update`/`delete` clause lattice, multi-key `by`, empty / single-row / heavily-duplicated tables, and malformed queries asserted to raise cleanly |
| `tests/fuzz.py` | malformed-input & deep-nesting crash fuzzer — asserts a clean K error, never a signal or a hang |
| `tests/test_capi.{c,sh}` | the dynamic C API: 81 assertions against `libamber.so`, linked as a satellite would link it (`src/ext.h` and nothing else from `src/`), run once at `-O2` and once under ASan + UBSan with leak detection — the ownership rules in the header are prose, and this is what checks them |
| `tests/test_qsql_script.sh`, `tests/test_comments.sh` | shell suites for the 2.0.0 loader work: bare qSQL inside a loaded `.k` file, and the unterminated bare-`/` comment now raising cleanly |
| `tests/run_tests.sh` | runs all of the above (`--asan` re-runs everything under ASan + UBSan) |
| `tests/test_repl_term.py` | **pty-driven REPL terminal suite**: asserts no `rlwrap:` diagnostic ever reaches a session, that `termios` is byte-for-byte restored after a normal exit *and* after `^C`, that the editing keys really edit, and that piped/non-tty behaviour is unchanged |
| `tests/test_ext_seam.sh`, `tests/ext_probe.c` | installs a miniature extension into `ext/`, checks the verb / `\`-command / `--help` hooks fire and that the engine's own suite is unaffected, then uninstalls it and checks the engine is back to stock |
| `tests/*.c` | standalone C test harnesses: `test_simd.c`/`test_parallel.c` (no Amber dependency), `test_ast.c` (links the full interpreter — ast.c is inherently built on Amber's real parser) |
| `bench.k` `bench-fin.k` `bench-std.k` `bench/` | attribute / index / window benchmarks; `bench/run_comparative.py` cross-engine harness (see [docs/BENCHMARKS.md §5](docs/BENCHMARKS.md)); `bench/queries/amber_*.k` and `bench/queries/k_*.k` are separate, independently-tuned scripts per engine — see [Comparative benchmark query files](#comparative-benchmark-query-files) |
| `docs/` | `AMBER.md` (reference) · `MISSING.md` (roadmap + known leniencies) · `BENCHMARKS.md` · `AUDIT-1.9.md` (the 1.9 security/correctness audit report) |
| `CHANGELOG.md` | release history (2.0.0 first) |
| `.gitattributes` | forces LF checkout of sources so the REPL's line-based loader works on Windows too |

<a name="roadmap"></a>
## Roadmap

Amber covers a large slice of q. [docs/MISSING.md](docs/MISSING.md) is an honest map of what's next —
top picks: wiring the `` `g`` grouped attribute into the C find path; **attribute preservation
through ops** (keep/drop by q's per-op rules); the missing atom types
(`short`/`real`/`byte`/`guid`); a true partitioned / memory-mapped HDB; and **live REPL syntax
highlighting** (colouring tokens *as you type*, which needs `repl.k`'s raw-keystroke input loop
rewritten).

<a name="isolation"></a>
## Isolation

Amber is a single self-contained folder. The interpreter is named `amber` (never `k` or `q`),
built only inside the folder, never placed on your `PATH`. It reads/writes no config, no
`QHOME`, no dotfiles. Your kdb+, kona and other k/q installs are untouched; deleting the folder
uninstalls Amber completely.

<a name="licence"></a>
## Licence

GNU AGPLv3 (see [LICENSE](LICENSE)). Amber's interpreter core derives from **ngn/k**, an AGPLv3
K interpreter by ngn; that attribution is preserved in [NOTICE](NOTICE), as the licence requires.
Amber is an independent language and is not affiliated with, nor a distribution of, that project.
