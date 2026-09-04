#!/usr/bin/env python3
# tests/test_statusbar.py - the Claude-Code-style REPL status bar (default ON).
# GNU AGPLv3 - see LICENSE and NOTICE.
#
# The 2.0.0 status bar is a fixed 4-row footer at the bottom of the terminal:
#     h-3  box top      ╭────────────────────────────────╮
#     h-2  input line   │ amber> <text>                  │   (cursor lives here)
#     h-1  box bottom   ╰────────────────────────────────╯
#      h   info line    ⬡ amber 2.0.1 · exec · mem · [native|portable] …
# Output scrolls in a DECSTBM region above it (rows 1..h-4).  It is ON by default;
# \sb toggles it off.  This drives the real ./a REPL over a pty and asserts the
# footer's escapes; if `pyte` is importable it ALSO renders the screen grid and
# asserts the box is where it should be and that command output actually appears
# in the scroll region (a byte-stream check alone can't see a clobbered cell).
# Deliberately tolerant cross-platform: the build tag is [native] on an
# AMBER_NATIVE CI leg and [portable] otherwise; live RSS may be 0 without /proc.
import os, pty, sys, time, select, subprocess, fcntl, termios, struct, re, signal

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
check("⬡ amber 2.0.1".encode() in txt,             "truecolor hex logo + brand on the info line")
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
check(ESC + b"#6" not in out,                       "paste preview does NOT use DECDWL (renders cleanly on every terminal)")
check(b"Pasted text #" in txt and b"p:1" in txt,    "paste preview shows the stored text with a line-numbered gutter")
check(b"3" in txt,                                  "previewed paste still evaluates (p+q -> 3)")

# 3c. Mouse-wheel-up pages the internal scroll-back: the region is repainted with
#     auto-wrap disabled (\x1b[?7l) so a long line can't spill onto the locked box.
cmds = [(str(n).encode() + b"\r", 0.15) for n in range(1, 13)]
cmds += [(b"\x1b[<64;5;5M", 0.15), (b"\x1b[<64;5;5M", 0.15), (b"\x1b[<65;5;5M", 0.15), (b"\\\\\r", 0.5)]
out = drive(cmds)
check(ESC + b"[?7l" in out and ESC + b"[?7h" in out, "wheel-up repaints the scroll-back region (auto-wrap toggled)")

# 3d. Deleting the folded placeholder (Ctrl-U) must ABANDON the batch, not run it.
out = drive([(b"\x1b[200~qa:12340\nqb:5\nqa+qb\x1b[201~", 0.5), (b"\x15", 0.2), (b"\r", 0.5), (b"\\\\\r", 0.5)])
check(b"12345" not in strip(out),                   "deleting the paste placeholder abandons the batch")

# 3e. Alt-V Alt-V previews the paste too (fallback for terminals that eat Ctrl-V, e.g. WSL).
out = drive([(b"\x1b[200~za:3\nzb:4\nza+zb\x1b[201~", 0.5), (b"\x1bv", 0.2), (b"\x1bv", 0.4), (b"\\\\\r", 0.5)])
txt = strip(out)
check(b"Pasted text #" in txt and b"za:3" in txt,   "Alt-V Alt-V shows the paste preview (WSL fallback)")

