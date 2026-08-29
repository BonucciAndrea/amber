#!/usr/bin/env python3
# tests/test_statusbar.py - the Claude-Code-style REPL status bar (default ON).
# GNU AGPLv3 - see LICENSE and NOTICE.
#
# Drives the real ./a REPL over a pty and asserts the 2.0.0 status bar:
#   * it is ON by default -- a single info line pinned on row h-1 (the input owns
#     row h) inside a DECSTBM scroll region locking the bottom, with the truecolor
#     hex logo, live exec timing, arena size and the [native]/[portable] build tag;
#   * multi-line paste folds to a `[Pasted text #N +M lines]` placeholder;
#   * it survives Ctrl-L and \clear;
#   * \sb toggles it off and releases the region;
#   * the region is released on every exit path.
# Deliberately tolerant cross-platform: the build tag is [native] on an
# AMBER_NATIVE CI leg and [portable] otherwise, and the live RSS is 0 on a host
# without /proc, so neither exact value is asserted -- only the shape.
import os, pty, sys, time, select, subprocess, fcntl, termios, struct, re

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ESC = b"\x1b"
REGION  = ESC + b"[1;22r"                   # scroll region on a 24-row term (rows 1..h-2)
RELEASE = ESC + b"[r"                       # region reset to full screen
EXIT    = b"\\\\\r"                         # two backslashes + Enter

def drive(cmds, rows=24, cols=120, settle=4.0):
    m, s = pty.openpty()
    fcntl.ioctl(s, termios.TIOCSWINSZ, struct.pack("HHHH", rows, cols, 0, 0))
    p = subprocess.Popen(["./a"], cwd=ROOT, stdin=s, stdout=s, stderr=s,
                         env={**os.environ, "TERM": "xterm-256color"})
    os.close(s)
    time.sleep(1.6)
    for b, dt in cmds:
        try: os.write(m, b)
        except OSError: break
        time.sleep(dt)
    out = b""; t0 = time.time()
    while time.time() - t0 < settle:
        r, _, _ = select.select([m], [], [], 0.3)
        if r:
            try: d = os.read(m, 65536)
            except OSError: break
            if not d: break
            out += d
        elif p.poll() is not None: break
    try: p.wait(timeout=3)
    except Exception: p.kill()
    try: os.close(m)
    except OSError: pass
    return out

def strip(b): return re.sub(rb"\x1b\[[0-9;?]*[A-Za-z]", b"", b).replace(b"\x01", b"").replace(b"\x03", b"")

ok = True
def check(cond, name):
    global ok
    print(("  PASS " if cond else "  FAIL ") + name); ok = ok and bool(cond)

# 1. ON by default: run a command, Ctrl-L, \clear, then exit.
out = drive([(b"2+2\r", 0.5), (b"\x0c", 0.4), (b"a:1 2 3\r", 0.4),
             (b"\\clear\r", 0.4), (EXIT, 0.6)])
txt = strip(out)
after_clear = out.rsplit(ESC + b"[2J", 1)[-1]
check(REGION in out,                       "scroll region set (rows 1..h-2)")
check("⬡ amber 2.0.0".encode() in txt,     "truecolor hex logo + brand + version")
check(b"exec:" in txt and b"ms" in txt,    "info line shows last-command exec timing")
check(b"mem:" in txt and b"MB" in txt,     "info line shows arena size")
check(re.search(rb"\[(native|portable)\]", txt) is not None, "build tag [native]/[portable]")
check(b"Tab complete" in txt and b"exit" in txt, "right-side key hints present")
check(b"4" in strip(out),                  "command evaluated (2+2 -> 4)")
check(REGION.decode().encode() in after_clear or b"amber 2.0.0" in strip(after_clear),
                                           "bar redrawn after Ctrl-L / \\clear")
check(RELEASE in out,                      "scroll region released on exit")

# 2. multi-line paste folds to a placeholder and the batch still runs.
out = drive([(b"\x1b[200~x:40\ny:60\nx+y\x1b[201~", 0.5), (b"\r", 0.6), (EXIT, 0.6)])
txt = strip(out)
check(re.search(rb"\[Pasted text #\d+ \+3 lines\]", txt) is not None, "multi-line paste folds to placeholder")
check(b"100" in txt,                       "folded paste batch evaluates (x+y -> 100)")

# 3. \sb toggles the bar OFF and releases the region.
out = drive([(b"\\sb\r", 0.6), (b"9*9\r", 0.5), (EXIT, 0.6)])
check(RELEASE in out and b"81" in strip(out), "\\sb releases the region; REPL keeps working")

print("test_statusbar: " + ("ALL PASSED" if ok else "FAILURES"))
sys.exit(0 if ok else 1)
