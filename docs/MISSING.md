# Amber vs kdb+/q — what's still missing

Amber covers a large slice of q's *vocabulary* (aggregations, dicts, tables, keyed tables,
the join family, qSQL-style select/by, strings, tick bars, native temporal types, all four
attributes, moving aggregates, a text-based on-disk / IPC layer, and the `.z`/`.Q`/`.j`/`.h`
namespaces). This is an honest map of what kdb+/q has that Amber does **not** yet — roughly in
order of how much it would change day-to-day use. "partial" means some of it exists.

## 0. New in 1.9 (not q gaps — Amber-specific)
Three items here are *beyond* stock q rather than catching up to it, so they don't map to a gap
above; they're recorded for completeness:
- **Native `aj` kernel** — the as-of join's match step is now a C `lower_bound` (branch-free /
  `cmov`) over each group's sorted ns-timestamp slice, replacing the per-row K `bin`. In-memory
  `aj`/`aj0` are covered; **on-disk `aj` over partitions** is still missing (see §4).
- **HFT zero-allocation arena** — a thread-local 16 MB bump allocator for the transient buffers
  produced during evaluation, rewound per eval cycle to keep `malloc`/`free` jitter off the hot
  path. (q has no user-facing equivalent; kdb+ manages this internally.)
- **Rust-style diagnostics** — opt-in (`AMBER_DIAG=1`) `error[CODE]` reports with a `-->` locator,
  a gutter-aligned source line, `^^^` underlines and a `= help:` note. This is *richer* than q's
  single-line `'error` and than Amber's own default caret line, which both remain.

## 1. Temporal types — done (1.7)
Native `date` / `time` / `timestamp` types with literal syntax (`2026.07.30`,
`10:00:05.000`, `2026.07.30D09:30:00.000000000`), auto-display, type-aware arithmetic
(`time+time`, `date-date`→days, `date+n`, comparisons), string casts `"D"$`/`"T"$`/`"P"$`,
and accessors `year`/`month`/`day`/`dow`/`thh`/`tmm`/`tss`. Columns keep numeric storage
so `xasc`/`s#` work unchanged.
- **Still missing:** `month`/`minute`/`second`/`timespan`/`datetime` as distinct types,
  `m` month-literals, and the dotted `t.hh` accessor form (Amber uses `thh t`).

## 2. Missing atom types
`short` (`h`), `real`/float32 (`e`), `byte` (`x`, `0x…`), `guid` (`g`, `0Ng`), plus the full
set of typed nulls/infinities (`0Nh 0Ne 0Wp 0Nd …`). Amber has long/float/char/symbol/bool
(and int) only, with `0N`/`0n` nulls.

## 3. qSQL (the template syntax) — mostly done
The `select … by … from … where …` template now works **bare** (no `sel"…"` wrapper), along
with `exec`, `update`, and `delete` — see AMBER.md §7. Still missing: the general functional
forms `?[t;where;by;select]` / `![t;where;by;cols]`, sorted/limited selects (`select[>px]`,
`select[5]`), `fby` *inside* a where-clause, and correlated subqueries.
- **Amber has:** bare + string `select/exec/update/delete`, plus the functional helpers
  `qwhere qselect qby fby xgroup ungroup`.

## 4. On-disk data (HDB) — partial (`hdb.k`)
Amber now has a **text-serialised** on-disk layer: `dset`/`dget` (value ↔ single file),
`splay`/`dload` (splayed table ↔ directory, one file per column plus a `.d`), and
`partsave`/`partload`/`parts` (**value-partitioned** database, one splayed dir per partition
value, with `par.txt`). Files are portable Amber text read back with `eval`, so they are
human-readable and version-independent.
- **Still missing:** true **date-partitioned** on-disk format, **memory-mapping** (data is fully
  read into RAM, not mapped), `.Q.dpft` (save partitioned in kdb layout), `.Q.en` (enumerate
  syms), `.Q.chk`, `.Q.ind`, `.Q.fs`/`.Q.fsn` (chunked file streaming), on-disk `aj` over
  partitions, and a binary (not text) on-disk encoding.

## 5. IPC & the tick architecture — partial (`ipc.k`)
Amber now ships `hopen`/`hclose`/`hsend`/`hrecv`/`hsync` (raw-socket messaging) and an
**in-process tickerplant** — `u.def` (define a stream), `u.sub`/`u.pub` (subscribe / publish),
`u.get`/`u.end`. `.z.pg`/`.z.ps` handlers exist as evaluate-stubs in `sys.k`.
- **Still missing:** the kdb+ **binary wire protocol** (Amber's sockets exchange plain text
  expressions, not IPC-encoded messages), real over-the-network `.z.pg`/`.z.ps`/`.z.po`/`.z.pc`
  handler dispatch, `.z.w`, websockets, TLS, and the full multi-process tickerplant / RDB / HDB /
  gateway pattern (`tick.q`, `r.q`, `u.q`, `w.q`).

