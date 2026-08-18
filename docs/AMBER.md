# Amber

**Amber** is a low-latency **array language**: a fast, compact array interpreter that provides
the major functionality of **q/kdb+** — aggregations, dictionaries, tables, keyed tables,
the join family (left / inner / union / plus / equi / **as-of** / **window**), a qSQL‑style
select/by, string utilities, and **column attributes** that accelerate search.

Amber is distributed under the **GNU AGPLv3 (v3 only)**. Its interpreter core derives from an
AGPLv3 k interpreter; that attribution is recorded in `NOTICE`, as the licence requires.

Amber ships as:

| file        | purpose                                                        |
|-------------|---------------------------------------------------------------|
| `amber`     | the interpreter (compiled from the C sources)                 |
| `amber.k`   | the Amber standard library (the q layer), auto‑loaded         |
| `repl.k`    | the interactive read‑eval‑print loop (loads `amber.k`)        |
| `test.k`    | the core test suite (153 assertions; `test-fin.k` +35, `test-ext.k` +79 = 267) |
| `*.c *.h`   | the interpreter sources (modified `a.c a.h m.c f.c` for attributes) |

---

## 1. Building

Amber builds anywhere a C11 compiler and a POSIX‑ish libc are available (Linux, macOS, WSL).

```sh
# from the source directory
make amber CC=gcc            # or CC=clang-17, CC=gcc-10, ...
```

`make amber` compiles every `*.c`, links `-lm -ldl`, and copies the result to `./amber`.
The build is an optimised C compile (`-O2`, portable) of the Amber interpreter.

Other targets:

```sh
make k        # build the interpreter as ./k (identical binary, short name)
make c        # clean (rm -rf o k amber libk.so libk.a)
```

Run the test suite:

```sh
./amber test.k
# ...
# ================ AMBER TEST SUITE ================
# 163 tests run, 0 failures
# ALL TESTS PASSED
# =================================================
# (also: ./amber test-fin.k  -> 35,  ./amber test-ext.k -> 79; 277 total)
```

Start an interactive session:

```sh
./a                            # the launcher: builds if stale, then opens the REPL
./amber repl.k                 # the same thing, without the build-if-stale check
```

Since **1.9.5** the REPL has its own line editor (`src/ln.c`) — history, arrow keys,
`Ctrl-A/E/W/U/K`, and Tab completion over your globals and table columns — so **do not wrap it in
`rlwrap` or `rlfe`**. Amber puts the terminal in raw mode and reads single keypresses, which is
exactly the case those wrappers cannot handle: rlwrap prints

```text
rlwrap: warning: rlwrap appears to do nothing for amber, which asks for
single keypresses all the time ...
```

into the middle of your session and then fights the REPL for the cursor. `AMBER_NO_EDIT=1`
disables the native editor if you need canonical-mode reads; `./a` then wraps in `rlwrap -n -a`,
which is silent. See the [changelog](../CHANGELOG.md) for the full account.

The banner appears and `amber.k` is loaded automatically, so `sum`, `avg`, `aj`, `lj`, … are
immediately in scope. Type `\` for the built‑in help pages, `\\` to quit.

To use the library from your own script, put `\l amber.k` on the first line:

```k
\l amber.k
t:([]sym:`a`b`a; px:100 200 300)
qby[t; `sym; (,`ap)!,{avg x`px}]   / see §7
```

---

## 2. How Amber is structured

Amber is **two layers**:

1. **The kernel (C).** Amber’s evaluator, memory manager, parser and ~200 primitive verbs.
   This is where values live and where the *attribute* machinery was added (see §9).

2. **The library (`amber.k`).** ~110 definitions, one per line, that recreate q’s vocabulary
   using kernel primitives. Loading is silent; every name lands in the root namespace.

Everything in the underlying array language is available and mixes freely with the q layer:
Amber is q semantics in a terse array notation, at array-language speed.

### Dialect notes (important)

Amber follows a terse **array grammar**, which differs from kdb+/q in a few ways you must know:

* **Dyadic library functions are called with brackets, not infix.** Amber does **not** allow a
  user‑defined function to be applied infix (`x f y` is a parse of two nouns). So write
  `lj[t;kt]`, `in[x;y]`, `except[a;b]`, `xasc[`sym;t]` — not `t lj kt`. Built‑in verbs
  (`+ - * % ! & | < > = ~ , ^ # _ $ ? @ .`) *are* infix as usual.
* **No `>=` / `<=` operators.** Use `~a<b` for `a>=b` and `~a>b` for `a<=b`.
* **Symbols cannot contain `_`.** `` `a_b `` is a parse error; use a quoted symbol `` `"a_b" ``.
* **Nested lambdas are not closures.** An inner `{…}` sees only its own parameters and globals,
  never the enclosing function’s locals. The library passes captured values in by projection
  (`f[captured]'list`); do the same in your own code.
* **`.x` after a *name* is dyadic apply, after a *verb* is monadic “value”.** The library uses the
  `value` function (`value x` ≡ `. x`) to avoid the ambiguity; prefer it in your code.

These are properties of the host, not bugs, and the library is written to respect them.

---

## 3. Type & null quick reference

| k type | list / atom | example            | null   |
|--------|-------------|--------------------|--------|
| int    | `` `I``/`` `i`` | `0 1 2`, `!5`   | `0N`   |
| float  | `` `F``/`` `f`` | `1.5 2.5`       | `0n`   |
| char   | `` `C``/`` `c`` | `"abc"`, `"x"`  | `" "`  |
| symbol | `` `S``/`` `s`` | `` `a`b`c``     | `` ` `` |
| bool   | `` `B``      | `101b`             |        |
| dict   | `` `m``      | `` `a`b!1 2``      |        |
| table  | `` `M``      | `` +`a`b!(1 2;3 4)`` |      |

`@x` returns the type symbol. `!n` gives `0..n-1` (a compact range whose `@` reads `` `I``).

