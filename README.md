<div align="center">

```
   █████╗ ███╗   ███╗██████╗ ███████╗██████╗
  ██╔══██╗████╗ ████║██╔══██╗██╔════╝██╔══██╗
  ███████║██╔████╔██║██████╔╝█████╗  ██████╔╝
  ██╔══██║██║╚██╔╝██║██╔══██╗██╔══╝  ██╔══██╗
  ██║  ██║██║ ╚═╝ ██║██████╔╝███████╗██║  ██║
  ╚═╝  ╚═╝╚═╝     ╚═╝╚═════╝ ╚══════╝╚═╝  ╚═╝
```

**A low-latency array language: columnar, vectorised, in-memory.**

![ci](https://github.com/BonucciAndrea/amber/actions/workflows/ci.yml/badge.svg)
![version](https://img.shields.io/badge/version-2.6.0-orange)
![license](https://img.shields.io/badge/license-AGPLv3-blue)
![tests](https://img.shields.io/badge/tests-1555%20K--suite%20cases-brightgreen)
![build](https://img.shields.io/badge/build-C99%20·%20portable%20·%20gcc%20+%20clang-informational)

</div>

## What is Amber

Amber is a small, fast, self-contained array language with the working vocabulary of
**q**: dictionaries, **tables & keyed tables** with `([]…)` literal syntax, the full
**join family** (left · inner · union · plus · equi · **as-of** · **window**), qSQL-style
select/by, strings, intraday **tick / OHLC** temporals, and **column attributes implemented
in C** that turn search from `O(n)` into `O(log n)`, which is **~1000–2000× faster** on large data.

Amber's interpreter core is built on **[ngn/k](https://codeberg.org/ngn/k)**, ngn's compact,
AGPLv3 implementation of the K array language. Amber keeps that engine's speed and small
footprint and layers a q vocabulary, C-level column attributes, native temporal types,
`([]…)` table syntax, a tick/HFT toolkit, and a modern REPL on top. (The attribution is
recorded in [NOTICE](NOTICE), as the AGPLv3 requires.)

```q
t:([]sym:`AAPL`MSFT`AAPL; px:187.3 411.2 187.4; sz:100 250 50)   / a table, rendered instantly
qby[t; `sym; (,`vwap)!,{wavg[x`sz;x`px]}]                        / vwap by symbol
```

<a name="whats-new"></a>
<a name="whats-new-241"></a>
<a name="whats-new-240"></a>
<a name="whats-new-242"></a>
<a name="whats-new-250"></a>
<a name="whats-new-260"></a>
## What's new in 2.6.0

Threads and fusion come in from the experimental branch. Sort, grade, find, `in`, distinct, gather,
`aj`, group aggregates and float sums/max/min now spread over all cores once a vector is big, and
`+ - * %`, comparisons and `abs` over float and int vectors run as one fused pass instead of one
pass per verb. On a 14-core laptop that's about 1.6× over 2.5.0 across the benchmark set, up to
4.6× on `find`. `AMBER_THREADS=1` turns the threads off.

One thing to know: float sums of 1M+ items now add in fixed 64K blocks, so the last digits can
differ from 2.5.0's straight sum, but they're the same at any thread count.

2.5.0 before it was a long bug list worked through: crashes, wrong answers, and a pile of spots
where Amber now answers like q (nulls in `sums`/`wavg`/`cov`, dicts with keys on one side, qSQL
where-clauses and column names, `wj` vs `wj1`, JSON keys, date strands). Details in the
[changelog](CHANGELOG.md).

<a name="whats-new-220"></a>
<a name="whats-new-older"></a>
## Earlier releases

2.4.2, 2.4.1, 2.4.0, 2.3.1, 2.3.0, 2.2.0 and everything before them are in
[`CHANGELOG.md`](CHANGELOG.md), which is the single place release notes live.

<a name="quickstart"></a>
## Quickstart

One folder, no install step, no system files. `./a` builds if it needs to and drops
you at the REPL; `./demo.sh` builds and runs a 5,000,000-row tick session end to end.

```sh
git clone https://github.com/BonucciAndrea/amber && cd amber
./a                          # build if needed, then the REPL
AMBER_NATIVE=1 ./a           # -march=native build
./demo.sh                    # the full Mega Demo
./amber script.k             # run a script
./amber --help               # options and the \-command reference
```

A script gets the bare interpreter: put `\l amber.k` on its first line to load the standard
library (`sum`, `avg`, `xbar`, the joins, qSQL's helpers). The REPL (`./a`) loads it for you.

```q
\l amber.k
`0:$sum 1 2 3      / 6
```

**Windows.** WSL works as is, and so does plain [Cygwin](https://cygwin.com) since 2.3.
Install it with a compiler, make and git (python3 only if you'll run the test suite),
either by ticking them in the installer or in one go:

```sh
setup-x86_64.exe -q -s https://mirrors.kernel.org/sourceware/cygwin/ -P gcc-core,make,git,python3
```

then open a Cygwin terminal and run the commands above. Clone with Cygwin's git (or set
`git config --global core.autocrlf false` first) so the scripts keep their LF endings.
The interpreter, the REPL and the test suite all run there; `libamber.so` is Linux and
macOS only.

Typed at the prompt, where a bare table renders as a grid and qSQL works directly:

```q
([]sym:`a`b`c; px:100 200 300)     / tables are first-class, no `show` needed

trade:([]sym:`a`b`a; time:3 4 9; px:100 200 300; sz:10 20 30)
quote:([]sym:`a`a`b`a; time:1 5 2 8; bid:10 11 20 12)
aj[`sym`time; trade; quote]        / as-of join, native C kernel

select last px by sym from trade   / qSQL, no sel"..." wrapper
select from trade where px>150
select[3;>px] from trade           / sorted and limited

`sym xasc trade                    / library dyads work infix
v:asc 2000000?1000000000           / `s attribute set by asc ...
v ? 12345 67890                    / ... so find is O(log n)

peach[{avg x?1.0}; 8#1000000]      / 8 tasks across real cores, no GIL
```

The demo, the full shell-rc recipe, a worked tour of every feature and the complete
command list are in [`docs/QUICKSTART.md`](docs/QUICKSTART.md).

<a name="terminal-charts"></a>
## Terminal charts

Braille line charts, Unicode candlesticks and histograms, drawn in the terminal with no
dependencies. Temporal axes are labelled as clocks and calendars rather than as the
integers underneath:

```q
gentq 1000                                          / sets `trades` and `quotes`

plot (14*{sin x%7}'!74;60;9)                        / braille line chart
tplot (trades`time; trades`px)                      / x axis reads 09:30:00
candle bars[10; select from trades where sym=`AAPL] / candlesticks, clock-labelled
```

Full gallery and the axis-unit rules: [`docs/QUICKSTART.md`](docs/QUICKSTART.md) and
[`docs/AMBER.md`](docs/AMBER.md).

<a name="native-temporal-types"></a>
## Native temporal types

Dates, times and timestamps are **first-class types** with literal syntax, auto-display and
type-aware arithmetic, with no wrappers:

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
| [`amber-arrow`](https://github.com/BonucciAndrea/amber-arrow) | `libamber.so` | `ArrowArrayStream` over an Amber table, batches that are *windows* onto one export, not slices of it. Plus `amberd`, the TCP query server, and an Arrow Flight daemon. |
| [`amber-jupyter`](https://github.com/BonucciAndrea/amber-jupyter) | `python-amber` | A Jupyter kernel. Amber cells and `%%python` cells in **one process**, so a column crosses as a pointer. |
| [`vscode-amber`](https://github.com/BonucciAndrea/vscode-amber) | `amberd` socket | Syntax highlighting, and a standalone LSP daemon with qSQL-aware completion, idiom hovers and diagnostics that never evaluate. |
| [`grafana-amber-datasource`](https://github.com/BonucciAndrea/grafana-amber-datasource) | `amberd` socket | Live dashboards. Bare qSQL panels, column-oriented on the wire. |
| [`amber-flame`](https://github.com/BonucciAndrea/amber-flame) | `python-amber` / `amberd` | A visual profiler, flamegraphs, Speedscope and Chrome tracing, built on the engine's own `\trace`. |

The engine gained **one build flag, one export map and one section of `ext.h`** for all of it.

<a name="why-attributes-matter"></a>
## Why attributes matter

`bench.k` times 5,000 lookups with `?` (find) on identical data, sorted-attributed vs not
(2.4.1). Without the attribute find hashes the whole column every call; with it, it's a binary
search per lookup:

| rows | plain | sorted (`` `s``) | speedup |
|-----:|------:|-----------------:|--------:|
| 100 k | 3.2 ms | 0.41 ms | **7×** |
| 500 k | 16.8 ms | 0.94 ms | **17×** |
| 2 M | 184 ms | 2.0 ms | **93×** |
| 5 M | 827 ms | 2.9 ms | **288×** |

Results are identical; only the time differs. `asc` / `xasc` set the attribute for you, and
`meta` shows it in the `a` column.

---

<a name="language-notes"></a>
## Language notes (30-second version)

Amber uses a terse array notation. A few things worth knowing:

* **Two-argument library dyads work infix *or* in brackets.** Since 2.0.0, `x in y`, `t lj kt`,
  `` `sym xasc t``, `5 within 3 9`, `` "/" sv `a`b`c`` all work infix, exactly like q, and the
  bracket form `f[x;y]` and prefix form `f x` still work. The infix set is a curated list
  (`in within like lj ij uj aj aj0 wj wj1 pj ej cross inter union except ss sv vs xasc xdesc`) plus
  the built-in symbol verbs (``+ - * % ! & | < > = ~ , ^ # _ $ ? @ .``). Arbitrary user lambdas are
  **not** infix.
* **No `>=` / `<=`.** Write `~a<b` and `~a>b`.
* **qSQL is bare, at the prompt *and* in scripts.** Type `select … by … from … where …` (also
  `exec` / `update` / `delete`) with no `sel"…"` wrapper; bare column names like `wavg[sz;px]` just
  work. Since 2.0.0 this bare form also works inside a `.k` file loaded once the stdlib is up (the
  loader runs each file through the same rewriter the REPL uses); the `sel"…"` / `exq"…"` / `upd"…"`
  / `del"…"` string forms and the functional forms `qselect`/`qby`/`qwhere` still work too.
* **`peach[f;y]` is real multi-core.** It runs on a pool of `AMBER_THREADS` threads (default: the
  online CPU count; `=1` forces serial), so heavy per-item work scales across cores with no GIL.
* **Big vectors use the threads on their own.** Since 2.6, sort, grade, find, `in`, distinct, gather,
  group aggregates, float sums and fused float/int expressions split work over the same pool once
  a vector gets big (32K items and up). Small vectors stay on one thread, no overhead.
* **Grids preview Q-style.** `show t` prints the first `CROWS` rows (default 20) then `..`, with
  a dimmed `[N rows x M cols]` footer and ANSI syntax highlighting.
* **Errors show a `^` caret** under the failing token plus a descriptive message; set
  `AMBER_DIAG=1` for the full Rust-style report (see [Rust-style diagnostics](docs/REPL.md#rust-style-diagnostics)).
* **Symbols have no `_`.** Use a quoted symbol `` `"a_b" ``.
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

**Attributes.** Amber has all four q-style attributes in C: `` `sa`` sorted, `` `ua`` unique,
`` `pa`` parted, `` `ga`` grouped (`` `at`` reads them, `meta` shows them). Sorted/parted give
O(log n) kernel find; grouped + the group index give O(1) per-symbol slicing.

<a name="documentation"></a>
## Documentation

| | |
|---|---|
| [`docs/QUICKSTART.md`](docs/QUICKSTART.md) | the demo, the full install recipe, a worked tour, every command |
| [`docs/AMBER.md`](docs/AMBER.md) | the language reference: every verb, adverb and qSQL clause |
| [`docs/REPL.md`](docs/REPL.md) | line editing, `` / `st` / `	race`, error reports, grid modes |
| [`docs/INTERNALS.md`](docs/INTERNALS.md) | architecture, extensions, SIMD/parallel, Arrow, `libamber.so` |
| [`docs/BENCHMARKS.md`](docs/BENCHMARKS.md) | the full engine matrix, methodology, and the query files |
| [`docs/INSTALL.md`](docs/INSTALL.md) | shell configuration |
| [`docs/MISSING.md`](docs/MISSING.md) | what is not implemented yet, and why |
| [`CHANGELOG.md`](CHANGELOG.md) | every release |

<a name="whats-inside"></a>
## What's inside

| file | |
|------|--|
| `a`, `build.sh` | launcher (build-if-stale) and portable compile (gcc / clang) |
| `src/*.c`, `src/*.h` | the interpreter, ngn/k core + Amber extensions (`src/p.c` the `([]…)` parser; `src/ar.c` Arrow; `src/arena.{h,c}` the HFT arena, 32-byte aligned; `src/diagnostic.{h,c}` the Rust-style formatter; the native `aj` kernel in `src/a.c`; `src/inspect.{h,c}` the `\v` inspector; `src/ast.{h,c}` the `\ast` visualiser; `src/trace.{h,c}` the `\trace` profiler; `src/fmtutil.{h,c}` and `src/ansi.h` shared formatting/colour helpers; `src/simd.{h,c}` AVX2/NEON/scalar kernels; `src/parallel.{h,c}` the thread pool and its parallel kernels; `src/vm.{h,c}` the bytecode disassembler behind `\disasm`; `src/csv.{h,c}` the native CSV parser behind `` `csvr``) |
| `amber.k` | the q vocabulary (auto-loaded) |
| `repl.k` | the REPL, banner, grid rendering, `\grid`/`\clear`, help; CRLF-safe module loader; reads its input through `` `rdl`` (the native editor) and exposes the optional `ext.*` hooks |
| `src/ln.{h,c}`, `src/lnk.c` | the native line editor (raw `termios`, history, Tab completion) and the `` `rdl`` verb that the REPL reads through, this is what replaced `rlwrap` |
| `src/ext.{h,c}`, `ext/` | **both** extension seams. Sections 1-5: the in-process registry, runtime verbs plus `\`-command / editor / startup hooks, and the (empty by default) directory `build.sh` compiles out-of-tree extensions from. Section 6: the out-of-process **dynamic C API** behind `libamber.so` (`amber_init`, `amber_eval_str`, `amber_get_vector_ptr`, …) |
| `src/libamber.map` | the linker export map for the shared build: only `amber_*` and `am_ext_*` reach a host process's dynamic namespace, so the engine's terse internals (`mr`, `run`, `add`, …) cannot collide with a host's symbols |
| `fin.k` | finance / HFT module (auto-loaded), see `\m` help |
| `std.k` `qsql.k` `temporal.k` `sys.k` `hdb.k` `ipc.k` `tick.k` | modules (auto-loaded) |
| `examples/` | `tour.k` · `basics.k` · `tick.k` · `hft.k` · `peach.k` · `wj.k` · `graphs.k` · … |
| `test.k` `test-fin.k` `test-ext.k` | assertion suites (202 + 35 + 79) |
| `tests/harness.k` | shared assertion harness, `t` (value), `tv` (trapped expression), `te` (must-raise), `tk` (must-not-raise), `hexpect` (assertion-count guard), `hreport` |
| `tests/test_matrix.k` | **309-case combinatorial matrix**: every primitive × every element type × sizes 0 / 1 / 10 / 100 000+ (crossing the SIMD and `PAR_THRESHOLD` boundaries), asserted as invariants (shape, algebraic identity, vector-kernel-vs-scalar-reference) rather than frozen literals |
| `tests/test_qsql.k` | **117-case qSQL matrix**, written in the **bare `select … from t` syntax you type** (run through the same `qrw` rewrite the REPL and loader apply): the full `select`/`exec`/`update`/`delete` clause lattice, multi-key `by`, empty / single-row / heavily-duplicated tables, and malformed queries asserted to raise cleanly |
| `tests/fuzz.py` | malformed-input & deep-nesting crash fuzzer, asserts a clean K error, never a signal or a hang |
| `tests/test_capi.{c,sh}` | the dynamic C API: 81 assertions against `libamber.so`, linked as a satellite would link it (`src/ext.h` and nothing else from `src/`), run once at `-O2` and once under ASan + UBSan with leak detection, the ownership rules in the header are prose, and this is what checks them |
| `tests/test_qsql_script.sh`, `tests/test_comments.sh` | shell suites for the 2.0.0 loader work: bare qSQL inside a loaded `.k` file, and the unterminated bare-`/` comment now raising cleanly |
| `tests/run_tests.sh` | runs all of the above (`--asan` re-runs everything under ASan + UBSan) |
| `tests/test_repl_term.py` | **pty-driven REPL terminal suite**: asserts no `rlwrap:` diagnostic ever reaches a session, that `termios` is byte-for-byte restored after a normal exit *and* after `^C`, that the editing keys really edit, and that piped/non-tty behaviour is unchanged |
| `tests/test_ext_seam.sh`, `tests/ext_probe.c` | installs a miniature extension into `ext/`, checks the verb / `\`-command / `--help` hooks fire and that the engine's own suite is unaffected, then uninstalls it and checks the engine is back to stock |
| `tests/*.c` | standalone C test harnesses: `test_simd.c`/`test_parallel.c` (no Amber dependency), `test_ast.c` (links the full interpreter, ast.c is inherently built on Amber's real parser) |
| `bench.k` `bench-fin.k` `bench-std.k` `bench/` | attribute / index / window benchmarks; `bench/run_comparative.py` cross-engine harness (see [docs/BENCHMARKS.md §5](docs/BENCHMARKS.md)); `bench/queries/amber_*.k` and `bench/queries/k_*.k` are separate, independently-tuned scripts per engine, see [Comparative benchmark query files](docs/BENCHMARKS.md#comparative-benchmark-query-files) |
| `docs/` | `AMBER.md` (reference) · `MISSING.md` (roadmap + known leniencies) · `BENCHMARKS.md` |
| `CHANGELOG.md` | release history (2.0.0 first) |
| `.gitattributes` | forces LF checkout of sources so the REPL's line-based loader works on Windows too |

<a name="roadmap"></a>
## Roadmap

Amber covers a large slice of q. [docs/MISSING.md](docs/MISSING.md) is an honest map of what's next;
top picks: wiring the `` `g`` grouped attribute into the C find path; **attribute preservation
through ops** (keep/drop by q's per-op rules); the missing atom types
(`short`/`real`/`byte`/`guid`); a true partitioned / memory-mapped HDB; and **live REPL syntax
highlighting** (colouring tokens *as you type*, which needs `repl.k`'s raw-keystroke input loop
rewritten).

<a name="isolation"></a>
## Isolation

Amber is a single self-contained folder. The interpreter is named `amber` (never `k` or `q`),
built only inside the folder, never placed on your `PATH`. It reads/writes no config, no
`QHOME`, no dotfiles. Your other k and q installs are untouched; deleting the folder
uninstalls Amber completely.

<a name="licence"></a>
## Licence

GNU AGPLv3 (see [LICENSE](LICENSE)). Amber's interpreter core derives from **ngn/k**, an AGPLv3
K interpreter by ngn; that attribution is preserved in [NOTICE](NOTICE), as the licence requires.
Amber is an independent language and is not affiliated with, nor a distribution of, that project.