# 3f. Claude-Code style: pasting the SAME block again views the full text; the
#     placeholder carries a "paste again to view" hint and still runs on Enter.
blk = b"\x1b[200~ra:1\nrb:2\nra+rb\x1b[201~"
out = drive([(blk, 0.5), (blk, 0.6), (b"\r", 0.4), (b"\\\\\r", 0.5)])
txt = strip(out)
check(b"paste again to view" in txt,                "placeholder shows the 'paste again to view' hint")
check(b"ra:1" in txt and b"ra+rb" in txt,           "paste-again reveals the full pasted text")
check(b"3" in txt,                                  "paste-again keeps the batch runnable (ra+rb -> 3)")

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
    check("⬡ amber 2.0.1" in disp[23], "[pyte] info line on the last row")
    check(any("42" in l for l in disp[:20]), "[pyte] eval output (42) visible in the scroll region")
    check(screen.cursor.y == 21, "[pyte] cursor sits on the box input row")
    # scrolling DOWN while already at live must not touch/break the box
    box_before = list(screen.display[20:23])
    for _ in range(4): os.write(m, b"\x1b[<65;5;5M"); pump(0.06)   # wheel down at live
    os.write(m, b"\x1b[6~"); pump(0.2)                             # PageDown at live
    check(list(screen.display[20:23]) == box_before and screen.display[20].lstrip().startswith("╭"),
          "[pyte] scroll-down at live leaves the box intact")
    # fill past the region, then wheel up: older output must reappear, the box must
    # stay locked, the cursor must stay in the box, and the info line shows SCROLL-BACK.
    for n in range(100, 125): os.write(m, (str(n) + "\r").encode()); pump(0.1)
    pump(0.3)
    for _ in range(14): os.write(m, b"\x1b[<64;5;5M"); pump(0.08)
    pump(0.2); d2 = screen.display
    check(any("100" in l for l in d2[:20]), "[pyte] wheel-up reveals scrolled-off output (100)")
    check(any(l.strip().startswith("╭") for l in d2[18:22]), "[pyte] box stays locked while scrolled")
    check(screen.cursor.y == 21, "[pyte] cursor stays in the box while scrolled")
    check("SCROLL" in d2[23] and "lines up" in d2[23], "[pyte] info line shows the scroll-back indicator")
    for _ in range(12): os.write(m, b"\x1b[<65;5;5M"); pump(0.08)
    pump(0.2)
    check("⬡ amber 2.0.1" in screen.display[23], "[pyte] wheel-down restores the normal info line")
    # an ERROR must survive scroll-back too (errors go to stderr, a separate path).
    # Scroll up a line at a time and confirm the error text reappears somewhere.
    os.write(m, b"undefined_zzz\r"); pump(0.4)
    for n in range(200, 212): os.write(m, (str(n) + "\r").encode()); pump(0.08)
    pump(0.3)
    saw_scroll = err_back = False
    def haserr(disp): return any(("undefined_zzz" in l) or ("E0101" in l) or ("not found" in l) for l in disp)
    for _ in range(12):
        os.write(m, b"\x1b[5~"); pump(0.12)                        # PageUp (keyboard scroll)
        if "SCROLL" in screen.display[23]: saw_scroll = True
        if haserr(screen.display): err_back = True; break
    check(saw_scroll, "[pyte] PageUp scrolls the transcript")
    check(err_back, "[pyte] error diagnostics survive scroll-back")
    # horizontal scroll: a table wider than the 100-col terminal must NOT wrap; the
    # box lines stay intact (no-wrap clips them), Shift+Right (CSI 1;2C) pans right
    # and the info line shows the horizontal indicator; Shift+Left returns to live.
    os.write(m, b"+(`$\"c\",'$!16)!16#,10000+!10\r"); pump(0.5)
    def boxrow(disp): return next((l for l in disp[:20] if l.strip().startswith("│") and "10000" in l), "")
    left_view = boxrow(screen.display)
    check(left_view != "" and len(left_view.rstrip()) <= 100, "[pyte] wide table clips to the terminal (no wrap)")
    for _ in range(8): os.write(m, b"\x1b[1;2C"); pump(0.08)   # Shift+Right x8 -> pan right
    pump(0.2)
    check("cols right" in screen.display[23], "[pyte] horizontal scroll shows the pan indicator")
    check(boxrow(screen.display) != left_view, "[pyte] horizontal scroll pans the view to hidden columns")
    for _ in range(30): os.write(m, b"\x1b[1;2D"); pump(0.05)  # Shift+Left -> back to the left edge
    pump(0.2)
    check("cols right" not in screen.display[23], "[pyte] Shift+Left returns to the left edge")
    os.write(m, b"\\\\\r"); pump(0.3)
    try: os.close(m)
    except OSError: pass
    try: p.wait(timeout=3)
    except Exception: p.kill()
except ImportError:
    print("  SKIP rendered-screen checks (pyte not installed)")

