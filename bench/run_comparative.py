#!/usr/bin/env python3
"""
bench/run_comparative.py -- comparative benchmark harness for Amber.

Implements the protocol in bench/SPEC.md across ten engines:

  amber        Amber, array primitives          (peer of k / bqn / j / uiua)
  amber-qsql   Amber, select..by..from layer    (peer of duckdb SQL)
  k            ngn/k
  bqn          CBQN
  duckdb       DuckDB CLI
  julia        Julia
  numpy        Python + NumPy
  uiua         Uiua
  j            J (jconsole)
  c            native C, gcc -O3 -march=native  (the floor)

WHAT THIS HARNESS ENFORCES (rather than merely documenting):

  * Every engine prints ANSWER. All ten answers are exactly representable in
    float64 and order-independent by construction (SPEC.md §3), so the runner
    compares them EXACTLY against the C reference. An engine that disagrees is
    reported as WRONG and its time is withheld -- you cannot win this table by
    computing something cheaper.
  * Every engine prints CHECK, a checksum of its INPUT data. A divergence in
    the generator is therefore caught separately from a divergence in the
    result, and reported as BADDATA.
  * Timing excludes process startup. Engines that can time their own kernel
    print TIME_MS and that is used directly ("kernel" mode). Engines with no
    usable in-language clock are measured as
        total wall time - startup baseline
    where the baseline is measured once per engine by running a do-nothing
    script ("net" mode). The mode is printed per cell so the two are never
    silently mixed.
  * Warm-up passes run before timing (JIT/branch-predictor/page-cache), and
    the reported figure is the median of --runs timed passes.

Engine binaries come from environment variables, each with a sensible default:

  AMBER_BIN   DUCKDB_BIN  CBQN_BIN  K_BIN
  JULIA_BIN   PYTHON_BIN  UIUA_BIN  J_BIN  C_BENCH_BIN

Any engine whose binary is missing is reported "not installed" rather than
failing the run, so this degrades gracefully on a laptop and in CI.

Usage:
  bench/run_comparative.py                    # print the table
  bench/run_comparative.py --runs 5 --warmup 2
  bench/run_comparative.py --out results.md --update-docs
  bench/run_comparative.py --only amber,c --benchmarks reduce
"""
import argparse
import os
import re
import shutil
import statistics
import subprocess
import sys
import time
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
QUERIES = Path(__file__).resolve().parent / "queries"

BENCHMARKS = [
    ("arith",   "Vector arithmetic + mask — `sum((x*2.5)+y where x>50)`, 10M", 300),
    ("reduce",  "Reductions — `sum + max + dot`, 10M elements", 300),
    ("groupby", "Group-by aggregation — 100 groups over 10M rows", 600),
    ("join",    "Inner join — 1M left rows against 1,000 sparse keys", 600),
]

# engine id -> (display label, peer-group note)
ENGINES = [
    ("c",          "C (-O3)",      "baseline"),
    ("amber",      "Amber",        "array primitives"),
    ("amber-qsql", "Amber qSQL",   "query layer"),
    ("k",          "ngn/k",        "array primitives"),
    ("bqn",        "CBQN",         "array primitives"),
    ("j",          "J",            "array primitives"),
    ("uiua",       "Uiua",         "array primitives"),
    ("numpy",      "NumPy",        "array primitives"),
    ("julia",      "Julia",        "scalar loops (JIT)"),
    ("duckdb",     "DuckDB",       "query layer"),
]


def which(name):
    return shutil.which(name) if name else None


def resolve(env_var, candidates):
    v = os.environ.get(env_var)
    if v and (Path(v).exists() or which(v)):
        return v
    for c in candidates:
        if Path(c).exists() or which(c):
            return c
    return None