## 6. Attributes — 4 of 4 (setters); find accel on 2
All four kdb+ attributes are set in C: **sorted (`` `sa``)**, **unique (`` `ua``)**,
**parted (`` `pa``)**, **grouped (`` `ga``)**, read back with `` `at``. **Sorted and parted**
vectors take the O(log n) binary-search find path; grouped pairs with `fin.k`'s group index
(`bysym`/`symrows`) for O(1) per-symbol slicing.
- **Still missing:** dedicated find/`where=` acceleration driven by the `` `u`` / `` `g``
  attribute *itself* (grouped speed currently comes from the separate group index, not the
  attribute), and **attribute preservation through ops** — the flag is dropped whenever an op
  builds a new vector, whereas q keeps/drops attributes by defined per-op rules.

## 7. Enumerations, foreign keys, linked columns
`` `sym$`` enumeration domains, `.Q.en`, foreign keys (`` `t$`` and dotted `order.customer.name`
traversal), linked columns, `.Q.fk`. None in Amber.

## 8. System namespaces — partial (`sys.k`)
Amber now provides the common members (as `.`-style names `z.*`/`Q.*`/`j.*`/`h.*`):
- **`.z.*`** clocks **done**: `z.p z.P z.n z.d z.D z.t z.T z.z z.w`. Handlers `z.pg z.ps z.po
  z.pc z.ts z.exit` exist but are **evaluate/no-op stubs** (no real timer `\t` or port dispatch).
  Missing: `.z.ph` (HTTP).
- **`.Q.*`** **done**: `Q.f Q.fmt` (number format), `Q.s` (show), `Q.ty Q.qt Q.id Q.dd`,
  `Q.gc Q.w` (mem placeholders), `Q.fc` (sequential fallback), `Q.trp` (protected).
  Missing: `.Q.dpft .Q.en` (partition/enumerate), `.Q.hg/.Q.hp` (HTTP get/post),
  `.Q.j10/.Q.x10` (base64), `.Q.pv/.Q.pf` (partition vars).
- **`.j.*`** JSON **done**: `j.j` (encode) / `j.k` (decode, via the core `` `j``).
- **`.h.*`** markup **partial**: a minimal HTML table/row renderer (`h.ht h.hrow h.hc`).
  Missing: CSV/XML/XLS rendering and an HTTP server.

## 9. Moving / window aggregates — mostly done (`std.k`, `fin.k`)
The moving family is implemented: `mcount msum mavg mprd mvar mdev mmin mmax` (`std.k`, O(n)
prefix sums; `mmin`/`mmax` are O(n·w) window scans) plus **`ema`** (C kernel, O(n) sweep).
- **Amber also has:** `sums prds mins maxs deltas ratios differ prev next wsum wavg xprev`.
- **Still missing:** `wj2`, `ajf`/`ajf0` (fill as-of), `ij`/`lj` fill variants, vectorised
  `ssr`, and `rank`/`xrank` *over tables*. (`mmin`/`mmax` could also move to an O(n)
  monotonic-deque form — see BENCHMARKS.md.)

## 10. Linear algebra & math — partial (`std.k`)
`mmu` (matrix multiply) and `dot` (vector dot product) are implemented. Amber also has
`cor cov var dev svar sdev med` and scalar math.
- **Still missing:** `inv` (inverse), `lsq` (least squares), `.q` solve; distributional
  `rand`/`binr`.

