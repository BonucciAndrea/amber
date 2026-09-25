# Amber internals and the C seams

How the engine is put together, the extension mechanism, the SIMD/parallel/
disassembler/CSV engine extensions, the HFT toolkit, the Arrow C Data
Interface, and the `libamber.so` dynamic API.

Moved out of [`README.md`](../README.md) so the landing page stays a landing
page. See also [`AMBER.md`](AMBER.md) for the language reference.

---

<a name="architecture"></a>
## Architecture

```
   ./a ──► build.sh ──► cc -std=c99 src/*.c ext/*.c ──► ./amber ──► repl.k
                                        │                             │
                                        │                             └─ amber.k · fin.k · std.k
                                        │                                qsql.k · temporal.k
                                        │                                sys.k · hdb.k · ipc.k
                                        │                                lib/ext.k  (optional)
       ┌────────────────────────────────┴─────────────────────────────────┐
       │                          the interpreter                          │
       │                                                                   │
       │  a.c b.c f.c h.c i.c m.c v.c …   ngn/k evaluator core, heap, verbs │
       │  p.c ast.c vm.c                  parser · `\ast` tree · `\disasm` │
       │  3.c simd.c parallel.c peachpool.c   vector kernels · threads     │
       │  arena.c ser.c csv.c ar.c        HFT arena · -8!/-9! · CSV · Arrow│
       │  e.c diagnostic.c                the Rust-style error reporter    │
       │  inspect.c trace.c               `\v` · `\trace`                  │
       │  ln.c lnk.c                      the line editor and its `rdl` verb│
       │  ext.c                           the extension seam (src/ext.h)   │
       └───────────────────────────────────────────────────────────────────┘
```

Three properties are worth stating explicitly, because they are what the layout is *for*:

* **No optional feature is switched off.** Everything in `src/` is compiled, always. There is no
  AI code, no network code and no TLS anywhere in this repository; `grep -r socket src/` finds
  only `src/0.c`'s IPC support, the same code a q-style `hopen` uses.
* **`src/ln.c` has no interpreter dependency.** It includes `<termios.h>` and `src/ext.h` and
  nothing else of Amber's; the interpreter-facing verb lives in the separate `src/lnk.c`. The
  editor can be lifted into another project as-is.
* **Extensions never patch `src/`.** See below.

<a name="extensions"></a>
### Extensions (`src/ext.h` + `ext/`)

`ext/` is empty in a stock checkout. An out-of-tree package installs itself by dropping `.c`
files there and re-running `./build.sh`; they are compiled with the same flags, linked into the
same binary, and plug themselves in from a constructor through the hooks in
[`src/ext.h`](src/ext.h):

