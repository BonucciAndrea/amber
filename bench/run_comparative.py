#!/usr/bin/env python3
"""
bench/run_comparative.py — comparative benchmark harness for Amber.

Runs the same three workloads across Amber, DuckDB (CLI), CBQN, and a K
interpreter (ngn/k, or kdb+/q as a fallback dialect), 5 times each, and
reports the median wall-clock time in milliseconds per engine plus a
speedup multiplier relative to Amber, as a Markdown table.

Workloads:
  1. vecsum      10,000,000-element vector sum                (`+/!10000000`)
  2. vecarith    1,000,000-element vector arithmetic + a tacit EMA scan
  3. groupby     columnar sum-by-group aggregation on a 1,000,000-row table

Engine binaries are found via environment variables, each with a sensible
default so this also runs unmodified on a laptop with the tools on PATH:

  AMBER_BIN   default: ./amber (relative to the repo root)
  DUCKDB_BIN  default: duckdb
  CBQN_BIN    default: cbqn (falls back to bqn)
  K_BIN       default: first of k, ngn-k, q found on PATH
  K_DIALECT   auto | k | q   (auto-detects by dialect-probing K_BIN)

Any engine whose binary isn't found is skipped (reported as "not installed"
in the table) rather than failing the whole run -- this script is meant to
degrade gracefully both on a bare laptop and in CI before the "download the
tools" step has run everything it possibly can.

Usage:
  bench/run_comparative.py                     # print the table to stdout
  bench/run_comparative.py --update-docs        # also patch docs/BENCHMARKS.md
  bench/run_comparative.py --out results.md     # also write the table to a file
  bench/run_comparative.py --runs 5             # override the run count (default 5)
"""
import argparse
import os
import shutil
import statistics
import subprocess
import sys
import time
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
QUERIES = Path(__file__).resolve().parent / "queries"

BENCHMARKS = [
    # (id, human label, timeout seconds per run)
    ("vecsum", "Vector sum — 10,000,000 elements (`+/!10000000`)", 30),
    ("vecarith", "Vector arithmetic + tacit EMA (1,000,000 elems, 50,000-elem scan)", 60),
    ("groupby", "Columnar group-by aggregation (1,000,000 rows, 10 groups)", 30),
]


def which(name):
    return shutil.which(name)


def find_amber():
    env = os.environ.get("AMBER_BIN")
    if env and Path(env).exists():
        return str(Path(env).resolve())
    cand = REPO_ROOT / "amber"
    if cand.exists():
        return str(cand)
    return which("amber")


def find_duckdb():
    env = os.environ.get("DUCKDB_BIN")
    if env and (Path(env).exists() or which(env)):
        return env
    for cand in ("/tmp/bench_tools/duckdb", "duckdb"):
        if Path(cand).exists() or which(cand):
            return cand
    return None


def find_cbqn():
    env = os.environ.get("CBQN_BIN")
    if env and (Path(env).exists() or which(env)):
        return env
    for cand in ("/tmp/bench_tools/cbqn", "cbqn", "bqn"):
        if Path(cand).exists() or which(cand):
            return cand
    return None


def find_k():
    env = os.environ.get("K_BIN")
    if env and (Path(env).exists() or which(env)):
        return env
    for cand in ("/tmp/bench_tools/k", "k", "ngn-k", "q"):
        if Path(cand).exists() or which(cand):
            return cand
    return None


def detect_k_dialect(k_bin):
    """ngn/k and kdb+/q share a lot of syntax but diverge on group/scan/mod.
    Probe with a tiny script that only parses cleanly in one dialect."""
    forced = os.environ.get("K_DIALECT", "auto")
    if forced in ("k", "q"):
        return forced
    if k_bin and Path(k_bin).name == "q":
        return "q"
    # k-dialect probe: `10!23` is "23 mod 10" in ngn/k -> 3. In q, `!` on two
    # ints with this arg order errors/behaves differently, so a clean "3" is
    # a strong signal we're talking to a k-family interpreter.
    try:
        r = subprocess.run([k_bin, "-e", "10!23"] if False else [k_bin],
                            input="10!23\n", capture_output=True, text=True, timeout=5)
        if "3" in r.stdout:
            return "k"
    except Exception:
        pass
    return "q"


