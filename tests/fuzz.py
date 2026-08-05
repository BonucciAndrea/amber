#!/usr/bin/env python3
"""tests/fuzz.py  -  malformed-input / deep-nesting crash fuzzer for Amber.
GNU AGPLv3 - see LICENSE and NOTICE.

Feeds the interpreter a stream of syntactically hostile lines -- unbalanced
brackets, dangling adverbs and modifiers, missing operands, truncated qSQL
clauses, pathological nesting depths, out-of-range numeric literals -- and
asserts that every one of them produces a CLEAN runtime error (a K 'err
message on stderr, exit status 0 or 1) rather than a signal: no SIGSEGV, no
SIGABRT, no SIGBUS, no hang.

usage:
    tests/fuzz.py [--amber ./amber] [--seed N] [--cases N] [--timeout SEC]

Exit status is 0 iff every case terminated cleanly.  Point --amber at a
sanitizer build (see tests/run_tests.sh --asan) to also catch silent
out-of-bounds accesses that do not happen to fault.
"""
import argparse, os, random, shutil, subprocess, sys, tempfile

# Token soup: real Amber verbs/adverbs/literals mixed with the things that
# break naive parsers (unmatched openers/closers, bare adverbs, half-written
# qSQL clauses, numeric literals that overflow every width).
TOKENS = [
    "+", "-", "*", "%", "!", "&", "|", "<", ">", "=", "~", ",", "^", "#", "_",
    "$", "?", "@", ".", ":", ";", "'", "/", "\\", "'/", "\\:", "/:", "':",
    "(", ")", "[", "]", "{", "}", "((", "))", "[[", "]]", "{{", "}}",
    "0", "1", "1 2 3", "1.5", "0N", "0n", "0w", "-0w", "0b", "01b", "`a", "`",
    '"s"', '"', "\\", "x", "y", "z", "t", "()", "!0", "0#0", "::",
    "9223372036854775807", "-9223372036854775808", "1000000000000000000000",
    "1e400", "-1e400", "0x", "1f", "..", "`.`", "`$", "`c$",
    "til", "sum", "avg", "asc", "desc", "distinct", "flip", "raze", "peach",
    "each", "+/", "*\\", "+\\", "&/", "|/", "#[", "_[", "$[", "^[", "@[", ".[", "?[",
    "select", " from ", " by ", " where ", "exec", "update", "delete",
    "sel\"", "qrw\"", "\\t ", "\\ast ", "\\trace ", "\\disasm ", "\\v", "\\m",
    "`s#", "`sa", "`at", "0!", "2!", "1#", "-1#", "0_", "-1_", "1:", "2:", "0:",
]

def random_cases(rng, n):
    for _ in range(n):
        yield "".join(rng.choice(TOKENS) for _ in range(rng.randint(1, 14)))

def structured_cases():
    """Deterministic pathological shapes -- run on every invocation."""
    for d in (8, 64, 512, 4096, 20000):
        yield "(" * d + "1" + ")" * d      # balanced, very deep
        yield "(" * d + "1"                # unbalanced open
        yield "1" + ")" * d                # unbalanced close
        yield "{" * d + "}" * d            # deep lambda nesting
        yield "[" * d + "]" * d            # deep bracket nesting
        yield "+" * d + "1"                # long monadic chain
        yield "1" + "+/" * d               # long adverb chain
        yield "," * d + "1 2 3"            # long enlist chain
        yield "`a" * d                     # long symbol run
        yield '"' + "a" * d                # unterminated string
        yield "{x}" + "/" * d + "1"        # dangling adverbs on a lambda
    # dangling / missing operands
    for s in ("1+", "+", "1 2 3@", "@", "1 2 3[", "]", "{", "}", ";", ":", "::",
              "1+*", "-/", "'", "/", "\\", "':", "/:", "\\:", "~", "^", "$",
              "f:", "f::", ".[", "?[", "@[;", "0N!", "`$", "`c$", "1:", "0:"):
        yield s
    # truncated / malformed qSQL
    for s in ("select", "select from", "select from t", "select by from t",
              "select a from", "select from t where", "select from t by",
              "exec", "exec from t", "update", "update from t", "delete",
              "delete from", "sel\"select\"", "sel\"\"", "qrw\"select\"",
              "select ,, from t", "select a,,b from t", "select from t where ,",
              "t:([]a:1 2 3);select b from t", "t:([]a:1 2 3);select a by b from t"):
        yield s
    # REPL system commands with missing / hostile arguments
    for s in ("\\", "\\ast", "\\ast ", "\\ast (((", "\\trace", "\\trace ",
              "\\trace (((", "\\disasm", "\\disasm (((", "\\t", "\\t:", "\\t:x",
              "\\v", "\\m", "\\d", "\\d .", "\\cd", "\\l", "\\l /nonexistent",
              "\\zzz", "\\ast {x}/////"):
        yield s

FATAL = {"AddressSanitizer", "runtime error:", "Segmentation", "Assertion"}

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--amber", default="./amber")
    ap.add_argument("--seed", type=int, default=1)
    ap.add_argument("--cases", type=int, default=3000)
    ap.add_argument("--timeout", type=float, default=10.0)
    a = ap.parse_args()

    rng = random.Random(a.seed)
    cases = list(structured_cases()) + list(random_cases(rng, a.cases))
    bad, hangs, n = [], 0, 0

    # The token soup contains Amber's file-IO verbs (0: 1: 2:), so a fraction of
    # the generated lines really do write files. Run every child in a throwaway
    # directory so the fuzzer never litters (or overwrites) the working tree.
    amber = os.path.abspath(a.amber)
    sandbox = tempfile.mkdtemp(prefix="amber-fuzz-")

    for src in cases:
        if "\n" in src or "\0" in src:
            continue
        n += 1
        try:
            p = subprocess.run([amber], input=src + "\n", capture_output=True,
                               text=True, timeout=a.timeout, cwd=sandbox)
        except subprocess.TimeoutExpired:
            hangs += 1
            bad.append(("HANG", src, ""))
            continue
        err = p.stderr or ""
        if p.returncode < 0:                       # killed by a signal
            bad.append(("SIG%d" % -p.returncode, src, err[:200]))
        elif any(f in err for f in FATAL):
            bad.append(("SANITIZER", src, err[:200]))

    shutil.rmtree(sandbox, ignore_errors=True)
    print("fuzz: %d cases, %d crashes, %d hangs" % (n, len(bad) - hangs, hangs))
    for kind, src, err in bad[:25]:
        print("  %-9s %r" % (kind, src[:120]))
        if err:
            print("            %s" % err.replace("\n", " ")[:160])
    if len(bad) > 25:
        print("  ... and %d more" % (len(bad) - 25))
    return 1 if bad else 0

if __name__ == "__main__":
    sys.exit(main())
