# Changelog

## 1.9 — version unification, memory/UB audit, and a combinatorial test suite

### Version
- **One canonical version string.** `AMBER_VERSION` now lives in `src/a.h` and is the only
  place a release is bumped. `binfo` (`` `bi 0``) returns it as a 5th element, the REPL banner
  reads it from there (it was hard-coded `v1.7` while the README already advertised 1.9), and
  `amber --version` prints it.
- **`amber --help` / `-h` and `amber --version` / `-v` added.** `main()` previously treated
  `argv[1]` unconditionally as a script path, so `amber --version` tried to open a file called
  `--version` and failed with an `'io` error. `--help` also lists the full `\`-command reference.

### Bugs fixed — memory safety and undefined behaviour
- **Out-of-bounds stack read in `a8()` (amend), `src/a.c`.** The `MC()` calls copied a fixed
  8-slot's worth of argument pointers (40/48/56/64 bytes) out of the caller's `a[]` regardless of
  how many arguments were passed. `a` is not always an 8-element buffer — `run()` (`src/b.c`)
  passes a pointer straight into its own dynamically sized stack frame — so every amend with
  `n<8` read past the end of live storage. Reproduced by AddressSanitizer as
  *dynamic-stack-buffer-overflow* on `test-fin.k` and four `examples/*.k`. Each copy is now
  clamped to the real argument count.
- **`NL` (the long null) was undefined behaviour.** `#define NL (1ll<<63)` shifts into the sign
  bit of a signed type. UBSan flagged it from 17 different sites across `a.c`, `h.c`, `p.c`,
  `r.c`, `f.c`, `s.c`, `c.c`, `i.c`, `3.c` and `csv.c`. Now `((L)(1ull<<63))`, same bits,
  defined semantics. The same treatment was applied to the remaining signed-shift sites in
  `1.c` (`neg`), `2.c` (`ozZ`), `3.c` (`mmmf`, `mxms`) and `p.c` (`pf`), and to `su()`'s
  `1<<31` in `m.c`.
- **Intentional integer wrap was undefined behaviour in the arithmetic kernels** (`src/2.c`,
  `src/v.c`). `aLL/aII/aHH/aGG`, `alL/aiI/ahH/agG`, the widest-width multiplies and `tilV()`'s
  packed-lane counter all rely on two's-complement wraparound, which `oZZ()`/`ozZ()` then detect
  after the fact — but signed overflow is UB, so the optimiser is entitled to assume it never
  happens and delete the very check that depends on it. All of them now do the arithmetic in the
  unsigned counterpart type: identical instructions, defined semantics.
- **Use-after-reset in the `` `simd`` self-test** (`src/a.c`). `arena_reset()` was called
  immediately after `arena_alloc()` and *before* the 400 009-element reference loop that wrote
  into the block, so every store landed in scratch the allocator had already rewound.
- **`memcpy(dst, NULL, 0)` in `run()`** (`src/b.c`) — undefined per the `nonnull` attribute on
  `memcpy`; now guarded.
- **`arena_alloc()` bounds check could wrap.** `off + bytes <= a_cap` overflows for a large
  `bytes` on a 32-bit target (wasm32), silently passing the check and returning a short block.
  Rewritten as `bytes <= a_cap - off`, which cannot wrap. The overflow-block path got the same
  treatment, and `ajc()` now rejects an element count whose byte size would overflow `size_t`.
- **`arena_init()` leaked tracked overflow blocks** when a previous slab reservation had failed
  (`a_base == 0`), because it reset `a_over` to 0 without freeing the list.
- **`arena_used()` under-reported.** It returned only the slab bump cursor, ignoring live
  overflow blocks; it now counts both.
- **`ast_new()` / `ast_add_child()` dereferenced a possibly-NULL `arena_alloc()`** (`src/ast.c`).
  `ast_new()` now degrades to a static sentinel node — returning NULL would only move the
  segfault, since all ~30 call sites dereference the result immediately.
- **`mmap()` failure test used a pointer-to-`char` cast** (`src/m.c`): `(L)p == (C)p`. Replaced
  with a plain `p == MAP_FAILED`.

### Bugs fixed — language semantics
- **A computed float null never matched the `0n` literal.** IEEE has no single NaN bit pattern:
  `0n` is the positive quiet NaN, but every NaN the FPU *computes* (`0%0`, `0w-0w`,
  `avg 0#0`, …) is the default NaN, which on x86-64 has the sign bit set. `~` compared bytes, so
  `(0%0)~0n` was `0b` even though both sides print as `0n` and both answer `1b` to `^`. `mtc_()`
  now compares float payloads value-wise with all NaNs treated as one value (only on the path
  where the byte-wise fast path has already failed, so equal data still costs one `memcmp`).
- **The float formatter printed `-0n`** for any computed NaN. A NaN has no meaningful sign;
  `sf()` no longer emits the leading `-` for it. `0%0` and `(-0w)+0w` now print `0n`.
- **`deltas` and `ratios` raised `'length` on an empty argument** (`amber.k`). `0,-1_()` is a
  one-element vector, so `x-0,-1_x` mismatched. Both now return the empty vector, as q does.
- **`sum`/`prd` of an empty FLOAT vector returned an integer.** `amber.k`'s wrappers
  short-circuit an empty argument to the literal `0`/`1`, dropping the float type that the
  underlying `+/`/`*/` reductions get right (`+/0#0.0` is `0.0`, but `sum 0#0.0` was `0`). This
  surfaced as `select t:sum px from none` producing an int column. Both now return `0.0`/`1.0`
  for a float argument.
- **`med` of an empty vector returned `0.0`** while `avg`/`dev`/`var` return the float null; it
  now returns `0n` like the rest of the aggregation family.
- **`qselect` was broken for more than one aggregation** (`amber.k`).
  `select a:sum px, b:max sz from t` built the dict `` `a`b!(210.0;6)`` and flipping it raised
  `'length`; the single-aggregate case only *appeared* to work because the each-result happened
  to be a one-item list. Each aggregate result is now joined onto `()` so an atom becomes a
  one-row column.
- **`select by sym from t` (a `by` clause with no projection) raised `'value`** (`qsql.k`). The
  `" by "` split never matched, because when the by-clause is the whole select-part it has no
  space in front of it; and even once split, an empty projection ignored the grouping entirely.
  `qsplit0` handles the position-0 case and the empty projection now aggregates each non-key
  column with `last`, matching q.

### New: `` `diag`` — runtime control of the stderr diagnostic
Amber renders its Rust-style diagnostic at error **creation** time, so code that catches an error
with `.[f;args;handler]` has already had the full report splashed across stderr. That is right for
an interactive line and wrong for anything whose job is to provoke errors it then handles — a test
suite's must-raise cases, `protect`, a retry loop — which drowned the terminal in red for errors
it dealt with perfectly. `` `diag 0`` turns the report off and returns the previous setting;
`` `diag 1`` turns it back on. The compact caret text is untouched: it is buffered and still
handed to the trap handler and to `` `err``, so nothing is lost. The existing `AMBER_DIAG=0`
environment variable still works and now just seeds the initial value.

### Diagnostics
- **`\trace`'s "Arena peak" was always `0 B`.** It reported `max(used-before, used-after)`, and
  every arena consumer rewinds the slab before returning. `arena.c` now maintains a true
  high-water mark (`arena_peak()` / `arena_reset_peak()`) that survives `arena_reset()`.
- **`\trace` timings below one microsecond rendered as `0us`;** the formatter now prints
  `ns` / `us` / `ms` as appropriate.
- **`\trace`'s report box is square again** — the bar glyph is 3 bytes but 1 column wide, and the
  old `printf` field widths counted bytes, leaving the right-hand border ragged.
- The `` `arn`` self-test now genuinely exercises the arena **overflow** path it advertises. It
  asked for 1 MB against a >=16 MB slab (`arena_init(1<<16)` only rewinds an already-larger slab,
  it never shrinks it), so the overflow branch was never taken; it now requests
  `arena_capacity() + 64 KB` and also asserts that the peak survives a rewind.

### Dead code and duplication removed
- `src/3.c`: deleted the unused static helpers `minfB`, `maxfB` and `eqlsL`.
- `src/a.c`, `src/h.c`, `src/w.c`, `src/i.c`: removed unused / write-only locals (`m`, `p`, `u`)
  and a discarded expression value (`f>2&&close(f)`).
- **One canonical `lower_bound`.** `ajlb()` (`src/a.c`) and `wjlb()` (`src/i.c`) were the same
  contract under two names with different (branch-free vs branchy) codegen. Both join kernels now
  call the exported branch-free `amlb()`.

### Tests
- **`tests/harness.k`** — shared assertion harness (`t`, `tv`, `te`, `tk`, `hexpect`, `hreport`).
  Every assertion is trapped, so a failing or throwing case can never abort a suite or hide the
  cases after it.
- **Suites are self-locating.** `\l amber.k` resolves against the *current working directory*, so
  a suite written that way only runs from the repo root — `amber /path/to/tests/test_matrix.k`
  died with a bare `'io`. Each suite now derives the repo root from its own script path
  (`` `argv 1``, the same trick `repl.k` uses) and runs from anywhere.
- **Suites exit non-zero on failure**, so CI and `tests/run_tests.sh` can gate on status as well
  as on the printed report.
- **`hexpect[n]` assertion-count guard.** In K, `f [a;b]` **with a space** is not a call: the
  parser reads `[a;b]` as a bracketed statement block and quietly builds a projection that is
  then discarded — no error, no output, the assertion simply never runs. An earlier revision of
  `tests/test_qsql.k` lost 42 of its 93 cases to exactly this **and still printed
  "ALL TESTS PASSED"**. Each suite now declares how many assertions it expects to record and
  fails if the number disagrees.
- **`tests/test_qsql.k` is written in the bare `select … from t` syntax** users actually type,
  put through the same `qrw` rewrite `repl.k`'s `line1` applies to every input line, instead of
  the `sel"…"` wrapper — so the suite exercises the rewriter and the query engine together, on
  the user's own code path. Section 9 asserts the explicit wrapper gives an identical answer.
- **`tests/test_matrix.k`** — 309-case combinatorial matrix: every primitive against every
  element type (Long / Float / Boolean / Char / Symbol / nested list / dict / table) at sizes
  0, 1, 10 and 100 000+, including the 99 999 / 100 000 / 100 001 / 250 000 sizes that straddle
  `PAR_THRESHOLD`. Cases assert invariants (shape, algebraic identity, vector-kernel-vs-scalar-
  reference agreement) rather than frozen literals.
- **`tests/test_qsql.k`** — 94-case qSQL matrix over the full clause lattice, multi-key `by`,
  empty / single-row / heavily-duplicated tables, the bare-qSQL rewriter, and malformed queries.
- **`tests/fuzz.py`** — malformed-input and deep-nesting crash fuzzer (unbalanced brackets,
  dangling adverbs, truncated qSQL, 20 000-deep nesting, out-of-range literals). Asserts a clean
  K error, never a signal or a hang.
- **`tests/run_tests.sh`** — one entry point for all suites plus the C unit tests; `--asan`
  rebuilds under AddressSanitizer + UBSan and re-runs everything, including the fuzzer.
- **The suites are quiet on success.** `tests/harness.k` calls `` `diag 0`` on load and restores
  the previous setting in `hreport`, so a run that provokes ~35 deliberate errors prints its
  summary and nothing else instead of ~35 full-colour diagnostic blocks. A case that fails
  unexpectedly still reports the error kind on its own FAIL line (`FAIL raised: 'length  <-
  deltas 0#0`), so nothing is hidden.
- CI (`.github/workflows/ci.yml`) now runs the new suites and the fuzzer on every push.

## Unreleased — separate, independently-tuned comparative benchmark query files
- **`bench/queries/amber_{vecsum,vecarith,groupby}.k`** added — `bench/run_comparative.py`'s
  Amber row previously reran the same `k_<id>.k` file used for the "K" (ngn/k) row. Amber now
  gets its own query file per workload, with a header comment on each documenting what
  engine-level optimization was tried, what it measured, and — for the ideas that didn't pay
  off — why not. See the README's
  [Comparative benchmark query files](../README.md#comparative-benchmark-query-files) section
  for a summary, or the files themselves for the full detail.
- **`check_parity()`** added to `bench/run_comparative.py`: runs `amber_<id>.k` and `k_<id>.k`
  once each before the timed runs and compares their printed output, so a claimed speed win can
  never silently also be a wrong answer. Reported on stderr and prefixed onto the generated
  Markdown table.
- **Known engine bug found and documented** (not fixed here — see docs/MISSING.md): a `.k`
  comment line containing only a bare `/` with no trailing space or text silently truncates
  parsing of the rest of the file, with no error raised.

## Unreleased — removed the `\hl` syntax-highlight command
- **Removed** `src/highlight.{h,c}`, the `\hl <expr>` REPL command, and `tests/test_highlight.c`.
  `\hl` only ever colorized a line you explicitly ran (`\hl select ...` echoed that one line back
  with ANSI colour) — it never highlighted your keystrokes *as you typed them*, which is what
  "REPL live syntax highlighting" actually means. Real live/incremental highlighting would
  require rewriting `repl.k`'s raw-keystroke input loop (a fragile, previously-regression-prone
  path in this project), which is a materially different and larger undertaking than a one-shot
  echo command — removed rather than kept as something that doesn't do what its name promises.
  See [docs/MISSING.md](MISSING.md) for this as a possible future direction.
- No other engine extension is affected: SIMD, the multithreaded vector engine, the bytecode
  disassembler, and the native CSV parser are all unchanged.

## Unreleased — `\ast` visualizer overhaul
- **Bug fixed: generic `<X-atom>` placeholders.** The previous formatter only understood five
  atom tags (symbol/int/long/float/char) and treated everything else -- multi-element data
  vectors (`1 2 3`, `` `a`b`c ``, `"hello"`, ...), verb/adverb atoms appearing bare (inside a
  hook/fork, or `\ast +` on its own), lambda bodies (`{x+1}`), and the `GAP` sentinel that marks
  a curried-away argument -- as an opaque "bare atom" and printed a useless `<v-atom>`/
  `<w-atom>`/`<o-atom>`/`<I-atom>`/`<S-atom>` placeholder. Every one of those shapes now gets an
  explicit, correctly-typed node: literal vectors preview their contents with a
  `(TypeName Vector[len])` annotation, scalars are labeled `Int64`/`Float64`/`Symbol`/`Char`,
  lambda literals show their real source text, and curried/partial applications (`1+`,
  `f[x;;z]`) render as an explicit **Projection** with a **Blank** node for the omitted slot.
- **Bug fixed: namespaced identifiers misread.** `.ns.sub` printed as `"ns.ns"` (a duplicated
  first segment) because the old code read a Symbol Vector's elements via `_A(v)[i]` (an 8-byte
  `A*`-stride cast) when Symbol Vectors actually store **packed 32-bit ids** -- reading through
  the wrong stride silently pulled back the wrong bytes. Fixed by reading through
  `(const I*)_V(v)` everywhere a Symbol Vector's elements are touched.
- **New: tacit-form annotations.** A 2-verb list `(f g)` is now an explicit **Hook**; a 3-verb
  list `(f g h)` is an explicit **Fork**; both are recognised by checking whether every list
  element is itself verb-like, rather than always rendering as a generic list literal.
- **New palette.** Verbs (bare, applied, curried, or the head of a call) render bold cyan;
  adverbs bold magenta; numeric/literal scalars bright green; variables/symbols yellow; the
  tree's Unicode connectors themselves dim gray. A combined glyph like `+/` is colour-split
  within one label (`+` cyan, `/` magenta) rather than picked as a single node colour.
- **Memory: arena-backed, not malloc/free.** The whole `ASTNode` tree built for one `\ast`
  invocation is now bump-allocated from Amber's real scratch region (`arena_alloc()`/
  `arena_reset()`, `src/arena.h`) instead of `malloc`/`calloc`/`realloc`/`free` -- the same
  region `src/csv.c`'s transient row grid and `src/trace.c`'s own diagnostics already use.
  `ast_free()` is now a documented no-op; `arena_reset()` in `ast_cmd()` does the real cleanup.
  (Amber has no symbol literally named `g_scratch`; `arena_alloc()`/`arena_reset()` *is* its
  scratch-arena API, used directly.)
- **Safety: recursion depth guard.** `ast_from_k()`'s walk now refuses to recurse past 64 levels
  and returns a synthetic "(max depth exceeded)" leaf instead, defense-in-depth alongside pk()'s
  own (tighter, ~60-level) parser nesting limit.
- **Tests:** a new `` `astt`` self-test builtin (wired into `test.k`, mirroring `` `arn``/
  `` `dgn``/`` `simd``/...) plus a new standalone `tests/test_ast.c` harness -- the latter links
  against the full interpreter (ast.c is inherently built on Amber's real parser/value
  representation, unlike `tests/test_simd.c`/`test_parallel.c`, which are
  dependency-free by design) and checks label correctness, a broad no-placeholder regression
  sweep, ANSI colour coverage for all five required categories, and that moderately deep (but
  parser-legal) nesting does not crash.
- **Honest scope note:** `\ast` still never compiles or runs the expression it's shown, with one
  pre-existing, unavoidable exception documented in `ast.h`'s file header -- `pk()` itself
  eagerly compiles lambda *literals* (`{...}`) at parse time (see `p.c`), so a compiled closure
  can appear as a leaf even in an otherwise-uncompiled tree. This module shows that closure's
  captured *source text*, never its bytecode (that remains `\disasm`'s job, `src/vm.{h,c}`).

## Unreleased — fix `peach` worker-count default
- **Bug:** `peach[f;y]`'s default worker count (`src/i.c`'s `peachNW()`) was a hardcoded `4`
  whenever `AMBER_THREADS` was unset, regardless of how many CPUs the host actually had. On a
  small box (e.g. 2 cores) this **oversubscribed** — forking 4 processes onto 2 cores made
  `peach` measurably *slower* than serial `` f'y `` through pure fork/context-switch overhead
  (observed: 0.55x–0.79x "speedup" running `examples/peach.k` in a 2-core sandbox). On a large
  box it left most cores idle.
- **Fix:** `peachNW()` now defaults to the actual online CPU count via
  `sysconf(_SC_NPROCESSORS_ONLN)` (new `peachCPUs()` helper in `src/i.c`), the same approach
  `src/parallel.c`'s `par_thread_count()`/`online_cpus()` already uses for the SIMD/vector
  engine — the two parallel primitives in Amber now agree on what "auto" means.
  `AMBER_THREADS=N` still overrides explicitly and is unchanged. Verified: re-running
  `examples/peach.k` in the same 2-core sandbox went from ~0.55–0.79x ("speedup") to a genuine
  ~1.7x.
- **Cleanup:** removed two stale duplicate source files at the project root (`i.c`, `ar.c`) —
  leftovers from before the `src/` reorganisation, byte-for-byte identical to their `src/`
  counterparts except for the fix above, never compiled by `build.sh` (which only builds
  `src/*.c`), and a source of confusion if edited by mistake.

## Unreleased — engine extensions (SIMD, parallel, disassembler, CSV)
- **SIMD vector kernels.** `src/simd.{h,c}` — AVX2 (x86_64), ARM NEON (`aarch64`, incl. Apple
  Silicon), and scalar C99 fallback implementations of `add`/`mul`/`sum` over `int64_t`/`double`
  arrays, selected at compile time. `src/arena.c`'s bump allocator now guarantees genuine
  32-byte alignment (`posix_memalign`, up from plain `malloc`'s 16-byte guarantee). Self-test +
  benchmark: `` `simd 0``. Build with `AMBER_NATIVE=1 ./build.sh` to activate AVX2 on x86_64
  (NEON activates unconditionally on `aarch64`).
- **Multithreaded vector engine.** `src/parallel.{h,c}` — splits arrays above 100,000 elements
  across POSIX threads (`AMBER_THREADS` env var, same convention as `peach`), each chunk
  processed by the SIMD kernels above. Self-test + benchmark: `` `para 0``.
- **Bytecode disassembler (`\disasm`).** `src/vm.{h,c}` — Amber's compiler/VM already exists in
  `src/b.c` (AST is compiled to opcodes + a constant pool, then run on a real stack VM); this
  mirrors that opcode table byte-for-byte and decodes real compiled bytecode with a
  self-consistency check, rather than adding a second, disconnected VM. Self-test: `` `vmd 0``.
- **Native CSV parser.** `src/csv.{h,c}` — `` `csvr "path.csv"`` parses a file straight into a
  genuine Amber table (`flp(names ! cols)`, the same shape `([]…)` produces) via the arena
  allocator, with per-column type inference (Long/Float/Symbol), quoted-field handling
  (embedded commas, `""`-escaped quotes), and null-mapped empty cells. Self-test: `` `csv0 0``.
- **Test suite:** `test.k` grew from 158 to 161 assertions (one self-test call per module
  above); `tests/test_simd.c` and `tests/test_parallel.c` are new standalone C harnesses (no
  Amber dependency) for the SIMD and parallel modules.

## Unreleased — REPL diagnostics + cleanup
- **`\v` rich workspace inspector.** `src/inspect.{h,c}` — every currently-defined global as an
  ASCII table (Name / Type / Shape·Length / Memory), with a recursive deep-memory-footprint
  walker (`iv_deepsize`) and a structural table-vs-dict classifier (`iv_as_table`) since both
  share the `tM` heap tag in Amber.
- **`\ast` AST visualiser.** `src/ast.{h,c}` — a parse-only, colour-coded tree view of an
  expression (verbs bold cyan, binary ops bold magenta, variables yellow, scalars green, vectors
  cyan, function application bold blue, list literals bold green, block separators dim gray),
  built from the same shape-dispatch `cr()` uses to compile.
- **`\trace` execution profiler.** `src/trace.{h,c}` — 4-phase timing (parse / arena setup /
  execute / format) via `clock_gettime(CLOCK_MONOTONIC)`, a Unicode block-bar report, and the
  arena's peak scratch usage for that one evaluation. Runs the input through the same
  `qsql.k`/`qrw` rewrite the interactive prompt uses first, so tracing a table expression or a
  bare `select … from … where …` query renders and profiles correctly instead of failing to parse.
- **`\timer` removed.** An earlier execution-timer badge (`src/badge.{h,c}`) was tried and then
  explicitly removed at the request of the project owner; `\v`/`\ast`/`\trace` above are its
  replacements for workspace/perf visibility.
- **Cleanup.** Consolidated three copies of a `fmt_bytes()` helper (`inspect.c`, `trace.c`,
  `badge.c`) into `src/fmtutil.{h,c}`; consolidated a duplicated `C_RST` ANSI-reset macro
  (`ast.c`, `diagnostic.c`) into `src/ansi.h`; removed dead code (`diagnostic.c`'s unused `Span
  all`, unused `<string.h>`/`<stdint.h>` includes); fixed two feature-test-macro gaps
  (`_POSIX_C_SOURCE`/`_DEFAULT_SOURCE`) that broke a strict `-std=c99` build of `trace.c`/`m.c`;
  tree compiles warning-clean under `-Wall -Wextra -std=c99` (aside from pre-existing sign-compare
  noise from `a.h`'s `LH`/`TU` macros) and passes the full `test.k` suite (158/158).

## 1.9 — Mac integration + HFT features
- **Native `aj` as-of-join kernel.** `aj`/`aj0` now compute their match indices in C
  (`src/a.c`, `ajc` + `ajlb`) instead of the per-row K `bin`. For each trade the kernel does a
  **branch-free `lower_bound`** (the comparisons lower to `cmov`, no data-dependent branches)
  over the trade's symbol-group slice of the sorted nanosecond timestamp column, returning the
  index of the most-recent quote on-or-before the trade (or null when the group is empty / no
  quote precedes the trade). `amber.k`'s `ajm` marshals `(qt; tt; groupBase; groupEnd)` and calls
  the `` `aj `` builtin; the pure-K reference is retained as `ajmK`. Correct on 64-bit ns
  timestamps, single-group (time-only) joins, absent symbols and before-first-quote rows. New
  tests in `test.k` (`ajNull`, `ajNoGrp`, `ajNs`).
- **HFT zero-allocation arena.** `src/arena.{h,c}` — a **thread-local 16 MB bump allocator**
  (`arena_init` / `arena_alloc` / `arena_reset` / `arena_free`, plus `arena_used`/`arena_capacity`)
  with a leak-free overflow path. Reserved at startup (`kinit`) and rewound once per evaluation
  cycle (`evs`), so the transient buffers produced while evaluating an expression come from a
  single pointer bump instead of `malloc`/`free`, keeping their latency jitter out of the hot
  path. The native `aj` kernel uses it for its transient match vector. Self-test builtin
  `` `arn `` (exercises bump / reset / the >slab overflow path); `test.k` asserts it.
- **Rust-style visual diagnostics.** `src/diagnostic.{h,c}` — a `Span` source-tracking struct
  (`src`, `start`, `end`, 1-based `line`/`col`) and a `report_diagnostic()` renderer that emits a
  Rust-compiler-style report: `error[CODE]: title`, a `-->` file:line:col locator, a gutter-aligned
  source line, per-span `^^^` underlines and a `= help:` note, ANSI-coloured when a colour terminal
  is present. The runtime error path (`src/e.c`, `eS`) — through which all parse / type / domain
  errors funnel — routes them through the formatter **by default**, printing the report above the
  existing compact caret line; set **`AMBER_DIAG=0`** to suppress it and get the terse one-line
  errors only. One `getenv` per error, never on success. The source noun is copied to a bounded,
  NUL-terminated buffer before rendering (Amber char vectors are length-prefixed, not
  NUL-terminated). Self-test builtin `` `dgn ``; `test.k` asserts the rendered report's structure.
- **CRLF-safe REPL loader.** `repl.k` loads its `.k` library line-by-line; a Windows/CRLF
  checkout previously left a trailing `\r` on every line, which broke each definition (so e.g.
  `qsql.k`'s `qrw` never loaded and the REPL failed on the first input). The loader now strips
  `\r` per line (`{. x^(`c$13)}`), so the REPL works regardless of checkout line endings. A new
  **`.gitattributes`** additionally forces LF checkout of `*.k`/`*.c`/`*.h`/`*.sh` so the problem
  cannot recur.
- **Portability.** The tree builds warning-clean under both **gcc and clang** (portable C99 /
  POSIX; thread-local via `__thread`), on x86-64 Linux and Apple-Silicon macOS. Test totals:
  `test.k` 158, `test-fin.k` 35, `test-ext.k` 79 = **272**, 0 failures.

## 1.8
- **Table size footer.** A bare table / keyed table at the prompt now prints a
  `[N rows x M cols]` summary underneath the grid, dimmed. `N` uses thousands separators
  (`[1,000 rows x 4 cols]`). Row/col counts are the *true* totals (`#x` / `#cols x`), computed
  before the `CROWS` grid truncation, so a previewed million-row table still reports its full
  size. Implemented in `repl.k` (`fmt`/`ftr`, `comma`).
- **Error ergonomics.** The terse one-word error is expanded to descriptive text
  (`'length: operands have mismatched counts`, `'type: wrong type for this operation`, …) via a
  lookup over the core's 12 error kinds, while the REPL still prints the offending input line with
  the `^` caret under the failing operator/verb (the caret comes from the C core's `err`/`eQ`
  machinery). Implemented as a repl-local handler `onerr`/`edesc` in `repl.k`.
- **ANSI syntax highlighting.** Grid output (tables, keyed tables, dictionaries) is coloured for
  dark terminals with a vivid **256-colour, 14-hue per-column palette** (`PAL`) — each column a
  distinct colour, headers bold white, nulls and the size footer dimmed; dictionary values keep a
  per-type tint. Colour is applied *after* width padding (ANSI is zero visual width), and a new
  `vlen`/`vstrip` strips ANSI before every column-width and header-underline computation, so
  alignment is exact. `COLOR:0` disables it globally. Implemented in `amber.k`
  (`amblk`/`amtab`/`amkeyed`/`amdict` + palette `CT`/`cwrap`/`ccol`/`ccell`); reached from the
  REPL through the existing `amfmt` call, so no cross-namespace globals are introduced.
- **Unicode table borders (`\grid`).** `\grid clean|rounded|sharp|heavy` toggles the table frame:
  `clean` (default) is the minimal dashed header rule; `rounded`/`sharp`/`heavy` draw a full box
  with curved / square / thick corners, `│`·`┃` column dividers and proper `┬┼┤`-style
  intersections. Borders are dimmed grey so colourised cells stay the focus; column widths are
  measured with `vlen` (ANSI stripped) and box segments repeat the multi-byte glyph per *character*
  (not per byte), so everything lines up around coloured data. Big tables truncate after `CROWS`
  rows with a dotted row inside the frame and the `[N rows x M cols]` footer below it. New `\clear`
  command clears the terminal (scrollback + screen). Border set in the root global `GRID`.
- **Grid polish.** Numeric and temporal columns are **right-aligned** (symbols, chars and strings
  stay left-aligned); the header underline is dimmed grey so data rows stand out; null sentinels
  (`0N`/`0n`/`0w`/blank) render faint grey. Float precision in grids is capped by a new **`PREC`**
  global (default 7 decimals; `PREC:0N` restores full precision), so a column of 14-decimal floats
  no longer sprawls. All applied after width padding, with `vlen` keeping alignment exact.

## 1.7
- **Native temporal types.** `date` (days since 2000.01.01), `time` (ms of day) and
  `timestamp` (ns since 2000.01.01) are now first-class C-level types with their own type
  tags. Literal syntax parses directly — `2026.07.30`, `10:00:05.000`,
  `2026.07.30D09:30:00.000000000` — and values auto-display in their own format.
  Type-aware arithmetic: `10:00:00.000+00:00:05.000` → `10:00:05.000`, `date-date` → days,
  `date+n` → date, plus comparisons. String casts `"D"$`/`"T"$`/`"P"$`, accessors
  `year`/`month`/`day`/`dow`/`thh`/`tmm`/`tss`, and `` `i$`` to extract the raw value.
  Columns keep numeric storage (as kdb does) so `xasc`/`s#` and name-based `time`-column
  display continue to work. Implemented across `a.h` (enum + type tables + `TU`/`TP` macros),
  `p.c` (literal scanner), `s.c` (formatter), `2.c` (arithmetic), `c.c` (casts).
- **C-kernel window join.** `wj` moved from an interpreted per-row K loop to a C routine
  (`wjc` in `i.c`): a vectorised binary-search range probe per row plus a contiguous slice
  sweep for the standard reducers (`first last min max sum avg count`). ~2× faster in a
  throttled sandbox (more on real cores), bit-exact vs the interpreted `wjK` fallback for the
  non-floating reducers. Arbitrary aggregators fall back to `wjK`.
- **C-kernel `ema`** — the exponential moving average was an interpreted per-element scan;
  now a single O(n) C sweep (`emaC`), verified identical to the K version.
- **SIMD math.** Portable vectorization pragmas (`clang loop vectorize` / `GCC ivdep`) on the
  hot elementwise float kernels in `2.c`.
- **Terminal charting.** `plot` draws a numeric vector as a 2×4 **Braille** line chart
  (Bresenham into a bitmask canvas, min/max-scaled with Y-axis labels); `candle` draws an
  OHLC table as **Unicode candlesticks** with ANSI colour (green up / red down), box-drawing
  wicks and block bodies, one space between candles. Exposed in `sys.k`; C kernels
  `plotC`/`candleC` in `i.c`. See `examples/graphs.k` for a 13-chart tour.
- **Apache Arrow C Data Interface (zero dependency).** `arrow.export` / `arrow.import`
  interop with PyArrow / Polars / DuckDB over the stable Arrow C ABI with no `libarrow`
  linkage. `ArrowSchema`/`ArrowArray` structs are embedded in `a.h`; export is zero-copy
  (child `buffers[1]` alias Amber column payloads and a `release` callback `mr`s them);
  import parses format strings, applies validity bitmaps as nulls, and rebuilds a table
  (a copy — Amber's inline object header precludes aliasing a foreign buffer). New file
  `ar.c`. Numeric widths, ranges and symbol (utf8) columns round-trip exactly.
- **Cleanup.** Removed the dead duplicate `gentq`; interpreted `wj` retained as `wjK`
  (arbitrary-aggregator fallback). Test suite now **267** (153 + 35 + 79), 0 failures.

## 1.6
- **Q-style grid preview.** `show t` (and a bare table/keyed-table/dict at the prompt) now
  prints only the first `CROWS` rows (default 20) followed by a `..` line, instead of dumping
  the whole thing. The cap is applied *before* formatting, so previewing a million-row table
  is instant. Set `CROWS:n` at the prompt to change it (e.g. `CROWS:10`); small tables print
  in full. Implemented in `amtab`/`amkeyed`/`amdict` (amber.k).
- **`examples/peach.k`** — a runnable Monte-Carlo demo that times serial `` f'y `` vs
  `peach[f;y]` so you can see the multi-core speedup on your own hardware
  (`AMBER_THREADS=8 ./amber examples/peach.k`).
- **Banner shows v1.6** and advertises bare qSQL + parallel `peach`.
- **`peach` is now genuinely parallel (multi-core), in C.** `peach[f;y]` forks
  `AMBER_THREADS` worker processes (default 4), each applies `f` to a slice of `y`,
  serialises its result (`` `k ``) down a pipe, and the parent deserialises (`val`) and
  concatenates. Because `(. `k v) ~ v` holds for every value, the result is identical to
  serial `` f'y `` — verified across vectors, symbols, tables, nested and ragged results, and
  a 200-iteration stress run; all 226 tests still pass. Set `AMBER_THREADS=N` to control the
  worker count (`=1` forces serial). Implemented as a new C primitive (`peachC` in `i.c`,
  wired through the `sym1` system-function table) — additive, so it can't affect the serial
  core. Fork-based (copy-on-write heap) so there are **no data races**: this matches how
  kdb+ gets multi-core and avoids the atomic-refcount tax that shared-memory threading would
  put on all single-threaded code. Unlike Python threads (GIL-bound), Amber's workers run on
  all cores at once. Best for coarse-grained, compute-heavy per-item work; see BENCHMARKS.md
  for when the fork/serialise overhead makes serial `'` the better choice.

## 1.5
- **The v1.5 extended modules now actually load in the REPL.** `repl.k` previously loaded only
  `amber.k` + `fin.k`, so `std`/`qsql`/`temporal`/`sys`/`hdb`/`ipc` — and therefore `sel`,
  `select … by …`, the moving aggregates, temporal helpers, etc. — were silently unavailable at
  the prompt. `repl.k` now loads all six in dependency order.
- **Bare qSQL.** You can type `select … by … from … where …`, `exec`, `update`, and `delete`
  directly — no `sel"…"` wrapper. A reader rewriter (`qrw` in `qsql.k`) turns such a line into
  the matching `sel/exq/upd/del "…"` call before evaluation; it also handles assignment
  (`r:select …`) and prefixes (`5#select …`, `count select …`), and leaves ordinary
  expressions and the explicit string forms untouched. Documented on the `\j` help page and in
  AMBER.md §7.
- **`select from t` fix.** `sel` now parses a column-less `select from t [where …]` (a leading
  `from`, all columns) correctly instead of mis-splitting the clause.
- **Benchmarks + sanity harness.** New `BENCHMARKS.md` and `bench/` (Amber vs numpy/pandas here;
  auto-runs growler/k, kdb+/q, DuckDB, Polars where installed — `bench/run.sh`). All aggregation
  results cross-check exactly against numpy/pandas; Amber's `group-by` and `distinct` beat pandas.
- **Faster as-of join.** `aj`'s matcher (`ajm`) now issues **one vectorised `bin` per group**
  (`'` on a sorted noun) instead of a per-row scalar search — ~6× faster (≈700→115 ms at 50 k
  rows), identical results, all 226 tests still pass. (`wj` still uses the per-row form.)
- **`build.sh` defaults to portable `-O3 -flto`** (was `-O2`). Link-time optimisation is
  auto-detected with a safe fallback if the compiler lacks it; `filter` ~2× faster and the
  hash/group kernels ~20% faster, with no `-march` so the binary stays portable.
  `AMBER_NATIVE=1 ./a` opts into `-march=native -funroll-loops` (group-by roughly halves again).
  PGO was tested and dropped — the gain was noisy and not worth the two-pass build. See
  BENCHMARKS.md.

## 1.4.1
- **`gentq` now marks both key columns**: `time` gets the `` `s`` sorted attribute and `sym`
  gets `` `p`` parted — both visible in `meta trades` / `meta quotes` (via `fin.k`'s
  `sortcol`/`partcol` helpers). `test-fin.k` asserts both (now 35 tests).
- **Help expanded**: the `\z` page documents all four attributes (`` `sa`` `` `ua`` `` `pa``
  `` `ga`` + `` `at``, with complexity), and a new **`\m` page** documents the whole `fin.k`
  HFT vocabulary. Added to the main `\` menu.

## 1.4
- **`fin.k` — a financial / HFT module** (auto-loaded after `amber.k`): a fixed `gentq[n]`
  that sets global `trades`/`quotes` tables with **numeric times** and attributes on the key
  columns (`` `s`` on `time`, `` `p`` on `sym`); an O(1) grouped index (`bysym`/`symrows`);
  order-book analytics (`mid` `spread`
  `spreadbps` `micro` `imbal`); trade analytics (`vwap` `twap` `tsign` `signedvol`
  `effspread` `notional`); returns/vol (`ret` `logret` `rvol` `movavg` `movsum` `movmax`
  `movmin` `ema` `rollstd`); `bars` (OHLCV) and `symstats`; and `pt` (time-formatted print).
- **All four kdb attributes now exist in C**: `` `sa`` (sorted), `` `ua`` (unique),
  `` `pa`` (parted), `` `ga`` (grouped); `` `at`` reports `s`/`u`/`p`/`g`; `meta` shows them.
  Sorted **and parted** columns get O(log n) kernel find; grouped + the group index give O(1)
  per-symbol slicing (see `bench-fin.k`: ~20,000x vs a linear scan).
- **Join fix**: joins require **numeric** time columns — store times as ms and format only for
  display (`tsym`/`pt`). `stime` on the stored column breaks `aj` (it makes time a string).
- New: `examples/hft.k` (full HFT walkthrough), `examples/attributes.k`,
  `examples/practice.k`, `test-fin.k` (32 tests), `bench-fin.k`. Core suite: 153 tests.


## 1.3
- **Fixed table/grid rendering** used by both `show` and the bare-expression REPL:
  `amdict` now handles list-valued dicts (e.g. `group`), `amcells` renders nested columns
  (e.g. `xgroup`), and `iskeyed` no longer crashes on plain vectors (it used odometer on
  non-dicts). `all`/`any` no longer rely on the unsupported `` `b$`` cast.
- **`./a` rebuilds when the C sources are newer than the binary** — no more running a stale
  build (this was why `([]…)` tables didn't render for some copies).
- **New banner** with a techy subtitle; **`examples/tour.k`** shows a worked, tested example of
  every function; test suite grown to **148 assertions** covering essentially the whole library.
- Rewrote the GitHub README.


## 1.2
- **`([]col:vals;…)` table literals** and **`([key:vals]col:vals)` keyed-table literals**,
  implemented in the C parser (`p.c`) — build tables the q way, no `+…!(…)` needed.
- **Own identity**: every reference to the upstream array core has been removed from the code,
  banner, help and docs; the language stands alone as Amber. (AGPLv3 attribution is retained in
  `NOTICE`, as the licence requires.)
- **New banner** — a clean wordmark, no clutter.
- **`bench.k`** — attribute speed harness (find/`in`, sorted vs unsorted, across sizes).
- **`MISSING.md`** — an honest map of kdb+/q features not yet in Amber, with a roadmap.
- **`round[d;x]`**, table-literal tests; suite now 104 assertions.


## 1.1
- **Temporal / tick support**: `hms hh mm sec milli minute second stime ptime` (time-of-day
  as ms since midnight, mirroring q's `time.hh` / `time.minute` accessors), plus
  `minbar` / `bar` / `xbar` for tick.minute-style OHLC bucketing.
- **`meta` shows attributes**: now a keyed table with `c` (column), `t` (type) and
  `a` (attribute) — so a sorted column shows `a: s`.
- **Grid rendering**: `show` / auto-print renders tables, keyed tables and dicts as clean
  q-style grids; `tsym[t;c]` formats time columns as `HH:MM:SS.mmm`.
- **Restructured help**: `\q` (scalars/agg/sets/strings), `\j` (tables/joins/qSQL),
  `\z` (temporal/attributes/display), plus a reorganised main `\` menu.
- **Examples**: `examples/basics.k` and `examples/tick.k` (realistic trades & quotes,
  as-of and window joins, VWAP, 1-minute OHLCV bars).
- **`round[d;x]`** helper.
- **CI**: GitHub Actions builds and runs the test suite (now 97 assertions) on every push.

## 1.0
- Amber: a low-latency array language with a q/kdb+ vocabulary. Aggregations, dictionaries, tables, keyed tables,
  joins (`lj ij uj pj ej aj aj0 wj asof`), qSQL (`qwhere qselect qby fby xgroup ungroup`),
  strings, and **C-level sorted attributes** that turn `?`/`in` into O(log n) binary search.
