#!/usr/bin/env python3
"""bench/suite.py - numpy/pandas, Polars and DuckDB side of the BENCHMARKS.md section 2 suite.

Run:  suite.py [numpy] [polars] [duckdb]      (no args = all three, in this process)
      ./bench/run_suite.sh runs it one engine per process, pinned, single-threaded.
Prints one line per workload:  RESULT <workload> <engine> <median_ms> <answer>
An engine whose module is missing is skipped with a SKIP line.

Data, checksums and timing mirror bench/suite.k exactly (see the block at its top):
closed formulas, no RNG; 2 warm-ups, 5 batches of n reps (batch >= ~20 ms), median
of the 5 batch means; answers come from the last warm-up result, outside the clock.
The "numpy" engine is the numpy/pandas column: numpy for vector ops, pandas for
windows, table sorts, group-by and joins.
"""
import os, sys, time, math, statistics, gc, warnings

for _v in ("POLARS_MAX_THREADS", "OMP_NUM_THREADS", "OPENBLAS_NUM_THREADS", "MKL_NUM_THREADS"):
    os.environ.setdefault(_v, "1")
warnings.filterwarnings("ignore")
import numpy as np

N = 1_000_000


# ---------------------------------------------------------------- harness
def bench(wid, engine, f, ck, mk=None):
    """mk: fresh-input mode, f(mk()) with mk() off the clock (see benchx in suite.k)."""
    def one():
        c = mk()
        t0 = time.perf_counter(); f(c)
        return time.perf_counter() - t0
    if mk is None:
        g = f
    else:
        g = lambda: f(mk())
    r = g(); r = g()                       # 2 warm-ups
    ans = ck(r); del r
    if mk is None:
        t0 = time.perf_counter(); f(); t1 = time.perf_counter() - t0
    else:
        t1 = one()
    n = max(1, math.ceil(0.020 / max(t1, 1e-6)))
    means = []
    for _ in range(5):
        if mk is None:
            t0 = time.perf_counter()
            for _ in range(n):
                f()
            means.append((time.perf_counter() - t0) * 1e3 / n)
        else:
            means.append(sum(one() for _ in range(n)) * 1e3 / n)
    print("RESULT %s %s %.6g %s" % (wid, engine, statistics.median(means), fmt(ans)), flush=True)
    gc.collect()


def fmt(a):
    if isinstance(a, (int, np.integer)):
        return str(int(a))
    return repr(float(a))


# ---------------------------------------------------------------- checksums (as suite.k)
def W(s):                                   # position-weighted sum: checks order
    s = np.asarray(s)
    w = np.arange(len(s), dtype=np.int64) % 1009
    return int((s * w).sum()) if s.dtype.kind in "iu" else float((s * w).sum())


def grk(keys, sums):                        # sum (1+key mod 251)*groupsum
    return float(((1 + np.asarray(keys) % 251) * np.asarray(sums)).sum())


def dck(d):                                 # distinct: count + sum
    d = np.asarray(d)
    return int(len(d) + d.sum())


def ckT1(px, tm):                           # 1-key table sort: tie-insensitive
    px, tm = np.asarray(px), np.asarray(tm)
    return W(px) + float((px * (tm % 1013)).sum())


def ckT2(px, tm):                           # unique keys: full row order
    px, tm = np.asarray(px), np.asarray(tm)
    w = np.arange(len(px), dtype=np.int64) % 1009
    return float((w * (px + tm % 1013)).sum())


# ---------------------------------------------------------------- data
def base():
    i = np.arange(N, dtype=np.int64)
    h = (2654435761 * i) % 100000
    return i, h, 0.001 * h


def table(i, px):
    names = np.array(["S%d" % k for k in range(5000)], dtype=object)
    return names[(7919 * i) % 5000], np.cumsum(1 + (7919 * i) % 5), px


def join_data(i, h):
    j = np.arange(1000, dtype=np.int64)
    rk = (104729 * j) % 10000019
    return rk[h % 1000], 1 + i % 7, rk, 1 + j          # lk, lv, rk, rv


AS = np.array(["S%d" % k for k in range(100)], dtype=object)


def trades(n):
    i = np.arange(n, dtype=np.int64)
    return AS[(7919 * i) % 100], np.cumsum(1 + (7919 * i) % 7)


def quotes(n):
    j = np.arange(n, dtype=np.int64)
    return AS[(104729 * j) % 100], np.cumsum(1 + (104729 * j) % 3), ((7907 * j) % 1000).astype(np.float64)


WIN = [("msum100", "sum", 100), ("mavg100", "mean", 100), ("mdev100", "std", 100),
       ("mmin100", "min", 100), ("mmax100", "max", 100), ("mavg10", "mean", 10),
       ("mavg1000", "mean", 1000), ("mmin10", "min", 10), ("mmin1000", "min", 1000)]
GRP = [("grp10", 10), ("grp1k", 1000), ("grp100k", 100000)]


