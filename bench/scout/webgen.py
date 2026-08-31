#!/usr/bin/env python3
"""Render bench/scout/results.json into publishable docs.

    python3 bench/scout/webgen.py --html   > fragment.html   (amber-website)
    python3 bench/scout/webgen.py --md     > fragment.md     (docs/BENCHMARKS.md)

One source of truth: the numbers on the website and the numbers in the repo are
generated from the same results.json, so they cannot drift apart. Re-run scout.py
to refresh the data, then re-run this to refresh both documents.
GNU AGPLv3 - see LICENSE and NOTICE.
"""
import json, math, os, sys, html

HERE = os.path.dirname(os.path.abspath(__file__))
D = json.load(open(os.path.join(HERE, "results.json")))

# Engine display order + short labels for the wide matrix.
ORDER = ["c", "amber-native", "amber", "amber-qsql", "peachq", "ngnk",
         "cbqn", "j", "numpy", "pandas", "polars", "duckdb", "amber-mt"]
SHORT = {"c": "C", "amber-native": "Amber<sub>nat</sub>", "amber": "Amber",
         "amber-qsql": "Amber<sub>qSQL</sub>", "peachq": "PeachQ",
         "ngnk": "ngn/k", "cbqn": "CBQN", "j": "J", "numpy": "NumPy",
         "pandas": "pandas", "polars": "Polars", "duckdb": "DuckDB",
         "amber-mt": "Amber<sub>14t</sub>"}
SHORT_MD = {k: v.replace("<sub>", "-").replace("</sub>", "") for k, v in SHORT.items()}

CATS = [
    ("Reductions & vector arithmetic", ["sum_f", "sum_i", "max_f", "dot", "arith_mask"]),
    ("Sort & grade",                   ["sort_f", "sort_presorted", "grade_i", "tablesort"]),
    ("Search, distinct & group-by",    ["find", "member", "distinct", "distinct_100k",
                                        "group_10", "group_100", "group_10k", "group_100k"]),
    ("Joins",                          ["join_inner", "asof"]),
    ("Moving windows",                 ["msum_16", "mavg_256", "mmax_64"]),
    ("qSQL-shaped",                    ["qsql_select"]),
]
DESC = {
 "sum_f": "sum of 10M float64", "sum_i": "sum of 10M int64",
 "max_f": "max of 10M float64", "dot": "dot product of two 10M float64 vectors",
 "arith_mask": "(a*b)+c under a boolean mask", "sort_f": "ascending sort, 10M random float64",
 "sort_presorted": "sort of already-sorted input (best case)", "grade_i": "grade-up (argsort) of 10M int64",
 "tablesort": "3-column table sorted by two keys", "find": "first index of a value in 10M elements",
 "member": "membership of 10M against a 10M set", "distinct": "distinct over ~10 groups",
 "distinct_100k": "distinct over 100k groups", "group_10": "group-by, 10 groups",
 "group_100": "group-by, 100 groups", "group_10k": "group-by, 10k groups",
 "group_100k": "group-by, 100k groups", "join_inner": "inner join on an int key",
 "asof": "as-of join, the tick-desk workload", "msum_16": "moving sum, window 16",
 "mavg_256": "moving average, window 256", "mmax_64": "moving max, window 64",
 "qsql_select": "select ... by ... from - the full query path",
}

def cell(op, eng, tbl=None):
    row = (tbl or D["matrix"]).get(op, {})
    c = row.get(eng)
    if not isinstance(c, dict): return None, "n/a"
    if c.get("status") != "OK":  return None, c.get("status", "-").lower()
    ms = c.get("ms")
    return (ms, fmt(ms)) if ms is not None else (None, "-")

def fmt(ms):
    if ms is None: return "-"
    if ms >= 1000: return "%.0f" % ms
    if ms >= 100:  return "%.0f" % ms
    if ms >= 10:   return "%.1f" % ms
    if ms >= 1:    return "%.2f" % ms
    return "%.3f" % ms

