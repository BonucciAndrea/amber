#!/usr/bin/env python3
"""bench/scout/engines/clickhouse_engine.py - ClickHouse (clickhouse-local) scout engine.

Run:  clickhouse_engine.py <op> <N> <runs> <warmup>
      (binary: $CLICKHOUSE_BIN, else ~/opt/bin/clickhouse, else `clickhouse` on PATH)

Protocol and data model: bench/scout/SCOUT_SPEC.md.  Standard library only.

Everything runs inside ONE `clickhouse local` process driven by one SQL script:

  1. SET max_threads = 1 (the single-thread comparison) and turn off the query
     result cache and the query condition cache, so no timed run can reuse the
     work of an earlier one.
  2. Generate the data INSIDE ClickHouse with the closed-form generator over
     numbers(N) into ENGINE = Memory tables (the analogue of DuckDB's in-memory
     tables), including every derived key column the other engines precompute.
  3. Run the kernel query <warmup> times, then <runs> times, each tagged with a
     log_comment.
  4. SYSTEM FLUSH LOGS and read the timed runs' durations back from
     system.query_log (enabled for this process by a two-line config file).

TIME_MS is ClickHouse's own measurement of each timed query, from query start to
QueryFinish (event_time_microseconds - query_start_time_microseconds): parse,
analysis, planning and execution of a one-row result.  Process start-up and data
generation are outside it.  This is the same span the DuckDB adapter times around
con.execute().fetchone(), minus the Python call overhead.

The SQL is the same shape as the DuckDB adapter's (py_engines.py): GROUP BY for
group_*, JOIN for find/join_inner, IN (subquery) for member, window functions for
scan_f, the sorts and the moving windows, ASOF JOIN for asof.  ClickHouse has no
lag(); lagInFrame() over an explicit frame is its equivalent.
"""
import os, re, shutil, statistics, subprocess, sys, tempfile

MOD, MUL = 1048573, 262147
KJOIN, MJOIN, NT = 1000, 1_000_000, 2_000_000
MQ, QP, MT = 200_000, 2000, 1_000_000
GROUPS = {"group_10": 10, "group_100": 100, "group_10k": 10000, "group_100k": 100000}

CONFIG = """<clickhouse>
  <query_log>
    <database>system</database>
    <table>query_log</table>
    <flush_interval_milliseconds>3600000</flush_interval_milliseconds>
  </query_log>
</clickhouse>
"""

SETTINGS = [
    "SET max_threads = 1",
    "SET use_query_cache = 0",
    "SET use_query_condition_cache = 0",
    # ClickHouse JIT-compiles an expression / aggregate (LLVM) only once it has
    # been seen 3 times by default, which put a 25-35 ms compile into the 2nd
    # timed run.  A threshold of 0 compiles on first use, i.e. inside the
    # warm-up passes, so the timed runs see the steady state the defaults reach
    # anyway -- the same role warm-up plays for Julia's JIT.
    "SET min_count_to_compile_expression = 0",
    "SET min_count_to_compile_aggregate_expression = 0",
]


def base_table(n):
    return ("CREATE TABLE v ENGINE = Memory AS SELECT toInt64(number) AS i, "
            "toInt64((%d * number) %% %d) AS h, h %% 1000 AS a, h %% 997 AS b, "
            "toFloat64(a) AS x, toFloat64(b) AS y FROM numbers(%d)" % (MUL, MOD, n))


RIGHT = ("CREATE TABLE r ENGINE = Memory AS SELECT toInt64((7919 * number) %% %d) AS k, "
         "2.0 * number AS vr, toInt64(number) AS j FROM numbers(%d)" % (MOD, KJOIN))

SORT_SQL = (
    "SELECT sumIf(s, rn IN (1, {q1}, {q2}, {q3}, {n})) + 1e9 * countIf(rn > 1 AND s < ps) "
    "FROM (SELECT x AS s, row_number() OVER w AS rn, lagInFrame(x) OVER w AS ps FROM {tbl} "
    "WINDOW w AS (ORDER BY x ASC ROWS BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW))")

TABLESORT_SQL = (
    "SELECT sumIf(px, rn IN (1, {q1}, {q2}, {q3}, {n})) + 1e9 * countIf(rn > 1 AND "
    "(sym < ps OR (sym = ps AND px < pp))) "
    "FROM (SELECT sym, px, row_number() OVER w AS rn, lagInFrame(sym) OVER w AS ps, "
    "lagInFrame(px) OVER w AS pp FROM t "
    "WINDOW w AS (ORDER BY sym ASC, px ASC ROWS BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW))")


