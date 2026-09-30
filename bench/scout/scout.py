#!/usr/bin/env python3
"""bench/scout/scout.py - the comparative scout runner.

Runs one fixed operation matrix (bench/scout/SCOUT_SPEC.md) across every array
language, K/q implementation and columnar engine reachable on this machine,
gates every timing on an exact answer match against the C reference, and emits
a ranked "who wins at what" report.

    python3 bench/scout/scout.py --list
    python3 bench/scout/scout.py --build --out bench/scout/results.json
    python3 bench/scout/scout.py --smoke
    python3 bench/scout/scout.py --n 10000000 --runs 5 --out bench/scout/results.json
    python3 bench/scout/scout.py --from-json bench/scout/results.json \
                                 --report bench/SCOUT_REPORT.md

Run it from the repository root, under WSL (the Amber/ngn/k/CBQN/J binaries and
the q.exe interop path are all resolved from there).
"""
import argparse, json, os, platform, re, shutil, statistics, subprocess, sys, time

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
ENG = os.path.join(ROOT, "bench", "scout", "engines")
HOME = os.path.expanduser("~")
OPTBIN = os.path.join(HOME, "opt", "bin")
VENV = os.path.join(HOME, "opt", "venv", "bin", "python")
PEACHQ = os.path.join(HOME, "opt", "peachq", "q")
QEXE = "/mnt/c/q/w64/q.exe"
CREF = "/tmp/scout_c_ref"
# q.exe is a WINDOWS binary reached through WSL interop: it cannot open a
# /mnt/c/... path, so its script argument stays relative to the run cwd (ROOT).
QREL = "bench/scout/engines/q.q"

# ---------------------------------------------------------------- op matrix
CORE_OPS = [
    "sum_f", "max_f", "dot", "sum_i", "arith_mask", "scan_f",
    "sort_f", "sort_presorted", "grade_i",
    "find", "member", "distinct", "distinct_100k",
    "group_10", "group_100", "group_10k", "group_100k",
    "join_inner", "msum_16", "mavg_256", "mmax_64",
]
TABLE_OPS = ["asof", "tablesort", "qsql_select"]
ALL_OPS = CORE_OPS + TABLE_OPS

OP_GROUP = {
    "sum_f": "Reductions & arithmetic", "max_f": "Reductions & arithmetic",
    "dot": "Reductions & arithmetic", "sum_i": "Reductions & arithmetic",
    "arith_mask": "Reductions & arithmetic", "scan_f": "Reductions & arithmetic",
    "sort_f": "Sort & grade", "sort_presorted": "Sort & grade",
    "grade_i": "Sort & grade", "tablesort": "Sort & grade",
    "find": "Search, distinct & group", "member": "Search, distinct & group",
    "distinct": "Search, distinct & group", "distinct_100k": "Search, distinct & group",
    "group_10": "Search, distinct & group", "group_100": "Search, distinct & group",
    "group_10k": "Search, distinct & group", "group_100k": "Search, distinct & group",
    "join_inner": "Joins", "asof": "Joins",
    "msum_16": "Moving windows", "mavg_256": "Moving windows", "mmax_64": "Moving windows",
    "qsql_select": "qSQL-shaped",
}
# The one op whose answer is genuinely order-dependent (SCOUT_SPEC section 2).
TOLERANCE_OPS = {"mavg_256": 1e-9}

# Ops whose problem size is FIXED by the spec and does not scale with --n.
FIXED_SIZE_OPS = {"join_inner", "asof", "tablesort", "qsql_select"}

# ---------------------------------------------------------------- engines
SINGLE = {
    "OMP_NUM_THREADS": "1", "AMBER_THREADS": "1", "POLARS_MAX_THREADS": "1",
    "OPENBLAS_NUM_THREADS": "1", "MKL_NUM_THREADS": "1",
    "NUMEXPR_NUM_THREADS": "1", "RAYON_NUM_THREADS": "1", "UIUA_THREADS": "1",
}