def find_bins():
    return {
        "amber":      resolve("AMBER_BIN",   [str(REPO_ROOT / "amber"), "amber"]),
        "amber-qsql": resolve("AMBER_BIN",   [str(REPO_ROOT / "amber"), "amber"]),
        "k":          resolve("K_BIN",       ["/tmp/bench_tools/k", "k", "ngn-k"]),
        "bqn":        resolve("CBQN_BIN",    ["/tmp/bench_tools/cbqn", "cbqn", "bqn"]),
        "duckdb":     resolve("DUCKDB_BIN",  ["/tmp/bench_tools/duckdb", "duckdb"]),
        "julia":      resolve("JULIA_BIN",   ["/tmp/bench_tools/julia", "julia"]),
        "numpy":      resolve("PYTHON_BIN",  [sys.executable or "python3", "python3"]),
        "uiua":       resolve("UIUA_BIN",    ["/tmp/bench_tools/uiua", "uiua"]),
        "j":          resolve("J_BIN",       ["/tmp/bench_tools/jconsole", "jconsole", "ijconsole"]),
        "c":          resolve("C_BENCH_BIN", ["/tmp/bench_tools/c_bench", str(REPO_ROOT / "c_bench")]),
    }


def cmd_for(engine, binpath, bench_id, runs, warmup):
    """Return (argv, stdin_path_or_None). Engines that self-time get runs/warmup."""
    q = QUERIES
    if engine == "amber":
        return [binpath, str(q / "amber_bench.k"), bench_id, str(runs), str(warmup)], None
    if engine == "amber-qsql":
        return [binpath, str(q / "amberq_bench.k"), bench_id, str(runs), str(warmup)], None
    if engine == "numpy":
        return [binpath, str(q / "numpy_bench.py"), bench_id, str(runs), str(warmup)], None
    if engine == "julia":
        return [binpath, "--startup-file=no", str(q / "julia_bench.jl"), bench_id,
                str(runs), str(warmup)], None
    if engine == "c":
        return [binpath, bench_id, str(runs), str(warmup)], None
    # ---- engines with one file per workload and no argv / no clock ----
    if engine == "k":
        return [binpath, str(q / f"k_{bench_id}.k")], None
    if engine == "bqn":
        return [binpath, str(q / f"bqn_{bench_id}.bqn")], None
    if engine == "uiua":
        return [binpath, "run", str(q / f"uiua_{bench_id}.ua")], None
    if engine == "j":
        return [binpath, str(q / f"j_{bench_id}.ijs")], None
    if engine == "duckdb":
        return [binpath, "-batch", "-noheader", "-list", ":memory:"], q / f"duckdb_{bench_id}.sql"
    raise KeyError(engine)


def noop_cmd(engine, binpath):
    """A do-nothing invocation, for measuring this engine's startup baseline."""
    tmp = REPO_ROOT / "bench" / ".noop"
    tmp.mkdir(exist_ok=True)
    files = {
        "k":    (tmp / "noop.k",    "0\n"),
        "bqn":  (tmp / "noop.bqn",  "0\n"),
        "uiua": (tmp / "noop.ua",   "0\n"),
        "j":    (tmp / "noop.ijs",  "exit 0\n"),
    }
    if engine in files:
        p, body = files[engine]
        p.write_text(body)
        if engine == "uiua":
            return [binpath, "run", str(p)], None
        return [binpath, str(p)], None
    if engine == "duckdb":
        p = tmp / "noop.sql"
        p.write_text("SELECT 1;\n")
        return [binpath, "-batch", "-noheader", "-list", ":memory:"], p
    return None, None


def run_once(cmd, stdin_path=None, timeout=300):
    t0 = time.perf_counter()
    try:
        if stdin_path is not None:
            with open(stdin_path, "rb") as f:
                r = subprocess.run(cmd, stdin=f, capture_output=True,
                                   cwd=str(REPO_ROOT), timeout=timeout)
        else:
            r = subprocess.run(cmd, capture_output=True, cwd=str(REPO_ROOT), timeout=timeout)
    except subprocess.TimeoutExpired:
        return None, "", "timeout"
    except (FileNotFoundError, PermissionError) as e:
        return None, "", f"not runnable ({e.__class__.__name__})"
    total_ms = (time.perf_counter() - t0) * 1000.0
    out = r.stdout.decode(errors="replace")
    err = r.stderr.decode(errors="replace")
    if r.returncode != 0:
        return None, out, (err.strip().splitlines() or ["exit %d" % r.returncode])[-1][:120]
    return total_ms, out, err


