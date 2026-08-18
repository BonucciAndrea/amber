/* ln.h  -  Amber 1.9.5 native line editor.
 *
 * A single-file, zero-dependency line editor in the linenoise tradition (raw
 * termios, one visible line, ANSI refresh) wired directly into Amber's REPL
 * read path.  Up to 1.9.4 the REPL read its input with a bare read(2) on a
 * 256-byte buffer: no editing, no history, no completion.  The usual answer
 * was to start Amber under `rlwrap`, and that is the bug this file closes.
 *
 * WHY IT REPLACES rlwrap
 * ----------------------
 * rlwrap runs the wrapped program on a pty and speaks readline on the user's
 * behalf.  That works only for a program that reads whole lines in canonical
 * mode.  The moment the program takes the terminal into raw / non-canonical
 * mode and reads single keypresses, rlwrap is doing nothing useful and says so
 * -- across stdout and stderr, in the middle of your session:
 *
 *     rlwrap: warning: rlwrap appears to do nothing for amber, which asks
 *     for single keypresses all the time ...
 *
 * and, worse, the two editors fight over the same cursor and the redraw is
 * garbled.  With this file in the build there is no reason to run rlwrap at
 * all: editing, history and completion are native, the terminal is put in raw
 * mode for exactly the duration of one line, and the original termios is
 * restored on every exit path -- normal return, EOF, ^C and atexit().
 *
 * ...AND IF SOMEONE RUNS IT UNDER rlwrap ANYWAY
 * ---------------------------------------------
 * They will: a shell alias, a wrapper script, a tmux or .desktop launcher, or
 * simply the habit the pre-1.9.5 docs taught.  Keeping the fix in the ./a
 * launcher would only ever help the one launch route the launcher controls, so
 * the check lives HERE instead.  am_ln_under_rlwrap() spots a parent rlwrap (or
 * rlfe) by process lineage -- rlwrap exports no environment variable to its
 * child -- and when it does, am_ln_interactive() returns 0 and Amber reads
 * whole lines in canonical mode, exactly as 1.9.4 did.
 *
 * Standing down is the right answer rather than a concession: rlwrap's warning
 * is correct, two line editors cannot share one cursor, and the user under
 * rlwrap still gets full readline editing and history.  Nothing is lost, the
 * warning cannot be printed, and the redraw cannot garble -- on ANY launch
 * route, not just via ./a.
 *
 * WHAT IT GUARANTEES
 * ------------------
 *   * When stdin/stdout are not a terminal (a pipe, a here-doc, a CI run), or
 *     TERM is dumb, or AMBER_NO_EDIT is set, am_readline() degrades to an
 *     unedited line read.  Batch behaviour is byte-for-byte what it always was.
 *   * Nothing is allocated on the keystroke path beyond the line buffer.
 *   * No curses, no readline, no terminfo: plain C99 + POSIX termios/ioctl.
 *
 * COMPLETION
 * ----------
 * Tab is served, in order, from:
 *   0. an extension (src/ext.h, am_ext_complete) when one is installed;
 *   1. globals in the workspace, table column names, `\` commands and the
 *      Amber/K verb vocabulary;
 *   2. lines already entered in this session (history).
 * If every source comes up empty the editor offers an extension hint
 * (am_ext_hint) as dim ghost text, which is NEVER inserted until the user
 * presses Tab, Right or Ctrl-F again.  With no extension installed there is no
 * hint, no network and no third phase -- the core is entirely local.
 *
 * The public entry point is am_repl_readline(); src/lnk.c exposes it to the
 * interpreter as the `rdl verb, which repl.k uses instead of a raw read, and
 * src/m.c's bare rep() loop uses it directly.
 *
 * Amber - GNU AGPLv3 - see LICENSE and NOTICE.
 */
#ifndef AMBER_LN_H
#define AMBER_LN_H

#include <stddef.h>

/* ---- completion plumbing (linenoise-compatible shape) -------------------- */
typedef struct amCompletions {
    size_t   len;
    char   **cvec;
} amCompletions;

typedef void (amCompletionCallback)(const char *buf, amCompletions *lc);

void  am_ln_set_completion_callback(amCompletionCallback *fn);
void  am_ln_add_completion(amCompletions *lc, const char *str);

/* Familiar linenoise spellings, for readers coming from that codebase. */
#define linenoiseCompletions            amCompletions
#define linenoiseSetCompletionCallback  am_ln_set_completion_callback
#define linenoiseAddCompletion          am_ln_add_completion
#define linenoise(p)                    am_ln_readline(p)

/* ---- editor -------------------------------------------------------------- */
/* Is the editor usable right now?  True only when stdin+stdout are a non-dumb
 * terminal, AMBER_NO_EDIT is unset, and we are not running inside rlwrap. */
int   am_ln_interactive(void);

/* Were we started under rlwrap / rlfe?  Determined once, by walking up to four
 * ancestor processes (Linux /proc, macOS sysctl, elsewhere one `ps`), and only
 * ever consulted for a session that would otherwise enter raw mode.
 *   AMBER_RLWRAP=1  force yes -- stand down for a wrapper we did not recognise
 *   AMBER_RLWRAP=0  force no  -- keep Amber's editor under rlwrap regardless
 * On a platform whose lineage we cannot read this answers 0, i.e. behave
 * exactly as before. */
int   am_ln_under_rlwrap(void);

/* Read one line.  Returns a malloc'd, NUL-terminated string WITHOUT the
 * trailing newline, or NULL at end-of-input.  `prompt` may be NULL. */
char *am_ln_readline(const char *prompt);

void  am_ln_history_add(const char *line);
int   am_ln_history_load(const char *path);
int   am_ln_history_save(const char *path);
void  am_ln_history_set_max(int n);
/* Read-only view of this session's history, oldest first (for extensions and
 * for the completion sources).  *n receives the count. */
const char *const *am_ln_history(int *n);

/* ---- REPL integration ---------------------------------------------------- */
/* Read one console line for the interpreter: prints `prompt`, edits when
 * interactive, and always returns a malloc'd buffer that INCLUDES the trailing
 * "\n" (which is what Amber's console-read contract expects), or NULL at EOF. */
char *am_repl_getline(const char *prompt, size_t *len);

/* Install the REPL's completion callback + load history.  Idempotent. */
void  am_repl_init(void);

/* ---- provided by src/m.c (needs the interpreter's global table) ---------- */
/* NUL-separated list of global names, terminated by an extra NUL.  Returns the
 * number of names written. */
unsigned am_globals(char *dst, size_t cap);
/* One-line schema digest of the workspace, e.g.
 *   "trades:table(time sym px size) q:table(...) n:LongVector[1000000]"
 * Truncated to fit `cap`; always NUL-terminated. */
void     am_schema_brief(char *dst, size_t cap);

#endif /* AMBER_LN_H */
