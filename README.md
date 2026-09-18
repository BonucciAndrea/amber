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
![version](https://img.shields.io/badge/version-2.2.0-orange)
![license](https://img.shields.io/badge/license-AGPLv3-blue)
![tests](https://img.shields.io/badge/tests-873%20K--suite%20cases-brightgreen)
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
<a name="whats-new-220"></a>
## What's new in 2.2.0

**A correctness and performance release.** Four defects that returned a **wrong value
rather than an error** are fixed -- none of them raised, so none of them was visible:

- A lambda **parameter** named `ss`, `in`, `except` or any other infix verb parsed as the
  verb, so `1_ss` silently built a projection instead of dropping an element.
- `1 2 3 in 2` answered `1 1`: an atom right argument fell through to `y?x`, which for an
  integer atom is k's **random deal**.
- Float literals below `1e-308` did not parse at all -- `pfu` returned without advancing
  the cursor.
- `?x` on a float column allocated `2n` hash slots up front (320 MB at 10M rows).

And the engine got faster where it matters for a price column:

- **A keys-only radix sort.** The order-preserving radix key is a bijection, so the
  permutation is applied to the **keys** and unfolded, instead of gathering the input. The
  gather was a third of the cost of a sort. Non-integral float64, 10M: **2.6x**.
- **The float range scan is split** from the integrality test, and `floor()` is gone from
  it. Already-sorted 10M float64: **5.8x**. Float `distinct`: **2.0x**. Grade: **1.4x**.
- **`+/(a±s*b)@&m` fuses** to a single masked pass: **1.5x**.
- **`select` stops materialising the filtered table** on the paths that ignore it:
  `select from t` is **8-19x** faster depending on how wide the table is.
- **`select[n;>col]`** -- q's sorted and limited select, including on a grouped result.
- **Charts label a temporal axis as a time**, not as the integer underneath: `xunit`/`yunit`,
  `tplot`/`dplot`/`pplot`, ticks on round clock and calendar boundaries, and candles that
  carry their bar times.

On the 24-operation comparative matrix (10M elements, one core, every timing gated on an
exact answer match against a C reference) Amber beats **CBQN on 12 of the 20 operations
both implement**, is faster than the **C reference on 16 of 24**, and is the fastest of
the twelve published engines on **9**. See [`docs/BENCHMARKS.md`](docs/BENCHMARKS.md),
[`bench/SCOUT_REPORT.md`](bench/SCOUT_REPORT.md) and [`CHANGELOG.md`](CHANGELOG.md) for
every number and every change.

One row moved the wrong way and is published rather than dropped: `max_f` is 3% slower,
which is code layout and not a code change -- LTO inlines `simd_max_f64` into the float
min/max reducer, and that function is 937 instructions, instruction-for-instruction
identical in both builds, differing only in address.

<a name="whats-new-older"></a>
## Earlier releases

2.1.0, 2.0.1, 2.0.0 and everything before them are in
[`CHANGELOG.md`](CHANGELOG.md), which is the single place release notes live.

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
The full shell-rc recipe -- `AMBER_HOME`, the `amber` / `amberx` / `amber-ai`
aliases, thread pinning, and why you symlink the launcher rather than put the
repo root on `PATH` -- is in [`docs/INSTALL.md`](docs/INSTALL.md).

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
./amber --version           # amber 2.1.0
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
python3 bench/scout/scout.py     # the widest run: 23 ops x 13 engines (PeachQ, ngn/k,
                                 #   CBQN, J, NumPy, pandas, Polars, DuckDB, a C reference)
python3 bench/scout/webgen.py --md   # regenerate the tables in docs/BENCHMARKS.md from results.json
```

---

<a name="terminal-charts"></a>
## Terminal charts (`chart` · `plot` · `plots` · `candle`)

Pipe a query straight into a chart. Charts are framed and axis-labelled, take several series
at once, and downsample a million-point series without turning it into a smear. The surface is
**Braille** — a 2×4 dot bitmask per character cell, so a `W`×`H` box is a `2W`×`4H` raster.

```
chart `y`title`ylabel!(14*{sin x%7}@!74;"AAPL mid";"price")

                               AAPL mid
price
    ┌────────────────────────────────────────────────────────────────┐
    │      ⣀⠔⠒⠒⠤⡀                             ⣀⠔⠒⠒⢄⡀                 │
 10 ┤⠄   ⡠⠊  ⠄  ⠈⠑⢄  ⠄   ⠄   ⠄   ⠄   ⠄   ⠄  ⡰⠉   ⠄ ⠈⠑⢄   ⠄   ⠄   ⠄   │
    │  ⢀⠎          ⠑⡄                     ⢀⠎          ⠣⡀             │
    │ ⡰⠁            ⠈⢢                  ⢀⠔⠁            ⠈⢢            │
  0 ┤⠜   ⠄   ⠄   ⠄   ⠄⠣⡀ ⠄   ⠄   ⠄   ⠄ ⢠⠊⠄   ⠄   ⠄   ⠄   ⠣⡀  ⠄   ⠄   │
    │                  ⠈⢆             ⡠⠃                  ⠑⢄         │
    │                    ⠱⡀         ⢀⠜                      ⠱⡀       │
-10 ┤⠂   ⠂   ⠂   ⠂   ⠂   ⠂⠈⢆⡀⠂   ⠂⣀⠔⠁⠂   ⠂   ⠂   ⠂   ⠂   ⠂   ⠈⢆⡀ ⠂   │
    │                       ⠈⠑⠤⠤⠔⠊                                   │
    └┬───────────────┬───────────────┬──────────────┬───────────────┬┘
     0              20              40             60              80
```

| verb | draws |
|---|---|
| `chart d` | anything below, from an option dictionary |
| `plot v` · `plot (v;W;H)` | one line series |
| `plots (a;b;c)` · ``plots `a`b!(x;y)`` | several series, with a legend |
| `xyplot (xs;ys)` · `scatter (xs;ys)` | y against a real x axis; points |
| `step v` · `area v` | piecewise-constant; filled |
| `hist v` · ``barh `a`b!3 7`` · `heat m` | distribution; labelled bars; matrix |
| `spark v` | **returns** a one-line string, to embed in a row of text |
| `candle t` | OHLC candlesticks, green up / red down |

`chart` takes `y x w h ylim xlim grid axis legend colour title xlabel ylabel names col style`.
Two details do most of the work: the axis range is the data's own range **snapped outward to a
1/2/5 boundary** (round tick values, without the wasted margin a coarse step would leave), and
past roughly `4W` points each pixel column is drawn as its **min→max envelope** rather than by
joining consecutive points — which keeps every spike instead of filling the box in solid.

```
plots `bid`ask!(q`bid;q`ask)                              / two series and a legend
chart `y`ylim`title!(px;98 102;"AAPL")                    / pin the axis to zoom
chart `y`x`names!((px;ma);(!#px;50+!#ma);("px";"MA(50)")) / per-series x, so a shorter
                                                          / moving average still lines up
candle bars[10; select from trades where sym=`AAPL]
```

See [`examples/graphs.k`](examples/graphs.k) for a 31-chart tour and
[docs/AMBER.md §9c](docs/AMBER.md) for the full option table.

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
  `AMBER_DIAG=1` for the full Rust-style report (see [Rust-style diagnostics](docs/REPL.md#rust-style-diagnostics)).
* **Symbols have no `_`** — use a quoted symbol `` `"a_b" ``.
* Tables: `([]col:vals;…)`; keyed tables: `([key:vals]col:vals)`. A bare table at the prompt
  auto-renders as a grid.

Full reference: **[docs/AMBER.md](docs/AMBER.md)**. Built-in help: `\` then `\q \j \z` for the Amber
vocabulary, ``\0 \+ \` \'`` for the core, `\v \ast \trace` for the session/diagnostic tools (see
[REPL diagnostics](docs/REPL.md#repl-diagnostics-v--ast--trace)), and `\disasm` for the bytecode disassembler
(see [Engine extensions](docs/INTERNALS.md#engine-extensions)).

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

<a name="documentation"></a>
## Documentation

| | |
|---|---|
| [`docs/AMBER.md`](docs/AMBER.md) | the language reference: every verb, adverb and qSQL clause |
| [`docs/REPL.md`](docs/REPL.md) | line editing, `` / `st` / `	race`, error reports, grid modes |
| [`docs/INTERNALS.md`](docs/INTERNALS.md) | architecture, extensions, SIMD/parallel, Arrow, `libamber.so` |
| [`docs/BENCHMARKS.md`](docs/BENCHMARKS.md) | the full engine matrix, methodology, and the query files |
| [`docs/INSTALL.md`](docs/INSTALL.md) | shell configuration |
| [`docs/MISSING.md`](docs/MISSING.md) | what is deliberately not implemented yet |
| [`CHANGELOG.md`](CHANGELOG.md) | every release |

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
| `bench.k` `bench-fin.k` `bench-std.k` `bench/` | attribute / index / window benchmarks; `bench/run_comparative.py` cross-engine harness (see [docs/BENCHMARKS.md §5](docs/BENCHMARKS.md)); `bench/queries/amber_*.k` and `bench/queries/k_*.k` are separate, independently-tuned scripts per engine — see [Comparative benchmark query files](docs/BENCHMARKS.md#comparative-benchmark-query-files) |
| `docs/` | `AMBER.md` (reference) · `MISSING.md` (roadmap + known leniencies) · `BENCHMARKS.md` |
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
