# Scout benchmark specification

`bench/scout/` is a **comparative scouting harness**: it runs one fixed operation matrix across
every array language, K/q implementation and columnar engine reachable on this machine, and
reports who wins each operation and by how much.

It is a strict superset of `bench/SPEC.md` and inherits all of its fairness rules. Where this
document is silent, `bench/SPEC.md` governs. Where it speaks, it wins.

---

## 1. Data model — one closed-form generator, no RNG

```
N                                       scale (default 10_000_000)
i     = 0 .. N-1                        int64
h[i]  = (262147 * i) mod 1048573        int64, 0 .. 1048572
a[i]  = h[i] mod 1000                   int64, 0 .. 999
b[i]  = h[i] mod  997                   int64, 0 .. 996
x[i]  = (float64) a[i]                  exact
y[i]  = (float64) b[i]                  exact
```

`1048573` is prime and coprime to `262147`, so `h` cycles over the full residue range: at
`N = 10M` every residue occurs 9 or 10 times, which makes group cardinality a clean function of
the modulus and keeps every workload's key distribution near-uniform.

The largest intermediate is `262147 * (N-1) = 2.62e12 < 2^53`, so float64-only engines (BQN,
Uiua, JavaScript) generate **bit-identical** input to the int64 engines. Same reasoning and same
constant as `bench/SPEC.md §1`, deliberately: scout numbers stay comparable with the existing
suite.

### Derived structures

| name | definition | used by |
|---|---|---|
| `g_C[i]` | `h[i] mod C`, `C ∈ {10, 100, 10_000, 100_000}` | `group_*`, `distinct_100k` |
| `kr[j]` | `(7919 * j) mod 1048573`, `j = 0..K-1`, `K = 1000` | `find`, `member`, `join_inner` |
| `vr[j]` | `2.0 * j` | `join_inner` |
| `kl[i]` | `kr[h[i] mod K]`, `i = 0..M-1`, `M = 1_000_000` | `join_inner` |
| `vl[i]` | `x[i]` | `join_inner` |

`kr` is **sparse and unsorted** on purpose (`bench/SPEC.md §2`): dense `0..K-1` keys turn a join
into an array index in every array language while DuckDB still builds a hash table.

### Table workloads (`tablesort`, `qsql_select`)

```
NT     = 2_000_000
sym[i] = h[i] mod 100            100 distinct symbols, named `s00 .. `s99
px[i]  = (float64)(h[i] mod 1000)
sz[i]  = h[i] mod 500
```

Symbol names are **zero-padded and letter-prefixed** (`s00`…`s99`) so that the lexicographic
order a symbol sort actually uses agrees with the numeric order the C reference sorts by.
With bare `` `0 … `99 `` the two disagree (`"10" < "2"`) and `tablesort` answers would not be
comparable across engines.

### As-of workload (`asof`)

Quotes are generated **already sorted by `(sym, time)`**, so no engine pays a hidden prep sort
inside the timed region:

```
MQ = 200_000, P = 2_000 quotes per symbol, 100 symbols
qsym[j]  = j div P                       0 .. 99, ascending
qtime[j] = 500 * (j mod P) + 1           1 .. 999_501, ascending within symbol
qbid[j]  = (float64)((j mod P) mod 1000)

MT = 1_000_000 trades
tsym[i]  = h[i] mod 100
ttime[i] = 1000 + i                      ascending, spans the quote time range
```

Every trade has at least one earlier quote for its symbol (min `ttime` 1000 > min `qtime` 1), so
the result contains **no nulls** and the answer is a clean sum.

---

## 2. Operation matrix

`core` operations are implemented by every engine. `table` operations are implemented only by the
engines that have a table/relational layer; an engine that lacks one is reported `n/a`, never
`WRONG`.

