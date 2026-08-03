/* diagnostic.c  -  Amber Rust-style visual diagnostic error reports.
 * GNU AGPLv3 - see LICENSE and NOTICE.  See diagnostic.h for the contract. */
#include "diagnostic.h"
#include <stdio.h>
#include <string.h>

/* ---- ANSI SGR colours (only emitted when color != 0) --------------------- */
#define C_ERR  "\x1b[1;31m"  /* bold red    : error[CODE]        */
#define C_TTL  "\x1b[1m"     /* bold        : title              */
#define C_LOC  "\x1b[1;34m"  /* bold blue   : --> and gutter |   */
#define C_CAR  "\x1b[1;31m"  /* bold red    : ^^^ underlines     */
#define C_HLP  "\x1b[1;36m"  /* bold cyan   : = help             */
#define C_RST  "\x1b[0m"

/* ---- tiny bounded string builder (snprintf semantics) -------------------- */
typedef struct { char *buf; size_t cap; size_t len; } SB;

static void sb_put(SB *s, const char *t) {
    size_t n = strlen(t);
    for (size_t i = 0; i < n; i++) {
        if (s->len + 1 < s->cap) s->buf[s->len] = t[i];
        s->len++;
    }
}
static void sb_putc(SB *s, char c) {
    if (s->len + 1 < s->cap) s->buf[s->len] = c;
    s->len++;
}
static void sb_col(SB *s, int color, const char *code) { if (color) sb_put(s, code); }
static void sb_rst(SB *s, int color) { if (color) sb_put(s, C_RST); }
static void sb_uint(SB *s, unsigned v) {
    char t[16]; int i = 0;
    if (v == 0) t[i++] = '0';
    while (v) { t[i++] = (char)('0' + v % 10); v /= 10; }
    while (i) sb_putc(s, t[--i]);
}
static void sb_spaces(SB *s, unsigned n) { while (n--) sb_putc(s, ' '); }

static unsigned digits(unsigned v) { unsigned d = 1; while (v >= 10) { v /= 10; d++; } return d; }

Span span_at(const char *src, uint32_t start, uint32_t end) {
    Span sp; sp.src = src; sp.start = start; sp.end = end; sp.line = 1; sp.col = 1;
    uint32_t ls = 0;
    for (uint32_t i = 0; i < start && src && src[i]; i++)
        if (src[i] == '\n') { sp.line++; ls = i + 1; }
    sp.col = start - ls + 1;
    return sp;
}

size_t report_diagnostic(char *buf, size_t buflen,
                         const char *code, const char *title,
                         const char *file, Span primary,
                         const Span *secondary, size_t nsecondary,
                         const char *help, int color) {
    SB sb = { buf, buflen, 0 };
    if (!file)  file  = "<amber>";
    if (!title) title = "";
    if (!code)  code  = "E0000";

    /* locate the source line that carries the primary span */
    const char *src = primary.src ? primary.src : "";
    uint32_t ls = 0, i;
    for (i = 0; i < primary.start && src[i]; i++)
        if (src[i] == '\n') ls = i + 1;
    uint32_t le = ls;
    while (src[le] && src[le] != '\n') le++;      /* [ls,le) = line text */
    uint32_t linelen = le - ls;

    unsigned G = digits(primary.line);

    /* error[CODE]: TITLE */
    sb_col(&sb, color, C_ERR); sb_put(&sb, "error["); sb_put(&sb, code); sb_put(&sb, "]"); sb_rst(&sb, color);
    sb_col(&sb, color, C_TTL); sb_put(&sb, ": "); sb_put(&sb, title); sb_rst(&sb, color);
    sb_putc(&sb, '\n');

    /*   --> file:line:col   (G leading spaces) */
    sb_spaces(&sb, G);
    sb_col(&sb, color, C_LOC); sb_put(&sb, "--> "); sb_rst(&sb, color);
    sb_put(&sb, file); sb_putc(&sb, ':'); sb_uint(&sb, primary.line);
    sb_putc(&sb, ':'); sb_uint(&sb, primary.col); sb_putc(&sb, '\n');

    /* blank gutter bar:  (G+1 spaces)| */
    sb_spaces(&sb, G + 1); sb_col(&sb, color, C_LOC); sb_putc(&sb, '|'); sb_rst(&sb, color); sb_putc(&sb, '\n');

    /* NN | <source line> */
    sb_uint(&sb, primary.line); sb_putc(&sb, ' ');
    sb_col(&sb, color, C_LOC); sb_put(&sb, "| "); sb_rst(&sb, color);
    for (i = 0; i < linelen; i++) sb_putc(&sb, src[ls + i]);
    sb_putc(&sb, '\n');

    /*    | <carets>   built column-aligned with the source line */
    sb_spaces(&sb, G + 1); sb_col(&sb, color, C_LOC); sb_put(&sb, "| "); sb_rst(&sb, color);
    {
        /* rightmost caret column, so we know how far to sweep */
        uint32_t maxc = 0;
        Span all;
        size_t sidx;
        for (sidx = 0; sidx <= nsecondary; sidx++) {
            Span sp = (sidx == 0) ? primary : secondary[sidx - 1];
            if (sp.start < ls) continue;
            uint32_t c0 = sp.start - ls;
            uint32_t c1 = c0 + (sp.end > sp.start ? sp.end - sp.start : 1);
            if (c1 > maxc) maxc = c1;
        }
        (void)all;
        sb_col(&sb, color, C_CAR);
        for (uint32_t c = 0; c < maxc; c++) {
            int caret = 0;
            for (sidx = 0; sidx <= nsecondary; sidx++) {
                Span sp = (sidx == 0) ? primary : secondary[sidx - 1];
                if (sp.start < ls) continue;
                uint32_t c0 = sp.start - ls;
                uint32_t c1 = c0 + (sp.end > sp.start ? sp.end - sp.start : 1);
                if (c >= c0 && c < c1) { caret = 1; break; }
            }
            sb_putc(&sb, caret ? '^' : ' ');
        }
        sb_rst(&sb, color);
    }
    sb_putc(&sb, '\n');

    /* blank gutter bar + optional help note */
    sb_spaces(&sb, G + 1); sb_col(&sb, color, C_LOC); sb_putc(&sb, '|'); sb_rst(&sb, color); sb_putc(&sb, '\n');
    if (help && help[0]) {
        sb_spaces(&sb, G + 1);
        sb_col(&sb, color, C_HLP); sb_put(&sb, "= help: "); sb_put(&sb, help); sb_rst(&sb, color);
        sb_putc(&sb, '\n');
    }

    if (sb.cap) sb.buf[sb.len < sb.cap ? sb.len : sb.cap - 1] = '\0';
    return sb.len;
}

void report_diagnostic_stderr(const char *code, const char *title,
                              const char *file, Span primary,
                              const Span *secondary, size_t nsecondary,
                              const char *help) {
    char buf[4096];
    report_diagnostic(buf, sizeof buf, code, title, file, primary,
                      secondary, nsecondary, help, 1);
    fputs(buf, stderr);
}
