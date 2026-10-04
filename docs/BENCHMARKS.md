# Amber: sanity checks & benchmarks

> **2.6.0 note, threads.** The tables below are still one core per engine, so they don't move.
> What 2.6 adds is threads inside primitives (see [AMBER.md](AMBER.md), "Threads inside
> primitives"). On the scout set (10M items, same laptop, 14 threads vs 2.5.0 on one), ms:
>
> | op | 2.5.0 | 2.6.0, 14 threads | |
> |---|---:|---:|---:|
> | `find` | 14.5 | 3.2 | 4.6× |
> | `member` | 9.3 | 2.8 | 3.4× |
> | `distinct` | 4.3 | 1.3 | 3.3× |
> | `sum_f` | 4.2 | 1.4 | 2.9× |
> | `max_f` | 4.0 | 1.7 | 2.3× |
> | `group_100` | 19.2 | 8.7 | 2.2× |
> | `dot` | 6.2 | 2.9 | 2.2× |
> | `tablesort` | 89.5 | 48.4 | 1.9× |
>
> About 1.6× over all 24 ops. Things that are one fast pass already (`sum_i`, `scan_f`, the
> presorted sort) stay where they were, and `group_100k` is level. With `AMBER_THREADS=1` 2.6.0
> runs at 2.5.0's speed.
>
> **2.4.1 note.** Every table that describes the current build was re-measured on 2.4.1, on one
> laptop (Intel Core Ultra 7 255U, WSL2, one core per engine unless a section says threads): §1,
> §2 and all its subsections, §2.10 (scout), §4 (peach) and §5 (CI, which re-runs itself on a
> cloud box). The release-history tables (the one just below, §6 and §7) are before/after records
> of old builds and stay as they were measured. 2.4.1 itself made reverse, `0 :':x` and `m!x`
> (power of two) one pass; the bignum collatz loop went ~300 -> ~255 ms.
>
> **2.1.0 note, a performance release with results bit-identical.** The compiler fuses the common
> idioms (`+/x*y`, `+/x@&m`, `x@&m`, `a+s*b`, `x@<x`), a one-pass group aggregate (`gsum` and
> friends, `` `gagg ``) replaces the index-list-and-each path for `select ... by`, membership
> (`in`) and distinct are one pass, value sorts of integral-valued floats are counting sorts,
> the as-of join's slice pass is a direct table, moving windows stop copying their input, and
> every kernel in `src/simd.c` is compiled twice so the **portable** build runs AVX2 at run
> time. Measured on the scout matrix ([§2.10](#210-scout--every-reachable-engine-23-operations),
> 10M elements, one core, same machine, same day, before vs after): group-by at 10/100/10k/100k
> groups **48/110/168/222 ms -> 20/21/22/30 ms**, `member` **51 -> 9.6**, `distinct_100k`
> **181 -> 20**, `asof` **37 -> 7.8**, `sort_f` **197 -> 58**, `msum_16` **43 -> 16**,
> `dot` **14.1 -> 6.8**, `arith_mask` **63 -> 23**, `qsql_select` **10.8 -> 5.1**. Amber now
> beats CBQN on 11 of the 19 operations both implement (it was 4). The remaining CBQN wins
> (`sum_f`, `max_f`, `sort_f`, `sort_presorted`, `distinct`) are its 16-bit storage of this
> dataset's "float" values, not kernel speed. The §2.10 tables below were regenerated from a
> full re-run of every engine on the 2.1.0 build.
>
> **1.9.5 note, batch 2: sliding windows and a cache-friendly radix sort.** The moving-window
> family (`msum`/`mavg`/`mvar`/`mdev`/`mmin`/`mmax`) is now a single-pass O(N) C kernel, and
> `mmin`/`mmax` are O(N) **independent of the window width** (they were O(N·W)). `<`, `asc`,
> `xasc` and `xdesc` run on a new key-carrying LSD radix sort that never gathers through the
> permutation, skips constant byte columns, and answers already-sorted input with the identity
> permutation. Measured: moving windows **11–274x**, multi-column table sort **25–30x**, as-of
> join **13–16x** (it sorts its right side). Full before/after in
> [§7](#7-195-batch-2--sliding-windows--radix-sort); every figure in [§2](#2-speed--amber-vs-numpypandas-polars-and-duckdb-ms-per-operation-single-core)
> was re-measured for this release across four engines.
>
> **1.9.3 note.** `peach` now ships worker results over the new binary serializer (`-8!`/`-9!`,
> `src/ser.c`) instead of formatting and reparsing `` `k `` text, and three bugs in its parent
> collection loop are fixed: a per-chunk refcount leak that also forced an O(total²) reallocation
> on every append, an ignored worker exit status that could return a silently wrong result, and an
> unvalidated decode. Measured here: 500,000 items under `AMBER_THREADS=4` in **~23 ms**, with
> total reserved heap **identical** before and after three further passes and a peak RSS of
> **11.5 MB**. The four comparative workloads below do not use `peach`, so their figures are
> unchanged by this release. `examples/peach_verify.k` (60 tests) covers the serializer and
> `peach`, and passes clean under ASan+UBSan with leak detection on.
>
> **1.9.2 note, self-benchmark, before vs after.** Integer `?` (find) now builds an index over
> its left argument instead of rescanning it per probe, which rewrites the inner-join cell;
> float `+/` vectorises; array payloads are cache-line aligned. Full detail and the
> before/after table are in [§6](#6-192-self-benchmark--before-vs-after) below.
>
> **1.9.1 note.** The `select … by … from` query layer now groups and probes on **raw column
> vectors** instead of boxing one K object per row: group-by is **24.7x** faster (8,171.7 ms ->
> 330.8 ms) and inner join **19.3x** faster (3,858.9 ms -> 199.7 ms) on the 10M-row suite,
> both now within ~1.1-1.5x of hand-written Amber array code, with byte-identical results. The
> CBQN scripts in `bench/queries/` also compile again (they previously failed on BQN identifier
> roles, so every CBQN cell was an error) and now time their own kernel with `•MonoTime`. The
> live table in §5 below is regenerated by CI.
>
> **1.9 note.** The as-of join (`aj`/`aj0`) now runs its match step in a native C kernel
> (branch-free `lower_bound` over each group's sorted nanosecond timestamps; `src/a.c`), using
> the HFT arena allocator for its transient buffer so a per-tick `aj` makes no `malloc`/`free`
> calls. The attribute/index figures below are unchanged.

## Version history: speed evolution at a glance

Every release's headline kernel change, consolidated from the before/after numbers already
measured in the sections below (§6, §7, §2.10). Each figure is a real, recorded measurement, and
this table only gathers them in one place; it introduces no new run. Median kernel ms unless
noted; lower is better. **Numbers within a row share one machine and one dataset; rows are not
comparable across machines**, and the live §2 tables always reflect the current build.

| Release | What got faster | Workload (measured) | Before | After | Speedup |
|---|---|---|---:|---:|:--:|
| **1.9** | as-of join match step → native branch-free C kernel | foundation for the 1.9.5 `aj` sort win | — | — | native |
| **1.9.1** | query layer groups/probes on raw column vectors, not one boxed K object per row | group-by, 10M-row suite | 8,171.7 | 330.8 | **24.7×** |
| | | inner join, 10M-row suite | 3,858.9 | 199.7 | **19.3×** |
| **1.9.2** | integer `?` (find) builds an index instead of rescanning per probe | inner join (1M × 1,000 sparse keys) | 180.95 | 5.66 | **32.0×** |
| | | `kr?kl` find micro-kernel (1M probes) | 211.4 | 2.6 | **81×** |
| | | `+/px`, 10M float sum (vectorised) | 8.9 | 6.3 | 1.41× |
| **1.9.3** | `peach` ships worker results over the `-8!`/`-9!` binary serializer | 500k items @ `AMBER_THREADS=4` | — | ~23 ms | native |
| **1.9.5** | single-pass O(N) moving windows + key-carrying LSD radix sort | `mmin[1000;x]`, window 1000 | 1,021 | 3.73 | **273.9×** |
| | | multi-column table sort (`xasc`, 1 float key) | 1,617 | 54.6 | **29.6×** |
| | | as-of join (50k × 100k; sorts its right side) | 67.3 | 4.20 | **16.0×** |
| **2.0.0** | table group-by keys on the interned 4-byte symbol id, not a per-character string sort | `select … by sym`, 1M rows | 587 | 34 | **17×** |
| **2.1.0** | one-pass fused group aggregate (`` `gagg ``) behind `gsum`/`select … by` | group-sum, 10M rows, 100k groups (scout) | 222 | 30 | **7.4×** |
| | | group-sum, 10M rows, 100 groups (scout) | 110 | 21 | **5.2×** |
| | one-pass membership (`` `memb ``) and bitmap distinct | `h in kr`, 10M vs 1,000 keys (scout) | 51 | 9.6 | **5.3×** |
| | | `?g`, 10M values, 100k distinct (scout) | 181 | 20 | **9.0×** |
| | direct-table slice pass in the as-of join | `aj`, 1M trades × 200k quotes (scout) | 37 | 7.8 | **4.8×** |
| | counting sort for integral-valued floats, no grade, no gather | `x@<x`, 10M float64, 1,000 distinct (scout) | 197 | 58 | **3.4×** |
| | compiler idiom fusion: `+/x*y`, `+/x@&m`, `x@&m`, `a+s*b` | `+/x*y`, 10M float64 (scout) | 14.1 | 6.8 | **2.1×** |
| | | `+/(y+2.5*x)@&x>50`, 10M (scout) | 63 | 23 | **2.7×** |
| | moving windows without the input copy / per-call mapping | `msum[16;x]`, 10M (scout) | 43 | 16 | **2.7×** |
| **2.2.0** | keys-only radix sort (permute the keys, not the input) | sort 10M non-integral float64 | 304.7 | 119.4 | **2.6×** |
| | float range scan split from the integrality test | sort 10M float64, already sorted | 20.6 | 3.6 | **5.8×** |
| | float `distinct` grows its hash with the distinct count | `?x`, 10M float64 | 26.7 | 13.6 | **2.0×** |
| | `select` stops materialising the filtered table where it's ignored | `select from t`, 10M rows | 5.7 | 0.7 | **8.2×** |
| **2.3.0** | `mmax`/`mmin` as van Herk/Gil-Werman, `msum`/`mavg` as blocked running sums | `mmax_64` / `msum_16`, 10M (scout) | — | — | **2.3× / 1.5×** |
| | integer distinct | `?x`, 10M ints (~100k distinct) | — | — | **3.9×** |
| **2.4.0** | masked float sums without the fused kernel, equal to the unfused expression | `+/x@&x>50`, 10M | 48.8 | 10.3 | **4.7×** |
| | | `+/((x*2.5)+y)@&x>50`, 10M | 56.1 | 26.5 | **2.1×** |
| | tail calls reuse the frame | `{$[x;o x-1;0]}1000000` | `'stack` | 66 | — |
| **2.4.1** | reverse as one copy, bytes 8 at a time | `\|b`, 8,192-byte boolean, 20k calls | 32 | 5 | **6.4×** |
| | `0 :':x` and `m!x` (power of two) in one pass | bignum collatz loop, 2^18 | ~300 | ~255 | **1.18×** |

Reproduce each row with the harness named in its section: the 1.9.1/1.9.2 rows via
`bench/run_comparative.py --runs 5 --warmup 2` (it prints both columns itself); the 1.9.5
rows via `bench/suite.k` against `bench/baseline_194_suite.txt` ([§7](#7-195-batch-2--sliding-windows--radix-sort));
the 2.0.0 row with `./amber bench/qbench/amber.k`, which reports the before/after directly;
the 2.1.0 rows with `python3 bench/scout/scout.py --engines amber --n 10000000` on the two builds;
the 2.2.0-2.4.1 rows are the before/after runs recorded in CHANGELOG.md at each release.
The releases in between (1.9.3, 1.9.4, 1.9.6) were correctness/serializer/tooling work whose four
comparative workloads did not move; see each release note above.

This document records (1) **correctness cross-checks** of Amber against the mainstream
array/columnar tools (numpy + pandas), and (2) **speed benchmarks** on the same workloads.
It also ships two **portable harnesses** in `bench/`: `run_suite.sh` (the 25-workload
cross-engine suite behind §2, covering Amber, numpy/pandas, Polars and DuckDB) and `run.sh`
(the original sanity harness, which additionally runs **growler/k** the moment it is
present on the machine).

[§1](#1-sanity-checks--amber-vs-numpypandas--n--1000000) comes from `bench/bench.k` +
`bench/bench.py`; [§2](#2-speed--amber-vs-numpypandas-polars-and-duckdb-ms-per-operation-single-core)
from `bench/suite.k` + `bench/suite.py`. Every engine builds the *same deterministic dataset*
from a closed formula, so results compare exactly:

```
i    = 0 .. N-1
h    = (262147 * i) mod 1048573                 / hash-ish spread, 1,048,573 distinct
px   = 0.001 * ((2654435761 * i) mod 100000)     / floats 0.000 .. 99.999
py   = 0.001 * ((40503 * i) mod 100000)
vi   = (2654435761 * i) mod 100000               / integer keys, 100,000 distinct
sym  = i mod 10                                  / 10 groups
```

No RNG anywhere, so every implementation sees bit-identical inputs and the numeric outputs
must match bit-for-bit (floats: to rounding). Every §2 workload additionally carries the
scalar answer its engine computed, and the reporter refuses to print a ratio for any case
where two engines disagree.

---

## 1. Sanity checks: Amber vs numpy/pandas  (N = 1,000,000)

| check | Amber | numpy / pandas | agree |
|-------|-------|----------------|:-----:|
| `+/px` (sum) | 49999500.0 | 49999500.0 | ✅ |
| `+/px>50` (count) | 499990 | 499990 | ✅ |
| group sums by `sym` | 4999500 … 5000400 (step 100) | 4999500 … 5000400 | ✅ |
| median of sorted px | 50.0 | 50.0 | ✅ |
| rolling-mean(100) checksum | 49999058.781311 | 49999058.781311 | ✅ (12 s.f.) |
| distinct count | 100000 | 100000 | ✅ |

Every aggregation agrees. One instructive note: computing "distinct" via a **float**
round-trip (`(px*1000).astype(int)`) gives numpy a spurious 99259 because `0.001*n*1000`
is not exactly `n`; Amber's integer path returns the correct 100000, and numpy agrees once
it too uses the integer column. Good demonstration of why the sanity harness uses integer
keys for `distinct`.

## 2. Speed: Amber vs numpy/pandas, Polars and DuckDB (ms per operation, single core)

> **Everything below was re-measured on 2.4.1, on one machine, in one sitting, by one harness**
> (`bench/suite.k` + `bench/suite.py`, driven by `bench/run_suite.sh`, rendered by `bench/render.py`).
> 25 workloads x 4 engines plus 12 Amber-only scaling cases. Every workload carries the scalar answer
> the engine computed, and `render.py` **refuses to print a ratio for any case where the answers
> disagree**. The suite ran three times end to end; each cell is the median of the three.
> This run: **37 cases, 0 answer mismatches** (64 engine answers checked against Amber's).

### At a glance

| Amber is fastest here | Amber is slowest here |
|---|---|
| **moving windows**, 0.6–3.2 ms vs 10–18 ms (pandas) and 4.4–11 ms (Polars) | **filter + count**, 0.98 ms vs ~0.26 ms (numpy, Polars) |
| **group-by at every cardinality**, 10 to 100,000 groups, 1.8–2.5 ms | **sorting float values**, 18.7 ms vs numpy's 6.7 ms |
| **distinct**, 0.25 ms vs 8.8 ms (DuckDB) and 42 ms (`np.unique`) | **as-of join**, behind Polars at both sizes (4.8 vs 2.8 ms, 49 vs 29 ms) |
| **inner join on sparse keys**, **multi-column table sort**, **integer sort** | |

### Method

| | |
|---|---|
| **Timing** | 2 warm-up passes, then 5 timed batches; a run reports the **median of the 5 batch means**, and the tables show the median of three full runs (single cells on a laptop move by tens of percent run to run). |
| **Threads** | Every engine pinned to **one core and one thread** (`taskset -c 3`, `AMBER_THREADS=1`, `POLARS_MAX_THREADS=1`, `SET threads TO 1`, `OMP_NUM_THREADS=1`). Amber's `peach` is measured separately in [§4](#4-parallelism--peach-multi-core). |
| **Data** | A closed formula, no RNG, so every engine sees bit-identical inputs (the header of `bench/suite.k`). The table sorts use `sym = "S"+((7919*i) mod 5000)` as symbols/strings and `time` a cumulative sum of small gaps; the as-of books are partitioned over 100 symbols with strictly increasing times, so "last quote at or before t" has exactly one answer. |
| **Fairness** | Results are materialised inside the engine (DuckDB writes `CREATE TABLE AS`, never `fetchall()`). Join right tables are built before the clock, for every engine. The as-of inputs are identical, time-ordered tables for everyone, so Amber's `aj` pays to regroup the quotes by symbol inside the clock. The already-sorted sort gets a fresh copy per call, because Amber's `asc` marks a vector sorted in place and would otherwise time nothing. `mdev` is the population stddev everywhere; windows grow over the first w-1 points (q's rule). |
| **Machine** | Intel Core Ultra 7 255U laptop, WSL2 (Ubuntu 24.04, 7.6 GB), gcc 13.3, Amber 2.4.1 native build. numpy 2.5.2 / pandas 3.0.5 / Polars 1.44.1 / DuckDB 1.5.5, Python 3.12.3. |
| **Reproduce** | `PY=python3 ./bench/run_suite.sh` (engines that aren't installed are skipped). |

### Headline: 1,000,000-row vectors

Lower is better; **bold** marks the fastest engine in each row. The bar encodes the Amber
vs numpy/pandas gap: **filled from the left = Amber faster**, filled from the right = slower,
length grows with the multiple.

| workload | Amber | numpy/pandas | Polars | DuckDB | Amber vs numpy/pandas |
|---|---:|---:|---:|---:|---|
| `+/px`, sum | **0.134** | 0.273 | 0.173 | 1.32 | **2.0x faster** ███░░░░░░░ |
| `+/px>50`, filter + count | 0.977 | **0.257** | 0.260 | 3.52 | 3.8x slower ░░░░██████ |
| group-by sum (10 groups) | **1.78** | 5.47 | 3.74 | 3.60 | **3.1x faster** █████░░░░░ |
| distinct (100k distinct keys) | **0.246** | 41.5 | 12.6 | 8.77 | **168.7x faster** ██████████ |
| moving average (window 100) | **0.874** | 10.5 | 4.55 | — | **12.0x faster** ██████████ |
| sort (1M floats) | 18.7 | **6.71** | 15.8 | 64.2 | 2.8x slower ░░░░░░████ |
| as-of join (50k trades x 100k quotes) | 4.83 | 8.47 | **2.84** | 29.3 | **1.8x faster** ██░░░░░░░░ |

---

### 2.1 Moving windows: 1M floats

Single-pass O(N) C kernels since 1.9.5 (`msum`/`mavg`/`mvar`/`mdev` carry a running difference),
and since 2.3 `mmax`/`mmin` are van Herk/Gil-Werman and `msum`/`mavg` blocked running sums. Amber is
the **fastest engine on every window workload**, 6–17x ahead of pandas and 2–7x ahead of Polars.

| workload | Amber | numpy/pandas | Polars | Amber vs numpy/pandas |
|---|---:|---:|---:|---|
| `msum[100;x]`, moving sum | **0.590** | 10.0 | 4.39 | **17.0x faster** ██████████ |
| `mavg[100;x]`, moving average | **0.874** | 10.5 | 4.55 | **12.0x faster** ██████████ |
| `mdev[100;x]`, moving stddev | **3.19** | 18.3 | 11.2 | **5.8x faster** ████████░░ |
| `mmin[100;x]`, moving min | **1.27** | 11.5 | 5.88 | **9.1x faster** ██████████ |
| `mmax[100;x]`, moving max | **1.17** | 11.3 | 5.52 | **9.7x faster** ██████████ |
| `mavg[10;x]`, window 10 | **0.888** | 10.7 | 4.64 | **12.1x faster** ██████████ |
| `mavg[1000;x]`, window 1000 | **1.05** | 10.8 | 5.09 | **10.3x faster** ██████████ |
| `mmin[10;x]`, window 10 | **0.991** | 11.3 | 5.64 | **11.4x faster** ██████████ |
| `mmin[1000;x]`, window 1000 | **1.69** | 11.4 | 5.82 | **6.8x faster** ████████░░ |

Window-width sensitivity, same 1M vector:

| window | Amber `mmin` | pandas `rolling().min()` | Polars `rolling_min` |
|---|---:|---:|---:|
| 10 | **0.991** | 11.3 | 5.64 |
| 100 | **1.27** | 11.5 | 5.88 |
| 1,000 | **1.69** | 11.4 | 5.82 |

Amber moves from 1.0 to 1.7 ms across a 100x change in window width. Before 1.9.5 the same three rows
read 314 / 383 / 1021 ms, because the old kernel was O(N·W).

---

### 2.2 Sort and grade: 1M elements

| workload | Amber | numpy/pandas | Polars | DuckDB | Amber vs numpy/pandas |
|---|---:|---:|---:|---:|---|
| sort 1M floats, `asc x` | 18.7 | **6.71** | 15.8 | 64.2 | 2.8x slower ░░░░░░████ |
| sort 1M integers, `asc x` | **1.74** | 9.50 | 11.2 | 33.5 | **5.5x faster** ███████░░░ |
| grade 1M floats, `iasc x` (argsort) | **23.5** | 26.9 | 30.1 | — | **1.1x faster** █░░░░░░░░░ |
| sort an ALREADY-SORTED 1M float vector | **0.408** | 8.32 | 1.45 | — | **20.4x faster** ██████████ |

* **Integer sort is now the fastest of the four by a long way** (1.7 ms, 5.5x numpy): the keys sit
  in 0..99,999, which the radix/counting path eats.
* **Float sort still loses to numpy's value sort**, 2.8x, because every Amber sort goes through the
  grade (23.5 ms, already ahead of numpy's argsort) plus a gather. A direct value sort is still the
  clearest open item (§2.8).
* **Already-sorted input is nearly free** (0.4 ms, a fresh unmarked copy each call): the radix
  kernel spots a non-decreasing column in its key pass and returns the identity.

---

### 2.3 Multi-column table sort: 1M rows x 3 columns

Stable multi-column LSD radix on the columns' raw arrays; symbols collate by name through a dense rank
of the distinct ids, so 5,000 symbols cost 5,000 string comparisons, not one per row. Amber leads all
three; single-threaded Polars is slow on the string-keyed sorts.

| workload | Amber | numpy/pandas | Polars | DuckDB | Amber vs numpy/pandas |
|---|---:|---:|---:|---:|---|
| ``xasc[,`px;t]``, 1 float key, 3 cols | **34.1** | 113 | 50.9 | 152 | **3.3x faster** █████░░░░░ |
| ``xasc[`sym`time;t]``, 2 keys, 3 cols | **50.5** | 114 | 299 | 150 | **2.3x faster** ████░░░░░░ |
| ``xdesc[`sym`time;t]``, 2 keys, descending | **46.3** | 128 | 310 | 148 | **2.8x faster** ████░░░░░░ |

---

### 2.4 Joins

| workload | Amber | numpy/pandas | Polars | DuckDB | Amber vs numpy/pandas |
|---|---:|---:|---:|---:|---|
| inner join (1M rows x 1,000 sparse keys) | **6.14** | 49.6 | 8.79 | 26.2 | **8.1x faster** █████████░ |
| as-of join (50k trades x 100k quotes) | 4.83 | 8.47 | **2.84** | 29.3 | **1.8x faster** ██░░░░░░░░ |
| as-of join (500k trades x 1M quotes) | 49.1 | 72.4 | **29.0** | 293 | **1.5x faster** ██░░░░░░░░ |

The inner join is a clear win (6.1 ms; Polars 8.8, DuckDB 26, pandas 50). The as-of join beats pandas
and DuckDB but trails Polars at both sizes, and nearly all of Amber's cost there is regrouping the
quotes by symbol inside the clock: with quotes already `xasc`'d by `` `sym`time `` (not the same
input, so not in the table) `aj` does 500k x 1M in ~9.8 ms and 50k x 100k in ~1.8 ms. All four engines
return the same joined-bid checksum.

---

### 2.5 Group-by vs cardinality: 1M rows

| workload | Amber | numpy/pandas | Polars | DuckDB | Amber vs numpy/pandas |
|---|---:|---:|---:|---:|---|
| 10 groups | **1.78** | 5.47 | 3.74 | 3.60 | **3.1x faster** █████░░░░░ |
| 1,000 groups | **1.92** | 8.11 | 15.0 | 3.69 | **4.2x faster** ██████░░░░ |
| 100,000 groups | **2.54** | 18.6 | 36.7 | 14.4 | **7.3x faster** █████████░ |

This used to be Amber's clearest weakness (170 ms at 100,000 groups, 3.3x slower than pandas), because
the idiomatic form called a lambda per group. Since 2.1 `gsum` (and `select … by`) is a one-pass fused
group aggregate in C, and Amber is now the fastest engine at all three cardinalities.

---

### 2.6 Scaling: Amber, 100k -> 10M rows

| workload | 100k | 1M | 10M | ns/row @10M | scaling 100k->10M |
|---|---:|---:|---:|---:|---|
| `+/px` sum | 0.004 | 0.127 | 3.43 | 0.3 | 857x time for 100x data |
| sort | 1.29 | 18.4 | 226 | 22.6 | 174x time for 100x data |
| moving average (100) | 0.084 | 0.979 | 8.77 | 0.9 | 104x time for 100x data |
| group-by sum | 0.161 | 1.75 | 18.8 | 1.9 | 117x time for 100x data |

Nothing is super-linear algorithmically; the big factors are the memory hierarchy. At 100k a float
column is 800 KB and the sum runs out of L2 (0.004 ms), at 10M it's 80 MB from DRAM, so `sum` reads
"857x for 100x data" while the window and group-by kernels scale about linearly.

---

### 2.7 Reading the results

* **Amber wins on windows, group-by at any cardinality, distinct, integer sorts, table sorts and
  sparse-key joins.** Tight C loops with no DataFrame object overhead, and since 2.1 the grouping and
  membership kernels are one pass.
* **Streaming arithmetic is at or ahead of parity**: `sum` is 2x numpy here at 1M rows.
* **Amber loses on `filter + count`** (3.8x): numpy and Polars fuse compare-and-count, Amber
  materialises the boolean vector first.
* **Amber loses on sorting float values** (2.8x) for the reason in §2.2, and on the **as-of join
  against Polars** for the regrouping reason in §2.4.
* **`distinct` flipped completely**: it used to trail Polars and DuckDB 3–4x, now it's 0.25 ms against
  their 9–13 ms. These keys are dense ints in 0..99,999, which suits Amber's bitmap path; a sparse key
  set would narrow that.

### 2.8 Still on the table

| # | Change | Expected | Where |
|---|---|---|---|
| 1 | **Direct value sort**, when `asc x` is consumed immediately, sort values in place instead of grading then gathering | closes most of the 2.8x float-sort gap | `src/o.c` |
| 2 | **Fused compare-and-count** for `+/x>c` | ~3x on filter, to parity | `src/3.c` |
| 3 | **As-of without the regroup**, reuse a symbol-grouped quote index across calls | ~5x on `aj`, ahead of Polars | `src/a.c` |
| 4 | **Parallel radix sort** on the `peach` pool | 4–8x on sort-heavy work | `src/v.c`, `src/peachpool.c` |
| 5 | **Parallel moving windows**, overlapping halo blocks, one per pool lane | 3–6x on windows | `src/v.c` |

Done since the last revision of this list: **native grouped reduce** (`gsum`, 2.1: group-by went from
170 ms to 2.5 ms at 100k groups), **`mmin`/`mmax` monotonic deque** then van Herk/Gil-Werman, and the
**C radix sort** for integer/float keys.

## 2.10 Scout: every reachable engine, 24 operations

The rest of §2 compares Amber with the four engines the CI harness can install everywhere.
**Scout** is the widest run in the project: 24 operations against *every* array language and
columnar engine that could be made to run on one machine (PeachQ (Rayforce), ngn/k, CBQN, J,
NumPy, pandas, Polars, DuckDB and a hand-written C reference) plus Amber's portable, native
and qSQL configurations (every engine on one thread).

The workloads, the data model and the fairness rules are specified once in
[`bench/scout/SCOUT_SPEC.md`](../bench/scout/SCOUT_SPEC.md); every engine implements that document
and nothing else. `bench/scout/scout.py` runs it, `bench/scout/results.json` is the raw output, and
the tables below are generated straight from that file by `bench/scout/webgen.py --md`, so these
numbers and the ones on the website cannot drift apart. The narrative analysis lives in
[`bench/SCOUT_REPORT.md`](../bench/SCOUT_REPORT.md).

**Correctness gate.** Every answer is an integer exactly representable in float64, so the result is
independent of summation order. SIMD pairwise, Kahan-compensated and naive left-fold summation all
produce the identical bit pattern. Answers are compared **bit-exactly** against the C reference; an
engine that disagrees gets `WRONG` instead of a time, and each engine separately checksums its own
*input* so a divergence in the generator is caught as `BADDATA`. The single exception is
`mavg_256`, which divides by a growing window count and is therefore order-dependent; it
is compared at a relative tolerance of 1e-9.

### Machine and versions

| | |
|---|---|
| **CPU** | Intel(R) Core(TM) Ultra 7 255U (14 cores) |
| **SIMD** | avx avx2 bmi2 sse4_2 |
| **OS** | Linux-6.18.33.2-microsoft-standard-WSL2-x86_64-with-glibc2.39 |
| **Compiler** | gcc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0 |
| **Rows** | N = 10,000,000 |
| **Runs** | 5 timed, 2 warm-up |
| **PeachQ** | v0.81 |
| **CBQN** | CBQN on commit af583e19566a032b89e0077b866b0ba0dcc2a365 |
| **NumPy / pandas** | 2.5.2 / 3.0.5 |
| **Polars / DuckDB** | 1.44.1 / 1.5.5 |
| **Amber build** | e8892b9 (2.4.1) |

### Amber vs the C reference: all 24 operations

| operation | Amber (ms) | C (ms) | ratio | what it is |
| --- | ---: | ---: | ---: | --- |
| `distinct_100k` | 2.20 | 67.5 | **30.66x faster** | distinct over 100k groups |
| `member` | 9.38 | 203 | **21.61x faster** | membership of 10M against a 10M set |
| `sort_presorted` | 7.73 | 109 | **14.07x faster** | sort of already-sorted input (best case) |
| `distinct` | 3.81 | 33.3 | **8.75x faster** | distinct over ~10 groups |
| `sum_i` | 0.809 | 4.23 | **5.23x faster** | sum of 10M integers 0..999 (engines differ in element width) |
| `grade_i` | 24.9 | 89.1 | **3.58x faster** | grade-up (argsort) of 10M int64 |
| `group_100k` | 33.3 | 118 | **3.54x faster** | group-by, 100k groups |
| `sort_f` | 29.5 | 89.8 | **3.05x faster** | ascending sort, 10M float64 from 1000 distinct values |
| `max_f` | 4.46 | 9.88 | **2.21x faster** | max of 10M float64 |
| `group_10k` | 26.6 | 53.2 | **2.00x faster** | group-by, 10k groups |
| `group_100` | 21.2 | 30.9 | **1.46x faster** | group-by, 100 groups |
| `dot` | 6.24 | 8.44 | **1.35x faster** | dot product of two 10M float64 vectors |
| `group_10` | 16.7 | 22.5 | **1.35x faster** | group-by, 10 groups |
| `sum_f` | 3.93 | 5.21 | **1.33x faster** | sum of 10M float64 |
| `find` | 11.7 | 14.6 | **1.25x faster** | first index of a value in 10M elements |
| `mmax_64` | 13.8 | 16.3 | **1.19x faster** | moving max, window 64 |
| `scan_f` | 8.36 | 9.24 | **1.11x faster** | running sum over 10M float64, materialised |
| `msum_16` | 11.0 | 11.4 | **1.04x faster** | moving sum, window 16 |
| `asof` | 7.50 | 7.28 | 1.03x slower | as-of join, the tick-desk workload |
| `mavg_256` | 12.6 | 11.6 | 1.08x slower | moving average, window 256 |
| `join_inner` | 2.91 | 2.64 | 1.10x slower | inner join on an int key |
| `tablesort` | 93.7 | 74.1 | 1.26x slower | 3-column table sorted by two keys |
| `arith_mask` | 15.2 | 9.89 | 1.54x slower | (a*b)+c under a boolean mask |
| `qsql_select` | 4.60 | 2.46 | 1.87x slower | select ... by ... from - the full query path |


Amber is faster on **18 of 24** operations, slower on **6**.

### The full matrix: 24 operations x 12 engines

| operation | C | Amber-nat | Amber | Amber-qSQL | PeachQ | ngn/k | CBQN | J | NumPy | pandas | Polars | DuckDB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| **Reductions & vector arithmetic** |  |  |  |  |  |  |  |  |  |  |  |  |
| `sum_f` | 5.21 | 3.93 | 4.63 | skip | 4.58 | 5.38 | **1.05** | 4.03 | 4.94 | 9.60 | 3.79 | 12.0 |
| `sum_i` | 4.23 | 0.809 | **0.801** | skip | 2.30 | 1.62 | 1.27 | 5.05 | 3.65 | 3.77 | 4.12 | 10.8 |
| `max_f` | 9.88 | 4.46 | 4.26 | skip | 4.68 | 12.5 | **1.18** | 4.43 | 3.91 | 13.0 | 4.91 | 35.8 |
| `dot` | 8.44 | 6.24 | **5.85** | skip | 8.50 | 15.8 | 6.58 | 27.7 | 6.00 | 7.20 | 12.6 | 21.2 |
| `arith_mask` | **9.89** | 15.2 | 14.5 | skip | 364 | 51.5 | 50.0 | 50.7 | 44.9 | 95.1 | 55.8 | 56.7 |
| `scan_f` | 9.24 | **8.36** | 9.11 | skip | 30.9 | 207 | 9.41 | 21.7 | 23.4 | 40.9 | 32.5 | 1659 |
| **Sort & grade** |  |  |  |  |  |  |  |  |  |  |  |  |
| `sort_f` | 89.8 | 29.5 | 31.2 | skip | 930 | 521 | **6.31** | 5258 | 53.7 | 856 | 96.1 | 856 |
| `sort_presorted` | 109 | 7.73 | 8.82 | skip | 667 | 263 | **1.35** | 94.2 | 59.6 | 114 | 18.2 | 624 |
| `grade_i` | 89.1 | 24.9 | 34.2 | skip | 40.9 | 123 | 34.6 | 71.8 | 615 | 770 | 254 | **23.2** |
| `tablesort` | **74.1** | 93.7 | 105 | skip | 1372 | skip | skip | skip | 207 | 118 | 438 | 384 |
| **Search, distinct & group-by** |  |  |  |  |  |  |  |  |  |  |  |  |
| `find` | 14.6 | **11.7** | 13.4 | skip | 82.0 | 1153 | 18.2 | 52.2 | 127 | skip | skip | 37.8 |
| `member` | 203 | **9.38** | 10.6 | skip | 143 | 2037 | 15.6 | 31.9 | 53.5 | 70.6 | 30.8 | 43.2 |
| `distinct` | 33.3 | **3.81** | 5.25 | skip | 525 | 219 | 5.19 | 34.0 | 300 | 45.6 | 85.9 | 40.1 |
| `distinct_100k` | 67.5 | **2.20** | 7.78 | skip | 1859 | 362 | 44.3 | 48.5 | 709 | 60.9 | 116 | 87.1 |
| `group_10` | 22.5 | **16.7** | 17.6 | 22.9 | 359 | 49.5 | 27.4 | 40.7 | 241 | 55.6 | 33.7 | 33.2 |
| `group_100` | 30.9 | **21.2** | 25.9 | 28.7 | 418 | 117 | 62.0 | 38.9 | 789 | 58.4 | 31.9 | 35.1 |
| `group_10k` | 53.2 | **26.6** | 28.0 | 31.3 | 1664 | 177 | 120 | 41.9 | 1093 | 120 | 316 | 92.1 |
| `group_100k` | 118 | **33.3** | 39.2 | 45.7 | 4196 | 439 | 241 | 60.1 | 704 | 133 | 355 | 142 |
| **Joins** |  |  |  |  |  |  |  |  |  |  |  |  |
| `join_inner` | 2.64 | 2.91 | 2.87 | 6.11 | 11.4 | 141 | **2.14** | 6.58 | 22.3 | 53.4 | 11.2 | 7.22 |
| `asof` | **7.28** | 7.50 | 7.29 | skip | 216 | skip | skip | skip | 25.5 | 70.6 | 15.8 | 212 |
| **Moving windows** |  |  |  |  |  |  |  |  |  |  |  |  |
| `msum_16` | 11.4 | 11.0 | **10.8** | skip | 416 | 256 | 28.7 | 56.2 | 122 | 122 | 49.6 | 1034 |
| `mavg_256` | **11.6** | 12.6 | 13.1 | skip | 10257 | 259 | 50.6 | 95.5 | 142 | 129 | 53.3 | 1334 |
| `mmax_64` | 16.3 | **13.8** | 15.0 | skip | 1989 | skip | skip | skip | 452 | 134 | 58.9 | 1471 |
| **qSQL-shaped** |  |  |  |  |  |  |  |  |  |  |  |  |
| `qsql_select` | **2.46** | 4.60 | 5.21 | 3.93 | 123 | skip | skip | skip | 46.5 | 16.2 | 12.1 | 9.05 |


Milliseconds, lower is better. **Bold** is the fastest engine in the row (every engine runs one thread).
`skip` means the engine cannot express the operation faithfully under `SCOUT_SPEC.md`.

### Scaling: Amber (native), 100k to 10M rows

| operation | 100,000 | 1,000,000 | 10,000,000 | 100k->10M |
| --- | ---: | ---: | ---: | ---: |
| `sum_f` | 0.006 | 0.253 | 3.42 | x569 |
| `sort_f` | 0.135 | 2.32 | 29.3 | x217 |
| `group_10k` | 0.324 | 2.73 | 28.1 | x87 |


### Where Amber is beaten, and by how much

Every operation where at least one single-threaded engine is faster than Amber,
largest gap first. This is the optimisation backlog, kept public on purpose.

| operation | Amber (ms) | best (ms) | best engine | headroom |
| --- | ---: | ---: | --- | ---: |
| `sort_presorted` | 7.73 | 1.35 | CBQN | **5.73x** |
| `sort_f` | 29.5 | 6.31 | CBQN | **4.67x** |
| `max_f` | 4.46 | 1.18 | CBQN | **3.78x** |
| `sum_f` | 3.93 | 1.05 | CBQN | **3.75x** |
| `qsql_select` | 4.60 | 2.46 | C | **1.87x** |
| `arith_mask` | 15.2 | 9.89 | C | **1.54x** |
| `join_inner` | 2.91 | 2.14 | CBQN | **1.36x** |
| `tablesort` | 93.7 | 74.1 | C | **1.26x** |
| `mavg_256` | 12.6 | 11.6 | C | **1.08x** |
| `grade_i` | 24.9 | 23.2 | DuckDB | **1.07x** |
| `dot` | 6.24 | 6.00 | NumPy | **1.04x** |
| `asof` | 7.50 | 7.28 | C | **1.03x** |

### Reading these numbers honestly

Two rows compare **storage width** as much as kernel speed. `sum_i` and `sort_f` draw from 1000
distinct small values, and the engines differ in how wide they keep them: Amber narrows integer
storage to the value range, CBQN likewise, while the C reference, NumPy and the SQL engines use
real int64/float64 throughout. Narrow storage is a genuine advantage -- it is less memory traffic --
but it is not "a faster `+/`", and CBQN's lead on `sum_f`, `max_f`, `sort_f` and `sort_presorted`
is mostly that. It is disclosed here rather than dropped.

`scan_f` is disclosed the other way. DuckDB's only spelling for a running sum is an ordered window
function, which must establish an order the array languages already have; the row is published with
that noted rather than omitted, on the same principle that puts `Amber-qSQL` beside `Amber`.


## 3. Running the cross-language harness (growler/k, q, DuckDB, Polars)

The cloud sandbox that produced the numbers above has no k interpreter and no package
access, so growler/k, q, DuckDB and Polars are **not** included in the tables. To get them,
run the harness locally:

```sh
cd bench
./run.sh            # detects amber, growler/k, q, python(numpy/pandas/duckdb/polars)
```

`run.sh` skips whatever isn't installed and prints one row per engine for each operation,
plus a PASS/FAIL sanity column comparing every engine's scalar results against Amber's.
Point it at a growler/k binary with `K=/path/to/growler ./run.sh`.

## 4. Parallelism: `peach` (multi-core)

`peach[f;y]` runs `f` over the items of `y` on a persistent pool of worker threads
(`src/peachpool.c`), sized by `AMBER_THREADS` (default: the online CPU count), and returns exactly what
serial `` f'y `` would. Workers read globals but can't assign them, so there's nothing to race on;
there's no fork and no copying a slice to a child any more (that's what the old 2-core numbers here
were paying for).

`{avg x?1.0}` over 8 items of 800k draws, best of 7, ms. Intel Core Ultra 7 255U (2 performance +
10 efficiency cores, 14 threads):

| `AMBER_THREADS` | serial `` f'y `` | `peach[f;y]` | speedup |
|---:|---:|---:|---:|
| 1 | 25.4 | 25.2 | 1.0x |
| 2 | 25.1 | 13.2 | **1.9x** |
| 4 | 24.6 | 8.9 | **2.8x** |
| 8 | 24.8 | 6.2 | **4.0x** |
| 14 | 25.3 | 6.3 | **4.0x** |

It flattens at 8 threads because there are only 8 items to hand out, and 6 of them land on
efficiency cores. A finer-grained real job does fine too: 400 baskets through a payoff kernel go
from 451 ms serial to 215 ms with one basket per task on 14 threads, same 400 answers.

**When to use it.** Anything where each item is real work (Monte-Carlo, per-symbol fits, partition
loads). For tiny items, chunk them so each task is at least tens of microseconds. `AMBER_THREADS=1`
forces serial.

## 5. Automated CI comparative benchmarks (Amber vs C, ngn/k, CBQN, J, Uiua, NumPy, Julia and DuckDB)

<!-- COMPARATIVE_BENCHMARKS:START (auto-generated by bench/run_comparative.py -- do not edit by hand) -->

_Median of 5 timed runs after 2 warm-up passes. Kernel time only — process startup is excluded (see below). Generated by `bench/run_comparative.py`; workloads defined in [`bench/SPEC.md`](../bench/SPEC.md)._

> ✅ Correctness gate passed: every engine produced the identical exact answer and the identical input checksum on every workload.

| Benchmark | C (-O3) | Amber | Amber qSQL | ngn/k | CBQN | J | Uiua | NumPy | Julia | DuckDB |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| Vector arithmetic + mask — `sum((x*2.5)+y where x>50)`, 10M | 9.16 | 17.36 | 40.94 | 307.46 | 39.20 | 148.05 | 840.66 | 35.39 | 9.39 | 28.00 |
| Reductions — `sum + max + dot`, 10M elements | 25.55 | 7.48 | 18.61 | 285.18 | 5.32 | 131.82 | 762.65 | 10.51 | 23.07 | 37.00 |
| Group-by aggregation — 100 groups over 10M rows | 7.05 | 13.54 | 17.00 | 317.74 | 37.45 | 186.04 | 1,754.24 | 13.78 | 7.78 | 19.00 |
| Inner join — 1M left rows against 1,000 sparse keys | 0.97 | 1.54 | 4.12 | 419.04 | 2.47 | 117.80 | _                            ─_ | 13.83 | 11.74 | 11.00 |

Relative to the C baseline (lower is better; 1.00× means it matched plain C):

| Benchmark | C (-O3) | Amber | Amber qSQL | ngn/k | CBQN | J | Uiua | NumPy | Julia | DuckDB |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| Vector arithmetic + mask — `sum((x*2.5)+y where x>50)`, 10M | 1.00× | 1.90× | 4.47× | 33.57× | 4.28× | 16.16× | 91.78× | 3.86× | 1.02× | 3.06× |
| Reductions — `sum + max + dot`, 10M elements | 1.00× | 0.29× | 0.73× | 11.16× | 0.21× | 5.16× | 29.85× | 0.41× | 0.90× | 1.45× |
| Group-by aggregation — 100 groups over 10M rows | 1.00× | 1.92× | 2.41× | 45.05× | 5.31× | 26.38× | 248.70× | 1.95× | 1.10× | 2.69× |
| Inner join — 1M left rows against 1,000 sparse keys | 1.00× | 1.58× | 4.25× | 431.56× | 2.55× | 121.32× | — | 14.24× | 12.09× | 11.33× |

**Timing mode per engine** — `kernel` means the engine timed its own kernel with a monotonic clock; `net` means it has no usable in-language clock and was measured as _total process time − startup baseline_:

| Engine | Peer group | Mode | Startup baseline (ms) |
|---|---|---|---:|
| C (-O3) | baseline | kernel | — |
| Amber | array primitives | kernel | — |
| Amber qSQL | query layer | kernel | — |
| ngn/k | array primitives | net | 2.13 |
| CBQN | array primitives | kernel | 3.38 |
| J | array primitives | net | 49.87 |
| Uiua | array primitives | net | 26.47 |
| NumPy | array primitives | kernel | — |
| Julia | scalar loops (JIT) | kernel | — |
| DuckDB | query layer | kernel | 13.88 |

Amber appears twice on purpose: `Amber` is array-primitive code (the fair peer of ngn/k, CBQN, J and Uiua) and `Amber qSQL` goes through the `select … by … from` layer (the fair peer of DuckDB's SQL planner). Reporting only the faster of the two would be choosing whichever comparison flatters Amber.

<!-- COMPARATIVE_BENCHMARKS:END -->


## 6. 1.9.2 self-benchmark: before vs after

Both columns are `bench/run_comparative.py --runs 5 --warmup 2` on the **same machine, same
data, same harness**, run once before the release and once after. Median kernel ms; lower
is better. (The two raw dumps these columns were read from are no longer checked in; CI
regenerates `bench/comparative_results.md` on every benchmark run.)

### Amber: array primitives (`bench/queries/amber_bench.k`)

| Workload | 1.9.1 | 1.9.2 | Change |
|---|---:|---:|---|
| Inner join, 1M rows against 1,000 sparse keys | 180.95 | **5.66** | **32.0x faster** |
| Reductions, sum + max + dot, 10M | 55.50 | 51.13 | 1.09x |
| Vector arithmetic + mask, 10M | 68.95 | 74.43 | no significant change |
| Group-by, 100 groups over 10M rows | 204.82 | 212.91 | no significant change |

### Amber: qSQL query layer (`bench/queries/amberq_bench.k`)

| Workload | 1.9.1 | 1.9.2 | Change |
|---|---:|---:|---|
| Inner join | 194.30 | **11.80** | **16.5x faster** |
| Vector arithmetic + mask | 118.36 | 90.43 | 1.31x |
| Reductions | 106.47 | 108.83 | no significant change |
| Group-by | 315.81 | 331.73 | no significant change |

**Read the small numbers with care.** On this 2-core sandbox the memory-bound workloads carry a
±15–20% run-to-run spread: a second independent sample of 1.9.2 gave arith 87.5, reduce 45.8,
groupby 179.7, join 5.26. Only the **join** change is far outside that band and can be called a
speed-up from the workload table alone; the arith/reduce/groupby movements above are noise, not
signal, and are reported as such rather than rounded into a multiplier.

### Micro-kernels: where the change is visible

Median of 5 timed passes after 2 warm-ups, measured directly and stable across repeats:

| Kernel | 1.9.1 | 1.9.2 | Change |
|---|---:|---:|---|
| `kr?kl`, 1M probes into 1,000 sparse keys | 211.4 | **2.6** | **81x faster** |
| `vr kr?kl`, probe + gather | 173.9 | **4.6** | **38x faster** |
| `+/px`, 10M float sum | 8.9 | **6.3** | 1.41x |
| `+/px*py`, 10M dot product | 18.7 | **15.8** | 1.18x |
| `\|/px`, 10M float max | 19.3 | 19.3 | unchanged (see below) |

### What did not change, and why

- **No expression fusion.** Amber evaluates eagerly, so `+/ (y+2.5*x) @ & x>50` has already
  materialised its intermediates before `+/` runs; fusing it needs a lazy/fusing evaluator, not a
  peephole match. A matcher narrow enough to ship here would have recognised roughly the benchmark
  expression and nothing else, exactly what §4 of [`bench/SPEC.md`](../bench/SPEC.md) forbids.
  The vector-arithmetic and group-by rows are therefore unchanged.
- **`|/` over floats still costs three passes.** It round-trips through the order-preserving
  `of1`/`of0` transforms, allocating two 80 MB temporaries, and dominates the reductions workload.
  Applying that transform inline to the raw bit patterns would give an identical result in one
  pass with no allocation, the clear next win, held back from this release because
  it touches float total-order and NaN semantics.
- **64-byte alignment produced no measurable gain** on this CPU (AVX2, no AVX-512). Payloads were
  already 32-byte aligned, which is what a 256-bit load wants; the change was kept because it is
  correct for AVX-512 and streaming stores, and it costs 32 bytes per allocation.

The correctness gate passed 100% on every run above: every engine produced the identical exact
answer and the identical input checksum on all four workloads.

## 7. 1.9.5 batch 2: sliding windows + radix sort

Both columns are `bench/suite.k` on the **same machine, same data, same harness**, minutes
apart: `bench/baseline_194_suite.txt` is the tree at commit `8027df8`, the right column is
the current build. Median kernel ms; lower is better. Every workload in the suite is listed,
including the ones that did not move, because a table that only shows the winners is an
advertisement rather than a benchmark.

### What moved

| workload | 8027df8 | 1.9.5 | speedup | |
|---|---:|---:|---:|---|
| `window` · `mmin[1000;x]`, window 1000 | 1,021 | **3.73** | **273.9x** | ██████████ |
| `window` · `mmax[100;x]`, moving max | 372 | **3.39** | **109.7x** | ████████░░ |
| `window` · `mmin[100;x]`, moving min | 383 | **3.80** | **101.0x** | ████████░░ |
| `window` · `mmin[10;x]`, window 10 | 314 | **3.84** | **81.7x** | ████████░░ |
| `tsort` · ``xasc[,`px;t]``, 1 float key, 3 cols | 1,617 | **54.6** | **29.6x** | ██████░░░░ |
| `tsort` · ``xdesc[`sym`time;t]``, 2 keys, descending | 1,086 | **42.4** | **25.6x** | ██████░░░░ |
| `tsort` · ``xasc[`sym`time;t]``, 2 keys, 3 cols | 1,039 | **42.1** | **24.7x** | ██████░░░░ |
| `join` · as-of join (50k trades x 100k quotes) | 67.3 | **4.20** | **16.0x** | █████░░░░░ |
| `window` · `mdev[100;x]`, moving stddev | 94.2 | **5.98** | **15.8x** | █████░░░░░ |
| `window` · `mavg[10;x]`, window 10 | 45.2 | **3.45** | **13.1x** | █████░░░░░ |
| `join` · as-of join (500k trades x 1M quotes) | 801 | **61.1** | **13.1x** | █████░░░░░ |
| `scale` · moving average (100), 1M rows | 45.9 | **3.53** | **13.0x** | ████░░░░░░ |
| `window` · `mavg[100;x]`, moving average | 45.9 | **3.62** | **12.7x** | ████░░░░░░ |
| `scale` · moving average (100), 100k rows | 4.04 | **0.336** | **12.0x** | ████░░░░░░ |
| `window` · `msum[100;x]`, moving sum | 41.4 | **3.47** | **11.9x** | ████░░░░░░ |
| `window` · `mavg[1000;x]`, window 1000 | 46.1 | **4.08** | **11.3x** | ████░░░░░░ |
| `core` · moving average (window 100) | 47.8 | **4.36** | **11.0x** | ████░░░░░░ |
| `sort` · sort an ALREADY-SORTED 1M float vector | 51.2 | **5.15** | **9.9x** | ████░░░░░░ |
| `scale` · moving average (100), 10M rows | 468 | **79.5** | **5.9x** | ███░░░░░░░ |
| `scale` · sort, 10M rows | 1,163 | **650** | **1.8x** | █░░░░░░░░░ |
| `scale` · sort, 100k rows | 3.80 | **2.98** | **1.3x** | █░░░░░░░░░ |
| `sort` · grade 1M floats, `iasc x` (argsort) | 54.9 | **43.7** | **1.3x** | █░░░░░░░░░ |
| `core` · sort (1M floats) | 65.1 | **55.9** | **1.2x** | █░░░░░░░░░ |

### What did not move (and was not expected to)

Nothing in this release touches `sum`, `filter`, `distinct`, group-by or the inner join.
Every row below is inside this sandbox's ±15–20% run-to-run spread on memory-bound work
(§6 documents the same band), in both directions.

| workload | 8027df8 | 1.9.5 | change |
|---|---:|---:|---|
| `scale` · sort, 1M rows | 57.6 | 51.5 | 1.12x faster |
| `sort` · sort 1M floats, `asc x` | 59.5 | 55.3 | 1.08x faster |
| `sort` · sort 1M integers, `asc x` | 31.1 | 29.8 | 1.05x faster |
| `card` · 1,000 groups | 11.7 | 11.4 | unchanged |
| `core` · `+/px`, sum | 0.338 | 0.345 | unchanged |
| `scale` · sum, 100k rows | 0.032 | 0.032 | unchanged |
| `scale` · sum, 1M rows | 0.339 | 0.361 | 1.07x slower |
| `join` · inner join (1M rows x 1,000 sparse keys) | 5.00 | 5.38 | 1.08x slower |
| `core` · distinct (100k distinct keys) | 50.3 | 54.6 | 1.09x slower |
| `scale` · group-by sum, 10M rows | 87.9 | 95.6 | 1.09x slower |
| `scale` · sum, 10M rows | 7.08 | 7.71 | 1.09x slower |
| `card` · 10 groups | 4.59 | 5.00 | 1.09x slower |
| `core` · `+/px>50`, filter + count | 1.41 | 1.58 | 1.12x slower |
| `scale` · group-by sum, 100k rows | 0.281 | 0.313 | 1.12x slower |
| `scale` · group-by sum, 1M rows | 4.44 | 5.07 | 1.14x slower |
| `card` · 100,000 groups | 148 | 170 | 1.14x slower |
| `core` · group-by sum (10 groups) | 4.59 | 5.49 | 1.20x slower |

**Why the as-of join moved 13–16x without anyone touching `aj`.** `aj` calls `ajord`, which
calls `xasc` when the right-hand book is not already in join order. The join's matcher was
already a native C kernel; the sort in front of it was not, and it dominated. That is the whole
change.

**Why `sortPre` moved 9.9x.** The radix kernel notices during its single key-extraction pass
that the column is already non-decreasing and returns the identity permutation: no histogram,
no permute. Real columns hit this constantly: anything carrying the `` `s `` attribute, anything
coming out of `ajord`, any second sort on the same key.

**Why the plain 1M `sort` rows barely moved.** The kernel this replaced (`ascZ`) was *already*
a byte-wise radix, so a single flat integer or float column only gains the ~1.3x visible on the
`iasc` row, and `asc x` then adds a fixed ~12 ms gather that this release does not touch. The
radix rewrite pays where the old kernel had no answer at all: multi-column keys (25–30x),
already-sorted input (9.9x), and clustered 64-bit keys such as nanosecond timestamps, where
range normalisation cuts eight passes to six. Closing the flat-column gap is §2.8 item 1, and
it is about the gather, not the sort.

Verification for this release: `tests/run_tests.sh`, `--asan` (ASan + UBSan + LeakSanitizer)
and `--tsan` all report ALL SUITES PASSED, including a new 297-assertion differential suite
(`tests/test_sort_window.k`) that checks every new kernel against the exact code it replaced.

---

<a name="comparative-benchmark-query-files"></a>
## Comparative benchmark query files

`bench/run_comparative.py` (see [docs/BENCHMARKS.md §5](docs/BENCHMARKS.md) for the live table,
refreshed on every push by CI) times **ten engines** on four workloads. The workloads, the data
model and the fairness rules are specified once, in **[`bench/SPEC.md`](bench/SPEC.md)**; every
engine implements that document and nothing else.

| engine | file | peer group |
|---|---|---|
| C (`gcc -O3 -march=native`) | `bench/queries/c_bench.c` | the floor everything is measured against |
| Amber (array primitives) | `bench/queries/amber_bench.k` | ngn/k · CBQN · J · Uiua |
| Amber (qSQL layer) | `bench/queries/amberq_bench.k` | DuckDB SQL |
| ngn/k | `bench/queries/k_<id>.k` | array primitives |
| CBQN | `bench/queries/bqn_<id>.bqn` | array primitives |
| J | `bench/queries/j_<id>.ijs` | array primitives |
| Uiua | `bench/queries/uiua_<id>.ua` | array primitives |
| NumPy | `bench/queries/numpy_bench.py` | array primitives |
| Julia | `bench/queries/julia_bench.jl` | scalar loops (JIT) |
| DuckDB | `bench/queries/duckdb_<id>.sql` | query layer |

Workloads: **vector arithmetic + boolean masking**, **reductions** (sum · max · dot over 10M
elements), **group-by aggregation** (100 groups over 10M rows), and an **inner join** (1M left
rows against 1,000 sparse keys).

<details>
<summary>How fairness is enforced, not just claimed</summary>

Every answer in this suite is an integer that fits in float64 exactly, and every sum is over such
integers, so the result is independent of summation order. SIMD pairwise, Kahan-compensated and
naive left-fold summation all produce the identical bit pattern. The harness therefore compares
answers **exactly** against the C reference and prints **WRONG** in place of a time for any engine
that disagrees. Each engine also emits a checksum of its *input* data, so a divergence in the
generator is caught separately (**BADDATA**). You cannot win a cell in this table by computing
something cheaper.

Two shortcuts the previous suite contained, both now removed and documented in `SPEC.md`:

* **`+/!10000000` is O(1) in Amber.** `src/3.c`'s `arf` constant-folds a sum over a *range* into
  the closed form `n(n-1)/2`. The old `vecsum` benchmark was exactly that expression, so Amber
  "won" it by never touching 10M elements while every other engine ran a real reduction. All
  data is now materialised before the clock starts.
* **A dense-key "join" is just an array index.** With right keys `0..K-1`, every array language
  answers the join with a single gather while DuckDB still builds a hash table. Right keys are
  now sparse and unsorted, forcing a genuine key lookup everywhere.

**Amber is reported twice, on purpose.** `Amber` is array-primitive code, the fair peer of
ngn/k, CBQN, J and Uiua. `Amber qSQL` routes the same workloads through the `select … by … from`
layer, the fair peer of DuckDB's SQL planner. Publishing only the faster of the two would mean
picking whichever comparison flatters Amber; the gap between the rows *is* the query layer's
overhead and is meant to be visible.

**Timing excludes startup.** Engines that can time their own kernel (Amber, C, NumPy, Julia,
DuckDB and CBQN, via `•MonoTime`) do so with a monotonic clock after warm-up passes, and the
harness uses that number directly. Engines with no usable in-language clock (ngn/k, Uiua, J) are
measured as *total process time − a measured startup baseline*. The results table labels which mode
produced each cell, so the two are never silently mixed.

</details>

The former lexer quirk that bit these files, where a `.k` comment line containing *only* a bare `/`
silently truncated everything after it, **is fixed in 2.0.0**: an unterminated bare-`/` block
comment now raises a clean parse error. See [docs/MISSING.md](docs/MISSING.md) §14.
