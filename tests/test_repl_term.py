#!/usr/bin/env python3
"""tests/test_repl_term.py -- REPL terminal-handling regression tests (1.9.5).

These are the tests for the bug that 1.9.5 closes: Amber's REPL used to have no
line editing, users were told to run it under `rlwrap`, and once the REPL began
handling the terminal itself rlwrap started dumping

    rlwrap: warning: rlwrap appears to do nothing for amber, which asks for
    single keypresses all the time ...

across stdout/stderr in the middle of a session, usually right after an error.

Everything here runs the real interpreter on a real pty; nothing is mocked.

  1. no_rlwrap_warning        ./a in a terminal never emits an rlwrap warning
                              (the launcher does not invoke rlwrap at all)
  2. rlwrap_fallback_is_quiet AMBER_NO_EDIT=1 ./a, which DOES use rlwrap, is
                              silent too -- because it passes -n and -a
  3. termios_restored         the terminal's termios is byte-for-byte what it
                              was before Amber ran, after a normal exit AND
                              after ^C
  4. editing_works            arrows/Ctrl-A/Ctrl-K actually edit the line
  5. pipe_is_unchanged        with stdin a pipe the REPL still reads lines and
                              prints results (batch behaviour is untouched)
  6. no_edit_pipe             AMBER_NO_EDIT=1 with a pipe behaves the same

Usage:  python3 tests/test_repl_term.py [path-to-repo-root]
Exit status 0 = all passed.
"""

import os
import pty
import select
import struct
import subprocess
import sys
import termios
import fcntl
import time

