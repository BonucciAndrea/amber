#!/usr/bin/env python3
# tests/test_statusbar.py - the Claude-Code-style REPL status bar (default ON).
# GNU AGPLv3 - see LICENSE and NOTICE.
#
# The 2.0.0 status bar is a fixed 4-row footer at the bottom of the terminal:
#     h-3  box top      ╭────────────────────────────────╮
#     h-2  input line   │ amber> <text>                  │   (cursor lives here)
#     h-1  box bottom   ╰────────────────────────────────╯
#      h   info line    ⬡ amber 2.0.0 · exec · mem · [native|portable] …
# Output scrolls in a DECSTBM region above it (rows 1..h-4).  It is ON by default;
# \sb toggles it off.  This drives the real ./a REPL over a pty and asserts the
# footer's escapes; if `pyte` is importable it ALSO renders the screen grid and
# asserts the box is where it should be and that command output actually appears
# in the scroll region (a byte-stream check alone can't see a clobbered cell).
# Deliberately tolerant cross-platform: the build tag is [native] on an
# AMBER_NATIVE CI leg and [portable] otherwise; live RSS may be 0 without /proc.
import os, pty, sys, time, select, subprocess, fcntl, termios, struct, re

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ESC = b"\x1b"
RELEASE = ESC + b"[r"

def drive(cmds, rows=24, cols=100, settle=4.0):
    m, s = pty.openpty()
    fcntl.ioctl(s, termios.TIOCSWINSZ, struct.pack("HHHH", rows, cols, 0, 0))
    p = subprocess.Popen(["./a"], cwd=ROOT, stdin=s, stdout=s, stderr=s,
                         env={**os.environ, "TERM": "xterm-256color"})
    os.close(s)
    out = bytearray()
    def drain(dur):                          # read CONTINUOUSLY so no byte is missed
        end = time.time() + dur
        while time.time() < end:
            r, _, _ = select.select([m], [], [], 0.1)
            if not r: continue
            try: d = os.read(m, 65536)
            except OSError: return
            if not d: return
            out.extend(d)
    drain(2.4)                               # banner + stdlib load (slow on CI runners)
    for b, dt in cmds:
        try: os.write(m, b)
        except OSError: break
        drain(dt)
    drain(settle)
    # Final drain: read everything still queued (the exit/teardown escapes) until
    # EOF -- on a slow runner these arrive after the settle window.
    for _ in range(40):
        r, _, _ = select.select([m], [], [], 0.2)
        if not r:
            if p.poll() is not None: break
            continue
        try: d = os.read(m, 65536)
        except OSError: break
        if not d: break
        out.extend(d)
    try: p.wait(timeout=3)
    except Exception: p.kill()
    try: os.close(m)
    except OSError: pass
    return bytes(out)

TORN = lambda out: (ESC + b"[r" in out) or (ESC + b"[?1049l" in out)  # region reset OR alt-screen leave

def strip(b): return re.sub(rb"\x1b\[[0-9;?]*[A-Za-z]", b"", b).replace(b"\x01", b"").replace(b"\x03", b"")

ok = True
def check(cond, name):
    global ok
    print(("  PASS " if cond else "  FAIL ") + name); ok = ok and bool(cond)

# 1. ON by default: run a command, Ctrl-L, \clear, then exit.
out = drive([(b"2+2\r", 0.5), (b"\x0c", 0.4), (b"a:1 2 3\r", 0.4),
             (b"\\clear\r", 0.4), (b"\\\\\r", 0.6)])
txt = strip(out)
check(ESC + b"[1;20r" in out,                      "DECSTBM scroll region set to rows 1..h-4")
check("╭".encode() in txt and "╮".encode() in txt, "box top border drawn")
check("╰".encode() in txt and "╯".encode() in txt, "box bottom border drawn")
check("│".encode() in txt,                          "box side border drawn")
check(b"amber>" in txt,                             "prompt rendered inside the box")
check("⬡ amber 2.0.0".encode() in txt,             "truecolor hex logo + brand on the info line")
check(b"exec:" in txt and b"mem:" in txt,           "info line shows exec timing + arena size")
check(re.search(rb"\[(native|portable)\]", txt) is not None, "build tag [native]/[portable]")
check(b"4" in txt,                                  "command evaluated (2+2 -> 4)")
check(TORN(out),                                    "scroll region released on exit")

# 2. multi-line paste folds to a placeholder and the batch still runs.
out = drive([(b"\x1b[200~x:40\ny:60\nx+y\x1b[201~", 0.5), (b"\r", 0.6), (b"\\\\\r", 0.6)])
txt = strip(out)
check(re.search(rb"\[Pasted text #\d+ \+3 lines\]", txt) is not None, "multi-line paste folds to placeholder")
check(b"100" in txt,                                "folded paste batch evaluates (x+y -> 100)")

