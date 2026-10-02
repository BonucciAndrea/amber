#!/usr/bin/env python3
"""bench/render.py - turn suite.k / suite.py output into the BENCHMARKS.md section 2 tables.

Run:  render.py amber.txt python.txt ...   (or pipe the RESULT lines on stdin)
Reads lines  RESULT <workload> <engine> <median_ms> <answer>  and prints markdown.

Answer check, against Amber's answer for the same workload:
  * integer answers (counts, int sorts, distinct, join)   exact
  * float answers                                         relative 1e-9
  * moving-window checksums (summation order differs)     relative 1e-6
A case whose answers disagree gets MISMATCH instead of a time, and never a ratio.
Bar: length round(3*log2(ratio)) of 10, filled from the left when Amber is faster,
from the right when slower; length 0 prints "~even".
"""
import sys, math, re

ENGINES = ["amber", "numpy", "polars", "duckdb"]
WINDOW = re.compile(r"^(m(sum|avg|dev|min|max)\d+|sc_mavg_)")
TOL_FLOAT, TOL_WINDOW = 1e-9, 1e-6

res = {}                                    # (workload, engine) -> (ms, answer string)
order = []
for path in (sys.argv[1:] or ["-"]):
    fh = sys.stdin if path == "-" else open(path)
    for line in fh:
        p = line.split()
        if len(p) == 5 and p[0] == "RESULT":
            if p[1] not in order:
                order.append(p[1])
            res[(p[1], p[2])] = (float(p[3]), p[4])


def agree(wid, a, b):
    if re.fullmatch(r"-?\d+", a) and re.fullmatch(r"-?\d+", b):
        return int(a) == int(b)
    x, y = float(a), float(b)
    tol = TOL_WINDOW if WINDOW.match(wid) else TOL_FLOAT
    return x == y or abs(x - y) <= tol * max(abs(x), abs(y))


bad = set()
for wid in order:
    ref = res.get((wid, "amber"))
    for e in ENGINES[1:]:
        if ref and (wid, e) in res and not agree(wid, ref[1], res[(wid, e)][1]):
            bad.add((wid, e))
bad_wl = {w for w, _ in bad}


def num(ms):                                # 3 significant figures, 3 decimals below 1
    if ms < 1:
        return "%.3f" % ms
    d = max(0, 2 - int(math.floor(math.log10(ms))))
    return "%.*f" % (d, ms)


def bar(a, b):                              # Amber ms vs numpy/pandas ms
    r = b / a if b >= a else a / b
    n = min(10, int(round(3 * math.log2(r))))
    if n == 0:
        return "~even " + "░" * 10
    if b >= a:
        return "**%.1fx faster** %s" % (r, "█" * n + "░" * (10 - n))
    return "%.1fx slower %s" % (r, "░" * (10 - n) + "█" * n)


def cells(wid, engines):
    ok = {e: res[(wid, e)][0] for e in engines if (wid, e) in res and (wid, e) not in bad}
    best = min(ok.values()) if ok else None
    out = []
    for e in engines:
        if (wid, e) in bad:
            out.append("MISMATCH")
        elif (wid, e) not in res:
            out.append("—")
        else:
            s = num(res[(wid, e)][0])
            out.append("**%s**" % s if res[(wid, e)][0] == best else s)
    return out


def table(title, rows, engines, ratio=True, head=None):
    names = {"amber": "Amber", "numpy": "numpy/pandas", "polars": "Polars", "duckdb": "DuckDB"}
    hdr = head or (["workload"] + [names[e] for e in engines] + (["Amber vs numpy/pandas"] if ratio else []))
    print(title + "\n")
    print("| " + " | ".join(hdr) + " |")
    print("|---|" + "---:|" * (len(hdr) - 1 - ratio) + ("---|" if ratio else ""))
    for wid, label in rows:
        c = cells(wid, engines)
        if ratio:
            if (wid, "numpy") in bad:
                c.append("MISMATCH")
            elif (wid, "amber") not in res or (wid, "numpy") not in res:
                c.append("—")
            else:
                c.append(bar(res[(wid, "amber")][0], res[(wid, "numpy")][0]))
        print("| " + " | ".join([label] + c) + " |")
    print()