def quartiles(n):
    """1-based row numbers of s[0], s[n div 4], s[n div 2], s[(3n) div 4], s[n-1]."""
    return dict(q1=n // 4 + 1, q2=n // 2 + 1, q3=(3 * n) // 4 + 1, n=n)


def plan(op, n):
    """Return (setup statements, kernel query) or None when the op is not implemented."""
    setup = [base_table(n)]          # every op prints CHECK from v
    if op == "sum_f":
        q = "SELECT sum(x) FROM v"
    elif op == "max_f":
        q = "SELECT max(x) FROM v"
    elif op == "dot":
        q = "SELECT sum(x * y) FROM v"
    elif op == "sum_i":
        q = "SELECT sum(a) FROM v"
    elif op == "arith_mask":
        q = "SELECT sum(y + 2.5 * x) FROM v WHERE x > 50"
    elif op == "scan_f":
        # SQL's running sum is an ordered window, like DuckDB's (SCOUT_SPEC scan_f note)
        q = ("SELECT sum(c) FROM (SELECT i, sum(x) OVER (ORDER BY i ASC ROWS BETWEEN "
             "UNBOUNDED PRECEDING AND CURRENT ROW) AS c FROM v) WHERE i IN (0, %d, %d)"
             % (n // 2, n - 1))
    elif op == "sort_f":
        q = SORT_SQL.format(tbl="v", **quartiles(n))
    elif op == "sort_presorted":
        setup.append("CREATE TABLE vp ENGINE = Memory AS SELECT x FROM v ORDER BY x")
        q = SORT_SQL.format(tbl="vp", **quartiles(n))
    elif op == "grade_i":
        q = "SELECT sum(i) FROM (SELECT i FROM v ORDER BY a ASC, i ASC LIMIT %d)" % min(1000, n)
    elif op == "find":
        setup += [RIGHT, "CREATE TABLE p ENGINE = Memory AS SELECT "
                         "toInt64((7919 * (h %% 1000)) %% %d) AS k FROM v" % MOD]
        q = "SELECT sum(r.j) FROM p INNER JOIN r ON p.k = r.k"
    elif op == "member":
        setup.append(RIGHT)
        q = "SELECT count() FROM v WHERE h IN (SELECT k FROM r)"
    elif op == "distinct":
        q = "SELECT 1e6 * count() + sum(d) FROM (SELECT DISTINCT a AS d FROM v)"
    elif op == "distinct_100k":
        setup.append("CREATE TABLE v5 ENGINE = Memory AS SELECT h % 100000 AS g5 FROM v")
        q = "SELECT 1e6 * count() + sum(d) FROM (SELECT DISTINCT g5 AS d FROM v5)"
    elif op in GROUPS:
        setup.append("CREATE TABLE gt ENGINE = Memory AS SELECT h %% %d AS g, x FROM v"
                     % GROUPS[op])
        q = "SELECT sum((1 + g % 251) * s) FROM (SELECT g, sum(x) AS s FROM gt GROUP BY g)"
    elif op == "join_inner":
        setup += [RIGHT,
                  "CREATE TABLE l ENGINE = Memory AS SELECT "
                  "toInt64((7919 * (hj %% 1000)) %% %d) AS k, toFloat64(hj %% 1000) AS vl "
                  "FROM (SELECT toInt64((%d * number) %% %d) AS hj FROM numbers(%d))"
                  % (MOD, MUL, MOD, MJOIN)]
        q = "SELECT sum(l.vl * r.vr) FROM l INNER JOIN r ON l.k = r.k"
    elif op == "msum_16":
        q = ("SELECT sum(w) FROM (SELECT sum(x) OVER (ORDER BY i ASC ROWS BETWEEN 15 "
             "PRECEDING AND CURRENT ROW) AS w FROM v)")
    elif op == "mavg_256":
        q = ("SELECT sum(w) FROM (SELECT avg(x) OVER (ORDER BY i ASC ROWS BETWEEN 255 "
             "PRECEDING AND CURRENT ROW) AS w FROM v)")
    elif op == "mmax_64":
        q = ("SELECT sum(w) FROM (SELECT max(x) OVER (ORDER BY i ASC ROWS BETWEEN 63 "
             "PRECEDING AND CURRENT ROW) AS w FROM v)")
    elif op in ("tablesort", "qsql_select"):
        setup.append("CREATE TABLE t ENGINE = Memory AS SELECT h2 %% 100 AS sym, "
                     "toFloat64(h2 %% 1000) AS px, h2 %% 500 AS sz FROM "
                     "(SELECT toInt64((%d * number) %% %d) AS h2 FROM numbers(%d))"
                     % (MUL, MOD, NT))
        if op == "tablesort":
            q = TABLESORT_SQL.format(**quartiles(NT))
        else:
            q = ("SELECT sum((1 + sym % 251) * s) FROM "
                 "(SELECT sym, sum(px) AS s FROM t WHERE sz > 250 GROUP BY sym)")
    elif op == "asof":
        setup += ["CREATE TABLE quote ENGINE = Memory AS SELECT "
                  "toInt64(intDiv(number, %d)) AS sym, toInt64(1 + 500 * (number %% %d)) AS time, "
                  "toFloat64((number %% %d) %% 1000) AS bid FROM numbers(%d)" % (QP, QP, QP, MQ),
                  "CREATE TABLE trade ENGINE = Memory AS SELECT "
                  "toInt64(((%d * number) %% %d) %% 100) AS sym, toInt64(1000 + number) AS time "
                  "FROM numbers(%d)" % (MUL, MOD, MT)]
        q = ("SELECT sum(q.bid) FROM trade AS tr ASOF JOIN quote AS q "
             "ON tr.sym = q.sym AND tr.time >= q.time")
    else:
        return None
    return setup, q


def clickhouse_bin():
    for c in (os.environ.get("CLICKHOUSE_BIN"),
              os.path.expanduser("~/opt/bin/clickhouse"), shutil.which("clickhouse")):
        if c and os.path.exists(c):
            return c
    return None


def main():
    if len(sys.argv) < 2:
        print("usage: clickhouse_engine.py <op> [N] [runs] [warmup]", file=sys.stderr)
        return 2
    op = sys.argv[1]
    n = int(sys.argv[2]) if len(sys.argv) > 2 else 10_000_000
    runs = int(sys.argv[3]) if len(sys.argv) > 3 else 5
    warm = int(sys.argv[4]) if len(sys.argv) > 4 else 2

    p = plan(op, n)
    ch = clickhouse_bin()
    if p is None or ch is None:
        print("SKIP %s%s" % (op, "" if ch else " (clickhouse binary not found)"))
        return 0
    setup, q = p

    stmts = list(SETTINGS) + setup
    stmts.append("SELECT 'CHECK', sum(a) + 3 * sum(b) FROM v")
    for _ in range(warm):
        stmts.append("%s SETTINGS log_comment = 'scout-warm'" % q)
    for _ in range(runs):
        stmts.append("%s SETTINGS log_comment = 'scout-timed'" % q)
    stmts.append("SYSTEM FLUSH LOGS")
    stmts.append("SELECT 'TIMED_US', dateDiff('microsecond', query_start_time_microseconds, "
                 "event_time_microseconds) FROM system.query_log WHERE type = 'QueryFinish' "
                 "AND log_comment = 'scout-timed' ORDER BY event_time_microseconds")

    work = tempfile.mkdtemp(prefix="scout-ch-")
    try:
        with open(os.path.join(work, "config.xml"), "w") as fh:
            fh.write(CONFIG)
        with open(os.path.join(work, "q.sql"), "w") as fh:
            fh.write(";\n".join(stmts) + ";\n")
        proc = subprocess.run([ch, "local", "--config-file", "config.xml",
                               "--queries-file", "q.sql", "--output-format", "TSV"],
                              cwd=work, stdin=subprocess.DEVNULL,
                              stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    finally:
        shutil.rmtree(work, ignore_errors=True)
    out = proc.stdout.decode("utf-8", "replace").splitlines()
    if proc.returncode != 0:
        sys.stderr.write(proc.stderr.decode("utf-8", "replace")[-2000:])
        return 1

    chk, answers, timed = None, [], []
    for line in out:
        cols = line.split("\t")
        if cols[0] == "CHECK":
            chk = cols[1]
        elif cols[0] == "TIMED_US":
            timed.append(int(cols[1]) / 1000.0)
        else:
            answers.append(cols[0])
    if chk is None or len(answers) != warm + runs or len(timed) != runs:
        sys.stderr.write("unexpected clickhouse output: %r\n" % out[:40])
        return 1
    print("BENCH   %s" % op)
    print("CHECK   %s" % chk)
    print("ANSWER  %.17g" % float(answers[-1]))
    print("TIME_MS %.6f" % sorted(timed)[runs // 2])
    return 0


if __name__ == "__main__":
    sys.exit(main())
