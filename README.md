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
![version](https://img.shields.io/badge/version-1.9.2-orange)
![license](https://img.shields.io/badge/license-AGPLv3-blue)
![tests](https://img.shields.io/badge/tests-277%20passing-brightgreen)
![build](https://img.shields.io/badge/build-C99%20·%20portable%20·%20gcc%20+%20clang-informational)

</div>

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

New in **1.9.2**: integer `?` (find) builds an index over its left argument instead of scanning
it per probe, turning the inner-join benchmark from **180.95 ms into 5.66 ms (32x)**; float `+/`
uses four independent accumulators so it vectorises; array payloads are cache-line aligned. See
[docs/BENCHMARKS.md](docs/BENCHMARKS.md) for before/after and [docs/CHANGELOG.md](docs/CHANGELOG.md).

New in **1.9.1**: the `select … by … from` query layer now groups and probes on **raw column
vectors** instead of boxing one K object per row, making group-by **24.7x** faster and inner
join **19.3x** faster (both now within ~1.1-1.5x of hand-written Amber array code); the CBQN
benchmark scripts compile and self-time correctly. See [docs/CHANGELOG.md](docs/CHANGELOG.md).

New in **1.9**: a **native C `aj` as-of-join kernel** (branch-free `lower_bound` over sorted
nanosecond timestamps) · an **HFT zero-allocation arena** (thread-local 16 MB bump allocator
that removes `malloc`/`free` jitter from the eval hot path) · **Rust-style visual diagnostics**
(`AMBER_DIAG=1` prints gutter-aligned, ANSI-coloured `error[…]` reports with `^^^` underlines) ·
an options/market-data generator (`genopt`, `gentq`) · Unicode grid modes (`\grid clean|rounded|sharp|heavy`) ·
and a **CRLF-safe REPL loader** so a Windows checkout runs the library unchanged.

Also new: a **SIMD kernel library** (AVX2 / ARM NEON / scalar) behind `+`-style vector ops ·
a **multithreaded vector engine** that splits large arrays across cores · a **bytecode
disassembler** (`\disasm`) for Amber's real compiler/VM · and a **native CSV parser**
(`` `csvr``) that reads a file straight into a typed table. See
[Engine extensions](#engine-extensions) below.

```q
t:([]sym:`AAPL`MSFT`AAPL; px:187.3 411.2 187.4; sz:100 250 50)   / a table, rendered instantly
qby[t; `sym; (,`vwap)!,{wavg[x`sz;x`px]}]                         / vwap by symbol
```

---

## 🚀 Quick Showcase

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
gentq N                                                    / N trades, 2N quotes, realistic microstructure
qby[trades; `sym; (,`vwap)!,{round[4]wavg[x`sz;x`px]}]      / VWAP per symbol
ema[2%51; pxSeries]                                          / 50-period EMA, tacit call into the C kernel
taq[trades; quotes]                                          / as-of join: every trade -> its nearest quote
```

Sample benchmark table from a real run (row counts and timings will vary by machine —
the script prints its own table every time):

```
== benchmark summary =======================================
stage      ms
----------
gentq   2381.9
vwap     151.2
ema        0.1
asof      704.0
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

Prefer to explore interactively? Open `notebooks/Amber-Notebook-Studio.html` directly in a
browser (no build step) and hit **🚀 Load HFT Demo** in the header — it generates a tick
session, charts price & volume on a canvas, and benchmarks a naive per-symbol filter against
the vectorised `qby` call, right there in the page.

---

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
sudo apt-get install -y rlwrap                                    # optional: history / arrow keys
chmod +x a build.sh install.sh                                    # restore exec bits if the copy dropped them
./a                                                               # builds, then opens the REPL
```

**macOS** (Intel or Apple Silicon) — needs Apple's `clang`:

```sh
xcode-select --install        # installs the Command Line Tools (clang); one-time
brew install rlwrap           # optional: history / arrow keys (needs Homebrew)
chmod +x a build.sh install.sh
./a
```

That's it — `./a` compiles the interpreter (portable `-O3`, no `-march=native`) and drops you at
the prompt; it recompiles automatically whenever the C sources change, so you never run a stale
build. If `./a` prints **`Permission denied`**, the executable bit was lost in transfer — the
`chmod +x` line above fixes it (or just run `bash a`).

The default build is portable C99 and always includes `-pthread` (needed by the multithreaded
vector engine) and the scalar SIMD fallback. For a machine-specific build that turns on **AVX2**
(x86_64) or **NEON** (Apple Silicon / any `aarch64`) vector kernels, set `AMBER_NATIVE=1`:

```sh
AMBER_NATIVE=1 ./build.sh      # adds -march=native; check with: `simd 0
```

`` `simd 0 `` prints which backend actually got selected (`scalar` / `avx2` / `neon`) to stderr.
NEON activates unconditionally on Apple Silicon regardless of `AMBER_NATIVE`, since `aarch64`
implies it; `AMBER_NATIVE=1` on Apple Silicon additionally unlocks `-march=native` tuning.

One-shot alternative: `bash install.sh` builds, runs the self-test, and adds an `a` alias to your
shell rc (`~/.zshrc` on macOS, `~/.bashrc` on Linux). To add the alias by hand:

```sh
echo "alias a='$PWD/a'" >> ~/.zshrc     # macOS (zsh);  use ~/.bashrc on Linux, then: source it
```

Nothing is installed system-wide — see [Isolation](#isolation).

---

## A quick taste

> These snippets are written as you'd type them at the interactive prompt (`./a`), where a bare
> table auto-renders as a grid and qSQL sugar (`select … by … from … where …`) works directly on
> the input line. Inside a `.k` script run via `./amber file.k`, wrap a bare table in `show`
> (`show t`) for the grid view, and use the `sel"…"` string form (or `qselect`/`qby`/`qwhere`
> directly) for qSQL — exactly the pattern used throughout `examples/*.k` and `test.k`.

```q
/ tables are first-class and render without `show` -- at the interactive prompt
([]sym:`a`b`c; px:100 200 300)
/  sym px
/  -------
/  a   100
/  b   200
/  c   300

meta ([]sym:`a`b; px:1.5 2.5)      / column types + attributes (c | t a)

/ the join every tick shop needs — as-of (native C kernel in 1.9)
trade:([]sym:`a`b`a; time:3 4 9; px:100 200 300)
quote:([]sym:`a`a`b`a; time:1 5 2 8; bid:10 11 20 12)
aj[`sym`time; trade; quote]        / last quote at/ before each trade

/ qSQL — type select/exec/update/delete straight, no sel"…" wrapper
select last px by sym from trade   / grouped aggregate
select from trade where px>150     / filter rows

/ 1-minute OHLCV bars (classic tickerplant query)
tb:+@[+trade; ,`time; minbar[1]@]
qby[tb; `sym`time; `o`h`l`c`v!({first x`px};{max x`px};{min x`px};{last x`px};{sum x`sz})]

/ sorted attribute => binary-search lookups
v:asc 2000000?1000000000                     / `s attribute set by asc
`at v                                        / `s
v ? 12345 67890                              / O(log n)  (see bench.k: ~1000x faster)

/ multi-core: peach runs f over items in parallel worker processes (no GIL)
peach[{avg x?1.0}; 8#1000000]                / 8 heavy tasks across AMBER_THREADS cores
```

Big tables print Q-style — the first `CROWS` rows (default 20) then `..`; set `CROWS:10`
to shorten. The cap is applied before formatting, so previewing a million-row table is
instant.

Check the interpreter version, or list every option and REPL command:

```sh
./amber --version           # amber 1.9.2
./amber --help              # options + the full \-command reference
```

Run the guided tours:

```sh
./amber examples/tour.k     # a worked example of EVERY function
./amber examples/basics.k   # a 2-minute intro
./amber examples/tick.k     # realistic trades & quotes: as-of/window joins, VWAP, OHLC
AMBER_THREADS=8 ./amber examples/peach.k   # multi-core speedup demo (serial vs peach)
./amber bench.k             # attribute speed benchmark
./amber test.k              # core suite (163); also test-fin.k (35) + test-ext.k (79)
tests/run_tests.sh          # EVERYTHING: build + all K suites + C unit tests + fuzz pass
./amber tests/test_matrix.k # the suites are self-locating - run them from any directory
tests/run_tests.sh --asan   # ... and re-run it all under AddressSanitizer + UBSan
bash bench/run.sh           # cross-engine sanity + speed (Amber vs numpy/pandas/…; see BENCHMARKS.md)
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
nothing about the default output. You can exercise the formatter directly from Amber with the
`` `dgn `` self-test builtin (returns `1` when the rendered report matches its expected shape):

```q
`dgn 0        / 1  — diagnostic formatter self-test
```

Turn it on for a session and leave it: it costs one `getenv` per error and never fires on
success.

---

### Turning the report off

The diagnostic is rendered when the error is *created*, so code that catches an error with
`.[f;args;handler]` still sees it on stderr. Since **1.9** that is switchable at runtime —
useful for anything that provokes errors on purpose (a test suite, `protect`, a retry loop):

```
prev:`diag 0        / suppress the report, returns the previous setting (1)
.[{1+`a};,0;{"caught"}]     / no output at all
`diag prev          / restore
```

The compact `'type` caret line is not suppressed — it is buffered and still handed to the trap
handler and to `` `err``, so a caught error can always be inspected. `AMBER_DIAG=0` in the
environment still works and seeds the initial value.

## REPL diagnostics (`\v` · `\ast` · `\trace`)

Three zero-dependency session commands for inspecting the workspace and the evaluator itself —
none of them touch `eval`/`arena`/core REPL behaviour; they only read state and print a report.

**`\v`** — a rich workspace inspector: every currently-defined global as an ASCII table
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

`\v` lists *every* global in scope, which after `repl.k`'s modules are loaded includes the
REPL/library's own internal state (`repl.*`, `PAL`, `GB`, `OUNI`, …) alongside your own — scan
for the names and types you defined, or `\d yourns` first to narrow the namespace.

**`\ast`** — a colour-coded parse tree (parse-only; nothing is executed, with one unavoidable
exception — see below):

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
Amber's tacit forms get their own explicit labels — a lambda literal shows its real source text,
a 2- or 3-verb train is an explicit **Hook**/**Fork**, and a curried/partial application (`1+`,
`f[x;;z]`) is an explicit **Projection** with a `Blank` node standing in for the omitted argument:

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

(A lambda shows its *source text*, never its bytecode — pk() itself compiles `{...}` literals
eagerly at parse time, the one exception to "nothing is executed"; disassembling what it compiled
to is `\disasm`'s job, not `\ast`'s.)

Verbs (bare, applied, curried, or the head of a call) render bold cyan, adverbs bold magenta,
numeric/literal scalars bright green, variables and symbols yellow, and the tree connectors
themselves dim gray; list literals, statement blocks, and tacit hook/fork train labels get their
own restrained accent colour so the shape of an expression still reads at a glance.

**`\trace`** — a 4-phase execution profiler (parse → arena setup → execute → format), with a
Unicode bar chart and the arena's peak scratch usage for that one evaluation. It prints the
expression's normal result first, then the report (timings vary run to run):

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

Since **1.9** the timer prints `ns` / `us` / `ms` as appropriate (sub-microsecond phases used to
render as a flat `0us`), and **Arena peak** is a true high-water mark taken from
`arena_peak()` — it used to be `max(used-before, used-after)`, and since every arena consumer
rewinds the slab before returning, that was `0 B` for literally every expression. Only
expressions that actually reach an arena-backed kernel (`aj`, `wj`, `` `csvr``, `\ast`) report a
non-zero peak:

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

---

<a name="engine-extensions"></a>
## Engine extensions: SIMD · parallel · disassembler · CSV

Four additive engine modules, each a standalone `.c`/`.h` pair that never touches core
evaluation (`a.c`'s dispatch, `b.c`'s compiler/VM, or the `+`/`*`/`+/` verb implementations).
One of them is an honest reinterpretation of the original ask, explained inline below.

(A fifth module, a `\hl <expr>` command that echoed one line back with ANSI syntax colour, was
removed — it only ever colorized a line you explicitly ran, not your keystrokes as you typed
them, which isn't what "live syntax highlighting" means. Genuine live/incremental highlighting
would require rewriting `repl.k`'s raw-keystroke input loop, which is out of scope for now; see
[Roadmap](#roadmap).)

**SIMD vector kernels** (`src/simd.{h,c}`) — `simd_add_i64/f64`, `simd_mul_i64/f64`,
`simd_sum_i64/f64` operate on plain `int64_t*`/`double*` arrays with an AVX2 path
(`<immintrin.h>`, x86_64), a NEON path (`<arm_neon.h>`, any `aarch64` — Apple Silicon
included), and a scalar C99 fallback, selected at compile time. `simd_backend()` reports which
one is active. The arena allocator (`src/arena.c`) was bumped to genuine **32-byte alignment**
via `posix_memalign` (previously 16-byte, from plain `malloc`) so SIMD loads over arena-backed
buffers are always aligned. Self-test + benchmark: `` `simd 0 `` (prints backend, size, and a
SIMD-vs-scalar timing comparison to stderr, returns `1` on success):

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
nulls via `#`/`~`/`@` — the same primitives `meta`/`qwhere` already rely on).

**Honest deviations, stated plainly:** (1) no `src/compiler.c` was added — `b.c` already *is*
the real compiler and VM, so a second one would be redundant/misleading; `vm.c` disassembles
the real bytecode instead. (2) the NEON path was written against the real ARM64 NEON intrinsics
and reviewed carefully, but this sandbox has no ARM cross-compiler available to actually build
and run it — it has not been executed on real Apple Silicon hardware.

---

## HFT toolkit — native `aj`, arena, generators

Amber 1.9 tightens the tick/quant path:

```q
gentq 100000                       / generate a full session: sets globals `trades` and `quotes`
genopt 2000                        / generate a random option chain into global `options`
m:aj[`sym`time; trades; quotes]    / TAQ: prevailing quote for every trade (native C kernel)
select from options where abs[strike-spot]<5     / near-the-money contracts
```

* **Native `aj` kernel.** `aj`/`aj0` now match each trade to its most-recent quote with a
  **branch-free `lower_bound`** binary search over each symbol group's sorted nanosecond
  timestamp slice (`src/a.c`, marshalled from `amber.k`). The pure-K reference (`ajmK`) is kept
  alongside it. Correct on 64-bit ns timestamps, empty groups, and no-match rows (→ null).
* **Zero-allocation arena.** A thread-local **16 MB bump allocator** (`src/arena.{h,c}`:
  `arena_init` / `arena_alloc` / `arena_reset` / `arena_free`) supplies transient scratch during
  evaluation and is rewound once per eval cycle, so per-tick work does not thrash the system
  `malloc`/`free` and the latency jitter they cause stays out of the hot path. Self-test:
  `` `arn 0 `` → `1`.

---

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

## Apache Arrow C Data Interface (zero dependency)

Interop with **PyArrow / Polars / DuckDB** over the stable Arrow C ABI — no `libarrow`
linkage. Export is **zero-copy**:

```q
p:arrow.export t                    / table  -> (schemaAddr; arrayAddr)  64-bit C-ABI pointers
arrow.import p                      / (schemaAddr; arrayAddr) -> Amber table
```

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

**How fairness is enforced, not just claimed.** Every answer in this suite is an integer that
fits in float64 exactly, and every sum is over such integers, so the result is independent of
summation order — SIMD pairwise, Kahan-compensated and naive left-fold summation all produce the
identical bit pattern. The harness therefore compares answers **exactly** against the C
reference and prints **WRONG** in place of a time for any engine that disagrees. Each engine also
emits a checksum of its *input* data, so a divergence in the generator is caught separately
(**BADDATA**). You cannot win a cell in this table by computing something cheaper.

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
DuckDB and — since 1.9.1 — CBQN, via `•MonoTime`) do so with a monotonic clock after warm-up
passes, and the harness uses that number directly. Engines with no usable in-language clock
(ngn/k, Uiua, J) are measured as *total process time − a measured startup baseline*. The results
table labels which mode produced each cell, so the two are never silently mixed.

**Known lexer quirk hit while writing these files:** a `.k` comment line containing *only* a
bare `/` (no trailing space or text) silently truncates parsing of everything after it, with no
error. `bench/queries/*.k` works around this with blank lines instead of bare `/` separators;
see [docs/MISSING.md](docs/MISSING.md) for this tracked as a known engine bug.

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

## Language notes (30-second version)

Amber uses a terse array notation. A few things that differ from kdb+/q:

* **Two-argument library functions take brackets:** `aj[c;x;y]`, `lj[t;kt]`, `in[x;y]`,
  ``xasc[`sym;t]``. Built-in symbols (``+ - * % ! & | < > = ~ , ^ # _ $ ? @ .``) are still infix.
* **No `>=` / `<=`** — write `~a<b` and `~a>b`.
* **qSQL is bare — at the prompt:** type `select … by … from … where …` (also `exec` / `update` /
  `delete`) with no `sel"…"` wrapper — bare column names like `wavg[sz;px]` just work. This
  line-level rewriting is a REPL convenience; inside a `.k` script use `sel"select …"` (also
  `exq"…"` `upd"…"` `del"…"`) or the functional forms `qselect`/`qby`/`qwhere` directly.
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
[REPL diagnostics](#repl-diagnostics-v--ast--trace) below), and `\disasm` for the bytecode
disassembler (see [Engine extensions](#engine-extensions)).

---

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

## What's inside

| file | |
|------|--|
| `a`, `build.sh` | launcher (build-if-stale) and portable compile (gcc / clang) |
| `src/*.c`, `src/*.h` | the interpreter — ngn/k core + Amber extensions (`src/p.c` the `([]…)` parser; `src/ar.c` Arrow; `src/arena.{h,c}` the HFT arena, 32-byte aligned; `src/diagnostic.{h,c}` the Rust-style formatter; the native `aj` kernel in `src/a.c`; `src/inspect.{h,c}` the `\v` inspector; `src/ast.{h,c}` the `\ast` visualiser; `src/trace.{h,c}` the `\trace` profiler; `src/fmtutil.{h,c}` and `src/ansi.h` shared formatting/colour helpers; `src/simd.{h,c}` AVX2/NEON/scalar kernels; `src/parallel.{h,c}` the pthreads vector engine; `src/vm.{h,c}` the bytecode disassembler behind `\disasm`; `src/csv.{h,c}` the native CSV parser behind `` `csvr``) |
| `amber.k` | the q/kdb+ vocabulary (auto-loaded) |
| `repl.k` | the REPL — banner, grid rendering, `\grid`/`\clear`, help; CRLF-safe module loader |
| `fin.k` | finance / HFT module (auto-loaded) — see `\m` help |
| `std.k` `qsql.k` `temporal.k` `sys.k` `hdb.k` `ipc.k` `tick.k` | modules (auto-loaded) |
| `examples/` | `tour.k` · `basics.k` · `tick.k` · `hft.k` · `peach.k` · `wj.k` · `graphs.k` · … |
| `test.k` `test-fin.k` `test-ext.k` | legacy assertion suites (163 + 35 + 79) |
| `tests/harness.k` | shared assertion harness — `t` (value), `tv` (trapped expression), `te` (must-raise), `tk` (must-not-raise), `hexpect` (assertion-count guard), `hreport` |
| `tests/test_matrix.k` | **309-case combinatorial matrix**: every primitive × every element type × sizes 0 / 1 / 10 / 100 000+ (crossing the SIMD and `PAR_THRESHOLD` boundaries), asserted as invariants (shape, algebraic identity, vector-kernel-vs-scalar-reference) rather than frozen literals |
| `tests/test_qsql.k` | **94-case qSQL matrix**, written in the **bare `select … from t` syntax you actually type** (run through the same `qrw` rewrite the REPL applies): the full `select`/`exec`/`update`/`delete` clause lattice, multi-key `by`, empty / single-row / heavily-duplicated tables, and malformed queries asserted to raise cleanly |
| `tests/fuzz.py` | malformed-input & deep-nesting crash fuzzer — asserts a clean K error, never a signal or a hang |
| `tests/run_tests.sh` | runs all of the above (`--asan` re-runs everything under ASan + UBSan) |
| `tests/*.c` | standalone C test harnesses: `test_simd.c`/`test_parallel.c` (no Amber dependency), `test_ast.c` (links the full interpreter — ast.c is inherently built on Amber's real parser) |
| `bench.k` `bench-fin.k` `bench-std.k` `bench/` | attribute / index / window benchmarks; `bench/run_comparative.py` cross-engine harness (Amber vs DuckDB vs CBQN vs ngn/k — see [docs/BENCHMARKS.md §5](docs/BENCHMARKS.md)); `bench/queries/amber_*.k` and `bench/queries/k_*.k` are separate, independently-tuned scripts per engine (not the same file reused), each `amber_*.k` documenting in its header what optimization was tried, what was measured, and why — see [Comparative benchmark query files](#comparative-benchmark-query-files) |
| `docs/` | `AMBER.md` (reference) · `MISSING.md` (roadmap + known leniencies) · `CHANGELOG.md` (history) · `BENCHMARKS.md` · `AUDIT-1.9.md` (the 1.9 security/correctness audit report) |
| `.gitattributes` | forces LF checkout of sources so the REPL's line-based loader works on Windows too |

## Roadmap

Amber covers a large slice of q. [docs/MISSING.md](docs/MISSING.md) is an honest map of what's next —
top picks: a **binary serialiser** (`` -8!``/`` -9!``) to replace the text transfer that `peach`,
IPC and the on-disk layer use; wiring the `` `g`` grouped attribute into the C find path; the
missing atom types (`short`/`real`/`byte`/`guid`); and a true partitioned / memory-mapped HDB.

<a name="isolation"></a>
## Isolation

Amber is a single self-contained folder. The interpreter is named `amber` (never `k` or `q`),
built only inside the folder, never placed on your `PATH`. It reads/writes no config, no
`QHOME`, no dotfiles. Your kdb+, kona and other k/q installs are untouched; deleting the folder
uninstalls Amber completely.

## Licence

GNU AGPLv3 (see [LICENSE](LICENSE)). Amber's interpreter core derives from **ngn/k**, an AGPLv3
K interpreter by ngn; that attribution is preserved in [NOTICE](NOTICE), as the licence requires.
Amber is an independent language and is not affiliated with, nor a distribution of, that project.