# 5. Resize / zoom.  On macOS a Cmd-+ / Cmd-- zoom (or a pinch) changes rows AND
#    columns and fires a BURST of SIGWINCHes; dragging a window edge does the same
#    more slowly.  Three things must hold afterwards, on every terminal:
#      * exactly ONE footer -- terminals disagree about what a resize does to the
#        old screen contents (xterm clips the alt screen at the bottom, Terminal.app
#        and iTerm2 keep the bottom and shift up), so a repaint that erases "where
#        the old footer was" strands a second box: the duplicated box bug.
#      * the transcript survives, repainted from the scroll-back ring.
#      * the box spans the FULL width at any width -- the pad used to run through a
#        fixed 256/300-byte buffer and silently clamped past ~313 columns, leaving
#        the input row's right border short of the edge while the borders above and
#        below still reached it (a Retina Mac zoomed out is easily 300-400 columns).
try:
    import pyte

    def zoom_session(steps, R0=24, C0=100, cmds=(b"1*1\r", b"2*2\r", b"3*3\r")):
        m, s = pty.openpty()
        fcntl.ioctl(s, termios.TIOCSWINSZ, struct.pack("HHHH", R0, C0, 0, 0))
        def ctty():                      # a real controlling tty, so the KERNEL sends SIGWINCH
            os.setsid()
            try: fcntl.ioctl(0, termios.TIOCSCTTY, 0)
            except Exception: pass
        p = subprocess.Popen(["./a"], cwd=ROOT, stdin=s, stdout=s, stderr=s,
                             preexec_fn=ctty, env={**os.environ, "TERM": "xterm-256color"})
        os.close(s)
        sc = pyte.Screen(C0, R0); st = pyte.ByteStream(sc)
        def pump(t):
            t0 = time.time()
            while time.time() - t0 < t:
                r, _, _ = select.select([m], [], [], 0.08)
                if r:
                    try: d = os.read(m, 65536)
                    except OSError: return
                    if d: st.feed(d)
        pump(1.8)
        for c in cmds: os.write(m, c); pump(0.4)
        for (rr, cc) in steps:
            fcntl.ioctl(m, termios.TIOCSWINSZ, struct.pack("HHHH", rr, cc, 0, 0))
            try: os.kill(p.pid, signal.SIGWINCH)     # belt and braces on odd CI ptys
            except Exception: pass
            time.sleep(0.05)
            sc.resize(rr, cc)
            pump(0.7)
        os.write(m, b"4*4\r"); pump(0.7)
        try: os.write(m, b"\\\\\r")
        except OSError: pass
        pump(0.4)
        try: os.close(m)
        except OSError: pass
        try: p.wait(timeout=3)
        except Exception: p.kill()
        return [l.rstrip() for l in sc.display]

    def one_footer(d):
        return (sum(1 for l in d if l.lstrip().startswith("\u256d")),
                sum(1 for l in d if l.lstrip().startswith("\u2570")),
                sum(1 for l in d if "\u2b21 amber" in l)) == (1, 1, 1)

    d = zoom_session([(16, 70)])
    check(one_footer(d),                     "[pyte] zoom in (24x100 -> 16x70): exactly one footer")
    check(all(v in d for v in ("1", "4", "9")), "[pyte] zoom in keeps the transcript")

    d = zoom_session([(40, 130)])
    check(one_footer(d),                     "[pyte] zoom out (24x100 -> 40x130): exactly one footer")
    check(all(v in d for v in ("1", "4", "9")), "[pyte] zoom out keeps the transcript")

    d = zoom_session([(34, 104), (29, 90), (25, 78), (21, 68)])
    check(one_footer(d),                     "[pyte] burst of zoom steps: exactly one footer")

    d = zoom_session([(24, 60)])             # width-only: the region must still be re-set
    check(one_footer(d),                     "[pyte] width-only resize: exactly one footer")

    d = zoom_session([], R0=24, C0=320, cmds=(b"1*1\r",))
    top  = [l for l in d if l.lstrip().startswith("\u256d")]
    side = [l for l in d if l.lstrip().startswith("\u2502")]
    info = [l for l in d if "\u2b21 amber" in l]
    check(bool(top) and len(top[0]) == 320 and bool(side) and len(side[0]) == 320,
          "[pyte] box spans the full width at 320 columns")
    check(bool(info) and info[0].endswith("exit"),
          "[pyte] info line right-aligns to the edge at 320 columns")
except ImportError:
    print("  SKIP resize/zoom checks (pyte not installed)")

