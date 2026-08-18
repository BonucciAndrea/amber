/* ext.h  -  Amber 1.9.5 extension seam (ABI 1).
 *
 * The engine in this repository is complete on its own: it has no AI code, no
 * network code and no optional feature that is switched off.  What it does
 * have is one small, neutral, documented seam so that an out-of-tree package
 * (for example the separate `amber-ai` repository) can add verbs, REPL
 * commands and editor behaviour by dropping .c files into `ext/` and running
 * ./build.sh again -- WITHOUT patching a single line of src/.
 *
 * That property is the whole point.  A user who installs an extension and
 * later pulls a new Amber release does not get a merge conflict, and a user
 * who never installs one carries no cost: every hook below is a null pointer
 * and every call site is a predictable, correctly-predicted branch on a code
 * path that already touches the terminal or the parser.
 *
 * ---- how an extension registers -------------------------------------------
 *
 *   #include "a.h"        // the interpreter's value type A
 *   #include "ext.h"
 *
 *   static A my_verb(A x) { ... }
 *   static int my_hint(const char *line, size_t len, char *dst, size_t cap);
 *
 *   __attribute__((constructor))
 *   static void my_register(void) {
 *       am_ext_verb("foo", (void *)my_verb);   //  `foo x   in Amber
 *       am_ext_hint = my_hint;                 //  ghost text in the editor
 *   }
 *
 * The constructor runs before main(), because ext/*.o is linked directly into
 * the executable (never as an archive member), so there is no initialisation
 * order to arrange and no registration call to add to src/.
 *
 * ---- ABI ------------------------------------------------------------------
 *
 * AMBER_EXT_ABI is bumped whenever an existing hook changes shape.  An
 * installer should check it (`grep AMBER_EXT_ABI src/ext.h`) before copying
 * files in.  Adding a NEW hook does not bump it.
 *
 * Amber - GNU AGPLv3 - see LICENSE and NOTICE.
 */
#ifndef AMBER_EXT_H
#define AMBER_EXT_H

#include <stddef.h>

#define AMBER_EXT_ABI 1

#ifdef __cplusplus
extern "C" {
#endif

/* ---- 1. verb registry ----------------------------------------------------
 *
 * Amber's backtick verbs (`t, `hex, `simd, ...) live in a fixed table in
 * src/a.c that is indexed by the symbol's own 4-byte packed value.  An
 * extension cannot edit that table, so sym1() consults this registry FIRST and
 * falls through to the built-in table when nothing is registered.  Names are
 * therefore at most four characters, exactly like the built-ins, and a
 * registered name shadows a built-in of the same spelling (which is how an
 * extension may replace behaviour rather than only add to it).
 *
 * `fn` must have the interpreter's monadic signature, A (*)(A), i.e. it takes
 * one reference and consumes it.  It is passed as void* purely so that this
 * header stays free of src/a.h -- extensions include both anyway.
 */
int   am_ext_verb(const char *name, void *fn);   /* 0 = registered, -1 = no */
void *am_ext_verb_lookup(int packed);            /* used by src/a.c sym1()  */
int   am_ext_verb_count(void);

/* ---- 2. startup ----------------------------------------------------------
 * Called once, lazily, the first time the REPL reads a line (am_repl_init()).
 * An extension uses it for whatever it must not do from a constructor -- read
 * the environment, open a file, warm a cache.
 */
extern void (*am_ext_startup)(void);
void am_ext_startup_once(void);                  /* idempotent              */

/* ---- 3. line editor ------------------------------------------------------
 *
 * am_ext_hint: offered a line and asked for an inline continuation.  Write at
 *   most cap-1 bytes plus NUL into dst and return 1; return 0 for "nothing".
 *   The editor calls this ONLY when the cursor is at end of line and every
 *   lexical completion source has already come up empty, and it renders the
 *   result as dim ghost text that is never inserted until the user explicitly
 *   accepts it with Tab / Right / Ctrl-F.  It MUST return quickly: it runs on
 *   the keystroke path, so anything that can block belongs behind a deadline
 *   inside the extension.
 *
 * am_ext_complete / am_ext_complete_late: extra sources of ordinary Tab
 *   candidates.  Completion layers, so there are two of them and the order is
 *   the point:
 *
 *     am_ext_complete       runs BEFORE the built-in lexical sources, and is
 *                           for syntax the engine cannot know about -- an
 *                           extension's own `\` command and its sub-commands.
 *                           If it produces candidates, nothing else is asked.
 *     am_ext_complete_late  runs AFTER globals, columns, vocabulary and
 *                           history have all come up empty, and before the
 *                           hint below.  It is for wide, fuzzy sources (a
 *                           recall file, a project index) that must never
 *                           shadow the name of a variable actually in scope.
 *
 *   `completions` is an amCompletions* (src/ln.h); add to it with
 *   am_ln_add_completion().  void* again only to keep this header standalone.
 */
extern int  (*am_ext_hint)(const char *line, size_t len, char *dst, size_t cap);
extern void (*am_ext_complete)(const char *line, void *completions);
extern void (*am_ext_complete_late)(const char *line, void *completions);

/* The line as it stood when the user ACCEPTED a ghost suggestion, or "" when
 * there is none.  Reading it clears it, so an extension can drain it once per
 * line and treat the acceptance as feedback. */
void        am_ext_set_accepted(const char *line);
const char *am_repl_take_accepted(void);

/* ---- 4. REPL commands ----------------------------------------------------
 * Consulted by src/m.c's backslash dispatcher BEFORE the "run it as a shell
 * command" fallback, with the text after the backslash (e.g. "ai why x").
 * Return 0 for "not mine, carry on"; otherwise return an Amber value (an `A`,
 * typed here as unsigned long long so this header needs no src/a.h) -- use the
 * generic null `au` when the command has only printed something.
 */
extern unsigned long long (*am_ext_bs)(const char *line);

/* ---- 5. cosmetics --------------------------------------------------------
 * Appended to `amber --help` (am_ext_usage) and shown in the REPL banner
 * (am_ext_banner, e.g. " [amber-ai 1.0.0]").  Both NULL by default; both must
 * point at storage that outlives the process, i.e. a string literal.
 */
extern const char *am_ext_usage;
extern const char *am_ext_banner;

#ifdef __cplusplus
}
#endif

#endif /* AMBER_EXT_H */
