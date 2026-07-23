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
| `test.k`    | the test suite (104 assertions)                               |
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
# 104 tests run, 0 failures
# ALL TESTS PASSED
# =================================================
```

Start an interactive session (a line‑editor wrapper is recommended for history/editing):

```sh
rlwrap ./amber repl.k          # rlwrap or rlfe; both optional
```

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

## 7. Selecting and grouping (qSQL, functional form)

Because Amber is k, qSQL is expressed with functions rather than the `select … by … from …`
sugar. The pieces:

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

`aj` sorts `quote` on `` `sym`time`` internally and uses **binary search** (`bin`) inside each
symbol group, so it is O(n log m).

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

kdb+ attaches *attributes* to vectors to speed up operations. Amber implements the **sorted
attribute** (`` `s``, kdb+’s `` `s#``) at the **kernel level**, because that is where search
lives. Two symbol‑verbs are exposed by the interpreter:

```k
`sa x     / set the sorted attribute on vector x  (returns x, attribute = `s)
`at x     / read the attribute of x               (`s if sorted, ` otherwise)
```

`asc` and `xasc` apply `` `sa`` for you, so idiomatic sorted data is attributed automatically.

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
  (`#define _at(x) (*(UC*)((x)-13))`). Attribute codes: `0` = none, `1` = sorted.
* **`m.c`** — the allocator `an()` now zeroes `_at(x)` on every allocation, so the flag is
  well‑defined for every object (the free‑list path did not previously clear this byte).
* **`a.c`** — two functions, `qsa` (set) and `qat` (get), wired into the `sym1` symbol‑verb
  dispatch table as `` `sa`` and `` `at``. `qsa` marks a simple vector sorted; `qat` reports it.
* **`f.c`** — four binary‑search probes `bGL/bHL/bIL/bLL`, and a one‑line change in `fnd` so the
  integer find path selects them when `` _at(x)==1`` (and the type is `tH/tI/tL`, not float/symbol):

  ```c
  B srt = !_tP(x) && xt!=tF && xt!=tS && _at(x)==1;
  TY(fGL)*f = (srt ? G(&bGL,bHL,bIL,bLL) : G(&fGL,fHL,fIL,fLL))[xw-3];
  ```

The attribute is intentionally **dropped by operations that build new vectors** (the byte is
zero on allocation), matching kdb+ semantics: it is a promise about *this* vector’s current
contents, re‑established by `asc`/`` `sa`` when you know the data is ordered.

Only integer widths are accelerated; floats and symbols keep the scan (raw‑bit order ≠ value
order for floats, and symbol order is interning order). This keeps results exactly correct.

---

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

## 10a. Temporal (tick.minute style)

Amber holds a **time of day** as an integer number of **milliseconds since midnight**, the
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
display    show amfmt amtab amkeyed amdict
attributes `sa (set sorted)   `at (get)     [kernel primitives]
```

## Help inside the REPL

Type `\` for the menu, then a topic: `\q` (scalars, aggregation, sets, strings),
`\j` (tables, keyed tables, joins, qSQL), `\z` (temporal, bars, attributes, display).
`\0 \+ \' \`` cover the core array language.

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

## v1.5 extended modules

Six modules load automatically after `fin.k`. Reach their help with `\w \s \u \y`.

### qSQL template — `qsql.k` (`\s`)
```q
sel "select c:e,… [by b,…] from t [where p,…]"   / -> table (keyed if by)
exq "exec e from t [where p]"                     / -> a single column
upd "update c:e,… [by b,…] from t [where p]"      / -> the updated table
del "delete [c,…] from t [where p]"               / -> rows kept / cols dropped
qexec[t; where-mask|() ; by-cols|() ; name!fns|()]/ functional form
```
Bare column names in the expressions are rewritten to `` x`col ``, so `wavg[sz;px]`,
`sum sz`, `max px` all work. Example:
`sel "select vwap:wavg[sz;px],n:#px by sym from trades where px>100"`.

### Moving / window + tooling — `std.k` (`\w`)
```q
msum[w;x] mavg[w;x] mcount[w;x] mprd[w;x] mvar[w;x] mdev[w;x]  / O(n) prefix-based
mmin[w;x] mmax[w;x]                                            / O(n*w) window
mmu[x;y] dot[x;y]                                              / matrix mult, dot
parse s   eval s   reval s   ser x   deser s   protect s        / parse/eval/serialize
long int float char sym bool   cast[`type;x]                    / casting
peach[f;x]   ts"expr"                                           / parallel-each, timer
```

### Date & timestamp types — `temporal.k` (`\u`)
`date` = days since 2000.01.01; `timestamp` = nanos since 2000.01.01.
```q
ymd2d[y;m;d]  d2ymd d  dstr d  pdate s  year d  month d  dayof d  dow d  dadd[d;n]
tstamp[date;ms]  tsdate p  tsms p  pstr p
```

### System namespaces / on-disk / IPC — `sys.k` `hdb.k` `ipc.k` (`\y`)
```q
z.p z.P z.n z.d z.D z.t z.T z.z              / clocks (kdb .z.* without the leading dot)
Q.f[n;x] Q.fmt[w;n;x] Q.s x Q.dd[a;b] Q.fc[f;x] Q.id x Q.trp[f;a;h]
j.j x   j.k s                                / JSON encode / decode
h.ht t  h.hc[tag;s]                          / HTML
dset[f;x] dget f  splay[dir;t] dload dir  partsave[dir;t;`c] partload dir  parts dir
hopen "host:port"  hclose h  hsend[h;m]  hrecv h  hsync[h;q]    / raw-socket IPC
u.def[nm;schema]  u.sub[nm;cb]  u.pub[nm;data]  u.get nm        / in-process tickerplant
```
Notes: `z.*` etc. drop kdb's leading dot (in this core `.` is the eval verb). On-disk
data is stored as portable Amber text — ideal for modest tables; very large single
columns exceed the text parser (binary serialization is on the roadmap). IPC uses raw
sockets, not the kdb+ binary wire protocol.

### Interpreter capacity
The bytecode's global index was widened from 1 byte to 2 (256 → 4096 globals) so the
whole extended vocabulary (287 globals) loads at once.

---

## Amber Notepad
`Amber-Notepad.html` is a single self-contained page — a JavaScript re-implementation of
a faithful Amber subset behind a notebook UI (amber-phosphor theme, live evaluation,
syntax highlighting, rendered tables with attributes). Open it in any browser; no install.

---

*Amber — a low-latency array language. GNU AGPLv3.*