# 6. The exec timer, and recovery from a terminal-side clear (macOS Cmd-K).
#    The timer bug this pins: k has NO bare ".1" float literal -- ".1" lexes as the
#    verb "." applied to 1, which yields 1 -- so `.1*_0.5+10*sblast` silently
#    reported TEN TIMES the real time (a 23 ms line read "230 ms"). Comparing the
#    bar against \t for the SAME expression catches that regardless of how fast the
#    machine is, which a fixed millisecond bound could not.
try:
    import pyte

    def sess(rows=30, cols=120):
        m, s = pty.openpty()
        fcntl.ioctl(s, termios.TIOCSWINSZ, struct.pack("HHHH", rows, cols, 0, 0))
        def ctty():
            os.setsid()
            try: fcntl.ioctl(0, termios.TIOCSCTTY, 0)
            except Exception: pass
        p = subprocess.Popen(["./a"], cwd=ROOT, stdin=s, stdout=s, stderr=s,
                             preexec_fn=ctty, env={**os.environ, "TERM": "xterm-256color"})
        os.close(s)
        sc = pyte.Screen(cols, rows); st = pyte.ByteStream(sc)
        def pump(t):
            t0 = time.time()
            while time.time() - t0 < t:
                r, _, _ = select.select([m], [], [], 0.03)
                if r:
                    try: d = os.read(m, 65536)
                    except OSError: return
                    if d: st.feed(d)
        pump(2.2)
        return m, p, sc, st, pump

    def barms(sc):
        mm = re.search(r"exec:\s*([0-9]+\.[0-9]+)\s*ms", sc.display[-1])
        return float(mm.group(1)) if mm else None

    def lastout(sc, rows=30):
        return [l.rstrip() for l in sc.display[:rows - 4] if l.strip()][-1]

    m, p, sc, st, pump = sess()
    os.write(m, b"1+1\r"); pump(1.2)
    v = barms(sc)
    check(re.search(r"exec:\s*[0-9]+\.[0-9]{3}\s*ms", sc.display[-1]) is not None,
          "[pyte] exec figure carries exactly 3 decimals")
    # \t reports the pure eval in whole ms; the bar reports the whole line. They must
    # be the same ORDER OF MAGNITUDE -- a 10x units bug blows this apart.
    os.write(m, b"\\t gentq 50000\r"); pump(3.0)
    try: tms = float(re.sub(r"[^0-9.]", "", lastout(sc)) or "0")
    except ValueError: tms = 0.0
    os.write(m, b"gentq 50000\r"); pump(3.0)
    bms = barms(sc)
    if tms >= 5 and bms is not None:
        check(bms < (3 * tms + 25), "[pyte] bar agrees with \\t (no 10x units bug): "
              "bar=%.1f ms vs \\t=%.0f ms" % (bms, tms))
        check(bms > (0.3 * tms), "[pyte] bar is not absurdly low vs \\t")
    else:
        print("  SKIP timer-vs-\\t (machine too fast to time gentq 50000 in whole ms)")
    # A trivial line must not be charged the REPL's own overhead: fmt used to fork
    # `tput` for the terminal width on EVERY value (~3-6 ms/line on macOS, where fork
    # cost scales with the 1 GB reserved heap). The size now comes from `bi 0.
    os.write(m, b"1+1\r"); pump(1.2)
    v = barms(sc)
    check(v is not None and v < 2.0, "[pyte] a trivial line costs <2 ms (no tput fork per value)")

    # macOS Cmd-K clears the terminal LOCALLY and sends the program nothing, so the
    # footer is wiped with no event to react to. The next keystroke must restore it.
    st.feed(b"\x1b[H\x1b[2J\x1b[3J")          # the terminal clears itself; the app never sees this
    gone = not any(l.lstrip().startswith("╭") for l in sc.display)
    os.write(m, b"\r"); pump(1.0)             # recovery is at the PROMPT, not per keystroke
    back = [l.rstrip() for l in sc.display]
    check(gone, "[pyte] simulated Cmd-K wipes the footer (precondition)")
    check(sum(1 for l in back if l.lstrip().startswith("╭")) == 1
          and sum(1 for l in back if l.lstrip().startswith("╰")) == 1
          and any("⬡ amber" in l for l in back),
          "[pyte] the next prompt after Cmd-K repaints the whole footer")
    os.write(m, b"2+2\r"); pump(1.0)
    check(any(l.strip() == "4" for l in sc.display), "[pyte] REPL still evaluates after Cmd-K")

    # Caret stability: ONE keystroke must move the cursor only within the input
    # row.  Repainting the borders/info line per keystroke (tried, for Cmd-K
    # recovery) makes the caret visibly flick to the rows above and below the
    # input line on every character typed.  Assert on the emitted bytes, since a
    # rendered grid cannot see a cursor that came back before the next flush.
    m2, p2 = None, None
    try:
        m2, s2 = pty.openpty()
        fcntl.ioctl(s2, termios.TIOCSWINSZ, struct.pack("HHHH", 30, 120, 0, 0))
        def ctty2():
            os.setsid()
            try: fcntl.ioctl(0, termios.TIOCSCTTY, 0)
            except Exception: pass
        p2 = subprocess.Popen(["./a"], cwd=ROOT, stdin=s2, stdout=s2, stderr=s2,
                              preexec_fn=ctty2, env={**os.environ, "TERM": "xterm-256color"})
        os.close(s2)
        acc = bytearray()
        def drain2(t):
            t0 = time.time()
            while time.time() - t0 < t:
                r, _, _ = select.select([m2], [], [], 0.03)
                if r:
                    try: d = os.read(m2, 65536)
                    except OSError: return
                    if not d: return
                    acc.extend(d)
        drain2(2.2)
        touched = set()
        for ch in b"abc":
            acc.clear()
            os.write(m2, bytes([ch])); drain2(0.5)
            touched |= {int(x) for x in re.findall(rb"\x1b\[(\d+);\d+H", bytes(acc))}
        check(touched <= {28}, "[pyte] a keystroke moves the caret only on the input "
              "row (no flicker); rows touched=%s" % sorted(touched))
        os.write(m2, b"\\\\\r"); drain2(0.5)
    finally:
        if m2 is not None:
            try: os.close(m2)
            except OSError: pass
        if p2 is not None:
            try: p2.wait(timeout=3)
            except Exception: p2.kill()
    os.write(m, b"\\\\\r"); pump(0.5)
    try: os.close(m)
    except OSError: pass
    try: p.wait(timeout=3)
    except Exception: p.kill()
