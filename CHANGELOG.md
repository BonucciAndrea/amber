# Changelog

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
