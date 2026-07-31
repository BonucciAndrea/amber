# Changelog

## 1.7
- **Native temporal types.** `date` (days since 2000.01.01), `time` (ms of day) and
  `timestamp` (ns since 2000.01.01) are now first-class C-level types with their own type
  tags. Literal syntax parses directly — `2026.07.30`, `10:00:05.000`,
  `2026.07.30D09:30:00.000000000` — and values auto-display in their own format.
  Type-aware arithmetic: `10:00:00.000+00:00:05.000` → `10:00:05.000`, `date-date` → days,
  `date+n` → date, plus comparisons. String casts `"D"$`/`"T"$`/`"P"$`, accessors
  `year`/`month`/`day`/`dow`/`thh`/`tmm`/`tss`, and `` `i$`` to extract the raw value.
  Columns keep numeric storage (as kdb does) so `xasc`/`s#` and name-based `time`-column
  display continue to work. Implemented across `a.h` (enum + type tables + `TU`/`TP` macros),
  `p.c` (literal scanner), `s.c` (formatter), `2.c` (arithmetic), `c.c` (casts).
- **C-kernel window join.** `wj` moved from an interpreted per-row K loop to a C routine
  (`wjc` in `i.c`): a vectorised binary-search range probe per row plus a contiguous slice
  sweep for the standard reducers (`first last min max sum avg count`). ~2× faster in a
  throttled sandbox (more on real cores), bit-exact vs the interpreted `wjK` fallback for the
  non-floating reducers. Arbitrary aggregators fall back to `wjK`.
- **C-kernel `ema`** — the exponential moving average was an interpreted per-element scan;
  now a single O(n) C sweep (`emaC`), verified identical to the K version.
- **SIMD math.** Portable vectorization pragmas (`clang loop vectorize` / `GCC ivdep`) on the
  hot elementwise float kernels in `2.c`.
- **Terminal charting.** `plot` draws a numeric vector as a 2×4 **Braille** line chart
  (Bresenham into a bitmask canvas, min/max-scaled with Y-axis labels); `candle` draws an
  OHLC table as **Unicode candlesticks** with ANSI colour (green up / red down), box-drawing
  wicks and block bodies, one space between candles. Exposed in `sys.k`; C kernels
  `plotC`/`candleC` in `i.c`. See `examples/graphs.k` for a 13-chart tour.