except ImportError:
    print("  SKIP timer / Cmd-K checks (pyte not installed)")

# 7. UTF-8: the buffer holds BYTES, the terminal lays out CELLS.  Conflating the
#    two put the box's closing border in the wrong column for any non-ASCII input
#    (an accented letter is 2 bytes but 1 cell; CJK and emoji are 1 lead byte but
#    2 cells), and made Backspace delete ONE BYTE of a multi-byte character,
#    leaving a broken sequence in the buffer and a replacement glyph on screen.
try:
    import pyte, unicodedata

    def cells(row):   # pyte collapses a wide char into one display cell
        return sum(2 if unicodedata.east_asian_width(c) in ("W", "F") else 1 for c in row)

    def utf8_sess(rows=24, cols=60):
        m, sl = pty.openpty()
        fcntl.ioctl(sl, termios.TIOCSWINSZ, struct.pack("HHHH", rows, cols, 0, 0))
        def ctty():
            os.setsid()
            try: fcntl.ioctl(0, termios.TIOCSCTTY, 0)
            except Exception: pass
        pr = subprocess.Popen(["./a"], cwd=ROOT, stdin=sl, stdout=sl, stderr=sl,
                              preexec_fn=ctty, env={**os.environ, "TERM": "xterm-256color"})
        os.close(sl)
        sc = pyte.Screen(cols, rows); st = pyte.ByteStream(sc)
        def pump(t):
            t0 = time.time()
            while time.time() - t0 < t:
                r, _, _ = select.select([m], [], [], 0.03)
                if r:
                    try: d = os.read(m, 65536)
                    except OSError: return
                    if not d: return
                    st.feed(d)
        pump(2.2)
        return m, pr, sc, pump

    m, pr, sc, pump = utf8_sess()
    def inputrow():
        rows_ = [l.rstrip() for l in sc.display[-4:] if l.lstrip().startswith("│")]
        return rows_[0] if rows_ else ""
    for label, txt in [("accents", 'x:"città però"'),
                       ("CJK",     'z:"日本語"'),
                       ("emoji",   'w:"\U0001f600\U0001f680"'),
                       ("mixed",   'q:"aà日\U0001f600z"')]:
        os.write(m, b"\x15"); pump(0.2)
        os.write(m, txt.encode()); pump(0.5)
        row = inputrow()
        check(cells(row) == 60 and row.count("│") == 2 and row.endswith("│"),
              "[pyte] %s: box closes exactly at the terminal edge" % label)
    # Backspace must remove a whole character, not one byte
    os.write(m, b"\x15"); pump(0.2)
    os.write(m, 'y:"città"'.encode()); pump(0.4)
    os.write(m, b"\x7f\x7f"); pump(0.4)
    row = inputrow()
    check("�" not in row and "citt" in row,
          "[pyte] Backspace over a multi-byte character leaves no broken byte")
    # A long line must not be quadratic: the caret window is computed from the
    # caret backwards, not by re-measuring the whole line on every keystroke.
    os.write(m, b"\x15"); pump(0.3)
    t0 = time.time()
    payload = b"x:" + b"9" * 3000
    for i in range(0, len(payload), 256):
        os.write(m, payload[i:i+256]); pump(0.05)
    pump(0.8)
    dt = time.time() - t0
    check(dt < 30, "[pyte] a 3000-char line stays responsive (%.1fs, no quadratic redraw)" % dt)
    check(cells(inputrow()) == 60, "[pyte] box still exact on a 3000-char line")
    os.write(m, b"\x15\r"); pump(0.5)
    os.write(m, b"6*7\r"); pump(1.0)
    check(any(l.strip() == "42" for l in sc.display), "[pyte] REPL evaluates after UTF-8/long-line abuse")
    os.write(m, b"\\\\\r"); pump(0.5)
    try: os.close(m)
    except OSError: pass
    try: pr.wait(timeout=3)
    except Exception: pr.kill()
