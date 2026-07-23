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
![license](https://img.shields.io/badge/license-AGPLv3-blue)
![tests](https://img.shields.io/badge/tests-226%20passing-brightgreen)
![build](https://img.shields.io/badge/build-C11%20·%20portable-informational)

</div>

Amber is a small, fast, self-contained array language, built on top of [ngn/k](https://codeberg.org/ngn/k), with the working vocabulary of
**q/kdb+** — dictionaries, **tables & keyed tables** with `([]…)` literal syntax, the full
**join family** (left · inner · union · plus · equi · **as-of** · **window**), the
**`select … by … from … where …` template**, strings, intraday **tick / OHLC** temporals,
**date/timestamp types**, **vectorised moving/window aggregates**, **on-disk data**,
**IPC + an in-process tickerplant**, **system namespaces** (`z.* Q.* j.* h.*`), and
**column attributes implemented in C** that turn search from `O(n)` into `O(log n)` —
**~1000–2000× faster** on large data.

```q
t:([]sym:`AAPL`MSFT`AAPL; px:187.3 411.2 187.4; sz:100 250 50)   / a table, rendered instantly
qby[t; `sym; (,`vwap)!,{wavg[x`sz;x`px]}]                         / vwap by symbol
```

---

## Install (Linux / WSL / macOS)

```sh
cd amber
./a                 # builds on first run (needs a C compiler), then opens the REPL
```

Type **`a`** from anywhere by adding an alias:

```sh
echo "alias a='$HOME/amber/a'" >> ~/.bashrc && source ~/.bashrc
```

`./a` recompiles automatically whenever the C sources change, so you never run a stale build.
Requirements: a C compiler (`sudo apt-get install build-essential`); optional `rlwrap` for
line-editing. Nothing is installed system-wide — see [Isolation](#isolation).

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

/ the join every tick shop needs — as-of
trade:([]sym:`a`b`a; time:3 4 9; px:100 200 300;sz:100 150 175)
quote:([]sym:`a`a`b`a; time:1 5 2 8; bid:10 11 20 12)
aj[`sym`time; trade; quote]        / last quote at/ before each trade

/ 1-minute OHLCV bars (classic tickerplant query)
tb:+@[+trade; ,`time; minbar[1]@]
qby[tb; `sym`time; `o`h`l`c`v!({first x`px};{max x`px};{min x`px};{last x`px};{sum x`sz})]

/ sorted attribute => binary-search lookups (20M)
v:asc 20000000?100000000;u:0+v;p:v@5000?#v                 / v has sorted attribute while u does not
t:`t[];a:u?p;lin:`t[]-t;t:`t[];b:v?p;bin:`t[]-t;           / lin and bin are both the runtimes in microseconds
`ratio`linus`binus`equal`atv`atu!(round[5;lin%bin];lin;bin;a~b;`at v;`at u)
```

Run the guided tours:

```sh
./amber examples/tour.k     # a worked example of EVERY function
./amber examples/basics.k   # a 2-minute intro
./amber examples/tick.k     # realistic trades & quotes: as-of/window joins, VWAP, OHLC
./amber examples/extended.k # tour of the v1.5 modules (qSQL, window, dates, HDB, tick)
./amber bench.k             # attribute speed benchmark
./amber bench-std.k         # vectorised moving/window-aggregate benchmark
./amber test.k              # 153 core + run test-fin.k / test-ext.k for 226 total
```

---

## Why attributes matter

`bench.k` measures `?` (find) on identical data, sorted-attributed vs not:

| rows | linear scan | binary (`` `s``) | speedup |
|-----:|------------:|-----------------:|--------:|
| 100 k | 87 ms | 0.6 ms | **141×** |
| 500 k | 417 ms | 0.9 ms | **470×** |
| 2 M | 1.73 s | 1.4 ms | **1244×** |
| 5 M | 4.23 s | 1.9 ms | **2261×** |
| 20 M | 22.74 s | 2.9 ms | **7818×** |

Results are identical; only the time differs. `asc` / `xasc` set the attribute for you, and
`meta` shows it in the `a` column.

---

## Language notes (30-second version)

Amber uses a terse array notation. A few things that differ from kdb+/q:

* **Two-argument library functions take brackets:** `aj[c;x;y]`, `lj[t;kt]`, `in[x;y]`,
  ``xasc[`sym;t]``. Built-in symbols (`+ - * % ! & | < > = ~ , ^ # _ $ ? @ .`) are still infix.
* **No `>=` / `<=`** — write `~a<b` and `~a>b`.
* **Symbols have no `_`** — use a quoted symbol `` `"a_b" ``.
* Tables: `([]col:vals;…)`; keyed tables: `([key:vals]col:vals)`. A bare table at the prompt
  auto-renders as a grid (or `show t`).

Full reference: **[AMBER.md](AMBER.md)**. Built-in help: `\` then `\q \j \z` for the Amber
vocabulary, ``\0 \+ \` \'`` for the core.

---

## Finance / HFT module (`fin.k`)

Auto-loaded after `amber.k`. Generate a market session and analyse it the way an HFT desk does:

```q
gentq 100000                       / sets global `trades` and `quotes` (numeric times, `s on time, `p on sym)
m:aj[`sym`time; trades; quotes]    / TAQ: prevailing quote for every trade
tsign m                            / Lee-Ready trade sign (+1 buy / -1 sell)
effspread m                        / effective spread = 2|px-mid|
qby[trades;`sym; `vwap!enlist {wavg[x`sz;x`px]}]   / VWAP by symbol
bars[1; trades]                    / 1-minute OHLCV bars
g:bysym trades                     / O(1) grouped index
symrows[trades; g; `AAPL]          / all AAPL rows in O(1)
```

Included: `mid spread spreadbps micro imbal` (book), `vwap twap tsign signedvol effspread
notional` (trades), `ret logret rvol movavg movsum movmax movmin ema rollstd` (returns/vol),
`bars symstats` (aggregation), `bysym symrows gidx` (O(1) index), `pt` (time-formatted print).
Walkthrough: `./amber examples/hft.k`.

**Attributes.** Amber has all four kdb-style attributes in C: `` `sa`` sorted, `` `ua`` unique,
`` `pa`` parted, `` `ga`` grouped (`` `at`` reads them, `meta` shows them). Sorted/parted give
O(log n) kernel find; grouped + the group index give O(1) per-symbol slicing
(`bench-fin.k` ~ 20,000x vs a scan).

## Extended modules (v1.5)

Six modules auto-load after `fin.k`. Built-in help: `\w \s \u \y`.

```q
/ qSQL template — bare column names just work (qsql.k, help \s)
sel "select vwap:wavg[sz;px],n:#px by sym from trades where px>100"
upd "update mid:0.5*bid+ask from quotes"
del "delete from trades where sz<100"

/ vectorised moving/window aggregates — O(n) prefix-based (std.k, help \w)
mavg[20; px]   msum[20; sz]   mdev[20; px]   mmax[20; px]

/ date & timestamp types, epoch 2000.01.01 (temporal.k, help \u)
dstr ymd2d[2024;1;15]                 / "2024.01.15"
dow ymd2d[2024;1;15]                  / `Mon
pstr tstamp[ymd2d[2024;1;15]; hms[9;30;0]]   / "2024.01.15D09:30:00.000"

/ system namespaces, JSON, on-disk, IPC/tick (sys.k / hdb.k / ipc.k, help \y)
z.d[]                                 / today (date)
j.j ([]a:1 2; b:`x`y)                 / -> JSON string
splay["db/trades"; trades]            / save a splayed table;  dload "db/trades"
u.def[`trade; ([]sym:0#`; px:0#0.0)]  / define a stream
u.sub[`trade; {[nm;d] show d}]        / subscribe;  u.pub[`trade; batch]
```

`parse eval ser deser cast peach` and the `\ts expr` timer round it out. The 256-global
cap was lifted to 4096 (a 2-byte bytecode index) so the whole vocabulary loads at once.

## Amber Notepad — a browser playground

`Amber-Notepad.html` is a **single self-contained page**: a real Amber interpreter
(written in JavaScript) behind a notebook UI with an amber-phosphor theme. Open it in any
browser — no install, no internet — and run Amber in stacked cells with live evaluation,
syntax highlighting, and rendered tables/keyed-tables/dicts (attributes shown). It covers
the everyday vocabulary — arithmetic, verbs, adverbs, lambdas, `([]…)` tables, `meta`,
qSQL `qby`, as-of joins, `gentq` and the finance functions — as a faithful subset of the
C interpreter, ideal for learning and quick experiments.

## What's inside

| file | |
|------|--|
| `a`, `build.sh` | launcher (build-if-stale) and portable compile |
| `*.c`, `*.h` | the interpreter (`p.c` carries the `([]…)` table-literal parser) |
| `amber.k` | the q/kdb+ vocabulary (auto-loaded) |
| `fin.k` | finance / HFT module (auto-loaded) — see `\m` help |
| `std.k` `qsql.k` `temporal.k` `sys.k` `hdb.k` `ipc.k` | v1.5 extended modules — `\w \s \u \y` |
| `repl.k` | the REPL — banner, grid rendering, help |
| `Amber-Notepad.html` | self-contained in-browser interpreter + notebook UI |
| `examples/` | `tour.k` · `basics.k` · `tick.k` · `hft.k` · `attributes.k` · `practice.k` |
| `test.k` `test-fin.k` `test-ext.k` | 226-assertion suite (153 + 35 + 38) |
| `bench.k` `bench-fin.k` `bench-std.k` | attribute · O(1) index · window-aggregate benchmarks |
| `AMBER.md`, `MISSING.md`, `CHANGELOG.md` | reference · roadmap · history |

## Roadmap

Amber covers a large slice of q. [MISSING.md](MISSING.md) is an honest map of what's next —
top picks: real temporal *types* with literal syntax, sorted/limited selects
(`select[>col]`), new C atom types (`byte` `real` `short`), and binary `-8!`/`-9!`.

<a name="isolation"></a>
## Isolation

Amber is a single self-contained folder. The interpreter is named `amber` (never `k` or `q`),
built only inside the folder, never placed on your `PATH`. It reads/writes no config, no
`QHOME`, no dotfiles. Your kdb+, kona and other k/q installs are untouched; deleting the folder
uninstalls Amber completely.

## Licence

GNU AGPLv3 (see [LICENSE](LICENSE)). Amber's interpreter core derives from an AGPLv3 k
interpreter; that attribution is preserved in [NOTICE](NOTICE), as the licence requires.
