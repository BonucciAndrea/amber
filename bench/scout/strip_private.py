#!/usr/bin/env python3
"""bench/scout/strip_private.py - remove the non-publishable engines from a
scout results file before it is committed or rendered.

    python3 bench/scout/strip_private.py raw.json > bench/scout/results.json
    python3 bench/scout/strip_private.py --check bench/scout/results.json

WHY THIS EXISTS. scout.py runs every engine it can reach on the machine,
including kdb+/q. Those figures are **not publishable**: the runs are made under
a KX evaluation licence whose terms say the licensee "will not disclose any
benchmark, test or performance information or any report which contains a
competitive analysis regarding the Software to any third party except as
explicitly authorized in advance by us in writing", and no such authorisation
was sought, and the published tables therefore do not mention that engine at
all -- not anonymised, simply absent. This docstring is the record of why;
nothing outside this file states the reason.

Until now that removal was done by hand, which is exactly the kind of step that
gets skipped once. This makes it a command, and `--check` makes it an assertion
that CI or a pre-commit hook can run: it exits non-zero if a private engine is
present anywhere in the file, including in the `engines` map and the `scaling`
buckets, not only in the main matrix.

PeachQ is NOT private. It is an open-source q implementation (Rayforce) under
its own licence and stays in the published tables.

GNU AGPLv3 - see LICENSE and NOTICE.
"""
import json, sys

# Engine keys whose numbers may not leave this machine. Keyed on scout.py's
# Engine.key, so adding a private engine there means adding it here too.
PRIVATE = {"q"}


def strip(doc):
    """Return a copy of `doc` with every PRIVATE engine removed, everywhere."""
    out = dict(doc)
    out["engines"] = {k: v for k, v in doc.get("engines", {}).items()
                      if k not in PRIVATE}
    out["matrix"] = {op: {e: r for e, r in cells.items() if e not in PRIVATE}
                     for op, cells in doc.get("matrix", {}).items()}
    out["scaling"] = {n: {op: {e: r for e, r in cells.items() if e not in PRIVATE}
                          for op, cells in ops.items()}
                      for n, ops in doc.get("scaling", {}).items()}
    # The machine block records each engine's version string; drop those too.
    out["machine"] = {k: v for k, v in doc.get("machine", {}).items()
                      if k not in PRIVATE}
    return out


def offenders(doc):
    """Every place a private engine still appears, as readable paths."""
    bad = []
    for k in doc.get("engines", {}):
        if k in PRIVATE:
            bad.append("engines.%s" % k)
    for op, cells in doc.get("matrix", {}).items():
        for e in cells:
            if e in PRIVATE:
                bad.append("matrix.%s.%s" % (op, e))
    for n, ops in doc.get("scaling", {}).items():
        for op, cells in ops.items():
            for e in cells:
                if e in PRIVATE:
                    bad.append("scaling.%s.%s.%s" % (n, op, e))
    for k in doc.get("machine", {}):
        if k in PRIVATE:
            bad.append("machine.%s" % k)
    return bad


def main(argv):
    check = "--check" in argv
    args = [a for a in argv[1:] if not a.startswith("--")]
    if not args:
        sys.stderr.write(__doc__)
        return 2
    doc = json.load(open(args[0]))
    if check:
        bad = offenders(doc)
        if bad:
            sys.stderr.write("%s: %d private-engine reference(s) present:\n  %s\n"
                             % (args[0], len(bad), "\n  ".join(bad[:20])))
            return 1
        sys.stderr.write("%s: clean (no private engine present)\n" % args[0])
        return 0
    json.dump(strip(doc), sys.stdout, indent=1)
    sys.stdout.write("\n")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
