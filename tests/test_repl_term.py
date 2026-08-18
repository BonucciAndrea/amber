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
  2b. rlwrap_direct_*         the cases the launcher CANNOT speak for: someone
                              runs `rlwrap ./amber repl.k` (what the pre-1.9.5
                              docs taught), or keeps an `alias amber='rlwrap
                              amber'`. The engine itself must stand down, so
                              these assert no warning, a REPL that still works,
                              the one-time explanatory note, and -- via
                              AMBER_RLWRAP=0 -- that the detection is what is
                              doing the work rather than luck
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

    # 2b. the launch routes the launcher cannot control ----------------------
    #     This is the regression that mattered: fixing only ./a left every user
    #     with an old alias or an old habit still seeing the warning.
    if have_rlwrap:
        for label, argv in (("rlwrap_direct", [AMBER, REPLK]),      # old docs
                            ("rlwrap_launcher", [A])):              # old alias
            out = pty_run(["rlwrap"] + argv, [b"nosuchname\r", b"2+2\r", b"\\\\\r"])
            check(label + "_is_quiet", "rlwrap: warning" not in out, out[-900:])
            check(label + "_repl_works", "E0101" in out and "4" in out, out[-900:])

        out = pty_run(["rlwrap", AMBER, REPLK], [b"2+2\r", b"\\\\\r"])
        check("rlwrap_note_shown", "started under rlwrap" in out, out[-700:])

        # The control: with detection disabled the warning must come BACK. If it
        # does not, this whole suite would be passing for some unrelated reason.
        out = pty_run(["rlwrap", AMBER, REPLK], [b"nosuchname\r", b"\\\\\r"],
                      {"AMBER_RLWRAP": "0"})
        check("rlwrap_override_restores", "rlwrap: warning" in out, out[-900:])

        # And forcing detection on must silence it even where we did not detect.
        out = pty_run([A], [b"2+2\r", b"\\\\\r"], {"AMBER_RLWRAP": "1"})
        check("rlwrap_force_stand_down", "rlwrap: warning" not in out and "4" in out,
              out[-700:])
    else:
        for n in ("rlwrap_direct_is_quiet", "rlwrap_note_shown",
                  "rlwrap_override_restores"):
            print("  %-26s SKIP (rlwrap not installed)" % n)

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

    # 7. no $TERM: the REPL must still PRINT ---------------------------------
    #    repl.k's fmt calls upd[], which shells out to `tput -S` for the window
    #    size. With $TERM unset -- a CI step, a cron job, a docker RUN, a
    #    systemd unit -- tput writes nothing, and the unguarded
    #    "(lines;cols)::" that used to be there assigned a 0-element vector to a
    #    2-element target and raised E0103 "Vector length mismatch" INSTEAD OF
    #    THE RESULT. The value was computed correctly every time; only the
    #    display broke, so this reads as a wrong answer rather than a bad probe.
    #    That is how it was found: `100+`pr7 0` in tests/test_ext_seam.sh and
    #    `#`aio[-1]` in amber-ai's installer both reported a length error and
    #    were diagnosed as "the extension did not link".
    #    Removing TERM from the child's environment is the whole test.
    noterm = dict(os.environ)
    noterm.pop("TERM", None)
    p = subprocess.run([A], input=b"100+7\n2 3 4\n\\\\\n", stdout=subprocess.PIPE,
                       stderr=subprocess.STDOUT, timeout=60, env=noterm)
    o = p.stdout.decode("utf-8", "replace")
    check("no_term_still_prints", "107" in o and "E0103" not in o, o[-900:])

    #    ...and a $TERM that names a terminal terminfo has never heard of must
    #    behave the same way: tput exits non-zero and prints nothing useful.
    p = subprocess.run([A], input=b"100+7\n\\\\\n", stdout=subprocess.PIPE,
                       stderr=subprocess.STDOUT, timeout=60,
                       env=dict(os.environ, TERM="nosuchterm-does-not-exist"))
    o = p.stdout.decode("utf-8", "replace")
    check("bogus_term_still_prints", "107" in o and "E0103" not in o, o[-900:])

    print()
    if FAILURES:
        print("FAILED: %d" % len(FAILURES))
        return 1
    print("all repl terminal tests passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
