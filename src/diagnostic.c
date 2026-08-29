/* diagnostic.c  -  Amber Rust-style visual diagnostic error reports.
 * GNU AGPLv3 - see LICENSE and NOTICE.  See diagnostic.h for the contract.
 *
 * Layout, and why each piece is shaped the way it is:
 *
 *   error[E0101]: Undefined variable `r`
 *    --> <amber>:1:3
 *     |
 *   1 | y:r+1
 *     |   ^ not found in this scope
 *     |
 *     = help: Verify that the variable is defined in the current scope.
 *
 *   * The gutter width is derived from the widest line number actually shown
 *     and every row is padded to it, so the `|` bars form one unbroken vertical
 *     rule no matter whether the error is on line 3 or line 3000.
 *   * Line numbers are right-aligned and dimmed: they are navigation, not
 *     content, and should not compete with the source text.
 *   * The primary span is underlined with `^` in bright red; secondary spans
 *     use `~` in blue so context reads as context. Both span the FULL token
 *     range, which is why the caller widens a bare offset to a token before
 *     calling (see eS in e.c) -- a lone `^` under a five-character name tells
 *     you far less than `^^^^^`.
 *   * The label sits inline at the end of the underline row, rustc-style, so
 *     the explanation is on the same visual line as the thing it explains.
 */
#include "diagnostic.h"
#include "ansi.h"
#include <stdio.h>
#include <string.h>

#define C_ERR  ANSI_ERR
#define C_TTL  ANSI_TTL
#define C_LOC  ANSI_LOC
#define C_CAR  ANSI_CAR
#define C_SEC  ANSI_SEC
#define C_NUM  ANSI_NUM
#define C_LBL  ANSI_WARN
#define C_HLP  ANSI_HLP
#define C_NOT  ANSI_NOTE
#define C_RST  ANSI_RST

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

/* Gutter, laid out exactly as rustc does it so the vertical rule is unbroken:
 *   numbered row :  <line right-aligned in G> " |"
 *   blank row    :  <G+1 spaces>              "|"
 * Both put the bar at column G+1, and the `-->` locator sits one column left
 * of it, which is what makes the whole block read as a single figure. */
static void gutter(SB *s, int color, unsigned G, unsigned line, int show) {
    if (show) {
        sb_spaces(s, G - digits(line));
        sb_col(s, color, C_NUM); sb_uint(s, line); sb_rst(s, color);
        sb_putc(s, ' ');
    } else {
        sb_spaces(s, G + 1);
    }
    sb_col(s, color, C_LOC); sb_putc(s, '|'); sb_rst(s, color);
}

Span span_at(const char *src, uint32_t start, uint32_t end) {
    Span sp; sp.src = src; sp.start = start; sp.end = end; sp.line = 1; sp.col = 1;
    uint32_t ls = 0;
    for (uint32_t i = 0; i < start && src && src[i]; i++)
        if (src[i] == '\n') { sp.line++; ls = i + 1; }
    sp.col = start - ls + 1;
    return sp;
}

