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
![tests](https://img.shields.io/badge/tests-148%20passing-brightgreen)
![build](https://img.shields.io/badge/build-C11%20·%20portable-informational)

</div>

Amber is a small, fast, self-contained array language with the working vocabulary of
**q/kdb+** — dictionaries, **tables & keyed tables** with `([]…)` literal syntax, the full
**join family** (left · inner · union · plus · equi · **as-of** · **window**), qSQL-style
select/by, strings, intraday **tick / OHLC** temporals, and **column attributes implemented
in C** that turn search from `O(n)` into `O(log n)` — **~1000–2000× faster** on large data.

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
trade:([]sym:`a`b`a; time:3 4 9; px:100 200 300)
quote:([]sym:`a`a`b`a; time:1 5 2 8; bid:10 11 20 12)
aj[`sym`time; trade; quote]        / last quote at/ before each trade

/ 1-minute OHLCV bars (classic tickerplant query)
tb:+@[+trade; ,`time; minbar[1]@]
qby[tb; `sym`time; `o`h`l`c`v!({first x`px};{max x`px};{min x`px};{last x`px};{sum x`sz})]

/ sorted attribute => binary-search lookups
v:asc 2000000?1000000000                     / `s attribute set by asc
`at v                                        / `s
v ? 12345 67890                              / O(log n)  (see bench.k: ~1000x faster)
```

Run the guided tours:

```sh
./amber examples/tour.k     # a worked example of EVERY function
./amber examples/basics.k   # a 2-minute intro
./amber examples/tick.k     # realistic trades & quotes: as-of/window joins, VWAP, OHLC
./amber bench.k             # attribute speed benchmark
./amber test.k              # the 148-assertion test suite
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

Results are identical; only the time differs. `asc` / `xasc` set the attribute for you, and
`meta` shows it in the `a` column.

---

## Language notes (30-second version)

Amber uses a terse array notation. A few things that differ from kdb+/q:

* **Two-argument library functions take brackets:** `aj[c;x;y]`, `lj[t;kt]`, `in[x;y]`,
  `xasc[`sym;t]`. Built-in symbols (`+ - * % ! & | < > = ~ , ^ # _ $ ? @ .`) are still infix.
* **No `>=` / `<=`** — write `~a<b` and `~a>b`.
* **Symbols have no `_`** — use a quoted symbol `` `"a_b" ``.
* Tables: `([]col:vals;…)`; keyed tables: `([key:vals]col:vals)`. A bare table at the prompt
  auto-renders as a grid (or `show t`).

Full reference: **[AMBER.md](AMBER.md)**. Built-in help: `\` then `\q \j \z` for the Amber
vocabulary, `\0 \+ \' \`` for the core.

---

## What's inside

| file | |
|------|--|
| `a`, `build.sh` | launcher (build-if-stale) and portable compile |
| `*.c`, `*.h` | the interpreter (`p.c` carries the `([]…)` table-literal parser) |
| `amber.k` | the q/kdb+ vocabulary (auto-loaded) |
| `repl.k` | the REPL — banner, grid rendering, help |
| `examples/` | `tour.k` · `basics.k` · `tick.k` |
| `test.k`, `bench.k` | 148-assertion suite; attribute benchmark |
| `AMBER.md`, `MISSING.md`, `CHANGELOG.md` | reference · roadmap · history |

## Roadmap

Amber covers a large slice of q. [MISSING.md](MISSING.md) is an honest map of what's next —
top picks: the `` `g`` grouped attribute, real temporal *types*, a `select…from…where` parser,
on-disk tables, and IPC.

<a name="isolation"></a>
## Isolation

Amber is a single self-contained folder. The interpreter is named `amber` (never `k` or `q`),
built only inside the folder, never placed on your `PATH`. It reads/writes no config, no
`QHOME`, no dotfiles. Your kdb+, kona and other k/q installs are untouched; deleting the folder
uninstalls Amber completely.

## Licence

GNU AGPLv3 (see [LICENSE](LICENSE)). Amber's interpreter core derives from an AGPLv3 k
interpreter; that attribution is preserved in [NOTICE](NOTICE), as the licence requires.