def best(op):
    vals = [(cell(op, e)[0], e) for e in ORDER if e != "amber-mt"]
    vals = [(v, e) for v, e in vals if v]
    return min(vals)[1] if vals else None

def ratios():
    out = []
    for op in [o for _, ops in CATS for o in ops]:
        a, _ = cell(op, "amber-native"); q, _ = cell(op, "c")
        if a and q: out.append((op, a, q, q / a))
    return sorted(out, key=lambda r: -r[3])


# ---------------------------------------------------------------- HTML output
def h_machine():
    m = D["machine"]
    return f"""<div class="bm-meta">
  <div><span class="bm-k">CPU</span><span class="bm-v">{html.escape(m['cpu'])} &middot; {m['cores']} cores</span></div>
  <div><span class="bm-k">SIMD</span><span class="bm-v">{html.escape(m['simd'])}</span></div>
  <div><span class="bm-k">OS</span><span class="bm-v">{html.escape(m['os'])}</span></div>
  <div><span class="bm-k">Rows</span><span class="bm-v">N = {D['n']:,} &middot; {D['runs']} timed runs, {D['warmup']} warm-up</span></div>
  <div><span class="bm-k">Amber</span><span class="bm-v">build {html.escape(str(m['amber']))}</span></div>
</div>"""

def h_diverging():
    rs = ratios()
    wins = [r for r in rs if r[3] >= 1]; losses = [r for r in rs if r[3] < 1]
    def bar(op, a, q, r):
        win = r >= 1
        mag = abs(math.log10(r)); w = min(100.0, mag / math.log10(50.0) * 100.0)
        cls = "win" if win else "loss"
        lab = ("%.2f&times; faster" % r) if win else ("%.2f&times; slower" % (1 / r))
        return (f'<div class="dv-row"><code class="dv-op" title="{html.escape(DESC.get(op,""))}">{op}</code>'
                f'<div class="dv-track"><div class="dv-half l">{"" if win else f"<i class=dv-bar-loss style=width:{w:.1f}%></i>"}</div>'
                f'<div class="dv-half r">{f"<i class=dv-bar-win style=width:{w:.1f}%></i>" if win else ""}</div></div>'
                f'<span class="dv-val {cls}">{lab}</span>'
                f'<span class="dv-ms">{fmt(a)} / {fmt(q)} ms</span></div>')
    body = "".join(bar(*r) for r in rs)
    return f"""<div class="dv-chart">
  <div class="dv-head"><span>operation</span><span class="dv-axis"><em>C faster</em><b></b><em>Amber faster</em></span><span></span><span class="dv-ms">Amber / C</span></div>
  {body}
</div>
<p class="bm-sum">Amber is faster on <strong>{len(wins)} of {len(rs)}</strong> operations, slower on <strong>{len(losses)}</strong>.
Bars are log-scaled; the centre line is parity.</p>"""

def h_matrix():
    head = "".join(f'<th class="{"eamber" if e.startswith("amber") else ""}">{SHORT[e]}</th>' for e in ORDER)
    rows = []
    for title, ops in CATS:
        rows.append(f'<tr class="cat"><td colspan="{len(ORDER)+1}">{html.escape(title)}</td></tr>')
        for op in ops:
            b = best(op)
            tds = []
            for e in ORDER:
                v, txt = cell(op, e)
                cls = []
                if e == b: cls.append("best")
                if e.startswith("amber"): cls.append("eamber")
                if v is None: cls.append("na")
                tds.append(f'<td class="{" ".join(cls)}">{txt}</td>')
            rows.append(f'<tr><th class="op"><code>{op}</code><span>{html.escape(DESC.get(op,""))}</span></th>{"".join(tds)}</tr>')
    return f"""<div class="table-wrap bm-matrix"><table>
<thead><tr><th class="op">operation</th>{head}</tr></thead>
<tbody>{"".join(rows)}</tbody></table></div>
<p class="bm-note">Milliseconds, lower is better. <span class="k-best">Amber-coloured</span> is the fastest
single-threaded engine in that row; <code>Amber<sub>14t</sub></code> is shown for reference but is excluded
from that comparison because it is the only multi-core column. <code>skip</code> means the engine has no
faithful way to express the operation under <code>SCOUT_SPEC.md</code>, not that it failed.</p>"""