size_t report_diagnostic_ex(char *buf, size_t buflen,
                            const char *code, const char *title,
                            const char *file, Span primary, const char *label,
                            const Span *secondary, size_t nsecondary,
                            const char *help, const char *note, int color) {
    SB sb = { buf, buflen, 0 };
    if (!file)  file  = "<amber>";
    if (!title) title = "";
    if (!code)  code  = "E0000";

    const char *src = primary.src ? primary.src : "";
    uint32_t ls = 0, i;
    for (i = 0; i < primary.start && src[i]; i++)
        if (src[i] == '\n') ls = i + 1;
    uint32_t le = ls;
    while (src[le] && src[le] != '\n') le++;      /* [ls,le) = the shown line */
    uint32_t linelen = le - ls;

    unsigned G = digits(primary.line);

    /* error[CODE]: TITLE */
    sb_col(&sb, color, C_ERR); sb_put(&sb, "error["); sb_put(&sb, code); sb_putc(&sb, ']');
    sb_rst(&sb, color);
    sb_col(&sb, color, C_TTL); sb_put(&sb, ": "); sb_put(&sb, title); sb_rst(&sb, color);
    sb_putc(&sb, '\n');

    /*  --> file:line:col   (aligned one column left of the gutter bar) */
    sb_spaces(&sb, G);
    sb_col(&sb, color, C_LOC); sb_put(&sb, "--> "); sb_rst(&sb, color);
    sb_put(&sb, file); sb_putc(&sb, ':'); sb_uint(&sb, primary.line);
    sb_putc(&sb, ':'); sb_uint(&sb, primary.col); sb_putc(&sb, '\n');

    gutter(&sb, color, G, 0, 0); sb_putc(&sb, '\n');

    /* NN | <source line> */
    gutter(&sb, color, G, primary.line, 1);
    sb_putc(&sb, ' ');
    for (i = 0; i < linelen; i++) sb_putc(&sb, src[ls + i]);
    sb_putc(&sb, '\n');

    /*    | <underlines> <label> */
    gutter(&sb, color, G, 0, 0);
    sb_putc(&sb, ' ');
    {
        uint32_t maxc = 0, c;
        size_t sidx;
        for (sidx = 0; sidx <= nsecondary; sidx++) {
            Span sp = (sidx == 0) ? primary : secondary[sidx - 1];
            if (sp.start < ls) continue;
            uint32_t c0 = sp.start - ls;
            uint32_t c1 = c0 + (sp.end > sp.start ? sp.end - sp.start : 1);
            if (c1 > maxc) maxc = c1;
        }
        /* One pass per column; primary wins over secondary where they
         * overlap so the fault site is never masked by a context span.
         * The SGR code is emitted once per run rather than once per glyph --
         * a 6-character underline was previously 6 colour+reset pairs, which
         * bloated captured output and made the raw text unreadable in logs. */
        int cur = 0;                       /* 0=blank 1=primary 2=secondary */
        for (c = 0; c < maxc; c++) {
            int kind = 0;
            uint32_t c0 = primary.start >= ls ? primary.start - ls : 0;
            uint32_t c1 = c0 + (primary.end > primary.start ? primary.end - primary.start : 1);
            if (primary.start >= ls && c >= c0 && c < c1) kind = 1;
            if (!kind) for (sidx = 0; sidx < nsecondary; sidx++) {
                Span sp = secondary[sidx];
                if (sp.start < ls) continue;
                uint32_t d0 = sp.start - ls;
                uint32_t d1 = d0 + (sp.end > sp.start ? sp.end - sp.start : 1);
                if (c >= d0 && c < d1) { kind = 2; break; }
            }
            if (kind != cur) {
                if (cur) sb_rst(&sb, color);
                if (kind == 1)      sb_col(&sb, color, C_CAR);
                else if (kind == 2) sb_col(&sb, color, C_SEC);
                cur = kind;
            }
            sb_putc(&sb, kind == 1 ? '^' : kind == 2 ? '~' : ' ');
        }
        if (cur) sb_rst(&sb, color);
        if (label && label[0]) {
            sb_putc(&sb, ' ');
            sb_col(&sb, color, C_LBL); sb_put(&sb, label); sb_rst(&sb, color);
        }
    }
    sb_putc(&sb, '\n');

    gutter(&sb, color, G, 0, 0); sb_putc(&sb, '\n');

    if (help && help[0]) {
        sb_spaces(&sb, G + 1);
        sb_col(&sb, color, C_HLP); sb_put(&sb, "= help: "); sb_put(&sb, help); sb_rst(&sb, color);
        sb_putc(&sb, '\n');
    }
    if (note && note[0]) {
        sb_spaces(&sb, G + 1);
        sb_col(&sb, color, C_NOT); sb_put(&sb, "= note: "); sb_put(&sb, note); sb_rst(&sb, color);
        sb_putc(&sb, '\n');
    }

    if (sb.cap) sb.buf[sb.len < sb.cap ? sb.len : sb.cap - 1] = '\0';
    return sb.len;
}

/* ---- back-compatible wrappers ------------------------------------------- */
size_t report_diagnostic(char *buf, size_t buflen,
                         const char *code, const char *title,
                         const char *file, Span primary,
                         const Span *secondary, size_t nsecondary,
                         const char *help, int color) {
    return report_diagnostic_ex(buf, buflen, code, title, file, primary, 0,
                                secondary, nsecondary, help, 0, color);
}

void am_ln_sb_capture(const char*, unsigned long);  /* ln.c: tee into the status-bar scroll-back ring */

void report_diagnostic_ex_stderr(const char *code, const char *title,
                                 const char *file, Span primary, const char *label,
                                 const Span *secondary, size_t nsecondary,
                                 const char *help, const char *note) {
    char buf[4096];
    report_diagnostic_ex(buf, sizeof buf, code, title, file, primary, label,
                         secondary, nsecondary, help, note, 1);
    fputs(buf, stderr);
    am_ln_sb_capture(buf, (unsigned long)strlen(buf));  /* so errors survive scroll-back */
}

void report_diagnostic_stderr(const char *code, const char *title,
                              const char *file, Span primary,
                              const Span *secondary, size_t nsecondary,
                              const char *help) {
    report_diagnostic_ex_stderr(code, title, file, primary, 0,
                                secondary, nsecondary, help, 0);
}
