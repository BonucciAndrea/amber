/* trace.h  -  Amber execution profiler ("\trace <expression>").
 * GNU AGPLv3 - see LICENSE and NOTICE.
 *
 * A REPL-side timing wrapper around Amber's normal parse -> compile -> run
 * -> print pipeline (pk() / cpl() / run() / out(), see p.c/b.c/m.c). It does
 * not modify the evaluator itself -- it just brackets each stage with
 * clock_gettime(CLOCK_MONOTONIC, ...) and samples the arena allocator
 * (arena.h) around it. All timing/formatting logic lives here and in
 * trace.c; the four pipeline calls it wraps are otherwise untouched.
 */
#ifndef AMBER_TRACE_H
#define AMBER_TRACE_H

/* Requires a.h to already be included by the translation unit (for the `A`
 * K-value type and pk()/cpl()/run()/out() etc). Not included here directly
 * because a.h has no #include guard and is meant to be pulled in exactly
 * once per .c file. */

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    long   parse_ns;   /* phase a: pk()               -- parse / AST gen   */
    long   arena_ns;   /* phase b: arena_reset() setup -- scratch allocator */
    long   exec_ns;    /* phase c: cpl() + run()      -- engine execution   */
    long   fmt_ns;     /* phase d: out()              -- result rendering  */
    long   total_ns;

    size_t arena_before;  /* arena_used() before execution                 */
    size_t arena_after;   /* arena_used() right after execution            */
    size_t arena_peak;    /* max(arena_before, arena_after) -- see trace.c  */
} TraceMetrics;

/* Defined in m.c (needs file-local access to the global symbol table):
 * runs `raw` through the K-level `qrw` SQL-syntax rewriter if it's
 * currently defined, copying the result into `buf` (size `n`); falls back
 * to `raw` unchanged if qrw isn't loaded or the rewrite fails. */
S try_rewrite(S raw, C *buf, N n);

/* The `\trace <expression>` REPL command handler: parses, compiles, runs
 * and prints `s` exactly as a normal input line would (after first running
 * it through try_rewrite(), so SQL-style queries and table results trace
 * and render correctly), but times each of the four phases and -- after
 * printing the normal result -- prints a breakdown table with a Unicode
 * block-bar showing the relative time each phase took. Always returns the
 * K "unit" value `au`. */
A trace_cmd(S s);

#ifdef __cplusplus
}
#endif

#endif /* AMBER_TRACE_H */