def run_once(cmd, stdin_path=None, cwd=None, timeout=30):
    """Run one benchmark process, return elapsed wall-clock ms, or None on failure."""
    t0 = time.perf_counter()
    try:
        if stdin_path is not None:
            with open(stdin_path, "rb") as f:
                r = subprocess.run(cmd, stdin=f, capture_output=True, cwd=cwd, timeout=timeout)
        else:
            r = subprocess.run(cmd, capture_output=True, cwd=cwd, timeout=timeout)
    except subprocess.TimeoutExpired:
        return None, b"", b"TIMEOUT"
    except FileNotFoundError:
        return None, b"", b"NOT FOUND"
    elapsed_ms = (time.perf_counter() - t0) * 1000.0
    if r.returncode != 0:
        return None, r.stdout, r.stderr
    return elapsed_ms, r.stdout, r.stderr


def median_runs(cmd_fn, runs, timeout):
    """cmd_fn() -> (cmd_list, stdin_path or None). Returns (median_ms or None, last_error)."""
    times = []
    last_err = b""
    for _ in range(runs):
        cmd, stdin_path = cmd_fn()
        ms, out, err = run_once(cmd, stdin_path=stdin_path, cwd=str(REPO_ROOT), timeout=timeout)
        if ms is None:
            last_err = err
            continue
        times.append(ms)
    if not times:
        return None, last_err
    return statistics.median(times), b""


def bench_amber(amber_bin, bench_id, runs, timeout):
    if not amber_bin:
        return None, "not installed"
    qf = QUERIES / f"k_{bench_id}.k"
    med, err = median_runs(lambda: ([amber_bin, str(qf)], None), runs, timeout)
    return med, (err.decode(errors="replace").strip()[:120] if med is None else None)


def bench_k(k_bin, dialect, bench_id, runs, timeout):
    if not k_bin:
        return None, "not installed"
    if dialect == "q":
        qf = QUERIES / f"q_{bench_id}.q"
        med, err = median_runs(lambda: ([k_bin, str(qf), "-q"], None), runs, timeout)
    else:
        qf = QUERIES / f"k_{bench_id}.k"
        med, err = median_runs(lambda: ([k_bin, str(qf)], None), runs, timeout)
    return med, (err.decode(errors="replace").strip()[:120] if med is None else None)


def bench_bqn(bqn_bin, bench_id, runs, timeout):
    if not bqn_bin:
        return None, "not installed"
    qf = QUERIES / f"bqn_{bench_id}.bqn"
    med, err = median_runs(lambda: ([bqn_bin, str(qf)], None), runs, timeout)
    return med, (err.decode(errors="replace").strip()[:120] if med is None else None)


def bench_duckdb(duckdb_bin, bench_id, runs, timeout):
    if not duckdb_bin:
        return None, "not installed"
    qf = QUERIES / f"duckdb_{bench_id}.sql"
    med, err = median_runs(lambda: ([duckdb_bin, "-batch", ":memory:"], qf), runs, timeout)
    return med, (err.decode(errors="replace").strip()[:120] if med is None else None)


def fmt_ms(v):
    if v is None:
        return None
    return f"{v:,.2f}"


def fmt_speedup(amber_ms, other_ms):
    if amber_ms is None or other_ms is None:
        return "—"
    ratio = amber_ms / other_ms
    if ratio >= 1.0:
        return f"{ratio:.1f}× faster than Amber"
    return f"{1/ratio:.1f}× slower than Amber"