def h_scaling():
    sizes = sorted(D["scaling"].keys(), key=int)
    ops = [o for _, l in CATS for o in l if o in D["scaling"][sizes[0]]]
    rows = []
    for op in ops:
        tds = []
        for s in sizes:
            v, txt = cell(op, "amber-native", D["scaling"][s])
            tds.append(f"<td>{txt}</td>")
        first, _ = cell(op, "amber-native", D["scaling"][sizes[0]])
        last, _  = cell(op, "amber-native", D["scaling"][sizes[-1]])
        growth = ("&times;%.0f" % (last / first)) if (first and last) else "-"
        rows.append(f'<tr><th class="op"><code>{op}</code></th>{"".join(tds)}<td class="growth">{growth}</td></tr>')
    head = "".join(f"<th>{int(s):,}</th>" for s in sizes)
    return f"""<div class="table-wrap bm-matrix"><table>
<thead><tr><th class="op">operation</th>{head}<th>100k&rarr;10M</th></tr></thead>
<tbody>{"".join(rows)}</tbody></table></div>
<p class="bm-note">Amber (native build), milliseconds. A 100&times; growth for a 100&times; row count is
linear; below 100&times; is superlinear cache behaviour paying off, above it is the algorithm's log factor
or a cache cliff.</p>"""


def headroom():
    """Every op where some single-threaded engine beats Amber, worst gap first."""
    out = []
    for op in [o for _, l in CATS for o in l]:
        a, _ = cell(op, "amber-native")
        if not a: continue
        cands = [(cell(op, e)[0], e) for e in ORDER if e not in ("amber-mt", "amber", "amber-native", "amber-qsql")]
        cands = [(v, e) for v, e in cands if v]
        if not cands: continue
        bv, be = min(cands)
        if bv < a: out.append((op, a, bv, be, a / bv))
    return sorted(out, key=lambda r: -r[4])

def h_headroom():
    rows = "".join(
        f'<tr><th class="op"><code>{op}</code></th><td>{fmt(a)}</td><td class="best">{fmt(bv)}</td>'
        f'<td class="eng">{SHORT[be]}</td><td class="gap">&times;{r:.2f}</td></tr>'
        for op, a, bv, be, r in headroom())
    return f"""<div class="table-wrap bm-matrix"><table>
<thead><tr><th class="op">operation</th><th>Amber (ms)</th><th>best (ms)</th><th>by</th><th>headroom</th></tr></thead>
<tbody>{rows}</tbody></table></div>"""

def md_headroom():
    return md_table(["operation", "Amber (ms)", "best (ms)", "best engine", "headroom"],
        [["`%s`" % op, fmt(a), fmt(bv), SHORT_MD[be], "**%.2fx**" % r] for op, a, bv, be, r in headroom()],
        ["---", "---:", "---:", "---", "---:"])


