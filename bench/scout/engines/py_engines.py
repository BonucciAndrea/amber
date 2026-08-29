#!/usr/bin/env python3
"""bench/scout/engines/py_engines.py - NumPy / pandas / Polars / DuckDB scout engines.

Run:  py_engines.py <engine> <op> <N> <runs> <warmup>
      engine in {numpy, pandas, polars, duckdb}

Protocol and data model: bench/scout/SCOUT_SPEC.md

Each engine uses the idiom a competent user of THAT engine would write, and the
report names the algorithm each one lands on -- that is the interesting part of
the comparison, so it is recorded rather than normalised away.  What is NOT
allowed is exploiting a property of this particular dataset (e.g. "the group
keys happen to be dense 0..C-1, so bincount them"); every group-by below works
for arbitrary keys.
"""
import sys, os, time, statistics, warnings
warnings.filterwarnings("ignore")

MOD, MUL = 1048573, 262147
KJOIN, MJOIN, NT = 1000, 1_000_000, 2_000_000
MQ, QP, MT, NSYM = 200_000, 2000, 1_000_000, 100

import numpy as np


# ---------------------------------------------------------------- base data
def gen_base(N):
    i = np.arange(N, dtype=np.int64)
    h = (MUL * i) % MOD
    a = h % 1000
    b = h % 997
    return h, a, b, a.astype(np.float64), b.astype(np.float64)


