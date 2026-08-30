#!/usr/bin/env python3
# tests/test_paste.py - bracketed-paste regression for the REPL line editor.
# GNU AGPLv3 - see LICENSE and NOTICE.
#
# amber 2.0.0: pasting a multi-line script into the REPL (bracketed paste,
# ESC[200~ ... ESC[201~) folds to a `[Pasted text #N +M lines]` placeholder and
# runs the whole block as one batch on Enter -- statements rejoined across lines
# by bracket balance, inline (`x:1 / c`) and full-line (`/ c`) comments handled.
# This drives the real ./a REPL over a pty and asserts the batch executed.
import os, pty, sys, time, select, subprocess

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)

def run(paste_lines):
    payload = "\x1b[200~" + "\n".join(paste_lines) + "\n\x1b[201~"
    m, s = pty.openpty()
    p = subprocess.Popen(["./a"], cwd=ROOT, stdin=s, stdout=s, stderr=s,
                         env={**os.environ, "TERM": "xterm", "AMBER_NO_RLWRAP": "1"})
    os.close(s)
    time.sleep(1.2)                       # let the banner + stdlib load settle
    os.write(m, payload.encode())
    time.sleep(0.6)                       # let the paste fold to the placeholder
    os.write(m, b"\r")                    # Enter on the UNEDITED placeholder runs the batch
    time.sleep(0.8)
    os.write(m, b"\\\\\n")                # then \\ to exit
    out = b""
    t0 = time.time()
    while time.time() - t0 < 6:
        r, _, _ = select.select([m], [], [], 0.3)
        if r:
            try: d = os.read(m, 65536)
            except OSError: break
            if not d: break
            out += d
        elif p.poll() is not None:
            break
    try: p.wait(timeout=3)
    except Exception: p.kill()
    os.close(m)
    return out.decode(errors="replace")

def main():
    # a:10 / b:20 / c is a full-line comment / a+b with a trailing comment
    out = run(["a:10", "b:20 / twenty", "/ a full line comment", "a+b / should print 30"])
    ok = True
    def check(cond, name):
        nonlocal ok
        print(f"  {'PASS' if cond else 'FAIL'} {name}")
        ok = ok and cond
    check("30" in out, "pasted expression with trailing comment ran (a+b -> 30)")
    check("error" not in out.lower() and "not found" not in out.lower(),
          "no error from the full-line / and inline comments")
    # a second paste: a bare qSQL line inside the block
    out2 = run(["t:([]s:`x`y`x; v:1 2 3)", "show select sum v by s from t"])
    check("error" not in out2.lower(), "pasted bare-qSQL block ran without error")
    print("test_paste: " + ("ALL PASSED" if ok else "FAILURES"))
    sys.exit(0 if ok else 1)

if __name__ == "__main__":
    main()