---

## 4. Scalar, aggregation and uniform functions

All are in `amber.k`. Monadic ones apply prefix (`sum x`); dyadic ones use brackets
(`wavg[x;y]`).

**Scalar / conversion**
`neg not null reciprocal sqrt floor ceiling signum abs exp log sin cos til enlist string type
key value first last reverse distinct group where flip count mod div xbar xlog`

**Aggregation** (list → atom)
`sum prd min max avg med var dev svar sdev cov scov cor wsum wavg all any`

**Uniform** (list → list)
`sums prds mins maxs deltas ratios differ prev next`

Examples:

```k
sum 1 2 3 4          / 10
avg 1 2 3 4          / 2.5
med 1 2 3 4 5        / 3.0
dev 2 4 4 4 5 5 7 9  / 2.0
deltas 1 3 6 10      / 1 2 3 4
wavg[10 20 30;1 2 3] / 2.333...       (weights;values)
xbar[10;0 5 12 23]   / 0 0 10 20      (bucket to multiples of 10)
signum -3 0 5        / -1 0 1
```

## 5. Ordering, ranking and set operations

`asc desc iasc idesc rank xrank rotate in except inter union raze sublist cross`

```k
asc 3 1 2            / 1 2 3          (and carries the `s sorted attribute — see §9)
iasc 30 10 20        / 1 2 0          (grade)
rank 30 10 20        / 2 0 1
in[3 5;1 2 3 4]      / 10b
except[1 2 3 4;2 4]  / 1 3
inter[1 2 3 4;2 4 6] / 2 4
sublist[3;1 2 3 4 5] / 1 2 3          (capped, unlike k's cyclic #)
cross[1 2;10 20]     / ((1;10);(1;20);(2;10);(2;20))
```

`asc` returns a sorted vector **with the sorted attribute set**, so subsequent `?`/`in` on it
run in O(log n) — see §9.

---

## 6. Dictionaries, tables and keyed tables

A **dictionary** is `keys!values`; a **table** is a flipped column dictionary `+d` (type `` `M``);
a **keyed table** is a dictionary whose key *and* value are both tables (exactly kdb+’s model).

```k
t:([]sym:`a`b`c; px:100 200 300; sz:10 20 30)   / a 3-row table (literal syntax)
t`px                 / 100 200 300            column access
t 1                  / `sym`px`sz!(`b;200;20) row as a dict
#t                   / 3                       row count
cols t               / `sym`px`sz
```

Table / keyed‑table toolkit (all in `amber.k`):

| function            | meaning                                                     |
|---------------------|-------------------------------------------------------------|
| `istable x`         | is `x` a table?                                             |
| `isdict x`          | is `x` a dictionary?                                        |
| `iskeyed x`         | is `x` a keyed table?                                       |
| `cols x`            | column names (key + value cols for a keyed table)          |
| `keys x`            | key column names of a keyed table                          |
| `xkey[k;t]`         | make a keyed table keyed on columns `k`                    |
| `unkey x`           | drop the key (keyed table → plain table)                   |
| `xcols[c;t]`        | reorder so columns `c` come first                          |
| `xcolall[nm;t]`     | rename all columns to `nm`                                 |
| `xasc[c;t]`         | sort table ascending by `c` (sets `` `s`` on the sort col) |
| `xdesc[c;t]`        | sort table descending by `c`                               |
| `meta x`            | table of `c` (column) and `t` (element type)               |
| `rows x`            | rows of a table as a list of tuples                        |
| `atr[t;i]`          | select rows `i` of table `t`                               |

```k
kt:xkey[`sym;t]      / keyed on sym
iskeyed kt           / 1
keys kt              / ,`sym
unkey kt             / back to the plain table
xasc[`px;t]          / rows ordered by px
meta t               / +`c`t!(`sym`px`sz;`s`i`i)
```

---

## 7. Selecting and grouping (qSQL)

Amber supports the **`select … by … from … where …` template directly** — type it at the
prompt with no wrapper:

```k
select avg px by sym from trades where px>100     / grouped aggregate
select sym,px from trades where px>180            / chosen columns, filtered
exec avg px from trades where sym=`AAPL           / one column/expression
update mid:0.5*bid+ask from quotes                / add/replace columns
delete from trades where sz<300                   / drop rows
r:select n:#px, avg px by sym from trades         / assign; also  5#select …  count select …
```