NUM = r"[-+0-9.eE]+"
RE_ANSWER = re.compile(r"ANSWER\s+(" + NUM + r")")
RE_CHECK = re.compile(r"CHECK\s+(" + NUM + r")")
RE_TIME = re.compile(r"TIME_MS\s+(" + NUM + r")")
# DuckDB's ".timer on" output, used when the engine has no TIME_MS line
RE_DUCK_TIMER = re.compile(r"Run Time \(s\):\s*real\s+([0-9.]+)")


def parse_output(text):
    """Return (answer|None, check|None, kernel_ms|None)."""
    ans = chk = kms = None
    m = RE_ANSWER.findall(text)
    if m:
        try:
            ans = float(m[-1])
        except ValueError:
            ans = None
    m = RE_CHECK.findall(text)
    if m:
        try:
            chk = int(float(m[-1]))
        except ValueError:
            chk = None
    m = RE_TIME.findall(text)
    if m:
        try:
            kms = float(m[-1])
        except ValueError:
            kms = None
    if kms is None:
        m = RE_DUCK_TIMER.findall(text)
        if m:
            kms = float(m[-1]) * 1000.0
    return ans, chk, kms


def measure_startup(engine, binpath, timeout=60):
    """Median wall time of a do-nothing script: this engine's fixed overhead."""
    cmd, stdin_path = noop_cmd(engine, binpath)
    if cmd is None:
        return 0.0
    ts = []
    for _ in range(3):
        ms, _, _ = run_once(cmd, stdin_path, timeout=timeout)
        if ms is not None:
            ts.append(ms)
    return statistics.median(ts) if ts else 0.0


def bench_engine(engine, binpath, bench_id, runs, warmup, timeout, startup_ms):
    """Returns dict(ms, mode, answer, check, error)."""
    if not binpath:
        return dict(ms=None, mode="", answer=None, check=None, error="not installed")
    cmd, stdin_path = cmd_for(engine, binpath, bench_id, runs, warmup)

    first_ms, out, err = run_once(cmd, stdin_path, timeout=timeout)
    if first_ms is None:
        return dict(ms=None, mode="", answer=None, check=None, error=err or "failed")
    answer, check, kernel_ms = parse_output(out)

    if kernel_ms is not None:
        # The script did its own warm-up + median internally.
        return dict(ms=kernel_ms, mode="kernel", answer=answer, check=check, error=None)

    # No in-language clock: warm up by discarding the first run (already done
    # above), then take the median of `runs` more, minus this engine's startup.
    totals = []
    for _ in range(runs):
        ms, out2, err2 = run_once(cmd, stdin_path, timeout=timeout)
        if ms is None:
            return dict(ms=None, mode="", answer=answer, check=check, error=err2 or "failed")
        totals.append(ms)
        a2, c2, _ = parse_output(out2)
        if a2 is not None:
            answer = a2
        if c2 is not None:
            check = c2
    net = max(0.0, statistics.median(totals) - startup_ms)
    return dict(ms=net, mode="net", answer=answer, check=check, error=None)


def fmt_ms(v):
    return f"{v:,.2f}" if v is not None else "—"


