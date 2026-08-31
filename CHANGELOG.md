# Changelog

## Unreleased — REPL correctness: UTF-8, multi-line input, an honest exec timer

Everything here is in `main` and is **not yet tagged**. It is all REPL/editor work
plus one engine-level undefined-behaviour fix.

### The line editor now counts cells, not bytes

The line buffer holds **bytes**; a terminal lays out **cells**. Conflating the two broke
ordinary input:

- `città però` typed with its accents pushed the box's closing border two columns in
  (each accented letter is 2 bytes but 1 cell), leaving stale glyphs behind — the row
  rendered as `…però    │││`.
- CJK and emoji pushed it the other way (1 lead byte counted, 2 cells drawn).
- **Backspace deleted one BYTE of a multi-byte character**, leaving a stray continuation
  byte that rendered as a replacement glyph and was then handed to the parser. `Delete`
  and the arrow keys had the same defect.
- Horizontal scrolling and truncation could cut a UTF-8 sequence in half.

`src/ln.c` gained a small UTF-8 layer — sequence length, codepoint, display width and
character-boundary stepping — and every layout and editing path now works in characters
and cells. The width table is **built in rather than taken from `wcwidth()`** on purpose:
`wcwidth()` answers according to `LC_CTYPE`, so the same line would lay out differently on
macOS and on a WSL box with a different locale, and a box that lands in a different column
per platform is worse than one that is uniformly approximate.

`u8_len` validates continuation bytes, so a lead byte claiming four bytes as the last byte
before the terminator can no longer read past the array. Verified under ASan+UBSan against
truncated emoji, lone continuation bytes, overlongs, surrogates and a buffer-full truncated
lead byte.

The caret window is computed **backward from the caret**, not by scanning forward from the
start of the buffer: measuring cells forward is quadratic per keystroke and made a
multi-thousand-character line hang outright. A 5,000-character line now types in ~2 s.

### Multi-line continuation

Typing a function across lines used to be impossible: `f:{` evaluated on the spot, printed a
puzzling `0#,!0`, and the following `}` was a syntax error. Multi-line definitions only
worked through paste.

A line that leaves a bracket open (`{`, `(`, `[`) is now understood as an incomplete
statement: the editor echoes the fragment, shows a `...>` continuation prompt aligned under
the main one, and keeps reading until the brackets balance. The fragments are joined and
handed over as **one** statement, which is also what goes into history — `Ctrl-P` recalls
`f:{ x+1 }`, never a fragment.

```
amber> f:{
  ...> x+1
  ...> }
amber> f 41
42
```

There is deliberately **no key binding** for this. Shift-Enter is indistinguishable from
Enter in a terminal — both send CR — unless the user opts into an extended keyboard protocol
(xterm `modifyOtherKeys`, kitty, a hand-made iTerm2 binding), so binding it would work for
almost nobody and would behave differently on macOS and WSL. Continuing on incomplete syntax
is what python, node, ghci and q all do, and needs no terminal support at all.

Depth comes from the **same** string- and comment-aware counter the bracketed-paste path uses
to rejoin a pasted multi-line function, and the fragments are joined the same way, so a
function typed by hand and one pasted produce identical text. A bracket inside a string
(`"a{b"`) or after a comment (`1+1 / a { comment`) therefore does not trigger it. `Ctrl-C`
abandons a continuation.

### The status bar's exec timer was reporting 10x

`sbexec` computed `.1*_0.5+10*sblast`. **k has no bare `.1` float literal** — `.1` lexes as the
verb `.` applied to `1`, which yields `1` — so the expression evaluated to `1*_0.5+10*sblast`,
exactly ten times the milliseconds. A 23 ms line read `230 ms`.

Separately, `fmt` called `upd[]` before formatting **every** value, and `upd` forked
`/usr/bin/env tput -S` to read the terminal size — on every single line. On macOS that cost
3–6 ms per line, because `fork` copies page tables for the 1 GB reserved heap (measured 3.0 ms,
rising to 5.8 ms after `gentq 200000`), and the status bar charged all of it to the user's
expression. The size now comes from `ioctl(TIOCGWINSZ)` via `` `bi 0`` — the same probe
`src/ln.c` lays the bar out with, so the two can never disagree. `fmt`: **3.3 ms → 0.005 ms**.

| line | before | now | `\t` (eval only) |
|---|---:|---:|---:|
| `1+1` | 62 ms | **0.053 ms** | – |
| `gentq 10000` | 230 ms | **9.735 ms** | 8–9 ms |
| `gentq 100000` | – | **82.796 ms** | 81 ms |

Resolution is now 3 decimals, built from integer thousandths so no IEEE artefact reaches the
display (`0.1*818` is `81.80000000000001`, and `$` prints every digit), zero-padded so 9.008 ms
does not render as `9.8`.

### Terminal robustness

- **Resize / zoom.** A resize now releases the scroll region, wipes the whole screen, re-sets the
  region and repaints the transcript from the scroll-back ring. It assumes nothing about what the
  terminal did to the old contents, because terminals disagree — xterm clips the alternate screen
  at the bottom, Terminal.app and iTerm2 keep the bottom and shift content up, and they differ
  again on whether DECSTBM survives. Any repaint that erased "where the old footer was" left a
  stranded copy of the box on some terminal: the duplicated box seen when zooming on macOS. The
  transcript now survives a resize and reflows to the new width.
- **A resize that lands while the interpreter is busy** was never seen by the editor's read loop,
  so the previous footer stayed on screen next to the newly drawn one. Handled at the prompt.
- **Wide terminals.** Both padding paths ran through fixed 256/300-byte buffers and silently
  clamped past ~313 columns, leaving the input row's closing border short of the edge while the
  borders above and below still reached it. A Retina Mac at a zoomed-out font is routinely
  300–400 columns.
- **`Ctrl-L` is now byte-identical to `\clear`** (it was missing the `ESC[3J` scrollback wipe).
  macOS `Cmd-K` cannot be intercepted — Terminal.app and iTerm2 clear the screen locally and send
  the program nothing — so rebinding `Cmd-K` to send `Ctrl-L` (iTerm2: Settings ▸ Keys ▸ Send Hex
  Code `0x0c`) is the only way to get exact `\clear` behaviour, and that now holds byte for byte.
- **The committed command echo erases to end of line.** A row repainted from the ring is not
  blank, so the tail of a longer previous line survived past the echoed command.
- **Caret stability.** Repainting the footer on every keystroke (tried, for `Cmd-K` recovery) made
  the caret visibly flick to the rows above and below the input line on every character typed. It
  is reverted; recovery happens once per prompt instead.
- `repl.k`'s `lines` now excludes the status bar's four footer rows, so `fmt` stops laying
  dictionaries and lists out taller than the visible region.
- Every allocation in the bracketed-paste path is checked. A paste is the one input whose size the
  user chooses, so allocation failure is reachable, and the old unchecked `realloc`/`malloc` would
  have dereferenced NULL.

### Engine

- **`az()` signed overflow (undefined behaviour).** It decided whether a 64-bit value fits in an
  `I` by computing `n-(I)n`. For `n = LLONG_MAX` (k's `0W`) the truncation is `-1`, so it computed
  `LLONG_MAX-(-1)` — UBSan flagged it on any qSQL path producing `0W`. It now round-trips the value
  instead: equivalent in range, defined out of it.

### Portability

- `.gitignore` covers `*.dylib` and `.DS_Store`, so a macOS build no longer leaves the tree dirty.

All of the above is covered by `tests/test_statusbar.py`, which now also drives resize/zoom bursts,
UTF-8 input, long-line responsiveness, `Cmd-K` recovery, caret stability (asserted on the emitted
bytes — a rendered grid cannot see a cursor that returned before the next flush) and multi-line
continuation.

## 2.0.0 — infix dyads, bare qSQL in scripts, two lexer/verb fixes

This release is about **ergonomics** — closing three long-standing gaps between the
syntax you can type and the syntax the docs told you to type — plus fixing two real
engine defects that had been tracked in `docs/MISSING.md`.

### Infix notation for the two-argument library dyads

The join family and the set/search dyads can now be written **infix**, exactly as
in kdb+/q, instead of only in bracket form:

```q
2 3 9 in 2 3 4          / 1 1 0        (was: in[2 3 9;2 3 4])
5 within 3 9            / 1b           (was: within[5;3 9])
t lj kt                 / left join    (was: lj[t;kt])
`sym xasc t             / sort         (was: xasc[`sym;t])
"/" sv `a`b`c           / join         (was: sv["/";`a`b`c])
1 2 3 except 2          / 1 3          (was: except[1 2 3;2])
```

The infix set is a curated list of Amber's two-argument dyads — `in within like lj
ij uj aj aj0 wj wj1 pj ej cross inter union except ss sv vs xasc xdesc`. The parser
change extends the same mechanism ngn/k already applies to every *unicode*-named
identifier (which were always infix) to these ASCII names. **The bracket form still
works unchanged**, `f[x;y]` and `f x` too, and a name is left an ordinary lvalue
when it is being defined or amended (`in:{…}` still assigns), so nothing that
worked before breaks. Lambda literals are deliberately **not** made infix: `f
{lambda} x` must keep meaning `f({lambda}[x])`, and a purely syntactic parser cannot
tell that apart from `noun {lambda} noun` the way q's type-aware one can. Verified
against the reference: `x in y`, `x within y`, `t lj kt` and the rest produce output
identical to kdb+/q and to the bracket form (`test.k`, 12 cases).

### Bare qSQL now works in a loaded `.k` file — no `sel"…"` wrapper

`select … by … from … where …` (and `exec` / `update` / `delete`) previously worked
**bare** only on the interactive input line; inside a script you had to wrap it in
`sel"…"`. The C loader (`bsl`, `src/m.c`) now runs every file it loads through the
same qSQL rewriter the REPL uses (`qrwf` / `qtry`, `qsql.k`), so a `.k` file loaded
once the standard library is up gets the identical bare-qSQL treatment:

```q
/ in a .k file, after the stdlib is loaded:
select sum px by sym from trades where px>100      / just works -- no sel"..."
```

The rewrite is line-by-line (matching the REPL), guarded by an `` `ERR`` sentinel so
a rewrite failure can never corrupt a script, and gated so nothing changes until
`qsql.k` defines the rewriter (the bare interpreter and the bootstrap are
untouched). A non-qSQL script is passed through byte-for-byte. (`test_qsql.k` +
`tests/test_qsql_script.sh`.)

### Fixes (both were tracked in `docs/MISSING.md §14`)

- **`&()` (where on a literal empty generic list) returned `,!0` instead of `!0`.**
  `whr`'s generic-list branch (`src/v.c`) ran its nested-grouping K expression on a
  simply-empty `()` — which also has type `` `A`` — instead of recognising "zero
  elements, nothing to do". It now short-circuits an empty generic list to an empty
  int vector, byte-identical to `&!0`. This was silently affecting `ss` and any qSQL
  path that built a literal empty `()`. (`test.k`, 5 cases.)
- **A bare `/` line opening an unterminated block comment silently truncated the
  file.** A `/`-on-its-own-line block comment with no closing `\` line ran to EOF and
  exited 0 with no diagnostic — quietly dropping the rest of the file. It now raises
  a clean parse error. Properly-closed `/ … \` blocks and trailing `/ …` line
  comments are unchanged. (`tests/test_comments.sh`.)

### REPL — bracketed paste and an optional status bar

- **Bracketed paste.** The native line editor (`src/ln.c`) now enables bracketed
  paste (`ESC[?2004h`). Pasting a multi-line script runs it **line by line, exactly
  as if each line were typed** — so inline (`x:1 / c`) and full-line (`/ c`)
  comments work, and bare qSQL in a pasted line is rewritten just as at the prompt.
  The first pasted line submits immediately; the rest are queued and drained under
  the prompt. (Whole-block `. text` eval was rejected — it raises `'limit` on a
  multi-statement string.) Paste mode is torn down on every exit path.
  (`tests/test_paste.py`, pty-driven.)
