# What's still missing

Amber covers a large slice of q's *vocabulary* (aggregations, dicts, tables, keyed tables,
the join family, qSQL-style select/by, strings, tick bars, native temporal types, all four
attributes, moving aggregates, a text-based on-disk / IPC layer, and the `.z`/`.Q`/`.j`/`.h`
namespaces). This is an honest map of what a full q implementation has that Amber does **not** yet, roughly in
order of how much it would change day-to-day use. "partial" means some of it exists.

## 0. New in 1.9 (not q gaps; Amber-specific)
Three items here are *beyond* stock q rather than catching up to it, so they don't map to a gap
above; they're recorded for completeness:
- **Native `aj` kernel.** The as-of join's match step is now a C `lower_bound` (branch-free /
  `cmov`) over each group's sorted ns-timestamp slice, replacing the per-row K `bin`. In-memory
  `aj`/`aj0` are covered; **on-disk `aj` over partitions** is still missing (see §4).
- **HFT zero-allocation arena.** A thread-local 16 MB bump allocator for the transient buffers
  produced during evaluation, rewound per eval cycle to keep `malloc`/`free` jitter off the hot
  path. (q has no user-facing equivalent; it is managed internally there.)
- **Rust-style diagnostics.** Opt-in (`AMBER_DIAG=1`) `error[CODE]` reports with a `-->` locator,
  a gutter-aligned source line, `^^^` underlines and a `= help:` note. This is *richer* than q's
  single-line `'error` and than Amber's own default caret line, which both remain.