# ================================================================== numpy / pandas
def run_numpy():
    import pandas as pd
    E = "numpy"
    i, h, px = base()
    ps = 0.001 * np.sort(h)                 # sorted px, built the same way as suite.k
    bench("sum", E, lambda: px.sum(), float)
    bench("filter", E, lambda: np.count_nonzero(px > 50.0), int)
    bench("distinct", E, lambda: np.unique(h), dck)

    s = pd.Series(px)
    for wid, op, w in WIN:                  # q semantics: growing window first, population std
        if op == "std":
            f = lambda w=w: s.rolling(w, min_periods=1).std(ddof=0)
        else:
            f = lambda w=w, op=op: getattr(s.rolling(w, min_periods=1), op)()
        bench(wid, E, f, lambda r: float(r.sum()))

    bench("sortf", E, lambda: np.sort(px), W)
    bench("sorti", E, lambda: np.sort(h), W)
    bench("gradef", E, lambda: np.argsort(px), lambda o: W(px[o]))
    bench("presorted", E, lambda c: np.sort(c), W, mk=lambda: ps * 1.0)

    for wid, G in GRP:
        df = pd.DataFrame({"g": h % G, "px": px})
        bench(wid, E, lambda df=df: df.groupby("g", sort=False)["px"].sum(),
              lambda r: grk(r.index.to_numpy(), r.to_numpy()))
    del df

    sym, tm, tpx = table(i, px)
    t = pd.DataFrame({"sym": sym, "time": tm, "px": tpx})
    bench("tsort_px", E, lambda: t.sort_values("px"), lambda r: ckT1(r["px"].to_numpy(), r["time"].to_numpy()))
    bench("tsort_st", E, lambda: t.sort_values(["sym", "time"]), lambda r: ckT2(r["px"].to_numpy(), r["time"].to_numpy()))
    bench("tdesc_st", E, lambda: t.sort_values(["sym", "time"], ascending=False),
          lambda r: ckT2(r["px"].to_numpy(), r["time"].to_numpy()))
    del t, sym

    lk, lv, rk, rv = join_data(i, h)
    L = pd.DataFrame({"k": lk, "lv": lv}); R = pd.DataFrame({"k": rk, "rv": rv})
    bench("ij", E, lambda: L.merge(R, on="k", how="inner"), lambda r: int((r["lv"] * r["rv"]).sum()))
    del L, R

    for wid, nt, nq in (("asof50k", 50_000, 100_000), ("asof500k", 500_000, 1_000_000)):
        ts, tt = trades(nt); qs, qt, qb = quotes(nq)
        T = pd.DataFrame({"sym": ts, "time": tt}); Q = pd.DataFrame({"sym": qs, "time": qt, "bid": qb})
        bench(wid, E, lambda T=T, Q=Q: pd.merge_asof(T, Q, on="time", by="sym", direction="backward"),
              lambda r: float(r["bid"].sum()))
        del T, Q


# ================================================================== Polars
def run_polars():
    import polars as pl
    E = "polars"
    i, h, px = base()
    df = pl.DataFrame({"h": h, "px": px})
    ps = 0.001 * np.sort(h)
    bench("sum", E, lambda: df.select(pl.col("px").sum()).item(), float)
    bench("filter", E, lambda: df.select((pl.col("px") > 50.0).sum()).item(), int)
    bench("distinct", E, lambda: df.select(pl.col("h").unique()), lambda r: dck(r["h"].to_numpy()))

    for wid, op, w in WIN:
        c = pl.col("px")
        e = (c.rolling_std(w, min_samples=1, ddof=0) if op == "std"
             else getattr(c, "rolling_" + op)(w, min_samples=1))
        bench(wid, E, lambda e=e: df.select(e), lambda r: float(r.to_series().sum()))

    bench("sortf", E, lambda: df.select(pl.col("px").sort()), lambda r: W(r["px"].to_numpy()))
    bench("sorti", E, lambda: df.select(pl.col("h").sort()), lambda r: W(r["h"].to_numpy()))
    bench("gradef", E, lambda: df.select(pl.col("px").arg_sort()), lambda r: W(px[r["px"].to_numpy()]))
    bench("presorted", E, lambda d: d.select(pl.col("ps").sort()), lambda r: W(r["ps"].to_numpy()),
          mk=lambda: pl.DataFrame({"ps": ps * 1.0}))

    for wid, G in GRP:
        gd = pl.DataFrame({"g": h % G, "px": px})
        bench(wid, E, lambda gd=gd: gd.group_by("g").agg(pl.col("px").sum()),
              lambda r: grk(r["g"].to_numpy(), r["px"].to_numpy()))
    del gd

    sym, tm, tpx = table(i, px)
    t = pl.DataFrame({"sym": pl.Series(sym.tolist(), dtype=pl.Utf8), "time": tm, "px": tpx})
    bench("tsort_px", E, lambda: t.sort("px"), lambda r: ckT1(r["px"].to_numpy(), r["time"].to_numpy()))
    bench("tsort_st", E, lambda: t.sort(["sym", "time"]), lambda r: ckT2(r["px"].to_numpy(), r["time"].to_numpy()))
    bench("tdesc_st", E, lambda: t.sort(["sym", "time"], descending=True),
          lambda r: ckT2(r["px"].to_numpy(), r["time"].to_numpy()))
    del t, sym

    lk, lv, rk, rv = join_data(i, h)
    L = pl.DataFrame({"k": lk, "lv": lv}); R = pl.DataFrame({"k": rk, "rv": rv})
    bench("ij", E, lambda: L.join(R, on="k", how="inner"), lambda r: int((r["lv"] * r["rv"]).sum()))
    del L, R

    for wid, nt, nq in (("asof50k", 50_000, 100_000), ("asof500k", 500_000, 1_000_000)):
        ts, tt = trades(nt); qs, qt, qb = quotes(nq)
        T = pl.DataFrame({"sym": pl.Series(ts.tolist(), dtype=pl.Utf8), "time": tt})
        Q = pl.DataFrame({"sym": pl.Series(qs.tolist(), dtype=pl.Utf8), "time": qt, "bid": qb})
        bench(wid, E, lambda T=T, Q=Q: T.join_asof(Q, on="time", by="sym", strategy="backward"),
              lambda r: float(r["bid"].sum()))
        del T, Q


