#!/usr/bin/env python3
"""bench/queries/numpy_bench.py -- Python + NumPy implementation of bench/SPEC.md.
GNU AGPLv3 - see LICENSE and NOTICE.

Run: python3 numpy_bench.py <arith|reduce|groupby|join> [runs] [warmup]

NumPy is eager, but a kernel whose result is never read can still be optimised
away by the interpreter at the Python level (dead local). Every kernel here
returns the scalar it computed and the driver stores it, so the full array
pipeline is materialised inside the timed region -- SPEC.md §4.2.
Data generation, dtype coercion and the join's lookup table are built before
the clock starts, matching every other engine.
"""
import sys, time, statistics
import numpy as np

N, M, K, G = 10_000_000, 1_000_000, 1_000, 100

# ---- data (SPEC.md §1) ---------------------------------------------------
i  = np.arange(N, dtype=np.int64)
h  = (262147 * i) % 1048573
a  = h % 1000
b  = h % 997
x  = a.astype(np.float64)
y  = b.astype(np.float64)
gk = a % G

kr = (7919 * np.arange(K, dtype=np.int64)) % 1048573
vr = 2.0 * np.arange(K, dtype=np.float64)
kl = kr[h[:M] % K]
vl = x[:M]

# Right-table lookup structure, built OUTSIDE the timer (SPEC.md §4.3): the
# array engines likewise get their right table materialised before timing.
order   = np.argsort(kr, kind="stable")
kr_sort = kr[order]
vr_sort = vr[order]

CHECK = int(a.sum()) + 3 * int(b.sum())


def k_arith():
    m = x > 50.0
    return float(((x * 2.5) + y)[m].sum())


def k_reduce():
    return float(x.sum()) + float(x.max()) + float(np.dot(x, y))


def k_groupby():
    gs = np.bincount(gk, weights=x, minlength=G)
    return float(((np.arange(G, dtype=np.float64) + 1.0) * gs).sum())


def k_join():
    # Real key lookup: binary search into the sorted right key column. kr is
    # sparse and unsorted by construction, so this cannot degrade to indexing.
    pos = np.searchsorted(kr_sort, kl)
    return float((vl * vr_sort[pos]).sum())


KERNELS = {"arith": k_arith, "reduce": k_reduce, "groupby": k_groupby, "join": k_join}


def main():
    bid = sys.argv[1] if len(sys.argv) > 1 else "reduce"
    runs = int(sys.argv[2]) if len(sys.argv) > 2 else 5
    warm = int(sys.argv[3]) if len(sys.argv) > 3 else 2
    kern = KERNELS.get(bid)
    if kern is None:
        print(f"unknown bench: {bid}", file=sys.stderr)
        return 2

    ans = None
    for _ in range(warm):
        ans = kern()
    times = []
    for _ in range(runs):
        t0 = time.perf_counter()
        ans = kern()
        times.append((time.perf_counter() - t0) * 1e3)

    print(f"BENCH {bid}")
    print(f"CHECK {CHECK}")
    print(f"ANSWER {ans!r}")
    print(f"TIME_MS {statistics.median(times):.4f}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