def amber_bin(variant):
    return "/tmp/scout-amber-%s" % variant


class Engine(object):
    def __init__(self, key, label, cmd, ops, note="", env=None, mt=False):
        self.key, self.label, self.cmd, self.ops = key, label, cmd, ops
        self.note, self.env, self.mt = note, env or {}, mt

    def available(self):
        return os.path.exists(self.cmd[0]) or shutil.which(self.cmd[0]) is not None

    def argv(self, op, n, runs, warm):
        return list(self.cmd) + [op, str(n), str(runs), str(warm)]


def build_engines(threads):
    k = lambda *a: list(a)
    py = lambda name: [VENV, os.path.join(ENG, "py_engines.py"), name]
    engines = [
        Engine("c", "C -O3 -march=native", [CREF], ALL_OPS,
               "hand-written reference; generic LSD radix sorts, open-address hash"),
        Engine("amber", "Amber (portable build)",
               [amber_bin("portable"), os.path.join(ENG, "amber.k")], ALL_OPS,
               "array primitives; ./build.sh, no -march=native"),
        Engine("amber-native", "Amber (AMBER_NATIVE=1)",
               [amber_bin("native"), os.path.join(ENG, "amber.k")], ALL_OPS,
               "array primitives; -march=native"),
        Engine("amber-qsql", "Amber qSQL layer",
               [amber_bin("native"), os.path.join(ENG, "amberq.k")],
               ["group_10", "group_100", "group_10k", "group_100k",
                "join_inner", "qsql_select"],
               "select ... by ... from; fair peer of DuckDB SQL"),
        Engine("q", "q", [QEXE, QREL], ALL_OPS, "the reference to beat"),
        Engine("peachq", "PeachQ (Rayforce)", [PEACHQ, QREL], ALL_OPS,
               "open-source q on the Rayforce engine"),
        Engine("ngnk", "ngn/k", [os.path.join(OPTBIN, "ngnk"),
                                 os.path.join(ENG, "ngnk.k")],
               CORE_OPS, "the interpreter Amber's core is derived from"),
        Engine("cbqn", "CBQN", [os.path.join(OPTBIN, "cbqn"),
                                os.path.join(ENG, "cbqn.bqn")],
               CORE_OPS, "SIMD object model; narrow int storage"),
        Engine("j", "J 9.6", [os.path.join(OPTBIN, "jconsole"),
                              os.path.join(ENG, "j.ijs")],
               CORE_OPS, "special-code idiom recognition"),
        Engine("numpy", "NumPy", py("numpy"), ALL_OPS, "ufunc per step"),
        Engine("pandas", "pandas", py("pandas"), ALL_OPS, "hash group-by, block manager"),
        Engine("polars", "Polars", py("polars"), ALL_OPS, "Arrow + vectorised kernels"),
        Engine("duckdb", "DuckDB", py("duckdb"), ALL_OPS, "vectorised push execution"),
        # 2.3: more engines. Installed under ~/opt (no system packages); each adapter's
        # header says how it is timed and how it is held to one thread.
        Engine("uiua", "Uiua 0.19", [os.path.join(OPTBIN, "uiua"), "run", "--no-format", os.path.join(ENG, "uiua.ua")],
               CORE_OPS, "stack-based array language, f64 only; classify+group"),
        Engine("gnuapl", "GNU APL 2.0", [os.path.join(OPTBIN, "gnu-apl"), "--script", "-f", os.path.join(ENG, "gnuapl.apl"), "--"],
               CORE_OPS, "tree-walking APL2 interpreter; sort-based group-by; member is O(n*k)"),
        Engine("dyalog", "Dyalog APL 20.0", [os.path.join(OPTBIN, "dyalogscript"), os.path.join(ENG, "dyalog.apls")],
               CORE_OPS, "key operator group-by, n-wise windows; narrow int storage"),
        Engine("julia", "Julia 1.13", [os.path.join(OPTBIN, "julia"), "-t", "1", "--startup-file=no", os.path.join(ENG, "julia.jl")],
               ALL_OPS, "Base Julia, JIT-compiled closures; Dict group-by and hash join"),
        Engine("r", "R 4.6 (base)", [os.path.join(OPTBIN, "R"), "--vanilla", "--no-echo", "-f", os.path.join(ENG, "r_engines.R"), "--args", "base"],
               ALL_OPS, "base-R vectors; rowsum/match hashing, radix sort"),
        Engine("rdt", "R data.table 1.18", [os.path.join(OPTBIN, "R"), "--vanilla", "--no-echo", "-f", os.path.join(ENG, "r_engines.R"), "--args", "datatable"],
               ["distinct", "distinct_100k", "group_10", "group_100", "group_10k", "group_100k",
                "join_inner", "msum_16", "mavg_256", "mmax_64"] + TABLE_OPS,
               "GForce by, X[i] and rolling joins, froll*; setDTthreads(1)"),
        Engine("clickhouse", "ClickHouse 26.9 (local)", [VENV, os.path.join(ENG, "clickhouse_engine.py")],
               ALL_OPS, "clickhouse-local, Memory tables, max_threads=1; query_log timing"),
        Engine("octave", "GNU Octave 10.3", [os.path.join(OPTBIN, "octave-cli"), "--norc", "--no-history", "--quiet", os.path.join(ENG, "octave.m")],
               CORE_OPS, "vectorised; unique+accumarray group-by, ismember"),
        Engine("numba", "Numba 0.67", [VENV, os.path.join(ENG, "numba_engine.py")],
               ALL_OPS, "@njit loops (compiled NumPy), typed Dict/set; one thread"),
        Engine("april", "April 1.0 (APL on SBCL)", [os.path.join(OPTBIN, "april-sbcl"), os.path.join(ENG, "april.lisp")],
               ["sum_f", "max_f", "dot", "sum_i", "scan_f", "sort_f", "sort_presorted", "grade_i",
                "distinct", "group_10", "group_100", "join_inner", "msum_16", "mmax_64"],
               "APL compiled to Common Lisp; 1 worker; O(n*k) ops left out"),
        Engine("growler", "growler/k", [os.path.join(OPTBIN, "growler"), os.path.join(ENG, "growler.k")],
               CORE_OPS, "maintained ngn/k fork"),
        Engine("goal", "Goal 1.8 (-tags full)", [os.path.join(OPTBIN, "goal"), os.path.join(ENG, "goal.goal")],
               CORE_OPS, "K-like, Go; math.mmax; rt.time clock"),
        Engine("kona", "Kona (K3)", [os.path.join(OPTBIN, "kona"), os.path.join(ENG, "kona.k")],
               CORE_OPS, "open-source K3; wall-clock _T timer"),
        Engine("klong", "Klong 20221212", [os.path.join(OPTBIN, "kg"), "-u", os.path.join(ENG, "klong.kg")],
               CORE_OPS, "decimal reals, CPU-time clock; N<=1e6"),
        Engine("ktye", "ktye/k", ["sh", os.path.join(ENG, "ktye_run.sh"), os.path.join(OPTBIN, "ktye-scout"),
               os.path.join(ENG, "ktye.k")], CORE_OPS, "32-bit ints; now[] native clock"),
        Engine("kap", "Kap (JVM 25)", [os.path.join(OPTBIN, "kap-jvm-text"), "--no-repl", "--no-init",
               "--load=" + os.path.join(ENG, "kap.kap")], CORE_OPS, "lazy APL on JVM; nanoTime",
               env={"JAVA_OPTS": "-Xmx4g"}),
        Engine("ok", "oK (node)", ["node", os.path.join(ENG, "ok_driver.js"), os.path.join(ENG, "ok.k")],
               CORE_OPS, "k in JavaScript; N<=1e6"),
        Engine("l", "l 20260929 (AVX2 eval build)", [os.path.join(OPTBIN, "l_l64"), QREL],
               ALL_OPS, "independent q implementation; runs q.q"),
    ]
    # before/after in one run: point SCOUT_AMBER_BASE at an older amber binary and it
    # gets its own row, run on the same data between the same engines.
    base = os.environ.get("SCOUT_AMBER_BASE")
    if base:
        engines.insert(3, Engine("amber-base", "Amber (baseline binary)",
                                 [base, os.path.join(ENG, "amber.k")], ALL_OPS,
                                 "whatever SCOUT_AMBER_BASE points at"))
    # amber 2.1: the "amber-mt" row is gone. It measured the reductions' OpenMP
    # `parallel for`, which was removed (a thread team per reduction was a net
    # loss); OMP_NUM_THREADS no longer changes anything. peach is the multi-core
    # path and is measured by tests/test_parallel.c, not by this matrix.
    return engines