| hook | what it lets an extension do |
|---|---|
| `am_ext_verb("xyz", fn)` | register a backtick verb (`` `xyz x``) at runtime |
| `am_ext_bs` | claim a `\`-command before the "unknown `\cmd` is a shell command" fallback |
| `am_ext_hint` | offer inline ghost text in the editor (never inserted until accepted) |
| `am_ext_complete` | add Tab candidates ahead of the built-in lexical sources |
| `am_ext_startup` | run once, lazily, when the REPL first reads a line |
| `am_ext_usage` / `am_ext_banner` | append to `--help` and to the banner |

The Amber-level half goes in `lib/`; `repl.k` loads `lib/ext.k` whole-file at startup if it
exists, fully trapped, and an extension may define the optional `ext.pre` / `ext.post` /
`ext.err` / `ext.raw` / `ext.tag` hooks. `tests/ext_probe.c` is a complete worked example and
`tests/test_ext_seam.sh` installs it, checks every hook and uninstalls it again.

The reason this exists: pulling a new Amber release must never conflict with a package you
installed, and a user who installs nothing must pay nothing, so every hook is a null pointer and
every call site is a predictable branch. The optional
[**amber-ai**](https://github.com/bonucciandrea/amber-ai) co-pilot (an entirely separate repo; this
engine contains **no AI code and no network code**) is installed exactly this way:

```sh
git clone https://github.com/bonucciandrea/amber-ai.git
cd amber-ai && ./install.sh /path/to/amber
```

---

<a name="engine-extensions"></a>
## Engine extensions: SIMD · parallel · disassembler · CSV

Four additive engine modules, each a standalone `.c`/`.h` pair that never touches core
evaluation (`a.c`'s dispatch, `b.c`'s compiler/VM, or the `+`/`*`/`+/` verb implementations).

> A fifth module, a `\hl <expr>` command that echoed one line back with ANSI syntax colour, was
> removed. It only ever colorized a line you explicitly ran, not your keystrokes as you typed
> them, which isn't what "live syntax highlighting" means. Real live highlighting would require
> rewriting `repl.k`'s raw-keystroke input loop, which is out of scope for now; see
> [Roadmap](#roadmap).

**SIMD vector kernels** (`src/simd.{h,c}`): `simd_add_i64/f64`, `simd_mul_i64/f64`,
`simd_sum_i64/f64` operate on plain `int64_t*`/`double*` arrays with an AVX2 path
(`<immintrin.h>`, x86_64), a NEON path (`<arm_neon.h>`, any `aarch64`, Apple Silicon included),
and a scalar C99 fallback, selected at compile time. `simd_backend()` reports which one is active.
The arena allocator (`src/arena.c`) was bumped to true **32-byte alignment** via
`posix_memalign` so SIMD loads over arena-backed buffers are always aligned. Self-test + benchmark:
`` `simd 0 `` (prints backend, size, and a SIMD-vs-scalar timing comparison to stderr, returns `1`
on success):

```
$ ./amber
amber> `simd 0
simd: backend=scalar n=400009 simd_add=1.44ms scalar_add=1.98ms ok=1
1
amber> \\ (rebuilt with AMBER_NATIVE=1)
simd: backend=avx2 n=400009 simd_add=1.51ms scalar_add=1.76ms ok=1
```

**Multithreaded vector engine** (`src/parallel.{h,c}`): `par_add_i64/f64`, `par_mul_i64/f64`,
`par_sum_i64/f64` split arrays **above 100,000 elements** (`PAR_THRESHOLD`) into one contiguous
chunk per POSIX thread (`pthread_create`/`pthread_join`), each chunk processed by the SIMD
kernels above; below the threshold it calls the SIMD kernel directly with no thread overhead.
Thread count follows the same `AMBER_THREADS` env var `peach` already uses (default: online CPU
count, capped at 64). Self-test + benchmark: `` `para 0 ``.

**Bytecode disassembler + `\disasm`** (`src/vm.{h,c}`). Amber's interpreter (`src/b.c`) already
compiles every expression to a flat opcode array + constant pool and runs it on a real stack
VM (`cr()`/`cpl()`/`run()`); the AST is never walked directly at eval time. Rather than bolt on
a second, disconnected VM, `vm.c` mirrors `b.c`'s real opcode table byte-for-byte and decodes
the actual compiled bytecode Amber already produces, with a self-consistency check (the decode
loop must consume exactly the bytecode length). `\disasm <expr>` compiles an expression and
prints its locals, constant pool, and instruction stream without executing it:

```
amber> \disasm (1+2)*3-4
locals (0):
constants (2):
  #0  -1
  #1  3
bytecode (6 bytes):
    0  MONAD    1
    1  CONST    #0
    2  CONSTDYAD const#1 dyad=3
    5  MONAD    0
```

(Amber's real compiler constant-folds `1+2` and `3-4` at compile time, so the constant pool holds
`3` and `-1` rather than the original literals, which is the kind of detail a disassembler for
the *real* VM surfaces that a from-scratch reimplementation would not.)

**Native CSV parser** (`src/csv.{h,c}`): `` `csvr "path.csv" `` parses a CSV file directly into
a real Amber table (the same `flp(names ! cols)` shape `([]…)` produces, verified against
`@`, `meta`, and `qwhere`), with per-column type inference (Long / Float / Symbol),
RFC-4180-subset quoted-field handling (embedded commas, `""`-escaped quotes), and empty cells
mapped to that column's null (`0N`/`0n`/`` ` ``). The file is mapped, split at row boundaries
into one chunk per thread (`AMBER_THREADS`), and each chunk is parsed straight into the final
columns with speculative typing: a column starts as Long and a chunk re-reads only that column
when a cell promotes it to Float or Symbol. Numbers go through an exact fast path (Clinger's:
at most 19 significant digits and a power of ten up to 10^22), and everything else goes to
`strtoll`/`strtod`, so every value is the one libc gives:

```
amber> t:`csvr "trades.csv"
amber> meta t
amber> select from t where px>150
```

Self-test: `` `csv0 0 `` round-trips a fixture CSV through the parser and checks shape, values
and nulls via `#`/`~`/`@`. It then compares the number parsers against `strtoll`/`strtod` on
300,000 random and edge-case strings (`AMBER_CSV_NUMTEST=n` changes the count). Finally it
checks the reader against the 2.2.0 reader, which `csv.c` keeps as a reference, **bit for bit**
on a fixture battery and 300 random files, splitting each file into 1 to 9 chunks.
`` `csvx "path.csv" `` runs that comparison on any file and returns 1 when they agree.

**Honest deviations, stated plainly:** (1) no `src/compiler.c` was added, because `b.c` already *is*
the real compiler and VM, so a second one would be redundant/misleading; `vm.c` disassembles
the real bytecode instead. (2) the NEON path was written against the real ARM64 NEON intrinsics
and reviewed carefully, but this sandbox has no ARM cross-compiler available to build
and run it, so it has not been executed on real Apple Silicon hardware.

---

<a name="hft-toolkit"></a>
## HFT toolkit: native `aj`, arena, generators

Amber tightens the tick/quant path:

```q
gentq 100000                       / generate a full session: sets globals `trades` and `quotes`
genopt 2000                        / generate a random option chain into global `options`
m:aj[`sym`time; trades; quotes]    / TAQ: prevailing quote for every trade (native C kernel)
select from options where abs[strike-spot]<5     / near-the-money contracts
```

* **Native `aj` kernel.** `aj`/`aj0` match each trade to its most-recent quote with a
  **branch-free `lower_bound`** binary search over each symbol group's sorted nanosecond
  timestamp slice (`src/a.c`, marshalled from `amber.k`). The pure-K reference (`ajmK`) is kept
  alongside it. Correct on 64-bit ns timestamps, empty groups, and no-match rows (→ null).
* **Zero-allocation arena.** A thread-local **16 MB bump allocator** (`src/arena.{h,c}`:
  `arena_init` / `arena_alloc` / `arena_reset` / `arena_free`) supplies transient scratch during
  evaluation and is rewound once per eval cycle, so per-tick work does not thrash the system
  `malloc`/`free` and the latency jitter they cause stays out of the hot path. Self-test:
  `` `arn 0 `` → `1`.

