/* trace.c  -  Amber execution profiler ("\trace <expression>"). See trace.h.
 *
 * Wraps the normal parse -> compile -> run -> print pipeline with
 * CLOCK_MONOTONIC timestamps. Nothing here modifies pk()/cpl()/run()/out()
 * or arena.c -- it only calls them, in exactly the sequence evs1() (m.c)
 * already uses for a normal input line, with a clock read between each
 * step.
 *
 * Feature-test macro must be defined before any system header (including
 * a.h's own <unistd.h>) is pulled in, or a strict `-std=c99` build hides
 * clock_gettime()/CLOCK_MONOTONIC/struct timespec entirely. */
/* ---- portability preamble: MUST precede every system header in this TU ----
 * Defining _POSIX_C_SOURCE puts Darwin's headers into STRICT POSIX mode, which
 * hides the BSD extensions this file relies on -- MAP_ANON above all others.
 * That is exactly the macOS CI failure: source that compiles clean against
 * glibc fails on Apple clang with "MAP_ANON undeclared here". _DARWIN_C_SOURCE
 * puts those declarations back; _GNU_SOURCE and _DEFAULT_SOURCE do the
 * equivalent job on glibc/musl. All three are purely ADDITIVE -- they only ever
 * unhide declarations, so none of them can change behaviour. (Verified: this
 * tree calls no function whose semantics _GNU_SOURCE alters, i.e. no
 * strerror_r, basename or qsort_r.)
 * These must sit above the first #include of the translation unit, not merely
 * above <sys/mman.h>: any system header may pull in <features.h> first and
 * latch the mode for the whole compilation. */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif
#ifndef _DARWIN_C_SOURCE
#define _DARWIN_C_SOURCE
#endif
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#include "a.h"
#include "trace.h"
#include "arena.h"
#include "fmtutil.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

#define BAR_WIDTH 20
#define BLOCK "\xe2\x96\xa0" /* U+25A0 BLACK SQUARE, i.e. "■" */

static long ns_between(struct timespec a, struct timespec b) {
    return (b.tv_sec - a.tv_sec) * 1000000000L + (b.tv_nsec - a.tv_nsec);
}

/* Render one phase's [■■■□□...] bar into buf (must hold BAR_WIDTH*3+1 bytes
 * for the multi-byte block glyphs, plus a NUL). */
static void render_bar(double frac, char *buf, size_t buflen) {
    if (frac < 0) frac = 0;
    if (frac > 1) frac = 1;
    int filled = (int)(frac * BAR_WIDTH + 0.5);
    size_t off = 0;
    for (int i = 0; i < BAR_WIDTH && off + 4 < buflen; i++) {
        if (i < filled) { memcpy(buf + off, BLOCK, 3); off += 3; }
        else buf[off++] = ' ';
    }
    buf[off] = 0;
}

/* Format a nanosecond duration as e.g. "142us" or "3.20ms". */
static void fmt_ns(long ns, char *buf, size_t buflen) {
    if      (ns >= 1000000L) snprintf(buf, buflen, "%.2fms", ns / 1e6);
    else if (ns >= 1000L)    snprintf(buf, buflen, "%.1fus", ns / 1e3);
    else                     snprintf(buf, buflen, "%ldns", ns);
}

/* Every row is composed to exactly INNER printable columns before the closing
 * "|" is added. The bar glyph is 3 bytes but 1 column wide, so the previous
 * plain printf() field widths counted bytes and left the right-hand border
 * ragged; RULE/INNER keep the box square. */
#define INNER 55

static void print_row(const char *body, int cols) {
    printf("| %s%*s |\n", body, cols < INNER - 2 ? (INNER - 2) - cols : 0, "");
}

static void print_rule(void) {
    char r[INNER + 3];
    r[0] = '+';
    for (int i = 1; i <= INNER; i++) r[i] = '-';
    r[INNER + 1] = '+'; r[INNER + 2] = 0;
    printf("%s\n", r);
}