## 1. Temporal types: done (1.7)
Native `date` / `time` / `timestamp` types with literal syntax (`2026.07.30`,
`10:00:05.000`, `2026.07.30D09:30:00.000000000`), auto-display, type-aware arithmetic
(`time+time`, `date-date`→days, `date+n`, comparisons), string casts `"D"$`/`"T"$`/`"P"$`,
and accessors `year`/`month`/`day`/`dow`/`thh`/`tmm`/`tss`. Columns keep numeric storage
so `xasc`/`s#` work unchanged.
- **Since 2.5:** A strand like `2026.01.01 2026.01.02` is a list; arithmetic outside the cases
  above is `'type`. There's no null date, so `date+0N` is `'domain` for now.
- **Still missing:** `month`/`minute`/`second`/`timespan`/`datetime` as distinct types,
  `m` month-literals, and the dotted `t.hh` accessor form (Amber uses `thh t`).
- **Differs from q:** a time of 100 hours or more prints every digit of its hours
  (`100:00:00.000`, up to `596:31:23.647`), so the text reads back as the same time; q prints
  `**:00:00.000`.

## 2. Missing atom types
`short` (`h`), `real`/float32 (`e`), `byte` (`x`, `0x…`), `guid` (`g`, `0Ng`), plus the full
set of typed nulls/infinities (`0Nh 0Ne 0Wp 0Nd …`). Amber has int (64-bit), float, char and symbol
only, with `0N`/`0n` nulls; a `b` literal such as `101b` is an int vector.
- **A `b` literal next to a strand.** Before an int strand it is indexed by it, as in q (`01b 1`
  is `01b[1]`, `1`; q gives `1b`; `01b 1 2` is `1 0N`). Before a float strand it joins it, and
  after any strand it is joined to it: `01b 1 2.5` is `0.0 1.0 1.0 2.5` (q: `'type`) and `1 01b` is `1 0 1` (q: `1`).
  Both joins are ngn/k's reading, which Amber's core follows, and are kept so that literals that
  work today keep their meaning (issue #83, Q7).

## 3. qSQL (the template syntax): mostly done
The `select … by … from … where …` template now works **bare** (no `sel"…"` wrapper), along
with `exec`, `update`, and `delete`; see AMBER.md §7. Since **2.0.0** this bare form also works
inside a **`.k` script** loaded once the stdlib is up (the loader runs each file through the same
`qrw` rewriter the REPL uses), so `sel"…"` is no longer needed in files either.
- **Since 2.2:** **sorted and limited selects** are done: `select[5]`, `select[-5]`,
  `select[>px]`, `select[<sym]`, `select[5;>px]` and multi-key `select[<sym;>px]`, on plain and
  keyed results alike, with q's clause order (where → by/select → sort → limit). See AMBER.md §7.
- **Since 2.2:** `fby` **inside a where-clause** works, as in `select from t where
  px=(max;px) fby sym`, and its group spec may be one column, a *list* of columns
  or a table. (The where-clause form was always implemented; this list used to say
  otherwise. The multi-column list form returned one value per column instead of
  one per row and is fixed.)
- **Since 2.5:** Where-clauses cascade (`where sym=`a, n=max n` is the biggest n among the a's),
  columns get q's names (`v v1`, `x`), and `select n, s:sum n` stretches `s`, as `update` does.
- **Differs from q:** a select whose items all give atoms is one row, in the bare and the functional
  form, also when no item reads a column or `i` (`select c:0.5 from t`, `select c:abs 0.5 from t`,
  `` ?[t;();0b;(,`c)!,0.5] ``), where q gives `'rank`. q gives one row only when an item applies
  one of its built-in aggregates. A check for the constant case cost every select of aggregates on a
  small table more than we could make free, so it is not made (#20 row 17).
- **Differs from q:** `med` of an empty general column is `0n` (floats), q's answer for an empty
  temporal column: an Amber temporal column is a general list too, so the two cannot be told apart.
  With `` t:([]sym:`a`b;n:1 2;g:(1;"a")) ``, `select med g by sym from t where n>5` gives `g` as `0#0n`
  (q: `()`), and `med ()` is `0n` (q: `` `float$() ``) (#94 Q9).
- Still missing: the general functional forms `?[t;where;by;select]` /
  `![t;where;by;cols]`, and correlated subqueries. Note that `?` at arity 3+ is
  already `ins` in this dialect, so the `?[…]` spelling cannot be added without
  breaking k semantics; `![…]` at arity 4 is free.
- **Amber has:** bare + string `select/exec/update/delete`, plus the functional helpers
  `qwhere qselect qby fby xgroup ungroup`.

## 3a. Nulls: the q-named functions follow q, the k primitives stay k

`sum`, `avg`, `min`, `max`, `mins` and `maxs` (amber.k, and qSQL's grouped aggregates) skip
nulls as q's do: `sum 0N 1` is `1`, `avg 0N 1` is `1.0`, `min 0N 5` is `5`, `mins 0n 1.0` is
`0w 1.0`, and an all-null list gives the identity (`min 0N 0N` is the largest int, q's `0W`).
The primitives keep k's treatment, where the int null is just the smallest int:

| Expression | Amber | q |
|---|---|---|
| `0N+1`, `0N-1`, `0N*2` | wrap: `-9223372036854775807`, `9223372036854775807`, `0` | `0N` |
| `+/0N 1` | `-9223372036854775807` | (no `+/` in q; `sum` gives `1`) |
| `&/0N 5` | `0N` (the null is the smallest) | `min` gives `5` |
| `0.5^1 0N` | `(1;0.5)`: a fill of another type makes a generic list | `1 0.5` |

Fill of floats is the exception (since 2.7.1): an int atom filling floats gives floats, as in q.
`0^1.5 0n` is `1.5 0.0` (2.7.0 and ngn/k: `(1.5;0)`), `0^0n` is `0.0`, `0N^1.5 0n` leaves `0n`,
a float item of a generic list is filled the same way (`0^(1;0n)` is `(1;0.0)`), and so are
the float values of a dict or table. A char or symbol filling floats still makes a generic list
(`` `a^1.5 0n `` is ``(1.5;`a)``). A null int atom becomes the fill (`0.5^0N` is `0.5`, as in k
and q), and so does a null float atom filled by a symbol (`` `a^0n `` is `` `a ``, as in k; q: `'type`).

Since 2.5 `sums prds prd wsum wavg svar sdev` skip nulls too, and `cov scov cor` drop a pair
with a null. Still k: on a dict, `sum` and `min` count the null (`sum `a`b!0N 1` is
`-9223372036854775807`; q gives `1`), and `avgs` and `med` treat nulls as the primitives do. So do
`sums` and `sum` of a general list, which add an int null in as `+` does (#116): `sums (1;0N;2.5)` is
`(1;-9223372036854775807;-9.223372036854776e18)` and `sum (1;0N;2.5)` is `-9.223372036854776e18`
(q: `(1;0N;0n)` and `0n`).
`wj` and `wj1` skip nulls in a float `sum` or `avg` and in `min`, but an int `sum` or `avg` adds
`0N` in, as `+` does: a window of ints `0N 2` sums to `-9223372036854775806` (q: `2`). Looking for
a null there would cost windows that have none (#94 Q4).

## 3b. Signed zero and NaN: one value each, as in q

Since 2.2 `-0.0` and `0.0` are **one value** for `=`, `~`, `in`, `=` (group), `?`
(distinct) and find, exactly as in q.

They also order as one value (issue #15): `-0.0<0.0` is `0`, grades are stable for them, so
`<(0.0;-0.0)` is `0 1` as in q, and every NaN is one value, sorting first. The keys-only
sort (`asc`, `x@<x`) never changes an item, and may list equal zeros or NaNs in bit order, which is
still ascending by value. `&` and `|` of two equal zeros give `0.0`, and so do the folds, scans
and grouped min/max over them; the moving `mmax`/`mmin` still keep `-0.0`. (q's `min`/`max`
keep the first zero; ngn/k orders `-0.0` below `0.0`.)

## 4. On-disk data (HDB): partial (`hdb.k`)
**Since 2.7:** Binary column files that map straight into memory, in q's layout: `` `:db/t/ set t ``
and `get`, a `sym` file per database, `Q.en`, `Q.dpft`, and `\l db` (or `loaddb`) for date- or
int-partitioned databases. `select`/`exec` prune partitions on the partition column, map only the
columns they use, and run per partition when grouped by it. The older `dset`/`splay`/`partsave`
names write the binary format too and still read the old text files (AMBER.md §9e).
- **Still missing:** `.Q.chk`, `.Q.ind`, `.Q.fs`/`.Q.fsn` (chunked file streaming), segmented
  databases (`par.txt` across disks), compression, on-disk `aj` over partitions, and map-reduce for
  aggregates that span partitions without grouping by the partition column (those copy the
  columns they need into one table first).
- **Differs from q:** `count` of a partitioned table (a name `loaddb` defines) is the count of the
  dict that names it (6, its keys), where q gives its rows; `sel"select count i from t"` gives them.
  Telling that dict apart in `count` would cost every call.
- **Still missing:** appending to a table on disk. There is no `upsert`, so no
  `` `:db/t/ upsert t `` (q appends the rows to the column files), and amber.k's `insert` takes the
  table itself, not a name or a path as q's `` `t insert r `` does (#94 Q13).

## 5. IPC & the tick architecture: partial (`ipc.k`)
Amber now ships `hopen`/`hclose`/`hsend`/`hrecv`/`hsync` (raw-socket messaging) and an
**in-process tickerplant**: `u.def` (define a stream), `u.sub`/`u.pub` (subscribe / publish),
`u.get`/`u.end`. `.z.pg`/`.z.ps` handlers exist as evaluate-stubs in `sys.k`.
- **Still missing:** q's **binary wire protocol** (Amber's sockets exchange plain text
  expressions, not IPC-encoded messages), real over-the-network `.z.pg`/`.z.ps`/`.z.po`/`.z.pc`
  handler dispatch, `.z.w`, websockets, TLS, and the full multi-process tickerplant / RDB / HDB /
  gateway pattern (`tick.q`, `r.q`, `u.q`, `w.q`).

## 6. Attributes: 4 of 4 (setters); find accel on 1 (sorted)
All four attributes are set in C: **sorted (`` `sa``)**, **unique (`` `ua``)**,
**parted (`` `pa``)**, **grouped (`` `ga``)**, read back with `` `at``. **Sorted** int vectors
stored 16 bits or wider take the O(log n) binary-search find path; sorted floats and
symbols, byte-wide ints and parted vectors (which need not be in order) take the ordinary
find, as an unflagged vector does;
grouped pairs with `fin.k`'s group index
(`bysym`/`symrows`) for O(1) per-symbol slicing.
- **Since 2.1.0:** every ascending value sort (`asc`, `x@<x`, `` `srt``, `xasc` on a flat
  numeric column) returns its result flagged `` `s``, so a later `?` on it, when it is an int
  vector stored 16 bits or wider, takes the O(log n) path without an explicit `` `sa``.
- **Still missing:** dedicated find/`where=` acceleration driven by the `` `u`` / `` `g``
  attribute *itself* (grouped speed currently comes from the separate group index, not the
  attribute), and general **attribute preservation through ops**. Apart from sorts, the flag
  is dropped whenever an op builds a new vector, whereas q keeps/drops attributes by defined
  per-op rules.
- **A general list of mixed types sorts in Amber's order of types, not q's.** `<`, `asc`,
  `iasc` and the `` `s `` check order it by type first, then by value, with the types in ngn/k's
  order: chars, floats, ints, symbols. q puts longs first, then floats, chars and symbols. So
  `` asc (1;"a";`b;2.5) `` is `` ("a";2.5;1;`b) `` (q: `` `s#(1;2.5;"a";`b) ``), and
  `` `s#(2.5;1) `` succeeds where `` `s#(1;2.5) `` is `'s-fail`, the other way round from q.
  Kept because the `` `s `` check has to agree with the grade that a sorted find relies on
  (issue #83, Q14).
- **Differs from q:** a dict marked `` `s `` keeps its mark through an indexed assignment by name
  (`d[k]:v`, `d[k]+:v`), as in q, but other amends of one held by nothing else drop it and add a new
  key, where q keeps the mark, or says `'step` for a new key (#115; AMBER.md §9 lists them all):
  `` attr @[`s#1 3 5!`a`b`c;3;:;`z] `` is `` ` `` (q: `` `s ``), and `` @[`s#1 3 5!`a`b`c;4;:;`z] ``
  is `` 1 3 5 4!`a`b`c`z `` (q: `'step`); likewise a dict below the first level of a variable
  (`` v:(1;`s#1 3!`a`b); v[1;2]:`z ``; q: `'step`), `d,:e`, and `` .[`d;k;:;v] `` at a key `d` has.
  Keeping the mark in these would put a test on every amend's path.

## 7. Enumerations, foreign keys, linked columns
`` `sym$`` enumeration domains, `.Q.en`, foreign keys (`` `t$`` and dotted `order.customer.name`
traversal), linked columns, `.Q.fk`. None in Amber.

## 8. System namespaces: partial (`sys.k`)
Amber now provides the common members (as `.`-style names `z.*`/`Q.*`/`j.*`/`h.*`):
- **`.z.*`** clocks **done**: `z.p z.P z.n z.d z.D z.t z.T z.z z.w`. Handlers `z.pg z.ps z.po
  z.pc z.ts z.exit` exist but are **evaluate/no-op stubs** (no real timer `\t` or port dispatch).
  Missing: `.z.ph` (HTTP).
- **`.Q.*`** **done**: `Q.f Q.fmt` (number format), `Q.s` (show), `Q.ty Q.qt Q.id Q.dd`,
  `Q.gc Q.w` (mem placeholders), `Q.fc` (sequential fallback), `Q.trp` (protected).
  Missing: `.Q.dpft .Q.en` (partition/enumerate), `.Q.hg/.Q.hp` (HTTP get/post),
  `.Q.j10/.Q.x10` (base64), `.Q.pv/.Q.pf` (partition vars).
- **`.j.*`** JSON **done**: `j.j` (encode) / `j.k` (decode), both the core `` `j``.
- **`.h.*`** markup **partial**: a minimal HTML table/row renderer (`h.ht h.hrow h.hc`).
  Missing: CSV/XML/XLS rendering and an HTTP server.

## 9. Moving / window aggregates: mostly done (`std.k`, `fin.k`)
The moving family is implemented: `mcount msum mavg mprd mvar mdev mmin mmax` (`std.k`, O(n)
prefix sums; `mmin`/`mmax` are O(n·w) window scans) plus **`ema`** (C kernel, O(n) sweep).
- **Amber also has:** `sums prds mins maxs deltas ratios differ prev next wsum wavg xprev`.
- **Differs from q:** `deltas` of chars and of a general list that starts with an atom gives k's `-':`
  answer, its first item kept (#112 Q3): `deltas "abd"` is `97 1 2` (q: `'type`), and
  `deltas (1;2;2.5)` is `(1;1;0.5)`, keeping its `1` as `deltas 1 2 4` does (q: `(0N;1;0.5)`, though
  `1 1 2` for `deltas 1 2 4`). ngn/k's `-':` gives the same answers.
- **Still missing:** `wj2`, `ajf`/`ajf0` (fill as-of), `ij`/`lj` fill variants, vectorised
  `ssr`, and `rank`/`xrank` *over tables*. (`mmin`/`mmax` could also move to an O(n)
  monotonic-deque form; see BENCHMARKS.md.)

## 10. Linear algebra & math: partial (`std.k`)
`mmu` (matrix multiply) and `dot` (vector dot product) are implemented. Amber also has
`cor cov var dev svar sdev med` and scalar math.
- **Still missing:** `inv` (inverse), `lsq` (least squares), `.q` solve; distributional
  `rand`/`binr`.

## 11. Casting / parsing / serialization: partial (`std.k`)
`parse`/`eval`/`reval` are implemented, along with a **text** `ser`/`deser` round-trip (portable
Amber text via `` `k``, inverted by `eval`) and `protect` (like `.Q.trp`). Amber also has
`sv vs ss ssr like`, string casts, and `` `k`` (k-repr).
- **Done in 1.9.3:** the **binary** serialiser `-8!`/`-9!` (`src/ser.c`). `-8!x` encodes any K
  value to a contiguous byte vector and `-9!y` decodes it back byte-exact, preserving attributes,
  nulls, infinities, nested empties and symbol *names* (not process-local ids). `peach` now uses
  it as its worker wire format instead of `` `k `` text. Lambdas and projections are
  out of scope and raise `'type`. Verified by `examples/peach_verify.k` (60 cases).
- **Still missing:** `-18!` (compress), `-11!`
  (replay log), the full `$` cast matrix (guid, byte), typed file reader `("SIF";",")0:file`,
  `vs`/`sv` for base-N and temporal, `md5`, `.Q.btoa` (base64), and the CSV writer `","0:t`, a
  table's lines of delimited text (q: `` ","0:([]a:1 2;s:`x`y) `` is `("a,s";"1,x";"2,y")`, a cell
  holding the delimiter in quotes, nulls empty, floats at `\P`'s precision, dates as `2026-01-01`;
  Amber gives `'type`). `` `csvr `` reads CSV, and `x 0: y` writes lines of text.
- **Differs from q:** `$` of a null or a float gives text that reads back as the same value, which
  the `` `k `` round trip relies on: `$0N` is `"0N"` and `$0n` is `"0n"` (q's `string` gives `""`),
  and `$1%3` is `"0.3333333333333333"` and `$1e20` is `"1e20"` (q writes the display precision,
  `\P`: `"0.3333333"`, `"1e+20"`).
- **Differs from q:** `` `i$ `` of a float truncates, as in ngn/k and as repl.k's reference card
  shows: `` `i$1.7 `` is `1` and `` `i$-1.5 `` is `-1`, where q rounds, halves away from zero (`2`, `-2`).

## 12. Concurrency & performance ops: partial
`peach` is real **multi-core** (a pool of `AMBER_THREADS` threads), and `ts` (`\ts`) times an
expression. **Since 2.6:** Big vectors use the same threads inside primitives (sort, find, `in`,
distinct, gather, group aggregates, float sums, fused expressions), which is roughly what q's `-s`
secondary threads buy you. `AMBER_THREADS` is the knob.
- **Still missing:** a *parallel* `.Q.fc` (Amber's is a sequential fallback), map-reduce over
  on-disk partitions, and compression.

## 13. Console / environment niceties: partial
`\ts` (via `ts`) and number formatting `.Q.f`/`.Q.fmt` are done.
- **Still missing:** `\c` console dims, a real `\w` (workspace) report (`Q.w` is a placeholder),
  `system"…"`, `getenv`/`setenv`, and editor tooling / a language server.
- **Differs from q:** `\cd` with no argument names the working directory as the operating system
  does, where q prints `$PWD` when that names it (#116): started in `/tmp` (on macOS a link to
  `/private/tmp`), `\cd` is `"/private/tmp"` (q: `"/tmp"`; with `PWD` unset, `"/private/tmp"` too).

## 14. Known engine bugs: both fixed in 2.0.0
- ~~**Bare `/` comment line silently truncates the rest of the file.**~~ **Fixed in 2.0.0.**
  A `.k` line containing *only* `/` opens a **block comment** that runs to the next line starting
  with `\` (standard K); with no closing `\` before EOF it used to run to end of file and exit 0
  with no diagnostic, silently dropping the rest of the file. `src/p.c`'s `pe` now raises a clean
  parse error (`P(!e,ep0())`) when that block comment is unterminated, converting silent data loss
  into a loud error. Properly-closed `/ … \` blocks and trailing `/ …` line comments are unchanged.
  Regression: `tests/test_comments.sh` (4 cases, wired into `run_tests.sh`).
- ~~**`` `&`` (where) on a literal empty generic list returns a spurious non-empty result.**~~
  **Fixed in 2.0.0.** `&()` returned `,!0` (a 1-element list) instead of `!0` (an empty vector).
  `src/v.c`'s `X1(whr,…)` `RA` (generic-list) branch now guards the empty case
  (`P(!xn,x(an(0,tI)))`) and returns an empty int vector byte-identical to `&!0`, before the
  nested-grouping K expression runs. Regression: `test.k` (`whrEmptyGen*`, 5 cases). The `ss`
  workaround in `amber.k` (`(#s)<#p` special-case) is now redundant but harmless; it can be
  reverted independently.

---

Already done (once gaps): **bare qSQL** `select/exec/update/delete` (1.5), **as-of join**,
vectorised in 1.5, **native C kernel** in 1.9, **multi-core `peach`** (1.6, fork-based),
**Q-style grid preview** (1.6), **Unicode `\grid` modes + diagnostics + arena** (1.9),
**native temporal types** (1.7), **C-kernel `wj`/`ema`** (1.7), **terminal charting**
`plot`/`candle` (1.7), **Apache Arrow C Data Interface** (1.7), **all four attributes** in C
(§6), the **moving-aggregate family** `m*` (§9), **`mmu`/`dot`** (§10), **`parse`/`eval`/`ser`**
(§11), a **text-based on-disk layer** `dset`/`splay`/`partsave` (§4), and a **text IPC / in-process
tickerplant** `hopen`/`u.*` (§5).

### Nice next steps (highest value first)
1. ~~**Binary serialiser (`` -8!``/`` -9!``)**~~ is **done in 1.9.3** (`src/ser.c`). `peach` and
   the on-disk / IPC layers all currently move values as **text** (`` `k ``) and re-parse them.
   A compact binary encode/decode would cut that transfer cost, widen the range of workloads where
   `peach` beats serial `'`, and unlock a real (binary-wire) IPC and a binary on-disk format.
2. **Grouped-attribute-driven `where sym=`.** The `` `g`` setter exists, but fast `where sym=`
   currently comes from `fin.k`'s separate group index rather than from the attribute itself.
   Wiring the attribute into the C find path (as sorted already is) would make it automatic.
3. **Missing atom types** (§2): `short`/`real`/`byte`/`guid` and their typed nulls/infinities.
4. **Attribute preservation through ops** (§6): keep/drop attributes by q's per-op rules instead
   of always dropping on a new allocation.
5. **True partitioned/mmap HDB** (§4): a date-partitioned, memory-mapped on-disk format beyond
   the current text splay, plus `.Q.dpft`/`.Q.en`.
6. **Live REPL syntax highlighting**, meaning colouring tokens *as you type*, not just on a line you've
   already run. This needs `repl.k`'s raw-keystroke input loop rewritten to re-tokenize and
   redraw the current line on every keypress (a fragile, previously-regression-prone path in
   this project; see CHANGELOG), not a new `\command`. A prior attempt shipped as a `\hl <expr>`
   one-shot echo command instead and was removed for not matching what "live" means.


---

## Known leniencies (accepted, not bugs), recorded 1.9

Surfaced by the qSQL matrix (`tests/test_qsql.k`) and pinned there with `tk[...]` so a change in
behaviour shows up as a test failure rather than a silent regression.

- ~~**Unknown `by` key does not raise.**~~ **Fixed in 1.9.7.** `select t:sum px by nosuchkey from t`
  used to group by nulls instead of rejecting the query the way q does. The old `qbc` turned
  each by-item into a symbol of its own source text, so grouping ran on a column no table has and
  `` b#+t`` yielded nulls. `qbyx` now compiles by-items with the same `qfn` machinery the
  select-list uses, so an unknown name raises as an undefined variable and, the reason the fix
  matters, `by time:1m xbar time` groups on the computed bucket instead of on one null key.
- **A trapped error still renders a diagnostic to stderr *by default*.** `.[f;args;handler]`
  (and `protect`, documented as `.Q.trp`-like) catches the error correctly, but the Rust-style
  report is written at error-creation time, before the handler runs. Rather than defer rendering
  (which would change what an interactive line prints), 1.9 adds the `` `diag`` runtime switch:
  `` `diag 0`` suppresses the report and returns the previous setting, `` `diag 1`` restores it.
  `tests/harness.k` uses it. Code that catches errors in bulk should do the same.
- **Folds and scans of chars under `% < > =` read the chars as numbers from the start, unlike
  ngn/k.** `%/,"a"` and `</,"a"` are 97, `<\"ab"` is `97 1` and `"a"</!0` is 97, where ngn/k
  keeps the char in the items the verb never sees (the one item of a fold of one, a scan's first
  item, a char seed over no items): `"a"`, `("a";1)`, `"a"`. Kept so that `%/` and `+/` agree
  (`+/,"a"` is 97 in both); see #17 and #64. Unseeded folds and scans of no chars differ too:
  `</""` is `0N` and `<\""` is `!0`, where ngn/k gives `" "` and `""`.
- **`+/` of floats, and an over with a float seed, sum the items four ways and add the seed last
  (#112 Q2, #113).** The items (ints made floats, with a float seed) are summed in four running sums
  joined at the end, and the seed is added last. The scan and `{x+y}/` fold left from the seed, adding
  one item at a time and rounding at each, as q's `+/`, `sum` and `sums` and ngn/k's `+/` do, so the
  over can differ from the scan's last item: `(0.1+/1 -1;*|0.1+\1 -1)` is `0.1 0.10000000000000009`;
  with `x:4503599627370497 -4503599627370496` (2^52+1 and -2^52), `(0.1+/x;*|0.1+\x)` is `1.1 1.0`;
  and with `y:1e16 1 1 1 1 1 1 1.0`, `(+/y;*|+\y)` is `1.0000000000000006e16 1e16`. q and ngn/k give
  the scan's answer for both forms of each (`0.10000000000000009`, `1.0`, `1e16`). Folding left would
  give up the fast sum.
- **No long-typed infinity literal.** `0w`/`-0w` exist for floats; `0W`/`-0W` do not parse, so
  the identity elements of `&/`/`|/` over an empty long vector can only be obtained from the
  primitives themselves.
- **`5#0#0` promotes byte/narrow-int nulls to long nulls.** `cn[tG]` aliases the long null, so a
  take from an empty narrow vector widens the element type.
- **Attribute syntax is `` `sa``/`` `ua``/`` `pa``/`` `ga`` (set) and `` `at`` (get), not q's
  `` `s#``/`` `u#``/`` `p#``/`` `g#``.** Several doc passages still write `s#` informally when
  describing the sorted attribute; the working syntax is `` `at(`sa 1 2 3)``.
- **`f [a;b]` with a space is not a call.** K reads `[a;b]` as a bracketed statement block, so
  the expression silently evaluates to a discarded projection `f[b;]`, with no error and no output.
  Bit this repo's own qSQL suite (42 of 93 cases stopped running while the suite still reported
  "ALL TESTS PASSED"); `tests/harness.k`'s `hexpect[n]` now guards against it.
- **A newline inside parentheses separates items, as in ngn/k, so `;` at the end of a line leaves
  an empty item.** An empty item is `::` (issue #83, Q6), so in a script `x:(1;` / ` 2;` / ` 3)`
  on three lines is `(1;::;2;::;3)`, where q reads `(1;2;3)` (and 2.7.2 gave `'parse`); a `;` at
  the start of the next line does the same. Over several lines, write the items without `;`
  (`(1` / ` 2` / ` 3)` is `1 2 3`). A table, `([]a:1 2;` / ` b:3 4)`, already reads as in q.
- **A bare `/` on a line of its own opens a block comment** that runs to the next line starting
  with `\` (standard K). Since **2.0.0** an *unterminated* one (no closing `\` before EOF) raises a
  clean parse error instead of silently truncating the file; a properly-closed `/ … \` block is
  unchanged. `tests/harness.k` still carries a warning comment about the sharp edge.
- **`insert` fills a column a row or a table lacks with item `0N` of `t`'s column**, as `aj`
  fills an unmatched row: ints, floats, symbols, chars and general columns of atoms get q's nulls,
  but a general column whose first item is a list gets that list's nulls (q: an empty list), a
  column of strings gets blanks as long as its first string (q: `""`), a boolean column `0N`
  (Amber's booleans are ints; q: `0b`), and a date, time or timestamp column `::`, since there is
  no temporal null (q: `0Nd`, `0Nt`, `0Np`).
- **An amend that leaves no column of a table a list fills each to the table's row count**
  (`` t[`a]:9 `` with `t:([]a:1 2 3)` gives `9 9 9`; an empty table stays empty), as `update` does;
  q gives `'rank` for these (`` t[`a]:9 ``, `` @[t;`a;:;9] ``, `` @[t;`a`b;:;9] ``).
- **Find gives `0N` for no match**, as in ngn/k: `1 2 3?5` is `0N`, where q gives the count, `3`.
  amber.k's `in` and `ij` rely on it.
- **A take from an empty general list gives empty strings**: `3#()` is `("";"";"")`, since an empty
  general list keeps a string as its type witness (CHANGELOG, "Empty general lists keep their type
  witness"); q gives `(();();())`.
- **`asof` gives `aj`'s answer on a table not sorted by time**: the row with the latest time at or
  before the one asked for, as `aj` (which sorts the quotes) matches it
  (`` asof[+`sym`time`bid!(`a`a`a`a;0 1 1 0;10 20 30 40);`sym`time!(`a;1)] `` is `` (,`bid)!,30 ``).
  q assumes the table is sorted, so its answer there is unspecified (it gives 40).
- **A table joined to a table of other columns is `'domain`** (issue #19): `` (+`a!,1 2),+`b!,3 4 ``,
  and `` (+`a!,1 2),,`b!3 `` (the row enlisted, a one-row table). A row dict of other columns makes
  a list of dicts: `` (+`a!,1 2),`b!3 ``. q gives `'mismatch` for all three
  (`` ([]a:1 2),([]b:3 4) ``, `` ([]a:1 2),enlist(enlist`b)!enlist 3 ``, `` ([]a:1 2),(enlist`b)!enlist 3 ``),
  and ngn/k a list of dicts.
- **Four q-named functions keep Amber's answer where q's differs (#83 Q10, #94 Q3).** `"ab" ss ""` is
  `0 1 2` (q: `'length`) and `` 0 1 in 0#` `` is `0 0` (q: `'type`): Amber answers where q refuses.
  `differ 1 1.00000000000001 1` is `1 1 1` (q: `100b`), since Amber has no comparison tolerance
  anywhere else. `` (!0) union 0#` `` is `!0` (q: `` `symbol$() ``), since q's type would cost every
  `union` a test for an empty `x`.
- **Five q-named functions keep Amber's answer for a dict or a table (#94 Q7).**
  `` distinct `a`b`c!1 1 2 `` is `1 2`, the distinct values, as ngn/k's `?` (q: `'type`); `raze` of a
  table is the table, as k's `,/` (q: its last row, as a dict); `flip ()!()` is an empty table with
  no columns (q: `'rank`); `` fills `a`b`c!(1;`x;0N) `` is `` `a`b`c!(1;`x;`x) `` (q: `'type`); and
  `xprev` of a keyed table is `'type` (q: `'length`).
- **`wsum` and `wavg` of a symbol list of another length are `'type` (#113).** `` wsum[1 2;`a`b`c] ``
  and `` wavg[1 2;`a`b`c] `` are `'type`, where q gives `'length`: a symbol list goes to `+/x*y`, and
  k's `*` checks the types before the lengths, as ngn/k's does. q's answer would cost a length test
  in front of every call that takes that path. Lists of numbers of different lengths are `'length`,
  as in q.
- **A table literal holds at most 256 columns in its key group and 256 in its value group (#114).**
  One more is `'limit`, as for the parser's other caps: `` . "([]",(";"/{"a",($x),":1"}'!257),")" ``
  is `'limit`, where q takes a literal of any width (3,000 and 100,000 columns tried). A wider table
  can be made with flip: `` #!+(`$"a",'$!300)!300#,,1 `` is `300`.