- **Apache Arrow C Data Interface (zero dependency).** `arrow.export` / `arrow.import`
  interop with PyArrow / Polars / DuckDB over the stable Arrow C ABI with no `libarrow`
  linkage. `ArrowSchema`/`ArrowArray` structs are embedded in `a.h`; export is zero-copy
  (child `buffers[1]` alias Amber column payloads and a `release` callback `mr`s them);
  import parses format strings, applies validity bitmaps as nulls, and rebuilds a table
  (a copy — Amber's inline object header precludes aliasing a foreign buffer). New file
  `ar.c`. Numeric widths, ranges and symbol (utf8) columns round-trip exactly.
- **Cleanup.** Removed the dead duplicate `gentq`; interpreted `wj` retained as `wjK`
  (arbitrary-aggregator fallback). Test suite now **267** (153 + 35 + 79), 0 failures.

## 1.6
- **Q-style grid preview.** `show t` (and a bare table/keyed-table/dict at the prompt) now
  prints only the first `CROWS` rows (default 20) followed by a `..` line, instead of dumping
  the whole thing. The cap is applied *before* formatting, so previewing a million-row table
  is instant. Set `CROWS:n` at the prompt to change it (e.g. `CROWS:10`); small tables print
  in full. Implemented in `amtab`/`amkeyed`/`amdict` (amber.k).
- **`examples/peach.k`** — a runnable Monte-Carlo demo that times serial `` f'y `` vs
  `peach[f;y]` so you can see the multi-core speedup on your own hardware
  (`AMBER_THREADS=8 ./amber examples/peach.k`).
- **Banner shows v1.6** and advertises bare qSQL + parallel `peach`.
- **`peach` is now genuinely parallel (multi-core), in C.** `peach[f;y]` forks
  `AMBER_THREADS` worker processes (default 4), each applies `f` to a slice of `y`,
  serialises its result (`` `k ``) down a pipe, and the parent deserialises (`val`) and
  concatenates. Because `(. `k v) ~ v` holds for every value, the result is identical to
  serial `` f'y `` — verified across vectors, symbols, tables, nested and ragged results, and
  a 200-iteration stress run; all 226 tests still pass. Set `AMBER_THREADS=N` to control the
  worker count (`=1` forces serial). Implemented as a new C primitive (`peachC` in `i.c`,
  wired through the `sym1` system-function table) — additive, so it can't affect the serial
  core. Fork-based (copy-on-write heap) so there are **no data races**: this matches how
  kdb+ gets multi-core and avoids the atomic-refcount tax that shared-memory threading would
  put on all single-threaded code. Unlike Python threads (GIL-bound), Amber's workers run on
  all cores at once. Best for coarse-grained, compute-heavy per-item work; see BENCHMARKS.md
  for when the fork/serialise overhead makes serial `'` the better choice.

## 1.5
- **The v1.5 extended modules now actually load in the REPL.** `repl.k` previously loaded only
  `amber.k` + `fin.k`, so `std`/`qsql`/`temporal`/`sys`/`hdb`/`ipc` — and therefore `sel`,
  `select … by …`, the moving aggregates, temporal helpers, etc. — were silently unavailable at
  the prompt. `repl.k` now loads all six in dependency order.
- **Bare qSQL.** You can type `select … by … from … where …`, `exec`, `update`, and `delete`
  directly — no `sel"…"` wrapper. A reader rewriter (`qrw` in `qsql.k`) turns such a line into
  the matching `sel/exq/upd/del "…"` call before evaluation; it also handles assignment
  (`r:select …`) and prefixes (`5#select …`, `count select …`), and leaves ordinary
  expressions and the explicit string forms untouched. Documented on the `\j` help page and in
  AMBER.md §7.
- **`select from t` fix.** `sel` now parses a column-less `select from t [where …]` (a leading
  `from`, all columns) correctly instead of mis-splitting the clause.
- **Benchmarks + sanity harness.** New `BENCHMARKS.md` and `bench/` (Amber vs numpy/pandas here;
  auto-runs growler/k, kdb+/q, DuckDB, Polars where installed — `bench/run.sh`). All aggregation
  results cross-check exactly against numpy/pandas; Amber's `group-by` and `distinct` beat pandas.
- **Faster as-of join.** `aj`'s matcher (`ajm`) now issues **one vectorised `bin` per group**
  (`'` on a sorted noun) instead of a per-row scalar search — ~6× faster (≈700→115 ms at 50 k
  rows), identical results, all 226 tests still pass. (`wj` still uses the per-row form.)
- **`build.sh` defaults to portable `-O3 -flto`** (was `-O2`). Link-time optimisation is
  auto-detected with a safe fallback if the compiler lacks it; `filter` ~2× faster and the
  hash/group kernels ~20% faster, with no `-march` so the binary stays portable.
  `AMBER_NATIVE=1 ./a` opts into `-march=native -funroll-loops` (group-by roughly halves again).
  PGO was tested and dropped — the gain was noisy and not worth the two-pass build. See
  BENCHMARKS.md.

## 1.4.1
- **`gentq` now marks both key columns**: `time` gets the `` `s`` sorted attribute and `sym`
  gets `` `p`` parted — both visible in `meta trades` / `meta quotes` (via `fin.k`'s
  `sortcol`/`partcol` helpers). `test-fin.k` asserts both (now 35 tests).
- **Help expanded**: the `\z` page documents all four attributes (`` `sa`` `` `ua`` `` `pa``
  `` `ga`` + `` `at``, with complexity), and a new **`\m` page** documents the whole `fin.k`
  HFT vocabulary. Added to the main `\` menu.

## 1.4
- **`fin.k` — a financial / HFT module** (auto-loaded after `amber.k`): a fixed `gentq[n]`
  that sets global `trades`/`quotes` tables with **numeric times** and attributes on the key
  columns (`` `s`` on `time`, `` `p`` on `sym`); an O(1) grouped index (`bysym`/`symrows`);
  order-book analytics (`mid` `spread`
  `spreadbps` `micro` `imbal`); trade analytics (`vwap` `twap` `tsign` `signedvol`
  `effspread` `notional`); returns/vol (`ret` `logret` `rvol` `movavg` `movsum` `movmax`
  `movmin` `ema` `rollstd`); `bars` (OHLCV) and `symstats`; and `pt` (time-formatted print).
- **All four kdb attributes now exist in C**: `` `sa`` (sorted), `` `ua`` (unique),
  `` `pa`` (parted), `` `ga`` (grouped); `` `at`` reports `s`/`u`/`p`/`g`; `meta` shows them.
  Sorted **and parted** columns get O(log n) kernel find; grouped + the group index give O(1)
  per-symbol slicing (see `bench-fin.k`: ~20,000x vs a linear scan).
- **Join fix**: joins require **numeric** time columns — store times as ms and format only for
  display (`tsym`/`pt`). `stime` on the stored column breaks `aj` (it makes time a string).
- New: `examples/hft.k` (full HFT walkthrough), `examples/attributes.k`,
  `examples/practice.k`, `test-fin.k` (32 tests), `bench-fin.k`. Core suite: 153 tests.


## 1.3
- **Fixed table/grid rendering** used by both `show` and the bare-expression REPL:
  `amdict` now handles list-valued dicts (e.g. `group`), `amcells` renders nested columns
  (e.g. `xgroup`), and `iskeyed` no longer crashes on plain vectors (it used odometer on
  non-dicts). `all`/`any` no longer rely on the unsupported `` `b$`` cast.
- **`./a` rebuilds when the C sources are newer than the binary** — no more running a stale
  build (this was why `([]…)` tables didn't render for some copies).
- **New banner** with a techy subtitle; **`examples/tour.k`** shows a worked, tested example of
  every function; test suite grown to **148 assertions** covering essentially the whole library.
- Rewrote the GitHub README.


## 1.2
- **`([]col:vals;…)` table literals** and **`([key:vals]col:vals)` keyed-table literals**,
  implemented in the C parser (`p.c`) — build tables the q way, no `+…!(…)` needed.
- **Own identity**: every reference to the upstream array core has been removed from the code,
  banner, help and docs; the language stands alone as Amber. (AGPLv3 attribution is retained in
  `NOTICE`, as the licence requires.)
- **New banner** — a clean wordmark, no clutter.
- **`bench.k`** — attribute speed harness (find/`in`, sorted vs unsorted, across sizes).
- **`MISSING.md`** — an honest map of kdb+/q features not yet in Amber, with a roadmap.
- **`round[d;x]`**, table-literal tests; suite now 104 assertions.


## 1.1
- **Temporal / tick support**: `hms hh mm sec milli minute second stime ptime` (time-of-day
  as ms since midnight, mirroring q's `time.hh` / `time.minute` accessors), plus
  `minbar` / `bar` / `xbar` for tick.minute-style OHLC bucketing.
- **`meta` shows attributes**: now a keyed table with `c` (column), `t` (type) and
  `a` (attribute) — so a sorted column shows `a: s`.
- **Grid rendering**: `show` / auto-print renders tables, keyed tables and dicts as clean
  q-style grids; `tsym[t;c]` formats time columns as `HH:MM:SS.mmm`.
- **Restructured help**: `\q` (scalars/agg/sets/strings), `\j` (tables/joins/qSQL),
  `\z` (temporal/attributes/display), plus a reorganised main `\` menu.
- **Examples**: `examples/basics.k` and `examples/tick.k` (realistic trades & quotes,
  as-of and window joins, VWAP, 1-minute OHLCV bars).
- **`round[d;x]`** helper.
- **CI**: GitHub Actions builds and runs the test suite (now 97 assertions) on every push.

## 1.0
- Amber: a low-latency array language with a q/kdb+ vocabulary. Aggregations, dictionaries, tables, keyed tables,
  joins (`lj ij uj pj ej aj aj0 wj asof`), qSQL (`qwhere qselect qby fby xgroup ungroup`),
  strings, and **C-level sorted attributes** that turn `?`/`in` into O(log n) binary search.