except ImportError:
    print("  SKIP UTF-8 checks (pyte not installed)")

# 8. Multi-line continuation. A line leaving a bracket open is not a statement
#    yet, so the editor keeps reading. There is deliberately no key for it:
#    Shift-Enter is indistinguishable from Enter in a terminal (both send CR)
#    unless the user opts into an extended keyboard protocol. Depth comes from
#    the same string/comment-aware counter the paste path uses, so a function
#    typed by hand and one pasted produce identical text.
try:
    import pyte

    m, pr, sc, pump = utf8_sess(rows=26, cols=90)
    def row():
        r = [l.rstrip() for l in sc.display[-4:] if l.lstrip().startswith("│")]
        return r[0] if r else ""
    def shown(v): return any(l.strip() == v for l in sc.display)

    os.write(m, b"f:{\r"); pump(0.7)
    check("...>" in row(), "[pyte] an open bracket shows the continuation prompt")
    check(sum(1 for l in sc.display[-4:] if l.lstrip().startswith("╭")) == 1,
          "[pyte] footer intact during continuation")
    os.write(m, b"x+1\r"); pump(0.6)
    os.write(m, b"}\r");   pump(1.0)
    os.write(m, b"f 41\r"); pump(1.2)
    check(shown("42"), "[pyte] hand-typed multi-line function is defined and callable")
    # history must hold the JOINED statement, not the last fragment. Two Ctrl-P:
    # the most recent entry is "f 41", the one before it is the definition.
    os.write(m, b"\x10"); pump(0.4)
    os.write(m, b"\x10"); pump(0.5)
    check("f:{ x+1 }" in row(), "[pyte] history holds the joined statement, not a fragment")
    os.write(m, b"\x15"); pump(0.3)
    # a bracket inside a string or after a comment must NOT trigger continuation
    for expr, want in [(b'"a{b"', '"a{b"'), (b"1+1 / a { comment", "2"), (b"{x*2}[21]", "42")]:
        os.write(m, b"\x15"); pump(0.2)
        os.write(m, expr + b"\r"); pump(1.2)
        check(shown(want) and "...>" not in row(),
              "[pyte] no false continuation for %s" % expr.decode())
    # Ctrl-C must abandon a continuation and leave the REPL usable
    os.write(m, b"h:{\r"); pump(0.6)
    os.write(m, b"\x03"); pump(0.6)
    check("...>" not in row(), "[pyte] Ctrl-C abandons the continuation")
    os.write(m, b"6*7\r"); pump(1.2)
    check(shown("42"), "[pyte] REPL still evaluates after abandoning a continuation")
    os.write(m, b"\\\\\r"); pump(0.5)
    try: os.close(m)
    except OSError: pass
    try: pr.wait(timeout=3)
    except Exception: pr.kill()
except ImportError:
    print("  SKIP continuation checks (pyte not installed)")

print("test_statusbar: " + ("ALL PASSED" if ok else "FAILURES"))
sys.exit(0 if ok else 1)
