#!/usr/bin/env python3
# tests/test_statusbar.py - the optional Claude-Code-style REPL status bar (\sb).
# GNU AGPLv3 - see LICENSE and NOTICE.
#
# Drives the real ./a REPL over a pty and asserts: \sb sets a 2-line bottom panel
# (a muted bar + a live info line) with a scroll region above it; the panel
# survives Ctrl-L and \clear; the scroll region is released on exit; and the
# DEFAULT REPL (bar off) emits none of it.
import os, pty, sys, time, select, subprocess, fcntl, termios, struct

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ESC = b"\x1b"
REGION   = ESC + b"[1;22r"                 # scroll region on a 24-row term
MAIN_BG  = ESC + b"[48;2;38;34;30m"
INFO_BG  = ESC + b"[48;2;28;25;22m"
ACCENT   = ESC + b"[1;38;2;217;119;87m"
RELEASE  = ESC + b"[r"
EXIT     = b"\\\\\n"                        # two backslashes + Enter

def drive(cmds, rows=24, cols=90):
    m, s = pty.openpty()
    fcntl.ioctl(s, termios.TIOCSWINSZ, struct.pack("HHHH", rows, cols, 0, 0))
    p = subprocess.Popen(["./a"], cwd=ROOT, stdin=s, stdout=s, stderr=s,
                         env={**os.environ, "TERM": "xterm-256color"})
    os.close(s)
    time.sleep(1.3)
    for b, dt in cmds:
        os.write(m, b); time.sleep(dt)
    out = b""; t0 = time.time()
    while time.time() - t0 < 5:
        r, _, _ = select.select([m], [], [], 0.3)
        if r:
            try: d = os.read(m, 65536)
            except OSError: break
            if not d: break
            out += d
        elif p.poll() is not None: break
    try: p.wait(timeout=3)
    except Exception: p.kill()
    os.close(m)
    return out

ok = True
def check(cond, name):
    global ok
    print(("  PASS " if cond else "  FAIL ") + name); ok = ok and bool(cond)

# 1. bar ON: enable, run a command, Ctrl-L, \clear, resize-ish, then exit
out = drive([(b"\\sb\n", 0.5), (b"2+2\n", 0.4), (b"\x0c", 0.4),
             (b"a:1 2 3\n", 0.4), (b"\\clear\n", 0.4), (EXIT, 0.6)])
after_2j = out.rsplit(ESC + b"[2J", 1)[-1]      # everything after the LAST clear-screen
check(REGION in out,   "scroll region set (rows 1..h-2)")
check(MAIN_BG in out,  "muted main bar painted")
check(INFO_BG in out,  "live info line painted")
check(ACCENT in out,   "Claude coral accent segment")
check("✻ amber".encode() in out, "brand mark + name on the bar")
check(b"MB heap" in out, "info line shows live heap")
check(b"last" in out and b"ms" in out, "info line shows last-command timing")
check(MAIN_BG in after_2j, "bar redrawn after Ctrl-L / \\clear (survives a screen clear)")
check(RELEASE in out,  "scroll region released on exit (terminal not left scrolled-in)")

# 2. bar OFF (default): none of the panel escapes appear
out2 = drive([(b"2+2\n", 0.4), (EXIT, 0.6)])
check(MAIN_BG not in out2 and REGION not in out2, "default REPL is unchanged (no panel when \\sb off)")

print("test_statusbar: " + ("ALL PASSED" if ok else "FAILURES"))
sys.exit(0 if ok else 1)
