# The Amber REPL

Line editing, the diagnostic verbs (``, `st`, `	race`), the error report
format, and the table grid modes.

Moved out of [`README.md`](../README.md) so the landing page stays a landing
page. Nothing here changed in the move.

---

<a name="repl-line-editing"></a>
## REPL line editing (native; do not use `rlwrap`)

Amber's REPL edits your line itself. `src/ln.c` is a single-file editor (~1,650 lines of C99 + POSIX
`termios`) in the [linenoise](https://github.com/antirez/linenoise) tradition: raw `termios`, one
visible line, ANSI refresh. No readline, no curses, no terminfo, and nothing allocated on the
keystroke path beyond the line buffer. **`rlwrap` is no longer needed, and must no longer be used.**

| key | |
|---|---|
| `←` `→`, `Ctrl-B` / `Ctrl-F` | move by character, and a *character* rather than a byte: `à`, `日` and `😀` step as one |
| `Ctrl-A` / `Ctrl-E`, `Home` / `End` | start / end of line |
| `↑` `↓`, `Ctrl-P` / `Ctrl-N` | history (persisted in `~/.amber_history`) |
| `Backspace`, `Del`, `Ctrl-W` / `Ctrl-U` / `Ctrl-K` | delete char / word / to start / to end |
| `Ctrl-L` | clear screen, byte-identical to `\clear` |
| `Ctrl-C` · `Ctrl-D` | abandon the line (or a continuation) · exit on an empty line |
| `PgUp` / `PgDn`, wheel | scroll the transcript while the status bar stays locked |
| `\sb` | toggle the status bar |

`Tab` is a **no-op**. Tab completion existed in 1.9.5 and was removed at 2.0.0: with k's terse
syntax it was near-useless and uncomfortable to use.

**Multi-line input.** A line that leaves a bracket open (`{`, `(`, `[`) is an incomplete statement,
so the editor keeps reading on a `...>` prompt until the brackets balance, then evaluates the
joined statement as one, then stores it in history as one entry:

```text
amber> f:{
  ...> x+1
  ...> }
amber> f 41
42
```

There is no key for this. Shift-Enter is **indistinguishable from Enter** in a
terminal (both send CR) unless you opt into an extended keyboard protocol, so continuing on
incomplete syntax, which is what python, node, ghci and q all do, is the only portable mechanism.
`Ctrl-C` abandons a continuation. A bracketed paste of a multi-line function is rejoined by the
same string- and comment-aware rule, so a function typed by hand and one pasted produce identical
text; a bracket inside a string (`"a{b"`) or after a comment never triggers it.

**UTF-8.** The buffer holds bytes but a terminal lays out cells, and the editor now converts
between the two: accented Latin, CJK and emoji all measure as the cells drawn, Backspace
removes a whole character, and horizontal scrolling never cuts a sequence in half. The width table
is built in rather than taken from `wcwidth()`, which answers per `LC_CTYPE` and would otherwise
lay the same line out differently on macOS and WSL.

**Why `rlwrap` must not be used.** rlwrap runs the wrapped program on a pty and speaks readline on
its behalf, which only works for a program that reads whole lines in *canonical* mode. Amber puts
the terminal into raw / non-canonical mode and reads single keypresses, so rlwrap is doing nothing
useful. It says so, in the middle of your session, usually right after an error:

```text
rlwrap: warning: rlwrap appears to do nothing for amber, which asks for
single keypresses all the time. Don't you need --always-readline
and possibly --no-children? (cf. the rlwrap manpage)
```

while the two editors fight over one cursor and garble the redraw. `./a` therefore **execs the
interpreter directly**. The one case where rlwrap still buys something is when the native editor
is switched off on purpose (a dumb terminal, an editor subshell, a screen reader):

```sh
AMBER_NO_EDIT=1 ./a          # Amber reads whole lines; ./a wraps it in: rlwrap -n -a
AMBER_NO_RLWRAP=1 AMBER_NO_EDIT=1 ./a   # ... or not even that
```

`-n` (`--no-warnings`) and `-a` (`--always-readline`) are passed unconditionally on that path, so
**no rlwrap diagnostic can reach your session on any path**. The editor also degrades to a plain
line read whenever stdin/stdout are not a terminal, so `echo '2+2' | ./a`, here-docs and CI runs
behave byte-for-byte as they always did. `tests/test_repl_term.py` asserts all of this on a real
pty, including that the terminal's `termios` is restored exactly after a normal exit *and* after
`Ctrl-C`.

---

<a name="rust-style-diagnostics"></a>
## Rust-style diagnostics

By default an error prints the terse core message with a `^` caret under the failing token:

```
'length: operands have mismatched counts
 prices+sizes
       ^
```

Set **`AMBER_DIAG=1`** and the same errors are additionally rendered as a **Rust-compiler-style
report**: an `error[CODE]` line, a `-->` locator, a gutter-aligned source line, `^^^` underlines,
and a `= help:` note (all ANSI-coloured on a colour terminal):

```sh
AMBER_DIAG=1 ./amber                # interactive
AMBER_DIAG=1 ./amber myscript.k     # running a script
```

```text
error[E0104]: Vector length mismatch
  --> test.k:12:8
   |
12 |   prices + sizes
   |   ^^^^^^   ^^^^^
   |
   = help: Both vectors must have matching lengths for element-wise `+`.
```

The formatter lives in `src/diagnostic.{h,c}` (a `Span` source-tracking struct + a
`report_diagnostic()` renderer); the runtime error path (`src/e.c`) routes every parse / type /
domain error through it when `AMBER_DIAG` is set, so the flag is entirely opt-in and changes
nothing about the default output. You can exercise the formatter directly with the `` `dgn ``
self-test builtin (returns `1` when the rendered report matches its expected shape):

```q
`dgn 0        / 1  — diagnostic formatter self-test
```

Turn it on for a session and leave it: it costs one `getenv` per error and never fires on success.

<details>
<summary>Turning the report off at runtime</summary>

The diagnostic is rendered when the error is *created*, so code that catches an error with
`.[f;args;handler]` still sees it on stderr. That is switchable at runtime, which helps for anything
that provokes errors on purpose (a test suite, `protect`, a retry loop):

```
prev:`diag 0        / suppress the report, returns the previous setting (1)
.[{1+`a};,0;{"caught"}]     / no output at all
`diag prev          / restore
```

The compact `'type` caret line is not suppressed. It is buffered and still handed to the trap
handler and to `` `err``, so a caught error can always be inspected. `AMBER_DIAG=0` in the
environment still works and seeds the initial value.

</details>

---

<a name="repl-diagnostics-v--ast--trace"></a>
## REPL diagnostics (`\v` · `\ast` · `\trace`)

Three zero-dependency session commands for inspecting the workspace and the evaluator itself.
None of them touch `eval`/`arena`/core REPL behaviour; they only read state and print a report.

**`\v`, a rich workspace inspector.** Every currently-defined global as an ASCII table
(Name / Type / Shape·Length / Memory), with a recursive deep-memory-footprint walker so table
and nested-list sizes are real, not a shallow guess:

```
amber> t:([]a:1 2 3;b:10 20 30)
amber> \v
+-------------+---------------+----------------+---------+
| Name        | Type          | Shape / Length | Memory  |
+-------------+---------------+----------------+---------+
| repl.prompt | Char Vector   | 7              | 64 B    |
| repl.edesc  | Table         | 41 x 12        | 1.5 KB  |
| PAL         | List          | 14             | 1.1 KB  |
| ...         | ...           | ...            | ...     |
| t           | Table         | 3 x 2          | 320 B   |
+-------------+---------------+----------------+---------+
```

`\v` lists *every* global in scope, which after `repl.k`'s modules load includes the library's own
internal state (`repl.*`, `PAL`, `GB`, `OUNI`, …) alongside your own, so scan for the names you
defined, or `\d yourns` first to narrow the namespace.

<details>
<summary><code>\ast</code>: colour-coded parse tree (parse-only)</summary>

```
amber> \ast (1+2)*3-4
Root
└── Binary Op : * (Multiply)
    ├── Binary Op : + (Add)
    │   ├── Scalar : 1 (Int64)
    │   └── Scalar : 2 (Int64)
    └── Binary Op : - (Subtract)
        ├── Scalar : 3 (Int64)
        └── Scalar : 4 (Int64)
```

Every leaf carries its literal type (`Int64`, `Float64`, `Symbol`, `Char`, or a
`(TypeName Vector[len])` preview for a literal vector) instead of a generic placeholder, and
Amber's tacit forms get explicit labels: a lambda literal shows its real source text, a 2- or
3-verb train is an explicit **Hook**/**Fork**, and a curried/partial application (`1+`, `f[x;;z]`)
is an explicit **Projection** with a `Blank` node standing in for the omitted argument:

```
amber> \ast {x+1}[3]
Root
└── Apply : Apply
    ├── Lambda : {x+1} (Lambda)
    └── Scalar : 3 (Int64)

amber> \ast +/1 2 3
Root
└── Verb : +/ (Add Over/Reduce)
    └── Vector : 0x01 0x02 0x03 (Byte Vector[3])
```

(A lambda shows its *source text*, never its bytecode. `pk()` itself compiles `{...}` literals
eagerly at parse time, the one exception to "nothing is executed"; disassembling what it compiled
to is `\disasm`'s job, not `\ast`'s.) Verbs render bold cyan, adverbs bold magenta, numeric
scalars bright green, variables and symbols yellow, tree connectors dim gray; list literals,
statement blocks, and hook/fork labels get their own restrained accent colour.

</details>

<details>
<summary><code>\trace</code>: 4-phase execution profiler</summary>

**`\trace`** profiles parse → arena setup → execute → format, with a Unicode bar chart and the
arena's peak scratch usage for that one evaluation. It prints the expression's normal result
first, then the report (timings vary run to run):

```
amber> \trace (1+2)*3-4
-3
+-------------------------------------------------------+
| Parse         454ns  [■■                  ]  10.5%    |
| Arena          38ns  [                    ]   0.9%    |
| Execute       1.1us  [■■■■■               ]  24.8%    |
| Format        2.8us  [■■■■■■■■■■■■■       ]  63.8%    |
+-------------------------------------------------------+
| Total: 4.3us      Arena peak: 0 B                     |
+-------------------------------------------------------+
```

The timer prints `ns` / `us` / `ms` as appropriate, and **Arena peak** is a true high-water mark
from `arena_peak()`. Only expressions that reach an arena-backed kernel (`aj`, `wj`,
`\ast`) report a non-zero peak:

```
amber> \trace aj[`s`t;tr;qt]
...
| Total: 335.4us    Arena peak: 32 B                    |
+-------------------------------------------------------+
```

`\trace` runs the same `select … by … from … where …` rewrite the interactive prompt uses, so
tracing a table expression or a bare qSQL query renders and profiles correctly:

```
amber> \trace select from t where a>1
+`a`b!(2 3;20 30)
+-------------------------------------------------------+
| Parse          2us  [                    ]   0.9% |
| Arena          3us  [                    ]   1.3% |
| Execute      212us  [■■■■■■■■■■■■■■■     ]  76.7% |
| Format        58us  [■■■■                ]  21.1% |
+-------------------------------------------------------+
| Total: 276us     Arena peak: 0 B                       |
+-------------------------------------------------------+
```

</details>

---

<a name="grid-modes"></a>
## Grid modes (`\grid`)

`\grid clean|rounded|sharp|heavy` sets the table frame. `clean` (default) is a minimal dashed
header rule; `rounded`/`sharp`/`heavy` draw a full box with curved / square / thick corners:

```
\grid rounded
([]a:1 2; b:3 4)
╭───┬───╮
│ a │ b │
├───┼───┤
│ 1 │ 3 │
│ 2 │ 4 │
╰───┴───╯
[2 rows x 2 cols]
```

Borders are dimmed so colourised cells stay the focus; `\clear` clears the screen. Grids also
carry **ANSI syntax highlighting** (a 14-hue per-column palette; `COLOR:0` to disable), right-aligned
numeric/temporal columns, and a `PREC`-capped float precision (default 7 decimals).
