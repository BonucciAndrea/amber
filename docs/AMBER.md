# Amber

**Amber** is a low-latency **array language**: a fast, compact array interpreter that provides
the major functionality of **q**: aggregations, dictionaries, tables, keyed tables,
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

Since **1.9.5** the REPL has its own line editor (`src/ln.c`) with history, arrow keys and
`Ctrl-A/E/W/U/K`, so **do not wrap it in `rlwrap` or `rlfe`**. (Tab completion was removed at
2.0.0: with k's terse syntax it was near-useless and uncomfortable to use, and Tab is now a no-op.)
The editor is UTF-8 aware: accented letters, CJK and emoji all measure as the cells the terminal
draws, and Backspace removes a whole character rather than one byte. A line that leaves a
bracket open (`{`, `(`, `[`) is treated as an incomplete statement: the editor shows a `...>`
prompt and keeps reading until the brackets balance, then evaluates the joined statement as one.
`Ctrl-C` abandons a continuation. A bracketed paste of a multi-line function is rejoined by the
same rule, so a function typed by hand and one pasted produce identical text. Amber puts the terminal in raw mode and reads single keypresses, which is
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

Amber follows a terse **array grammar**, which differs from q in a few ways you must know:

* **The two-argument library dyads work infix *or* in bracket form (since 2.0.0).** The join
  family and the set/search dyads (`in within like lj ij uj aj aj0 wj wj1 pj ej cross inter
  union except ss sv vs xasc xdesc`) may be written `x f y` just like q, as well as
  `f[x;y]`: `t lj kt`, `2 3 in 1 2`, `5 within 3 9`, `` `sym xasc t``, `` "/" sv `a`b`c`` all work,
  and are identical to the bracket call. Built‑in verbs
  (`+ - * % ! & | < > = ~ , ^ # _ $ ? @ .`) are infix as usual. An *arbitrary* user lambda is
  still bracket/prefix only (`{x+y}[a;b]`, not `a {x+y} b`); see the CHANGELOG for why lambda
  infix is excluded.
* **No `>=` / `<=` operators.** Use `~a<b` for `a>=b` and `~a>b` for `a<=b`.
* **Symbols cannot contain `_`.** `` `a_b `` is a parse error; use a quoted symbol `` `"a_b" ``.
* **Nested lambdas are not closures.** An inner `{…}` sees only its own parameters and globals,
  never the enclosing function’s locals. The library passes captured values in by projection
  (`f[captured]'list`); do the same in your own code.
* **`.x` after a *name* is dyadic apply, after a *verb* is monadic “value”.** The library uses the
  `value` function (`value x` ≡ `. x`) to avoid the ambiguity; prefer it in your code.

These are properties of the host, not bugs, and the library is written to respect them.

### Tacit trains: hooks and forks (since 2.0.1)

A parenthesised, **semicolon-separated** list whose every element is a function
is applied as a **train** rather than indexed. `::` counts as data here, not as a
function, so a list holding it is indexed: `(::;::)@0 1` is `(::;::)`. Two shapes exist:

| shape | name | monadic `t y` | dyadic `t[x;y]` |
|-------|------|---------------|-----------------|
| `(f;g)`   | **hook** | `y f (g y)`         | `x f (g y)`             |
| `(f;g;h)` | **fork** | `(f y) g (h y)`     | `(x f y) g (x h y)`     |

```text
avg:(+/;%;#)                      / mean, tacitly:  (sum % count)
avg 2 4 6 8 10                    / 6.0
(|/;-;&/) 5 2 9 1                 / range = max - min = 8
(%;+/) 1 2 3 4                    / normalise to sum 1 -> 0.1 0.2 0.3 0.4
(,;|) 1 2 3                       / a value followed by its reverse -> 1 2 3 3 2 1
(+;*;-)[10;3]                     / dyadic fork -> (10+3)*(10-3) = 91
```

The elements may be primitives, derived verbs (`+/`), lambdas or projections; a
train is an ordinary value, so it can be named, stored and passed around. Apply
it **monadically by juxtaposition** (`t y`) or **dyadically in bracket form**
(`t[x;y]`). Note the distinction from a **space-separated** group: `(f g)` (no
semicolon) is plain right-to-left composition `f(g x)`, whereas `(f;g)` is a
hook. Any list that is not exactly two or three functions, whether four+ verbs or a
list with a non-function element, still **indexes** as before. See the online
[Tacit programming guide](https://amber-lang.org/docs/tacit.html) for a full tour.

---

## 3. Type & null quick reference

| k type | list / atom | example            | null   |
|--------|-------------|--------------------|--------|
| int    | `` `I``/`` `i`` | `0 1 2`, `!5`, `101b` | `0N` |
| float  | `` `F``/`` `f`` | `1.5 2.5`       | `0n`   |
| char   | `` `C``/`` `c`` | `"abc"`, `"x"`  | `" "`  |
| symbol | `` `S``/`` `s`` | `` `a`b`c``     | `` ` `` |
| dict   | `` `m``      | `` `a`b!1 2``      |        |
| table  | `` `M``      | `` +`a`b!(1 2;3 4)`` |      |

`@x` returns the type symbol. `!n` gives `0..n-1` (a compact range whose `@` reads `` `I``).
There is no separate bool type: a `b` literal such as `101b` is an int vector (`@101b` is `` `I``),
and comparisons give ints 0 and 1.

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

**Adverb words** (q's, infix: `last each x` is `last'x`)
`each over scan prior`

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
sublist[1 2;1 2 3 4 5] / 2 3        ((start;count), as q)
cross[1 2;10 20]     / ((1;10);(1;20);(2;10);(2;20))
```

`asc` returns a sorted vector **with the sorted attribute set**, so a subsequent `?` on it (an int
vector stored 16 bits or wider) runs in O(log n); see §9.

---

### 5a. Fused idioms and one-pass kernels (since 2.1.0)

The compiler recognises a few expression shapes and compiles them to one fused primitive
instead of a chain that materialises intermediates. The result is identical to the unfused
expression in every case (the fused entry point falls back to the same primitives for any
operand shape it does not handle):

```
+/x*y  +/x=y  +/x<y  +/x>y     sum of products / count of matches, no product or mask vector
+/x@&m                        sum of the elements selected by a 0/1 mask, no compressed vector
x@&m                          compress by mask, no index vector
a+s*b  a-s*b   (s a literal)  one pass, no s*b vector
x@<x   x@>x                   sort by value: counting sort for integral-valued data, else grade+index
```

`\disasm +/x*y` shows the fused call. Two one-pass kernels are also reachable directly:

```
`memb(x;y)                    x in y, one byte per element   (amber.k's `in` uses it)
`gagg(op;k;v)  `gagg(op;k;v;m) group aggregate: op in `sum`count`min`max`avg`first`last,
                              returns (keys;values;first row per group) in first-appearance
                              order; m is a 0/1 mask of rows to keep. Wrapped by
                              gsum[k;v] gavg gmin gmax gcount[k] gfirst glast -> keys!values
```

**Fused element-wise trees (since 2.6.0).** Inside a lambda, a tree of `+ - * % & |` over float
or int vectors and numbers (with `< > =` allowed at the top, and `+/` on top of it all) compiles to
one fused op. At run time, if the vectors are 2048 items or more, the whole tree runs in one pass
of 512-item blocks that stay in cache, split over the threads, instead of one full vector per verb.
Every element comes out the same double or int the unfused code makes, int widths included.
Anything it doesn't handle just runs the normal code.

```k
f:{[a;b;c]+/(a*b)-c%2.0}    / one pass, no a*b or c%2.0 vectors
```

Every ascending value sort (`asc x`, `x@<x`, `` `srt x``, a single-column `xasc`) returns its
result flagged `` `s``, so `?` on it, when it is an int vector stored 16 bits or wider, takes the O(log
n) path without an explicit `` `sa``.

## 6. Dictionaries, tables and keyed tables

A **dictionary** is `keys!values`; a **table** is a flipped column dictionary `+d` (type `` `M``);
a **keyed table** is a dictionary whose key *and* value are both tables (exactly q’s model).

```k
t:([]sym:`a`b`c; px:100 200 300; sz:10 20 30)   / a 3-row table (literal syntax)
t`px                 / 100 200 300            column access
t 1                  / `sym`px`sz!(`b;200;20) row as a dict
#t                   / 3                       row count
cols t               / `sym`px`sz
```

Assigning to a table's columns:

- `` t[`px]:v `` or `` @[t;`px;f] ``: an atom fills every row (an empty table stays empty); a list or dict
  must have the row count, unless every column is replaced (`` t[`px]:7 8 `` on a one-column table gives
  two rows).
- `` .[t;(::;`px);:;v] `` amends every row; with no rows the table is kept when `v` is an atom or empty,
  else `'length`.

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
kt`b                 / `px`sz!200 20          the row for key `b (kt@`b too)
kt`z                 / `px`sz!0N 0N           a key not there: a row of nulls
kt[`b;`px]           / 200
```

A keyed table is looked up by key as in q: for one key column the key is a value, for several it is a
list of one value per column, or a dict of them (`` m(1;`x) ``, `` m`a`b!(1;`x) ``). A table of keys,
or a list of keys, gives a table of rows; a row or table goes by column name. A key not there gives the
row `t 0N` gives (`::` in a temporal column and blanks in a string one, where q has typed nulls). `t?r`
finds a row (a dict) or each row of a table in `t`, and `kt _ k` drops a key.

`kt,kt2` is q's upsert: the rows of `kt2` whose keys `kt` has replace them, and the others are added,
each one, as q (a new key given twice is added twice); the columns go by name. Amending at keys adds a
key not there once, with a row of nulls, then amends its row: `` kt[`d]:4 40 `` (a list is the row's
values in column order), `` kt[`b;`px]:5 ``. An amend that fails leaves a variable that is a keyed table as it was; one
that fails on a keyed table inside a dict or list (`` h[`a;1]:5 ``) still loses the variable.

`kt+kt2` (and `- * % & |`) works over the union of the keys, as for dicts: a key on both sides gets the verb row by
row, a key on one side passes its row through (`-` negates one of `kt2`'s), `kt`'s keys first; the key and value
columns go by name. Comparing two keyed tables (`< > =`) is still `'nyi`.

---

## 7. Selecting and grouping (qSQL)

Amber supports the **`select … by … from … where …` template directly**. Type it at the
prompt with no wrapper:

```k
select avg px by sym from trades where px>100     / grouped aggregate
select sym,px from trades where px>180            / chosen columns, filtered
exec avg px from trades where sym=`AAPL           / one column/expression -> a vector
exec px,sz from trades where sym=`AAPL            / several -> a `px`sz dict of columns
update mid:0.5*bid+ask from quotes                / add/replace columns
delete from trades where sz<300                   / drop rows
r:select n:#px, avg px by sym from trades         / assign; also  5#select …  count select …
```

Bare column names in the expressions become `x`col`, so `wavg[sz;px]` just works. Columns get
q's names: the first name in the expression (`sum px*sz` is `px`, `2*px` is `x`), repeats
numbered (`px px1 px2`). An atom next to full columns is stretched to the column length, and
`where a, b` cascades as in q: `b` only sees the rows `a` kept. The same
templates are also callable as strings (`sel"select …"`, `exq`, `upd`, `del`) and via the
**functional form** below (handy when you build the query programmatically).

### Sorted and limited selects: `select[…]`

```k
select[5] from trades                 / the first 5 rows
select[-5] from trades                / the last 5
select[10 5] from trades              / 5 rows from row 10
select[>px] from trades               / sorted by px, descending
select[<sym] from trades              / ... ascending
select[5;>px] from trades             / sorted, THEN limited -- q's order
select[<sym;>px] from trades          / several keys: the FIRST listed is primary
select[3;>s] s:sum px by sym from trades      / on a grouped result too
select[2;>px*sz] from trades          / a sort key may be any expression
```

The clauses run in q's order: **where → by/select → sort → limit**. A bracket item beginning
`>` or `<` is a sort key; anything else is the row limit, and a negative limit takes from the
end; two numbers are a first row and a count, as q's (a null, or a negative in a pair, is `'type`).
A bare column name goes through `xasc`/`xdesc`, so the result keeps the `` `s `` attribute where
that applies; any other expression is computed over the result and graded. On a keyed
(by-clause) result the spec is applied to the rows and the key is put back. The bracket is
accepted by the bare prompt form and by `sel"…"` alike, and only after `select`, since `exec`,
`update` and `delete` take no bracket in q.

**Interactive prompt vs. `.k` scripts.** The bare `select … from …` rewriting above is applied
line-by-line by `repl.k` (via `qsql.k`'s `qrw`) as you type at the `amber>` prompt. A script run
with `./amber file.k` does not go through that per-line rewrite, so bare qSQL sugar will not parse
there. Use the `sel"…"`/`exq"…"`/`upd"…"`/`del"…"` string forms, or call `qwhere`/`qselect`/`qby`
directly, exactly as `test.k` and every script under `examples/` already do.

**Inside a lambda.** The string forms (and bare qSQL, which turns into them) only see globals, so
`{[x] sel"select from t where n>x"}` can't see `x`. Use the functional forms there, they take locals fine:
`{[x] qwhere[t;t[`n]>x]}`.

| function             | q analogue                                     |
|----------------------|------------------------------------------------|
| `qwhere[t;mask]`     | `select from t where mask`                     |
| `qselect[t;a]`       | `select …` (`a` = `name!func` computed columns)|
| `qby[t;b;a]`         | `select … by b` → **keyed table**              |
| `xgroup[k;t]`        | `` `k xgroup t`` (keyed by k, nested values)    |
| `ungroup x`          | flatten nested columns                          |
| `fby[(f;d);g]`       | `(f;d) fby g`, where `g` may be one column, a list of columns, or a table; works inside a where-clause |
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
fby[(sum;t`sz);t`sym]                  / 90 60 90 60 90
/ ... on several group columns, as a list of columns or as a table:
fby[(sum;t`sz);(t`sym;t`ex)]           / grouped on the (sym;ex) pair
fby[(sum;t`sz);([]s:t`sym;e:t`ex)]     / the same thing
/ and infix, inside a where-clause, exactly as in q:
sel"select from t where px=(max;px) fby sym"
```

`qby` returns a **keyed table** keyed on the by‑columns, just like q.

---

## 8. Joins

Every join is a function; call it in bracket form `lj[t;kt]` or, since 2.0.0, **infix** `t lj kt`
(both are identical). Left operand is the “driver” table.

| function             | kind                | notes                                   |
|----------------------|---------------------|-----------------------------------------|
| `lj[t;kt]`           | **left** join       | `kt` keyed; unmatched → nulls           |
| `ij[t;kt]`           | **inner** join      | keep matched rows only                  |
| `uj[x;y]`            | **union** join      | union of columns, rows concatenated; keyed: union of keys, `y`'s rows update `x`'s, as q |
| `pj[t;kt]`           | **plus** join       | add matched numeric value columns       |
| `ej[c;x;y]`          | **equi** join on `c`| inner join on the given columns         |
| `aj[c;x;y]`          | **as‑of** join      | last `y` row per key with time ≤ `x`.time |
| `aj0[c;x;y]`         | as‑of, `y`’s time   | like `aj` but result time is `y`’s      |
| `asof[t;d]`          | as‑of lookup        | single as‑of row for the dict `d`       |
| `wj[w;c;t;q;aggs]`   | **window** join     | aggregate `q` over a window per `t` row |
| `wj1[w;c;t;q;aggs]`  | window join, inside | like wj, but only the quotes inside the window (wj also counts the one in force at its start, as q) |

`c` is `` `key…`time``: the last name is the ordering (time) column, the rest are exact‑match keys.

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

q attaches *attributes* to vectors to speed up operations. Amber implements **all four
of them**, sorted (`` `s``), unique (`` `u``), parted (`` `p``) and grouped (`` `g``),
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
`fin.k`'s `gentq` puts `` `p`` on `sym`. Only **sorted** int vectors stored 16 bits or wider get the binary-search find.
Parted just means equal values sit together, not that they're in order, so it stays on the
hash path. Grouped pairs with `fin.k`'s group index for O(1) per-symbol slicing.

Since 2.3 an attribute gets checked when you set it, like q: `` `sa`` on unsorted data is
`'s-fail` (same idea for `'u-fail`, `'p-fail`). Writing into the vector in place drops it.
Before that, `s[0]:9000` on a sorted `s` kept `` `s``, and `s?9000` said `0N`.

### Setting them the q way (since 2.7.0)

```q
`s#1 2 3          / sorted        `s#3 1 2 is 's-fail
`u#`a`b`c         / unique        `u#1 1 2 is 'u-fail
`p#1 1 2 2        / parted        `p#1 2 1 is 'p-fail (q says 'u-fail there; we kept the clearer name)
`g#`a`b`a         / grouped       anything goes
`#x               / take it off
attr x            / `s `u `p `g, or ` for none
```

It works the way kdb+ 4.1 does, checked case by case against q.exe: every list can carry one (dates,
times, chars, bools and general lists too), an atom is `'type`, `` `p `` on a general list is `'type`.
Some table and dict rules:

* `` `s#t `` wants the whole table in row order. The table is `` `s `` and its first column gets
  `` `s `` (one column) or `` `p `` (more). `` `u `` `` `p `` `` `g `` on a table or dict is `'type`.
* `` `s#d `` on a dict (or keyed table) checks the keys and makes lookups step: `` (`s#1 3 5!`a`b`c) 4 ``
  is `` `b ``, the value at the last key not above 4. Below the first key you get a null.
* What keeps one is what q keeps: `asc` gives `` `s ``, `distinct` keeps `` `s `` and turns `` `u ``/`` `g ``
  into `` `u `` (and `group`'s keys do the same), and anything that hands back the same list (`(count x)#x`, `x,()`, `raze enlist x`).
  `reverse`, appends, indexing, arithmetic and amends drop it.
* `` update `g#a from t `` and `` select `s#a from t `` work, and the column keeps its name.

`tests/test_attr.k` has 543 of these cases with q's answers written in, and the session fuzzer ran
12,000 random ones against q with no difference.

### Why it makes search faster

Without the attribute, Amber’s find (`?`) of one item in an integer vector stored 16 bits or wider is an
**O(n) linear scan** (`f.c: fGL/fHL/fIL/fLL`). Binary search (`bin`, the `x'y` form) already exists but you have to
ask for it. The sorted attribute lets `?` *decide for itself*: when the left vector is
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

* **`a.h`**: a new header accessor `_at(x)` at the previously‑unused header byte `-13`
  (`#define _at(x) (*(UC*)((x)-13))`). Attribute codes: `0` = none, `1` = sorted, `2` = unique,
  `3` = parted, `4` = grouped.
* **`m.c`**: the allocator `an()` now zeroes `_at(x)` on every allocation, so the flag is
  well‑defined for every object (the free‑list path did not previously clear this byte).
* **`a.c`**: five functions, `qsa`/`qua`/`qpa`/`qga` (set) and `qat` (get), wired into the
  `sym1` symbol‑verb dispatch table as `` `sa`` `` `ua`` `` `pa`` `` `ga`` and `` `at``. Each
  setter marks a simple vector with its code; `qat` reports it as `` `s`u`p`g``.
* **`f.c`**: four binary‑search probes `bGL/bHL/bIL/bLL`, and a one‑line change in `fnd` so the
  integer find path selects them when the vector is **sorted** (and the type is
  `tH/tI/tL`, not float/symbol):

  ```c
  B srt = !_tP(x) && xt!=tF && xt!=tS && _at(x)==1;
  TY(fGL)*f = (srt ? G(&bGL,bHL,bIL,bLL) : G(&fGL,fHL,fIL,fLL))[xw-3];
  ```

The attribute is intentionally **dropped by operations that build new vectors** (the byte is
zero on allocation), matching q semantics: it is a promise about *this* vector’s current
contents, re‑established by `asc`/`` `sa`` when you know the data is ordered.

Only integer widths are accelerated; floats and symbols keep the scan (raw‑bit order ≠ value
order for floats, and symbol order is interning order). This keeps results exactly correct.

---

## 9a. Parallelism: `peach`

`peach[f;y]` is a drop-in parallel `` f'y `` (each). It runs `f` over the items of `y` on a
persistent pool of worker threads (`src/peachpool.c`), sized by `AMBER_THREADS` (default: the
online CPU count), and the result is **identical** to serial `` f'y `` for every value (vectors,
symbols, tables, nested, ragged). No fork, no copying `y` to a child, no serialising the results
back; that's what it used to cost before the pool.

Workers can read globals but not assign them: a global assignment inside `f` is
`'noupdate`, so there's nothing to race on. Return the values instead. An error inside `f` comes
back out of `peach` with its own message, so it traps like any other:

```k
.[{peach[{$[x=5;`err"boom";x]};!10]};,0;{[e]"caught: ",e}]   / "caught: 'boom..."
```

```k
peach[{avg x?1.0}; 8#800000]         / 8 heavy tasks across cores: ~4x on 8 threads
AMBER_THREADS=8 ./amber examples/peach.k   / a Monte-Carlo demo timing serial vs peach
```

Use it when each item is real work (Monte-Carlo, per-symbol fits, bootstraps, partition loads).
Tiny items are fine too if you chunk them so each task does tens of microseconds; a 400-basket
payoff sweep goes 451 ms -> 215 ms on 14 threads. `AMBER_THREADS=1` forces serial, and the wasm
build has no threads, so it runs serially there. Numbers in BENCHMARKS.md §4.

### Threads inside primitives (since 2.6.0)

You don't have to ask for these. Once a vector gets big, these primitives split it over the same
`AMBER_THREADS` pool `peach` uses:

```
sort and grade      asc desc < > xasc xdesc, ints and floats     32K items and up
search              ? (find) in                                   32K and up
as-of join          the aj match                                  128K and up
gather              x@i with a big index                          64K and up
distinct            ? of ints                                     256K and up
group aggregates    `gagg / gsum gavg gmin gmax gcount ...       64K rows and up
float reductions    sum, dot, max, min                            1M items and up
fused trees         see 5a                                        2048 items and up
```

Small vectors never touch the pool, and `AMBER_THREADS=1` turns all of it off. The answers are
the same as on one thread, with one exception: float `sum`/`dot` of 1M+ items add in fixed 64K
blocks and then add the blocks in order. That's the same answer for any thread count, but the
last digits can differ from a plain left-to-right sum (and from 2.5.0). Float group sums and
averages add per 256K-row block the same way, from 256K rows.

## 9b. Display: Q-style grid preview

`show t` and a bare table / keyed table / dict at the prompt print only the first `CROWS`
rows (default **20**), then a `..` line to show there is more, exactly like q's console. The
cap is applied *before* formatting, so previewing a million-row table is instant. Set
`CROWS:10` (or any n) at the prompt to change the preview height; small results print in full.

A bare table / keyed table also prints a dimmed **size footer** `[N rows x M cols]` under the
grid (thousands-separated, e.g. `[1,000 rows x 4 cols]`); the counts are the true totals, taken
before the `CROWS` truncation.

**ANSI syntax highlighting.** Grid output (tables, keyed tables, dictionaries) is coloured for
dark terminals with a vivid 256-colour, **14-hue per-column palette** (`PAL`): each column gets a
distinct colour, headers are bold white, and nulls / the size footer are dimmed. Colour is applied
*after* width padding, and `vlen`/`vstrip` strip the escape bytes before every column-width and
header-underline calculation, so alignment is exact. Set `COLOR:0` to disable (e.g. when
redirecting output to a file); the per-column cycle lives in `PAL` and the type/attribute tints in
the `CT` dictionary.

**Table borders.** `\grid clean|rounded|sharp|heavy` picks the frame style: `clean` (default,
minimal dashed rule), `rounded` (`╭─┬─╮`), `sharp` (`┌─┬─┐`) or `heavy` (`┏━┳━┓`). Column widths
are measured with `vlen` (ANSI stripped) so the box lines up exactly around coloured cells; borders
are dimmed and the `[N rows x M cols]` footer sits below the bottom edge. `\clear` clears the
terminal.

Numeric and temporal columns are **right-aligned** (symbol / char / string columns left-aligned),
the header underline is dimmed grey, and null sentinels (`0N` / `0n` / `0w` / a blank cell) render
faint grey. Float precision in a grid is controlled by the **`PREC`** global, the number of
decimals to show (default `7`); set `PREC:0N` for full precision. `PREC` affects grid display only,
never the stored values.

## 9b′. Error ergonomics: Rust-style diagnostics

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

Before 1.9.4 the same failure printed **twice**: the report above, immediately followed by the
legacy ngn/k block (`'value` / source line / bare `^`). The compact form is still built (it is
what `.[f;args;handler]` receives and what `` `err`` returns), it is no longer echoed to
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
| `E0110` | `stack` | Call stack depth exceeded | Recursion deeper than the interpreter stack allows (2048 calls; a tail call, one whose result is the function's result, reuses the frame and does not count) |
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
`tests/harness.k` and `std.k`'s `protect` use so a suite that provokes errors it
then catches does not drown stderr in red. `` `diag`` with a non-numeric argument reads the
setting without changing it.

### Implementation

`src/diagnostic.{h,c}` holds the renderer: a `Span` (`src`, `start`, `end`, 1-based `line`/`col`),
`report_diagnostic_ex()` (code, title, label, secondary spans, help, note, colour) and the
back-compatible `report_diagnostic()`. The palette lives in `src/ansi.h` so other consumers can
match it. `src/e.c` owns the category catalogue (`edtab`), the token widener (`etok`) and `eD()`,
which every error path funnels through: the evaluator/compiler via `eS()` (`src/b.c`) and the
parser directly (`src/p.c`). Exercise it from Amber with the `` `dgn`` self-test builtin
(`` `dgn 0`` → `1`), which checks the layout, the underline characters, the inline label, gutter
alignment, ANSI suppression when colour is off, and the complete category → code matrix above.

---

## 9c. Terminal charts: `chart` and its presets

Charts are a first-class output format, not a debugging aid: framed, with a scaled and
labelled axis, several series at a time, and a downsampler that keeps a million-point series
readable. The drawing surface is **Braille**: a 2×4 dot bitmask per character cell, so a
`W`×`H` box is really a `2W`×`4H` raster.

Everything funnels into **`chart`**, which takes a dictionary of options. The other verbs are
presets over it, so there is exactly one place where a default lives.

| verb | takes | draws |
|---|---|---|
| `chart d` | option dictionary | anything below, fully specified |
| `plot v` · `plot (v;W;H)` · `plot (a;b;c)` | a vector, or a list/dict of columns | one line, or one labelled line per column |
| `plots (a;b;c)` · `plots \`a\`b!(x;y)` | several vectors, or a dict | several series + legend |
| `xyplot (xs;ys)` | two vectors | y against a **real x axis** |
| `scatter (xs;ys)` | two vectors | points, unjoined |
| `step v` / `area v` | a vector | piecewise-constant / filled |
| `hist v` · `hist (v;nbins)` | a vector | a binned distribution |
| `barh \`a\`b!3 7` | dict or `(labels;values)` | labelled horizontal block bars |
| `heat \`a\`b!(r1;r2)` | dict or `(labels;rows)` | a matrix as truecolour cells |
| `spark v` | a vector | **returns** a one-line string, to embed in a row |
| `candle t` | an OHLC table | Unicode candlesticks, green up / red down |

| `tplot (ts;vs)` · `dplot` · `pplot` | two vectors | `xyplot` with the x axis labelled as a time / date / timestamp |

### Temporal axes

An axis whose values are a **time** is labelled as a time, not as the integer the column
holds. Amber's temporal types keep numeric storage inside a column (§1), so a *vector*
cannot carry its own unit; only an atom can. The unit is therefore either **stated** or
**inferred**:

| `xunit` / `yunit` | the values are | labels look like |
|---|---|---|
| `` `num `` | plain numbers | `1000`, `2000`, … |
| `` `time `` (`` `ms ``) | milliseconds of day, for Amber's `time` columns | `09:30:00`, and `09:30:00.250` once the step is sub-second |
| `` `sec `` | seconds of day | `09:30:00` |
| `` `date `` (`` `day ``) | days since `2000.01.01`, for Amber's `date` columns | `2024.03.02` |
| `` `stamp `` (`` `ns ``) | nanoseconds since `2000.01.01`, for `timestamp` columns | a clock inside one day, `MM.DDDHH:MM` under a month, else a date |
| `` `auto `` (default) | (none) | inferred, see below |

Ticks land on **round clock and calendar boundaries** (`09:30`, `10:00`, a Monday, the first of
the month) rather than on 1-2-5 counts of the underlying integer, and the number of ticks is
chosen from how wide that unit's labels are, so a 19-character timestamp label gets fewer,
well-spaced ticks instead of a row of labels that collide and get dropped. The plot area is
unaffected: the labels live in the gutter, so the same series at the same size draws the same
shape whatever the unit.

`` `auto `` claims a narrow range on purpose, because guessing wrong turns an ordinary count into a clock:

* whole numbers at or above `1e15` are **ns timestamps** (any instant more than eleven days past
  the epoch, and nothing anyone plots is a raw count that big);
* whole numbers inside one day and **at least an hour in** are **milliseconds of day**;
* everything else is a plain number.

So `xyplot (trades\`time; trades\`px)` reads as a clock for a normal session, while an index of
a few million samples stays a number. A session that starts near midnight, or a
`date` column (whose values overlap ordinary small counts), needs the unit stated:
`chart \`x\`y\`xunit!(t;v;\`time)`, or the `tplot` / `dplot` / `pplot` shorthands.

`candle` passes a `time` or `date` column straight through, so OHLC bars from `bars[10;t]` are
labelled with the clock times of the bars.

### Options

`chart` accepts these keys; anything omitted takes the default in `chopt`.

| key | default | meaning |
|---|---|---|
| `y` | (none) | a vector, or a list of vectors (one per series) |
| `x` | index | matching x coordinates; one vector, or one per series |
| `w` `h` | fit terminal | size in character cells |
| `ylim` `xlim` | auto | `(lo;hi)` to pin an axis |
| `grid` | `1` | `0` none, `1` horizontal gridlines, `2` horizontal and vertical |
| `axis` | `1` | frame, tick marks and tick labels; `0` gives the bare canvas |
| `legend` | `1` | draw `names` under the chart |
| `colour` | `2` | `0` off, `1` on, `2` on only when stdout is a terminal |
| `title` `xlabel` `ylabel` | `""` | text |
| `names` | `()` | one label per series |
| `col` | palette | one 256-colour code per series |
| `style` | `0` | per series: `0` line, `1` scatter, `2` step, `3` area |
| `xunit` `yunit` | `` `auto `` | how to label that axis; see **Temporal axes** above |

```q
plot t`px                                       / one column, framed and labelled
plots `bid`ask!(q`bid;q`ask)                    / two series, legend from the keys
chart `y`title`ylabel`ylim!(px;"AAPL";"price";98 102)
chart `y`x`names!((px;ma);(!#px;50+!#ma);("price";"MA(50)"))   / per-series x
candle bars[10; select from trades where sym=`AAPL]
```

### Two things that make the output readable

**The axis is snapped, not padded.** `axcalc` takes the data's own range and rounds it
*outward* to the nearest 1/2/5 boundary, so ticks land on numbers a reader decodes without
arithmetic. Raw min/max would pin the extremes to the frame, where they look clipped; a fixed
percentage pad would land the axis on values like `7.31942`.

**More points than pixels are drawn as an envelope.** Above roughly `4W` points, each pixel
column is drawn as the min→max range of the points that fall in it, rather than joining
consecutive points. One vertical segment per column keeps every spike and avoids the solid
smear a joined million-point series produces. The envelope needs a non-decreasing `x`, so
parametric curves that double back (a Lissajous figure, a spiral) fall back to segment drawing
automatically.

All of these print directly via `` `0:``. The C kernels are `plotC`/`candleC` in `i.c`; the
`plt` kernel also still accepts the bare `(v;W;H)` form, which returns an unframed canvas.
See [`examples/graphs.k`](../examples/graphs.k) for a 31-chart tour.

## 9d. Apache Arrow C Data Interface: `arrow.export` · `arrow.import`

Zero-dependency interop with PyArrow / Polars / DuckDB over the stable Arrow C ABI (no
`libarrow`). `arrow.export t` → `(schemaAddr; arrayAddr)` (64-bit C-ABI pointers); export is
**zero-copy**: each Arrow child `buffers[1]` aliases the Amber column payload and a `release`
callback drops the refcount when the consumer finishes. `arrow.import (schemaAddr; arrayAddr)`
→ Amber table (a copy: Amber's inline object header precludes aliasing a foreign buffer),
translating format strings and validity bitmaps (→ `0N`/`0n`/null). Numeric widths, ranges
and symbol (utf8) columns round-trip exactly. Structs live in `a.h`, logic in `ar.c`.

## 9e. On disk: splayed and partitioned tables (since 2.7.0)

Binary column files that map straight into memory, laid out the way q lays out a database:

```q
`:db/t/ set t                                   / splay t: db/t/<col> per column, db/t/.d, symbols in db/sym
t2:get `:db/t                                   / back, mapped: a 10M-row table loads in ~6 ms
`:f set 1 2 3                                   / one value, one file
Q.dpft[`:hdb;2026.01.02;`sym;`trade]            / one day of trade, sorted by sym with `p on it
\l hdb                                          / map the whole database (or loaddb "hdb")
select sum sz by sym from trade where date=2026.01.02
```

* **Columns.** A file is a 4 KB header then the data, page-aligned, so a numeric column is one
  `mmap` and its pages come in as a query reads them. Writes go to a temp file and get renamed, so
  a reader never sees half a file, and a map is private: changing the vector in Amber never
  touches the disk. A splayed table's files (and the sym file's new names) are all written before
  any is renamed, `.d` last, so a `set` that fails leaves the old table; until the renames the disk
  holds both tables. That holds after a `set`, not during one: a reader that loads the table while
  the files are being renamed can still see some old and some new. Two `set`s into one database at
  once share its `sym.tmp`, so run them one at a time. Symbols are stored as indexes into the database's `sym` file (it only grows).
  Anything nested goes in as `-8!` bytes.
* **Partitioned tables.** `\l db` (or `loaddb`) finds `db/<date or int>/<table>/` and defines each
  table as a small dict naming its partitions. Nothing is read until a query asks.
  `select`/`exec` keep only the partitions the where-clause allows on the partition column, then
  map only the columns the query names. Grouped by the partition column, a query runs per
  partition and the results get stacked, so `select sum sz by date from trade` over 10 days of
  1M rows is ~6 ms. `update`/`delete` on one is `'par`, as q. `ptab[`trade;()]` gives the whole
  thing as one table.
* **Older files.** `dset`/`dget`, `splay`/`dload` and `partsave`/`partload` still work, now on
  the binary format, and they still read the text files Amber wrote before 2.7. They read one by
  evaluating it as Amber source, so use them only on files you trust. `get` reads only the binary
  format: a file or a splayed table in any other is `'format`, and is never evaluated (as q's `get`).

Two things to know: the partition column is built only when the query uses it, and a select
spanning many partitions copies the columns it needs into one table (q maps them in place).

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
like["abcde";"a*e"]           / 1             (glob: * and ?, and [ ] classes)
like["ab";"a[^a-c]"]          / 0             ([abc] [a-z], ^ negates; as q)
like[("cat";"dog";"cab");"c*"]/ 101b
```

---

## 10b. Binary serialization: `-8!` · `-9!`

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
(`0w`, `-0w`), and **column attributes**, so a `` `s``-sorted column arrives still sorted and keeps
the O(log n) binary-search path in `?` (an int column stored 16 bits or wider).

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
are intercepted; every other left argument still means `mod`.

`peach` uses this as its worker wire format (§9a). See `examples/peach_verify.k` for the full
verification suite.

## 10a. Temporal

**Native types (1.7).** `date`, `time` and `timestamp` are first-class types with literal
syntax and type-aware arithmetic:

```q
2026.07.30                      / date  (days since 2000.01.01)
10:00:00.000 + 00:00:05.000     / time + time -> 10:00:05.000
-10:00:00.000                   / a negative time, as q; -t and neg t negate a time
2026.08.15 - 2026.07.30         / date - date -> 16 (days) ; date+n -> date
2026.07.30D09:30:00.000000000   / timestamp (ns since 2000.01.01)
year 2026.07.30                 / accessors: year month day dow  ·  thh tmm tss (time)
"D"$"2026.12.25"                / string casts: "D"$ (date) "T"$ (time) "P"$ (timestamp)
`i$2026.07.30                   / extract the raw numeric value
2026.01.01 2026.01.02           / A strand of dates (or times) is a list of them
```

Anything else is `'type` (date+2.3, date*2, date+date), and date+0N is `'domain`: there's no null
date yet, so it used to just hand the date back.

Two kinds compare as in q: a date and a time are `'type`, a date against a timestamp is its
midnight (`2026.07.30=2026.07.30D00:00:00.000000000` is 1), and a timestamp against a time is its
time of day, to the millisecond.

Columns keep numeric storage (as q does internally), so `xasc` and the `s#` attribute work
unchanged and a `time`-named column auto-renders as `HH:MM:SS.mmm` in a grid.

Underneath, a **time of day** is an integer number of **milliseconds since midnight**, the
same convention as q’s `time`. The q dotted temporal accessors map to plain Amber calls:

| q            | Amber        | meaning                              |
|--------------|--------------|--------------------------------------|
| `t.hh`       | `hh t`       | hour of day (0–23; as q's `` `hh$ ``, a negative time keeps its sign: -1 for `-01:00:00.000`) |
| `t.mm`       | `mm t`       | minute of hour (0–59; -59–0 for a negative time, as q's `` `mm$ ``: -30 for `-01:30:00.000`) |
| `t.ss`       | `sec t`      | second of minute (`ss` = string-search; signed as `mm`) |
| `t.minute`   | `minute t`   | minutes since midnight               |
| `t.second`   | `second t`   | seconds since midnight               |
| (millis)     | `milli t`    | millisecond (0–999)                  |
| build        | `hms[h;m;s]` | construct a time                     |
| parse        | `ptime "HH:MM:SS.mmm"` | string → ms                 |
| format       | `stime t`    | ms → `"HH:MM:SS.mmm"`, every digit of the hours past 99 (`"100:00:00.000"`) |

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
adverbs    each over scan prior
order/set  rank iasc idesc asc desc xrank xprev rotate in except inter union raze
           sublist cross
tables     istable isdict iskeyed cols keys rows atr xkey unkey xcolall xcols xasc
           xdesc meta insert
grouping   qwhere qselect qby xgroup ungroup fby
           gsum gavg gmin gmax gcount gfirst glast   (2.1: one-pass `gagg kernel)
joins      lj ij uj pj ej aj aj0 asof wj
strings    lower upper ltrim rtrim trim ss ssr sv vs like lk1
temporal   hms hh mm sec milli minute second stime ptime  bar minbar tsym
           year month day dow thh tmm tss  dstr pdate pstr ptstamp  (native types)
display    show amfmt amtab amkeyed amdict
charts     chart plot plots xyplot scatter step area hist barh heat spark candle
           COLOR CT cwrap vlen vstrip (ANSI highlighting)  CROWS (preview height)
attributes `sa `ua `pa `ga (set sorted/unique/parted/grouped)   `at (get)  [kernel primitives]
           ascending value sorts (asc, x@<x, `srt, xasc) return `s-flagged results (2.1)
kernels    `memb (membership) `gagg (group aggregate) `srt (value sort) `mw (windows)
           `xs (multi-column grade) `aj `wjb `ajs (as-of join)  [2.1: see section 5a]
           `ejx (the rows of an equi-join; amber.k's ej uses it)
           `hh `mm `sec (hour, minute, second of a time or of ms; amber.k's hh mm sec use them)
           `tjn (a list of dates, times or timestamps alone as its numbers; temporal.k's tdays uses it)
moving     mcount msum mavg mprd mvar mdev mmin mmax   (std.k, O(n) prefix)
math       dot mmu (matrix multiply)                   (std.k)
parse/ser  parse eval reval ser deser protect          (std.k; text serialise)
cast       long int float char sym bool cast           (std.k)
parallel   peach (multi-core, thread pool)              (std.k) ; ts (\ts timing)
.z/.Q/.j/.h  z.p z.d z.t … · Q.f Q.dd Q.trp Q.s … · j.j j.k (JSON) · h.ht (HTML)  (sys.k)
on-disk    set get dset dget splay dload loaddb ptab Q.en Q.dpft partsave partload parts   (hdb.k, binary, mapped)
ipc/tick   hopen hclose hsend hrecv hsync · u.def u.sub u.pub u.get u.end  (ipc.k)
arrow      arrow.export arrow.import                    (Arrow C Data Interface)
```

### 11a. Extended modules (`std.k` `sys.k` `hdb.k` `ipc.k`)

Loaded automatically by `repl.k` after `amber.k`/`fin.k`. They add, in lightweight text‑based
form, a large slice of q's system vocabulary:

* **`std.k`**: vectorised **moving aggregates** (`mcount msum mavg mprd mvar mdev mmin mmax`,
  O(n) prefix sums), a little linear algebra (`dot`, `mmu`), **`parse`/`eval`/`reval`** and a
  text **`ser`/`deser`** round‑trip (portable Amber text, *not* the q binary `-8!`/`-9!`),
  `protect` (like `.Q.trp`), typed cast helpers, `peach`, and `ts` (time an expression).
* **`sys.k`**: the `.z` clocks/handlers (`z.p z.P z.n z.d z.D z.t z.T z.z`; `z.pg z.ps z.po
  z.pc z.ts z.exit` are stubs), `.Q` utilities (`Q.f Q.fmt Q.s Q.ty Q.qt Q.id Q.dd Q.gc Q.w
  Q.fc Q.trp`), `.j` JSON (`j.j`/`j.k`), a minimal `.h` HTML renderer, and `plot`/`candle`.
* **`hdb.k`**: on‑disk data, binary and memory‑mapped since 2.7 (see §9e): `set`/`get`,
  `dset`/`dget` (value ↔ file), `splay`/`dload` (splayed table ↔ directory, one file per column with
  a `.d`), `Q.en`/`Q.dpft`/`loaddb`/`ptab` (partitioned databases), and the older
  `partsave`/`partload`/`parts`. Text files from before 2.7 still read through `dget`/`dload`, not `get`.
* **`ipc.k`**: raw‑socket messaging (`hopen hclose hsend hrecv hsync`, a text protocol rather than the
  q binary wire) and an in‑process tickerplant (`u.def u.sub u.pub u.get u.end`).

## Help inside the REPL

Type `\` for the menu, then a topic: `\q` (scalars, aggregation, sets, strings),
`\j` (tables, keyed tables, joins, qSQL), `\z` (temporal, bars, attributes, display),
`\m` (finance/HFT module). `\0 \+ \' \`` `\h` cover the core array language and its cheat-sheet.

**Session commands** (one-liners in the `\` menu itself, no separate topic page):
`\l file.k` load a script, `\d ns` switch/show namespace, `\t:n expr` time `n` runs, `\f` list
functions, `\cd path` change directory, `\grid MODE` set the table border
(`clean`/`rounded`/`sharp`/`heavy`), `\clear` clear the screen, `\a` print the licence, `\\` exit.

**Diagnostics:** `\v` a rich workspace inspector (every global as a Name/Type/Shape/Memory
table; see `src/inspect.{h,c}`); `\ast expr` a colour-coded parse tree, parse-only, nothing is
executed (`src/ast.{h,c}`, where every leaf is typed explicitly, `Int64`/`Float64`/`Symbol`/`Char`/a
`(TypeName Vector[len])` preview, never a generic placeholder; tacit hooks `(f g)`, forks
`(f g h)`, and curried projections `1+`/`f[x;;z]` get their own explicit labels; see
[CHANGELOG](../CHANGELOG.md)); `\trace expr` a 4-phase timing report (parse/arena/exec/format) plus
the arena's peak scratch usage for that evaluation, running the same qSQL rewrite the prompt uses
so tracing a table or `select …` expression renders correctly (`src/trace.{h,c}`); `\disasm expr`
compiles an expression and prints the real bytecode Amber's compiler/VM (`src/b.c`) produces for
it, including locals, constant pool and instruction stream, without executing it (`src/vm.{h,c}`).

**Engine extensions** (all additive, standalone modules; see the README's
[Engine extensions](INTERNALS.md#engine-extensions) section for full detail and benchmarks):
SIMD vector kernels (`src/simd.{h,c}`, AVX2/NEON/scalar, self-test `` `simd 0``), a
thread pool plus the parallel kernels behind sort, find, distinct, group sums and fusion
(`src/parallel.{h,c}`, self-test `` `para 0``), and a native CSV parser that reads a file straight into a typed table
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
ids: asc 1000*distinct trade`time / `s-attributed integer vector, past a byte wide
`at ids                           / `s   -> ? on ids runs in O(log n)
ids?3000 5000                     / binary-searched find
```

---

*Amber: a low-latency array language. GNU AGPLv3.*
