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
    if (ns >= 1000000L) snprintf(buf, buflen, "%.2fms", ns / 1e6);
    else                snprintf(buf, buflen, "%ldus", ns / 1000);
}

static void print_phase(const char *name, long ns, long total_ns) {
    char t[24], bar[BAR_WIDTH * 3 + 1], pct[16];
    fmt_ns(ns, t, sizeof t);
    render_bar(total_ns > 0 ? (double)ns / (double)total_ns : 0, bar, sizeof bar);
    snprintf(pct, sizeof pct, "%5.1f%%", total_ns > 0 ? 100.0 * ns / total_ns : 0.0);
    printf("| %-9s %8s  [%s] %s |\n", name, t, bar, pct);
}

static void print_report(const TraceMetrics *m) {
    printf("+-------------------------------------------------------+\n");
    print_phase("Parse",   m->parse_ns, m->total_ns);
    print_phase("Arena",   m->arena_ns, m->total_ns);
    print_phase("Execute", m->exec_ns,  m->total_ns);
    print_phase("Format",  m->fmt_ns,   m->total_ns);
    printf("+-------------------------------------------------------+\n");

    char tot[16], mem[16];
    fmt_ns(m->total_ns, tot, sizeof tot);
    fmt_bytes((double)m->arena_peak, mem, sizeof mem);
    printf("| Total: %-8s  Arena peak: %-8s                  |\n", tot, mem);
    printf("+-------------------------------------------------------+\n");
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
    if (!parsed) { printf("\\trace: parse error\n"); return au; }

    arena_reset();
    m.arena_before = arena_used();
    clock_gettime(CLOCK_MONOTONIC, &t2);            /* b) arena setup       */

    A compiled = cpl(aCm(input, p), parsed, 0);
    if (!compiled) { printf("\\trace: compile error\n"); return au; }
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
    m.arena_peak = m.arena_after > m.arena_before ? m.arena_after : m.arena_before;

    print_report(&m);
    return au;
}