CSS = """<style>
.bm-meta{display:grid;grid-template-columns:repeat(auto-fit,minmax(250px,1fr));gap:10px 26px;margin:1.4em 0;
  padding:16px 18px;border:1px solid var(--border);border-radius:var(--radius);background:var(--surface)}
.bm-meta>div{display:flex;gap:10px;align-items:baseline;font-size:.86rem}
.bm-k{color:var(--text-faint);text-transform:uppercase;letter-spacing:.08em;font-size:.68rem;font-weight:600;min-width:52px}
.bm-v{color:var(--text-dim);font-family:var(--font-mono);font-size:.8rem}
.dv-chart{margin:1.5em 0;border:1px solid var(--border);border-radius:var(--radius);background:var(--surface);padding:14px 16px}
.dv-head,.dv-row{display:grid;grid-template-columns:120px 1fr 116px 118px;gap:12px;align-items:center}
.dv-head{font-size:.68rem;text-transform:uppercase;letter-spacing:.09em;color:var(--text-faint);
  font-weight:600;padding-bottom:9px;margin-bottom:6px;border-bottom:1px solid var(--border)}
.dv-axis{display:flex;align-items:center;gap:8px;justify-content:center}
.dv-axis em{font-style:normal;opacity:.8}
.dv-axis b{flex:0 0 1px;height:11px;background:var(--border-strong)}
.dv-row{padding:3px 0;font-size:.8rem}
.dv-op{font-family:var(--font-mono);font-size:.76rem;color:var(--text-dim);overflow:hidden;text-overflow:ellipsis}
.dv-track{display:flex;align-items:center;height:14px;border-left:1px solid transparent;border-right:1px solid transparent}
.dv-half{flex:1 1 50%;display:flex;height:9px}
.dv-half.l{justify-content:flex-end;border-right:1px solid var(--border-strong)}
.dv-half.r{justify-content:flex-start}
.dv-bar-win{display:block;height:9px;border-radius:0 3px 3px 0;background:linear-gradient(90deg,var(--accent),var(--accent-deep))}
.dv-bar-loss{display:block;height:9px;border-radius:3px 0 0 3px;background:linear-gradient(270deg,var(--red),#b34a4a)}
.dv-val{font-size:.76rem;font-weight:600;text-align:right}
.dv-val.win{color:var(--accent-soft)} .dv-val.loss{color:var(--red)}
.dv-ms{font-family:var(--font-mono);font-size:.72rem;color:var(--text-faint);text-align:right}
.bm-sum{font-size:.86rem;color:var(--text-dim);margin-top:.6em}
.bm-matrix table{font-size:.79rem;border-collapse:collapse;width:100%}
.bm-matrix th,.bm-matrix td{padding:6px 9px;text-align:right;white-space:nowrap;border-bottom:1px solid var(--border)}
.bm-matrix thead th{position:sticky;top:0;background:var(--surface-solid);z-index:1;font-size:.7rem;
  text-transform:uppercase;letter-spacing:.06em;color:var(--text-faint);font-weight:600}
.bm-matrix th.op{text-align:left;font-weight:500;min-width:190px;position:sticky;left:0;background:var(--surface-solid);z-index:2}
.bm-matrix thead th.op{z-index:3}
.bm-matrix th.op code{font-size:.76rem;color:var(--text)}
.bm-matrix th.op span{display:block;font-size:.68rem;color:var(--text-faint);font-weight:400;white-space:normal;margin-top:1px}
.bm-matrix td{font-family:var(--font-mono);color:var(--text-dim)}
.bm-matrix td.eamber,.bm-matrix thead th.eamber{background:rgba(255,176,32,.045)}
.bm-matrix td.best{color:var(--accent);font-weight:600}
.bm-matrix td.na{color:var(--text-faint);opacity:.55}
.bm-matrix tr.cat td{text-align:left;font-size:.7rem;text-transform:uppercase;letter-spacing:.09em;
  color:var(--accent-soft);font-weight:700;padding-top:15px;background:transparent;border-bottom:1px solid var(--border-strong)}
.bm-matrix td.growth{color:var(--text-faint)}
.bm-matrix td.eng{font-family:var(--font-sans);color:var(--text-dim);text-align:left}
.bm-matrix td.gap{color:var(--red);font-weight:600}
.bm-note{font-size:.8rem;color:var(--text-faint);margin-top:.55em}
.k-best{color:var(--accent);font-weight:600}
@media (max-width:720px){.dv-head,.dv-row{grid-template-columns:96px 1fr 92px}.dv-ms{display:none}}
</style>"""

def html_out():
    return "\n".join([CSS, h_machine(), "<!--DV-->", h_diverging(), "<!--MX-->",
                      h_matrix(), "<!--SC-->", h_scaling(), "<!--HR-->", h_headroom()])