Bare column names in the expressions become `x`col`, so `wavg[sz;px]` just works. The same
templates are also callable as strings (`sel"select …"`, `exq`, `upd`, `del`) and via the
**functional form** below (handy when you build the query programmatically).

**Interactive prompt vs. `.k` scripts.** The bare `select … from …` rewriting above is applied
line-by-line by `repl.k` (via `qsql.k`'s `qrw`) as you type at the `amber>` prompt. A script run
with `./amber file.k` does not go through that per-line rewrite, so bare qSQL sugar will not parse
there — use the `sel"…"`/`exq"…"`/`upd"…"`/`del"…"` string forms, or call `qwhere`/`qselect`/`qby`
directly, exactly as `test.k` and every script under `examples/` already do.

| function             | q analogue                                     |
|----------------------|------------------------------------------------|
| `qwhere[t;mask]`     | `select from t where mask`                     |
| `qselect[t;a]`       | `select …` (`a` = `name!func` computed columns)|
| `qby[t;b;a]`         | `select … by b` → **keyed table**              |
| `xgroup[k;t]`        | `` `k xgroup t`` (nested value columns)         |
| `ungroup x`          | flatten nested columns                          |
| `fby[(f;d);g]`       | `(f;d) fby g`                                   |
| `insert[t;r]`        | append rows                                     |

`a` (the aggregate spec) is a dictionary from result‑name to a function that receives the group
sub‑table and returns a value:

```k
t:+`sym`px`sz!(`a`b`a`b`a;100 200 300 400 500;10 20 30 40 50)

/ select sz>20:
qwhere[t;t[`sz]>20]

/ select sum sz, avg px by sym:
qby[t;`sym;`tot`avgpx!({sum x`sz};{avg x`px})]
/  =>  (+(,`sym)!,`a`b) ! +`tot`avgpx!(90 60; 300.0 300.0)

/ fby: group-broadcast (sum sz within each sym, aligned to rows):
fby[(sum;t`sz);t`sym]    / 90 60 90 60 90
```

`qby` returns a **keyed table** keyed on the by‑columns, just like q.

---

## 8. Joins

Every join is a function; call it with brackets. Left operand is the “driver” table.

| function             | kind                | notes                                   |
|----------------------|---------------------|-----------------------------------------|
| `lj[t;kt]`           | **left** join       | `kt` keyed; unmatched → nulls           |
| `ij[t;kt]`           | **inner** join      | keep matched rows only                  |
| `uj[x;y]`            | **union** join      | union of columns, rows concatenated     |
| `pj[t;kt]`           | **plus** join       | add matched numeric value columns       |
| `ej[c;x;y]`          | **equi** join on `c`| inner join on the given columns         |
| `aj[c;x;y]`          | **as‑of** join      | last `y` row per key with time ≤ `x`.time |
| `aj0[c;x;y]`         | as‑of, `y`’s time   | like `aj` but result time is `y`’s      |
| `asof[t;d]`          | as‑of lookup        | single as‑of row for the dict `d`       |
| `wj[w;c;t;q;aggs]`   | **window** join     | aggregate `q` over a window per `t` row |

`c` is `` `key…`time`` — the last name is the ordering (time) column, the rest are exact‑match keys.

### As‑of join

```k
trade:+`sym`time`px !(`a`b`a; 3 4 9; 100 200 300)
quote:+`sym`time`bid!(`a`a`b`a; 1 5 2 8; 10 11 20 12)

aj[`sym`time;trade;quote]
/  sym time px  bid
/  a   3    100 10     <- last a-quote at/ before t=3 is t=1 (bid 10)
/  b   4    200 20     <- last b-quote at/ before t=4 is t=2 (bid 20)
/  a   9    300 12     <- last a-quote at/ before t=9 is t=8 (bid 12)
```

`aj` sorts `quote` on `` `sym`time`` internally and matches with **binary search** inside each
symbol group, so it is O(n log m). As of **1.9** the match step runs in a **native C kernel**
(`src/a.c`: `ajc` + a branch-free / `cmov` `lower_bound`, `ajlb`) over the group's sorted
nanosecond timestamp slice, driven from `amber.k`'s `ajm`; the pure-K reference is kept as `ajmK`.
Empty groups and trades before a symbol's first quote resolve to null. The kernel's transient
match vector comes from the HFT arena allocator (`src/arena.{h,c}`), so a per-tick `aj` does not
call the system `malloc`/`free`.

### Window join

`w` is a pair `(begins;ends)` of time vectors aligned to the `t` rows; `aggs` is a list of
triples `(name; aggregate‑function; column)`:

```k
w:(trade[`time]-2; trade`time)            / a 2-unit look-back window per trade
wj[w; `sym`time; trade; quote; ,(`mx;max;`bid)]
/  sym time px  mx
/  a   3    100 10      max bid for a in [1,3]
/  b   4    200 20      max bid for b in [2,4]
/  a   9    300 12      max bid for a in [7,9]
```

Pass several aggregates at once: `((`mx;max;`bid);(`mn;min;`bid);(`n;count;`bid))`.

---

## 9. Attributes (the C‑level change)

### What was added

kdb+ attaches *attributes* to vectors to speed up operations. Amber implements **all four
kdb+ attributes** — sorted (`` `s``), unique (`` `u``), parted (`` `p``) and grouped (`` `g``) —
at the **kernel level**, because that is where search lives. Five symbol‑verbs are exposed by
the interpreter:

```k
`sa x     / set the sorted attribute on vector x  (returns x, attribute = `s)
`ua x     / set unique                            (attribute = `u)
`pa x     / set parted                            (attribute = `p)
`ga x     / set grouped                           (attribute = `g)
`at x     / read the attribute of x               (`s`u`p`g, or ` if none)
```

`asc` and `xasc` apply `` `sa`` for you, so idiomatic sorted data is attributed automatically;
`fin.k`'s `gentq` sets `` `s`` on `time` and `` `p`` on `sym`. **Sorted *and* parted** vectors
take the O(log n) binary‑search find path; grouped pairs with `fin.k`'s group index for O(1)
per‑symbol slicing.

### Why it makes search faster

Amber’s find (`?`) and membership (`in`) on integer vectors are an **O(n) linear scan**
(`f.c: fGL/fHL/fIL/fLL`). Binary search (`bin`, the `x'y` form) already exists but you have to
ask for it. The sorted attribute lets `?`/`in` *decide for themselves*: when the left vector is
attributed sorted, find dispatches to a new **O(log n) binary search** instead of the scan.

Measured (2,000,000‑row sorted int vector, 5,000 look‑ups, identical results):

```
binary (`s#)  ~1.7 ms
linear         ~1900 ms
speedup        ~1100x
```

The test suite asserts both **correctness** (`bin? == linear?`) and that the attributed path is
at least 2× faster on a 200k sample.

### How it is implemented (files changed)

* **`a.h`** — a new header accessor `_at(x)` at the previously‑unused header byte `-13`
  (`#define _at(x) (*(UC*)((x)-13))`). Attribute codes: `0` = none, `1` = sorted, `2` = unique,
  `3` = parted, `4` = grouped.
* **`m.c`** — the allocator `an()` now zeroes `_at(x)` on every allocation, so the flag is
  well‑defined for every object (the free‑list path did not previously clear this byte).
* **`a.c`** — five functions, `qsa`/`qua`/`qpa`/`qga` (set) and `qat` (get), wired into the
  `sym1` symbol‑verb dispatch table as `` `sa`` `` `ua`` `` `pa`` `` `ga`` and `` `at``. Each
  setter marks a simple vector with its code; `qat` reports it as `` `s`u`p`g``.
* **`f.c`** — four binary‑search probes `bGL/bHL/bIL/bLL`, and a one‑line change in `fnd` so the
  integer find path selects them when the vector is **sorted or parted** (and the type is
  `tH/tI/tL`, not float/symbol):

  ```c
  B srt = !_tP(x) && xt!=tF && xt!=tS && (_at(x)==1 || _at(x)==3);
  TY(fGL)*f = (srt ? G(&bGL,bHL,bIL,bLL) : G(&fGL,fHL,fIL,fLL))[xw-3];
  ```

The attribute is intentionally **dropped by operations that build new vectors** (the byte is
zero on allocation), matching kdb+ semantics: it is a promise about *this* vector’s current
contents, re‑established by `asc`/`` `sa`` when you know the data is ordered.

Only integer widths are accelerated; floats and symbols keep the scan (raw‑bit order ≠ value
order for floats, and symbol order is interning order). This keeps results exactly correct.

---

## 9a. Parallelism — `peach`

`peach[f;y]` is a drop-in parallel replacement for `` f'y `` (each): it forks
`AMBER_THREADS` worker **processes** (default: the online CPU count, via `sysconf`;
previously a hardcoded `4`, which oversubscribed small boxes and under-used large ones — see
[CHANGELOG](CHANGELOG.md)), each applies `f` to a slice of `y`,
serialises its result with the binary serializer (`-8!`, §10b — it was `` `k `` text before
1.9.3) and streams it back, and the parent decodes it with `-9!` and concatenates. The result is
**identical** to serial `` f'y `` for every value (vectors, symbols, tables, nested, ragged).

Since 1.9.3 a worker that fails — an error raised inside `f`, a signal, a chunk that cannot be
encoded — is detected via the child's exit status and surfaced as a clean, trappable
`'worker error in peach`, instead of the parent silently returning a short result. Every child is
still reaped, so no zombies and no orphaned pipes are left behind:

```k
.[{peach[{$[x=5;'"boom";x]};!10]};,0;{[e]"caught: ",e}]   / 'worker error in peach
```

```k
peach[{avg x?1.0}; 8#1000000]        / 8 heavy tasks, one per worker, across cores
AMBER_THREADS=8 ./amber examples/peach.k   / a Monte-Carlo demo timing serial vs peach
```

It uses `fork` (copy-on-write heap), so there are no shared-memory data races and no
atomic-refcount tax on ordinary single-threaded code — the same reason kdb+ parallelises
with processes rather than threading its interpreter. And with no GIL, every worker runs on
a real core at once. Use it for **coarse-grained, compute-heavy** per-item work (Monte-Carlo,
per-symbol fits, bootstraps, parallel loads); for fine-grained work the fork + serialise
round-trip makes plain `'` faster. `AMBER_THREADS=1` forces serial. See BENCHMARKS.md §4.

## 9b. Display — Q-style grid preview

`show t` and a bare table / keyed table / dict at the prompt print only the first `CROWS`
rows (default **20**), then a `..` line to show there is more — exactly like q's console. The
cap is applied *before* formatting, so previewing a million-row table is instant. Set
`CROWS:10` (or any n) at the prompt to change the preview height; small results print in full.

A bare table / keyed table also prints a dimmed **size footer** `[N rows x M cols]` under the
grid (thousands-separated, e.g. `[1,000 rows x 4 cols]`); the counts are the true totals, taken
before the `CROWS` truncation.

**ANSI syntax highlighting.** Grid output (tables, keyed tables, dictionaries) is coloured for
dark terminals with a vivid 256-colour, **14-hue per-column palette** (`PAL`) — each column gets a
distinct colour, headers are bold white, and nulls / the size footer are dimmed. Colour is applied
*after* width padding, and `vlen`/`vstrip` strip the escape bytes before every column-width and
header-underline calculation, so alignment is exact. Set `COLOR:0` to disable (e.g. when
redirecting output to a file); the per-column cycle lives in `PAL` and the type/attribute tints in
the `CT` dictionary.

<<<<<<< HEAD:docs/AMBER.md
**Table borders.** `\grid clean|rounded|sharp|heavy` picks the frame style: `clean` (default,
minimal dashed rule), `rounded` (`╭─┬─╮`), `sharp` (`┌─┬─┐`) or `heavy` (`┏━┳━┓`). Column widths
are measured with `vlen` (ANSI stripped) so the box lines up exactly around coloured cells; borders
are dimmed and the `[N rows x M cols]` footer sits below the bottom edge. `\clear` clears the
terminal.

Numeric and temporal columns are **right-aligned** (symbol / char / string columns left-aligned),
the header underline is dimmed grey, and null sentinels (`0N` / `0n` / `0w` / a blank cell) render
faint grey. Float precision in a grid is controlled by the **`PREC`** global — the number of
decimals to show (default `7`); set `PREC:0N` for full precision. `PREC` affects grid display only,
never the stored values.

=======
>>>>>>> main:AMBER.md
## 9b′. Error ergonomics — Rust-style diagnostics

Every error is rendered **once**, as a single Rust-compiler-style report: a category-specific
`error[CODE]` line, a `-->` locator, a gutter-aligned source line, an underline spanning the whole
offending **token**, an inline label naming what is wrong, and an actionable `= help:` note.

```text
error[E0101]: Undefined variable `prices`
 --> <amber>:1:3
  |
1 | y:prices+1
  |   ^^^^^^ not found in this scope
  |
  = help: Verify that the variable is defined in the current scope or check for typos.
```

```text
error[E0103]: Vector length mismatch
 --> <amber>:1:6
  |
1 | 1 2 3+1 2
  |      ^ operands have different counts
  |
  = help: Conforming operations require vectors of matching lengths or atom-vector pairs.
```

```text
error[E0105]: Syntax or parse error
 --> <amber>:1:4
  |
1 | (1+
  |    ^ unexpected token here
  |
  = help: Check for unbalanced brackets (), [], {}, unterminated strings, or invalid syntax.
```

Before 1.9.4 the same failure printed **twice** — the report above, immediately followed by the
legacy ngn/k block (`'value` / source line / bare `^`). The compact form is still built (it is
what `.[f;args;handler]` receives and what `` `err`` returns), it is simply no longer echoed to
stderr once a rich report has been shown. Disable diagnostics and the compact form is printed
instead, so an error is never silently swallowed.

### Error code matrix

| Code | Category | Title | Raised when |
|---|---|---|---|
| `E0101` | `value` | Undefined variable or value | A name is referenced that is not bound in any enclosing scope |
| `E0102` | `type` | Type mismatch or invalid operand type | A primitive was handed an operand type it has no case for |
| `E0103` | `length` | Vector length mismatch | Conforming operands have different counts |
| `E0104` | `domain` | Out of domain operation | The value is outside the operation's mathematical domain |
| `E0105` | `parse` | Syntax or parse error | Unbalanced brackets, unterminated string, malformed expression |
| `E0106` | `index` | Index out of bounds | An index is negative or past the end |
| `E0107` | `rank` | Function rank mismatch | Wrong number of arguments for the function's rank |
| `E0108` | `limit` | Resource or allocation limit exceeded | Vector size, recursion depth or memory limit hit |
| `E0109` | `io` | Input/output failure | File or socket open/read/write failed |
| `E0110` | `stack` | Call stack depth exceeded | Recursion deeper than the interpreter stack allows |
| `E0111` | `compile` | Expression could not be compiled | Parses, but cannot be compiled |
| `E0112` | `nyi` | Operation not implemented for these types | Primitive has no implementation for these operands yet |

### Reading a report

* **Underlines span the token, not a byte.** The core widens a bare error offset to the full
  identifier, operator or string literal before rendering, so a six-character name is underlined
  `^^^^^^`. Secondary/context spans use `~~~~~` in a different colour so they read as context
  rather than as the fault site.
* **The offending token is named.** For an undefined name it is promoted into the title itself
  (``Undefined variable `prices` ``); other categories carry it in the inline label.
* **The gutter is a single unbroken vertical rule.** Line numbers are right-aligned and dimmed,
  and every row places its `|` in the same column regardless of line-number width.

### Turning it off

```sh
./amber                 # interactive REPL — diagnostics shown automatically
./amber myscript.k      # a script — same
AMBER_DIAG=0 ./amber    # opt out: compact one-line errors only
```

`` `diag 0`` does the same at runtime and returns the previous setting, which is what
`tests/harness.k` and `std.k`'s `protect` use so a suite that deliberately provokes errors it
then catches does not drown stderr in red. `` `diag`` with a non-numeric argument reads the
setting without changing it.

### Implementation

`src/diagnostic.{h,c}` holds the renderer: a `Span` (`src`, `start`, `end`, 1-based `line`/`col`),
`report_diagnostic_ex()` (code, title, label, secondary spans, help, note, colour) and the
back-compatible `report_diagnostic()`. The palette lives in `src/ansi.h` so other consumers can
match it. `src/e.c` owns the category catalogue (`edtab`), the token widener (`etok`) and `eD()`,
which every error path funnels through — the evaluator/compiler via `eS()` (`src/b.c`) and the
parser directly (`src/p.c`). Exercise it from Amber with the `` `dgn`` self-test builtin
(`` `dgn 0`` → `1`), which checks the layout, the underline characters, the inline label, gutter
alignment, ANSI suppression when colour is off, and the complete category → code matrix above.

---

## 9c. Terminal charts — `plot` · `candle`

`plot v` (or `plot (v;W;H)`) renders a numeric vector as a **Braille** line chart — a 2×4
dot bitmask per character cell gives 2× horizontal and 4× vertical resolution — with min/max
scaling, Y-axis tick labels, and Bresenham-drawn curves. `candle t` renders an OHLC table
(columns `open/high/low/close` or `o/h/l/c`, keyed or not) as **Unicode candlesticks** with
ANSI colour (green up / red down), box-drawing wicks (`│`) and block bodies (`█`).

```q
plot t`px                               / braille line chart of a price column
plot (10*{sin x%6}'!120;100;16)         / (series; width; height)
candle bars[10; select from trades where sym=`AAPL]
```

Both print directly (via `` `0:``); the C kernels are `plotC`/`candleC` in `i.c`. See
`examples/graphs.k` for a 13-chart tour.

## 9d. Apache Arrow C Data Interface — `arrow.export` · `arrow.import`

Zero-dependency interop with PyArrow / Polars / DuckDB over the stable Arrow C ABI (no
`libarrow`). `arrow.export t` → `(schemaAddr; arrayAddr)` (64-bit C-ABI pointers); export is
**zero-copy** — each Arrow child `buffers[1]` aliases the Amber column payload and a `release`
callback drops the refcount when the consumer finishes. `arrow.import (schemaAddr; arrayAddr)`
→ Amber table (a copy: Amber's inline object header precludes aliasing a foreign buffer),
translating format strings and validity bitmaps (→ `0N`/`0n`/null). Numeric widths, ranges
and symbol (utf8) columns round-trip exactly. Structs live in `a.h`, logic in `ar.c`.

## 10. Strings

`lower upper ltrim rtrim trim ss ssr sv vs like`

```k
upper "HeLLo"                 / "HELLO"
trim  "  ab "                 / "ab"
ss  ["abracadabra";"ra"]      / 2 9          (match offsets)
ssr ["abracadabra";"ra";"XX"] / "abXXcadabXX"
sv  ["/";("a";"bc";"d")]      / "a/bc/d"      (join)
sv  [10;1 2 3]                / 123           (base decode)
vs  ["/";"a/bc/d"]            / ("a";"bc";"d")(split)
like["abcde";"a*e"]           / 1             (glob: * and ?)
like[("cat";"dog";"cab");"c*"]/ 101b
```

---

## 10b. Binary serialization — `-8!` · `-9!`

`-8!x` encodes any K value into a compact, contiguous byte vector (`tC`); `-9!y` decodes that
vector back into the original value. The round trip is exact:

```k
(-9! -8! x) ~ x            / true for every supported shape
b:-8!+`a`b!(1 2 3;4 5 6)   / a table -> bytes
-9!b                       / -> the identical table
```

| | |
|---|---|
| `-8!x` | encode `x` to a byte vector |
| `-9!y` | decode a byte vector produced by `-8!` |

**What round-trips.** Atoms (`` `i `` `` `l `` `` `f `` `` `c `` `` `s ``, date, time, timestamp),
every vector type including bit vectors, symbol vectors, general/nested lists, dictionaries,
tables, keyed tables, empty vectors and the empty general list, nulls (`0N`, `0n`), infinities
(`0w`, `-0w`), and **column attributes** — a `` `s``-sorted column arrives still sorted and keeps
the O(log n) binary-search path in `?`.

Two things worth knowing:

- **Symbols travel as names, not ids.** Symbol ids are process-local, so shipping raw ids across a
  `peach` fork (or between runs) would decode to the wrong name. Names are re-interned on decode.
- **Functions are not supported.** Lambdas, projections, compositions and derived verbs raise
  `'type`: serializing a closure means serializing its captured environment and bytecode, which is
  a different feature from a data wire format.

`-9!` is a parser for bytes that may have come from another process, so it is bounds-checked and
depth-limited throughout; a truncated, corrupt or over-long buffer raises a clean `'domain` rather
than reading past the end.

`-8!`/`-9!` occupy the negative-integer `!` slots, as in q. Only `-8` and `-9` on an integer atom
are intercepted — every other left argument still means `mod`.

`peach` uses this as its worker wire format (§9a). See `examples/peach_verify.k` for the full
verification suite.

## 10a. Temporal

**Native types (1.7).** `date`, `time` and `timestamp` are first-class types with literal
syntax and type-aware arithmetic:

```q
2026.07.30                      / date  (days since 2000.01.01)
10:00:00.000 + 00:00:05.000     / time + time -> 10:00:05.000
2026.08.15 - 2026.07.30         / date - date -> 16 (days) ; date+n -> date
2026.07.30D09:30:00.000000000   / timestamp (ns since 2000.01.01)
year 2026.07.30                 / accessors: year month day dow  ·  thh tmm tss (time)
"D"$"2026.12.25"                / string casts: "D"$ (date) "T"$ (time) "P"$ (timestamp)
`i$2026.07.30                   / extract the raw numeric value
```

Columns keep numeric storage (as kdb does internally), so `xasc` and the `s#` attribute work
unchanged and a `time`-named column auto-renders as `HH:MM:SS.mmm` in a grid.

Underneath, a **time of day** is an integer number of **milliseconds since midnight**, the
same convention as kdb+’s `time`. The q dotted temporal accessors map to plain Amber calls:

| q            | Amber        | meaning                              |
|--------------|--------------|--------------------------------------|
| `t.hh`       | `hh t`       | hour of day (0–23)                   |
| `t.mm`       | `mm t`       | minute of hour (0–59)                |
| `t.ss`       | `sec t`      | second of minute (`ss` = string-search) |
| `t.minute`   | `minute t`   | minutes since midnight               |
| `t.second`   | `second t`   | seconds since midnight               |
| (millis)     | `milli t`    | millisecond (0–999)                  |
| build        | `hms[h;m;s]` | construct a time                     |
| parse        | `ptime "HH:MM:SS.mmm"` | string → ms                 |
| format       | `stime t`    | ms → `"HH:MM:SS.mmm"`                 |

Bucketing for bars:

```q
minbar[w;t]    / w-minute bar, returned as a time (ms)
bar[w;u;t]     / generic: w buckets of u ms (bar[5;60000;t] = 5-min bars)
xbar[w;x]      / plain bucketing of any integer column
```

Classic 1-minute OHLCV, exactly like a q tickerplant query:

```q
tb: +@[+trade; ,`time; minbar[1]@]        / snap the time column onto 1-minute bars
qby[tb; `sym`time;
    `open`high`low`close`vol!({first x`px};{max x`px};{min x`px};{last x`px};{sum x`sz})]
```

`tsym[t;c]` renders the time columns `c` as `HH:MM:SS.mmm` when you `show` a table. See
`examples/tick.k` for a complete trades + quotes + as-of/window + OHLC walkthrough.

## 11. Full library index

```
scalar     neg not null reciprocal sqrt floor ceiling signum abs exp log sin cos
           til enlist string type key value first last reverse distinct group where
           flip count mod div xbar xlog round
aggregate  sum prd min max avg med var dev svar sdev cov scov cor wsum wavg all any
uniform    sums prds mins maxs deltas ratios differ prev next
order/set  rank iasc idesc asc desc xrank xprev rotate in except inter union raze
           sublist cross
tables     istable isdict iskeyed cols keys rows atr xkey unkey xcolall xcols xasc
           xdesc meta insert
grouping   qwhere qselect qby xgroup ungroup fby
joins      lj ij uj pj ej aj aj0 asof wj
strings    lower upper ltrim rtrim trim ss ssr sv vs like lk1
temporal   hms hh mm sec milli minute second stime ptime  bar minbar tsym
           year month day dow thh tmm tss  dstr pdate pstr ptstamp  (native types)
display    show amfmt amtab amkeyed amdict  plot candle
           COLOR CT cwrap vlen vstrip (ANSI highlighting)  CROWS (preview height)
attributes `sa `ua `pa `ga (set sorted/unique/parted/grouped)   `at (get)  [kernel primitives]
moving     mcount msum mavg mprd mvar mdev mmin mmax   (std.k, O(n) prefix)
math       dot mmu (matrix multiply)                   (std.k)
parse/ser  parse eval reval ser deser protect          (std.k; text serialise)
cast       long int float char sym bool cast           (std.k)
parallel   peach (multi-core, fork-based C kernel)      (std.k) ; ts (\ts timing)
.z/.Q/.j/.h  z.p z.d z.t … · Q.f Q.dd Q.trp Q.s … · j.j j.k (JSON) · h.ht (HTML)  (sys.k)
on-disk    dset dget splay dload partsave partload parts   (hdb.k, text-serialised)
ipc/tick   hopen hclose hsend hrecv hsync · u.def u.sub u.pub u.get u.end  (ipc.k)
arrow      arrow.export arrow.import                    (Arrow C Data Interface)
```

### 11a. Extended modules (`std.k` `sys.k` `hdb.k` `ipc.k`)

Loaded automatically by `repl.k` after `amber.k`/`fin.k`. They add, in lightweight text‑based
form, a large slice of q's system vocabulary:

* **`std.k`** — vectorised **moving aggregates** (`mcount msum mavg mprd mvar mdev mmin mmax`,
  O(n) prefix sums), a little linear algebra (`dot`, `mmu`), **`parse`/`eval`/`reval`** and a
  text **`ser`/`deser`** round‑trip (portable Amber text, *not* the kdb binary `-8!`/`-9!`),
  `protect` (like `.Q.trp`), typed cast helpers, `peach`, and `ts` (time an expression).
* **`sys.k`** — the `.z` clocks/handlers (`z.p z.P z.n z.d z.D z.t z.T z.z`; `z.pg z.ps z.po
  z.pc z.ts z.exit` are stubs), `.Q` utilities (`Q.f Q.fmt Q.s Q.ty Q.qt Q.id Q.dd Q.gc Q.w
  Q.fc Q.trp`), `.j` JSON (`j.j`/`j.k`), a minimal `.h` HTML renderer, and `plot`/`candle`.
* **`hdb.k`** — on‑disk data: `dset`/`dget` (value ↔ file), `splay`/`dload` (splayed table ↔
  directory, one file per column with a `.d`), `partsave`/`partload`/`parts` (value‑partitioned
  database with `par.txt`). Storage is portable Amber text read back with `eval` — human‑readable
  and version‑independent, but not memory‑mapped.
* **`ipc.k`** — raw‑socket messaging (`hopen hclose hsend hrecv hsync`, text protocol — not the
  kdb binary wire) and an in‑process tickerplant (`u.def u.sub u.pub u.get u.end`).

## Help inside the REPL

Type `\` for the menu, then a topic: `\q` (scalars, aggregation, sets, strings),
`\j` (tables, keyed tables, joins, qSQL), `\z` (temporal, bars, attributes, display),
`\m` (finance/HFT module). `\0 \+ \' \`` `\h` cover the core array language and its cheat-sheet.

**Session commands** (one-liners in the `\` menu itself, no separate topic page):
`\l file.k` load a script, `\d ns` switch/show namespace, `\t:n expr` time `n` runs, `\f` list
functions, `\cd path` change directory, `\grid MODE` set the table border
(`clean`/`rounded`/`sharp`/`heavy`), `\clear` clear the screen, `\a` print the licence, `\\` exit.

**Diagnostics:** `\v` a rich workspace inspector (every global as a Name/Type/Shape/Memory
table — see `src/inspect.{h,c}`); `\ast expr` a colour-coded parse tree, parse-only, nothing is
executed (`src/ast.{h,c}` — every leaf is typed explicitly, `Int64`/`Float64`/`Symbol`/`Char`/a
`(TypeName Vector[len])` preview, never a generic placeholder; tacit hooks `(f g)`, forks
`(f g h)`, and curried projections `1+`/`f[x;;z]` get their own explicit labels; see
[CHANGELOG](CHANGELOG.md)); `\trace expr` a 4-phase timing report (parse/arena/exec/format) plus
the arena's peak scratch usage for that evaluation, running the same qSQL rewrite the prompt uses
so tracing a table or `select …` expression renders correctly (`src/trace.{h,c}`); `\disasm expr`
compiles an expression and prints the real bytecode Amber's compiler/VM (`src/b.c`) produces for
it — locals, constant pool, instruction stream — without executing it (`src/vm.{h,c}`).

**Engine extensions** (all additive, standalone modules — see the README's
[Engine extensions](../README.md#engine-extensions) section for full detail and benchmarks):
SIMD vector kernels (`src/simd.{h,c}`, AVX2/NEON/scalar, self-test `` `simd 0``), a
multithreaded vector engine for arrays over 100,000 elements (`src/parallel.{h,c}`, self-test
`` `para 0``), and a native CSV parser that reads a file straight into a typed table
(`src/csv.{h,c}`, `` `csvr "path.csv"``, self-test `` `csv0 0``).

---

## 12. Worked example

```k
\l amber.k

/ build two tables
trade:+`sym`time`px`sz!(`ibm`msft`ibm`msft`ibm; 1 2 3 4 5; 100 50 101 51 102; 10 20 30 40 50)
ref  :xkey[`sym; +`sym`name!(`ibm`msft; `"Big Blue"`"Redmond")]

/ enrich with a left join
enriched: lj[trade; ref]

/ VWAP by symbol
qby[trade; `sym; (,`vwap)!,{wavg[x`sz; x`px]}]

/ as-of the reference prices at each trade time
quote:+`sym`time`bid!(`ibm`ibm`msft`msft; 1 4 1 3; 99 100 49 50)
aj[`sym`time; trade; quote]

/ fast repeated lookups: attribute a sorted integer key
ids: asc distinct trade`time      / `s-attributed integer vector
`at ids                           / `s   -> `?/`in on ids run in O(log n)
in[3 5; ids]                      / binary-searched membership
```

---

*Amber — a low-latency array language. GNU AGPLv3.*