- **Optional Claude-Code-style status bar — `\sb`.** Off by default (the default
  REPL is byte-for-byte unchanged and every terminal test still passes). `\sb`
  sets a DECSTBM scroll region and paints a **two-line bottom panel**: a muted bar
  with a coral-accented `✻ amber 2.0.0` brand and key hints, and a live info line
  under it (the last command's wall-time, then a digest of your workspace tables).
  `src/ln.c` OWNS the rendering — a new `am_ln_statusbar()` fed by repl.k through
  the `` `sbb`` verb — so the panel survives **Ctrl-L, `\clear`, a resize and every
  keystroke**, and the atexit hook releases the scroll region on **every** exit
  path (`\\`, Ctrl-D, a crash). Standard Unicode only (`✻ ↑ ↓ ·`) — no Nerd Font
  needed; truecolour palette degrades to the nearest 256-colour cell.
  (`tests/test_statusbar.py`, pty-driven.)

### Also

- Grouping a table **by a symbol column** is now ~19&times; faster: `group`
  (`src/o.c`) groups symbols by their interned 4-byte id instead of lexically
  string-sorting them. Same partition, byte-identical first-appearance key order;
  `select … by sym` on 1M rows went 587&nbsp;ms → 34&nbsp;ms — Amber's own before/after
  (`docs/BENCHMARKS.md`, and the benchmarks page on the site). (`test.k`, 6 cases.)
- Right-to-left evaluation of a function's bracketed arguments (already the case,
  and matching kdb+/q) is now pinned by side-effect-based regression tests, together
  with nested/chained-bracket results, so a parser change can't silently flip them
  (`test.k`, 7 cases).

## 1.9.6 — `libamber.so`: the out-of-process seam

### What this release is

1.9.5 gave the engine a seam for extending it **in process**: drop a `.c` file into
`ext/`, rebuild, and it plugs itself in through `src/ext.h` without a line of `src/`
being patched. That covers everything written in C. It covers nothing written in
Python, Go, TypeScript or anything else — and a columnar engine that cannot be
reached from a notebook, a dashboard or a gRPC service is an engine nobody outside
this repository can use.

So this release adds the matching **out-of-process** seam, and adds it in the
smallest shape that could work: one build flag, one export map, one new section of
an existing header, and one test file.

```
./build.sh                 ->  ./amber          (byte-for-byte unchanged)
./build.sh --shared        ->  ./amber + libamber.so
./build.sh --shared-only   ->  libamber.so
```

### The C API (`src/ext.h` §6, `src/ext.c`)

About sixty entry points, all named `amber_*`, all taking and returning C99
builtins or an opaque 64-bit handle. Nothing in the header — or in the core build —
mentions Python.h, an Arrow header, gRPC or any other consumer's world.

- **lifecycle** `amber_init` (optionally loading the `.k` standard library),
  `amber_shutdown`
- **evaluation** `amber_eval_str`, `amber_eval_qsql` (bare `select … from …`
  accepted verbatim, via `qsql.k`'s own rewriter), `amber_call` (native arguments,
  no string round trip), `amber_last_error`, `amber_set_diagnostics`
- **ownership** `amber_retain` / `amber_release`, with the rule stated once: a
  function that *returns* a value returns one you own, a function that *takes* one
  borrows it
- **the zero-copy seam** `amber_get_vector_ptr` hands back the engine's own column
  payload, with its type, length and element width **in bits** (bool vectors are
  genuinely bit-packed)
- **tables and dictionaries**, **constructors** for pushing data back in, the
  **Arrow C Data Interface** (including a caller-allocates form that leaves nothing
  to free), **rendering** (with an ANSI-stripped variant), and `amber_plugin_load`

### Three decisions worth recording

**The executable did not change.** The shared library is a separate object set
(`-fPIC -Dshared`), not a relink of `o/*.o`. Position-independent code and the TLS
model below both change code generation, and sharing objects would have silently
pessimised `./amber` — the one thing this repository will not trade away.

**Only `amber_*` and `am_ext_*` are exported** (`src/libamber.map`). The engine's
internals are called `mr`, `su`, `us`, `err`, `run`, `add`, `sub`, `pk`, `cpl`.
Those are perfect inside one static binary and a collision waiting to happen inside
a library loaded next to NumPy, libarrow and libpython. Everything else is now
genuinely absent from the dynamic symbol table.

**Thread-local storage drops to global-dynamic in the shared build only.** This is
a correctness fix, not a tuning choice. Initial-exec TLS is resolved out of the
static block the loader sizes before `main()`; a dlopen'd library borrows from a
small surplus reserve and fails *nondeterministically* — `cannot allocate memory in
static TLS block` — depending on what else the host process imported first.
`./amber` keeps initial-exec and pays nothing.

### Also

- `amber_arrow_export_into` / a move-based `amber_arrow_import`, so a binding can
  allocate the two Arrow structs however it likes instead of being forced to
  `malloc` them because that is what this library happens to `free`.
- Dotted global names (`arrow.export`, `u.pub`) are reachable through
  `amber_call` / `amber_get_global`. The global table keys a namespaced global on
  the pair (namespace, name), so interning `"arrow.export"` as one symbol matched
  nothing — and failed with a bare `'value` and no hint as to why.
- `libamber.so` records a `SONAME`, so a process that loads it twice gets **one**
  engine rather than two silently independent ones.

### Verification

`tests/test_capi.c` — 78 assertions, linked against the shared library exactly as a
satellite would link it: it includes `src/ext.h` and nothing else from `src/`, and
never dereferences an `amber_value`. If it ever needs a second `-I`, the API is
wrong.

`tests/test_capi.sh` runs it twice: once at `-O2`, once under
`-fsanitize=address,undefined` with `detect_leaks=1`. Both legs are wired into
`tests/run_tests.sh` and into its `--asan` pass. The full suite — 163 + 35 + 79
legacy assertions, the 309-case matrix, the 94-case qSQL matrix, the sort/window
suite, the peach verifier at 1 and 4 lanes, the C unit tests, the pty REPL suite,
the extension-seam round trip, and ~1,000 fuzz cases — passes unchanged, and passes
again under ASan + UBSan.

### The ecosystem this exists for

Six satellite projects, none of them mentioned anywhere in `src/`:
`python-amber` (zero-copy NumPy / pandas / Arrow), `amber-arrow` (a streaming
`ArrowArrayStream`, the `amberd` query server, and an Arrow Flight daemon),
`amber-jupyter` (a kernel where Amber and Python cells share one process and one
heap), `vscode-amber` (highlighting and a standalone LSP), and
`grafana-amber-datasource` and `amber-flame` (live dashboards, and a profiler).

## 1.9.5 — the REPL owns the terminal: native line editing, and the end of `rlwrap`

### The bug

Amber's REPL had no line editing. The documented workaround, since 1.7, was to start it under
`rlwrap`, which gives any line-oriented program readline history and arrow keys by running it on
a pty. That works only for a program that reads **whole lines in canonical mode**. As soon as a
program takes the terminal into raw / non-canonical mode and reads single keypresses, rlwrap is
doing nothing for it — and says so, at an unpredictable moment, across both `stdout` and
`stderr`, most often immediately after an error report:

```text
rlwrap: warning: rlwrap appears to do nothing for amber, which asks for
single keypresses all the time. Don't you need --always-readline
and possibly --no-children? (cf. the rlwrap manpage)

warnings can be silenced by the --no-warnings (-n) option
```

Two things go wrong at once. The warning itself is interleaved into program output — it lands in
the middle of a diagnostic, or between a query and its result, and it is not filterable because
it arrives on both streams. And underneath it, two editors are driving one cursor: rlwrap's
readline believes it owns the line, Amber's redraw believes the same, and the display garbles —
characters echoed twice, the prompt repainted at the wrong column, `Ctrl-A` doing nothing.

### The fix: own the terminal, and stop wrapping

The root cause is not rlwrap's message; it is that terminal handling was split across two
processes that could not agree. 1.9.5 removes the split by giving the REPL its own editor and
taking the wrapper out of the picture entirely.

**`src/ln.c` / `src/ln.h` — a native single-file line editor.** ~700 lines of C99 + POSIX in the
linenoise tradition: raw `termios`, one visible line with horizontal scrolling, ANSI refresh.
No readline, no curses, no terminfo, no third-party code, nothing allocated on the keystroke path
beyond the line buffer. It provides character/word movement, `Ctrl-A/E/B/F/K/U/W/L`, `Home`/`End`,
`Del`, arrow-key and `Ctrl-P/N` history persisted to `~/.amber_history`, and Tab completion over
the workspace's globals, table column names, `\` commands, the Amber/K vocabulary and whole lines
already entered this session.

**`src/lnk.c` — the `` `rdl`` verb**, the editor's only interpreter-facing surface. `repl.k` now
reads its input through `` `rdl`` instead of a raw `read(2)` on fd 1, and `src/m.c`'s bare `rep()`
loop (`./amber` with no script) does the same.

**`./a` no longer invokes `rlwrap`.** The default path execs the interpreter directly, so the
warning cannot be produced. The single remaining path that touches rlwrap is the deliberate
`AMBER_NO_EDIT=1` fallback — a dumb terminal, an editor subshell, a screen reader — where Amber
reads whole lines in canonical mode and rlwrap is genuinely the right tool. Even there it is
invoked as `rlwrap -n -a`:

* `-n` / `--no-warnings` — no rlwrap diagnostic can ever be printed, whatever it concludes;
* `-a` / `--always-readline` — readline stays on when rlwrap cannot detect a prompt.

`AMBER_NO_RLWRAP=1` opts out of even that.

**And the engine stands down on its own, however it was started.** Fixing the launcher only ever
helps the one launch route the launcher controls, and that is not how people start Amber. A shell
alias left over from 1.9.4 (`alias amber='rlwrap amber'`), a wrapper script, a tmux or `.desktop`
entry, or simply the `rlwrap ./amber repl.k` line the old `docs/AMBER.md` recommended — every one
of those still produced the warning on the first error, because the binary had no defence of its
own. It does now:

`am_ln_under_rlwrap()` (src/ln.c) detects a parent `rlwrap` or `rlfe` by **process lineage** —
rlwrap exports no environment variable to the program it wraps, so lineage is the only reliable
signal — walking up to four ancestors so an intervening wrapper script that does not `exec()`
cannot hide it. Linux reads `/proc`, macOS uses `sysctl(KERN_PROC)`, anything else falls back to
a single `ps`. The walk runs at most **once** per process and only after stdin is already known to
be a terminal, so a pipe, a here-doc or a CI job never pays for it; a platform whose lineage
cannot be read answers "no wrapper" and behaves exactly as before.

When it fires, `am_ln_interactive()` returns 0 and Amber reads whole lines in canonical mode,
exactly as 1.9.4 did. **Standing down is the correct answer, not a concession:** rlwrap's warning
is legitimate — two line editors cannot share one cursor — and the user under rlwrap still gets
full readline editing and history. Nothing is lost, the warning cannot be printed, and the redraw
cannot garble. A single explanatory line goes to stderr once per session, so a user who wanted
rlwrap learns that Amber has an editor of its own:

```text
amber: started under rlwrap -- leaving line editing to it.
       amber has had its own editor since 1.9.5: start it with ./a (no rlwrap) to use that instead.
```

Two overrides, for the cases judgement should not be automatic: `AMBER_RLWRAP=1` forces the
stand-down for a wrapper Amber did not recognise, and `AMBER_RLWRAP=0` keeps Amber's own editor
under rlwrap regardless (the pre-fix behaviour, warning included).

**On no path — launcher, alias, wrapper, or bare `rlwrap ./amber repl.k` — can an rlwrap message
now reach a session.** The build flags make no difference to any of this: a portable build and an
`AMBER_NATIVE=1 ./build.sh` build behave identically, and `tests/test_repl_term.py` asserts it on
both.

### Terminal state is restored on every exit path

Raw mode is entered per line and left on every way out: normal return, EOF, `Ctrl-C`, `Ctrl-D`,
and an `atexit()` handler for anything else — so a crash or a signal cannot leave the user's
shell without an echo. `tests/test_repl_term.py` asserts this by comparing the pty's `termios`
byte-for-byte before and after a run, on both the clean-exit and the `Ctrl-C` path.

### Batch behaviour is unchanged, by construction

When stdin/stdout are not a terminal, or `TERM` is dumb, or `AMBER_NO_EDIT` is set,
`am_repl_getline()` degrades to a plain one-byte-at-a-time line read with the prompt still
printed, consuming nothing past the newline. `echo '2+2' | ./a`, here-docs, `./amber script.k` and
CI runs are byte-for-byte what they were in 1.9.4.

### `tests/test_repl_term.py` — seventeen pty-driven regression tests

Nothing here is mocked; every case runs the real interpreter on a real pty:

| test | asserts |
|---|---|
| `no_rlwrap_warning` | no `rlwrap:` output ever reaches a `./a` session |
| `error_still_reported` | the diagnostic that used to be interleaved still prints |
| `rlwrap_fallback_is_quiet` | `AMBER_NO_EDIT=1 ./a`, which *does* use rlwrap, is silent |
| `rlwrap_direct_is_quiet` | `rlwrap ./amber repl.k` — what the old docs taught — is silent |
| `rlwrap_launcher_is_quiet` | `rlwrap ./a` — an old shell alias — is silent |
| `rlwrap_*_repl_works` | and both still evaluate: the error prints, `2+2` is `4` |
| `rlwrap_note_shown` | the one-time explanation appears |
| `rlwrap_override_restores` | `AMBER_RLWRAP=0` brings the warning back, proving the detection is what silences it rather than luck |
| `rlwrap_force_stand_down` | `AMBER_RLWRAP=1` silences a wrapper we did not detect |
| `termios_restored_on_exit` | `termios` identical before/after a normal exit |
| `termios_restored_after_^C` | `termios` identical before/after `Ctrl-C` |
| `editing_ctrl_a_insert` | `Ctrl-A` + insert really edits (`99` → `1+99` → `100`) |
| `editing_ctrl_k_kill` | `Ctrl-A` `Ctrl-K` really clears the line |
| `pipe_is_unchanged` | piped input still evaluates and prints |
| `no_edit_pipe` | `AMBER_NO_EDIT=1` with a pipe behaves identically |
| `bare_repl_pipe` | `./amber` with no script reads through the editor too |

### Cross-platform build: the macOS/`-std=c99` failures

Three separate things broke a macOS build (and, in CI, an arm64 runner) while a Linux build stayed
perfectly green. All three are now fixed and guarded by the CI matrix.

**1. `MAP_ANON` vanished on Darwin.** `src/m.c`, `src/a.c`, `src/arena.c` and `src/trace.c` each
define `_POSIX_C_SOURCE` so that a strict `-std=c99` build still sees `PTHREAD_MUTEX_RECURSIVE`
and friends. On glibc that is harmless — glibc keeps the BSD extensions visible anyway. On Darwin
it is not: defining `_POSIX_C_SOURCE` switches the headers into **strict POSIX mode**, which
*hides* every BSD extension, `MAP_ANON` among them. The result was source that compiles clean on
Linux and fails on Apple clang with `MAP_ANON undeclared here`. Every one of those four
translation units now opens with

```c
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif
#ifndef _DARWIN_C_SOURCE
#define _DARWIN_C_SOURCE
#endif
```

placed above the **first** `#include` of the file — not merely above `<sys/mman.h>`, because any
system header may pull in `<features.h>` first and latch the mode for the whole compilation. All
three macros are purely additive: they only ever *unhide* declarations. (Checked rather than
assumed: this tree calls no `strerror_r`, `basename` or `qsort_r`, the three functions whose
semantics `_GNU_SOURCE` actually changes.)

`src/m.c` additionally normalises the spelling, since which of the two a platform exposes depends
on those same macros:

```c
#ifndef MAP_ANON
#  ifdef MAP_ANONYMOUS
#    define MAP_ANON MAP_ANONYMOUS
#  else
#    define MAP_ANON 0x20
#  endif
#endif
```

**2. `AMBER_NATIVE=1` could not build on Apple Silicon.** `-march=native` is x86 syntax; Apple
clang on arm64 rejects it outright. `build.sh` now *probes* the tuning flag exactly as it already
probed `-flto` and `-fopenmp`: it tries `-march=native`, then `-mcpu=native` (aarch64 GCC and
clang), and if neither is accepted it builds portable and says so in the banner. The documented
`AMBER_NATIVE=1 ./build.sh` line therefore succeeds everywhere instead of failing on every arm64
runner.

**3. `readlink -f` does not exist on macOS.** It is GNU coreutils; BSD readlink gained `-f` only
in macOS 12.3 (2022). Every script in the tree used it to find its own directory, so on any older
Mac `./a`, `./build.sh`, `./install.sh`, `demo.sh` and both test runners resolved to an empty path
and `cd`'d somewhere arbitrary. All six now use an `am_scriptdir` helper built on POSIX `readlink`
(no `-f`) plus `cd -P`, which behaves identically on macOS, Linux, WSL2 and BusyBox and still
follows a chain of symlinks. A CI job greps for any reintroduction.

### CI

`.github/workflows/ci.yml` is rebuilt around the platforms that actually broke:

| job | what it covers |
|---|---|
| `toolchains` | GCC 11/12/13 + Clang on Linux; builds, then compiles **every TU under strict `-std=c99`** and links it — the mode that exposes the feature-macro class of bug |
| `unix` | the real matrix: `ubuntu-latest` **and** `macos-latest`, each with **both** a GCC and a Clang. On macOS `gcc` is a Clang shim, so that leg installs a genuine Homebrew GCC and resolves its versioned name at run time rather than pinning `gcc-13` and breaking when Homebrew moves on. Builds with `AMBER_NATIVE=1`, runs the full suite, and runs the pty terminal suite separately |
| `sanitizers` | the whole suite under ASan + UBSan |
| `windows-wsl` | unchanged: Ubuntu userland inside WSL |
| `scripts` | shellcheck, a grep that fails the build if `readlink -f` reappears, and a check that the **executable bit is committed** — see below |

`rlwrap` is now installed on the Linux and macOS legs. It is a *test* dependency: without it the
rlwrap regression cases in `tests/test_repl_term.py` silently SKIP, so the regression they guard
would have gone unnoticed. `actions/checkout` is `@v4` and `actions/setup-python` is `@v5` with a
pinned 3.12, so a runner-image bump cannot quietly change what the suite runs on.

### `./install.sh`

Rewritten as a real installer rather than a build-and-alias script:

* **Toolchain detection** — no compiler produces the exact package command for *this* system
  (`apt-get` / `dnf` / `yum` / `pacman` / `apk` / `xcode-select --install`) instead of a compiler
  backtrace. It also states that `make` is *not* needed, so nobody goes looking for it.
* **Shell detection** — writes to the rc file the user's **login** shell reads, not the one the
  script happens to run under. `bash install.sh` from a zsh session used to append to `~/.bashrc`,
  which zsh never sources, so the alias silently never appeared. macOS bash gets
  `~/.bash_profile` (Terminal starts login shells); zsh gets `$ZDOTDIR/.zshrc`; fish is detected
  and told what to do by hand rather than handed bash syntax.
* **Idempotent** — the block is delimited by `# >>> amber >>>` markers and *replaced* on re-run,
  not appended.
* **Permissions** — repairs the executable bit on every script in the tree first.

### The executable bit

Every script in the published repository was committed as mode `100644`, so a fresh
`git clone` followed by the README's very first command answered `bash: ./install.sh: Permission
denied`. The archive ships them `755`, `install.sh` repairs them, and the `scripts` CI job now
fails the build if the index regresses. When committing, the fix is
`git update-index --chmod=+x a build.sh install.sh demo.sh tests/*.sh tests/*.py`.

### Portability / correctness fixes carried in the same release

* **`src/m.c` compiles under strict `-std=c99` again.** `_DEFAULT_SOURCE` and `_POSIX_C_SOURCE`
  sat *below* `#include <stdio.h>`, which silently defeated both: `MAP_ANON` (used by `mm()`) and
  `PTHREAD_MUTEX_RECURSIVE` (glibc guards it behind `__USE_UNIX98`/`__USE_XOPEN2K8`) were hidden,
  so a strict C99 build did not compile at all. Both macros now precede the first system header of
  the translation unit. Neither changes a line of evaluation or memory logic.
* **`build.sh` passes `-Isrc`** and prunes object files whose source has gone, so a removed file
  can no longer be linked in forever from a stale `o/*.o`.

### New: the extension seam (`src/ext.h`, `src/ext.c`, `ext/`)

1.9.5 adds one small, neutral seam — no AI, no network, no feature flags — so that an
out-of-tree package can add verbs, `\`-commands and editor behaviour **without patching a single
line of `src/`**. `build.sh` compiles `ext/*.c` (empty in a stock checkout) with the same flags
into the same binary; a package registers itself from a constructor:

| hook | effect |
|---|---|
| `am_ext_verb("xyz", fn)` | register a backtick verb at runtime; `sym1()` consults the registry before its built-in table |
| `am_ext_bs` | claim a `\`-command ahead of the "unknown `\cmd` runs a shell command" fallback |
| `am_ext_hint` | supply inline ghost text in the editor — never inserted until the user accepts it |
| `am_ext_complete` | add Tab candidates before the built-in lexical sources |
| `am_ext_startup` | run once, lazily, when the REPL first reads a line |
| `am_ext_usage` / `am_ext_banner` | append to `--help` / the banner |

On the Amber side, `repl.k` loads `lib/ext.k` whole-file at startup if it exists — trapped, with
diagnostics off — and offers the optional `ext.pre` / `ext.post` / `ext.err` / `ext.raw` /
`ext.tag` hooks. `tests/ext_probe.c` exercises every hook and `tests/test_ext_seam.sh` installs
it, verifies it, and uninstalls it again, asserting the engine is byte-for-byte stock afterwards.

`AMBER_EXT_ABI` (currently `1`) is bumped only when an existing hook changes shape; an installer
should check it before copying files in.

### What is deliberately *not* here

This repository is the engine. It contains **no AI code, no HTTP client, no model backend and no
outbound network calls of any kind**. The optional
[`amber-ai`](https://github.com/bonucciandrea/amber-ai) package — local, offline,
schema-aware completions and `\ai` diagnostics — is a separate repository that installs into an
existing Amber through the seam above, and is not required by, referenced by, or linked into
anything here.

### Upgrading from 1.9.4

Nothing to do beyond rebuilding.

* If you have `alias amber='rlwrap amber'`, a wrapper script, or `rlwrap` in a `.desktop` /
  systemd / tmux launcher — **remove the `rlwrap`**. Amber no longer needs it. You no longer
  *have* to remove it (the engine detects it and stands down quietly), but you will keep getting
  rlwrap's editing rather than Amber's, and Amber says so once per session.
* `~/.amber_history` is created on first interactive use; delete it any time.
* `AMBER_NO_EDIT=1` restores pre-1.9.5 read behaviour if you need it.

---

## 1.9.4 — one error, one report: Rust-style diagnostics across every category

### Errors print once
An error used to be reported **twice**: `eS()` rendered the Rust-style report to stderr at error
*creation* time, and then the legacy ngn/k caret block accumulated in `b[]` was dumped to stderr
again on the way out (`epr()` for a script, `onerr` in `repl.k` for the REPL). Every failing
script showed the same failure in two different formats, back to back.

`b[]` is deliberately **unchanged** — it is what `.[f;args;handler]` hands a trap handler and what
`` `err`` returns, a documented and test-covered contract. What changed is that a new flag
(`amdiagshown`, set by the renderer, cleared when an error starts or is consumed) tells `epr()` to
stay quiet when a rich report has already been shown, and `repl.k`'s `onerr` to skip its
descriptive echo. With diagnostics off (`AMBER_DIAG=0` / `` `diag 0``) the compact form is printed
exactly as before, so nothing is ever silently swallowed.

### Every category has a code, a title and actionable help
`E0001: evaluation error` and `= help: the caret marks the offending token` are gone. `src/e.c`
now carries a catalogue keyed by the category name that `err0()` already writes into the error
buffer, so the lookup needed no new plumbing through the interpreter:

| Code | Category | Title | Raised when |
|---|---|---|---|
| `E0101` | `value` | Undefined variable or value | A name is referenced that is not bound in any enclosing scope |
| `E0102` | `type` | Type mismatch or invalid operand type | A primitive was handed an operand type it has no case for |
| `E0103` | `length` | Vector length mismatch | Conforming operands have different counts |
| `E0104` | `domain` | Out of domain operation | The value is outside the operation's mathematical domain |
| `E0105` | `parse` | Syntax or parse error | Unbalanced brackets, unterminated string, malformed expression |
| `E0106` | `index` | Index out of bounds | An index is negative or past the end |
| `E0107` | `rank` | Function rank mismatch | Wrong number of arguments for the function's rank |
| `E0108` | `limit` | Resource or allocation limit exceeded | Vector size, recursion depth or memory limit hit |
| `E0109` | `io` | Input/output failure | File or socket open/read/write failed |
| `E0110` | `stack` | Call stack depth exceeded | Recursion deeper than the interpreter stack allows |
| `E0111` | `compile` | Expression could not be compiled | Parses, but cannot be compiled |
| `E0112` | `nyi` | Operation not implemented for these types | Primitive has no implementation for these operands yet |

### The parser reports like everything else
`src/p.c` called `eQ()` directly, so a syntax error was the one category that still printed only
the legacy block. The renderer is now factored into `eD()` (raw source bytes in, report out) and
the parser calls it, so `E0105` looks like every other category.

### Underlines span tokens, and name them
A bare offset is widened to the whole token before rendering (`etok`): identifiers expand over
their full name, string literals over the whole literal, operators stay one character. So an
undefined `prices` is underlined `^^^^^^`, not `^`. The token is then named — promoted into the
title for undefined names (``Undefined variable `prices` ``), carried in the inline label
otherwise.

### Visual polish
* Gutter rows all place `|` in the same column, matching `rustc` exactly; line numbers are
  right-aligned and dimmed so they read as navigation, not content.
* Inline labels sit at the end of the underline row, so the explanation is on the same visual line
  as the thing it explains.
* Secondary spans underline with `~` in blue; the primary always wins where they overlap, so the
  fault site can never be masked by context.
* Palette moved to `src/ansi.h` and switched to the bright (9x/6x) variants, which stay legible on
  light terminals where plain `3x` red does not.
* SGR codes are emitted once per **run** rather than once per glyph — a six-character underline
  was previously six colour+reset pairs, which bloated captured output and made logs unreadable.
* `report_diagnostic_ex()` adds inline labels and a `= note:` line; `report_diagnostic()` is kept
  as a wrapper so existing callers are untouched.

### `` `dgn`` self-test extended
Now checks the full layout (code, title, locator, `^^^` primary vs `~~~` secondary, inline label,
help and note), that colour-off output contains **no** ANSI bytes at all, that every gutter bar
lands in the same column, and the complete category → code matrix — plus that an unknown category
reports absence rather than crashing.

### Validation
601 tests + `examples/peach_verify.k` (60) pass, C unit tests pass, fuzzer clean (1,629 cases),
and everything is clean under AddressSanitizer + UndefinedBehaviorSanitizer with leak detection,
including every diagnostic path.


## 1.9.3 — binary serializer (`-8!`/`-9!`), binary IPC for `peach`, three `peach` bugs fixed

### New: compact binary serialization
`-8!x` encodes any K value into a contiguous byte vector (`tC`); `-9!y` decodes it back.
`(-9! -8! x) ~ x` holds for every supported shape — verified over 60 cases in the new
`examples/peach_verify.k`.

Amber's only wire format was previously TEXT: `` `k `` rendered a value and `` . `` reparsed it.
That could not represent everything (attributes were dropped, and nested empties and some
null/infinity edge cases do not reparse to themselves), and it cost a full format-then-parse
round trip on every transfer.

Format: a 4-byte `"AMB"`+version header, then one recursive node per value —

| shape | encoding |
|---|---|
| packed atoms (`ti` `tc` `tu` `tv` `tw` `tx` `tdt` `ttm`) | tag + `i32` value |
| symbol atom (`ts`) | tag + `u32` length + name bytes |
| ref-carrying (`tA` `tM` `tm`) | tag + attr + `u64` count + recursive children |
| symbol vector (`tS`) | tag + attr + count + per-element length-prefixed names |
| all other heap types | tag + attr + count + raw payload bytes |

Because the last case copies the payload verbatim and recovers the element width from the tag via
`Tw[t]`, bit vectors (`tB`, one bit per element) and the narrower date/time widths need no special
case, and **nulls and infinities survive exactly** — they are just their bit patterns (`0N` is
`1<<63`, `0w` is the f64 infinity), never routed through a decimal formatter that could round.

Two details that are easy to get wrong and are covered by tests:

- **Attributes travel.** `_at(x)` (0=none, 1=`` `s``-sorted) rides in the attr byte of every heap
  node, so a sorted column arrives still flagged sorted and keeps the O(log n) binary-search path
  in `fnd()` on the far side instead of silently degrading to a scan.
- **Symbols are shipped by NAME, not by id.** Symbol ids are process-local: a forked `peach`
  worker can intern a symbol the parent has never seen, so raw 32-bit ids would decode to the
  wrong name (or garbage) in the parent. Every symbol is written as its name and re-interned with
  `us()` on the way in.
- **Empty general lists keep their type witness.** `()` is allocated with room for one element,
  count forced to 0, and slot 0 holding an empty `tC` (`aA0`); `mtc_` compares that witness
  because it loops `xn|!xn` times. The encoder therefore writes `max(n,1)` children for every
  ref-carrying type, or `(-9!-8!())~()` would be false.

`-9!` parses untrusted bytes, so every read is bounds-checked, the recursion is depth-limited, and
a truncated, corrupt or over-long buffer yields a clean `'domain` — never a read past the end and
never a half-built object left unfreed. Lambdas and projections (`to`/`tp`/`tq`/`tr`) are
deliberately **not** supported and raise `'type`: serializing a closure means serializing its
captured environment and bytecode, which is a much larger feature than a data wire format.

`-8!`/`-9!` occupy the negative-integer `!` slots, exactly as in q. Only `-8` and `-9` on a
genuine integer atom are intercepted; every other left argument — negative ones included — and
every char atom still reach `mod()` unchanged, so no existing `!` behaviour moves.

### `peach` now uses the binary wire, and three bugs are fixed
`src/i.c`'s worker previously wrote `kst(r)` (text) and the parent ran `val(rda(...))`. Workers
now write `-8!` bytes and the parent decodes with `-9!`. The parent's collection loop was one
line, and it had three distinct defects:

1. **A leak, and quadratic copying.** `out = cat(out, part)` — `cat` is `A2(cat,cat11(xR,y))`: it
   bumps `out`'s refcount and hands that *extra* reference to `cat11`. The caller's own reference
   to the previous accumulator was never released, so every chunk past the first leaked an entire
   result vector. The surviving refcount also made `MINE(out)` false inside `aa()`, so the append
   could not grow in place and reallocated the whole accumulator each time — O(total²) copying on
   top of the leak. Fixed by calling `cat11` directly, which **owns both** arguments: nothing is
   left holding a stray reference and `out` stays uniquely owned, so the append extends in place.
   (Bumping and then releasing — `cat` plus `mr(old)` — plugs the leak but keeps the refcount at 2
   during the call, so it would still reallocate every chunk.)
2. **Worker exit status was discarded.** `wait4(pid, 0, 0, 0)` ignored the status word, so a
   worker that died on a signal or exited non-zero was indistinguishable from success — the parent
   simply saw a short pipe and produced a **silently wrong result**. The status is now inspected
   with `WIFEXITED`/`WEXITSTATUS`/`WIFSIGNALED` and any failure becomes a clean, trappable
   `'worker error in peach`.
3. **Unvalidated decode.** `val(rda(...))` assumed the pipe held a parseable value. `-9!` returns
   0 on a truncated or corrupt buffer, and that is now treated as a worker failure rather than
   dereferenced.

Every child is still waited for even after a failure is detected, so no zombie is left behind and
no pipe fd is orphaned (`rda()` closes its own read end).

Measured on this 2-core sandbox, `peach` over 500,000 items under `AMBER_THREADS=4`: **~23 ms**,
with total reserved heap **identical** before and after three further passes (1,073,741,824 bytes
both times) and a peak RSS of **11.5 MB**. Before the fix the same loop grew without bound.

### New: `examples/peach_verify.k`
A runnable verifier (also suitable for CI — it exits non-zero on any failure) covering the
serializer round trip across every supported shape, attribute preservation, malformed-input
rejection, `peach` result-shape equivalence against serial `'`, the 500k scaling/flat-memory
check, and worker error propagation including that the interpreter is still usable afterwards.
**60 tests, 0 failures.**


## 1.9.2 — O(n+m) integer lookup, vectorised reductions, cache-line alignment

### `?` (find) no longer scans its left argument per probe
`x?y` on integer vectors was `fLL`/`fIL`/..., a LINEAR SCAN of `x` for every element of `y`, i.e.
**O(#x · #y)**. The comparative suite's inner join — 1M left keys probed against 1,000 sparse
right keys — was therefore 500M comparisons: **180.95 ms, 126x the C baseline**, Amber's worst
cell by a wide margin.

`src/f.c` now builds an index over `x` once and answers each probe in O(1), in whichever of two
shapes fits the data:

- **Direct LUT** when the key *range* is small (≤ 64K slots, so the table stays L2-resident):
  `lut[v-lo] = i`, no hashing at all. This is the case the 256-entry byte path (`fndGx`) already
  handled; it now covers `tH`/`tI`/`tL` too.
- **Compact open-addressed hash** otherwise. A flat table is the wrong shape for a *sparse*
  domain: the benchmark's 1,000 keys span a ~1e6 range, so a direct table is 4 MB and every probe
  is an L3/DRAM miss — measured at 28 ms, only 7x better than the scan. Sizing the table to the
  key *count* instead (2·m rounded up — 24 KB here) keeps it in L1 and probes ~10x faster again.

Both fill **backwards**, so the lowest index wins and `?`'s first-occurrence semantics are exact.
Neither is built unless it beats the scan it replaces, and `` `s#``-sorted `x` keeps its existing
O(log m) binary search, so this is a pure fast path. Verified against an unmodified 1.9.1 binary
over 28,429 result lines spanning ranges above and below the LUT cap, negative and 2e9 offsets,
nulls, self-find and atom find — byte-identical throughout.

### Reductions vectorise
`+/` over floats was a serialised `v += p[i]` chain running at ~3.5 cycles/element — the latency
of `addsd`. The compiler may not reassociate it, because IEEE addition is not associative. `sumF`
in `src/3.c` now keeps **four independent partial sums**, which breaks the dependency and lets the
vectoriser issue one wide add per group; the integer `addf*` kernels carry `omp parallel for simd
reduction` hints (`build.sh` probes for `-fopenmp`; without it the four-way unrolling still does
the work). Measured on 10M elements: `+/` **8.9 → 6.3 ms**, `+/x*y` **18.7 → 15.8 ms**.

This changes results for **inexact** float data by 1–2 ulp — always in the direction of *more*
accuracy, since pairwise summation beats a left fold (`+/100000#0.1`: 10000.000000018848 →
9999.999999995287, true value 10000.0). Exactly-representable data — including everything
`bench/SPEC.md` specifies — is bit-identical, as are `0n`, `0w` and `-0w`. It is the same
trade-off `simd.c`'s existing `simd_sum_f64()` already makes. All 601 tests pass unchanged.

### Array payloads are cache-line aligned
`HD` (the array header size, and therefore the alignment of every payload pointer) was 32, which
left every vector buffer exactly **32 bytes past** a 64-byte boundary — measured `ptr%64 == 32`
for every allocation size — splitting a cache line on the first wide access of every array. `HD`
is now 64 and `an()`'s bucket-index constant moves with it; `ARENA_ALIGN` goes 32 → 64 to match.
All payloads are now 64-byte aligned (verified by probe). Because this touches the core buddy
allocator it was validated under **AddressSanitizer + UndefinedBehaviorSanitizer** across every
suite plus the fuzzer, clean.

### Not in this release
**Expression/loop fusion was not implemented.** Amber evaluates eagerly, so by the time `+/`
sees its argument the intermediate vectors already exist; fusing `+/ (y+2.5*x) @ & x>50` into one
pass needs a lazy or fusing evaluator, not a peephole match. A pattern-matcher narrow enough to
fit this release would have recognised essentially the benchmark expression and little else,
which is precisely what `bench/SPEC.md` forbids. The vector-arithmetic and group-by workloads are
therefore **unchanged** in 1.9.2. `|/` over floats also still round-trips through the
order-preserving `of1`/`of0` transforms (three passes and two 80 MB temporaries); collapsing that
into one pass is the clear next win for the reductions workload.


## 1.9.1 — qSQL runs on raw column vectors; CBQN benchmarks fixed

### `select … by … from` no longer boxes a K object per row
`qby`, `xgroup` and `ij` all grouped and probed through `rows:{+. x}`, which flips a table's
column dict into **one boxed K value per row**. A 10-million-row `select … by …` therefore
allocated 10M transient objects in the refcounted heap purely so they could be hashed and thrown
away — 7.9 s of the 8.4 s such a query took, against **87 ms** for the native vector group-by
over the very same data.

The query layer now hands the **raw column vectors** straight to the C kernel's vector `=`
(group), `?` (find) and `@` (index) primitives:

- **`qgrp[t;b]`** (new, `amber.k`) returns `(key-value columns; group row-index vectors)`.
  A single-column `by` groups the column itself. A multi-column `by` is rank-encoded per column
  (`?x` distinct, `u?x` find) and mixed radix-style into one dense integer key, so distinct
  tuples still get distinct codes — with a `2^53` cardinality guard that falls back to the old
  path rather than risk a collision. Key **values** are recovered by indexing the raw columns
  with each group's first row, one gather per by-column instead of a flip over every row.
- **`ij`** rank-encodes both sides over one shared code space, so the probe is a native vector
  `?` on a flat integer vector. Single-key joins skip the encode and probe the raw key column.
- **`qproj`/`qrefs`** (new, `qsql.k`) narrow a table to the columns an aggregate can actually
  reach before `qby` materialises its per-group sub-tables. `atr` indexes *every* column it is
  handed, so on a wide table this stopped copying dozens of unread columns per group; the cost
  now scales with columns **used**, not columns **present**.

Group ordering is unchanged (first appearance), and the outputs are byte-identical to 1.9 across
a differential suite covering single- and multi-column `by`, `where`, `update … by`, `xgroup`,
single- and multi-key `ij`, and empty tables. The 94-case `tests/test_qsql.k` matrix passes
unchanged.

Median kernel ms, 10M rows, same machine. The 1.9.1 column is `bench/run_comparative.py
--runs 5 --warmup 2`; the 1.9 column is the same benchmark script run against an unmodified
1.9 checkout (median of 3 timed passes after 1 warm-up):

| workload | 1.9 qSQL | 1.9.1 qSQL | speed-up | 1.9.1 vs Amber array code |
|---|---:|---:|---:|---:|
| group-by (100 groups over 10M rows) | 8,171.7 | 330.8 | **24.7x** | 1.48x |
| inner join (1M rows against 1,000 sparse keys) | 3,858.9 | 199.7 | **19.3x** | 1.09x |
| vector arithmetic + mask | 107.6 | 102.2 | — | 1.19x |
| reductions (sum · max · dot) | 105.7 | 109.6 | — | 2.06x |

`arith` and `reduce` never touched `qby`; their gap over the array-primitive row is qSQL's fixed
per-call parse-and-compile cost, not a per-row cost, and is unchanged.

### CBQN benchmark scripts compile and self-time
Every `bench/queries/bqn_*.bqn` file failed to compile, so **every CBQN cell in the published
table was an error rather than a measurement**:

- **Identifier roles.** BQN takes a name's role from its first letter — an initial capital is a
  **function**, lowercase is a **subject**. The files opened with `N ← 10000000`, binding a
  number to a function-role name, and CBQN rejected the whole file up front with *"Role of the
  two sides in assignment must match"*. All data names are now lowercase (`n`, `m`, `kn`, `gn`,
  `chk`); only genuine functions (`Kern`, `Time`, `Arg`) are capitalised.
- **`•args` is now optional and safe.** Not every BQN environment binds `•args` — the online
  REPL and `•Import`-ed scopes have no argv, and naming it directly there fails with *"Unknown
  system values: •args"* before a line runs. The reference now goes through `•BQN`, which keeps
  it out of the file's own compilation unit, wrapped in `⎊` (Catch) so "no arguments present"
  yields the documented default instead of a crash. The scripts run identically under
  `cbqn file.bqn`, under the harness (which passes no arguments), and pasted into a browser REPL.
- **Kernel timing.** The files now time themselves with `•MonoTime` and report `TIME_MS`, so the
  10M-element vectors are materialised *before* the clock starts. Previously CBQN had no clock,
  so the harness fell back to net-of-startup wall time and charged CBQN for data generation as
  well as for the query.
- The join uses `⊐` (Index of) — the same lookup Amber spells `kr?kl` — instead of sorting and
  binary-searching with `⍋`.

CBQN now passes the correctness gate on all four workloads with answers exactly equal to the C
reference (`17187568834`, `2515218396283`, `260496190510`, `665380820688`).

### Version
`AMBER_VERSION` in `src/a.h` gains an explicit `AMBER_VERSION_PATCH`, so `./amber --version`,
the REPL banner and the WASM `amber_version()` export all report **1.9.1** from the single
source of truth.


## Unreleased — comparative benchmark suite rebuilt for cross-engine fairness

### Two shortcuts removed from the old suite
- **`+/!10000000` measured an O(1) closed form in Amber.** `src/3.c`'s `arf` constant-folds a sum
  over a *range* into `n(n-1)/2`, so the `vecsum` benchmark had Amber returning the answer without
  touching 10M elements while DuckDB, CBQN and ngn/k ran real reductions. All benchmark data is
  now materialised before the timer starts.
- **The join was really an array index.** With right keys `0..K-1`, every array language answers
  with a single gather while DuckDB still builds a hash table. Right keys are now sparse and
  unsorted (`(7919*j) mod 1048573`), which forces a genuine key lookup in every engine.
- **The old generator overflowed 53 bits.** `2654435761 * i` exceeds `2^53` for `i > 3.4e6`, so
  f64-only engines (Uiua, JS) would have silently generated *different data* from the int64
  engines. The multiplier is now `262147`, keeping every intermediate under `2^53`.

### New: `bench/SPEC.md`
The data model, the four workloads, the exact-arithmetic argument and the fairness rules are now
specified in one document that every engine implements. Every answer is an integer exactly
representable in float64, and every sum is over such integers, so the result is **independent of
summation order** — SIMD pairwise, Kahan and naive left-fold summation all agree bit-for-bit.
There is therefore no floating-point excuse for a mismatch.

### `bench/run_comparative.py` rewritten
- **Ten engines**: C (`-O3 -march=native`, the floor), Amber (array primitives), Amber (qSQL),
  ngn/k, CBQN, J, Uiua, NumPy, Julia, DuckDB.
- **Four workloads**: vector arithmetic + masking, reductions (sum/max/dot over 10M), group-by
  (100 groups over 10M rows), inner join (1M rows against 1,000 sparse keys).
- **Correctness gate.** Answers are compared *exactly* against the C reference; a disagreeing
  engine is printed as **WRONG** and its time is withheld. Each engine also emits a checksum of
  its input data, so a generator divergence is caught separately as **BADDATA**. `--fail-on-wrong`
  turns either into a non-zero exit.
- **Startup excluded.** Engines that can time their own kernel do so with a monotonic clock after
  warm-up passes; engines with no in-language clock are measured as *total − startup baseline*,
  with the baseline measured per engine from a do-nothing script. The table labels which mode
  produced each cell.
- **Amber is reported twice** — primitives (peer of K/BQN/J/Uiua) and qSQL (peer of DuckDB SQL) —
  because publishing only the faster row would be picking the flattering comparison.

### `.github/workflows/benchmarks.yml`
Adds Uiua (release binary, cargo fallback), Julia (`julia-actions/setup-julia`, standalone
fallback), Python + NumPy, J (jconsole) and the native C baseline. Every install is
`continue-on-error` and exports its path via `$GITHUB_ENV` (`UIUA_BIN`, `JULIA_BIN`, `PYTHON_BIN`,
`J_BIN`, `C_BENCH_BIN`, …); a missing engine is reported as "not installed" instead of failing the
job. The workflow's `./amber -e '1+1'` smoke test — `-e` is not an Amber flag and the call only
survived because of a trailing `|| true` — is replaced with `./amber --version`.

### Caveat recorded honestly
The Amber, NumPy and C implementations were executed and cross-checked during development: all
three agree exactly on all four workloads. The ngn/k files were additionally verified by running
them under Amber (same K dialect family). **CBQN, DuckDB, Julia, Uiua and J could not be executed
in the authoring environment** — CI is their first real run. They are written defensively (one
file per workload, no argument parsing, no in-language timer where it was not certain) and the
harness reports a clear per-cell error rather than failing the job.

## 1.9 — version unification, memory/UB audit, and a combinatorial test suite

### Version
- **One canonical version string.** `AMBER_VERSION` now lives in `src/a.h` and is the only
  place a release is bumped. `binfo` (`` `bi 0``) returns it as a 5th element, the REPL banner
  reads it from there (it was hard-coded `v1.7` while the README already advertised 1.9), and
  `amber --version` prints it.
- **`amber --help` / `-h` and `amber --version` / `-v` added.** `main()` previously treated
  `argv[1]` unconditionally as a script path, so `amber --version` tried to open a file called
  `--version` and failed with an `'io` error. `--help` also lists the full `\`-command reference.

### Bugs fixed — memory safety and undefined behaviour
- **Out-of-bounds stack read in `a8()` (amend), `src/a.c`.** The `MC()` calls copied a fixed
  8-slot's worth of argument pointers (40/48/56/64 bytes) out of the caller's `a[]` regardless of
  how many arguments were passed. `a` is not always an 8-element buffer — `run()` (`src/b.c`)
  passes a pointer straight into its own dynamically sized stack frame — so every amend with
  `n<8` read past the end of live storage. Reproduced by AddressSanitizer as
  *dynamic-stack-buffer-overflow* on `test-fin.k` and four `examples/*.k`. Each copy is now
  clamped to the real argument count.
- **`NL` (the long null) was undefined behaviour.** `#define NL (1ll<<63)` shifts into the sign
  bit of a signed type. UBSan flagged it from 17 different sites across `a.c`, `h.c`, `p.c`,
  `r.c`, `f.c`, `s.c`, `c.c`, `i.c`, `3.c` and `csv.c`. Now `((L)(1ull<<63))`, same bits,
  defined semantics. The same treatment was applied to the remaining signed-shift sites in
  `1.c` (`neg`), `2.c` (`ozZ`), `3.c` (`mmmf`, `mxms`) and `p.c` (`pf`), and to `su()`'s
  `1<<31` in `m.c`.
- **Intentional integer wrap was undefined behaviour in the arithmetic kernels** (`src/2.c`,
  `src/v.c`). `aLL/aII/aHH/aGG`, `alL/aiI/ahH/agG`, the widest-width multiplies and `tilV()`'s
  packed-lane counter all rely on two's-complement wraparound, which `oZZ()`/`ozZ()` then detect
  after the fact — but signed overflow is UB, so the optimiser is entitled to assume it never
  happens and delete the very check that depends on it. All of them now do the arithmetic in the
  unsigned counterpart type: identical instructions, defined semantics.
- **Use-after-reset in the `` `simd`` self-test** (`src/a.c`). `arena_reset()` was called
  immediately after `arena_alloc()` and *before* the 400 009-element reference loop that wrote
  into the block, so every store landed in scratch the allocator had already rewound.
- **`memcpy(dst, NULL, 0)` in `run()`** (`src/b.c`) — undefined per the `nonnull` attribute on
  `memcpy`; now guarded.
- **`arena_alloc()` bounds check could wrap.** `off + bytes <= a_cap` overflows for a large
  `bytes` on a 32-bit target (wasm32), silently passing the check and returning a short block.
  Rewritten as `bytes <= a_cap - off`, which cannot wrap. The overflow-block path got the same
  treatment, and `ajc()` now rejects an element count whose byte size would overflow `size_t`.
- **`arena_init()` leaked tracked overflow blocks** when a previous slab reservation had failed
  (`a_base == 0`), because it reset `a_over` to 0 without freeing the list.
- **`arena_used()` under-reported.** It returned only the slab bump cursor, ignoring live
  overflow blocks; it now counts both.
- **`ast_new()` / `ast_add_child()` dereferenced a possibly-NULL `arena_alloc()`** (`src/ast.c`).
  `ast_new()` now degrades to a static sentinel node — returning NULL would only move the
  segfault, since all ~30 call sites dereference the result immediately.
- **`mmap()` failure test used a pointer-to-`char` cast** (`src/m.c`): `(L)p == (C)p`. Replaced
  with a plain `p == MAP_FAILED`.

### Bugs fixed — language semantics
- **A computed float null never matched the `0n` literal.** IEEE has no single NaN bit pattern:
  `0n` is the positive quiet NaN, but every NaN the FPU *computes* (`0%0`, `0w-0w`,
  `avg 0#0`, …) is the default NaN, which on x86-64 has the sign bit set. `~` compared bytes, so
  `(0%0)~0n` was `0b` even though both sides print as `0n` and both answer `1b` to `^`. `mtc_()`
  now compares float payloads value-wise with all NaNs treated as one value (only on the path
  where the byte-wise fast path has already failed, so equal data still costs one `memcmp`).
- **The float formatter printed `-0n`** for any computed NaN. A NaN has no meaningful sign;
  `sf()` no longer emits the leading `-` for it. `0%0` and `(-0w)+0w` now print `0n`.
- **`deltas` and `ratios` raised `'length` on an empty argument** (`amber.k`). `0,-1_()` is a
  one-element vector, so `x-0,-1_x` mismatched. Both now return the empty vector, as q does.
- **`sum`/`prd` of an empty FLOAT vector returned an integer.** `amber.k`'s wrappers
  short-circuit an empty argument to the literal `0`/`1`, dropping the float type that the
  underlying `+/`/`*/` reductions get right (`+/0#0.0` is `0.0`, but `sum 0#0.0` was `0`). This
  surfaced as `select t:sum px from none` producing an int column. Both now return `0.0`/`1.0`
  for a float argument.
- **`med` of an empty vector returned `0.0`** while `avg`/`dev`/`var` return the float null; it
  now returns `0n` like the rest of the aggregation family.
- **`qselect` was broken for more than one aggregation** (`amber.k`).
  `select a:sum px, b:max sz from t` built the dict `` `a`b!(210.0;6)`` and flipping it raised
  `'length`; the single-aggregate case only *appeared* to work because the each-result happened
  to be a one-item list. Each aggregate result is now joined onto `()` so an atom becomes a
  one-row column.
- **`select by sym from t` (a `by` clause with no projection) raised `'value`** (`qsql.k`). The
  `" by "` split never matched, because when the by-clause is the whole select-part it has no
  space in front of it; and even once split, an empty projection ignored the grouping entirely.
  `qsplit0` handles the position-0 case and the empty projection now aggregates each non-key
  column with `last`, matching q.

### New: `` `diag`` — runtime control of the stderr diagnostic
Amber renders its Rust-style diagnostic at error **creation** time, so code that catches an error
with `.[f;args;handler]` has already had the full report splashed across stderr. That is right for
an interactive line and wrong for anything whose job is to provoke errors it then handles — a test
suite's must-raise cases, `protect`, a retry loop — which drowned the terminal in red for errors
it dealt with perfectly. `` `diag 0`` turns the report off and returns the previous setting;
`` `diag 1`` turns it back on. The compact caret text is untouched: it is buffered and still
handed to the trap handler and to `` `err``, so nothing is lost. The existing `AMBER_DIAG=0`
environment variable still works and now just seeds the initial value.

### Diagnostics
- **`\trace`'s "Arena peak" was always `0 B`.** It reported `max(used-before, used-after)`, and
  every arena consumer rewinds the slab before returning. `arena.c` now maintains a true
  high-water mark (`arena_peak()` / `arena_reset_peak()`) that survives `arena_reset()`.
- **`\trace` timings below one microsecond rendered as `0us`;** the formatter now prints
  `ns` / `us` / `ms` as appropriate.
- **`\trace`'s report box is square again** — the bar glyph is 3 bytes but 1 column wide, and the
  old `printf` field widths counted bytes, leaving the right-hand border ragged.
- The `` `arn`` self-test now genuinely exercises the arena **overflow** path it advertises. It
  asked for 1 MB against a >=16 MB slab (`arena_init(1<<16)` only rewinds an already-larger slab,
  it never shrinks it), so the overflow branch was never taken; it now requests
  `arena_capacity() + 64 KB` and also asserts that the peak survives a rewind.

### Dead code and duplication removed
- `src/3.c`: deleted the unused static helpers `minfB`, `maxfB` and `eqlsL`.
- `src/a.c`, `src/h.c`, `src/w.c`, `src/i.c`: removed unused / write-only locals (`m`, `p`, `u`)
  and a discarded expression value (`f>2&&close(f)`).
- **One canonical `lower_bound`.** `ajlb()` (`src/a.c`) and `wjlb()` (`src/i.c`) were the same
  contract under two names with different (branch-free vs branchy) codegen. Both join kernels now
  call the exported branch-free `amlb()`.

### Tests
- **`tests/harness.k`** — shared assertion harness (`t`, `tv`, `te`, `tk`, `hexpect`, `hreport`).
  Every assertion is trapped, so a failing or throwing case can never abort a suite or hide the
  cases after it.
- **Suites are self-locating.** `\l amber.k` resolves against the *current working directory*, so
  a suite written that way only runs from the repo root — `amber /path/to/tests/test_matrix.k`
  died with a bare `'io`. Each suite now derives the repo root from its own script path
  (`` `argv 1``, the same trick `repl.k` uses) and runs from anywhere.
- **Suites exit non-zero on failure**, so CI and `tests/run_tests.sh` can gate on status as well
  as on the printed report.
- **`hexpect[n]` assertion-count guard.** In K, `f [a;b]` **with a space** is not a call: the
  parser reads `[a;b]` as a bracketed statement block and quietly builds a projection that is
  then discarded — no error, no output, the assertion simply never runs. An earlier revision of
  `tests/test_qsql.k` lost 42 of its 93 cases to exactly this **and still printed
  "ALL TESTS PASSED"**. Each suite now declares how many assertions it expects to record and
  fails if the number disagrees.
- **`tests/test_qsql.k` is written in the bare `select … from t` syntax** users actually type,
  put through the same `qrw` rewrite `repl.k`'s `line1` applies to every input line, instead of
  the `sel"…"` wrapper — so the suite exercises the rewriter and the query engine together, on
  the user's own code path. Section 9 asserts the explicit wrapper gives an identical answer.
- **`tests/test_matrix.k`** — 309-case combinatorial matrix: every primitive against every
  element type (Long / Float / Boolean / Char / Symbol / nested list / dict / table) at sizes
  0, 1, 10 and 100 000+, including the 99 999 / 100 000 / 100 001 / 250 000 sizes that straddle
  `PAR_THRESHOLD`. Cases assert invariants (shape, algebraic identity, vector-kernel-vs-scalar-
  reference agreement) rather than frozen literals.
- **`tests/test_qsql.k`** — 94-case qSQL matrix over the full clause lattice, multi-key `by`,
  empty / single-row / heavily-duplicated tables, the bare-qSQL rewriter, and malformed queries.
- **`tests/fuzz.py`** — malformed-input and deep-nesting crash fuzzer (unbalanced brackets,
  dangling adverbs, truncated qSQL, 20 000-deep nesting, out-of-range literals). Asserts a clean
  K error, never a signal or a hang.
- **`tests/run_tests.sh`** — one entry point for all suites plus the C unit tests; `--asan`
  rebuilds under AddressSanitizer + UBSan and re-runs everything, including the fuzzer.
- **The suites are quiet on success.** `tests/harness.k` calls `` `diag 0`` on load and restores
  the previous setting in `hreport`, so a run that provokes ~35 deliberate errors prints its
  summary and nothing else instead of ~35 full-colour diagnostic blocks. A case that fails
  unexpectedly still reports the error kind on its own FAIL line (`FAIL raised: 'length  <-
  deltas 0#0`), so nothing is hidden.
- CI (`.github/workflows/ci.yml`) now runs the new suites and the fuzzer on every push.

## Unreleased — separate, independently-tuned comparative benchmark query files
- **`bench/queries/amber_{vecsum,vecarith,groupby}.k`** added — `bench/run_comparative.py`'s
  Amber row previously reran the same `k_<id>.k` file used for the "K" (ngn/k) row. Amber now
  gets its own query file per workload, with a header comment on each documenting what
  engine-level optimization was tried, what it measured, and — for the ideas that didn't pay
  off — why not. See the README's
  [Comparative benchmark query files](README.md#comparative-benchmark-query-files) section
  for a summary, or the files themselves for the full detail.
- **`check_parity()`** added to `bench/run_comparative.py`: runs `amber_<id>.k` and `k_<id>.k`
  once each before the timed runs and compares their printed output, so a claimed speed win can
  never silently also be a wrong answer. Reported on stderr and prefixed onto the generated
  Markdown table.
- **Known engine bug found and documented** (not fixed here — see docs/MISSING.md): a `.k`
  comment line containing only a bare `/` with no trailing space or text silently truncates
  parsing of the rest of the file, with no error raised.

## Unreleased — removed the `\hl` syntax-highlight command
- **Removed** `src/highlight.{h,c}`, the `\hl <expr>` REPL command, and `tests/test_highlight.c`.
  `\hl` only ever colorized a line you explicitly ran (`\hl select ...` echoed that one line back
  with ANSI colour) — it never highlighted your keystrokes *as you typed them*, which is what
  "REPL live syntax highlighting" actually means. Real live/incremental highlighting would
  require rewriting `repl.k`'s raw-keystroke input loop (a fragile, previously-regression-prone
  path in this project), which is a materially different and larger undertaking than a one-shot
  echo command — removed rather than kept as something that doesn't do what its name promises.
  See [docs/MISSING.md](docs/MISSING.md) for this as a possible future direction.
- No other engine extension is affected: SIMD, the multithreaded vector engine, the bytecode
  disassembler, and the native CSV parser are all unchanged.

## Unreleased — `\ast` visualizer overhaul
- **Bug fixed: generic `<X-atom>` placeholders.** The previous formatter only understood five
  atom tags (symbol/int/long/float/char) and treated everything else -- multi-element data
  vectors (`1 2 3`, `` `a`b`c ``, `"hello"`, ...), verb/adverb atoms appearing bare (inside a
  hook/fork, or `\ast +` on its own), lambda bodies (`{x+1}`), and the `GAP` sentinel that marks
  a curried-away argument -- as an opaque "bare atom" and printed a useless `<v-atom>`/
  `<w-atom>`/`<o-atom>`/`<I-atom>`/`<S-atom>` placeholder. Every one of those shapes now gets an
  explicit, correctly-typed node: literal vectors preview their contents with a
  `(TypeName Vector[len])` annotation, scalars are labeled `Int64`/`Float64`/`Symbol`/`Char`,
  lambda literals show their real source text, and curried/partial applications (`1+`,
  `f[x;;z]`) render as an explicit **Projection** with a **Blank** node for the omitted slot.
- **Bug fixed: namespaced identifiers misread.** `.ns.sub` printed as `"ns.ns"` (a duplicated
  first segment) because the old code read a Symbol Vector's elements via `_A(v)[i]` (an 8-byte
  `A*`-stride cast) when Symbol Vectors actually store **packed 32-bit ids** -- reading through
  the wrong stride silently pulled back the wrong bytes. Fixed by reading through
  `(const I*)_V(v)` everywhere a Symbol Vector's elements are touched.
- **New: tacit-form annotations.** A 2-verb list `(f g)` is now an explicit **Hook**; a 3-verb
  list `(f g h)` is an explicit **Fork**; both are recognised by checking whether every list
  element is itself verb-like, rather than always rendering as a generic list literal.
- **New palette.** Verbs (bare, applied, curried, or the head of a call) render bold cyan;
  adverbs bold magenta; numeric/literal scalars bright green; variables/symbols yellow; the
  tree's Unicode connectors themselves dim gray. A combined glyph like `+/` is colour-split
  within one label (`+` cyan, `/` magenta) rather than picked as a single node colour.
- **Memory: arena-backed, not malloc/free.** The whole `ASTNode` tree built for one `\ast`
  invocation is now bump-allocated from Amber's real scratch region (`arena_alloc()`/
  `arena_reset()`, `src/arena.h`) instead of `malloc`/`calloc`/`realloc`/`free` -- the same
  region `src/csv.c`'s transient row grid and `src/trace.c`'s own diagnostics already use.
  `ast_free()` is now a documented no-op; `arena_reset()` in `ast_cmd()` does the real cleanup.
  (Amber has no symbol literally named `g_scratch`; `arena_alloc()`/`arena_reset()` *is* its
  scratch-arena API, used directly.)
- **Safety: recursion depth guard.** `ast_from_k()`'s walk now refuses to recurse past 64 levels
  and returns a synthetic "(max depth exceeded)" leaf instead, defense-in-depth alongside pk()'s
  own (tighter, ~60-level) parser nesting limit.
- **Tests:** a new `` `astt`` self-test builtin (wired into `test.k`, mirroring `` `arn``/
  `` `dgn``/`` `simd``/...) plus a new standalone `tests/test_ast.c` harness -- the latter links
  against the full interpreter (ast.c is inherently built on Amber's real parser/value
  representation, unlike `tests/test_simd.c`/`test_parallel.c`, which are
  dependency-free by design) and checks label correctness, a broad no-placeholder regression
  sweep, ANSI colour coverage for all five required categories, and that moderately deep (but
  parser-legal) nesting does not crash.
- **Honest scope note:** `\ast` still never compiles or runs the expression it's shown, with one
  pre-existing, unavoidable exception documented in `ast.h`'s file header -- `pk()` itself
  eagerly compiles lambda *literals* (`{...}`) at parse time (see `p.c`), so a compiled closure
  can appear as a leaf even in an otherwise-uncompiled tree. This module shows that closure's
  captured *source text*, never its bytecode (that remains `\disasm`'s job, `src/vm.{h,c}`).

## Unreleased — fix `peach` worker-count default
- **Bug:** `peach[f;y]`'s default worker count (`src/i.c`'s `peachNW()`) was a hardcoded `4`
  whenever `AMBER_THREADS` was unset, regardless of how many CPUs the host actually had. On a
  small box (e.g. 2 cores) this **oversubscribed** — forking 4 processes onto 2 cores made
  `peach` measurably *slower* than serial `` f'y `` through pure fork/context-switch overhead
  (observed: 0.55x–0.79x "speedup" running `examples/peach.k` in a 2-core sandbox). On a large
  box it left most cores idle.
- **Fix:** `peachNW()` now defaults to the actual online CPU count via
  `sysconf(_SC_NPROCESSORS_ONLN)` (new `peachCPUs()` helper in `src/i.c`), the same approach
  `src/parallel.c`'s `par_thread_count()`/`online_cpus()` already uses for the SIMD/vector
  engine — the two parallel primitives in Amber now agree on what "auto" means.
  `AMBER_THREADS=N` still overrides explicitly and is unchanged. Verified: re-running
  `examples/peach.k` in the same 2-core sandbox went from ~0.55–0.79x ("speedup") to a genuine
  ~1.7x.
- **Cleanup:** removed two stale duplicate source files at the project root (`i.c`, `ar.c`) —
  leftovers from before the `src/` reorganisation, byte-for-byte identical to their `src/`
  counterparts except for the fix above, never compiled by `build.sh` (which only builds
  `src/*.c`), and a source of confusion if edited by mistake.

## Unreleased — engine extensions (SIMD, parallel, disassembler, CSV)
- **SIMD vector kernels.** `src/simd.{h,c}` — AVX2 (x86_64), ARM NEON (`aarch64`, incl. Apple
  Silicon), and scalar C99 fallback implementations of `add`/`mul`/`sum` over `int64_t`/`double`
  arrays, selected at compile time. `src/arena.c`'s bump allocator now guarantees genuine
  32-byte alignment (`posix_memalign`, up from plain `malloc`'s 16-byte guarantee). Self-test +
  benchmark: `` `simd 0``. Build with `AMBER_NATIVE=1 ./build.sh` to activate AVX2 on x86_64
  (NEON activates unconditionally on `aarch64`).
- **Multithreaded vector engine.** `src/parallel.{h,c}` — splits arrays above 100,000 elements
  across POSIX threads (`AMBER_THREADS` env var, same convention as `peach`), each chunk
  processed by the SIMD kernels above. Self-test + benchmark: `` `para 0``.
- **Bytecode disassembler (`\disasm`).** `src/vm.{h,c}` — Amber's compiler/VM already exists in
  `src/b.c` (AST is compiled to opcodes + a constant pool, then run on a real stack VM); this
  mirrors that opcode table byte-for-byte and decodes real compiled bytecode with a
  self-consistency check, rather than adding a second, disconnected VM. Self-test: `` `vmd 0``.
- **Native CSV parser.** `src/csv.{h,c}` — `` `csvr "path.csv"`` parses a file straight into a
  genuine Amber table (`flp(names ! cols)`, the same shape `([]…)` produces) via the arena
  allocator, with per-column type inference (Long/Float/Symbol), quoted-field handling
  (embedded commas, `""`-escaped quotes), and null-mapped empty cells. Self-test: `` `csv0 0``.
- **Test suite:** `test.k` grew from 158 to 161 assertions (one self-test call per module
  above); `tests/test_simd.c` and `tests/test_parallel.c` are new standalone C harnesses (no
  Amber dependency) for the SIMD and parallel modules.

## Unreleased — REPL diagnostics + cleanup
- **`\v` rich workspace inspector.** `src/inspect.{h,c}` — every currently-defined global as an
  ASCII table (Name / Type / Shape·Length / Memory), with a recursive deep-memory-footprint
  walker (`iv_deepsize`) and a structural table-vs-dict classifier (`iv_as_table`) since both
  share the `tM` heap tag in Amber.
- **`\ast` AST visualiser.** `src/ast.{h,c}` — a parse-only, colour-coded tree view of an
  expression (verbs bold cyan, binary ops bold magenta, variables yellow, scalars green, vectors
  cyan, function application bold blue, list literals bold green, block separators dim gray),
  built from the same shape-dispatch `cr()` uses to compile.
- **`\trace` execution profiler.** `src/trace.{h,c}` — 4-phase timing (parse / arena setup /
  execute / format) via `clock_gettime(CLOCK_MONOTONIC)`, a Unicode block-bar report, and the
  arena's peak scratch usage for that one evaluation. Runs the input through the same
  `qsql.k`/`qrw` rewrite the interactive prompt uses first, so tracing a table expression or a
  bare `select … from … where …` query renders and profiles correctly instead of failing to parse.
- **`\timer` removed.** An earlier execution-timer badge (`src/badge.{h,c}`) was tried and then
  explicitly removed at the request of the project owner; `\v`/`\ast`/`\trace` above are its
  replacements for workspace/perf visibility.
- **Cleanup.** Consolidated three copies of a `fmt_bytes()` helper (`inspect.c`, `trace.c`,
  `badge.c`) into `src/fmtutil.{h,c}`; consolidated a duplicated `C_RST` ANSI-reset macro
  (`ast.c`, `diagnostic.c`) into `src/ansi.h`; removed dead code (`diagnostic.c`'s unused `Span
  all`, unused `<string.h>`/`<stdint.h>` includes); fixed two feature-test-macro gaps
  (`_POSIX_C_SOURCE`/`_DEFAULT_SOURCE`) that broke a strict `-std=c99` build of `trace.c`/`m.c`;
  tree compiles warning-clean under `-Wall -Wextra -std=c99` (aside from pre-existing sign-compare
  noise from `a.h`'s `LH`/`TU` macros) and passes the full `test.k` suite (158/158).

## 1.9 — Mac integration + HFT features
- **Native `aj` as-of-join kernel.** `aj`/`aj0` now compute their match indices in C
  (`src/a.c`, `ajc` + `ajlb`) instead of the per-row K `bin`. For each trade the kernel does a
  **branch-free `lower_bound`** (the comparisons lower to `cmov`, no data-dependent branches)
  over the trade's symbol-group slice of the sorted nanosecond timestamp column, returning the
  index of the most-recent quote on-or-before the trade (or null when the group is empty / no
  quote precedes the trade). `amber.k`'s `ajm` marshals `(qt; tt; groupBase; groupEnd)` and calls
  the `` `aj `` builtin; the pure-K reference is retained as `ajmK`. Correct on 64-bit ns
  timestamps, single-group (time-only) joins, absent symbols and before-first-quote rows. New
  tests in `test.k` (`ajNull`, `ajNoGrp`, `ajNs`).
- **HFT zero-allocation arena.** `src/arena.{h,c}` — a **thread-local 16 MB bump allocator**
  (`arena_init` / `arena_alloc` / `arena_reset` / `arena_free`, plus `arena_used`/`arena_capacity`)
  with a leak-free overflow path. Reserved at startup (`kinit`) and rewound once per evaluation
  cycle (`evs`), so the transient buffers produced while evaluating an expression come from a
  single pointer bump instead of `malloc`/`free`, keeping their latency jitter out of the hot
  path. The native `aj` kernel uses it for its transient match vector. Self-test builtin
  `` `arn `` (exercises bump / reset / the >slab overflow path); `test.k` asserts it.
- **Rust-style visual diagnostics.** `src/diagnostic.{h,c}` — a `Span` source-tracking struct
  (`src`, `start`, `end`, 1-based `line`/`col`) and a `report_diagnostic()` renderer that emits a
  Rust-compiler-style report: `error[CODE]: title`, a `-->` file:line:col locator, a gutter-aligned
  source line, per-span `^^^` underlines and a `= help:` note, ANSI-coloured when a colour terminal
  is present. The runtime error path (`src/e.c`, `eS`) — through which all parse / type / domain
  errors funnel — routes them through the formatter **by default**, printing the report above the
  existing compact caret line; set **`AMBER_DIAG=0`** to suppress it and get the terse one-line
  errors only. One `getenv` per error, never on success. The source noun is copied to a bounded,
  NUL-terminated buffer before rendering (Amber char vectors are length-prefixed, not
  NUL-terminated). Self-test builtin `` `dgn ``; `test.k` asserts the rendered report's structure.
- **CRLF-safe REPL loader.** `repl.k` loads its `.k` library line-by-line; a Windows/CRLF
  checkout previously left a trailing `\r` on every line, which broke each definition (so e.g.
  `qsql.k`'s `qrw` never loaded and the REPL failed on the first input). The loader now strips
  `\r` per line (`{. x^(`c$13)}`), so the REPL works regardless of checkout line endings. A new
  **`.gitattributes`** additionally forces LF checkout of `*.k`/`*.c`/`*.h`/`*.sh` so the problem
  cannot recur.
- **Portability.** The tree builds warning-clean under both **gcc and clang** (portable C99 /
  POSIX; thread-local via `__thread`), on x86-64 Linux and Apple-Silicon macOS. Test totals:
  `test.k` 158, `test-fin.k` 35, `test-ext.k` 79 = **272**, 0 failures.

## 1.8
- **Table size footer.** A bare table / keyed table at the prompt now prints a
  `[N rows x M cols]` summary underneath the grid, dimmed. `N` uses thousands separators
  (`[1,000 rows x 4 cols]`). Row/col counts are the *true* totals (`#x` / `#cols x`), computed
  before the `CROWS` grid truncation, so a previewed million-row table still reports its full
  size. Implemented in `repl.k` (`fmt`/`ftr`, `comma`).
- **Error ergonomics.** The terse one-word error is expanded to descriptive text
  (`'length: operands have mismatched counts`, `'type: wrong type for this operation`, …) via a
  lookup over the core's 12 error kinds, while the REPL still prints the offending input line with
  the `^` caret under the failing operator/verb (the caret comes from the C core's `err`/`eQ`
  machinery). Implemented as a repl-local handler `onerr`/`edesc` in `repl.k`.
- **ANSI syntax highlighting.** Grid output (tables, keyed tables, dictionaries) is coloured for
  dark terminals with a vivid **256-colour, 14-hue per-column palette** (`PAL`) — each column a
  distinct colour, headers bold white, nulls and the size footer dimmed; dictionary values keep a
  per-type tint. Colour is applied *after* width padding (ANSI is zero visual width), and a new
  `vlen`/`vstrip` strips ANSI before every column-width and header-underline computation, so
  alignment is exact. `COLOR:0` disables it globally. Implemented in `amber.k`
  (`amblk`/`amtab`/`amkeyed`/`amdict` + palette `CT`/`cwrap`/`ccol`/`ccell`); reached from the
  REPL through the existing `amfmt` call, so no cross-namespace globals are introduced.
- **Unicode table borders (`\grid`).** `\grid clean|rounded|sharp|heavy` toggles the table frame:
  `clean` (default) is the minimal dashed header rule; `rounded`/`sharp`/`heavy` draw a full box
  with curved / square / thick corners, `│`·`┃` column dividers and proper `┬┼┤`-style
  intersections. Borders are dimmed grey so colourised cells stay the focus; column widths are
  measured with `vlen` (ANSI stripped) and box segments repeat the multi-byte glyph per *character*
  (not per byte), so everything lines up around coloured data. Big tables truncate after `CROWS`
  rows with a dotted row inside the frame and the `[N rows x M cols]` footer below it. New `\clear`
  command clears the terminal (scrollback + screen). Border set in the root global `GRID`.
- **Grid polish.** Numeric and temporal columns are **right-aligned** (symbols, chars and strings
  stay left-aligned); the header underline is dimmed grey so data rows stand out; null sentinels
  (`0N`/`0n`/`0w`/blank) render faint grey. Float precision in grids is capped by a new **`PREC`**
  global (default 7 decimals; `PREC:0N` restores full precision), so a column of 14-decimal floats
  no longer sprawls. All applied after width padding, with `vlen` keeping alignment exact.

## 1.7
- **Native temporal types.** `date` (days since 2000.01.01), `time` (ms of day) and
  `timestamp` (ns since 2000.01.01) are now first-class C-level types with their own type
  tags. Literal syntax parses directly — `2026.07.30`, `10:00:05.000`,
  `2026.07.30D09:30:00.000000000` — and values auto-display in their own format.
  Type-aware arithmetic: `10:00:00.000+00:00:05.000` → `10:00:05.000`, `date-date` → days,
  `date+n` → date, plus comparisons. String casts `"D"$`/`"T"$`/`"P"$`, accessors
  `year`/`month`/`day`/`dow`/`thh`/`tmm`/`tss`, and `` `i$`` to extract the raw value.
  Columns keep numeric storage (as kdb does) so `xasc`/`s#` and name-based `time`-column
  display continue to work. Implemented across `a.h` (enum + type tables + `TU`/`TP` macros),
  `p.c` (literal scanner), `s.c` (formatter), `2.c` (arithmetic), `c.c` (casts).
- **C-kernel window join.** `wj` moved from an interpreted per-row K loop to a C routine
  (`wjc` in `i.c`): a vectorised binary-search range probe per row plus a contiguous slice
  sweep for the standard reducers (`first last min max sum avg count`). ~2× faster in a
  throttled sandbox (more on real cores), bit-exact vs the interpreted `wjK` fallback for the
  non-floating reducers. Arbitrary aggregators fall back to `wjK`.
- **C-kernel `ema`** — the exponential moving average was an interpreted per-element scan;
  now a single O(n) C sweep (`emaC`), verified identical to the K version.
- **SIMD math.** Portable vectorization pragmas (`clang loop vectorize` / `GCC ivdep`) on the
  hot elementwise float kernels in `2.c`.
- **Terminal charting.** `plot` draws a numeric vector as a 2×4 **Braille** line chart
  (Bresenham into a bitmask canvas, min/max-scaled with Y-axis labels); `candle` draws an
  OHLC table as **Unicode candlesticks** with ANSI colour (green up / red down), box-drawing
  wicks and block bodies, one space between candles. Exposed in `sys.k`; C kernels
  `plotC`/`candleC` in `i.c`. See `examples/graphs.k` for a 13-chart tour.
- **Apache Arrow C Data Interface (zero dependency).** `arrow.export` / `arrow.import`
  interop with PyArrow / Polars / DuckDB over the stable Arrow C ABI with no `libarrow`
  linkage. `ArrowSchema`/`ArrowArray` structs are embedded in `a.h`; export is zero-copy
  (child `buffers[1]` alias Amber column payloads and a `release` callback `mr`s them);
  import parses format strings, applies validity bitmaps as nulls, and rebuilds a table
  (a copy — Amber's inline object header precludes aliasing a foreign buffer). New file
  `ar.c`. Numeric widths, ranges and symbol (utf8) columns round-trip exactly.
- **Cleanup.** Removed the dead duplicate `gentq`; interpreted `wj` retained as `wjK`
  (arbitrary-aggregator fallback). Test suite now **267** (153 + 35 + 79), 0 failures.

## 1.6
- **Q-style grid preview.** `show t` (and a bare table/keyed-table/dict at the prompt) now
  prints only the first `CROWS` rows (default 20) followed by a `..` line, instead of dumping
  the whole thing. The cap is applied *before* formatting, so previewing a million-row table
  is instant. Set `CROWS:n` at the prompt to change it (e.g. `CROWS:10`); small tables print
  in full. Implemented in `amtab`/`amkeyed`/`amdict` (amber.k).
- **`examples/peach.k`** — a runnable Monte-Carlo demo that times serial `` f'y `` vs
  `peach[f;y]` so you can see the multi-core speedup on your own hardware
  (`AMBER_THREADS=8 ./amber examples/peach.k`).
- **Banner shows v1.6** and advertises bare qSQL + parallel `peach`.
- **`peach` is now genuinely parallel (multi-core), in C.** `peach[f;y]` forks
  `AMBER_THREADS` worker processes (default 4), each applies `f` to a slice of `y`,
  serialises its result (`` `k ``) down a pipe, and the parent deserialises (`val`) and
  concatenates. Because `(. `k v) ~ v` holds for every value, the result is identical to
  serial `` f'y `` — verified across vectors, symbols, tables, nested and ragged results, and
  a 200-iteration stress run; all 226 tests still pass. Set `AMBER_THREADS=N` to control the
  worker count (`=1` forces serial). Implemented as a new C primitive (`peachC` in `i.c`,
  wired through the `sym1` system-function table) — additive, so it can't affect the serial
  core. Fork-based (copy-on-write heap) so there are **no data races**: this matches how
  kdb+ gets multi-core and avoids the atomic-refcount tax that shared-memory threading would
  put on all single-threaded code. Unlike Python threads (GIL-bound), Amber's workers run on
  all cores at once. Best for coarse-grained, compute-heavy per-item work; see BENCHMARKS.md
  for when the fork/serialise overhead makes serial `'` the better choice.

## 1.5
- **The v1.5 extended modules now actually load in the REPL.** `repl.k` previously loaded only
  `amber.k` + `fin.k`, so `std`/`qsql`/`temporal`/`sys`/`hdb`/`ipc` — and therefore `sel`,
  `select … by …`, the moving aggregates, temporal helpers, etc. — were silently unavailable at
  the prompt. `repl.k` now loads all six in dependency order.
- **Bare qSQL.** You can type `select … by … from … where …`, `exec`, `update`, and `delete`
  directly — no `sel"…"` wrapper. A reader rewriter (`qrw` in `qsql.k`) turns such a line into
  the matching `sel/exq/upd/del "…"` call before evaluation; it also handles assignment
  (`r:select …`) and prefixes (`5#select …`, `count select …`), and leaves ordinary
  expressions and the explicit string forms untouched. Documented on the `\j` help page and in
  AMBER.md §7.
- **`select from t` fix.** `sel` now parses a column-less `select from t [where …]` (a leading
  `from`, all columns) correctly instead of mis-splitting the clause.
- **Benchmarks + sanity harness.** New `BENCHMARKS.md` and `bench/` (Amber vs numpy/pandas here;
  auto-runs growler/k, kdb+/q, DuckDB, Polars where installed — `bench/run.sh`). All aggregation
  results cross-check exactly against numpy/pandas; Amber's `group-by` and `distinct` beat pandas.
- **Faster as-of join.** `aj`'s matcher (`ajm`) now issues **one vectorised `bin` per group**
  (`'` on a sorted noun) instead of a per-row scalar search — ~6× faster (≈700→115 ms at 50 k
  rows), identical results, all 226 tests still pass. (`wj` still uses the per-row form.)
- **`build.sh` defaults to portable `-O3 -flto`** (was `-O2`). Link-time optimisation is
  auto-detected with a safe fallback if the compiler lacks it; `filter` ~2× faster and the
  hash/group kernels ~20% faster, with no `-march` so the binary stays portable.
  `AMBER_NATIVE=1 ./a` opts into `-march=native -funroll-loops` (group-by roughly halves again).
  PGO was tested and dropped — the gain was noisy and not worth the two-pass build. See
  BENCHMARKS.md.

## 1.4.1
- **`gentq` now marks both key columns**: `time` gets the `` `s`` sorted attribute and `sym`
  gets `` `p`` parted — both visible in `meta trades` / `meta quotes` (via `fin.k`'s
  `sortcol`/`partcol` helpers). `test-fin.k` asserts both (now 35 tests).
- **Help expanded**: the `\z` page documents all four attributes (`` `sa`` `` `ua`` `` `pa``
  `` `ga`` + `` `at``, with complexity), and a new **`\m` page** documents the whole `fin.k`
  HFT vocabulary. Added to the main `\` menu.

## 1.4
- **`fin.k` — a financial / HFT module** (auto-loaded after `amber.k`): a fixed `gentq[n]`
  that sets global `trades`/`quotes` tables with **numeric times** and attributes on the key
  columns (`` `s`` on `time`, `` `p`` on `sym`); an O(1) grouped index (`bysym`/`symrows`);
  order-book analytics (`mid` `spread`
  `spreadbps` `micro` `imbal`); trade analytics (`vwap` `twap` `tsign` `signedvol`
  `effspread` `notional`); returns/vol (`ret` `logret` `rvol` `movavg` `movsum` `movmax`
  `movmin` `ema` `rollstd`); `bars` (OHLCV) and `symstats`; and `pt` (time-formatted print).
- **All four kdb attributes now exist in C**: `` `sa`` (sorted), `` `ua`` (unique),
  `` `pa`` (parted), `` `ga`` (grouped); `` `at`` reports `s`/`u`/`p`/`g`; `meta` shows them.
  Sorted **and parted** columns get O(log n) kernel find; grouped + the group index give O(1)
  per-symbol slicing (see `bench-fin.k`: ~20,000x vs a linear scan).
- **Join fix**: joins require **numeric** time columns — store times as ms and format only for
  display (`tsym`/`pt`). `stime` on the stored column breaks `aj` (it makes time a string).
- New: `examples/hft.k` (full HFT walkthrough), `examples/attributes.k`,
  `examples/practice.k`, `test-fin.k` (32 tests), `bench-fin.k`. Core suite: 153 tests.


## 1.3
- **Fixed table/grid rendering** used by both `show` and the bare-expression REPL:
  `amdict` now handles list-valued dicts (e.g. `group`), `amcells` renders nested columns
  (e.g. `xgroup`), and `iskeyed` no longer crashes on plain vectors (it used odometer on
  non-dicts). `all`/`any` no longer rely on the unsupported `` `b$`` cast.
- **`./a` rebuilds when the C sources are newer than the binary** — no more running a stale
  build (this was why `([]…)` tables didn't render for some copies).
- **New banner** with a techy subtitle; **`examples/tour.k`** shows a worked, tested example of
  every function; test suite grown to **148 assertions** covering essentially the whole library.
- Rewrote the GitHub README.


## 1.2
- **`([]col:vals;…)` table literals** and **`([key:vals]col:vals)` keyed-table literals**,
  implemented in the C parser (`p.c`) — build tables the q way, no `+…!(…)` needed.
- **Own identity**: every reference to the upstream array core has been removed from the code,
  banner, help and docs; the language stands alone as Amber. (AGPLv3 attribution is retained in
  `NOTICE`, as the licence requires.)
- **New banner** — a clean wordmark, no clutter.
- **`bench.k`** — attribute speed harness (find/`in`, sorted vs unsorted, across sizes).
- **`MISSING.md`** — an honest map of kdb+/q features not yet in Amber, with a roadmap.
- **`round[d;x]`**, table-literal tests; suite now 104 assertions.


## 1.1
- **Temporal / tick support**: `hms hh mm sec milli minute second stime ptime` (time-of-day
  as ms since midnight, mirroring q's `time.hh` / `time.minute` accessors), plus
  `minbar` / `bar` / `xbar` for tick.minute-style OHLC bucketing.
- **`meta` shows attributes**: now a keyed table with `c` (column), `t` (type) and
  `a` (attribute) — so a sorted column shows `a: s`.
- **Grid rendering**: `show` / auto-print renders tables, keyed tables and dicts as clean
  q-style grids; `tsym[t;c]` formats time columns as `HH:MM:SS.mmm`.
- **Restructured help**: `\q` (scalars/agg/sets/strings), `\j` (tables/joins/qSQL),
  `\z` (temporal/attributes/display), plus a reorganised main `\` menu.
- **Examples**: `examples/basics.k` and `examples/tick.k` (realistic trades & quotes,
  as-of and window joins, VWAP, 1-minute OHLCV bars).
- **`round[d;x]`** helper.
- **CI**: GitHub Actions builds and runs the test suite (now 97 assertions) on every push.

## 1.0
- Amber: a low-latency array language with a q/kdb+ vocabulary. Aggregations, dictionaries, tables, keyed tables,
  joins (`lj ij uj pj ej aj aj0 wj asof`), qSQL (`qwhere qselect qby fby xgroup ungroup`),
  strings, and **C-level sorted attributes** that turn `?`/`in` into O(log n) binary search.