# ---------------------------------------------------------------- running
PROTO = re.compile(r"^(BENCH|CHECK|ANSWER|TIME_MS)\s+(\S+)\s*$", re.M)


def run_one(engine, op, n, runs, warm, timeout, cwd):
    env = dict(os.environ)
    env.update(SINGLE)
    env.update(engine.env)
    argv = engine.argv(op, n, runs, warm)
    if engine.key in ("q", "peachq"):
        argv = argv + ["-q", "-s", "0"]
    t0 = time.time()
    try:
        p = subprocess.run(argv, cwd=cwd, env=env, timeout=timeout,
                           stdin=subprocess.DEVNULL,
                           stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    except subprocess.TimeoutExpired:
        return {"status": "TIMEOUT", "wall_s": timeout}
    except OSError as e:
        return {"status": "ERROR", "detail": str(e)}
    out = p.stdout.decode("utf-8", "replace")
    err = p.stderr.decode("utf-8", "replace")
    wall = time.time() - t0
    if re.search(r"^SKIP\b", out, re.M):
        return {"status": "SKIP", "wall_s": wall}
    fields = dict(PROTO.findall(out))
    if "ANSWER" not in fields:
        detail = (err.strip() or out.strip())[:400]
        return {"status": "ERROR", "detail": detail, "wall_s": wall}
    try:
        rec = {"status": "OK", "wall_s": wall,
               "answer": float(fields["ANSWER"]),
               "check": float(fields.get("CHECK", "nan"))}
    except ValueError:
        return {"status": "ERROR", "detail": "unparsable ANSWER/CHECK", "wall_s": wall}
    if "TIME_MS" in fields:
        rec["ms"] = float(fields["TIME_MS"])
        rec["timing"] = "in-engine"
    else:
        rec["ms"] = None
        rec["timing"] = "none"
    return rec


def verify(rec, ref, op):
    """Gate a timing on the C reference. Returns the final status."""
    if rec["status"] != "OK" or ref is None or ref.get("status") != "OK":
        return rec["status"]
    if rec["check"] == rec["check"] and ref["check"] == ref["check"]:
        if rec["check"] != ref["check"]:
            return "BADDATA"
    tol = TOLERANCE_OPS.get(op)
    a, b = rec["answer"], ref["answer"]
    if tol is None:
        return "OK" if a == b else "WRONG"
    scale = max(abs(a), abs(b), 1.0)
    return "OK" if abs(a - b) <= tol * scale else "WRONG"


# ---------------------------------------------------------------- machine
def machine_info():
    def sh(c):
        try:
            return subprocess.run(c, shell=True, stdout=subprocess.PIPE,
                                  stderr=subprocess.DEVNULL,
                                  timeout=30).stdout.decode().strip()
        except Exception:
            return "?"
    cpu = "?"
    try:
        for line in open("/proc/cpuinfo"):
            if line.startswith("model name"):
                cpu = line.split(":", 1)[1].strip()
                break
    except Exception:
        pass
    flags = ""
    try:
        for line in open("/proc/cpuinfo"):
            if line.startswith("flags"):
                have = set(line.split(":", 1)[1].split())
                flags = " ".join(sorted(
                    f for f in ("sse4_2", "avx", "avx2", "avx512f", "bmi2")
                    if f in have))
                break
    except Exception:
        pass
    return {
        "cpu": cpu, "simd": flags, "cores": os.cpu_count(),
        "os": platform.platform(),
        "gcc": sh("gcc --version | head -1"),
        "python": sys.version.split()[0],
        "q": sh("%s -q -s 0 <<< '-1 string .z.K; exit 0;' 2>/dev/null" % QEXE) or "?",
        "peachq": sh("ls %s >/dev/null 2>&1 && echo v0.81" % PEACHQ),
        "cbqn": sh("%s/cbqn --version 2>/dev/null | head -1" % OPTBIN),
        "j": sh("echo 'echo 9!:14 $0' | %s/jconsole 2>/dev/null | head -1" % OPTBIN),
        "numpy": sh("%s -c 'import numpy;print(numpy.__version__)'" % VENV),
        "pandas": sh("%s -c 'import pandas;print(pandas.__version__)'" % VENV),
        "polars": sh("%s -c 'import polars;print(polars.__version__)'" % VENV),
        "duckdb": sh("%s -c 'import duckdb;print(duckdb.__version__)'" % VENV),
        "amber": sh("cd %s && git rev-parse --short HEAD" % ROOT),
    }


def ensure_binaries(force=False):
    """Build the C reference and both Amber variants into /tmp."""
    src = os.path.join(ENG, "c_ref.c")
    if force or not os.path.exists(CREF) or os.path.getmtime(src) > os.path.getmtime(CREF):
        subprocess.run(["gcc", "-O3", "-march=native", "-o", CREF, src, "-lm"],
                       check=True)
        print("built C reference -> %s" % CREF)
    for variant, env in (("portable", {}), ("native", {"AMBER_NATIVE": "1"})):
        dst = amber_bin(variant)
        if force or not os.path.exists(dst):
            e = dict(os.environ)
            e.update(env)
            subprocess.run(["./build.sh"], cwd=ROOT, env=e, check=True,
                           stdout=subprocess.DEVNULL)
            # copy off the DrvFs mount: rebuilding in place gives "Text file busy"
            shutil.copy(os.path.join(ROOT, "amber"), dst)
            os.chmod(dst, 0o755)
            print("built Amber %s -> %s" % (variant, dst))


# ---------------------------------------------------------------- main run
def run_matrix(engines, ops, n, runs, warm, timeout, results=None, save=None):
    """Run each op across every engine.  `save` is called after EVERY op so a
    long matrix survives being interrupted: the partial JSON is always on disk
    and `--resume` picks up from it."""
    results = results if results is not None else {}
    ref_engine = [e for e in engines if e.key == "c"][0]
    for op in ops:
        if op in results and all(v.get("status") for v in results[op].values()):
            print("  %-16s (cached)" % op)
            continue
        ref = run_one(ref_engine, op, n, runs, warm, timeout, ROOT)
        ref["status"] = ref["status"]
        results.setdefault(op, {})["c"] = ref
        print("  %-16s c            %-8s %s" % (
            op, ref["status"], fmt_ms(ref.get("ms"))))
        for e in engines:
            if e.key == "c":
                continue
            if op not in e.ops:
                results[op][e.key] = {"status": "SKIP"}
                continue
            rec = run_one(e, op, n, runs, warm, timeout, ROOT)
            rec["status"] = verify(rec, ref, op)
            results[op][e.key] = rec
            print("  %-16s %-12s %-8s %s%s" % (
                op, e.key, rec["status"], fmt_ms(rec.get("ms")),
                ("  " + rec.get("detail", "")[:90]) if rec["status"] == "ERROR" else ""))
        if save:
            save()
        sys.stdout.flush()
    return results


def fmt_ms(ms):
    if ms is None:
        return ""
    if ms >= 1000:
        return "%.1f ms" % ms
    if ms >= 10:
        return "%.2f ms" % ms
    return "%.3f ms" % ms


# ---------------------------------------------------------------- CLI
def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--list", action="store_true")
    ap.add_argument("--smoke", action="store_true")
    ap.add_argument("--n", type=int, default=10_000_000)
    ap.add_argument("--runs", type=int, default=5)
    ap.add_argument("--warmup", type=int, default=2)
    ap.add_argument("--timeout", type=float, default=900.0)
    ap.add_argument("--ops", default="")
    ap.add_argument("--engines", default="")
    ap.add_argument("--out", default="")
    ap.add_argument("--scaling", default="")
    ap.add_argument("--build", action="store_true",
                    help="force a rebuild of the C reference and both Amber "
                         "binaries, then run the matrix")
    ap.add_argument("--build-only", action="store_true",
                    help="build and exit without running anything")
    ap.add_argument("--resume", action="store_true",
                    help="load --out if it exists and skip ops already recorded")
    args = ap.parse_args()

    threads = os.cpu_count() or 1
    engines = build_engines(threads)
    if args.engines:
        want = set(args.engines.split(","))
        want.add("c")
        engines = [e for e in engines if e.key in want]

    if args.list:
        print("%-14s %-34s %s" % ("KEY", "ENGINE", "AVAILABLE"))
        for e in engines:
            print("%-14s %-34s %s" % (e.key, e.label,
                                      "yes" if e.available() else "NO (%s)" % e.cmd[0]))
        info = machine_info()
        print()
        for k in sorted(info):
            print("%-10s %s" % (k, info[k]))
        return 0

    # --build forces a rebuild of the C reference and both Amber variants and
    # then RUNS, which is what every use of it wants. It used to `return 0`
    # immediately after building, so `scout.py --build --out results.json`
    # silently produced no results and looked like the harness had died.
    # --build-only keeps the old behaviour for a caller that really wants it.
    ensure_binaries(force=args.build or args.build_only)
    if args.build_only:
        return 0

    engines = [e for e in engines if e.available()]
    n = 100_000 if args.smoke else args.n
    runs = 3 if args.smoke else args.runs
    ops = args.ops.split(",") if args.ops else ALL_OPS

    payload = None
    if args.resume and args.out and os.path.exists(args.out):
        payload = json.load(open(args.out))
        print("resuming from %s (%d ops already recorded)"
              % (args.out, len(payload.get("matrix", {}))))
    if payload is None:
        payload = {"machine": machine_info(), "n": n, "runs": runs,
                   "warmup": args.warmup,
                   "engines": {}, "matrix": {}, "scaling": {}}
    payload["engines"].update(
        {e.key: {"label": e.label, "note": e.note, "mt": e.mt} for e in engines})

    def save():
        if args.out:
            tmp = args.out + ".tmp"
            with open(tmp, "w") as fh:
                json.dump(payload, fh, indent=1)
            os.replace(tmp, args.out)

    print("scout: N=%d runs=%d warmup=%d engines=%s" % (
        n, runs, args.warmup, ",".join(e.key for e in engines)))
    run_matrix(engines, ops, n, runs, args.warmup, args.timeout,
               results=payload["matrix"], save=save)

    if args.scaling:
        for sn in [int(v) for v in args.scaling.split(",")]:
            print("scaling N=%d" % sn)
            bucket = payload["scaling"].setdefault(str(sn), {})
            run_matrix(engines, ["sum_f", "group_10k", "sort_f"], sn, runs,
                       args.warmup, args.timeout, results=bucket, save=save)

    save()
    if args.out:
        print("wrote %s" % args.out)
    return 0


if __name__ == "__main__":
    sys.exit(main())