static void print_phase(const char *name, long ns, long total_ns) {
    char t[24], bar[BAR_WIDTH * 3 + 1], line[160];
    fmt_ns(ns, t, sizeof t);
    render_bar(total_ns > 0 ? (double)ns / (double)total_ns : 0, bar, sizeof bar);
    snprintf(line, sizeof line, "%-9s %9s  [%s] %5.1f%%",
             name, t, bar, total_ns > 0 ? 100.0 * ns / total_ns : 0.0);
    /* printable columns: name9 sp time9 sp2 '[' bar ']' sp pct6 */
    print_row(line, 9 + 1 + 9 + 2 + 1 + BAR_WIDTH + 1 + 1 + 6);
}

static void print_report(const TraceMetrics *m) {
    print_rule();
    print_phase("Parse",   m->parse_ns, m->total_ns);
    print_phase("Arena",   m->arena_ns, m->total_ns);
    print_phase("Execute", m->exec_ns,  m->total_ns);
    print_phase("Format",  m->fmt_ns,   m->total_ns);
    print_rule();

    char tot[16], mem[16], line[160];
    fmt_ns(m->total_ns, tot, sizeof tot);
    fmt_bytes((double)m->arena_peak, mem, sizeof mem);
    snprintf(line, sizeof line, "Total: %-10s Arena peak: %-10s", tot, mem);
    print_row(line, (int)strlen(line));
    print_rule();
}

A trace_cmd(S s) {
    TraceMetrics m; memset(&m, 0, sizeof m);
    struct timespec t0, t1, t2, t3, t4;

    /* Rewrite `select ... from t`-style SQL sugar to plain K first (if
     * qsql.k is loaded), exactly like the interactive REPL does -- so both
     * the trace and the printed result cover the real, table-aware
     * expression instead of failing to parse the English sugar. */
    C rbuf[512];
    S input = try_rewrite(s, rbuf, sizeof rbuf);

    S p = input;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    A parsed = pk(&p, 10);                         /* a) parse / AST gen   */
    clock_gettime(CLOCK_MONOTONIC, &t1);
    if (!parsed) { printf("\\trace: parse error\n"); fflush(stdout); return au; }

    arena_reset();
    arena_reset_peak();          /* start the high-water gauge from here */
    m.arena_before = arena_used();
    clock_gettime(CLOCK_MONOTONIC, &t2);            /* b) arena setup       */

    A compiled = cpl(aCm(input, p), parsed, 0);
    if (!compiled) { printf("\\trace: compile error\n"); fflush(stdout); arena_reset(); return au; }
    A result = run(compiled, 0, 0);                 /* c) engine execution  */
    clock_gettime(CLOCK_MONOTONIC, &t3);
    mr(compiled);                                    /* release the closure  */
    m.arena_after = arena_used();

    if (result) out(result);                        /* d) result rendering  */
    clock_gettime(CLOCK_MONOTONIC, &t4);

    if (result) mr(result);
    arena_reset();

    m.parse_ns = ns_between(t0, t1);
    m.arena_ns = ns_between(t1, t2);
    m.exec_ns  = ns_between(t2, t3);
    m.fmt_ns   = ns_between(t3, t4);
    m.total_ns = m.parse_ns + m.arena_ns + m.exec_ns + m.fmt_ns;
    /* Every arena consumer (ajc/wjc/csv/ast) rewinds the slab before it
     * returns, so both `arena_before` and `arena_after` are ~always 0 and the
     * old max() of the two reported "0 B" for literally every expression.
     * arena_peak() is a true high-water mark maintained inside arena.c and
     * survives arena_reset(), so it reports what the evaluation really used. */
    m.arena_peak = arena_peak();
    if (m.arena_peak < m.arena_after) m.arena_peak = m.arena_after;

    print_report(&m);
    /* Flush: print_report() emits the timing box via C stdio (printf), which on
     * a non-line-buffered stdout (macOS in the status-bar box) would otherwise
     * sit in the buffer until a later command flushes it -- so the report only
     * appeared after a subsequent \disasm.  The result above went out via out()
     * (raw write), so flushing here keeps result-then-report ordering.  Mirrors
     * vm_disasm_cmd() / ast_cmd(). */
    fflush(stdout);
    return au;
}