## 11. Casting / parsing / serialization — partial (`std.k`)
`parse`/`eval`/`reval` are implemented, along with a **text** `ser`/`deser` round-trip (portable
Amber text via `` `k``, inverted by `eval`) and `protect` (like `.Q.trp`). Amber also has
`sv vs ss ssr like`, string casts, and `` `k`` (k-repr).
- **Still missing:** the **binary** serialiser `-8!`/`-9!` (current `ser`/`deser` is text, not the
  compact IPC encoding — this is the top "nice next step" below), `-18!` (compress), `-11!`
  (replay log), the full `$` cast matrix (guid, byte), typed file reader `("SIF";",")0:file`,
  `vs`/`sv` for base-N and temporal, `md5`, `.Q.btoa` (base64).

## 12. Concurrency & performance ops — partial
`peach` is real **multi-core** (forks `AMBER_THREADS` worker processes, C kernel), and `ts`
(`\ts`) times an expression.
- **Still missing:** kdb-style secondary threads (`-s`), a *parallel* `.Q.fc` (Amber's is a
  sequential fallback), map-reduce over on-disk partitions, and compression. `peach` currently
  ships each worker's result back as **text** (`` `k``) — a binary serialiser (§11) would cut
  that transfer cost.

## 13. Console / environment niceties — partial
`\ts` (via `ts`) and number formatting `.Q.f`/`.Q.fmt` are done.
- **Still missing:** `\c` console dims, a real `\w` (workspace) report (`Q.w` is a placeholder),
  `system"…"`, `getenv`/`setenv`, `\cd`, and editor tooling / a language server.

## 14. Known engine bugs (not q gaps — real defects, tracked here until fixed)
- **Bare `/` comment line silently truncates the rest of the file.** A `.k` comment line
  containing *only* `/` — no trailing space, no trailing text — causes the lexer to stop
  parsing everything after it with no error raised (exit code 0, empty output). A comment line
  with a trailing space (`` / `` won't repro but `` /  `` with content will) or any actual text
  after the slash is unaffected. Found while writing `bench/queries/amber_*.k`'s multi-paragraph
  header comments (worked around there with blank lines instead of bare `/` separators). Root
  cause not yet isolated to a specific lexer function; needs a repro-minimized case and a fix in
  the comment-scanning path before this entry can move to "done."
- **`` `&`` (where) on a literal empty generic list returns a spurious non-empty result.**
  `&()` returns `,!0` (a 1-element list containing an empty vector) instead of the correct `!0`
  (a plain empty vector) — compare `&!0`, which correctly returns `!0`. Traced to `src/v.c`'s
  `X1(whr,...)`, the `RA` (generic-list) branch: its embedded K expression
  `` {$[`A~@x;(,&#'*'x),,'/x@\:!0|/#'x:o'x;,&x]} `` is written for *nested* input (grouping /
  per-key counts) and mishandles a simply-empty `()`, which also has type `` `A``, falling into
  that same branch instead of being recognized as "zero elements, nothing to do." Found via
  `qsql.k`'s `ss` (string search), which calls `&` on the result of `` '``-ing a K-defined
  predicate over an explicitly empty range whenever the search pattern is longer than the
  remaining string — exactly the case hit by bare `select from t` / `exec col from t` with no
  `where`/`by` clause, so this was silently breaking a wide swath of everyday qSQL. Worked
  around at the K level (`amber.k`'s `ss` now special-cases `(#s)<#p` before ever calling `&`)
  rather than risk destabilizing `` `&``, which is a widely-used core verb — but the underlying
  C bug is still there for any *other* caller that happens to construct a literal empty `()` and
  feed it through `` `&``. Needs a proper fix in `whr`'s `RA` branch (an early `P(!xn,...)`-style
  guard before the nested-list logic runs) plus a regression test, then this `ss` workaround can
  be reverted.

---

Already done (once gaps): **bare qSQL** `select/exec/update/delete` (1.5), **as-of join** —
vectorised in 1.5, **native C kernel** in 1.9, **multi-core `peach`** (1.6, fork-based),
**Q-style grid preview** (1.6), **Unicode `\grid` modes + diagnostics + arena** (1.9),
**native temporal types** (1.7), **C-kernel `wj`/`ema`** (1.7), **terminal charting**
`plot`/`candle` (1.7), **Apache Arrow C Data Interface** (1.7), **all four attributes** in C
(§6), the **moving-aggregate family** `m*` (§9), **`mmu`/`dot`** (§10), **`parse`/`eval`/`ser`**
(§11), a **text-based on-disk layer** `dset`/`splay`/`partsave` (§4), and a **text IPC / in-process
tickerplant** `hopen`/`u.*` (§5).

### Nice next steps (highest value first)
1. **Binary serialiser (`` -8!``/`` -9!``)** — the single biggest remaining lever. `peach` and
   the on-disk / IPC layers all currently move values as **text** (`` `k ``) and re-parse them.
   A compact binary encode/decode would cut that transfer cost, widen the range of workloads where
   `peach` beats serial `'`, and unlock a real (binary-wire) IPC and a binary on-disk format.
2. **Grouped-attribute-driven `where sym=`** — the `` `g`` setter exists, but fast `where sym=`
   currently comes from `fin.k`'s separate group index rather than from the attribute itself.
   Wiring the attribute into the C find path (as sorted/parted already are) would make it automatic.
3. **Missing atom types** (§2) — `short`/`real`/`byte`/`guid` and their typed nulls/infinities.
4. **Attribute preservation through ops** (§6) — keep/drop attributes by q's per-op rules instead
   of always dropping on a new allocation.
5. **True partitioned/mmap HDB** (§4) — a date-partitioned, memory-mapped on-disk format beyond
   the current text splay, plus `.Q.dpft`/`.Q.en`.
6. **Live REPL syntax highlighting** — colouring tokens *as you type*, not just on a line you've
   already run. This needs `repl.k`'s raw-keystroke input loop rewritten to re-tokenize and
   redraw the current line on every keypress (a fragile, previously-regression-prone path in
   this project — see CHANGELOG), not a new `\command`. A prior attempt shipped as a `\hl <expr>`
   one-shot echo command instead and was removed for not matching what "live" means.
