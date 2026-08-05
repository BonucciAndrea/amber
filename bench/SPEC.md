# Comparative benchmark specification

Every engine in `bench/run_comparative.py` implements **this document**, nothing else.
The rules exist so a number in the results table can only be won by being faster, never by
solving a smaller problem.

## 1. Data model — identical, deterministic, and exactly representable

```
N = 10_000_000        elements for the vector workloads
M =  1_000_000        left-table rows for the join
K =      1_000        right-table rows / distinct join keys
G =        100        groups for the group-by

i     = 0 .. N-1                                int64
h[i]  = (262147 * i)  mod 1048573               int64, 0 .. 1048572
a[i]  = h[i] mod 1000                           int64, 0 .. 999
b[i]  = h[i] mod  997                           int64, 0 .. 996
x[i]  = (float64) a[i]                          exact
y[i]  = (float64) b[i]                          exact
```

**Why this generator and not a hash-style `2654435761 * i`.** The largest intermediate here is
`262147 * (N-1) = 2.62e12`, comfortably below `2^53 = 9.007e15`. That matters because not every
engine has 64-bit integers: Uiua and JavaScript compute in `float64` throughout, so a multiplier
of `2654435761` (the constant the previous suite used) overflows 53 bits at `i > 3.4e6` and those
engines would silently generate *different data* from the int64 engines. Staying under `2^53`
makes the generator bit-identical in both worlds.

All operands are non-negative, so `mod` is identical under both truncated (C, Julia) and floored
(Python, K) semantics. No engine needs a compatibility shim.

## 2. Workloads

| id | task | answer |
|---|---|---|
| `arith` | vector arithmetic + boolean masking | `sum over {i : x[i] > 50} of (x[i]*2.5 + y[i])` |
| `reduce` | three reductions over 10M elements | `sum(x) + max(x) + dot(x,y)` |
| `groupby` | group sum on integer keys, `g[i] = a[i] mod 100` | `sum over g of (g+1) * groupsum[g]` |
| `join` | inner join, `M` left rows against `K` right rows | `sum over matched pairs of (vl * vr)` |

Join tables:

```
kr[j] = (7919 * j) mod 1048573      j = 0 .. K-1     right key   (sparse, unsorted, NOT 0..K-1)
vr[j] = 2.0 * j                                      right value
kl[i] = kr[ h[i] mod K ]            i = 0 .. M-1     left key    (every row matches exactly one)
vl[i] = x[i]                                         left value
```

**Why the right keys are sparse.** If `kr` were the dense range `0..K-1`, the "join" degenerates
into a single array index (`vr[kl]`) in every array language — an O(1) lookup, not a join, while
DuckDB would still build a hash table. The previous suite had exactly this hole. Sparse,
unsorted keys force every engine to perform a real key lookup.

## 3. Exact arithmetic — why the answers are bit-comparable

Every reported answer is an **integer that fits in float64 without rounding**, and every sum is
over such integers, so the result is independent of summation order:

| workload | bound on the answer | vs `2^53 = 9.007e15` |
|---|---|---|
| `arith` | `< N * (999*2.5 + 996)` = 3.49e10 | exact |
| `reduce` | `sum <= 1e10`, `max <= 999`, `dot <= N*999*996` = 9.94e12 | exact |
| `groupby` | `< G * G * (N/G) * 999` = 9.99e12 | exact |
| `join` | `< M * 999 * 1998` = 2.00e12 | exact |

`x[i]*2.5` is exact (an integer times 2.5 is a multiple of 0.5). Consequently **there is no
floating-point excuse for a mismatch**: SIMD pairwise summation, Kahan compensation, a different
reduction tree and a naive left fold all produce the identical bit pattern. The runner compares
answers exactly and marks any engine that disagrees as `WRONG` instead of publishing its time.

Each engine also prints `CHECK = sum(a) + 3*sum(b)` (≤ 4e10, exact) so a divergence in the
*input* is caught separately from a divergence in the *result*.

## 4. Fairness rules

1. **Same algorithm, same steps.** No engine may use a primitive that skips work another engine
   must do. Concretely, this suite forbids:
   - `+/!n` style range sums. Amber constant-folds `+/!10000000` into the closed-form
     `n(n-1)/2` in `src/3.c` (`arf`) — an **O(1)** answer measured against everyone else's O(n)
     reduction. The previous `vecsum` benchmark was exactly this, and Amber "won" it by not doing
     the work. All data here is materialised before the timer starts.
   - dense-key "joins" that are really array indexing (see §2).
2. **Materialisation is forced.** Lazy or deferred engines must realise the full result inside
   the timed region: DuckDB runs the query to a scalar, NumPy/Julia assign and read the result,
   Uiua/BQN print-consume it. Each kernel returns a scalar that is then printed, so nothing can
   be dead-code-eliminated.
3. **Timing excludes startup and warm-up.** Each engine times only the kernel with its own
   monotonic clock, after `--warmup` untimed passes (default 2), and reports the median of
   `--runs` timed passes. Data generation is outside the timed region for every engine.
   Engines with no usable in-language clock fall back to `total process time − measured startup
   baseline`; the results table labels which mode each cell used. As of **1.9.1** CBQN times its
   own kernel with `•MonoTime`, so it is measured on the same basis as C, Amber, NumPy and
   Julia rather than being charged for data generation.
4. **Same types.** float64 values, int64 (or exactly-representable float64) keys, everywhere.
5. **Two Amber rows.** `amber` uses array primitives (the fair peer of K/BQN/J/Uiua);
   `amber-qsql` goes through the `select … by … from` query layer (the fair peer of DuckDB SQL).
   Reporting only the faster one would be picking whichever comparison flatters Amber.
   Since **1.9.1** that layer groups and probes on raw column vectors rather than boxing a K
   object per row, so the gap between the two rows is the query layer's real overhead (~1.1-1.5x
   on group-by and join) rather than an allocation artefact.

## 5. Protocol

Each engine script takes the workload id as its first argument and writes exactly four lines:

```
BENCH   <id>
CHECK   <integer>
ANSWER  <float64, 17 significant digits>
TIME_MS <median kernel milliseconds, float>
```

`TIME_MS` may be omitted by an engine with no clock; the runner then falls back to subtracting a
measured startup baseline. BQN scripts must not depend on `•args` being bound — the runner passes
no arguments, and several BQN environments do not provide it at all; take defaults instead (see
`bench/queries/bqn_*.bqn` for the `•BQN`+`⎊` wrapper used here). Anything else on stdout is ignored, so engines that unavoidably print
a banner are still usable.