| id | tier | kernel | answer |
|---|---|---|---|
| `sum_f` | core | `+/x` | `sum(x)` |
| `max_f` | core | `\|/x` | `max(x)` |
| `dot` | core | `+/x*y` | `sum(x*y)` |
| `sum_i` | core | `+/a` (int64) | `sum(a)` |
| `arith_mask` | core | `+/(y+2.5*x)@&x>50` | `sum` over the masked expression |
| `sort_f` | core | ascending sort of `x` | order-statistic checksum + `1e9 *` inversions |
| `sort_presorted` | core | ascending sort of an already-sorted copy of `x` | same |
| `grade_i` | core | **stable** grade-up of `a` | `sum` of the first 1000 indices |
| `find` | core | `kr ? probe`, `probe[i] = kr[h[i] mod K]` | `sum` of returned indices (`= sum(a)`) |
| `member` | core | `h in kr` | `count` of members |
| `distinct` | core | `?a` (1000 distinct out of `N`) | `1e6 * count + sum(distinct)` |
| `distinct_100k` | core | `?g_100k` | `1e6 * count + sum(distinct)` |
| `group_10` | core | group-sum of `x` by `g_10` | `Σ_g (1 + g mod 251) * groupsum[g]` |
| `group_100` | core | group-sum of `x` by `g_100` | same |
| `group_10k` | core | group-sum of `x` by `g_10k` | same |
| `group_100k` | core | group-sum of `x` by `g_100k` | same |
| `join_inner` | core | inner join `kl` ⋈ `kr`, `M` × `K` | `sum` over matched pairs of `vl * vr` |
| `msum_16` | core | moving sum, width 16 | `sum` of the result vector |
| `mavg_256` | core | moving average, width 256 | `sum` of the result vector (**tolerance op**) |
| `mmax_64` | core | moving max, width 64 | `sum` of the result vector |
| `asof` | table | as-of join on `(sym, time)` | `sum` of matched `qbid` |
| `tablesort` | table | sort `NT`-row table by `(sym, px)` | order-statistic checksum + `1e9 *` lex inversions |
| `qsql_select` | table | `select sum px by sym from t where sz > 250` | `Σ_g (1 + g mod 251) * groupsum[g]` |

`join_inner`, `asof`, `tablesort` and `qsql_select` have problem sizes **fixed by this spec**;
they do not scale with `--n`, so they appear once and are excluded from the scaling curves.

### Why the sort answers look like that

A sort's result is a whole vector, so the harness needs a scalar that a *wrong* sort cannot fake:

```
ordstat    = s[0] + s[N div 4] + s[N div 2] + s[(3N) div 4] + s[N-1]
inversions = #{ j in 1..N-1 : s[j] < s[j-1] }
ANSWER     = ordstat + 1e9 * inversions
```

The order statistics pin the distribution; the inversion count (one extra linear pass, identical
work for every engine, negligible next to the sort itself) proves the output is actually ordered.
`tablesort` uses the lexicographic version: `sym[j] < sym[j-1]`, or equal `sym` and
`px[j] < px[j-1]`.

`grade_i` requires a **stable** grade, which makes the permutation unique. K, q, J, BQN and
`numpy.argsort(kind='stable')` are stable.

### Exactness

Every answer except `mavg_256` is an integer that fits in float64 without rounding, so it is
**independent of summation order**: SIMD pairwise reduction, Kahan compensation and a naive left
fold all produce the identical bit pattern, and the harness compares exactly.

| op | bound on the answer | vs `2^53 = 9.007e15` |
|---|---|---|
| `sum_f`, `sum_i`, `find` | `< 1e10` | exact |
| `dot` | `< N * 999 * 996 = 9.94e12` | exact |
| `arith_mask` | `< N * (999*2.5 + 996) = 3.49e10` | exact |
| `group_*`, `qsql_select` | `< 251 * 1e10 = 2.51e12` | exact |
| `join_inner` | `< M * 999 * 1998 = 2.00e12` | exact |
| `msum_16` | `< N * 16 * 999 = 1.60e11` | exact |
| `mmax_64` | `< N * 999 = 9.99e9` | exact |
| `asof` | `< MT * 999 = 9.99e8` | exact |
| sorts | `5*999` when correct (0 inversions) | exact |

