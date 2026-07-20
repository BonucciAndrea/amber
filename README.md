# Amber — a q/kdb+ language on ngn/k

![ci](https://github.com/BonucciAndrea/amber/actions/workflows/ci.yml/badge.svg)
![license](https://img.shields.io/badge/license-AGPLv3-blue)
![tests](https://img.shields.io/badge/tests-97%20passing-brightgreen)

Amber is a small, self-contained vector language. It is the **ngn/k** interpreter plus a
**q/kdb+ vocabulary** (aggregations, dictionaries, **tables & keyed tables**, the whole join
family incl. **as-of** and **window** joins, a qSQL-style select/by, strings, and **intraday
temporal / tick.minute bars**) and **sorted column attributes implemented in C** that make
search ~1000× faster on large data.

This folder is everything you need. Nothing is installed system-wide. **It cannot interfere
with your ngn/k, kona, or kdb+ installs** — see “Isolation” below.

**New in 1.1:** temporal functions (`hms hh mm sec minute second stime`, `minbar` OHLC
bucketing), `meta` now shows column attributes, clean grid rendering with `show`, a
restructured `\` help (`\q \j \z`), and `examples/` with realistic trades & quotes.
See [CHANGELOG.md](CHANGELOG.md).

## Try it in 30 seconds

```sh
cd amber && ./a           # builds on first run, opens the REPL
```
```q
\l examples/tick.k         # realistic trades/quotes: as-of & window joins, VWAP, OHLC bars
\z                         # help page for temporal / tick bars / attributes
```

---

## TL;DR — how to run it

1. Put this whole `amber/` folder anywhere on your Ubuntu machine, e.g. your home directory:

   ```sh
   ~/amber/
   ```

2. From inside the folder, run the launcher `a`:

   ```sh
   cd ~/amber
   ./a
   ```

   The **first** time, it compiles the interpreter (a few seconds — you need a C compiler,
   see Requirements). Every time after that it launches instantly.

3. You get the Amber banner and a prompt. Try:

   ```q
   t:+`sym`px`sz!(`ibm`msft`ibm;100 200 101;10 20 30)
   show t          / pretty grid
   meta t          / column types AND attributes
   sum t`sz        / 60
   ```

4. Quit with `\\` (two backslashes) then Enter.

### Making it literally one key — type `a` from anywhere

Add an alias to your shell so you can just type `a`:

```sh
echo "alias a='$HOME/amber/a'" >> ~/.bashrc
source ~/.bashrc
```

Now typing **`a`** and Enter in any terminal starts Amber. (If you use zsh, use `~/.zshrc`.)
This adds only the name `a`; it does **not** touch `k` or `q`.

---

## Requirements

* A C compiler. On Ubuntu:

  ```sh
  sudo apt-get install build-essential
  ```

  (`gcc` or `clang` both work; the build auto-detects `cc`/`gcc`/`clang`.)

* **Optional:** `rlwrap` for arrow-key history/editing at the prompt:

  ```sh
  sudo apt-get install rlwrap
  ```

  If present, `a` uses it automatically. If not, Amber still runs (just no line editing).

Nothing else. No Python, no libraries, no root.

---

## Isolation — it will NOT touch ngn/k, kona, or kdb+

This is important, so here is exactly what Amber does and does not do:

* The interpreter is named **`amber`**, not `k` and not `q`. It is built **inside this folder
  only** (`./amber`). It is never copied to `/usr/bin`, `/usr/local/bin`, or anywhere on your
  `PATH`.
* The launcher is named **`a`**. The optional alias adds only the name `a`. Your `k` (ngn/k /
  kona) and `q` (kdb+) commands are completely untouched.
* Building writes only into this folder: object files go to `./o/`, the binary to `./amber`.
  Nothing outside the folder is created or modified.
* Amber does not read or write kdb+/ngn/k config, `QHOME`, `.k_history`, or any dotfiles.
* Deleting the folder removes Amber completely. There is nothing else to uninstall.

If you want to be doubly safe, keep this folder well away from your kdb+/ngn/k directories
(e.g. `~/amber`, not inside `~/q` or your ngn/k checkout).

---

## What you can do

Amber is q’s vocabulary with k’s notation. A few things to know up front (host = ngn/k):

* **Call two-argument library functions with brackets, not infix:** `lj[t;kt]`, `aj[c;x;y]`,
  `in[x;y]`, `xasc[`sym;t]`. (ngn/k does not allow infix user functions. Built-in symbols like
  `+ - * ! & | < > = , # _ $ ? @` are still infix.)
* **Print a table as a grid:** `show t`. At the prompt a bare `t` also auto-renders as a grid.
* **See column types and attributes:** `meta t` → a keyed table with `t` (type) and **`a`
  (attribute)** columns.

### Tables, keyed tables, meta with attributes

```q
t:+`sym`px`sz!(`ibm`msft`ibm;100 200 101;10 20 30)
show t
/  sym  px  sz
/  -------------
/  ibm  100 10
/  msft 200 20
/  ibm  101 30

kt:xkey[`sym;t]          / keyed table (keyed on sym)
show kt                  / rendered with a  key | value  divider

meta xasc[`px;t]         / sort by px, then inspect:
/  c   | t a
/  ---------
/  sym | s          <- 'a' column shows the sorted attribute
/  px  | i s
/  sz  | i
```

`xasc` and `asc` set the sorted attribute (`` `s``) automatically; `meta` shows it in the `a`
column, exactly like kdb+.

### The join family

```q
trade:+`sym`time`px !(`a`b`a; 3 4 9; 100 200 300)
quote:+`sym`time`bid!(`a`a`b`a; 1 5 2 8; 10 11 20 12)

show aj[`sym`time; trade; quote]     / as-of join (last quote at/before each trade)
show wj[(trade[`time]-2; trade`time); `sym`time; trade; quote; ,(`mx;max;`bid)]  / window join
show lj[trade; xkey[`sym; +`sym`name!(`a`b; `Apple`Boeing)]]                     / left join
```

Also available: `ij` (inner), `uj` (union), `pj` (plus), `ej` (equi), `aj0` (as-of, y’s time),
`asof` (single as-of lookup).

### Select / group-by (qSQL, functional form)

```q
t:+`sym`px`sz!(`a`b`a`b`a; 100 200 300 400 500; 10 20 30 40 50)
show qwhere[t; t[`sz]>20]                        / where sz>20
show qby[t; `sym; `vol`vwap!({sum x`sz}; {wavg[x`sz; x`px]})]   / by sym: sum sz, vwap
```

### Temporal & tick bars (`tick.minute` style)

Times are held as **milliseconds since midnight** (like q's `time`). q's dotted accessors
`t.hh`, `t.minute`, … become plain calls in Amber:

```q
t:hms[9;30;15]            / 09:30:15  -> 34215000
hh t                     / 9      (q t.hh)
minute t                 / 570    (q t.minute)  -- minutes since midnight
sec t                    / 15     (q t.ss; ss is reserved for string-search)
stime t                  / "09:30:15.000"
ptime "09:30:15.123"     / 34215123

/ 1-minute OHLCV bars (the classic tickerplant query):
tb:+@[+trade; ,`time; minbar[1]@]      / bucket the time column to 1-min bars
qby[tb; `sym`time; `open`high`low`close`vol!({first x`px};{max x`px};{min x`px};{last x`px};{sum x`sz})]
```

`minbar[5;times]` gives 5-minute bars; `bar[w;u;t]` is generic. Run
`./amber examples/tick.k` for a full trades/quotes/as-of/window/OHLC walkthrough.

### Attributes make search fast (the C-level feature)

```q
v: asc 2000000 ? 100000000        / 2M sorted ints, `s attribute set by asc
`at v                             / `s
v ? 12345 67890                   / O(log n) binary search  (linear would be ~1000x slower)
```

The full function list and design notes are in **AMBER.md**.

---

## Rebuilding / troubleshooting

* **Force a fresh build:** `rm -f amber && ./a` (or run `./build.sh`).
* **Pick a compiler:** `CC=clang ./build.sh`.
* **“No C compiler found”:** `sudo apt-get install build-essential`.
* **Prompt has no history/editing:** install `rlwrap` (`sudo apt-get install rlwrap`).
* **`./a: Permission denied`:** `chmod +x a build.sh` then retry.
* **Run the test suite any time:** `./amber test.k` → should print `84 tests run, 0 failures`.

---

## Files in this folder

| file            | what it is                                                     |
|-----------------|----------------------------------------------------------------|
| `a`             | the launcher — build-if-needed, then start the REPL            |
| `build.sh`      | portable compile script (writes only `./o` and `./amber`)     |
| `amber.k`       | the q layer (standard library), auto-loaded by the REPL       |
| `repl.k`        | the interactive loop (Amber banner, grid table rendering)     |
| `test.k`        | 84-assertion test suite                                       |
| `AMBER.md`      | **full language & library reference**                         |
| `*.c`, `*.h`    | the interpreter (ngn/k; `a.c a.h m.c f.c` modified for attrs) |
| `LICENSE`       | GNU AGPLv3                                                     |

Amber is a derivative of [ngn/k](https://codeberg.org/ngn/k) and is licensed **GNU AGPLv3**;
upstream copyright notices are preserved in the C sources as the licence requires.