---

<a name="apache-arrow"></a>
## Apache Arrow C Data Interface (zero dependency)

Interop with **PyArrow / Polars / DuckDB** over the stable Arrow C ABI, with no `libarrow`
linkage. Export is **zero-copy**:

```q
p:arrow.export t                    / table  -> (schemaAddr; arrayAddr)  64-bit C-ABI pointers
arrow.import p                      / (schemaAddr; arrayAddr) -> Amber table
```

<a name="libamber"></a>
## `libamber.so`: the dynamic C API seam

Amber has always had one seam for extending it **in process**: drop a `.c` file into
`ext/`, rebuild, and it plugs itself in through `src/ext.h` without a line of `src/`
being patched. There is also a matching **out-of-process** seam: the same engine,
built as a shared library, with a small documented C API on the front of it.

```bash
./build.sh                 # ./amber          (unchanged; identical binary, zero new cost)
./build.sh --shared        # ./amber  +  libamber.so
./build.sh --shared-only   # libamber.so only
AMBER_SHARED=1 ./build.sh  # same as --shared, for callers that cannot pass a flag
```

```c
#include "ext.h"                       /* section 6 -- nothing else from src/ */

amber_init("/path/to/amber");          /* boots the engine, loads the .k stdlib */
amber_value t = amber_eval_qsql("select vwap:wavg[sz;px] by sym from trades");

int type; long long n; int bits;
const void *px = amber_get_vector_ptr(amber_table_column(t, 2), &type, &n, &bits);
/* `px` IS the engine's column payload. Not a copy of it. */
```

**~60 entry points, all named `amber_*`.** Boot and evaluate (`amber_init`, `amber_eval_str`,
`amber_eval_qsql`, `amber_call`), reference counting (`amber_retain` / `amber_release`), the
zero-copy vector seam (`amber_get_vector_ptr`), tables and dictionaries, constructors for pushing
data back in, the Arrow C Data Interface, rendering, and `amber_plugin_load` for dlopening a native
plugin into a running engine.

<details>
<summary>What the shared build changes, and what it does not</summary>

**The executable is untouched.** `./build.sh` with no flags produces byte-for-byte what it produced
before, from the same objects, with the same flags. The shared library is a *separate* object set
(`-fPIC -Dshared`): position-independent code and the global-dynamic TLS model a dlopen'd library
needs both change code generation, and sharing objects between the two would silently pessimise
`./amber`.

**Only `amber_*` and `am_ext_*` are exported.** Amber's internal C is written in a terse K-derived
idiom. The engine's own globals are called `mr`, `su`, `us`, `err`, `run`, `add`, `sub`, `pk`,
`cpl`. Those are perfect inside one static binary and actively dangerous inside a library loaded
next to NumPy, libarrow and libpython. An export map (`src/libamber.map`) makes everything else
**absent** from the dynamic symbol table, so it cannot be bound to by accident and cannot
interpose on a host's symbol of the same name.

**The library records a `SONAME`.** A satellite that dlopens a *second* copy of `libamber.so` gets
a second **engine**: two heaps, two symbol tables, two global namespaces, and values from one that
are meaningless to the other, with no error, because nothing is technically wrong. The SONAME is
what lets the loader recognise an already-loaded copy and reuse it, so `python-amber`,
`libamber_arrow.so` and any plugin in one process share one engine.

**TLS drops to global-dynamic in the shared build only.** `./amber` keeps the initial-exec model on
the allocator's hot path. A dlopen'd library cannot: it is resolved out of the static TLS block the
loader sizes before `main()`, and borrowing from glibc's small surplus reserve fails
*nondeterministically*, with `cannot allocate memory in static TLS block`, depending on what else
the host imported first. See the note above `AM_TLS_IE` in `src/a.h`.

</details>

**Verification.**

```bash
tests/test_capi.sh              # release build, then ASan + UBSan
tests/run_tests.sh --asan       # the whole suite, the C API included
```

`tests/test_capi.c` is the only consumer of `libamber.so` inside this repository, and it is written
the way a satellite would write it: it includes `src/ext.h` and nothing else from `src/`, never
dereferences an `amber_value`, and links the shared library rather than the objects. **If it ever
needs a second `-I`, the API is wrong.** 81 assertions, clean under
`-fsanitize=address,undefined` with `detect_leaks=1`, because a C API whose ownership rules are
only documented is a C API whose ownership rules are wrong, and LeakSanitizer is what checks the
prose.
