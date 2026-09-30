#!/usr/bin/env python3
"""bench/scout/engines/numba_engine.py - Numba ("compiled NumPy") scout engine.

Run:  numba_engine.py <op> <N> <runs> <warmup>
Protocol and data model: bench/scout/SCOUT_SPEC.md; data generation is shared
with py_engines.py (same NumPy arrays, built outside the timed region).

Numba's idiom is the explicit loop, compiled by LLVM: every kernel below is a
single-threaded @njit function (no parallel=True, no fastmath, so float
reductions keep their sequential order -- as in the -O3 C reference, which is
not built with -ffast-math either).  Hash-based ops use Numba's typed Dict /
set, built INSIDE the kernel; sorts use np.sort / np.argsort(kind="mergesort")
(stable) inside njit; the moving max is the monotonic-deque loop; the as-of
join is a per-trade binary search over that symbol's quote range.

JIT compilation happens on the first call, i.e. inside the warm-up passes;
cache=False so nothing is written next to this file.
"""
import sys, time, statistics, os, warnings
warnings.filterwarnings("ignore")
sys.dont_write_bytecode = True      # importing py_engines must not drop a __pycache__ in the repo
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import numpy as np
from numba import njit, types
from numba.typed import Dict

from py_engines import (gen_base, join_right, MOD, MUL, KJOIN, MJOIN, NT,
                        MQ, QP, MT)