**`mavg_256` is the one exception.** A moving average divides by a growing window count
(1, 2, …, 256), and division by 3 is not exact, so the summed result genuinely depends on the
reduction order. It is compared at a relative tolerance of `1e-9`, comfortably looser than the
worst case for a naive left fold over 10M positive terms (`N * eps ≈ 2.2e-9` bounded loosely;
every real engine does far better). Every other op is bit-exact, and the report labels this one.

Each engine also prints `CHECK = sum(a) + 3*sum(b)` over the full `N`-element base vectors —
identical for every op — so a divergence in the *input* is diagnosed separately from a divergence
in the *result*.

---

## 3. Fairness rules (in addition to `bench/SPEC.md §4`)

1. **One thread for the core comparison.** The runner exports `OMP_NUM_THREADS=1`,
   `AMBER_THREADS=1`, `POLARS_MAX_THREADS=1`, `OPENBLAS_NUM_THREADS=1`, `MKL_NUM_THREADS=1`,
   `NUMEXPR_NUM_THREADS=1`, `RAYON_NUM_THREADS=1`, `UIUA_THREADS=1`; q and PeachQ are launched
   `-s 0`; DuckDB runs `SET threads TO 1`. Amber's multi-core row (`amber-mt`) is measured
   **separately** and never mixed into the single-thread ranking.
2. **No closed-form escapes.** `+/!n`-style range reductions are forbidden (Amber constant-folds
   them in `src/3.c`); all data is materialised before the timer starts.
3. **No benchmark-shaped special cases.** No engine may recognise this specific expression. Any
   optimisation an engine applies must be a general rewrite that also fires on a program that has
   never seen this file.
4. **Materialisation is forced.** Each kernel returns a scalar that is printed, so nothing can be
   dead-code eliminated, and lazy engines must realise the result inside the timed region.
5. **Kernel time only.** Every engine times its own kernel with its own monotonic clock, after
   `--warmup` untimed passes (default 2), and reports the **median** of `--runs` timed passes
   (default 5). Data generation, startup and printing are outside the timed region.
6. **Correctness gates the timing.** A time is published only if `CHECK` and `ANSWER` both match
   the C reference. Otherwise the cell reads `WRONG` (bad answer), `BADDATA` (bad checksum),
   `ERROR`, `TIMEOUT` or `SKIP`.
7. **Idiomatic per engine, disclosed in the report.** Each engine uses the formulation a
   competent user of that engine would write (K's `?` find, DuckDB's `ASOF JOIN`, NumPy's
   `searchsorted`), and the report names the algorithm each one lands on — the algorithmic
   difference is the finding, not a flaw to be normalised away. What is forbidden is exploiting
   a property of *this* dataset, such as bincounting group keys because they happen to be dense.

---

## 4. Protocol

Each engine script takes `<op> <N> <runs> <warmup>` and writes exactly:

```
BENCH   <op>
CHECK   <integer>
ANSWER  <float64, 17 significant digits>
TIME_MS <median kernel milliseconds, float>
```

Anything else on stdout is ignored, so engines that print a banner are still usable. An engine
that cannot implement an op writes `SKIP <op>` and exits 0.

---

## 5. Reproducing

```sh
# from the repository root, under WSL
python3 bench/scout/scout.py --list                       # engines + availability
python3 bench/scout/scout.py --build                      # C ref + both Amber builds
python3 bench/scout/scout.py --smoke                      # N=100_000 sanity pass
python3 bench/scout/scout.py --n 10000000 --runs 5 \
        --out bench/scout/results.json                    # the full matrix
python3 bench/scout/scout.py --scaling 100000,1000000,10000000 \
        --out bench/scout/scaling.json
python3 bench/scout/report.py bench/scout/results.json > bench/SCOUT_REPORT.md
```