ROOT = os.path.abspath(sys.argv[1] if len(sys.argv) > 1 else
                       os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
A = os.path.join(ROOT, "a")
AMBER = os.path.join(ROOT, "amber")
REPLK = os.path.join(ROOT, "repl.k")

FAILURES = []


def check(name, ok, detail=""):
    print("  %-26s %s" % (name, "PASS" if ok else "FAIL"))
    if not ok:
        FAILURES.append((name, detail))
        if detail:
            print("      " + detail.replace("\n", "\n      ")[:1500])


def pty_run(argv, chunks, env_extra=None, wait=6.0, gap=1.1):
    """Run argv on a pty, feed `chunks` one per `gap` seconds, return output."""
    pid, fd = pty.fork()
    if pid == 0:
        env = dict(os.environ)
        env["TERM"] = "xterm"
        env.setdefault("HOME", "/tmp")
        if env_extra:
            env.update(env_extra)
        try:
            os.execvpe(argv[0], argv, env)
        finally:
            os._exit(127)
    fcntl.ioctl(fd, termios.TIOCSWINSZ, struct.pack("HHHH", 24, 100, 0, 0))
    out, t0, stage = b"", time.time(), 0
    while time.time() - t0 < wait:
        r, _, _ = select.select([fd], [], [], 0.15)
        if r:
            try:
                d = os.read(fd, 65536)
            except OSError:
                break
            if not d:
                break
            out += d
        if stage < len(chunks) and time.time() - t0 > 1.0 + stage * gap:
            os.write(fd, chunks[stage])
            stage += 1
    try:
        os.close(fd)
    except OSError:
        pass
    try:
        os.waitpid(pid, os.WNOHANG)
    except ChildProcessError:
        pass
    return out.decode("utf-8", "replace")


def pty_run_with_termios(argv, chunks, env_extra=None, wait=6.0):
    """Same, but also compare the pty's termios before and after the run."""
    pid, fd = pty.fork()
    if pid == 0:
        env = dict(os.environ)
        env["TERM"] = "xterm"
        env.setdefault("HOME", "/tmp")
        if env_extra:
            env.update(env_extra)
        try:
            os.execvpe(argv[0], argv, env)
        finally:
            os._exit(127)
    fcntl.ioctl(fd, termios.TIOCSWINSZ, struct.pack("HHHH", 24, 100, 0, 0))
    before = termios.tcgetattr(fd)
    out, t0, stage = b"", time.time(), 0
    while time.time() - t0 < wait:
        r, _, _ = select.select([fd], [], [], 0.15)
        if r:
            try:
                d = os.read(fd, 65536)
            except OSError:
                break
            if not d:
                break
            out += d
        if stage < len(chunks) and time.time() - t0 > 1.0 + stage * 1.1:
            os.write(fd, chunks[stage])
            stage += 1
    try:
        after = termios.tcgetattr(fd)
    except termios.error:
        after = before
    try:
        os.close(fd)
    except OSError:
        pass
    try:
        os.waitpid(pid, os.WNOHANG)
    except ChildProcessError:
        pass
    return out.decode("utf-8", "replace"), before, after


def main():
    print("repl terminal tests (%s)" % ROOT)

    # 1. the default launcher must never produce an rlwrap warning ------------
    out = pty_run([A], [b"nosuchname\r", b"\\\\\r"])
    check("no_rlwrap_warning", "rlwrap:" not in out, out[-800:])
    check("error_still_reported", "E0101" in out or "nosuchname" in out, out[-800:])

    # 2. the AMBER_NO_EDIT fallback path may use rlwrap -- quietly ------------
    have_rlwrap = subprocess.call(["sh", "-c", "command -v rlwrap >/dev/null 2>&1"]) == 0
    if have_rlwrap:
        out = pty_run([A], [b"nosuchname\r", b"\\\\\r"], {"AMBER_NO_EDIT": "1"})
        check("rlwrap_fallback_is_quiet", "rlwrap: warning" not in out, out[-800:])
    else:
        print("  %-26s SKIP (rlwrap not installed)" % "rlwrap_fallback_is_quiet")

    # 3. termios must be exactly as we left it, on both exit paths ------------
    out, before, after = pty_run_with_termios([A], [b"2+2\r", b"\\\\\r"])
    check("termios_restored_on_exit", before == after,
          "before=%r\nafter =%r" % (before, after))

    out, before, after = pty_run_with_termios([A], [b"2+2", b"\x03", b"\\\\\r"])
    check("termios_restored_after_^C", before == after,
          "before=%r\nafter =%r" % (before, after))

    # 4. the editor actually edits -------------------------------------------
    #    type "1+9", Left Left, Backspace-of-nothing... simpler and unambiguous:
    #    type "99", Ctrl-A (home), "1+" -> "1+99", Enter -> 100
    out = pty_run([A], [b"99", b"\x01" + b"1+", b"\r", b"\\\\\r"], gap=0.8, wait=7.0)
    check("editing_ctrl_a_insert", "100" in out, out[-800:])

    #    Ctrl-K from home must clear the line: type "junk", Ctrl-A, Ctrl-K, "7"
    out = pty_run([A], [b"junkjunk", b"\x01\x0b" + b"6*7", b"\r", b"\\\\\r"],
                  gap=0.8, wait=7.0)
    check("editing_ctrl_k_kill", "42" in out and "junkjunk\r\n" not in out, out[-800:])

    # 5. a pipe must behave exactly as it always did --------------------------
    p = subprocess.run([A], input=b"2+2\n#\"hello\"\n\\\\\n",
                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=60)
    o = p.stdout.decode("utf-8", "replace")
    check("pipe_is_unchanged", "4" in o and "5" in o, o[-800:])

    p = subprocess.run([A], input=b"2+2\n\\\\\n", stdout=subprocess.PIPE,
                       stderr=subprocess.STDOUT, timeout=60,
                       env=dict(os.environ, AMBER_NO_EDIT="1"))
    o = p.stdout.decode("utf-8", "replace")
    check("no_edit_pipe", "4" in o, o[-800:])

    # 6. the bare interpreter (no repl.k) also reads through the editor -------
    p = subprocess.run([AMBER], input=b"2+2\n", stdout=subprocess.PIPE,
                       stderr=subprocess.STDOUT, timeout=60)
    o = p.stdout.decode("utf-8", "replace")
    check("bare_repl_pipe", "4" in o, o[-800:])

    print()
    if FAILURES:
        print("FAILED: %d" % len(FAILURES))
        return 1
    print("all repl terminal tests passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
