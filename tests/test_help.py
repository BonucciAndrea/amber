#!/usr/bin/env python3
# tests/test_help.py - the \ help pages: every page renders, with and without colour and in the screen
# reader setting, every example shows the result its page says, and every function a page names exists.
# GNU AGPLv3 - see LICENSE and NOTICE.
#
# The pages are the text at the end of repl.k, so this reads them from there: a page that goes stale fails here.
import os, re, subprocess, sys

root = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
L = open(os.path.join(root, "repl.k"), encoding="utf8").read().split("\n")
pages = L[next(i for i in range(len(L) - 1) if L[i] == "/" and L[i + 1].startswith("-" * 80)):]
ANSI = re.compile(r"\x1b\[[0-9;?]*[a-zA-Z]")

ex, runs, names, keys = [], [], set(), []
page = None
for l in pages:
    if l.startswith("\\") and (len(l) == 2 or l == "\\ menu"):
        page = l
        keys.append(l)
    elif l.startswith("  ") and page != "\\h":
        (ex if " -> " in l else runs).append((page, l[2:]))
    elif "¦" in l:
        f = l.split("¦")
        if len(f) > 2 and " -> " in f[2]:
            ex.append((page, f[2]))
        if page in ("\\q", "\\j", "\\m", "\\o", "\\g", "\\i", "\\z", "\\r"):
            for tok in f[0].split():
                tok = tok.split("[")[0]
                if re.fullmatch(r"\.?[a-zA-Z][a-zA-Z0-9.]*", tok):
                    names.add(tok)
# placeholders on the pages, qSQL keywords (only the prompt's rewrite knows them) and .z.pw, which is yours to set
STOP = {"t", "x", "v", "d", "n", "h", "f", "db", "mode", "expr", "file", "dir", "ns", "lines", "w", "e", "p", "q", "args",
        "msg", "lo", "hi", "s", "g", "xs", "ys", "b", "i", "select", "exec", "update", "delete", "from", "where", "by",
        "date", "attr", ".z.pw"}
names = sorted(n for n in names if n not in STOP)
assert len(keys) > 10 and ex and names, "found no pages in repl.k"

lines = []
for i, (pg, code) in enumerate(ex):
    left, right = code.split(" -> ", 1)
    lines += ['`0:,"@@L%d"' % i, left, '`0:,"@@R%d"' % i, right]
for i, (pg, code) in enumerate(runs):
    lines += ['`0:,"@@X%d"' % i, code]
for i, n in enumerate(names):
    lines += ['`0:,"@@N%d"' % i, "@" + n]
for i, k in enumerate(keys):
    lines += ['`0:,"@@P%d"' % i, "\\" if k == "\\ menu" else k]
inp = "\n".join(lines + ["\\\\"]) + "\n"


def run(**env):
    e = dict(os.environ)
    for k in ("NO_COLOR", "AMBER_NO_EDIT"):
        e.pop(k, None)
    e.update(env)
    r = subprocess.run(["./amber", "repl.k"], cwd=root, input=inp, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                       text=True, env=e, timeout=300)
    blocks, cur = {}, None
    for ln in ANSI.sub("", r.stdout).replace("amber> ", "").split("\n"):
        m = re.fullmatch(r"@@([LRXNP]\d+)", ln.strip())
        if m:
            cur = m.group(1)
            blocks[cur] = []
        elif cur:
            blocks[cur].append(ln)
    return {k: "\n".join(v).strip() for k, v in blocks.items()}


fail = 0
def bad(*a):
    global fail
    fail += 1
    print("  FAIL", *a)

B = run(NO_COLOR="1")
for i, (pg, code) in enumerate(ex):
    l, r = B.get("L%d" % i, "?"), B.get("R%d" % i, "?")
    if l != r or "error[" in l:
        bad(pg, code, "| got:", l.replace("\n", " ")[:100])
for i, (pg, code) in enumerate(runs):
    if "error[" in B.get("X%d" % i, "?"):
        bad(pg, code, "| errors")
for i, n in enumerate(names):
    if "error[" in B.get("N%d" % i, "?"):
        bad("not defined:", n)
for mode, R in (("plain", B), ("colour", run()), ("screen reader", run(AMBER_NO_EDIT="1", NO_COLOR="1"))):
    for i, k in enumerate(keys):
        o = R.get("P%d" % i, "?")
        if "error[" in o or re.search(r"^help: ", o, re.M) or o.count("\n") < 5:
            bad("page", k, "(%s)" % mode, o[:120].replace("\n", " "))
        if mode == "screen reader" and ("─" in o or "⬡" in o):
            bad("page", k, "draws rules for a screen reader")
print("  %d examples, %d names, %d pages, 3 ways: %s" % (len(ex), len(names), len(keys), "FAIL" if fail else "PASS"))
sys.exit(1 if fail else 0)