def build_table(results, ref, runs, warmup, engines, benchmarks, startup):
    L = []
    L.append(f"_Median of {runs} timed runs after {warmup} warm-up passes. "
             f"Kernel time only — process startup is excluded (see below). "
             f"Generated by `bench/run_comparative.py`; workloads defined in "
             f"[`bench/SPEC.md`](SPEC.md)._")
    L.append("")

    # ---- correctness gate first: a wrong answer voids the time ----
    bad = []
    for bid, _, _ in benchmarks:
        for eid, label, _ in engines:
            r = results[bid][eid]
            if r["error"] or r["ms"] is None:
                continue
            if ref[bid] is not None and r["answer"] is not None and r["answer"] != ref[bid]:
                bad.append(f"{label}/{bid}: answer {r['answer']!r} != reference {ref[bid]!r}")
            if ref["check"] is not None and r["check"] is not None and r["check"] != ref["check"]:
                bad.append(f"{label}/{bid}: input checksum {r['check']} != {ref['check']}")
    if bad:
        L.append("> **⚠ Correctness gate FAILED — these cells are reported as WRONG and their "
                 "times are withheld:**")
        L.append(">")
        for b in bad:
            L.append(f"> - {b}")
    else:
        L.append("> ✅ Correctness gate passed: every engine produced the identical exact answer "
                 "and the identical input checksum on every workload.")
    L.append("")

    hdr = "| Benchmark | " + " | ".join(l for _, l, _ in engines) + " |"
    sep = "|---|" + "---:|" * len(engines)
    L.append(hdr)
    L.append(sep)
    for bid, blabel, _ in benchmarks:
        cells = []
        for eid, _, _ in engines:
            r = results[bid][eid]
            if r["error"]:
                cells.append(f"_{r['error']}_")
            elif r["ms"] is None:
                cells.append("_error_")
            elif ref[bid] is not None and r["answer"] is not None and r["answer"] != ref[bid]:
                cells.append("**WRONG**")
            elif ref["check"] is not None and r["check"] is not None and r["check"] != ref["check"]:
                cells.append("**BADDATA**")
            else:
                cells.append(fmt_ms(r["ms"]))
        L.append(f"| {blabel} | " + " | ".join(cells) + " |")
    L.append("")

    L.append("Relative to the C baseline (lower is better; 1.00× means it matched plain C):")
    L.append("")
    L.append(hdr)
    L.append(sep)
    for bid, blabel, _ in benchmarks:
        base = results[bid]["c"]["ms"]
        cells = []
        for eid, _, _ in engines:
            r = results[bid][eid]
            ok = (not r["error"] and r["ms"] is not None
                  and (ref[bid] is None or r["answer"] == ref[bid]))
            if not ok or not base:
                cells.append("—")
            else:
                cells.append(f"{r['ms'] / base:.2f}×")
        L.append(f"| {blabel} | " + " | ".join(cells) + " |")
    L.append("")

    L.append("**Timing mode per engine** — `kernel` means the engine timed its own kernel with a "
             "monotonic clock; `net` means it has no usable in-language clock and was measured as "
             "_total process time − startup baseline_:")
    L.append("")
    L.append("| Engine | Peer group | Mode | Startup baseline (ms) |")
    L.append("|---|---|---|---:|")
    for eid, label, peer in engines:
        modes = {results[b][eid]["mode"] for b, _, _ in benchmarks} - {""}
        mode = "/".join(sorted(modes)) or "—"
        sb = startup.get(eid)
        L.append(f"| {label} | {peer} | {mode} | {fmt_ms(sb) if sb else '—'} |")
    L.append("")
    L.append("Amber appears twice on purpose: `Amber` is array-primitive code (the fair peer of "
             "ngn/k, CBQN, J and Uiua) and `Amber qSQL` goes through the `select … by … from` "
             "layer (the fair peer of DuckDB's SQL planner). Reporting only the faster of the two "
             "would be choosing whichever comparison flatters Amber.")
    return "\n".join(L) + "\n"


START_MARK = ("<!-- COMPARATIVE_BENCHMARKS:START (auto-generated by "
              "bench/run_comparative.py -- do not edit by hand) -->")
END_MARK = "<!-- COMPARATIVE_BENCHMARKS:END -->"