# ------------------------------------------------------------------ MD output
def md_table(headers, rows, align=None):
    al = align or (["---"] + ["---:"] * (len(headers) - 1))
    return ("| " + " | ".join(headers) + " |\n| " + " | ".join(al) + " |\n"
            + "".join("| " + " | ".join(r) + " |\n" for r in rows))

def md_out():
    m = D["machine"]; o = []
    o.append("| | |\n|---|---|")
    for k, v in [("CPU", "%s (%s cores)" % (m["cpu"], m["cores"])), ("SIMD", m["simd"]),
                 ("OS", m["os"]), ("Compiler", m["gcc"]), ("Rows", "N = {:,}".format(D["n"])),
                 ("Runs", "%s timed, %s warm-up" % (D["runs"], D["warmup"])),
                 ("PeachQ", m["peachq"]), ("CBQN", m["cbqn"]),
                 ("NumPy / pandas", "%s / %s" % (m["numpy"], m["pandas"])),
                 ("Polars / DuckDB", "%s / %s" % (m["polars"], m["duckdb"])),
                 ("Amber build", str(m["amber"]))]:
        o.append("| **%s** | %s |" % (k, v))
    o.append("")
    rs = ratios()
    o.append("### Amber vs the C reference — all %d operations\n" % len(rs))
    o.append(md_table(["operation", "Amber (ms)", "C (ms)", "ratio", "what it is"],
        [["`%s`" % op, fmt(a), fmt(q),
          ("**%.2fx faster**" % r) if r >= 1 else ("%.2fx slower" % (1 / r)), DESC.get(op, "")]
         for op, a, q, r in rs],
        ["---", "---:", "---:", "---:", "---"]))
    wins = sum(1 for r in rs if r[3] >= 1)
    o.append("\nAmber is faster on **%d of %d** operations, slower on **%d**.\n" % (wins, len(rs), len(rs) - wins))
    o.append("### The full matrix — %d operations x %d engines\n" % (
        sum(len(l) for _, l in CATS), len(ORDER)))
    hdr = ["operation"] + [SHORT_MD[e] for e in ORDER]
    rows = []
    for title, ops in CATS:
        rows.append(["**%s**" % title] + [""] * len(ORDER))
        for op in ops:
            b = best(op)
            rows.append(["`%s`" % op] + [("**%s**" % cell(op, e)[1]) if e == b else cell(op, e)[1] for e in ORDER])
    o.append(md_table(hdr, rows))
    o.append("\nMilliseconds, lower is better. **Bold** is the fastest single-threaded engine in the row;")
    o.append("`Amber-14t` is excluded from that comparison as the only multi-core column.")
    o.append("`skip` means the engine cannot express the operation faithfully under `SCOUT_SPEC.md`.\n")
    sizes = sorted(D["scaling"].keys(), key=int)
    o.append("### Scaling — Amber (native), 100k to 10M rows\n")
    srows = []
    for _, ops in CATS:
        for op in ops:
            if op not in D["scaling"][sizes[0]]: continue
            f0, _ = cell(op, "amber-native", D["scaling"][sizes[0]])
            fn, _ = cell(op, "amber-native", D["scaling"][sizes[-1]])
            srows.append(["`%s`" % op] + [cell(op, "amber-native", D["scaling"][s])[1] for s in sizes]
                         + [("x%.0f" % (fn / f0)) if (f0 and fn) else "-"])
    o.append(md_table(["operation"] + ["{:,}".format(int(s)) for s in sizes] + ["100k->10M"], srows))
    o.append("\n### Where Amber is beaten, and by how much\n")
    o.append("Every operation where at least one single-threaded engine is faster than Amber,")
    o.append("largest gap first. This is the optimisation backlog, kept public on purpose.\n")
    o.append(md_headroom())
    return "\n".join(o)

if __name__ == "__main__":
    sys.stdout.write(md_out() if "--md" in sys.argv else html_out())
