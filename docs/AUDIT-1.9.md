# Amber 1.9 — Security, Correctness, Hygiene & Documentation Audit

> **Historical record.** This report documents the audit of release **1.9** and is left as
> written; the version strings and figures below are the ones 1.9 produced. The current release
> is **1.9.1** — on this checkout the reproduce steps below print `amber 1.9.1`, and the qSQL and
> CBQN benchmark changes are described in [CHANGELOG.md](CHANGELOG.md).

**Repository:** https://github.com/BonucciAndrea/amber
**Scope:** every `.c`, `.h`, `.k`, doc and test in the tree (≈6 200 lines of C plus the K library).
**Method:** full read of the core (`a.h`, `a.c`, `m.c`, `arena.*`, `ast.c`, `trace.c`, `p.c`, `o.c`,
`v.c`, `2.c`, `3.c`, `1.c`, `i.c`, `b.c`, `h.c`, `w.c`, `s.c`, `0.c`), plus three mechanical sweeps:
`-Wall -Wextra` warning triage, an AddressSanitizer + UndefinedBehaviorSanitizer build run over
every suite and example, and a purpose-built malformed-input fuzzer (`tests/fuzz.py`).

**Result:** 26 defects fixed, 6 dead/duplicated code items removed, version unified at 1.9,
403 new test assertions added on top of the existing 198, and every suite passes clean under
ASan + UBSan with no leaks and no crashes across 1 629 fuzz cases. The suites are
mutation-checked (§5.1): deliberately reintroducing each fixed bug turns them red.

---

## 1. Baseline

| | before | after |
|---|---|---|
| `./amber test.k` | 163 pass | 163 pass |
| `./amber test-fin.k` | 35 pass | 35 pass |
| `tests/test_matrix.k` | — | **309 pass** (new) |
| `tests/test_qsql.k` | — | **94 pass** (new) |
| `tests/test_simd.c` / `test_parallel.c` | not wired into any runner | pass, wired |
| `tests/test_ast.c` | did not link as documented | pass, wired |
| UBSan diagnostics on the suites + examples | **27 distinct sites** | 0 |
| ASan diagnostics | 1 dynamic-stack-buffer-overflow (6 files) | 0 |
| Fuzz cases (malformed / deep-nested input) | — | 1 629, **0 crashes, 0 hangs** |

---

## 2. Phase 1 — version 1.9 and documentation

Before this audit the version existed in exactly one place — a **string literal in `repl.k`'s
banner that said `v1.7`** — while `README.md` already advertised 1.9. There was no `VERSION`
macro, no `--version` flag, and no CMake/Makefile (the build is `build.sh`).