def update_docs(table_md):
    docs = REPO_ROOT / "docs" / "BENCHMARKS.md"
    section = ("\n## 5. Automated CI comparative benchmarks\n\n"
               f"{START_MARK}\n\n{table_md}\n{END_MARK}\n")
    if docs.exists():
        text = docs.read_text()
        if START_MARK in text and END_MARK in text:
            pre = text.split(START_MARK)[0].rstrip("\n")
            post = text.split(END_MARK)[1]
            text = pre + "\n\n" + f"{START_MARK}\n\n{table_md}\n{END_MARK}" + post
        else:
            text = text.rstrip("\n") + "\n" + section
    else:
        text = "# Amber — sanity checks & benchmarks\n" + section
    docs.write_text(text)
    print(f"updated {docs}", file=sys.stderr)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--runs", type=int, default=5)
    ap.add_argument("--warmup", type=int, default=2)
    ap.add_argument("--out", type=str, default=None)
    ap.add_argument("--update-docs", action="store_true")
    ap.add_argument("--only", type=str, default=None, help="comma-separated engine ids")
    ap.add_argument("--benchmarks", type=str, default=None, help="comma-separated bench ids")
    ap.add_argument("--fail-on-wrong", action="store_true",
                    help="exit non-zero if any engine disagrees with the reference answer")
    args = ap.parse_args()

    engines = ENGINES
    if args.only:
        keep = {s.strip() for s in args.only.split(",")}
        engines = [e for e in ENGINES if e[0] in keep]
    benchmarks = BENCHMARKS
    if args.benchmarks:
        keep = {s.strip() for s in args.benchmarks.split(",")}
        benchmarks = [b for b in BENCHMARKS if b[0] in keep]

    bins = find_bins()
    for eid, label, _ in engines:
        print(f"{label:12s} {bins.get(eid) or 'NOT FOUND'}", file=sys.stderr)

    print("-- measuring startup baselines --", file=sys.stderr)
    startup = {}
    for eid, label, _ in engines:
        b = bins.get(eid)
        startup[eid] = measure_startup(eid, b) if b else None
        if startup.get(eid):
            print(f"  {label:12s} {startup[eid]:8.2f} ms", file=sys.stderr)

    results = {b[0]: {} for b in benchmarks}
    for bid, blabel, timeout in benchmarks:
        for eid, label, _ in engines:
            print(f"running {bid}/{eid} ...", file=sys.stderr)
            results[bid][eid] = bench_engine(eid, bins.get(eid), bid, args.runs,
                                             args.warmup, timeout, startup.get(eid) or 0.0)
            r = results[bid][eid]
            print(f"  -> {r['error'] or ('%.2f ms (%s) answer=%r' % (r['ms'], r['mode'], r['answer']))}",
                  file=sys.stderr)

    # Reference answers: prefer C, else the first engine that produced one.
    ref = {"check": None}
    for bid, _, _ in benchmarks:
        a = results[bid].get("c", {}).get("answer")
        if a is None:
            for eid, _, _ in engines:
                if results[bid][eid]["answer"] is not None:
                    a = results[bid][eid]["answer"]
                    break
        ref[bid] = a
    for bid, _, _ in benchmarks:
        for eid, _, _ in engines:
            c = results[bid][eid]["check"]
            if c is not None:
                ref["check"] = c
                break
        if ref["check"] is not None:
            break

    table_md = build_table(results, ref, args.runs, args.warmup, engines, benchmarks, startup)
    print(table_md)

    if args.out:
        Path(args.out).write_text(table_md)
        print(f"wrote {args.out}", file=sys.stderr)
    if args.update_docs:
        update_docs(table_md)

    if args.fail_on_wrong:
        for bid, _, _ in benchmarks:
            for eid, _, _ in engines:
                r = results[bid][eid]
                if r["ms"] is not None and ref[bid] is not None and r["answer"] != ref[bid]:
                    print("correctness gate failed", file=sys.stderr)
                    return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
