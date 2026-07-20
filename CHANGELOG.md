# Changelog

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