def sort_answer(s):
    n = len(s)
    ord_ = s[0] + s[n // 4] + s[n // 2] + s[(3 * n) // 4] + s[n - 1]
    inv = int(np.count_nonzero(s[1:] < s[:-1]))
    return float(ord_) + 1e9 * inv


def lex_answer(sp, rp):
    n = len(rp)
    ord_ = rp[0] + rp[n // 4] + rp[n // 2] + rp[(3 * n) // 4] + rp[n - 1]
    inv = np.count_nonzero((sp[1:] < sp[:-1]) |
                           ((sp[1:] == sp[:-1]) & (rp[1:] < rp[:-1])))
    return float(ord_) + 1e9 * int(inv)


def join_right():
    j = np.arange(KJOIN, dtype=np.int64)
    return (7919 * j) % MOD, 2.0 * j


# ================================================================== NumPy
def build_numpy(op, N):
    h, a, b, x, y = gen_base(N)
    chk = float(a.sum() + 3 * b.sum())

    def general_group(g, v):
        """No dense-key assumption: factorise, then bincount the codes."""
        keys, codes = np.unique(g, return_inverse=True)
        sums = np.bincount(codes, weights=v, minlength=len(keys))
        return float(((1 + keys % 251) * sums).sum())

    def hash_find(table, probe):
        """`?`-equivalent: position of each probe in an unsorted table."""
        order = np.argsort(table, kind="stable")
        pos = np.searchsorted(table[order], probe)
        return order[pos]

    if op == "sum_f":      f = lambda: float(x.sum())
    elif op == "max_f":    f = lambda: float(x.max())
    elif op == "dot":      f = lambda: float(x.dot(y))
    elif op == "sum_i":    f = lambda: float(a.sum())
    elif op == "arith_mask":
        f = lambda: float((y + 2.5 * x)[x > 50.0].sum())
    elif op == "sort_f":   f = lambda: sort_answer(np.sort(x))
    elif op == "sort_presorted":
        p = np.sort(x)
        f = lambda: sort_answer(np.sort(p))
    elif op == "grade_i":
        k = min(1000, N)
        f = lambda: float(np.argsort(a, kind="stable")[:k].sum())
    elif op == "find":
        kr, _ = join_right()
        pr = kr[h % KJOIN]
        f = lambda: float(hash_find(kr, pr).sum())
    elif op == "member":
        kr, _ = join_right()
        f = lambda: float(np.count_nonzero(np.isin(h, kr)))
    elif op == "distinct":
        f = lambda: 1e6 * len(np.unique(a)) + float(np.unique(a).sum())
    elif op == "distinct_100k":
        g5 = h % 100000
        f = lambda: 1e6 * len(np.unique(g5)) + float(np.unique(g5).sum())
    elif op.startswith("group_"):
        c = {"group_10": 10, "group_100": 100, "group_10k": 10000,
             "group_100k": 100000}[op]
        g = h % c
        f = lambda: general_group(g, x)
    elif op == "join_inner":
        kr, vr = join_right()
        hj = (MUL * np.arange(MJOIN, dtype=np.int64)) % MOD
        kl = kr[hj % KJOIN]
        vl = (hj % 1000).astype(np.float64)
        f = lambda: float((vl * vr[hash_find(kr, kl)]).sum())
    elif op == "msum_16":
        def f():
            c = np.concatenate(([0.0], np.cumsum(x)))
            lo = np.maximum(np.arange(len(x)) - 15, 0)
            return float((c[1:] - c[lo]).sum())
    elif op == "mavg_256":
        def f():
            c = np.concatenate(([0.0], np.cumsum(x)))
            idx = np.arange(len(x))
            lo = np.maximum(idx - 255, 0)
            return float(((c[1:] - c[lo]) / (idx - lo + 1)).sum())
    elif op == "mmax_64":
        from numpy.lib.stride_tricks import sliding_window_view
        def f():
            w = 64
            v = np.concatenate((np.full(w - 1, -np.inf), x))
            return float(sliding_window_view(v, w).max(axis=1).sum())
    elif op == "tablesort":
        ht = (MUL * np.arange(NT, dtype=np.int64)) % MOD
        sy, px = ht % 100, (ht % 1000).astype(np.float64)
        def f():
            o = np.lexsort((px, sy))
            return lex_answer(sy[o], px[o])
    elif op == "qsql_select":
        ht = (MUL * np.arange(NT, dtype=np.int64)) % MOD
        sy, px, sz = ht % 100, (ht % 1000).astype(np.float64), ht % 500
        def f():
            m = sz > 250
            return general_group(sy[m], px[m])
    elif op == "asof":
        qj = np.arange(MQ, dtype=np.int64)
        qs, qt = qj // QP, 1 + 500 * (qj % QP)
        qb = ((qj % QP) % 1000).astype(np.float64)
        htr = (MUL * np.arange(MT, dtype=np.int64)) % MOD
        ts, tt = htr % 100, 1000 + np.arange(MT, dtype=np.int64)
        cq = (qs << np.int64(32)) | qt      # (sym,time) lexicographic in one key
        ct = (ts << np.int64(32)) | tt
        def f():
            pos = np.searchsorted(cq, ct, side="right") - 1
            return float(qb[pos].sum())
    else:
        return None, chk
    return f, chk


# ================================================================== pandas
def build_pandas(op, N):
    import pandas as pd
    h, a, b, x, y = gen_base(N)
    chk = float(a.sum() + 3 * b.sum())
    sx, sy_, sa = pd.Series(x), pd.Series(y), pd.Series(a)

    if op == "sum_f":   f = lambda: float(sx.sum())
    elif op == "max_f": f = lambda: float(sx.max())
    elif op == "dot":   f = lambda: float(sx.dot(sy_))
    elif op == "sum_i": f = lambda: float(sa.sum())
    elif op == "arith_mask":
        f = lambda: float((sy_ + 2.5 * sx)[sx > 50.0].sum())
    elif op == "sort_f":
        f = lambda: sort_answer(sx.sort_values().to_numpy())
    elif op == "sort_presorted":
        p = pd.Series(np.sort(x))
        f = lambda: sort_answer(p.sort_values().to_numpy())
    elif op == "grade_i":
        k = min(1000, N)
        f = lambda: float(sa.argsort(kind="stable")[:k].sum())
    elif op == "distinct":
        f = lambda: 1e6 * sa.nunique() + float(pd.unique(sa).sum())
    elif op == "distinct_100k":
        g5 = pd.Series(h % 100000)
        f = lambda: 1e6 * g5.nunique() + float(pd.unique(g5).sum())
    elif op.startswith("group_"):
        c = {"group_10": 10, "group_100": 100, "group_10k": 10000,
             "group_100k": 100000}[op]
        df = pd.DataFrame({"g": h % c, "x": x})
        def f():
            s = df.groupby("g", sort=False)["x"].sum()
            return float(((1 + s.index.to_numpy() % 251) * s.to_numpy()).sum())
    elif op == "member":
        kr, _ = join_right()
        sh = pd.Series(h)
        f = lambda: float(sh.isin(kr).sum())
    elif op == "join_inner":
        kr, vr = join_right()
        hj = (MUL * np.arange(MJOIN, dtype=np.int64)) % MOD
        left = pd.DataFrame({"k": kr[hj % KJOIN], "vl": (hj % 1000).astype(np.float64)})
        right = pd.DataFrame({"k": kr, "vr": vr})
        def f():
            m = left.merge(right, on="k", how="inner")
            return float((m["vl"] * m["vr"]).sum())
    elif op == "msum_16":
        f = lambda: float(sx.rolling(16, min_periods=1).sum().sum())
    elif op == "mavg_256":
        f = lambda: float(sx.rolling(256, min_periods=1).mean().sum())
    elif op == "mmax_64":
        f = lambda: float(sx.rolling(64, min_periods=1).max().sum())
    elif op == "qsql_select":
        ht = (MUL * np.arange(NT, dtype=np.int64)) % MOD
        df = pd.DataFrame({"sym": ht % 100, "px": (ht % 1000).astype(np.float64),
                           "sz": ht % 500})
        def f():
            s = df[df.sz > 250].groupby("sym", sort=False)["px"].sum()
            return float(((1 + s.index.to_numpy() % 251) * s.to_numpy()).sum())
    elif op == "tablesort":
        ht = (MUL * np.arange(NT, dtype=np.int64)) % MOD
        df = pd.DataFrame({"sym": ht % 100, "px": (ht % 1000).astype(np.float64)})
        def f():
            r = df.sort_values(["sym", "px"], kind="stable")
            return lex_answer(r["sym"].to_numpy(), r["px"].to_numpy())
    elif op == "asof":
        qj = np.arange(MQ, dtype=np.int64)
        q = pd.DataFrame({"sym": qj // QP, "time": 1 + 500 * (qj % QP),
                          "bid": ((qj % QP) % 1000).astype(np.float64)})
        htr = (MUL * np.arange(MT, dtype=np.int64)) % MOD
        t = pd.DataFrame({"sym": htr % 100,
                          "time": 1000 + np.arange(MT, dtype=np.int64)})
        t = t.sort_values("time", kind="stable")
        q = q.sort_values("time", kind="stable")
        def f():
            m = pd.merge_asof(t, q, on="time", by="sym", direction="backward")
            return float(m["bid"].sum())
    else:
        return None, chk
    return f, chk


# ================================================================== Polars
def build_polars(op, N):
    import polars as pl
    h, a, b, x, y = gen_base(N)
    chk = float(a.sum() + 3 * b.sum())
    df = pl.DataFrame({"h": h, "a": a, "x": x, "y": y})

    if op == "sum_f":   f = lambda: float(df.select(pl.col("x").sum()).item())
    elif op == "max_f": f = lambda: float(df.select(pl.col("x").max()).item())
    elif op == "dot":
        f = lambda: float(df.select((pl.col("x") * pl.col("y")).sum()).item())
    elif op == "sum_i": f = lambda: float(df.select(pl.col("a").sum()).item())
    elif op == "arith_mask":
        f = lambda: float(df.filter(pl.col("x") > 50.0)
                            .select((pl.col("y") + 2.5 * pl.col("x")).sum()).item())
    elif op == "sort_f":
        f = lambda: sort_answer(df.select(pl.col("x").sort())["x"].to_numpy())
    elif op == "sort_presorted":
        p = pl.DataFrame({"x": np.sort(x)})
        f = lambda: sort_answer(p.select(pl.col("x").sort())["x"].to_numpy())
    elif op == "grade_i":
        k = min(1000, N)
        f = lambda: float(df.select(pl.col("a").arg_sort())["a"].to_numpy()[:k].sum())
    elif op == "distinct":
        def f():
            d = df.select(pl.col("a").unique())["a"]
            return 1e6 * len(d) + float(d.sum())
    elif op == "distinct_100k":
        d5 = pl.DataFrame({"g": h % 100000})
        def f():
            d = d5.select(pl.col("g").unique())["g"]
            return 1e6 * len(d) + float(d.sum())
    elif op.startswith("group_"):
        c = {"group_10": 10, "group_100": 100, "group_10k": 10000,
             "group_100k": 100000}[op]
        gd = pl.DataFrame({"g": h % c, "x": x})
        def f():
            r = gd.group_by("g").agg(pl.col("x").sum())
            return float(((1 + r["g"].to_numpy() % 251) * r["x"].to_numpy()).sum())
    elif op == "member":
        kr, _ = join_right()
        f = lambda: float(df.select(
            pl.col("h").is_in(pl.Series(kr).implode()).sum()).item())
    elif op == "join_inner":
        kr, vr = join_right()
        hj = (MUL * np.arange(MJOIN, dtype=np.int64)) % MOD
        L = pl.DataFrame({"k": kr[hj % KJOIN], "vl": (hj % 1000).astype(np.float64)})
        R = pl.DataFrame({"k": kr, "vr": vr})
        f = lambda: float(L.join(R, on="k", how="inner")
                           .select((pl.col("vl") * pl.col("vr")).sum()).item())
    elif op == "msum_16":
        f = lambda: float(df.select(
            pl.col("x").rolling_sum(16, min_samples=1).sum()).item())
    elif op == "mavg_256":
        f = lambda: float(df.select(
            pl.col("x").rolling_mean(256, min_samples=1).sum()).item())
    elif op == "mmax_64":
        f = lambda: float(df.select(
            pl.col("x").rolling_max(64, min_samples=1).sum()).item())
    elif op == "qsql_select":
        ht = (MUL * np.arange(NT, dtype=np.int64)) % MOD
        T = pl.DataFrame({"sym": ht % 100, "px": (ht % 1000).astype(np.float64),
                          "sz": ht % 500})
        def f():
            r = T.filter(pl.col("sz") > 250).group_by("sym").agg(pl.col("px").sum())
            return float(((1 + r["sym"].to_numpy() % 251) * r["px"].to_numpy()).sum())
    elif op == "tablesort":
        ht = (MUL * np.arange(NT, dtype=np.int64)) % MOD
        T = pl.DataFrame({"sym": ht % 100, "px": (ht % 1000).astype(np.float64)})
        def f():
            r = T.sort(["sym", "px"], maintain_order=True)
            return lex_answer(r["sym"].to_numpy(), r["px"].to_numpy())
    elif op == "asof":
        qj = np.arange(MQ, dtype=np.int64)
        Q = pl.DataFrame({"sym": qj // QP, "time": 1 + 500 * (qj % QP),
                          "bid": ((qj % QP) % 1000).astype(np.float64)}).sort("time")
        htr = (MUL * np.arange(MT, dtype=np.int64)) % MOD
        Tr = pl.DataFrame({"sym": htr % 100,
                           "time": 1000 + np.arange(MT, dtype=np.int64)}).sort("time")
        f = lambda: float(Tr.join_asof(Q, on="time", by="sym", strategy="backward")
                            .select(pl.col("bid").sum()).item())
    else:
        return None, chk
    return f, chk


# ================================================================== DuckDB
DUCK_SQL = {
    "sum_f":       "SELECT sum(x) FROM v",
    "max_f":       "SELECT max(x) FROM v",
    "dot":         "SELECT sum(x*y) FROM v",
    "sum_i":       "SELECT sum(a)::DOUBLE FROM v",
    "arith_mask":  "SELECT sum(y+2.5*x) FROM v WHERE x>50",
    "member":      "SELECT count(*)::DOUBLE FROM v WHERE h IN (SELECT k FROM r)",
    "distinct":    "SELECT 1e6*count(*)+sum(d) FROM (SELECT DISTINCT a AS d FROM v)",
    "distinct_100k":
        "SELECT 1e6*count(*)+sum(d) FROM (SELECT DISTINCT h%100000 AS d FROM v)",
    "join_inner":  "SELECT sum(l.vl*r.vr) FROM l JOIN r ON l.k=r.k",
    "find":        "SELECT sum(r.j)::DOUBLE FROM p JOIN r ON p.k=r.k",
    "msum_16":
        "SELECT sum(w) FROM (SELECT sum(x) OVER (ORDER BY i "
        "ROWS BETWEEN 15 PRECEDING AND CURRENT ROW) AS w FROM v)",
    "mavg_256":
        "SELECT sum(w) FROM (SELECT avg(x) OVER (ORDER BY i "
        "ROWS BETWEEN 255 PRECEDING AND CURRENT ROW) AS w FROM v)",
    "mmax_64":
        "SELECT sum(w) FROM (SELECT max(x) OVER (ORDER BY i "
        "ROWS BETWEEN 63 PRECEDING AND CURRENT ROW) AS w FROM v)",
    "qsql_select":
        "SELECT sum((1+sym%251)*s) FROM "
        "(SELECT sym, sum(px) AS s FROM t WHERE sz>250 GROUP BY sym)",
    "asof":
        "SELECT sum(q.bid) FROM trade tr ASOF JOIN quote q "
        "ON tr.sym=q.sym AND tr.time>=q.time",
}

SORT_SQL = """
WITH s AS (SELECT x AS v, row_number() OVER (ORDER BY x) AS rn,
                  lag(x) OVER (ORDER BY x) AS pv FROM {tbl}),
     n AS (SELECT count(*) AS c FROM s)
SELECT (SELECT sum(v) FROM s, n WHERE rn IN (1, c/4+1, c/2+1, (3*c)/4+1, c))
     + 1e9*(SELECT count(*) FROM s WHERE pv IS NOT NULL AND v<pv)"""

TABLESORT_SQL = """
WITH s AS (SELECT sym, px, row_number() OVER (ORDER BY sym, px) AS rn,
                  lag(sym) OVER (ORDER BY sym, px) AS ps,
                  lag(px)  OVER (ORDER BY sym, px) AS pp
           FROM t),
     n AS (SELECT count(*) AS c FROM s)
SELECT (SELECT sum(px) FROM s, n WHERE rn IN (1, c/4+1, c/2+1, (3*c)/4+1, c))
     + 1e9*(SELECT count(*) FROM s
            WHERE ps IS NOT NULL AND (sym<ps OR (sym=ps AND px<pp)))"""


def build_duckdb(op, N):
    import duckdb, pandas as pd
    h, a, b, x, y = gen_base(N)
    chk = float(a.sum() + 3 * b.sum())
    con = duckdb.connect()
    con.execute("SET threads TO 1")
    i = np.arange(N, dtype=np.int64)

    if op in ("sum_f", "max_f", "dot", "sum_i", "arith_mask", "distinct",
              "distinct_100k", "msum_16", "mavg_256", "mmax_64", "member",
              "sort_f", "sort_presorted", "grade_i"):
        con.register("vv", pd.DataFrame({"i": i, "h": h, "a": a, "x": x, "y": y}))
        con.execute("CREATE TABLE v AS SELECT * FROM vv")
    if op in ("member", "join_inner", "find"):
        kr, vr = join_right()
        con.register("rr", pd.DataFrame(
            {"k": kr, "vr": vr, "j": np.arange(KJOIN, dtype=np.int64)}))
        con.execute("CREATE TABLE r AS SELECT * FROM rr")
    if op == "join_inner":
        hj = (MUL * np.arange(MJOIN, dtype=np.int64)) % MOD
        kr, _ = join_right()
        con.register("ll", pd.DataFrame(
            {"k": kr[hj % KJOIN], "vl": (hj % 1000).astype(np.float64)}))
        con.execute("CREATE TABLE l AS SELECT * FROM ll")
    if op == "find":
        kr, _ = join_right()
        con.register("pp", pd.DataFrame({"k": kr[h % KJOIN]}))
        con.execute("CREATE TABLE p AS SELECT * FROM pp")
    if op in ("qsql_select", "tablesort"):
        ht = (MUL * np.arange(NT, dtype=np.int64)) % MOD
        con.register("tt", pd.DataFrame(
            {"sym": ht % 100, "px": (ht % 1000).astype(np.float64), "sz": ht % 500}))
        con.execute("CREATE TABLE t AS SELECT * FROM tt")
    if op == "asof":
        qj = np.arange(MQ, dtype=np.int64)
        con.register("qq", pd.DataFrame(
            {"sym": qj // QP, "time": 1 + 500 * (qj % QP),
             "bid": ((qj % QP) % 1000).astype(np.float64)}))
        con.execute("CREATE TABLE quote AS SELECT * FROM qq")
        htr = (MUL * np.arange(MT, dtype=np.int64)) % MOD
        con.register("trr", pd.DataFrame(
            {"sym": htr % 100, "time": 1000 + np.arange(MT, dtype=np.int64)}))
        con.execute("CREATE TABLE trade AS SELECT * FROM trr")

    if op.startswith("group_"):
        c = {"group_10": 10, "group_100": 100, "group_10k": 10000,
             "group_100k": 100000}[op]
        con.register("gg", pd.DataFrame({"g": h % c, "x": x}))
        con.execute("CREATE TABLE g AS SELECT * FROM gg")
        sql = ("SELECT sum((1+g%251)*s) FROM "
               "(SELECT g, sum(x) AS s FROM g GROUP BY g)")
    elif op == "sort_f":
        sql = SORT_SQL.format(tbl="v")
    elif op == "sort_presorted":
        con.execute("CREATE TABLE vp AS SELECT x FROM v ORDER BY x")
        sql = SORT_SQL.format(tbl="vp")
    elif op == "grade_i":
        k = min(1000, N)
        sql = "SELECT sum(i)::DOUBLE FROM (SELECT i FROM v ORDER BY a, i LIMIT %d)" % k
    elif op == "tablesort":
        sql = TABLESORT_SQL
    elif op in DUCK_SQL:
        sql = DUCK_SQL[op]
    else:
        con.close()
        return None, chk

    def f():
        return float(con.execute(sql).fetchone()[0])
    return f, chk


# ================================================================== driver
BUILDERS = {"numpy": build_numpy, "pandas": build_pandas,
            "polars": build_polars, "duckdb": build_duckdb}


def main():
    if len(sys.argv) < 3:
        print("usage: py_engines.py <engine> <op> [N] [runs] [warmup]", file=sys.stderr)
        return 2
    engine, op = sys.argv[1], sys.argv[2]
    N = int(sys.argv[3]) if len(sys.argv) > 3 else 10_000_000
    runs = int(sys.argv[4]) if len(sys.argv) > 4 else 5
    warm = int(sys.argv[5]) if len(sys.argv) > 5 else 2

    try:
        f, chk = BUILDERS[engine](op, N)
    except ImportError as e:
        print("SKIP %s (%s)" % (op, e))
        return 0
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
    print("ANSWER  %.17g" % ans)
    print("TIME_MS %.6f" % statistics.median(ts))
    return 0


if __name__ == "__main__":
    sys.exit(main())