# ------------------------------------------------------------------ helpers
@njit
def sort_answer(s):
    n = s.shape[0]
    o = s[0] + s[n // 4] + s[n // 2] + s[(3 * n) // 4] + s[n - 1]
    inv = 0
    for j in range(1, n):
        if s[j] < s[j - 1]:
            inv += 1
    return o + 1e9 * inv


@njit
def lex_answer(sp, rp):
    n = rp.shape[0]
    o = rp[0] + rp[n // 4] + rp[n // 2] + rp[(3 * n) // 4] + rp[n - 1]
    inv = 0
    for j in range(1, n):
        if sp[j] < sp[j - 1] or (sp[j] == sp[j - 1] and rp[j] < rp[j - 1]):
            inv += 1
    return o + 1e9 * inv


@njit
def group_sum(g, v):
    d = Dict.empty(key_type=types.int64, value_type=types.float64)
    for i in range(g.shape[0]):
        k = g[i]
        d[k] = d.get(k, 0.0) + v[i]
    s = 0.0
    for k, t in d.items():
        s += (1 + k % 251) * t
    return s


# ------------------------------------------------------------------ kernels
@njit
def k_sum_f(x):
    s = 0.0
    for i in range(x.shape[0]):
        s += x[i]
    return s


@njit
def k_max_f(x):
    m = x[0]
    for i in range(x.shape[0]):
        if x[i] > m:
            m = x[i]
    return m


@njit
def k_scan_f(x):
    n = x.shape[0]
    o = np.empty(n)
    acc = 0.0
    for i in range(n):
        acc += x[i]
        o[i] = acc
    return o[0] + o[n // 2] + o[n - 1]


@njit
def k_dot(x, y):
    s = 0.0
    for i in range(x.shape[0]):
        s += x[i] * y[i]
    return s


@njit
def k_sum_i(a):
    s = 0
    for i in range(a.shape[0]):
        s += a[i]
    return float(s)


@njit
def k_arith(x, y):
    s = 0.0
    for i in range(x.shape[0]):
        if x[i] > 50.0:
            s += y[i] + 2.5 * x[i]
    return s


@njit
def k_sort(x):
    return sort_answer(np.sort(x))


@njit
def k_grade(a, k):
    p = np.argsort(a, kind="mergesort")          # stable
    s = 0
    for j in range(k):
        s += p[j]
    return float(s)


@njit
def k_find(kr, probe):
    d = Dict.empty(key_type=types.int64, value_type=types.int64)
    for j in range(kr.shape[0]):
        if kr[j] not in d:
            d[kr[j]] = j
    s = 0
    for i in range(probe.shape[0]):
        s += d[probe[i]]
    return float(s)


@njit
def k_member(h, kr):
    st = set()
    for j in range(kr.shape[0]):
        st.add(kr[j])
    c = 0
    for i in range(h.shape[0]):
        if h[i] in st:
            c += 1
    return float(c)


@njit
def k_distinct(a):
    st = set()
    cnt = 0
    sm = 0
    for i in range(a.shape[0]):
        v = a[i]
        if v not in st:
            st.add(v)
            cnt += 1
            sm += v
    return 1e6 * cnt + sm


@njit
def k_join(kl, vl, kr, vr):
    d = Dict.empty(key_type=types.int64, value_type=types.float64)
    for j in range(kr.shape[0]):
        d[kr[j]] = vr[j]
    s = 0.0
    for i in range(kl.shape[0]):
        k = kl[i]
        if k in d:
            s += vl[i] * d[k]
    return s


@njit
def k_msum(x, w):
    n = x.shape[0]
    o = np.empty(n)
    run = 0.0
    s = 0.0
    for i in range(n):
        run += x[i]
        if i >= w:
            run -= x[i - w]
        o[i] = run
        s += run
    return s


@njit
def k_mavg(x, w):
    n = x.shape[0]
    o = np.empty(n)
    run = 0.0
    s = 0.0
    for i in range(n):
        run += x[i]
        if i >= w:
            run -= x[i - w]
        c = i + 1 if i + 1 < w else w
        o[i] = run / c
        s += o[i]
    return s


@njit
def k_mmax(x, w):
    n = x.shape[0]
    dq = np.empty(n, dtype=np.int64)
    o = np.empty(n)
    head = 0
    tail = 0
    s = 0.0
    for i in range(n):
        while tail > head and x[dq[tail - 1]] <= x[i]:
            tail -= 1
        dq[tail] = i
        tail += 1
        if dq[head] <= i - w:
            head += 1
        o[i] = x[dq[head]]
        s += o[i]
    return s


@njit
def k_tablesort(sy, px):
    p = np.argsort(px, kind="mergesort")         # stable LSD: minor key first,
    q = p[np.argsort(sy[p], kind="mergesort")]   # then a stable pass on the major key
    return lex_answer(sy[q], px[q])


@njit
def k_qsql(sy, px, sz):
    d = Dict.empty(key_type=types.int64, value_type=types.float64)
    for i in range(sy.shape[0]):
        if sz[i] > 250:
            k = sy[i]
            d[k] = d.get(k, 0.0) + px[i]
    s = 0.0
    for k, t in d.items():
        s += (1 + k % 251) * t
    return s


@njit
def k_asof(qs, qt, qb, ts, tt):
    # quotes are sorted by (sym, time): find each symbol's range once, then
    # binary-search the last quote with time <= trade time inside it
    nq = qs.shape[0]
    s = 0.0
    for i in range(ts.shape[0]):
        lo, hi = 0, nq                           # first row with qs >= sym
        while lo < hi:
            mid = (lo + hi) // 2
            if qs[mid] < ts[i]:
                lo = mid + 1
            else:
                hi = mid
        start = lo
        hi = nq                                  # last row with (qs, qt) <= (sym, t)
        while lo < hi:
            mid = (lo + hi) // 2
            if qs[mid] == ts[i] and qt[mid] <= tt[i]:
                lo = mid + 1
            else:
                hi = mid
        if lo > start:
            s += qb[lo - 1]
    return s


# ------------------------------------------------------------------ builder
def build(op, N):
    h, a, b, x, y = gen_base(N)
    chk = float(a.sum() + 3 * b.sum())
    groups = {"group_10": 10, "group_100": 100, "group_10k": 10000, "group_100k": 100000}
    if op == "sum_f":            f = lambda: k_sum_f(x)
    elif op == "max_f":          f = lambda: k_max_f(x)
    elif op == "scan_f":         f = lambda: k_scan_f(x)
    elif op == "dot":            f = lambda: k_dot(x, y)
    elif op == "sum_i":          f = lambda: k_sum_i(a)
    elif op == "arith_mask":     f = lambda: k_arith(x, y)
    elif op == "sort_f":         f = lambda: k_sort(x)
    elif op == "sort_presorted":
        p = np.sort(x)
        f = lambda: k_sort(p)
    elif op == "grade_i":
        k = min(1000, N)
        f = lambda: k_grade(a, k)
    elif op == "find":
        kr, _ = join_right()
        pr = kr[h % KJOIN]
        f = lambda: k_find(kr, pr)
    elif op == "member":
        kr, _ = join_right()
        f = lambda: k_member(h, kr)
    elif op == "distinct":       f = lambda: k_distinct(a)
    elif op == "distinct_100k":
        g5 = h % 100000
        f = lambda: k_distinct(g5)
    elif op in groups:
        g = h % groups[op]
        f = lambda: group_sum(g, x)
    elif op == "join_inner":
        kr, vr = join_right()
        hj = (MUL * np.arange(MJOIN, dtype=np.int64)) % MOD
        kl, vl = kr[hj % KJOIN], (hj % 1000).astype(np.float64)
        f = lambda: k_join(kl, vl, kr, vr)
    elif op == "msum_16":        f = lambda: k_msum(x, 16)
    elif op == "mavg_256":       f = lambda: k_mavg(x, 256)
    elif op == "mmax_64":        f = lambda: k_mmax(x, 64)
    elif op in ("tablesort", "qsql_select"):
        ht = (MUL * np.arange(NT, dtype=np.int64)) % MOD
        sy, px, sz = ht % 100, (ht % 1000).astype(np.float64), ht % 500
        f = (lambda: k_tablesort(sy, px)) if op == "tablesort" else (lambda: k_qsql(sy, px, sz))
    elif op == "asof":
        qj = np.arange(MQ, dtype=np.int64)
        qs, qt = qj // QP, 1 + 500 * (qj % QP)
        qb = ((qj % QP) % 1000).astype(np.float64)
        htr = (MUL * np.arange(MT, dtype=np.int64)) % MOD
        ts, tt = htr % 100, 1000 + np.arange(MT, dtype=np.int64)
        f = lambda: k_asof(qs, qt, qb, ts, tt)
    else:
        return None, chk
    return f, chk


def main():
    op = sys.argv[1]
    N = int(sys.argv[2]) if len(sys.argv) > 2 else 10_000_000
    runs = int(sys.argv[3]) if len(sys.argv) > 3 else 5
    warm = int(sys.argv[4]) if len(sys.argv) > 4 else 2
    f, chk = build(op, N)
    if f is None:
        print("SKIP %s" % op)
        return 0
    ans = 0.0
    for _ in range(warm):
        ans = f()
    ts = []
    for _ in range(runs):
        t0 = time.perf_counter()
        ans = f()
        ts.append((time.perf_counter() - t0) * 1e3)
    print("BENCH   %s" % op)
    print("CHECK   %.0f" % chk)
    print("ANSWER  %.17g" % float(ans))
    print("TIME_MS %.6f" % statistics.median(ts))
    return 0


if __name__ == "__main__":
    sys.exit(main())
