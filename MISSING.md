# Amber vs kdb+/q — what's still missing

Amber covers a large slice of q's *vocabulary* (aggregations, dicts, tables, keyed tables,
the join family, qSQL-style select/by, strings, tick bars, and one attribute). This is an
honest map of what kdb+/q has that Amber does **not** yet — roughly in order of how much it
would change day-to-day use. "partial" means some of it exists.

## 1. Temporal types (biggest gap)
kdb+ has first-class temporal **types** with literals, arithmetic and auto-formatting:
`date` (`2024.01.15`), `month` (`2024.01m`), `year`, `time` (`09:30:00.000`),
`minute` (`09:30`), `second` (`09:30:00`), `timestamp` (`2024.01.15D09:30:00.000000000`),
`timespan` (`0D01:00:00`), `datetime`. Casting (`` `date$ ``, `"T"$"09:30"`), temporal
arithmetic, and the dotted accessors (`t.hh`, `d.month`, `p.date`).
- **Amber has (partial):** time-of-day as ms-since-midnight with `hms hh mm sec minute second
  milli stime ptime` and `minbar`/`bar` bucketing. No true types, no date/timestamp, no
  literals, no `$` temporal casts, no timestamp arithmetic.

## 2. Missing atom types
`short` (`h`), `real`/float32 (`e`), `byte` (`x`, `0x…`), `guid` (`g`, `0Ng`), plus the full
set of typed nulls/infinities (`0Nh 0Ne 0Wp 0Nd …`). Amber has long/float/char/symbol/bool
(and int) only, with `0N`/`0n` nulls.

## 3. qSQL (the template syntax)
Real `select … by … from … where …`, `update`, `delete`, `exec`, and their functional forms
`?[t;where;by;select]` and `![t;where;by;cols]`. Sorted/limited selects (`select[>px]`,
`select[5]`), `fby` inside where, correlated subqueries.
- **Amber has (partial):** functional helpers `qwhere qselect qby fby xgroup ungroup` — but not
  the parsed `select…from…` template, `update`, `delete`, or `exec`.

## 4. On-disk data (HDB) — entirely absent
Splayed tables, **date-partitioned databases**, `set`/`get` to disk, `\l db`, memory-mapping,
`.Q.dpft` (save partitioned), `.Q.en` (enumerate syms), `par.txt`, `.Q.chk`, `.Q.ind`,
`.Q.fs`/`.Q.fsn` (chunked file streaming), on-disk `aj` over partitions. Amber is in-memory only.

## 5. IPC & the tick architecture — absent
`hopen`/`hclose`, sync (`h"expr"`) and async (`neg[h]`) messaging, `.z.pg`/`.z.ps` query
handlers, `.z.po`/`.z.pc` connect/disconnect, `.z.w`, websockets, TLS. And the whole
tickerplant / RDB / HDB / gateway pattern (`tick.q`, `r.q`, `u.q`, `w.q`, `.u.sub`/`.u.pub`).

## 6. Attributes — 1 of 4
Amber implements **sorted (`` `s``)**. Missing: **`` `u`` unique**, **`` `p`` parted**,
**`` `g`` grouped** (the real-time hash index that powers fast `where sym=` on RDBs). No
attribute preservation through most ops (q keeps/drops them with defined rules).

## 7. Enumerations, foreign keys, linked columns
`` `sym$`` enumeration domains, `.Q.en`, foreign keys (`` `t$`` and dotted `order.customer.name`
traversal), linked columns, `.Q.fk`. None in Amber.

## 8. System namespaces
- **`.z.*`** clocks/handlers: `.z.p .z.P .z.z .z.t .z.d .z.T .z.D`, timer `.z.ts` + `\t`,
  `.z.exit`, `.z.pg .z.ps .z.po .z.pc .z.ph` (HTTP).
- **`.Q.*`** utilities: `.Q.dpft .Q.en .Q.hg/.Q.hp` (HTTP get/post) `.Q.gc .Q.w` (mem)
  `.Q.ty .Q.qt .Q.id .Q.j10/.Q.x10` (base64) `.Q.fc` (parallel) `.Q.trp` (protected)
  `.Q.dd .Q.pv/.Q.pf` (partitions) `.Q.s` (show) `.Q.f/.Q.fmt` (number format).
- **`.h.*`** HTTP/markup: HTML/CSV/XML/XLS rendering, an HTTP server.
- **`.j.*`** JSON: `.j.j` / `.j.k` (the array core has `` `j``; Amber doesn't wrap it yet).

## 9. Moving / window aggregates
`mavg msum mcount mmin mmax mdev mmu` (moving) and `ema`, `wj2`, plus `ajf`/`ajf0` (fill
as-of), `ij`f/`lj` fill variants, `ssr` vectorised, `rank`/`xrank` over tables.
- **Amber has:** `sums prds mins maxs deltas ratios differ prev next wsum wavg xprev`. Missing
  the `m*` moving family and `ema`.

## 10. Linear algebra & math
`mmu` (matrix multiply), `inv` (inverse), `lsq` (least squares), `.q` solve; distributional
`rand`, `binr`. Amber has `cor cov var dev svar sdev med` and scalar math.

## 11. Casting / parsing / serialization
The full `$` cast matrix (temporal, guid, byte), typed file reader `("SIF";",")0:file`,
`vs`/`sv` for base-N and temporal, `parse`/`eval`/`reval`, `-8!`/`-9!` (serialize/deserialize),
`-18!` (compress), `-11!` (replay log), `md5`, `.Q.btoa`. Amber has `sv vs ss ssr like`,
string casts, and `` `k`` (k-repr).

## 12. Concurrency & performance ops
`peach` (parallel each), secondary threads (`-s`), `.Q.fc` (parallel-on-cut), map-reduce over
partitions, compression, `\ts` (time+space). Amber is single-threaded, in-memory.

## 13. Console / environment niceties
`\c` console dims, `\ts`, `\w` (workspace) — the array core has `\w`; `system"…"`, `getenv`/
`setenv`, `\cd`. Number formatting `.Q.f`. Editor tooling / language server.

---

### Nice next steps (highest value first)
1. **`` `g`` grouped attribute** in C — pairs with the sorted work you already have and unlocks
   fast `where sym=`.
2. **Real temporal types** (at least `date` + `timestamp`) with literals and `$` casts.
3. **`select … from … where …` parser** on top of the existing `qwhere`/`qby` engine (the
   `([]…)` literal work in `p.c` shows the pattern).
4. **`set`/`get` to disk** for a minimal splayed/partitioned HDB.
5. **`hopen`/IPC** for a toy tickerplant.