# ================================================================== DuckDB
def run_duckdb():
    import duckdb, pandas as pd
    E = "duckdb"
    con = duckdb.connect()
    con.execute("SET threads TO 1")

    def load(name, **cols):
        con.register("_src", pd.DataFrame(cols))
        con.execute("CREATE OR REPLACE TABLE %s AS SELECT * FROM _src" % name)
        con.unregister("_src")

    def q(sql):                             # scalar query: one tiny row back
        return lambda: con.execute(sql).fetchone()[0]

    def ct(sql):                            # ordered / set result: materialised in the engine
        return lambda: con.execute("CREATE OR REPLACE TABLE r AS " + sql)

    def col(*names):
        d = con.execute("SELECT %s FROM r" % ",".join(names)).fetchnumpy()
        return [np.asarray(d[c]) for c in names]

    i, h, px = base()
    load("v", h=h, px=px, g10=h % 10, g1k=h % 1000)
    bench("sum", E, q("SELECT sum(px) FROM v"), float)
    bench("filter", E, q("SELECT count(*) FROM v WHERE px > 50"), int)
    bench("distinct", E, ct("SELECT DISTINCT h FROM v"), lambda _: dck(col("h")[0]))
    bench("sortf", E, ct("SELECT px FROM v ORDER BY px"), lambda _: W(col("px")[0]))
    bench("sorti", E, ct("SELECT h FROM v ORDER BY h"), lambda _: W(col("h")[0]))
    for wid, g in (("grp10", "g10"), ("grp1k", "g1k"), ("grp100k", "h")):
        bench(wid, E, ct("SELECT %s AS g, sum(px) AS s FROM v GROUP BY %s" % (g, g)),
              lambda _: grk(*col("g", "s")))

    sym, tm, tpx = table(i, px)
    load("t", sym=sym, time=tm, px=tpx)
    del sym
    for wid, order, ck in (("tsort_px", "px", ckT1), ("tsort_st", "sym, time", ckT2),
                           ("tdesc_st", "sym DESC, time DESC", ckT2)):
        bench(wid, E, ct("SELECT * FROM t ORDER BY " + order), lambda _, ck=ck: ck(*col("px", "time")))
    con.execute("DROP TABLE t")

    lk, lv, rk, rv = join_data(i, h)
    load("l", k=lk, lv=lv); load("rt", k=rk, rv=rv)
    bench("ij", E, ct("SELECT l.k, l.lv, rt.rv FROM l JOIN rt ON l.k = rt.k"),
          lambda _: int(con.execute("SELECT sum(lv*rv) FROM r").fetchone()[0]))
    con.execute("DROP TABLE l"); con.execute("DROP TABLE rt"); con.execute("DROP TABLE v")

    for wid, nt, nq in (("asof50k", 50_000, 100_000), ("asof500k", 500_000, 1_000_000)):
        ts, tt = trades(nt); qs, qt, qb = quotes(nq)
        load("trade", sym=ts, time=tt); load("quote", sym=qs, time=qt, bid=qb)
        bench(wid, E, ct("SELECT tr.sym, tr.time, qu.bid FROM trade tr ASOF LEFT JOIN quote qu "
                         "ON tr.sym = qu.sym AND tr.time >= qu.time"),
              lambda _: float(con.execute("SELECT sum(bid) FROM r").fetchone()[0]))
    con.close()


ENGINES = {"numpy": run_numpy, "polars": run_polars, "duckdb": run_duckdb}
MODULES = {"numpy": "pandas", "polars": "polars", "duckdb": "duckdb"}

if __name__ == "__main__":
    for name in (sys.argv[1:] or list(ENGINES)):
        try:
            __import__(MODULES[name])
        except ImportError as e:
            print("SKIP %s (%s)" % (name, e), flush=True)
            continue
        ENGINES[name]()
        gc.collect()
