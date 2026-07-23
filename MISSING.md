# Amber vs kdb+/q — what's implemented, what's still missing

Amber covers a large slice of q's *vocabulary*. This is an honest map. The **v1.5**
release added a big batch of the items that used to be listed here; those are now in the
**Implemented** section, with the remaining gaps below.

---

## Implemented in v1.5 (new modules)

- **qSQL template syntax** (`qsql.k`): `sel "select … by … from … where …"`, plus
  `exq` (exec), `upd` (update), `del` (delete), and the functional `qexec[t;w;b;d]`.
  Column names in expressions are rewritten to `x\`col` and run on the existing
  qwhere/qby/qselect engine.
- **Temporal types** (`temporal.k`): `date` (days since 2000.01.01) and `timestamp`
  (nanos since 2000.01.01) with constructors (`ymd2d`, `tstamp`), formatting (`dstr`,
  `pstr`), parsing (`pdate`), accessors (`year month dayof dow`) and arithmetic. Held
  numerically, kdb-style. *(Still no dedicated literal syntax or true C-level types.)*
- **On-disk data / HDB** (`hdb.k`): `dset`/`dget` (a value ↔ a file), `splay`/`dload`
  (a table ↔ a splayed directory), `partsave`/`partload`/`parts` (a value-partitioned
  database). Stored as portable Amber text.
- **IPC & a tick architecture** (`ipc.k`): `hopen`/`hclose`/`hsend`/`hrecv`/`hsync`
  raw-socket messaging, and a fully in-process **tickerplant** — `u.def`/`u.sub`/
  `u.pub`/`u.get` (define a stream, subscribe callbacks, publish rows).
  *(The core speaks raw sockets, not the kdb+ binary wire protocol.)*
- **System namespaces** (`sys.k`): `z.*` clocks (`z.p z.P z.n z.d z.D z.t z.T z.z`),
  `Q.*` utilities (`Q.f Q.fmt Q.s Q.dd Q.fc Q.gc Q.id Q.n Q.a Q.A Q.trp …`), `j.*`
  JSON (`j.j` encode, `j.k` decode) and `h.*` HTML (`h.ht h.hc`).
  *(Written `z.p` etc. — without kdb's leading dot, which is the eval verb here.)*
- **Moving / window aggregates** (`std.k`): `msum mavg mcount mprd mvar mdev mmin mmax`
  — the O(n) prefix-based ones are truly vectorised; plus `mmu` (matrix multiply) and
  `dot`. (`movavg`/`ema`/`rollstd` also remain in `fin.k`.)
- **Casting / parsing / serialization** (`std.k`): `parse eval reval ser deser protect`
  and typed constructors `long int float char sym bool` + `cast[\`type;x]`.
- **All four column attributes** in C: `\`sa` sorted, `\`ua` unique, `\`pa` parted,
  `\`ga` grouped; `\`at` reads them; `meta` shows them.
- **`peach`** (parallel-each; sequential in this single-threaded build) and **`\ts`**
  (time an expression at the REPL).
- **Interpreter capacity**: the global table was widened from a **1-byte** to a
  **2-byte** index (256 → 4096 globals) so the full extended vocabulary loads at once.

---

## Still missing

### Atom types
`short` (`h`), `real`/float32 (`e`), `byte` (`x`, `0x…`), `guid` (`g`) — Amber has
long/float/char/symbol/bool/int only. These need new C-level types.

### True temporal *types* & literals
Amber holds dates/timestamps numerically; kdb has first-class types with literal
syntax (`2024.01.15`, `09:30:00.000`, `2024.01.15D…`) and auto-formatting on display.

### qSQL depth
`select[>col]` / `select[n]` (sorted/limited selects), correlated subqueries, `fby`
inside `where`, and parsed `insert`/`upsert` statements.

### On-disk depth
Real memory-mapping, date-partitioned HDBs with `par.txt` across drives, `.Q.dpft`,
`.Q.en` enumeration, on-disk `aj` over partitions, compression.

### Real threading
`peach` is a sequential fallback; true secondary threads (`-s`) and `.Q.fc` parallelism
need a threaded core.

### IPC wire protocol
`hsync` exchanges text, not the kdb+ binary protocol; no `z.pg`/`z.ps` dispatch over
real connections, no websockets/TLS.

### Enumerations, foreign keys, linked columns
`\`sym$` domains, `.Q.en`, foreign-key traversal (`order.customer.name`), linked columns.

### Misc
`-8!`/`-9!` binary serialize (Amber serializes as text instead), a `\ts` space metric
(the core has no allocator introspection), `inv`/`lsq` linear algebra, real console
niceties (`\c`, editor tooling).

---

### Next steps (highest value first)
1. Real temporal **types** with literal syntax on top of the numeric layer.
2. `select[>col]` / `select[n]` sorted-and-limited selects.
3. New C atom types (`byte`, `real`, `short`).
4. Binary `-8!`/`-9!` serialization for compact on-disk/IPC.
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

## 6. Enumerations, foreign keys, linked columns
`` `sym$`` enumeration domains, `.Q.en`, foreign keys (`` `t$`` and dotted `order.customer.name`
traversal), linked columns, `.Q.fk`. None in Amber.

