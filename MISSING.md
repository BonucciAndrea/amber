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
