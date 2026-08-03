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
![version](https://img.shields.io/badge/version-1.9-orange)
![license](https://img.shields.io/badge/license-AGPLv3-blue)
![tests](https://img.shields.io/badge/tests-272%20passing-brightgreen)
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

New in **1.9**: a **native C `aj` as-of-join kernel** (branch-free `lower_bound` over sorted
nanosecond timestamps) · an **HFT zero-allocation arena** (thread-local 16 MB bump allocator
that removes `malloc`/`free` jitter from the eval hot path) · **Rust-style visual diagnostics**
(`AMBER_DIAG=1` prints gutter-aligned, ANSI-coloured `error[…]` reports with `^^^` underlines) ·
an options/market-data generator (`genopt`, `gentq`) · Unicode grid modes (`\grid clean|rounded|sharp|heavy`) ·
and a **CRLF-safe REPL loader** so a Windows checkout runs the library unchanged.

```q
t:([]sym:`AAPL`MSFT`AAPL; px:187.3 411.2 187.4; sz:100 250 50)   / a table, rendered instantly
qby[t; `sym; (,`vwap)!,{wavg[x`sz;x`px]}]                         / vwap by symbol
```

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

One-shot alternative: `bash install.sh` builds, runs the self-test, and adds an `a` alias to your
shell rc (`~/.zshrc` on macOS, `~/.bashrc` on Linux). To add the alias by hand:

```sh
echo "alias a='$PWD/a'" >> ~/.zshrc     # macOS (zsh);  use ~/.bashrc on Linux, then: source it
```

Nothing is installed system-wide — see [Isolation](#isolation).

---

## A quick taste

```q
/ tables are first-class and render without `show`
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

Run the guided tours:

```sh
./amber examples/tour.k     # a worked example of EVERY function
./amber examples/basics.k   # a 2-minute intro
./amber examples/tick.k     # realistic trades & quotes: as-of/window joins, VWAP, OHLC
AMBER_THREADS=8 ./amber examples/peach.k   # multi-core speedup demo (serial vs peach)
./amber bench.k             # attribute speed benchmark
./amber test.k              # core suite (158); also test-fin.k (35) + test-ext.k (79) = 272
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
  `xasc[`sym;t]`. Built-in symbols (`+ - * % ! & | < > = ~ , ^ # _ $ ? @ .`) are still infix.
* **No `>=` / `<=`** — write `~a<b` and `~a>b`.
* **qSQL is bare:** type `select … by … from … where …` (also `exec` / `update` / `delete`)
  with no `sel"…"` wrapper — bare column names like `wavg[sz;px]` just work.
* **`peach[f;y]` is real multi-core** — it forks `AMBER_THREADS` worker processes (default 4;
  `=1` forces serial), so heavy per-item work scales across cores with no GIL.
* **Grids preview Q-style** — `show t` prints the first `CROWS` rows (default 20) then `..`, with
  a dimmed `[N rows x M cols]` footer and ANSI syntax highlighting.
* **Errors show a `^` caret** under the failing token plus a descriptive message; set
  `AMBER_DIAG=1` for the full Rust-style report (see [Rust-style diagnostics](#rust-style-diagnostics)).
* **Symbols have no `_`** — use a quoted symbol `` `"a_b" ``.
* Tables: `([]col:vals;…)`; keyed tables: `([key:vals]col:vals)`. A bare table at the prompt
  auto-renders as a grid.

Full reference: **[docs/AMBER.md](docs/AMBER.md)**. Built-in help: `\` then `\q \j \z` for the Amber
vocabulary, `\0 \+ \' \`` for the core.

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
| `src/*.c`, `src/*.h` | the interpreter — ngn/k core + Amber extensions (`src/p.c` the `([]…)` parser; `src/ar.c` Arrow; `src/arena.{h,c}` the HFT arena; `src/diagnostic.{h,c}` the Rust-style formatter; the native `aj` kernel in `src/a.c`) |
| `amber.k` | the q/kdb+ vocabulary (auto-loaded) |
| `repl.k` | the REPL — banner, grid rendering, `\grid`/`\clear`, help; CRLF-safe module loader |
| `fin.k` | finance / HFT module (auto-loaded) — see `\m` help |
| `std.k` `qsql.k` `temporal.k` `sys.k` `hdb.k` `ipc.k` `tick.k` | modules (auto-loaded) |
| `examples/` | `tour.k` · `basics.k` · `tick.k` · `hft.k` · `peach.k` · `wj.k` · `graphs.k` · … |
| `test.k` `test-fin.k` `test-ext.k` | 272-assertion suite (158 + 35 + 79) |
| `bench.k` `bench-fin.k` `bench-std.k` `bench/` | attribute / index / window benchmarks; cross-engine harness |
| `docs/` | `AMBER.md` (reference) · `MISSING.md` (roadmap) · `CHANGELOG.md` (history) · `BENCHMARKS.md` |
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