## 7. System namespaces
- **`.z.*`** clocks/handlers: `.z.p .z.P .z.z .z.t .z.d .z.T .z.D`, timer `.z.ts` + `\t`,
  `.z.exit`, `.z.pg .z.ps .z.po .z.pc .z.ph` (HTTP).
- **`.Q.*`** utilities: `.Q.dpft .Q.en .Q.hg/.Q.hp` (HTTP get/post) `.Q.gc .Q.w` (mem)
  `.Q.ty .Q.qt .Q.id .Q.j10/.Q.x10` (base64) `.Q.fc` (parallel) `.Q.trp` (protected)
  `.Q.dd .Q.pv/.Q.pf` (partitions) `.Q.s` (show) `.Q.f/.Q.fmt` (number format).
- **`.h.*`** HTTP/markup: HTML/CSV/XML/XLS rendering, an HTTP server.
- **`.j.*`** JSON: `.j.j` / `.j.k` (the array core has `` `j``; Amber doesn't wrap it yet).

## 8. Moving / window aggregates
`mavg msum mcount mmin mmax mdev mmu` (moving) and `ema`, `wj2`, plus `ajf`/`ajf0` (fill
as-of), `ij`f/`lj` fill variants, `ssr` vectorised, `rank`/`xrank` over tables.
- **Amber has:** `sums prds mins maxs deltas ratios differ prev next wsum wavg xprev`. Missing
  the `m*` moving family and `ema`.

## 9. Linear algebra & math
`mmu` (matrix multiply), `inv` (inverse), `lsq` (least squares), `.q` solve; distributional
`rand`, `binr`. Amber has `cor cov var dev svar sdev med` and scalar math.

## 10. Casting / parsing / serialization
The full `$` cast matrix (temporal, guid, byte), typed file reader `("SIF";",")0:file`,
`vs`/`sv` for base-N and temporal, `parse`/`eval`/`reval`, `-8!`/`-9!` (serialize/deserialize),
`-18!` (compress), `-11!` (replay log), `md5`, `.Q.btoa`. Amber has `sv vs ss ssr like`,
string casts, and `` `k`` (k-repr).

## 11. Concurrency & performance ops
`peach` (parallel each), secondary threads (`-s`), `.Q.fc` (parallel-on-cut), map-reduce over
partitions, compression, `\ts` (time+space). Amber is single-threaded, in-memory.

## 12. Console / environment niceties
`\c` console dims, `\ts`, `\w` (workspace) — the array core has `\w`; `system"…"`, `getenv`/
`setenv`, `\cd`. Number formatting `.Q.f`. Editor tooling / language server.

---

### Nice next steps (highest value first)
1. **Real temporal types** (at least `date` + `timestamp`) with literals and `$` casts.
2. **`select … from … where …` parser** on top of the existing `qwhere`/`qby` engine (the
   `([]…)` literal work in `p.c` shows the pattern).
3. **`set`/`get` to disk** for a minimal splayed/partitioned HDB.
4. **`hopen`/IPC** for a toy tickerplant.