# 3. \sb toggles the footer off and releases the region.
out = drive([(b"\\sb\r", 0.6), (b"9*9\r", 0.5), (b"\\\\\r", 0.6)])
check(TORN(out) and b"81" in strip(out),            "\\sb releases the region; REPL keeps working")

# 3b. Ctrl-V Ctrl-V previews the last folded paste, ENLARGED via DECDWL (\x1b#6).
out = drive([(b"\x1b[200~p:1\nq:2\np+q\x1b[201~", 0.5), (b"\x16", 0.2), (b"\x16", 0.4),
             (b"\r", 0.5), (b"\\\\\r", 0.6)])
txt = strip(out)
check(ESC + b"#6" in out,                           "Ctrl-V Ctrl-V emits DECDWL enlarge for the paste preview")
check(b"Pasted text #" in txt and b"p:1" in txt,    "paste preview shows the stored text")
check(b"3" in txt,                                  "previewed paste still evaluates (p+q -> 3)")

# 3c. Mouse-wheel-up pages the internal scroll-back: the region is repainted with
#     auto-wrap disabled (\x1b[?7l) so a long line can't spill onto the locked box.
cmds = [(str(n).encode() + b"\r", 0.15) for n in range(1, 13)]
cmds += [(b"\x1b[<64;5;5M", 0.15), (b"\x1b[<64;5;5M", 0.15), (b"\x1b[<65;5;5M", 0.15), (b"\\\\\r", 0.5)]
out = drive(cmds)
check(ESC + b"[?7l" in out and ESC + b"[?7h" in out, "wheel-up repaints the scroll-back region (auto-wrap toggled)")

# 4. OPTIONAL rendered-screen check (only if pyte is installed) -- proves the box
#    is where it should be and output actually lands in the scroll region.
try:
    import pyte
    m, s = pty.openpty()
    fcntl.ioctl(s, termios.TIOCSWINSZ, struct.pack("HHHH", 24, 100, 0, 0))
    p = subprocess.Popen(["./a"], cwd=ROOT, stdin=s, stdout=s, stderr=s,
                         env={**os.environ, "TERM": "xterm-256color"})
    os.close(s)
    screen = pyte.Screen(100, 24); stream = pyte.ByteStream(screen)
    def pump(t):
        t0 = time.time()
        while time.time() - t0 < t:
            r, _, _ = select.select([m], [], [], 0.08)
            if r:
                try: d = os.read(m, 65536)
                except OSError: return
                if d: stream.feed(d)
    pump(1.6)
    os.write(m, b"6*7\r"); pump(0.8)
    disp = screen.display
    check(any(l.strip().startswith("╭") for l in disp[18:22]), "[pyte] box top on a footer row")
    check(disp[21].lstrip().startswith("│") and "amber>" in disp[21], "[pyte] input row is the box interior")
    check("⬡ amber 2.0.0" in disp[23], "[pyte] info line on the last row")
    check(any("42" in l for l in disp[:20]), "[pyte] eval output (42) visible in the scroll region")
    check(screen.cursor.y == 21, "[pyte] cursor sits on the box input row")
    # fill past the region, then wheel up: older output must reappear, the box must
    # stay locked, the cursor must stay in the box, and the info line shows SCROLL-BACK.
    for n in range(100, 125): os.write(m, (str(n) + "\r").encode()); pump(0.1)
    pump(0.3)
    for _ in range(14): os.write(m, b"\x1b[<64;5;5M"); pump(0.08)
    pump(0.2); d2 = screen.display
    check(any("100" in l for l in d2[:20]), "[pyte] wheel-up reveals scrolled-off output (100)")
    check(any(l.strip().startswith("╭") for l in d2[18:22]), "[pyte] box stays locked while scrolled")
    check(screen.cursor.y == 21, "[pyte] cursor stays in the box while scrolled")
    check("SCROLL-BACK" in d2[23], "[pyte] info line shows the scroll-back indicator")
    for _ in range(12): os.write(m, b"\x1b[<65;5;5M"); pump(0.08)
    pump(0.2)
    check("⬡ amber 2.0.0" in screen.display[23], "[pyte] wheel-down restores the normal info line")
    os.write(m, b"\\\\\r"); pump(0.3)
    try: os.close(m)
    except OSError: pass
    try: p.wait(timeout=3)
    except Exception: p.kill()
except ImportError:
    print("  SKIP rendered-screen checks (pyte not installed)")

print("test_statusbar: " + ("ALL PASSED" if ok else "FAILURES"))
sys.exit(0 if ok else 1)