| file | change |
|---|---|
| `src/a.h` | **new** `AMBER_VERSION_MAJOR/MINOR/AMBER_VERSION` — the single source of truth |
| `src/m.c` | `binfo` (`` `bi 0``) returns the version as a 5th element |
| `src/0.c` | **new** `--version`/`-v` and `--help`/`-h`; `main()` previously treated `argv[1]` unconditionally as a script path, so `amber --version` failed with `'io` |
| `src/amber_wasm.c` | **new** exported `amber_version()` so the browser build reads the same constant |
| `repl.k` | banner reads the version from `binfo` instead of the hard-coded `v1.7` |
| `README.md` | `--version`/`--help` documented; `\trace` section rewritten for the real output; test-suite inventory rewritten |
| `docs/CHANGELOG.md` | full `## 1.9` section |
| `docs/MISSING.md` | **new** "Known leniencies" section (see §6) |
| `.github/workflows/ci.yml` | runs `tests/run_tests.sh` on Linux, macOS and Windows/WSL |

Documentation claims were checked against the implementation. Corrections made:

- `\trace`'s documented "Arena peak" output was `0 B` in every README example **because it was
  always 0** — a bug, not a doc error (§4).
- The README's `\trace` timings showed `11us`/`5us`/`4us`; the formatter could not represent
  sub-microsecond phases at all and printed `0us` (§4).
- Attribute syntax: several doc passages write kdb's `` `s#`` informally. Amber's actual syntax is
  `` `sa``/`` `ua``/`` `pa``/`` `ga`` to set and `` `at`` to read; recorded in `docs/MISSING.md`
  and pinned by `tests/test_matrix.k`.

---

## 3. Phase 2 — dead code and duplication removed

| item | file | note |
|---|---|---|
| `minfB()` | `src/3.c` | unused static, deleted |
| `maxfB()` | `src/3.c` | unused static, deleted |
| `eqlsL()` | `src/3.c` | unused static, deleted |
| unused local `m` | `src/a.c` (`i1`, tL index path) | deleted |
| write-only array `p[r]` | `src/h.c` (`rsh`) | the loop only ever needed the running product `m` |
| unused local `u` | `src/w.c` (`wf`) | deleted |
| discarded value `f>2&&close(f)` | `src/i.c` (`fw`) | rewritten as a statement |
| **`ajlb()` + `wjlb()`** | `src/a.c`, `src/i.c` | byte-identical lower-bound contract under two names, one branch-free and one branchy. Consolidated into the exported `amlb()`; both join kernels now get the cmov version |

`fmt_bytes()` was already de-duplicated into `src/fmtutil.{h,c}` by an earlier release; no other
duplicated implementations were found. `src/g.h` is machine-generated (by `g.k`) and was left
alone deliberately — its unused accessor macros are the generator's output, not hand-written dead
code.

---

## 4. Phase 3/4 — defects found and fixed

### 4.1 Memory safety

**A1 — out-of-bounds stack read in `a8()` (amend).** `src/a.c`
The five `MC()` calls in `a8()` copied a fixed eight-slot's worth of argument pointers
(40/48/56/64 bytes) out of the caller's `a[]` regardless of how many arguments were actually
passed. `a` is not always an eight-element buffer — `run()` (`src/b.c`) hands over a pointer
straight into its own dynamically sized stack frame — so **every amend with `n<8` read past the
end of live storage.** Reproduced by AddressSanitizer as *dynamic-stack-buffer-overflow* on
`test-fin.k` and four `examples/*.k`. The copied slots were never *used* (each recursive call is
bounded by `n`), so this was a pure read overrun — but it is still one, and on a stack-probing or
tagged-memory target it faults. Fixed with an `AC()` clamp to the real argument count.

**A2 — `arena_alloc()` bounds check could wrap.** `src/arena.c`
`off + bytes <= a_cap` overflows for a large `bytes` on a 32-bit target (wasm32, where `size_t`
is 32 bits and an element count × element width can exceed it), silently passing the check and
returning a short block that the caller then runs off the end of. Rewritten as
`bytes <= a_cap - off`, which cannot wrap; the overflow-block path got the same treatment; and
`ajc()` now rejects an element count whose byte size would overflow `size_t` before allocating.

**A3 — use-after-reset in the `` `simd`` self-test.** `src/a.c`
`arena_reset()` was called immediately after `arena_alloc()` and **before** the 400 009-element
reference loop that writes into the block, so every store landed in scratch the allocator had
already rewound and was free to hand out again. Only harmless because nothing else allocated in
between. The reset now follows the loop.

**A4 — `arena_init()` leaked tracked overflow blocks.** `src/arena.c`
On the first-reservation path it set `a_over = 0` without freeing the list. Reachable whenever a
slab reservation fails: `a_base` stays 0, so every subsequent `arena_alloc()` re-enters
`arena_init()` and drops the accumulated overflow blocks on the floor. Now calls `arena_reset()`.

**A5 — `arena_used()` under-reported.** `src/arena.c`
It returned only the slab bump cursor and ignored live overflow blocks, contradicting its own
header contract ("bytes currently handed out"). Overflow blocks now carry their size and are
counted.

**A6 — `ast_new()` / `ast_add_child()` dereferenced a possibly-NULL `arena_alloc()`.** `src/ast.c`
`arena.h` documents that `arena_alloc()` can return NULL. `memset(n, 0, sizeof *n)` ran
unconditionally. Returning NULL would only move the segfault (all ~30 call sites dereference the
result immediately), so `ast_new()` now degrades to a static sentinel node and the tree stays
walkable; `ast_add_child()` NULL-checks its grown array.

**A7 — `memcpy(dst, NULL, 0)` in `run()`.** `src/b.c` — undefined per `memcpy`'s `nonnull`
attribute, flagged by UBSan on every startup. Guarded.

**A8 — `mmap()` failure test cast a pointer to `char`.** `src/m.c`: `(L)p == (C)p`. Works by
accident (only `MAP_FAILED` and NULL fit in a signed char) but is a `-Wpointer-to-int-cast`
warning and a portability trap. Replaced with `p == MAP_FAILED`.

Reference counting in `a.c`/`m.c` (`_R`/`m0`/`mr`/`mut`/`kv`/`aa`) was read in full and
cross-checked with the built-in `\m` heap consistency checker (`bsm`), which walks every live
region and reports refcount, type, protection and dangling-pointer anomalies. It reports clean
after every suite. No refcount leak, double decrement or premature free was found.

### 4.2 Undefined behaviour (all found by UBSan; none change observable behaviour)

**B1 — `NL`, the long null, was UB.** `src/a.h`: `#define NL (1ll<<63)` shifts into the sign bit
of a signed type. UBSan flagged it from **17 distinct sites** across `a.c`, `h.c`, `p.c`, `r.c`,
`f.c`, `s.c`, `c.c`, `i.c`, `3.c` and `csv.c`. Now `((L)(1ull<<63))`.

**B2 — remaining signed-shift sites**: `1.c` (`neg`'s narrow-type minimum probe), `2.c` (`ozZ`'s
lane broadcast), `3.c` (`mmmf`, `mxms`), `p.c` (`pf`'s sign bit), `m.c` (`su()`'s `1<<31` in
`int`). All rewritten through the unsigned counterpart type.

**B3 — intentional wrap in the arithmetic kernels was UB.** `src/2.c`, `src/v.c`
`aLL/aII/aHH/aGG`, `alL/aiI/ahH/agG`, the widest-width multiplies, and `tilV()`'s packed-lane
counter all rely on two's-complement wraparound, which `oZZ()`/`ozZ()` then detect after the fact
from the sign bits. Signed overflow being UB, **the optimiser is entitled to assume it cannot
happen and delete the very overflow check that depends on it** — a classic latent miscompile.
All rewritten in the unsigned counterpart type: identical instructions, defined semantics.

**B4 — negation and subtraction of `LLONG_MIN`.** `src/1.c` (`neg`, `flr`), `src/v.c` (`til`),
`src/2.c` (`arizz`'s modulo path). Reachable straight from user input (`-0N`, `_ -0w`, `2!-0w`).
All done through `W` now; outputs verified byte-identical to the pre-audit build.

**B5 — the numeric-literal parser overflowed on out-of-range input.** `src/p.c`
`pu`/`pl`/`pfu`/`pTmp` accumulated in signed `L`, so `1000000000000000000000`, a huge `HH:MM`
field, or a malformed exponent (`1.5e`, `1.5each`) was signed overflow — **undefined behaviour
reachable directly from a REPL line.** Found by the fuzzer under UBSan. All accumulators are now
unsigned, and `pfu` normalises a no-digit mantissa to 0 and clamps the exponent in `L` before
narrowing it to `int`. Observable results are unchanged (verified against a pristine HEAD build).

### 4.3 Language semantics

**C1 — a computed float null never matched the `0n` literal.** `src/o.c`, `src/s.c`
IEEE 754 has no single NaN bit pattern: `0n` is the positive quiet NaN (`0x7ff8…`), but every NaN
the FPU *computes* — `0%0`, `0w-0w`, `avg 0#0`, `dev 0#0` — is the default NaN, which on x86-64
has the sign bit set (`0xfff8…`). `mtc_()` compared bytes, so:

```
(0%0)~0n   ->  0b        / but 0n~0n -> 1b, and ^0%0 -> 1b, and both print "0n"
```

There was no way to write a working equality test against a computed float null. `mtc_()` now
compares float payloads value-wise with all NaNs treated as one value — only on the path where
the byte-wise fast path has already failed, so equal data still costs exactly one `memcmp`.
Separately, `sf()` no longer prints a leading `-` for NaN (a NaN has no meaningful sign), so
`0%0` and `(-0w)+0w` render as `0n` instead of `-0n`.

**C2 — `deltas` and `ratios` raised `'length` on an empty argument.** `amber.k`
`0,-1_()` is a one-element vector, so `x-0,-1_x` mismatched. Both now return the empty vector,
as q does.

**C3 — `med` of an empty vector returned `0.0`.** `amber.k` — while `avg`, `dev`, `var` and
`sdev` all return the float null. Now returns `0n`, consistent with the rest of the family.

**C4 — `qselect` was broken for more than one aggregation.** `amber.k`
`select a:sum px, b:max sz from t` built the dict `` `a`b!(210.0;6)`` and flipping it raised
`'length`. The single-aggregate case only *appeared* to work because the each-result happened to
be a one-item list whose one item was the atom. Each aggregate result is now joined onto `()`, so
an atom becomes a one-row column and genuine vectors pass through untouched.

**C5 — `select by sym from t` raised `'value`.** `qsql.k`
Two bugs stacked. The `" by "` split never matched, because when the by-clause is the whole
select-part it has no space in front of it to match against; and even once split, an empty
projection ignored the grouping entirely and returned the ungrouped table. `qsplit0()` handles the
position-0 case, and an empty projection now aggregates every non-key column with `last`,
matching q.

**C6 — `sum`/`prd` of an empty float vector returned an integer.** `amber.k`
The wrappers short-circuit an empty argument to the literal `0`/`1`, discarding the float type
that the underlying reductions get right (`+/0#0.0` is `0.0`, but `sum 0#0.0` was `0`). Surfaced
by the qSQL suite as `select t:sum px from none` producing an int column. Both wrappers now
return `0.0`/`1.0` for a float argument.

**C7 — a caught error still splashed a full diagnostic across stderr.** `src/e.c`, `src/a.c`
Amber renders its Rust-style report at error **creation** time, so `.[f;args;handler]` — and
`protect`, documented as `.Q.trp`-like — had already printed the whole thing by the time the
handler ran. Correct for an interactive line, wrong for anything that provokes errors on purpose:
the new test suites' ~35 must-raise cases produced ~35 full-colour error blocks on a run that
passed cleanly, which made a working suite look catastrophically broken. Rather than defer
rendering (which would change what an interactive line prints), 1.9 adds a runtime switch:
`` `diag 0`` suppresses the report and returns the previous setting, `` `diag 1`` restores it.
The compact caret text is untouched — still buffered, still handed to the handler and to
`` `err``. `tests/harness.k` uses it, so the suites are silent on success and still report the
error kind on any unexpected FAIL.

### 4.4 System diagnostics (`\trace`, `\ast`)

**D1 — "Arena peak" was always `0 B`.** `src/trace.c`, `src/arena.{c,h}`
It reported `max(used-before, used-after)`. `used-before` is sampled immediately after an
`arena_reset()`, so it is 0 by construction; and every arena consumer (`ajc`, `wjc`, `csv_read`,
`ast_cmd`) rewinds the slab before returning, so `used-after` is 0 too. **Every `\trace` in the
README therefore shows `Arena peak: 0 B`, including the ones meant to demonstrate the feature.**
`arena.c` now maintains a true high-water mark — `arena_peak()` / `arena_reset_peak()` — that
deliberately survives `arena_reset()`, which is the only way a consumer sampling *after* the
evaluation can see anything. `\trace aj[…]` now reports a real figure.

**D2 — timings below one microsecond rendered as a flat `0us`.** `src/trace.c` — the formatter
had only `ms` and `us` branches with integer division. It now prints `ns`/`us`/`ms`, so the
microsecond accuracy the command advertises is actually visible (a parse phase reads `454ns`, not
`0us`).

**D3 — the report box was not square.** The bar glyph `■` is three bytes but one column wide, and
the `printf` field widths counted bytes, leaving the right-hand border ragged. Rows are now
composed to an explicit printable-column width.

**D4 — the `` `arn`` arena self-test never reached the overflow path it advertises.** `src/a.c`
It called `arena_init(1<<16)` then asked for 1 MB — but `arena_init` with a capacity that fits
the existing slab only *rewinds* it, it never shrinks it, so the request was served from the
≥16 MB slab and the overflow branch was never taken. It now requests
`arena_capacity() + 64 KB` and additionally asserts that the peak survives a rewind.

**D5 — counters reset correctly across consecutive REPL executions.** Verified: `arena_reset()`
zeroes both the bump cursor and the overflow accounting; `arena_reset_peak()` re-arms the
high-water gauge per `\trace`; `evs()` rewinds the arena at the end of every eval cycle. `\trace`
also leaks nothing on its compile-error path any more (it returned without rewinding).

### 4.5 Parser and AST robustness

`tests/fuzz.py` drives the parser with unbalanced brackets, dangling adverbs and modifiers,
missing operands, truncated qSQL clauses, hostile `\ast`/`\trace`/`\disasm` arguments and nesting
20 000 levels deep. Under ASan + UBSan: **1 629 cases, 0 crashes, 0 hangs.** Two real defects
came out of it (B5 and B4) and are fixed. Operator precedence, adverb chaining and the qSQL
grammar are covered by assertion in `tests/test_matrix.k` and `tests/test_qsql.k`.

---

## 5. Phase 5 — new test suite

| file | what it does |
|---|---|
| `tests/harness.k` | shared assertion harness: `t` (value), `tv` (expression evaluated **inside** the trap), `te` (must raise), `tk` (must not raise), `hreport`. Every assertion is trapped, so a failing or throwing case can never abort a suite or hide the cases after it |
| `tests/test_matrix.k` | **309 cases.** Every primitive against every element type (Long, Float, Boolean, Char, Symbol, nested list, dict, table) at sizes 0, 1, 10 and 100 000+, including 99 999 / 100 000 / 100 001 / 250 000 to straddle `PAR_THRESHOLD`. Eleven groups: shape invariants, algebraic identities, vector-kernel-vs-scalar-reference agreement, empty/singleton, special values (`0N` `0n` `0w` `-0w` NaN), sort/distinct/group/index, take/drop/slice, type mismatches that must raise, dictionaries, nested lists, compound pipelines |
| `tests/test_qsql.k` | **94 cases,** written in the bare `select … from t` syntax. The full `select`/`exec`/`update`/`delete` clause lattice, multi-key `by`, aggregations over empty / single-row / heavily-duplicated (1 000-row, 2-key) tables, the functional `qexec` form, the bare-qSQL rewriter `qrw`, and 17 malformed queries asserted to raise cleanly |
| `tests/fuzz.py` | malformed-input and deep-nesting crash fuzzer; runs each child in a throwaway directory so it cannot litter the tree |
| `tests/run_tests.sh` | one entry point: build → all K suites → the three C unit tests → fuzz. `--asan` rebuilds under ASan + UBSan and re-runs everything |

### 5.1 Suite integrity — three defects in the tests themselves

The first cut of these suites had four problems that made them unreliable, all now fixed:

1. **They only ran from the repo root.** `\l amber.k` resolves against the *current working
   directory*, so `amber /path/to/tests/test_matrix.k` died with a bare `'io`. Each suite now
   derives the repo root from its own script path (`` `argv 1``), the same trick `repl.k` uses.
2. **42 of `test_qsql.k`'s 93 cases silently never ran — and it printed "ALL TESTS PASSED".**
   In K, `f [a;b]` **with a space** is not a call: the parser reads `[a;b]` as a bracketed
   statement block and quietly builds a projection that is then thrown away. No error, no
   output. The suites now call `hexpect[n]` to assert exactly how many assertions were recorded,
   which catches any silently skipped block regardless of cause.
3. **The run looked catastrophic even when it passed.** ~35 deliberate must-raise cases each
   printed a full Rust-style diagnostic to stderr, because Amber renders at error-creation time
   (C7). Fixed at the engine level with the new `` `diag`` switch, which `tests/harness.k` now
   uses; the suites print their summary and nothing else.
4. **One throwing expression aborted the whole file.** `t[tag; got; want]` evaluates `got` in the
   caller, outside the trap — so a genuinely broken engine made the suite print *no report at
   all*. All 98 standalone cases were converted to `tv[tag;"expr";want]`, which evaluates inside
   the trap; suites also `exit 1` on failure so status can be gated on.

**Mutation check.** To prove the suites are not vacuous, each fixed defect was reintroduced and
the suites re-run:

| mutation | detected |
|---|---|
| revert `deltas` empty guard (`amber.k`) | `emptyDeltas` FAIL |
| revert `med` empty → `0.0` (`amber.k`) | `emptyMed` FAIL |
| revert `sum` float-empty type fix (`amber.k`) | `emptyAggVal` FAIL |
| revert `qselect` multi-aggregate fix (`amber.k`) | `selTwoAgg` FAIL |
| revert `select by sym from t` fix (`qsql.k`) | `selByNoProj`, `selByCount`, `byOnly` FAIL |
| revert NaN-aware match (`src/o.c`) | `emptyAvg`, `nanFromInf`, `zeroOverZero` FAIL — and the message reads `got<0n> want<0n>`, which is precisely the bug |
| off-by-one in the i64 add kernel (`src/2.c`) | 13 FAIL across sizes 1, 10 and 100 000 (all three code paths) |
| reintroduce the `f [a;b]` space bug in one call | `assertionCount` FAIL |

Failure output stays diagnosable despite the quiet mode — e.g. reverting the `deltas` fix gives:

```
309 tests run, 1 failures
  emptyDeltas: FAIL raised: 'length  <-  deltas 0#0
```

**Design note.** The matrix asserts *invariants* — shape and type preservation, algebraic
identities, agreement between the vectorised C kernel and a scalar K reference, and stability of
the answer across the sizes that switch Amber between its scalar, SIMD and multi-threaded paths —
rather than thousands of frozen literals. Frozen literals rot and mostly re-test the formatter;
invariants keep testing the engine.

The three pre-existing C harnesses were not wired into any runner and `tests/test_ast.c` **did not
link as its own header described** (`src/0.c` owns `main()`, but it also owns `pg` and the
non-wasm `js_eval()` stub, so it cannot simply be excluded). It is now built with `-Dldstatic` —
the guard `0.c` already wraps `main()` in — and all three pass.

---

## 6. Known leniencies (recorded, not changed)

Pinned in `tests/test_qsql.k` / `tests/test_matrix.k` with `tk[...]` so a behaviour change shows
up as a test failure, and written up in `docs/MISSING.md`:

1. **An unknown `by` key does not raise** — `select t:sum px by nosuchkey from t` groups by nulls.
   Root cause is core dict semantics (`` b#+t`` on a missing key yields nulls), so a fix belongs
   in `sel`/`qbc` validation, not in `#`.
2. ~~A trapped error still renders a diagnostic to stderr.~~ **Fixed** — see C7: the new
   `` `diag`` runtime switch. Left in this list only to note that it is no longer a limitation.
3. **No long-typed infinity literal** (`0W`/`-0W` do not parse; `0w`/`-0w` are float-only).
4. **`5#0#0` widens narrow-int nulls to long nulls** — `cn[tG]` aliases the long null.
5. **Attribute syntax is `` `sa``/`` `ua``/`` `pa``/`` `ga`` + `` `at``**, not kdb's `` `s#`` etc.
6. **A bare `/` alone on a line opens a block comment** that runs to the next line starting with
   `\`, silently truncating the rest of the file with no error. Standard K, previously recorded in
   the changelog; `tests/harness.k` now carries a warning comment (it bit the author of these
   suites).
7. **Integer literals wider than 64 bits wrap silently** rather than raising. Now defined
   behaviour rather than UB, but still not an error.

---

## 7. How to reproduce

```sh
./build.sh
./amber --version                 # amber 1.9
tests/run_tests.sh                # all suites + C unit tests + fuzz
tests/run_tests.sh --asan         # the same, under AddressSanitizer + UBSan
```

Final run on this checkout:

```
test.k               163 tests run, 0 failures
test-fin.k            35 tests run, 0 failures
tests/test_matrix.k  309 tests run, 0 failures
tests/test_qsql.k     94 tests run, 0 failures
tests/test_simd.c    PASS      tests/test_parallel.c  PASS      tests/test_ast.c  PASS
fuzz: 1629 cases, 0 crashes, 0 hangs
ALL SUITES PASSED
```

Under `--asan` (`detect_leaks=1`, `print_stacktrace=1`): all four K suites report 0 failures with
**no `runtime error:`, no `AddressSanitizer:` and no `LeakSanitizer:` output**, and the fuzzer
reports 0 crashes across seeds 1, 7, 42 and 100–102.