def build_table(results, runs):
    """results: {bench_id: {'amber':(ms,err), 'duckdb':(ms,err), 'cbqn':(ms,err), 'k':(ms,err,label)}}"""
    lines = []
    lines.append(f"_Median of {runs} runs per cell; process wall-clock time including interpreter startup. "
                  f"Generated by `bench/run_comparative.py`._")
    lines.append("")
    lines.append("| Benchmark | Amber (ms) | DuckDB (ms) | CBQN (ms) | K (ms) |")
    lines.append("|---|---:|---:|---:|---:|")
    for bench_id, label in [(b[0], b[1]) for b in BENCHMARKS]:
        row = results[bench_id]
        amber_ms, _ = row["amber"]
        duck_ms, duck_err = row["duckdb"]
        bqn_ms, bqn_err = row["cbqn"]
        k_ms, k_err = row["k"]

        def cell(ms, err):
            if ms is not None:
                return fmt_ms(ms)
            return f"_{err}_" if err else "_error_"

        lines.append(f"| {label} | {cell(amber_ms, None)} | {cell(duck_ms, duck_err)} | "
                      f"{cell(bqn_ms, bqn_err)} | {cell(k_ms, k_err)} |")
    lines.append("")
    lines.append("Speedup relative to Amber (>1× means the other engine is faster):")
    lines.append("")
    lines.append("| Benchmark | DuckDB | CBQN | K |")
    lines.append("|---|---|---|---|")
    for bench_id, label in [(b[0], b[1]) for b in BENCHMARKS]:
        row = results[bench_id]
        amber_ms, _ = row["amber"]
        duck_ms, _ = row["duckdb"]
        bqn_ms, _ = row["cbqn"]
        k_ms, _ = row["k"]
        lines.append(f"| {label} | {fmt_speedup(amber_ms, duck_ms)} | "
                      f"{fmt_speedup(amber_ms, bqn_ms)} | {fmt_speedup(amber_ms, k_ms)} |")
    return "\n".join(lines) + "\n"


START_MARK = "<!-- COMPARATIVE_BENCHMARKS:START (auto-generated by bench/run_comparative.py -- do not edit by hand) -->"
END_MARK = "<!-- COMPARATIVE_BENCHMARKS:END -->"


def update_docs(table_md):
    docs = REPO_ROOT / "docs" / "BENCHMARKS.md"
    section = (
        "\n## 5. Automated CI comparative benchmarks (Amber vs DuckDB vs CBQN vs K)\n\n"
        f"{START_MARK}\n\n{table_md}\n{END_MARK}\n"
    )
    if docs.exists():
        text = docs.read_text()
        if START_MARK in text and END_MARK in text:
            pre = text.split(START_MARK)[0].rstrip("\n")
            post = text.split(END_MARK)[1]
            new_section = f"{START_MARK}\n\n{table_md}\n{END_MARK}"
            text = pre + "\n\n" + new_section + post
        else:
            text = text.rstrip("\n") + "\n" + section
    else:
        text = "# Amber — sanity checks & benchmarks\n" + section
    docs.write_text(text)
    print(f"updated {docs}", file=sys.stderr)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--runs", type=int, default=5)
    ap.add_argument("--update-docs", action="store_true")
    ap.add_argument("--out", type=str, default=None)
    args = ap.parse_args()

    amber_bin = find_amber()
    duckdb_bin = find_duckdb()
    cbqn_bin = find_cbqn()
    k_bin = find_k()
    dialect = detect_k_dialect(k_bin) if k_bin else "k"

    print(f"amber:  {amber_bin or 'NOT FOUND'}", file=sys.stderr)
    print(f"duckdb: {duckdb_bin or 'NOT FOUND'}", file=sys.stderr)
    print(f"cbqn:   {cbqn_bin or 'NOT FOUND'}", file=sys.stderr)
    print(f"k:      {k_bin or 'NOT FOUND'} (dialect={dialect})", file=sys.stderr)

    results = {}
    for bench_id, label, timeout in BENCHMARKS:
        print(f"running {bench_id} ...", file=sys.stderr)
        results[bench_id] = {
            "amber": bench_amber(amber_bin, bench_id, args.runs, timeout),
            "duckdb": bench_duckdb(duckdb_bin, bench_id, args.runs, timeout),
            "cbqn": bench_bqn(cbqn_bin, bench_id, args.runs, timeout),
            "k": bench_k(k_bin, dialect, bench_id, args.runs, timeout),
        }

    table_md = build_table(results, args.runs)
    print(table_md)

    if args.out:
        Path(args.out).write_text(table_md)
        print(f"wrote {args.out}", file=sys.stderr)

    if args.update_docs:
        update_docs(table_md)


if __name__ == "__main__":
    main()