ALL4 = ENGINES
measured = [w for w in order if not w.startswith("sc_")]
checked = sum(1 for w in measured for e in ENGINES[1:] if (w, e) in res)
print("This run: **%d cases, %d answer mismatches** (%d engine answers cross-checked against Amber).\n"
      % (len(order), len(bad_wl), checked))
if bad:
    print("Mismatches: " + ", ".join("%s/%s" % b for b in sorted(bad)) + "\n")

table("### Headline: 1,000,000-row vectors", [
    ("sum", "`+/px`, sum"), ("filter", "`+/px>50`, filter + count"),
    ("grp10", "group-by sum (10 groups)"), ("distinct", "distinct (100k distinct keys)"),
    ("mavg100", "moving average (window 100)"), ("sortf", "sort (1M floats)"),
    ("asof50k", "as-of join (50k trades x 100k quotes)")], ALL4)

table("### 2.1 Moving windows: 1M floats", [
    ("msum100", "`msum[100;x]`, moving sum"), ("mavg100", "`mavg[100;x]`, moving average"),
    ("mdev100", "`mdev[100;x]`, moving stddev"), ("mmin100", "`mmin[100;x]`, moving min"),
    ("mmax100", "`mmax[100;x]`, moving max"), ("mavg10", "`mavg[10;x]`, window 10"),
    ("mavg1000", "`mavg[1000;x]`, window 1000"), ("mmin10", "`mmin[10;x]`, window 10"),
    ("mmin1000", "`mmin[1000;x]`, window 1000")], ENGINES[:3])

table("Window-width sensitivity:", [("mmin10", "10"), ("mmin100", "100"), ("mmin1000", "1,000")],
      ENGINES[:3], ratio=False,
      head=["window", "Amber `mmin`", "pandas `rolling().min()`", "Polars `rolling_min`"])

table("### 2.2 Sort and grade: 1M elements", [
    ("sortf", "sort 1M floats, `asc x`"), ("sorti", "sort 1M integers, `asc x`"),
    ("gradef", "grade 1M floats, `iasc x` (argsort)"),
    ("presorted", "sort an ALREADY-SORTED 1M float vector")], ALL4)

table("### 2.3 Multi-column table sort: 1M rows x 3 columns", [
    ("tsort_px", "``xasc[,`px;t]``, 1 float key, 3 cols"),
    ("tsort_st", "``xasc[`sym`time;t]``, 2 keys, 3 cols"),
    ("tdesc_st", "``xdesc[`sym`time;t]``, 2 keys, descending")], ALL4)

table("### 2.4 Joins", [
    ("ij", "inner join (1M rows x 1,000 sparse keys)"),
    ("asof50k", "as-of join (50k trades x 100k quotes)"),
    ("asof500k", "as-of join (500k trades x 1M quotes)")], ALL4)

table("### 2.5 Group-by vs cardinality: 1M rows", [
    ("grp10", "10 groups"), ("grp1k", "1,000 groups"), ("grp100k", "100,000 groups")], ALL4)

print("### 2.6 Scaling: Amber, 100k -> 10M rows\n")
print("| workload | 100k | 1M | 10M | ns/row @10M | scaling 100k->10M |")
print("|---|---:|---:|---:|---:|---|")
for key, label in (("sum", "`+/px` sum"), ("sort", "sort"), ("mavg", "moving average (100)"),
                   ("grp", "group-by sum")):
    t = [res.get(("sc_%s_%s" % (key, s), "amber"), (None,))[0] for s in ("100k", "1m", "10m")]
    if None in t:
        continue
    print("| %s | %s | %s | %s | %.1f | %.0fx time for 100x data |"
          % (label, num(t[0]), num(t[1]), num(t[2]), t[2] * 1e6 / 1e7, t[2] / t[0]))
print()
