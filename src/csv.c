/* csv.c  -  see csv.h.
 * GNU AGPLv3 - see LICENSE and NOTICE.
 *
 * Table shape verified interactively before the first version of this parser:
 *   amber> d:`a`b!(1 2 3;10 20 30)
 *   amber> t:+d
 *   amber> @t                 / `M  -- same tag a `([]a:..;b:..)` literal gets
 *   amber> meta t             / renders a real column/type/attribute table
 *   amber> qwhere[t;t[`a]>1]  / qSQL functional forms work on it unmodified
 * so csv_read() builds exactly that: `exc(names,cols)` (the `!` dyad, a.h)
 * to make the {names;values} dict, then `flp()` (a.h) to flip it into a
 * table -- the identical two calls `names!values` followed by `+` make at
 * the prompt.
 *
 * amber 2.2.1: the reader was rewritten for speed and memory. The 2.2.0
 * reader copied the file into the arena, built a pointer per field and then
 * ran strtoll()+strtod() over every cell during inference and once more to
 * build the columns: 12 s and 5.2 GB peak RSS for a 693 MB, 16.5M-row file.
 * This one
 *   - maps the file (read() into one buffer where mmap is unavailable),
 *   - splits it into one chunk per thread at row boundaries and counts each
 *     chunk's rows in parallel, so every column is allocated once at its
 *     final length and every chunk knows the row it starts at,
 *   - parses each chunk straight into those columns, typing speculatively
 *     (Long, promoted to Float, promoted to Symbol when a cell disproves the
 *     guess, re-reading only that column's rows of that chunk),
 *   - parses numbers with an exact fast path and hands everything else to
 *     strtoll()/strtod(), so every value is what the libc call would give,
 *   - interns symbol columns afterwards, serially, in the old order.
 * The results are bit-identical to the old reader, which is kept below as
 * ref_cols(): `csv0 checks the two against each other on a fixture battery
 * and on random files, and `csvx "path" does it for any file.
 *
 * #94 Q15 (both readers): bytes after a closing quote stay in the field, the
 * quote with them ("a"b is a"b, as q), where they used to end the row; and a
 * NUL is an ordinary byte, where it used to end the file (the old reader
 * read it as a C string).
 */
#if !defined(wasm)
/* Portability preamble, same as a.c/arena.c: must precede every system
 * header. mmap/madvise and MAP_* are BSD/SVID extensions on some libcs. */
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
#define _POSIX_C_SOURCE 200112L
#endif
#endif
#include "a.h"
#include "arena.h"
#include "csv.h"
#include "parallel.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <float.h>
#if !defined(wasm)
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#define CSV_MMAP 1
#else
#define CSV_MMAP 0
#endif

/* CSV_FASTF (the fast float path) and CSV_EL (the Eisel-Lemire method) are set in csv.h. */

/* The self-test feeds both readers missing and empty files on purpose. */
static int csv_quiet;
#define CSV_MSG(...) do { if (!csv_quiet) fprintf(stderr, __VA_ARGS__); } while (0)

/* [s,s+n), a whole number strtoll has read whole: is it within 64 bits?
 * strtoll clamps one past them (0W, or 0N below), and errno says so only on
 * some libcs (wasm's sets none), so count its digits: blanks, a sign and
 * leading zeros left off, at most 19, and 19 no more than 2^63-1 (2^63 for
 * a negative one, which is 0N). */
static int csv_fits(const char *s, size_t n) {
    const char *e = s + n;
    while (s < e && (*s == ' ' || (unsigned)(*s - '\t') < 5)) s++;
    int neg = s < e && *s == '-';
    if (s < e && (*s == '-' || *s == '+')) s++;
    while (s < e && *s == '0') s++;
    size_t d = (size_t)(e - s);
    return d < 19 || (d == 19 && memcmp(s, neg ? "9223372036854775808" : "9223372036854775807", 19) <= 0);
}

/* ======================================================================
 * Reference reader: the 2.2.0 implementation, unchanged apart from names,
 * returning its two halves instead of the table, and #94 Q15's two rules:
 * it reads by length (a field is its start and length, NUL-terminated after
 * it for strtoll/strtod/sym), and bytes after a closing quote stay in the
 * field. Only the self-test and `csvx call it. It is the oracle csv_cols()
 * must match bit for bit.
 * ====================================================================== */

typedef struct { char *s; size_t n; } RefF;

static RefF ref_field(char **pp, char *end, char *delim) {
    char *p = *pp;
    char *out = p;
    if (*p == '"') {
        char *w = p, *r = p + 1;
        while (r < end && !(*r == '"' && r[1] != '"')) {
            if (*r == '"' && r[1] == '"') { *w++ = '"'; r += 2; }
            else *w++ = *r++;
        }
        if (r < end) r++;
        if (r < end && *r != ',' && *r != '\n' && *r != '\r') {   /* the quote and what follows it, "" as one quote */
            *w++ = '"';
            while (r < end && *r != ',' && *r != '\n' && *r != '\r') { if (*r == '"' && r[1] == '"') r++; *w++ = *r++; }
        }
        *delim = *r;
        *w = 0;
        *pp = r;
        return (RefF){ out, (size_t)(w - out) };
    }
    while (p < end && *p != ',' && *p != '\n' && *p != '\r') p++;
    *delim = *p;
    *p = 0;
    *pp = p;
    return (RefF){ out, (size_t)(p - out) };
}

static RefF **ref_grid(char *data, char *end, U *nrows_out, U *ncols_out) {
    /* 2.2.0 counted only '\n' here, but a lone '\r' ends a row too, so a file
     * with old-Mac line endings wrote past this table (heap corruption, then
     * garbage columns). Counting both bytes is a true upper bound. */
    U maxlines = 1;
    for (char *p = data; p < end; p++) if (*p == '\n' || *p == '\r') maxlines++;
    RefF **rows = (RefF **)arena_alloc(maxlines * sizeof(RefF *));
    U nrows = 0;
    char *p = data;
    if (end - p >= 3 && (unsigned char)p[0] == 0xEF && (unsigned char)p[1] == 0xBB && (unsigned char)p[2] == 0xBF) p += 3;
    U ncols = 0;
    while (p < end) {
        if (*p == '\r' && p[1] == '\n') { p += 2; continue; }
        if (*p == '\n') { p++; continue; }
        U cap = ncols ? ncols : 32, cnt = 0;
        RefF *fields = (RefF *)arena_alloc(cap * sizeof(RefF));
        char delim = 0;
        for (;;) {
            RefF f = ref_field(&p, end, &delim);
            if (cnt >= cap) {
                RefF *grown = (RefF *)arena_alloc(cap * 2 * sizeof(RefF));
                memcpy(grown, fields, cnt * sizeof(RefF));
                fields = grown; cap *= 2;
            }
            fields[cnt++] = f;
            if (delim == ',') { p++; continue; }
            break;
        }
        if (delim == '\r') { p++; if (*p == '\n') p++; }
        else if (delim == '\n') p++;
        if (!ncols) { ncols = cnt; }
        if (cnt < ncols) {
            RefF *padded = (RefF *)arena_alloc(ncols * sizeof(RefF));
            memcpy(padded, fields, cnt * sizeof(RefF));
            for (U i = cnt; i < ncols; i++) padded[i] = (RefF){ (char *)"", 0 };
            fields = padded;
        }
        rows[nrows++] = fields;
    }
    *nrows_out = nrows;
    *ncols_out = ncols;
    return rows;
}

enum { COL_LONG, COL_FLOAT, COL_SYM };

/* the whole field must be read: a NUL inside it stops strtoll short of its end. A whole number past 64 bits is no
 * Long (strtoll gave 0W or 0N), so its column reads as Float */
static int ref_is_long(RefF f, long long *out) {
    if (!f.n) return 1;
    char *end;
    long long v = strtoll(f.s, &end, 10);
    if (end == f.s || end != f.s + f.n || !csv_fits(f.s, f.n)) return 0;
    *out = v;
    return 1;
}
static int ref_is_float(RefF f, double *out) {
    if (!f.n) return 1;
    char *end;
    double v = strtod(f.s, &end);
    if (end == f.s || end != f.s + f.n) return 0;
    *out = v;
    return 1;
}
static int ref_classify(RefF *rows_col, U nrows) {
    int could_long = 1, could_float = 1;
    for (U r = 0; r < nrows; r++) {
        long long li; double fv;
        if (could_long && !ref_is_long(rows_col[r], &li)) could_long = 0;
        if (could_float && !ref_is_float(rows_col[r], &fv)) could_float = 0;
        if (!could_long && !could_float) break;
    }
    if (could_long) return COL_LONG;
    if (could_float) return COL_FLOAT;
    return COL_SYM;
}

static int ref_cols(S path, A *names_out, A *cols_out) {
    FILE *fp = fopen(path, "rb");
    if (!fp) { CSV_MSG("csv: cannot open '%s'\n", path); return 0; }
    fseek(fp, 0, SEEK_END);
    long fsz = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if (fsz < 0) { fclose(fp); CSV_MSG("csv: cannot stat '%s'\n", path); return 0; }
    arena_reset();
    char *data = (char *)arena_alloc((size_t)fsz + 1);
    size_t got = fread(data, 1, (size_t)fsz, fp);
    fclose(fp);
    data[got] = 0;
    U nrows_total, ncols;
    RefF **rows = ref_grid(data, data + got, &nrows_total, &ncols);
    if (nrows_total == 0 || ncols == 0) {
        arena_reset();
        CSV_MSG("csv: '%s' is empty\n", path);
        return 0;
    }
    RefF *header = rows[0];
    U nrows = nrows_total - 1;
    A names = aS(ncols);
    I *namev = (I *)_V(names);
    for (U c = 0; c < ncols; c++) namev[c] = (I)sym(header[c].s);
    A cols = aA(ncols);
    A *colv = _A(cols);
    RefF *coldata = (RefF *)arena_alloc((nrows ? nrows : 1) * sizeof(RefF));
    for (U c = 0; c < ncols; c++) {
        for (U r = 0; r < nrows; r++) coldata[r] = rows[r + 1][c];
        int kind = nrows ? ref_classify(coldata, nrows) : COL_SYM;
        A vec;
        if (kind == COL_LONG) {
            vec = aL(nrows); L *v = _V(vec);
            for (U r = 0; r < nrows; r++) {
                long long li;
                v[r] = (coldata[r].n && ref_is_long(coldata[r], &li)) ? (L)li : NL;
            }
        } else if (kind == COL_FLOAT) {
            vec = aF(nrows); F *v = _V(vec);
            for (U r = 0; r < nrows; r++) {
                double fv;
                v[r] = (coldata[r].n && ref_is_float(coldata[r], &fv)) ? (F)fv : NF;
            }
        } else {
            vec = aS(nrows); I *v = (I *)_V(vec);
            for (U r = 0; r < nrows; r++) v[r] = (I)sym(coldata[r].s);
        }
        colv[c] = vec;
    }
    arena_reset();
    *names_out = names; *cols_out = cols;
    return 1;
}

/* ======================================================================
 * The reader.
 *
 * Lexing rules, restated from the reference so the two can be compared
 * line by line. Positions are bounded by an explicit end instead of a NUL
 * terminator, and a NUL is an ordinary byte (#94 Q15).
 *   row start  skip "\n" and "\r\n" (blank lines); a lone "\r" there is a
 *              row holding one empty field
 *   field      '"' opens a quoted field that runs to a '"' not followed by
 *              another '"'; "" inside is a literal quote. Anything else runs
 *              to ',', '\n', '\r' or the end. Bytes after the closing quote
 *              (#94 Q15) stay in the field, the quote with them, up to ',',
 *              '\n', '\r' or the end; "" there is one quote too
 *   after it   ',' starts the next field; "\r\n", "\r" or "\n" ends the row
 * ====================================================================== */

enum { CM_LONG, CM_FLOAT, CM_SYM };           /* ordered: promotion only goes up */
#define NLB ((uint64_t)1 << 63)               /* bits of NL */
#define NFB ((uint64_t)0x7ff8000000000000ull) /* bits of NF (NFL, a.h) */
#define CSV_DROP ((size_t)16 << 20)           /* release mapped input every 16 MB */
#define CSV_NONE ((size_t)-1)                /* no quote seen (the wasm libc has no SIZE_MAX) */

typedef struct {
    const char *b;      /* file bytes */
    size_t len;         /* bytes read (the file size) */
    size_t fsz;         /* mapped/read size */
    int mapped;
    size_t page;
    U nc;               /* column count (the header's field count) */
    int nk;             /* chunks */
    size_t cb[PAR_MAX_THREADS + 1];   /* chunk byte bounds */
    size_t cr[PAR_MAX_THREADS + 1];   /* first row of each chunk; cr[nk] = rows */
    size_t quo[PAR_MAX_THREADS], cnt[PAR_MAX_THREADS];
    unsigned char *lm;  /* nk*nc: each chunk's mode for each column */
    unsigned char *uns; /* nk*nc: a Long cell there is not (double)-convertible */
    uint64_t **col;     /* nc column payloads, 8 bytes a row */
    unsigned char *gm;  /* nc: each column's final mode (phase 2) */
    int bad;            /* a chunk's row count disagreed with phase 0 */
} Csv;

static int csv_forced_threads;  /* self-test: split even tiny files this many ways */

static void drop_pages(Csv *cv, size_t lo, size_t hi) {
#if CSV_MMAP && defined(MADV_DONTNEED)
    /* Already-parsed input is clean file-backed memory: tell the kernel we are
     * done with it so peak RSS is the columns, not columns plus file. A later
     * re-read just faults the page back in from the page cache. */
    if (!cv->mapped) return;
    size_t pg = cv->page;
    lo = (lo + pg - 1) / pg * pg; hi = hi / pg * pg;
    if (hi > lo) madvise((void *)(cv->b + lo), hi - lo, MADV_DONTNEED);
#else
    (void)cv; (void)lo; (void)hi;
#endif
}

static inline int at_delim(const char *p, const char *e) {
    return p >= e || *p == ',' || *p == '\n' || *p == '\r';
}

static const char *skip_blank(const char *p, const char *e) {
    for (;;) {
        if (p < e && *p == '\n') p++;
        else if (p + 1 < e && p[0] == '\r' && p[1] == '\n') p += 2;
        else return p;
    }
}

/* r follows a closing quote, and is not a delimiter: the bytes up to the
 * next one stay in the field, the quote with them ("a"b is a"b, as q), so
 * the content runs to there; field_str collapses "" pairs there too. */
__attribute__((noinline, cold))   /* rare: kept out of the lexer that phase 1 inlines */
static const char *lex_tail(const char *r, const char *e, const char **t, int *esc) {
    for (; r < e && *r != ',' && *r != '\n' && *r != '\r'; r++)
        if (*r == '"' && r + 1 < e && r[1] == '"') *esc = 1;
    *t = r;
    return r;
}

/* Lexes the field starting at p. [*s,*t) is its content with the quotes
 * stripped; *esc says it holds "" pairs still to be collapsed. Returns the
 * position of the byte that ended it (== e at the end). Inlined at every
 * call, as before the test after a closing quote: with it, gcc called it out
 * of line, a call per field in phase 1 and in the symbols' pass. */
static inline __attribute__((always_inline)) const char *lex_field(const char *p, const char *e, const char **s, const char **t, int *esc) {
    if (p < e && *p == '"') {
        const char *r = p + 1;
        *esc = 0; *s = r;
        while (r < e) {
            if (*r == '"') {
                if (r + 1 < e && r[1] == '"') { r += 2; *esc = 1; continue; }
                break;
            }
            r++;
        }
        *t = r;
        if (r < e) r++;  /* the closing quote */
        if (r < e && *r != ',' && *r != '\n' && *r != '\r') r = lex_tail(r, e, t, esc);
        return r;
    }
    const char *q = p;
    while (q < e && *q != ',' && *q != '\n' && *q != '\r') q++;
    *s = p; *t = q; *esc = 0;
    return q;
}

/* q is where a row's last field ended: step over the terminator. */
static const char *row_next(const char *q, const char *e) {
    if (q < e) {
        if (*q == '\r') { q++; if (q < e && *q == '\n') q++; }
        else if (*q == '\n') q++;
    }
    return q;
}

static const char *lex_row(const char *p, const char *e) {
    for (;;) {
        const char *s, *t; int esc;
        const char *q = lex_field(p, e, &s, &t, &esc);
        if (q < e && *q == ',') { p = q + 1; continue; }
        return row_next(q, e);
    }
}

/* Copies a field's content into *buf (grown as needed), collapsing "" pairs,
 * NUL-terminated -- the string the reference would have handed to sym(). */
static char *field_str(const char *s, const char *t, int esc, char **buf, size_t *cap) {
    size_t n = (size_t)(t - s);
    if (n + 1 > *cap) {
        size_t c = *cap ? *cap : 256;
        while (c < n + 1) c *= 2;
        char *nb = (char *)realloc(*buf, c);
        if (!nb) return 0;
        *buf = nb; *cap = c;
    }
    char *w = *buf;
    if (!esc) { memcpy(w, s, n); w[n] = 0; return w; }
    for (const char *r = s; r < t; ) {
        if (*r == '"' && r + 1 < t && r[1] == '"') { *w++ = '"'; r += 2; }
        else *w++ = *r++;
    }
    *w = 0;
    return *buf;
}

/* ---- numbers ---------------------------------------------------------- */

/* [+-]?[0-9]{1,18} at p. Returns the byte after it, or 0 if p does not start
 * with that. Any such string is one strtoll() reads exactly, without
 * overflow. *negz: it was a negative zero ("-0", "-00"), which strtod reads
 * as -0.0, so (double) of the Long would not reproduce the Float. */
static const char *fast_long(const char *p, const char *e, L *v, int *negz) {
    int neg = 0;
    if (p < e && (*p == '-' || *p == '+')) { neg = *p == '-'; p++; }
    const char *s = p;
    uint64_t w = 0;
    while (p < e && (unsigned)(*p - '0') < 10 && p - s < 19) { w = w * 10 + (unsigned)(*p - '0'); p++; }
    size_t nd = (size_t)(p - s);
    if (!nd || nd > 18) return 0;
    *v = neg ? -(L)w : (L)w;
    *negz = neg && !w;
    return p;
}

/* [s,t) as [+-]?0*[0-9]{1,19}, at most 19 digits after its leading zeros,
 * within 64 bits (2^63-1, or 2^63 when negative): the Longs fast_long leaves
 * to strtoll, 19 digits (nanosecond timestamps, 64-bit ids) or zero-padded.
 * Out of line, so the short numbers' path stays as it was. *uns for the two
 * (double) would not turn into strtod's Float: -2^63, 0N's bits, and a
 * negative zero (strtod's -0.0). */
__attribute__((noinline))
static int long19(const char *s, const char *t, L *v, unsigned char *uns) {
    int neg = s < t && *s == '-';
    if (s < t && (*s == '-' || *s == '+')) s++;
    const char *d = s;
    while (s < t && *s == '0') s++;
    if (t == d || t - s > 19) return 0;
    uint64_t w = 0;
    for (; s < t; s++) { if ((unsigned)(*s - '0') > 9) return 0; w = w * 10 + (unsigned)(*s - '0'); }
    if (w > ((uint64_t)1 << 63) - 1 + (uint64_t)neg) return 0;
    *v = neg ? (L)(0 - w) : (L)w;
    if (neg && (!w || w >> 63)) *uns = 1;
    return 1;
}

/* [+-]?digits[.digits][(e|E)[+-]?digits] at p, at most 19 significant
 * digits and a decimal exponent the table covers. Returns the byte after it
 * and the correctly rounded value, or 0 when the string is outside that
 * subset (the caller then asks strtod). Exactness (Clinger 1990): the
 * significand w <= 2^53 and 10^k (k <= 22) are both exact doubles, so w*10^k
 * or w/10^k is a single correctly rounded IEEE operation -- the same double
 * strtod returns. The sign goes on as a bit, not a negation: the build's
 * -fno-signed-zeros lets the compiler drop the sign of a zero. */
static const char *fast_float(const char *p, const char *e, F *v) {
#if CSV_FASTF
    int neg = 0, nd = 0, dx = 0, any = 0;
    if (p < e && (*p == '-' || *p == '+')) { neg = *p == '-'; p++; }
    uint64_t w = 0;
    while (p < e && (unsigned)(*p - '0') < 10) {
        unsigned d = (unsigned)(*p++ - '0'); any = 1;
        if (!w && !d) continue;
        if (nd == 19) return 0;
        w = w * 10 + d; nd++;
    }
    if (p < e && *p == '.') {
        p++;
        while (p < e && (unsigned)(*p - '0') < 10) {
            unsigned d = (unsigned)(*p++ - '0'); any = 1; dx--;
            if (!w && !d) continue;
            if (nd == 19) return 0;
            w = w * 10 + d; nd++;
        }
    }
    if (!any) return 0;
    if (p < e && (*p == 'e' || *p == 'E')) {
        int en = 0, ex = 0, k = 0;
        p++;
        if (p < e && (*p == '-' || *p == '+')) { en = *p == '-'; p++; }
        while (p < e && (unsigned)(*p - '0') < 10) { if (++k > 4) return 0; ex = ex * 10 + (*p++ - '0'); }
        if (!k) return 0;
        dx += en ? -ex : ex;
    }
    double r;
    if (!w) r = 0.0;
    else {
        if (w > ((uint64_t)1 << 53) || dx < -22 || dx > 22) return 0;
        r = dx < 0 ? (double)w / csv_p10[-dx] : (double)w * csv_p10[dx];
    }
    uint64_t bits;
    memcpy(&bits, &r, 8);
    if (neg) bits |= (uint64_t)1 << 63;
    memcpy(v, &bits, 8);
    return p;
#else
    (void)p; (void)e; (void)v;
    return 0;
#endif
}

/* strtoll/strtod over [s,s+n) exactly as the reference calls them: the whole
 * string must be consumed (a NUL in it stops them short), and a Long must be
 * within 64 bits (csv_fits). */
static int libc_num(const char *s, size_t n, int fl, L *lv, F *fv) {
    char sb[128];
    char *buf = n < sizeof sb ? sb : (char *)malloc(n + 1);
    if (!buf) return 0;
    memcpy(buf, s, n); buf[n] = 0;
    char *end; int ok;
    if (fl) { double d = strtod(buf, &end); ok = end != buf && end == buf + n; if (ok) *fv = d; }
    else { long long d = strtoll(buf, &end, 10); ok = end != buf && end == buf + n && csv_fits(buf, n); if (ok) *lv = (L)d; }
    if (buf != sb) free(buf);
    return ok;
}

/* Non-empty content [s,t) as a Long / Float, or 0 if the reference would
 * reject it. */
static int cell_long(const char *s, const char *t, L *v, unsigned char *uns) {
    int nz;
    if (fast_long(s, t, v, &nz) == t) { if (nz) *uns = 1; return 1; }
    if (t - s >= 19 && long19(s, t, v, uns)) return 1;
    if (libc_num(s, (size_t)(t - s), 0, v, 0)) { *uns = 1; return 1; }
    return 0;
}
static int cell_float(const char *s, const char *t, F *v) {
    if (fast_float(s, t, v) == t) return 1;
    return libc_num(s, (size_t)(t - s), 1, 0, v);
}

#if CSV_EL
/* 5^q for q in -342..308, each as 128 bits with the top bit set (truncated; for q < 0 rounded
 * up), two 64-bit words per power: the table of the Eisel-Lemire method (Lemire, "Number Parsing
 * at a Gigabyte per Second", 2021), generated as fast_float generates it (MIT; see NOTICE). */
static const uint64_t csv_p5[651 * 2] = {
    0xeef453d6923bd65aull, 0x113faa2906a13b3full,
    0x9558b4661b6565f8ull, 0x4ac7ca59a424c507ull,
    0xbaaee17fa23ebf76ull, 0x5d79bcf00d2df649ull,
    0xe95a99df8ace6f53ull, 0xf4d82c2c107973dcull,
    0x91d8a02bb6c10594ull, 0x79071b9b8a4be869ull,
    0xb64ec836a47146f9ull, 0x9748e2826cdee284ull,
    0xe3e27a444d8d98b7ull, 0xfd1b1b2308169b25ull,
    0x8e6d8c6ab0787f72ull, 0xfe30f0f5e50e20f7ull,
    0xb208ef855c969f4full, 0xbdbd2d335e51a935ull,
    0xde8b2b66b3bc4723ull, 0xad2c788035e61382ull,
    0x8b16fb203055ac76ull, 0x4c3bcb5021afcc31ull,
    0xaddcb9e83c6b1793ull, 0xdf4abe242a1bbf3dull,
    0xd953e8624b85dd78ull, 0xd71d6dad34a2af0dull,
    0x87d4713d6f33aa6bull, 0x8672648c40e5ad68ull,
    0xa9c98d8ccb009506ull, 0x680efdaf511f18c2ull,
    0xd43bf0effdc0ba48ull, 0x0212bd1b2566def2ull,
    0x84a57695fe98746dull, 0x014bb630f7604b57ull,
    0xa5ced43b7e3e9188ull, 0x419ea3bd35385e2dull,
    0xcf42894a5dce35eaull, 0x52064cac828675b9ull,
    0x818995ce7aa0e1b2ull, 0x7343efebd1940993ull,
    0xa1ebfb4219491a1full, 0x1014ebe6c5f90bf8ull,
    0xca66fa129f9b60a6ull, 0xd41a26e077774ef6ull,
    0xfd00b897478238d0ull, 0x8920b098955522b4ull,
    0x9e20735e8cb16382ull, 0x55b46e5f5d5535b0ull,
    0xc5a890362fddbc62ull, 0xeb2189f734aa831dull,
    0xf712b443bbd52b7bull, 0xa5e9ec7501d523e4ull,
    0x9a6bb0aa55653b2dull, 0x47b233c92125366eull,
    0xc1069cd4eabe89f8ull, 0x999ec0bb696e840aull,
    0xf148440a256e2c76ull, 0xc00670ea43ca250dull,
    0x96cd2a865764dbcaull, 0x380406926a5e5728ull,
    0xbc807527ed3e12bcull, 0xc605083704f5ecf2ull,
    0xeba09271e88d976bull, 0xf7864a44c633682eull,
    0x93445b8731587ea3ull, 0x7ab3ee6afbe0211dull,
    0xb8157268fdae9e4cull, 0x5960ea05bad82964ull,
    0xe61acf033d1a45dfull, 0x6fb92487298e33bdull,
    0x8fd0c16206306babull, 0xa5d3b6d479f8e056ull,
    0xb3c4f1ba87bc8696ull, 0x8f48a4899877186cull,
    0xe0b62e2929aba83cull, 0x331acdabfe94de87ull,
    0x8c71dcd9ba0b4925ull, 0x9ff0c08b7f1d0b14ull,
    0xaf8e5410288e1b6full, 0x07ecf0ae5ee44dd9ull,
    0xdb71e91432b1a24aull, 0xc9e82cd9f69d6150ull,
    0x892731ac9faf056eull, 0xbe311c083a225cd2ull,
    0xab70fe17c79ac6caull, 0x6dbd630a48aaf406ull,
    0xd64d3d9db981787dull, 0x092cbbccdad5b108ull,
    0x85f0468293f0eb4eull, 0x25bbf56008c58ea5ull,
    0xa76c582338ed2621ull, 0xaf2af2b80af6f24eull,
    0xd1476e2c07286faaull, 0x1af5af660db4aee1ull,
    0x82cca4db847945caull, 0x50d98d9fc890ed4dull,
    0xa37fce126597973cull, 0xe50ff107bab528a0ull,
    0xcc5fc196fefd7d0cull, 0x1e53ed49a96272c8ull,
    0xff77b1fcbebcdc4full, 0x25e8e89c13bb0f7aull,
    0x9faacf3df73609b1ull, 0x77b191618c54e9acull,
    0xc795830d75038c1dull, 0xd59df5b9ef6a2417ull,
    0xf97ae3d0d2446f25ull, 0x4b0573286b44ad1dull,
    0x9becce62836ac577ull, 0x4ee367f9430aec32ull,
    0xc2e801fb244576d5ull, 0x229c41f793cda73full,
    0xf3a20279ed56d48aull, 0x6b43527578c1110full,
    0x9845418c345644d6ull, 0x830a13896b78aaa9ull,
    0xbe5691ef416bd60cull, 0x23cc986bc656d553ull,
    0xedec366b11c6cb8full, 0x2cbfbe86b7ec8aa8ull,
    0x94b3a202eb1c3f39ull, 0x7bf7d71432f3d6a9ull,
    0xb9e08a83a5e34f07ull, 0xdaf5ccd93fb0cc53ull,
    0xe858ad248f5c22c9ull, 0xd1b3400f8f9cff68ull,
    0x91376c36d99995beull, 0x23100809b9c21fa1ull,
    0xb58547448ffffb2dull, 0xabd40a0c2832a78aull,
    0xe2e69915b3fff9f9ull, 0x16c90c8f323f516cull,
    0x8dd01fad907ffc3bull, 0xae3da7d97f6792e3ull,
    0xb1442798f49ffb4aull, 0x99cd11cfdf41779cull,
    0xdd95317f31c7fa1dull, 0x40405643d711d583ull,
    0x8a7d3eef7f1cfc52ull, 0x482835ea666b2572ull,
    0xad1c8eab5ee43b66ull, 0xda3243650005eecfull,
    0xd863b256369d4a40ull, 0x90bed43e40076a82ull,
    0x873e4f75e2224e68ull, 0x5a7744a6e804a291ull,
    0xa90de3535aaae202ull, 0x711515d0a205cb36ull,
    0xd3515c2831559a83ull, 0x0d5a5b44ca873e03ull,
    0x8412d9991ed58091ull, 0xe858790afe9486c2ull,
    0xa5178fff668ae0b6ull, 0x626e974dbe39a872ull,
    0xce5d73ff402d98e3ull, 0xfb0a3d212dc8128full,
    0x80fa687f881c7f8eull, 0x7ce66634bc9d0b99ull,
    0xa139029f6a239f72ull, 0x1c1fffc1ebc44e80ull,
    0xc987434744ac874eull, 0xa327ffb266b56220ull,
    0xfbe9141915d7a922ull, 0x4bf1ff9f0062baa8ull,
    0x9d71ac8fada6c9b5ull, 0x6f773fc3603db4a9ull,
    0xc4ce17b399107c22ull, 0xcb550fb4384d21d3ull,
    0xf6019da07f549b2bull, 0x7e2a53a146606a48ull,
    0x99c102844f94e0fbull, 0x2eda7444cbfc426dull,
    0xc0314325637a1939ull, 0xfa911155fefb5308ull,
    0xf03d93eebc589f88ull, 0x793555ab7eba27caull,
    0x96267c7535b763b5ull, 0x4bc1558b2f3458deull,
    0xbbb01b9283253ca2ull, 0x9eb1aaedfb016f16ull,
    0xea9c227723ee8bcbull, 0x465e15a979c1cadcull,
    0x92a1958a7675175full, 0x0bfacd89ec191ec9ull,
    0xb749faed14125d36ull, 0xcef980ec671f667bull,
    0xe51c79a85916f484ull, 0x82b7e12780e7401aull,
    0x8f31cc0937ae58d2ull, 0xd1b2ecb8b0908810ull,
    0xb2fe3f0b8599ef07ull, 0x861fa7e6dcb4aa15ull,
    0xdfbdcece67006ac9ull, 0x67a791e093e1d49aull,
    0x8bd6a141006042bdull, 0xe0c8bb2c5c6d24e0ull,
    0xaecc49914078536dull, 0x58fae9f773886e18ull,
    0xda7f5bf590966848ull, 0xaf39a475506a899eull,
    0x888f99797a5e012dull, 0x6d8406c952429603ull,
    0xaab37fd7d8f58178ull, 0xc8e5087ba6d33b83ull,
    0xd5605fcdcf32e1d6ull, 0xfb1e4a9a90880a64ull,
    0x855c3be0a17fcd26ull, 0x5cf2eea09a55067full,
    0xa6b34ad8c9dfc06full, 0xf42faa48c0ea481eull,
    0xd0601d8efc57b08bull, 0xf13b94daf124da26ull,
    0x823c12795db6ce57ull, 0x76c53d08d6b70858ull,
    0xa2cb1717b52481edull, 0x54768c4b0c64ca6eull,
    0xcb7ddcdda26da268ull, 0xa9942f5dcf7dfd09ull,
    0xfe5d54150b090b02ull, 0xd3f93b35435d7c4cull,
    0x9efa548d26e5a6e1ull, 0xc47bc5014a1a6dafull,
    0xc6b8e9b0709f109aull, 0x359ab6419ca1091bull,
    0xf867241c8cc6d4c0ull, 0xc30163d203c94b62ull,
    0x9b407691d7fc44f8ull, 0x79e0de63425dcf1dull,
    0xc21094364dfb5636ull, 0x985915fc12f542e4ull,
    0xf294b943e17a2bc4ull, 0x3e6f5b7b17b2939dull,
    0x979cf3ca6cec5b5aull, 0xa705992ceecf9c42ull,
    0xbd8430bd08277231ull, 0x50c6ff782a838353ull,
    0xece53cec4a314ebdull, 0xa4f8bf5635246428ull,
    0x940f4613ae5ed136ull, 0x871b7795e136be99ull,
    0xb913179899f68584ull, 0x28e2557b59846e3full,
    0xe757dd7ec07426e5ull, 0x331aeada2fe589cfull,
    0x9096ea6f3848984full, 0x3ff0d2c85def7621ull,
    0xb4bca50b065abe63ull, 0x0fed077a756b53a9ull,
    0xe1ebce4dc7f16dfbull, 0xd3e8495912c62894ull,
    0x8d3360f09cf6e4bdull, 0x64712dd7abbbd95cull,
    0xb080392cc4349decull, 0xbd8d794d96aacfb3ull,
    0xdca04777f541c567ull, 0xecf0d7a0fc5583a0ull,
    0x89e42caaf9491b60ull, 0xf41686c49db57244ull,
    0xac5d37d5b79b6239ull, 0x311c2875c522ced5ull,
    0xd77485cb25823ac7ull, 0x7d633293366b828bull,
    0x86a8d39ef77164bcull, 0xae5dff9c02033197ull,
    0xa8530886b54dbdebull, 0xd9f57f830283fdfcull,
    0xd267caa862a12d66ull, 0xd072df63c324fd7bull,
    0x8380dea93da4bc60ull, 0x4247cb9e59f71e6dull,
    0xa46116538d0deb78ull, 0x52d9be85f074e608ull,
    0xcd795be870516656ull, 0x67902e276c921f8bull,
    0x806bd9714632dff6ull, 0x00ba1cd8a3db53b6ull,
    0xa086cfcd97bf97f3ull, 0x80e8a40eccd228a4ull,
    0xc8a883c0fdaf7df0ull, 0x6122cd128006b2cdull,
    0xfad2a4b13d1b5d6cull, 0x796b805720085f81ull,
    0x9cc3a6eec6311a63ull, 0xcbe3303674053bb0ull,
    0xc3f490aa77bd60fcull, 0xbedbfc4411068a9cull,
    0xf4f1b4d515acb93bull, 0xee92fb5515482d44ull,
    0x991711052d8bf3c5ull, 0x751bdd152d4d1c4aull,
    0xbf5cd54678eef0b6ull, 0xd262d45a78a0635dull,
    0xef340a98172aace4ull, 0x86fb897116c87c34ull,
    0x9580869f0e7aac0eull, 0xd45d35e6ae3d4da0ull,
    0xbae0a846d2195712ull, 0x8974836059cca109ull,
    0xe998d258869facd7ull, 0x2bd1a438703fc94bull,
    0x91ff83775423cc06ull, 0x7b6306a34627ddcfull,
    0xb67f6455292cbf08ull, 0x1a3bc84c17b1d542ull,
    0xe41f3d6a7377eecaull, 0x20caba5f1d9e4a93ull,
    0x8e938662882af53eull, 0x547eb47b7282ee9cull,
    0xb23867fb2a35b28dull, 0xe99e619a4f23aa43ull,
    0xdec681f9f4c31f31ull, 0x6405fa00e2ec94d4ull,
    0x8b3c113c38f9f37eull, 0xde83bc408dd3dd04ull,
    0xae0b158b4738705eull, 0x9624ab50b148d445ull,
    0xd98ddaee19068c76ull, 0x3badd624dd9b0957ull,
    0x87f8a8d4cfa417c9ull, 0xe54ca5d70a80e5d6ull,
    0xa9f6d30a038d1dbcull, 0x5e9fcf4ccd211f4cull,
    0xd47487cc8470652bull, 0x7647c3200069671full,
    0x84c8d4dfd2c63f3bull, 0x29ecd9f40041e073ull,
    0xa5fb0a17c777cf09ull, 0xf468107100525890ull,
    0xcf79cc9db955c2ccull, 0x7182148d4066eeb4ull,
    0x81ac1fe293d599bfull, 0xc6f14cd848405530ull,
    0xa21727db38cb002full, 0xb8ada00e5a506a7cull,
    0xca9cf1d206fdc03bull, 0xa6d90811f0e4851cull,
    0xfd442e4688bd304aull, 0x908f4a166d1da663ull,
    0x9e4a9cec15763e2eull, 0x9a598e4e043287feull,
    0xc5dd44271ad3cdbaull, 0x40eff1e1853f29fdull,
    0xf7549530e188c128ull, 0xd12bee59e68ef47cull,
    0x9a94dd3e8cf578b9ull, 0x82bb74f8301958ceull,
    0xc13a148e3032d6e7ull, 0xe36a52363c1faf01ull,
    0xf18899b1bc3f8ca1ull, 0xdc44e6c3cb279ac1ull,
    0x96f5600f15a7b7e5ull, 0x29ab103a5ef8c0b9ull,
    0xbcb2b812db11a5deull, 0x7415d448f6b6f0e7ull,
    0xebdf661791d60f56ull, 0x111b495b3464ad21ull,
    0x936b9fcebb25c995ull, 0xcab10dd900beec34ull,
    0xb84687c269ef3bfbull, 0x3d5d514f40eea742ull,
    0xe65829b3046b0afaull, 0x0cb4a5a3112a5112ull,
    0x8ff71a0fe2c2e6dcull, 0x47f0e785eaba72abull,
    0xb3f4e093db73a093ull, 0x59ed216765690f56ull,
    0xe0f218b8d25088b8ull, 0x306869c13ec3532cull,
    0x8c974f7383725573ull, 0x1e414218c73a13fbull,
    0xafbd2350644eeacfull, 0xe5d1929ef90898faull,
    0xdbac6c247d62a583ull, 0xdf45f746b74abf39ull,
    0x894bc396ce5da772ull, 0x6b8bba8c328eb783ull,
    0xab9eb47c81f5114full, 0x066ea92f3f326564ull,
    0xd686619ba27255a2ull, 0xc80a537b0efefebdull,
    0x8613fd0145877585ull, 0xbd06742ce95f5f36ull,
    0xa798fc4196e952e7ull, 0x2c48113823b73704ull,
    0xd17f3b51fca3a7a0ull, 0xf75a15862ca504c5ull,
    0x82ef85133de648c4ull, 0x9a984d73dbe722fbull,
    0xa3ab66580d5fdaf5ull, 0xc13e60d0d2e0ebbaull,
    0xcc963fee10b7d1b3ull, 0x318df905079926a8ull,
    0xffbbcfe994e5c61full, 0xfdf17746497f7052ull,
    0x9fd561f1fd0f9bd3ull, 0xfeb6ea8bedefa633ull,
    0xc7caba6e7c5382c8ull, 0xfe64a52ee96b8fc0ull,
    0xf9bd690a1b68637bull, 0x3dfdce7aa3c673b0ull,
    0x9c1661a651213e2dull, 0x06bea10ca65c084eull,
    0xc31bfa0fe5698db8ull, 0x486e494fcff30a62ull,
    0xf3e2f893dec3f126ull, 0x5a89dba3c3efccfaull,
    0x986ddb5c6b3a76b7ull, 0xf89629465a75e01cull,
    0xbe89523386091465ull, 0xf6bbb397f1135823ull,
    0xee2ba6c0678b597full, 0x746aa07ded582e2cull,
    0x94db483840b717efull, 0xa8c2a44eb4571cdcull,
    0xba121a4650e4ddebull, 0x92f34d62616ce413ull,
    0xe896a0d7e51e1566ull, 0x77b020baf9c81d17ull,
    0x915e2486ef32cd60ull, 0x0ace1474dc1d122eull,
    0xb5b5ada8aaff80b8ull, 0x0d819992132456baull,
    0xe3231912d5bf60e6ull, 0x10e1fff697ed6c69ull,
    0x8df5efabc5979c8full, 0xca8d3ffa1ef463c1ull,
    0xb1736b96b6fd83b3ull, 0xbd308ff8a6b17cb2ull,
    0xddd0467c64bce4a0ull, 0xac7cb3f6d05ddbdeull,
    0x8aa22c0dbef60ee4ull, 0x6bcdf07a423aa96bull,
    0xad4ab7112eb3929dull, 0x86c16c98d2c953c6ull,
    0xd89d64d57a607744ull, 0xe871c7bf077ba8b7ull,
    0x87625f056c7c4a8bull, 0x11471cd764ad4972ull,
    0xa93af6c6c79b5d2dull, 0xd598e40d3dd89bcfull,
    0xd389b47879823479ull, 0x4aff1d108d4ec2c3ull,
    0x843610cb4bf160cbull, 0xcedf722a585139baull,
    0xa54394fe1eedb8feull, 0xc2974eb4ee658828ull,
    0xce947a3da6a9273eull, 0x733d226229feea32ull,
    0x811ccc668829b887ull, 0x0806357d5a3f525full,
    0xa163ff802a3426a8ull, 0xca07c2dcb0cf26f7ull,
    0xc9bcff6034c13052ull, 0xfc89b393dd02f0b5ull,
    0xfc2c3f3841f17c67ull, 0xbbac2078d443ace2ull,
    0x9d9ba7832936edc0ull, 0xd54b944b84aa4c0dull,
    0xc5029163f384a931ull, 0x0a9e795e65d4df11ull,
    0xf64335bcf065d37dull, 0x4d4617b5ff4a16d5ull,
    0x99ea0196163fa42eull, 0x504bced1bf8e4e45ull,
    0xc06481fb9bcf8d39ull, 0xe45ec2862f71e1d6ull,
    0xf07da27a82c37088ull, 0x5d767327bb4e5a4cull,
    0x964e858c91ba2655ull, 0x3a6a07f8d510f86full,
    0xbbe226efb628afeaull, 0x890489f70a55368bull,
    0xeadab0aba3b2dbe5ull, 0x2b45ac74ccea842eull,
    0x92c8ae6b464fc96full, 0x3b0b8bc90012929dull,
    0xb77ada0617e3bbcbull, 0x09ce6ebb40173744ull,
    0xe55990879ddcaabdull, 0xcc420a6a101d0515ull,
    0x8f57fa54c2a9eab6ull, 0x9fa946824a12232dull,
    0xb32df8e9f3546564ull, 0x47939822dc96abf9ull,
    0xdff9772470297ebdull, 0x59787e2b93bc56f7ull,
    0x8bfbea76c619ef36ull, 0x57eb4edb3c55b65aull,
    0xaefae51477a06b03ull, 0xede622920b6b23f1ull,
    0xdab99e59958885c4ull, 0xe95fab368e45ecedull,
    0x88b402f7fd75539bull, 0x11dbcb0218ebb414ull,
    0xaae103b5fcd2a881ull, 0xd652bdc29f26a119ull,
    0xd59944a37c0752a2ull, 0x4be76d3346f0495full,
    0x857fcae62d8493a5ull, 0x6f70a4400c562ddbull,
    0xa6dfbd9fb8e5b88eull, 0xcb4ccd500f6bb952ull,
    0xd097ad07a71f26b2ull, 0x7e2000a41346a7a7ull,
    0x825ecc24c873782full, 0x8ed400668c0c28c8ull,
    0xa2f67f2dfa90563bull, 0x728900802f0f32faull,
    0xcbb41ef979346bcaull, 0x4f2b40a03ad2ffb9ull,
    0xfea126b7d78186bcull, 0xe2f610c84987bfa8ull,
    0x9f24b832e6b0f436ull, 0x0dd9ca7d2df4d7c9ull,
    0xc6ede63fa05d3143ull, 0x91503d1c79720dbbull,
    0xf8a95fcf88747d94ull, 0x75a44c6397ce912aull,
    0x9b69dbe1b548ce7cull, 0xc986afbe3ee11abaull,
    0xc24452da229b021bull, 0xfbe85badce996168ull,
    0xf2d56790ab41c2a2ull, 0xfae27299423fb9c3ull,
    0x97c560ba6b0919a5ull, 0xdccd879fc967d41aull,
    0xbdb6b8e905cb600full, 0x5400e987bbc1c920ull,
    0xed246723473e3813ull, 0x290123e9aab23b68ull,
    0x9436c0760c86e30bull, 0xf9a0b6720aaf6521ull,
    0xb94470938fa89bceull, 0xf808e40e8d5b3e69ull,
    0xe7958cb87392c2c2ull, 0xb60b1d1230b20e04ull,
    0x90bd77f3483bb9b9ull, 0xb1c6f22b5e6f48c2ull,
    0xb4ecd5f01a4aa828ull, 0x1e38aeb6360b1af3ull,
    0xe2280b6c20dd5232ull, 0x25c6da63c38de1b0ull,
    0x8d590723948a535full, 0x579c487e5a38ad0eull,
    0xb0af48ec79ace837ull, 0x2d835a9df0c6d851ull,
    0xdcdb1b2798182244ull, 0xf8e431456cf88e65ull,
    0x8a08f0f8bf0f156bull, 0x1b8e9ecb641b58ffull,
    0xac8b2d36eed2dac5ull, 0xe272467e3d222f3full,
    0xd7adf884aa879177ull, 0x5b0ed81dcc6abb0full,
    0x86ccbb52ea94baeaull, 0x98e947129fc2b4e9ull,
    0xa87fea27a539e9a5ull, 0x3f2398d747b36224ull,
    0xd29fe4b18e88640eull, 0x8eec7f0d19a03aadull,
    0x83a3eeeef9153e89ull, 0x1953cf68300424acull,
    0xa48ceaaab75a8e2bull, 0x5fa8c3423c052dd7ull,
    0xcdb02555653131b6ull, 0x3792f412cb06794dull,
    0x808e17555f3ebf11ull, 0xe2bbd88bbee40bd0ull,
    0xa0b19d2ab70e6ed6ull, 0x5b6aceaeae9d0ec4ull,
    0xc8de047564d20a8bull, 0xf245825a5a445275ull,
    0xfb158592be068d2eull, 0xeed6e2f0f0d56712ull,
    0x9ced737bb6c4183dull, 0x55464dd69685606bull,
    0xc428d05aa4751e4cull, 0xaa97e14c3c26b886ull,
    0xf53304714d9265dfull, 0xd53dd99f4b3066a8ull,
    0x993fe2c6d07b7fabull, 0xe546a8038efe4029ull,
    0xbf8fdb78849a5f96ull, 0xde98520472bdd033ull,
    0xef73d256a5c0f77cull, 0x963e66858f6d4440ull,
    0x95a8637627989aadull, 0xdde7001379a44aa8ull,
    0xbb127c53b17ec159ull, 0x5560c018580d5d52ull,
    0xe9d71b689dde71afull, 0xaab8f01e6e10b4a6ull,
    0x9226712162ab070dull, 0xcab3961304ca70e8ull,
    0xb6b00d69bb55c8d1ull, 0x3d607b97c5fd0d22ull,
    0xe45c10c42a2b3b05ull, 0x8cb89a7db77c506aull,
    0x8eb98a7a9a5b04e3ull, 0x77f3608e92adb242ull,
    0xb267ed1940f1c61cull, 0x55f038b237591ed3ull,
    0xdf01e85f912e37a3ull, 0x6b6c46dec52f6688ull,
    0x8b61313bbabce2c6ull, 0x2323ac4b3b3da015ull,
    0xae397d8aa96c1b77ull, 0xabec975e0a0d081aull,
    0xd9c7dced53c72255ull, 0x96e7bd358c904a21ull,
    0x881cea14545c7575ull, 0x7e50d64177da2e54ull,
    0xaa242499697392d2ull, 0xdde50bd1d5d0b9e9ull,
    0xd4ad2dbfc3d07787ull, 0x955e4ec64b44e864ull,
    0x84ec3c97da624ab4ull, 0xbd5af13bef0b113eull,
    0xa6274bbdd0fadd61ull, 0xecb1ad8aeacdd58eull,
    0xcfb11ead453994baull, 0x67de18eda5814af2ull,
    0x81ceb32c4b43fcf4ull, 0x80eacf948770ced7ull,
    0xa2425ff75e14fc31ull, 0xa1258379a94d028dull,
    0xcad2f7f5359a3b3eull, 0x096ee45813a04330ull,
    0xfd87b5f28300ca0dull, 0x8bca9d6e188853fcull,
    0x9e74d1b791e07e48ull, 0x775ea264cf55347eull,
    0xc612062576589ddaull, 0x95364afe032a819eull,
    0xf79687aed3eec551ull, 0x3a83ddbd83f52205ull,
    0x9abe14cd44753b52ull, 0xc4926a9672793543ull,
    0xc16d9a0095928a27ull, 0x75b7053c0f178294ull,
    0xf1c90080baf72cb1ull, 0x5324c68b12dd6339ull,
    0x971da05074da7beeull, 0xd3f6fc16ebca5e04ull,
    0xbce5086492111aeaull, 0x88f4bb1ca6bcf585ull,
    0xec1e4a7db69561a5ull, 0x2b31e9e3d06c32e6ull,
    0x9392ee8e921d5d07ull, 0x3aff322e62439fd0ull,
    0xb877aa3236a4b449ull, 0x09befeb9fad487c3ull,
    0xe69594bec44de15bull, 0x4c2ebe687989a9b4ull,
    0x901d7cf73ab0acd9ull, 0x0f9d37014bf60a11ull,
    0xb424dc35095cd80full, 0x538484c19ef38c95ull,
    0xe12e13424bb40e13ull, 0x2865a5f206b06fbaull,
    0x8cbccc096f5088cbull, 0xf93f87b7442e45d4ull,
    0xafebff0bcb24aafeull, 0xf78f69a51539d749ull,
    0xdbe6fecebdedd5beull, 0xb573440e5a884d1cull,
    0x89705f4136b4a597ull, 0x31680a88f8953031ull,
    0xabcc77118461cefcull, 0xfdc20d2b36ba7c3eull,
    0xd6bf94d5e57a42bcull, 0x3d32907604691b4dull,
    0x8637bd05af6c69b5ull, 0xa63f9a49c2c1b110ull,
    0xa7c5ac471b478423ull, 0x0fcf80dc33721d54ull,
    0xd1b71758e219652bull, 0xd3c36113404ea4a9ull,
    0x83126e978d4fdf3bull, 0x645a1cac083126eaull,
    0xa3d70a3d70a3d70aull, 0x3d70a3d70a3d70a4ull,
    0xccccccccccccccccull, 0xcccccccccccccccdull,
    0x8000000000000000ull, 0x0000000000000000ull,
    0xa000000000000000ull, 0x0000000000000000ull,
    0xc800000000000000ull, 0x0000000000000000ull,
    0xfa00000000000000ull, 0x0000000000000000ull,
    0x9c40000000000000ull, 0x0000000000000000ull,
    0xc350000000000000ull, 0x0000000000000000ull,
    0xf424000000000000ull, 0x0000000000000000ull,
    0x9896800000000000ull, 0x0000000000000000ull,
    0xbebc200000000000ull, 0x0000000000000000ull,
    0xee6b280000000000ull, 0x0000000000000000ull,
    0x9502f90000000000ull, 0x0000000000000000ull,
    0xba43b74000000000ull, 0x0000000000000000ull,
    0xe8d4a51000000000ull, 0x0000000000000000ull,
    0x9184e72a00000000ull, 0x0000000000000000ull,
    0xb5e620f480000000ull, 0x0000000000000000ull,
    0xe35fa931a0000000ull, 0x0000000000000000ull,
    0x8e1bc9bf04000000ull, 0x0000000000000000ull,
    0xb1a2bc2ec5000000ull, 0x0000000000000000ull,
    0xde0b6b3a76400000ull, 0x0000000000000000ull,
    0x8ac7230489e80000ull, 0x0000000000000000ull,
    0xad78ebc5ac620000ull, 0x0000000000000000ull,
    0xd8d726b7177a8000ull, 0x0000000000000000ull,
    0x878678326eac9000ull, 0x0000000000000000ull,
    0xa968163f0a57b400ull, 0x0000000000000000ull,
    0xd3c21bcecceda100ull, 0x0000000000000000ull,
    0x84595161401484a0ull, 0x0000000000000000ull,
    0xa56fa5b99019a5c8ull, 0x0000000000000000ull,
    0xcecb8f27f4200f3aull, 0x0000000000000000ull,
    0x813f3978f8940984ull, 0x4000000000000000ull,
    0xa18f07d736b90be5ull, 0x5000000000000000ull,
    0xc9f2c9cd04674edeull, 0xa400000000000000ull,
    0xfc6f7c4045812296ull, 0x4d00000000000000ull,
    0x9dc5ada82b70b59dull, 0xf020000000000000ull,
    0xc5371912364ce305ull, 0x6c28000000000000ull,
    0xf684df56c3e01bc6ull, 0xc732000000000000ull,
    0x9a130b963a6c115cull, 0x3c7f400000000000ull,
    0xc097ce7bc90715b3ull, 0x4b9f100000000000ull,
    0xf0bdc21abb48db20ull, 0x1e86d40000000000ull,
    0x96769950b50d88f4ull, 0x1314448000000000ull,
    0xbc143fa4e250eb31ull, 0x17d955a000000000ull,
    0xeb194f8e1ae525fdull, 0x5dcfab0800000000ull,
    0x92efd1b8d0cf37beull, 0x5aa1cae500000000ull,
    0xb7abc627050305adull, 0xf14a3d9e40000000ull,
    0xe596b7b0c643c719ull, 0x6d9ccd05d0000000ull,
    0x8f7e32ce7bea5c6full, 0xe4820023a2000000ull,
    0xb35dbf821ae4f38bull, 0xdda2802c8a800000ull,
    0xe0352f62a19e306eull, 0xd50b2037ad200000ull,
    0x8c213d9da502de45ull, 0x4526f422cc340000ull,
    0xaf298d050e4395d6ull, 0x9670b12b7f410000ull,
    0xdaf3f04651d47b4cull, 0x3c0cdd765f114000ull,
    0x88d8762bf324cd0full, 0xa5880a69fb6ac800ull,
    0xab0e93b6efee0053ull, 0x8eea0d047a457a00ull,
    0xd5d238a4abe98068ull, 0x72a4904598d6d880ull,
    0x85a36366eb71f041ull, 0x47a6da2b7f864750ull,
    0xa70c3c40a64e6c51ull, 0x999090b65f67d924ull,
    0xd0cf4b50cfe20765ull, 0xfff4b4e3f741cf6dull,
    0x82818f1281ed449full, 0xbff8f10e7a8921a4ull,
    0xa321f2d7226895c7ull, 0xaff72d52192b6a0dull,
    0xcbea6f8ceb02bb39ull, 0x9bf4f8a69f764490ull,
    0xfee50b7025c36a08ull, 0x02f236d04753d5b4ull,
    0x9f4f2726179a2245ull, 0x01d762422c946590ull,
    0xc722f0ef9d80aad6ull, 0x424d3ad2b7b97ef5ull,
    0xf8ebad2b84e0d58bull, 0xd2e0898765a7deb2ull,
    0x9b934c3b330c8577ull, 0x63cc55f49f88eb2full,
    0xc2781f49ffcfa6d5ull, 0x3cbf6b71c76b25fbull,
    0xf316271c7fc3908aull, 0x8bef464e3945ef7aull,
    0x97edd871cfda3a56ull, 0x97758bf0e3cbb5acull,
    0xbde94e8e43d0c8ecull, 0x3d52eeed1cbea317ull,
    0xed63a231d4c4fb27ull, 0x4ca7aaa863ee4bddull,
    0x945e455f24fb1cf8ull, 0x8fe8caa93e74ef6aull,
    0xb975d6b6ee39e436ull, 0xb3e2fd538e122b44ull,
    0xe7d34c64a9c85d44ull, 0x60dbbca87196b616ull,
    0x90e40fbeea1d3a4aull, 0xbc8955e946fe31cdull,
    0xb51d13aea4a488ddull, 0x6babab6398bdbe41ull,
    0xe264589a4dcdab14ull, 0xc696963c7eed2dd1ull,
    0x8d7eb76070a08aecull, 0xfc1e1de5cf543ca2ull,
    0xb0de65388cc8ada8ull, 0x3b25a55f43294bcbull,
    0xdd15fe86affad912ull, 0x49ef0eb713f39ebeull,
    0x8a2dbf142dfcc7abull, 0x6e3569326c784337ull,
    0xacb92ed9397bf996ull, 0x49c2c37f07965404ull,
    0xd7e77a8f87daf7fbull, 0xdc33745ec97be906ull,
    0x86f0ac99b4e8dafdull, 0x69a028bb3ded71a3ull,
    0xa8acd7c0222311bcull, 0xc40832ea0d68ce0cull,
    0xd2d80db02aabd62bull, 0xf50a3fa490c30190ull,
    0x83c7088e1aab65dbull, 0x792667c6da79e0faull,
    0xa4b8cab1a1563f52ull, 0x577001b891185938ull,
    0xcde6fd5e09abcf26ull, 0xed4c0226b55e6f86ull,
    0x80b05e5ac60b6178ull, 0x544f8158315b05b4ull,
    0xa0dc75f1778e39d6ull, 0x696361ae3db1c721ull,
    0xc913936dd571c84cull, 0x03bc3a19cd1e38e9ull,
    0xfb5878494ace3a5full, 0x04ab48a04065c723ull,
    0x9d174b2dcec0e47bull, 0x62eb0d64283f9c76ull,
    0xc45d1df942711d9aull, 0x3ba5d0bd324f8394ull,
    0xf5746577930d6500ull, 0xca8f44ec7ee36479ull,
    0x9968bf6abbe85f20ull, 0x7e998b13cf4e1ecbull,
    0xbfc2ef456ae276e8ull, 0x9e3fedd8c321a67eull,
    0xefb3ab16c59b14a2ull, 0xc5cfe94ef3ea101eull,
    0x95d04aee3b80ece5ull, 0xbba1f1d158724a12ull,
    0xbb445da9ca61281full, 0x2a8a6e45ae8edc97ull,
    0xea1575143cf97226ull, 0xf52d09d71a3293bdull,
    0x924d692ca61be758ull, 0x593c2626705f9c56ull,
    0xb6e0c377cfa2e12eull, 0x6f8b2fb00c77836cull,
    0xe498f455c38b997aull, 0x0b6dfb9c0f956447ull,
    0x8edf98b59a373fecull, 0x4724bd4189bd5eacull,
    0xb2977ee300c50fe7ull, 0x58edec91ec2cb657ull,
    0xdf3d5e9bc0f653e1ull, 0x2f2967b66737e3edull,
    0x8b865b215899f46cull, 0xbd79e0d20082ee74ull,
    0xae67f1e9aec07187ull, 0xecd8590680a3aa11ull,
    0xda01ee641a708de9ull, 0xe80e6f4820cc9495ull,
    0x884134fe908658b2ull, 0x3109058d147fdcddull,
    0xaa51823e34a7eedeull, 0xbd4b46f0599fd415ull,
    0xd4e5e2cdc1d1ea96ull, 0x6c9e18ac7007c91aull,
    0x850fadc09923329eull, 0x03e2cf6bc604ddb0ull,
    0xa6539930bf6bff45ull, 0x84db8346b786151cull,
    0xcfe87f7cef46ff16ull, 0xe612641865679a63ull,
    0x81f14fae158c5f6eull, 0x4fcb7e8f3f60c07eull,
    0xa26da3999aef7749ull, 0xe3be5e330f38f09dull,
    0xcb090c8001ab551cull, 0x5cadf5bfd3072cc5ull,
    0xfdcb4fa002162a63ull, 0x73d9732fc7c8f7f6ull,
    0x9e9f11c4014dda7eull, 0x2867e7fddcdd9afaull,
    0xc646d63501a1511dull, 0xb281e1fd541501b8ull,
    0xf7d88bc24209a565ull, 0x1f225a7ca91a4226ull,
    0x9ae757596946075full, 0x3375788de9b06958ull,
    0xc1a12d2fc3978937ull, 0x0052d6b1641c83aeull,
    0xf209787bb47d6b84ull, 0xc0678c5dbd23a49aull,
    0x9745eb4d50ce6332ull, 0xf840b7ba963646e0ull,
    0xbd176620a501fbffull, 0xb650e5a93bc3d898ull,
    0xec5d3fa8ce427affull, 0xa3e51f138ab4cebeull,
    0x93ba47c980e98cdfull, 0xc66f336c36b10137ull,
    0xb8a8d9bbe123f017ull, 0xb80b0047445d4184ull,
    0xe6d3102ad96cec1dull, 0xa60dc059157491e5ull,
    0x9043ea1ac7e41392ull, 0x87c89837ad68db2full,
    0xb454e4a179dd1877ull, 0x29babe4598c311fbull,
    0xe16a1dc9d8545e94ull, 0xf4296dd6fef3d67aull,
    0x8ce2529e2734bb1dull, 0x1899e4a65f58660cull,
    0xb01ae745b101e9e4ull, 0x5ec05dcff72e7f8full,
    0xdc21a1171d42645dull, 0x76707543f4fa1f73ull,
    0x899504ae72497ebaull, 0x6a06494a791c53a8ull,
    0xabfa45da0edbde69ull, 0x0487db9d17636892ull,
    0xd6f8d7509292d603ull, 0x45a9d2845d3c42b6ull,
    0x865b86925b9bc5c2ull, 0x0b8a2392ba45a9b2ull,
    0xa7f26836f282b732ull, 0x8e6cac7768d7141eull,
    0xd1ef0244af2364ffull, 0x3207d795430cd926ull,
    0x8335616aed761f1full, 0x7f44e6bd49e807b8ull,
    0xa402b9c5a8d3a6e7ull, 0x5f16206c9c6209a6ull,
    0xcd036837130890a1ull, 0x36dba887c37a8c0full,
    0x802221226be55a64ull, 0xc2494954da2c9789ull,
    0xa02aa96b06deb0fdull, 0xf2db9baa10b7bd6cull,
    0xc83553c5c8965d3dull, 0x6f92829494e5acc7ull,
    0xfa42a8b73abbf48cull, 0xcb772339ba1f17f9ull,
    0x9c69a97284b578d7ull, 0xff2a760414536efbull,
    0xc38413cf25e2d70dull, 0xfef5138519684abaull,
    0xf46518c2ef5b8cd1ull, 0x7eb258665fc25d69ull,
    0x98bf2f79d5993802ull, 0xef2f773ffbd97a61ull,
    0xbeeefb584aff8603ull, 0xaafb550ffacfd8faull,
    0xeeaaba2e5dbf6784ull, 0x95ba2a53f983cf38ull,
    0x952ab45cfa97a0b2ull, 0xdd945a747bf26183ull,
    0xba756174393d88dfull, 0x94f971119aeef9e4ull,
    0xe912b9d1478ceb17ull, 0x7a37cd5601aab85dull,
    0x91abb422ccb812eeull, 0xac62e055c10ab33aull,
    0xb616a12b7fe617aaull, 0x577b986b314d6009ull,
    0xe39c49765fdf9d94ull, 0xed5a7e85fda0b80bull,
    0x8e41ade9fbebc27dull, 0x14588f13be847307ull,
    0xb1d219647ae6b31cull, 0x596eb2d8ae258fc8ull,
    0xde469fbd99a05fe3ull, 0x6fca5f8ed9aef3bbull,
    0x8aec23d680043beeull, 0x25de7bb9480d5854ull,
    0xada72ccc20054ae9ull, 0xaf561aa79a10ae6aull,
    0xd910f7ff28069da4ull, 0x1b2ba1518094da04ull,
    0x87aa9aff79042286ull, 0x90fb44d2f05d0842ull,
    0xa99541bf57452b28ull, 0x353a1607ac744a53ull,
    0xd3fa922f2d1675f2ull, 0x42889b8997915ce8ull,
    0x847c9b5d7c2e09b7ull, 0x69956135febada11ull,
    0xa59bc234db398c25ull, 0x43fab9837e699095ull,
    0xcf02b2c21207ef2eull, 0x94f967e45e03f4bbull,
    0x8161afb94b44f57dull, 0x1d1be0eebac278f5ull,
    0xa1ba1ba79e1632dcull, 0x6462d92a69731732ull,
    0xca28a291859bbf93ull, 0x7d7b8f7503cfdcfeull,
    0xfcb2cb35e702af78ull, 0x5cda735244c3d43eull,
    0x9defbf01b061adabull, 0x3a0888136afa64a7ull,
    0xc56baec21c7a1916ull, 0x088aaa1845b8fdd0ull,
    0xf6c69a72a3989f5bull, 0x8aad549e57273d45ull,
    0x9a3c2087a63f6399ull, 0x36ac54e2f678864bull,
    0xc0cb28a98fcf3c7full, 0x84576a1bb416a7ddull,
    0xf0fdf2d3f3c30b9full, 0x656d44a2a11c51d5ull,
    0x969eb7c47859e743ull, 0x9f644ae5a4b1b325ull,
    0xbc4665b596706114ull, 0x873d5d9f0dde1feeull,
    0xeb57ff22fc0c7959ull, 0xa90cb506d155a7eaull,
    0x9316ff75dd87cbd8ull, 0x09a7f12442d588f2ull,
    0xb7dcbf5354e9beceull, 0x0c11ed6d538aeb2full,
    0xe5d3ef282a242e81ull, 0x8f1668c8a86da5faull,
    0x8fa475791a569d10ull, 0xf96e017d694487bcull,
    0xb38d92d760ec4455ull, 0x37c981dcc395a9acull,
    0xe070f78d3927556aull, 0x85bbe253f47b1417ull,
    0x8c469ab843b89562ull, 0x93956d7478ccec8eull,
    0xaf58416654a6babbull, 0x387ac8d1970027b2ull,
    0xdb2e51bfe9d0696aull, 0x06997b05fcc0319eull,
    0x88fcf317f22241e2ull, 0x441fece3bdf81f03ull,
    0xab3c2fddeeaad25aull, 0xd527e81cad7626c3ull,
    0xd60b3bd56a5586f1ull, 0x8a71e223d8d3b074ull,
    0x85c7056562757456ull, 0xf6872d5667844e49ull,
    0xa738c6bebb12d16cull, 0xb428f8ac016561dbull,
    0xd106f86e69d785c7ull, 0xe13336d701beba52ull,
    0x82a45b450226b39cull, 0xecc0024661173473ull,
    0xa34d721642b06084ull, 0x27f002d7f95d0190ull,
    0xcc20ce9bd35c78a5ull, 0x31ec038df7b441f4ull,
    0xff290242c83396ceull, 0x7e67047175a15271ull,
    0x9f79a169bd203e41ull, 0x0f0062c6e984d386ull,
    0xc75809c42c684dd1ull, 0x52c07b78a3e60868ull,
    0xf92e0c3537826145ull, 0xa7709a56ccdf8a82ull,
    0x9bbcc7a142b17ccbull, 0x88a66076400bb691ull,
    0xc2abf989935ddbfeull, 0x6acff893d00ea435ull,
    0xf356f7ebf83552feull, 0x0583f6b8c4124d43ull,
    0x98165af37b2153deull, 0xc3727a337a8b704aull,
    0xbe1bf1b059e9a8d6ull, 0x744f18c0592e4c5cull,
    0xeda2ee1c7064130cull, 0x1162def06f79df73ull,
    0x9485d4d1c63e8be7ull, 0x8addcb5645ac2ba8ull,
    0xb9a74a0637ce2ee1ull, 0x6d953e2bd7173692ull,
    0xe8111c87c5c1ba99ull, 0xc8fa8db6ccdd0437ull,
    0x910ab1d4db9914a0ull, 0x1d9c9892400a22a2ull,
    0xb54d5e4a127f59c8ull, 0x2503beb6d00cab4bull,
    0xe2a0b5dc971f303aull, 0x2e44ae64840fd61dull,
    0x8da471a9de737e24ull, 0x5ceaecfed289e5d2ull,
    0xb10d8e1456105dadull, 0x7425a83e872c5f47ull,
    0xdd50f1996b947518ull, 0xd12f124e28f77719ull,
    0x8a5296ffe33cc92full, 0x82bd6b70d99aaa6full,
    0xace73cbfdc0bfb7bull, 0x636cc64d1001550bull,
    0xd8210befd30efa5aull, 0x3c47f7e05401aa4eull,
    0x8714a775e3e95c78ull, 0x65acfaec34810a71ull,
    0xa8d9d1535ce3b396ull, 0x7f1839a741a14d0dull,
    0xd31045a8341ca07cull, 0x1ede48111209a050ull,
    0x83ea2b892091e44dull, 0x934aed0aab460432ull,
    0xa4e4b66b68b65d60ull, 0xf81da84d5617853full,
    0xce1de40642e3f4b9ull, 0x36251260ab9d668eull,
    0x80d2ae83e9ce78f3ull, 0xc1d72b7c6b426019ull,
    0xa1075a24e4421730ull, 0xb24cf65b8612f81full,
    0xc94930ae1d529cfcull, 0xdee033f26797b627ull,
    0xfb9b7cd9a4a7443cull, 0x169840ef017da3b1ull,
    0x9d412e0806e88aa5ull, 0x8e1f289560ee864eull,
    0xc491798a08a2ad4eull, 0xf1a6f2bab92a27e2ull,
    0xf5b5d7ec8acb58a2ull, 0xae10af696774b1dbull,
    0x9991a6f3d6bf1765ull, 0xacca6da1e0a8ef29ull,
    0xbff610b0cc6edd3full, 0x17fd090a58d32af3ull,
    0xeff394dcff8a948eull, 0xddfc4b4cef07f5b0ull,
    0x95f83d0a1fb69cd9ull, 0x4abdaf101564f98eull,
    0xbb764c4ca7a4440full, 0x9d6d1ad41abe37f1ull,
    0xea53df5fd18d5513ull, 0x84c86189216dc5edull,
    0x92746b9be2f8552cull, 0x32fd3cf5b4e49bb4ull,
    0xb7118682dbb66a77ull, 0x3fbc8c33221dc2a1ull,
    0xe4d5e82392a40515ull, 0x0fabaf3feaa5334aull,
    0x8f05b1163ba6832dull, 0x29cb4d87f2a7400eull,
    0xb2c71d5bca9023f8ull, 0x743e20e9ef511012ull,
    0xdf78e4b2bd342cf6ull, 0x914da9246b255416ull,
    0x8bab8eefb6409c1aull, 0x1ad089b6c2f7548eull,
    0xae9672aba3d0c320ull, 0xa184ac2473b529b1ull,
    0xda3c0f568cc4f3e8ull, 0xc9e5d72d90a2741eull,
    0x8865899617fb1871ull, 0x7e2fa67c7a658892ull,
    0xaa7eebfb9df9de8dull, 0xddbb901b98feeab7ull,
    0xd51ea6fa85785631ull, 0x552a74227f3ea565ull,
    0x8533285c936b35deull, 0xd53a88958f87275full,
    0xa67ff273b8460356ull, 0x8a892abaf368f137ull,
    0xd01fef10a657842cull, 0x2d2b7569b0432d85ull,
    0x8213f56a67f6b29bull, 0x9c3b29620e29fc73ull,
    0xa298f2c501f45f42ull, 0x8349f3ba91b47b8full,
    0xcb3f2f7642717713ull, 0x241c70a936219a73ull,
    0xfe0efb53d30dd4d7ull, 0xed238cd383aa0110ull,
    0x9ec95d1463e8a506ull, 0xf4363804324a40aaull,
    0xc67bb4597ce2ce48ull, 0xb143c6053edcd0d5ull,
    0xf81aa16fdc1b81daull, 0xdd94b7868e94050aull,
    0x9b10a4e5e9913128ull, 0xca7cf2b4191c8326ull,
    0xc1d4ce1f63f57d72ull, 0xfd1c2f611f63a3f0ull,
    0xf24a01a73cf2dccfull, 0xbc633b39673c8cecull,
    0x976e41088617ca01ull, 0xd5be0503e085d813ull,
    0xbd49d14aa79dbc82ull, 0x4b2d8644d8a74e18ull,
    0xec9c459d51852ba2ull, 0xddf8e7d60ed1219eull,
    0x93e1ab8252f33b45ull, 0xcabb90e5c942b503ull,
    0xb8da1662e7b00a17ull, 0x3d6a751f3b936243ull,
    0xe7109bfba19c0c9dull, 0x0cc512670a783ad4ull,
    0x906a617d450187e2ull, 0x27fb2b80668b24c5ull,
    0xb484f9dc9641e9daull, 0xb1f9f660802dedf6ull,
    0xe1a63853bbd26451ull, 0x5e7873f8a0396973ull,
    0x8d07e33455637eb2ull, 0xdb0b487b6423e1e8ull,
    0xb049dc016abc5e5full, 0x91ce1a9a3d2cda62ull,
    0xdc5c5301c56b75f7ull, 0x7641a140cc7810fbull,
    0x89b9b3e11b6329baull, 0xa9e904c87fcb0a9dull,
    0xac2820d9623bf429ull, 0x546345fa9fbdcd44ull,
    0xd732290fbacaf133ull, 0xa97c177947ad4095ull,
    0x867f59a9d4bed6c0ull, 0x49ed8eabcccc485dull,
    0xa81f301449ee8c70ull, 0x5c68f256bfff5a74ull,
    0xd226fc195c6a2f8cull, 0x73832eec6fff3111ull,
    0x83585d8fd9c25db7ull, 0xc831fd53c5ff7eabull,
    0xa42e74f3d032f525ull, 0xba3e7ca8b77f5e55ull,
    0xcd3a1230c43fb26full, 0x28ce1bd2e55f35ebull,
    0x80444b5e7aa7cf85ull, 0x7980d163cf5b81b3ull,
    0xa0555e361951c366ull, 0xd7e105bcc332621full,
    0xc86ab5c39fa63440ull, 0x8dd9472bf3fefaa7ull,
    0xfa856334878fc150ull, 0xb14f98f6f0feb951ull,
    0x9c935e00d4b9d8d2ull, 0x6ed1bf9a569f33d3ull,
    0xc3b8358109e84f07ull, 0x0a862f80ec4700c8ull,
    0xf4a642e14c6262c8ull, 0xcd27bb612758c0faull,
    0x98e7e9cccfbd7dbdull, 0x8038d51cb897789cull,
    0xbf21e44003acdd2cull, 0xe0470a63e6bd56c3ull,
    0xeeea5d5004981478ull, 0x1858ccfce06cac74ull,
    0x95527a5202df0ccbull, 0x0f37801e0c43ebc8ull,
    0xbaa718e68396cffdull, 0xd30560258f54e6baull,
    0xe950df20247c83fdull, 0x47c6b82ef32a2069ull,
    0x91d28b7416cdd27eull, 0x4cdc331d57fa5441ull,
    0xb6472e511c81471dull, 0xe0133fe4adf8e952ull,
    0xe3d8f9e563a198e5ull, 0x58180fddd97723a6ull,
    0x8e679c2f5e44ff8full, 0x570f09eaa7ea7648ull,
};

/* w * 10^q, correctly rounded, for w of at most 19 decimal digits (w != 0): the Eisel-Lemire
 * method, as in fast_float's compute_float for binary64. A 128-bit product of w with the
 * truncated 5^q gives the 54 bits needed and the rounding; for at most 19 digits that product is
 * always sufficient (Mushtak and Lemire, "Fast number parsing without fallback", 2023), so there
 * is no fallback here. Returns the bits of the double. */
static inline uint64_t csv_el(uint64_t w, long long q) {
    if (q < -342) return 0;
    if (q > 308) return (uint64_t)0x7FF << 52;
    int lz = __builtin_clzll(w); w <<= lz;
    const uint64_t *t = csv_p5 + 2 * (q + 342);
    unsigned __int128 p = (unsigned __int128)w * t[0];
    uint64_t hi = (uint64_t)(p >> 64), lo = (uint64_t)p;
    if ((hi & 0x1FF) == 0x1FF) {
        uint64_t h2 = (uint64_t)(((unsigned __int128)w * t[1]) >> 64);
        lo += h2; if (h2 > lo) hi++;
    }
    int up = (int)(hi >> 63), sh = up + 64 - 52 - 3;
    uint64_t m = hi >> sh;
    int e = (int)((((152170 + 65536) * q) >> 16) + 63) + up - lz + 1023;
    if (e <= 0) {   /* subnormal */
        if (-e + 1 >= 64) return 0;
        m >>= -e + 1; m += m & 1; m >>= 1;
        return m | ((uint64_t)(m >= ((uint64_t)1 << 52)) << 52);
    }
    if (lo <= 1 && q >= -4 && q <= 23 && (m & 3) == 1 && (m << sh) == hi) m &= ~(uint64_t)1;   /* a tie: to even */
    m += m & 1; m >>= 1;
    if (m >= ((uint64_t)2 << 52)) { m = (uint64_t)1 << 52; e++; }
    m &= ~((uint64_t)1 << 52);
    if (e >= 0x7FF) return (uint64_t)0x7FF << 52;
    return m | (uint64_t)e << 52;
}

/* csv_el as a double, and 0.0 for w = 0: what csv_fast (src/csv.h) calls when Clinger's fast path
 * cannot take the number, out of line so that the readers' common path stays small. */
__attribute__((noinline)) F csv_elf(uint64_t w, long long q) {
    if (!w) return 0.0;
    uint64_t bits = csv_el(w, q); F v; memcpy(&v, &bits, 8); return v;
}

#endif

/* A number of more than 19 digits, of which the first 19 make w (leading zeros among them make it
 * shorter): it lies between w*10^q and (w+1)*10^q, so when the Eisel-Lemire method rounds both to
 * the same double, that is the answer (as fast_float does it): 1 and *v. 0 when they differ,
 * rarely, or w is 0, or the method is not built; the caller then asks csv_float. */
int csv_span(uint64_t w, long long q, F *v) {
#if CSV_EL
    if (!w) return 0;
    uint64_t a = csv_el(w, q), w1 = w + 1; long long q1 = q;
    if (w1 == 10000000000000000000ull) { w1 /= 10; q1++; }   /* 20 digits: 10^19 is 10^18*10 */
    if (a != csv_el(w1, q1)) return 0;
    memcpy(v, &a, 8); return 1;
#else
    (void)w; (void)q; (void)v;
    return 0;
#endif
}

#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
/* Eight digits at p as an integer, in three multiplies (as fast_float does): the caller knows all
 * eight are digits. */
static inline uint64_t csv_8(const char *p) {
    uint64_t u; memcpy(&u, p, 8);
    u -= 0x3030303030303030ull; u = u * 10 + (u >> 8);
    return ((u & 0x000000FF000000FFull) * (100 + (1000000ull << 32)) + ((u >> 16) & 0x000000FF000000FFull) * (1 + (10000ull << 32))) >> 32 & 0xFFFFFFFFull;
}
#define CSV_8 1
#endif

/* A number of more than 19 digits, as the JSON reader has scanned it: i integer digits at b, the
 * fraction's at f, the number 0.d1d2d3... times 10^e. Its first 19 digits make w (eight at a time
 * where they can), and csv_span decides if it can (1 and *v); 0 and the caller asks csv_float. */
__attribute__((noinline)) int csv_long(const char *b, long long i, const char *f, long long e, F *v) {   /* out of line, so the reader's common path stays small */
    uint64_t w = 0; int k = (int)(i < 19 ? i : 19), r = 19 - k;   /* k digits from b, then r from f */
#ifdef CSV_8
    for (; k >= 8; k -= 8, b += 8) w = w * 100000000 + csv_8(b);
    for (; k; k--) w = w * 10 + (uint64_t)(*b++ - '0');
    for (; r >= 8; r -= 8, f += 8) w = w * 100000000 + csv_8(f);
#endif
    for (; k; k--) w = w * 10 + (uint64_t)(*b++ - '0');
    for (; r; r--) w = w * 10 + (uint64_t)(*f++ - '0');
    return csv_span(w, e - 19, v);
}

/* The unsigned decimal at [s,t) -- digits[.digits][(e|E)[+-]digits] -- correctly rounded. The
 * literal reader (src/p.c pf, so `F$ too) and the JSON reader (src/j.c) check their own syntax,
 * take csv_fast, csv_span or csv_long themselves, and call this only when those cannot decide.
 * An exponent with no digits counts as 0 (the literal reader's rule; strtod would stop before
 * it). The first 19 significant digits make w: if the rest are all 0, w*10^q is the number,
 * exact; if not, csv_span decides if it can. Only when it cannot, rarely, does strtod decide,
 * given a normal form in a fixed buffer: the first 780 significant digits, then a 1 if any later
 * digit is not 0 (a decimal needs at most 768 significant digits to decide its rounding, so the
 * sticky digit keeps the answer), then the exponent, clamped. */
F csv_float(const char *s, const char *t) {
    F v; uint64_t w = 0; int k = 0, frac, nz = 0; long long q = 0, ex = 0;
    const char *p = s, *u;
#define CSV_D(p) (p < t && (unsigned)(*p - '0') < 10)
    while (p < t && *p == '0') p++;   /* leading zeros */
    for (; CSV_D(p) && k < 19; p++, k++) w = w * 10 + (uint64_t)(*p - '0');   /* the integer digits: the first 19 make w */
    for (u = p; CSV_D(p); p++) nz |= *p - '0';   /* the rest scale it */
    q = p - u;
    if (p < t && *p == '.') {
        p++;
        if (!k) { u = p; while (p < t && *p == '0') p++; q = u - p; }
        for (u = p; CSV_D(p) && k < 19; p++, k++) w = w * 10 + (uint64_t)(*p - '0');
        q -= p - u;
        for (; CSV_D(p); p++) nz |= *p - '0';
    }
#undef CSV_D
    if (!k) return 0.0;
    const char *de = p;   /* the digits end here */
    if (p < t && (*p == 'e' || *p == 'E')) {
        p++; int en = p < t && *p == '-'; p += p < t && (*p == '-' || *p == '+');
        while (p < t && (unsigned)(*p - '0') < 10) { if (ex < 100000000000000000ll) ex = ex * 10 + (*p - '0'); p++; }   /* saturates far beyond any offset the digits can make up for */
        if (en) ex = -ex;
    }
    q += ex;
    if (nz ? csv_span(w, q, &v) : csv_fast(w, q, &v)) return v;
    char b[800]; int n = 0, sticky = 0; long long dx = ex;
    frac = 0;
    for (p = s; p < de; p++) {
        if (*p == '.') { frac = 1; continue; }
        if (!n && *p == '0') { dx -= frac; continue; }
        if (n < 780) { b[n++] = *p; dx -= frac; }
        else { dx += !frac; sticky |= *p != '0'; }
    }
    if (sticky) { b[n++] = '1'; dx--; }
#if !defined(wasm)
    if (dx > 100000) dx = 100000; else if (dx < -100000) dx = -100000;
    snprintf(b + n, sizeof b - (size_t)n, "e%lld", dx);
    return strtod(b, 0);
#else
    /* wasm: its strtod (src/wasmlibc.c) scales by repeated *10 and cannot take this form, so
     * scale as the literal reader did before: the first 19 digits by a table of powers of ten,
     * in two steps below 1e-308. Not correctly rounded, as before, but never 0 or NaN in range. */
    static F p10[309];
    if (p10[0] != 1) { F z = 1; for (int i = 0; i <= 308; i++) { p10[i] = z; z *= 10; } }
    w = 0; for (int i = 0; i < n && i < 19; i++) w = w * 10 + (uint64_t)(b[i] - '0');
    long long e = dx + (n > 19 ? n - 19 : 0);
    if (e > 308) { F r = p10[308]; return r * r; }
    if (e < -308) return e < -616 ? 0.0 : ((F)w / p10[308]) / p10[-e - 308];
    return e < 0 ? (F)w / p10[-e] : (F)w * p10[e];
#endif
}

static inline uint64_t null_bits(int m) { return m == CM_LONG ? NLB : m == CM_FLOAT ? NFB : 0; }

/* ---- phase 0: find quotes, count rows ---------------------------------- */

/* Counts chunk k's rows assuming it holds no quote (true unless quo[] says
 * otherwise, and then the count is discarded): so it stops at the first
 * quote, as rechunk_quoted counts the whole file again with the lexer. */
static void phase0(void *ctx, int k) {
    Csv *cv = (Csv *)ctx;
    const char *b = cv->b, *p = b + cv->cb[k], *ce = b + cv->cb[k + 1], *fe = b + cv->len;
    size_t rows = 0, quo = CSV_NONE, dropped = cv->cb[k];
    while (p < ce) {
        char ch = *p;
        if (ch == '\n') { p++; continue; }
        if (ch == '\r' && p + 1 < fe && p[1] == '\n') { p += 2; continue; }
        rows++;
        for (; p < ce; p++) {
            ch = *p;
            if ((unsigned char)ch > '"') continue;   /* most bytes: one compare */
            if (ch == '\n' || ch == '\r') break;
            if (ch == '"') { quo = (size_t)(p - b); goto done; }
        }
        if (p < ce) {
            if (*p == '\r') { p++; if (p < fe && *p == '\n') p++; }
            else p++;
        }
        if ((size_t)(p - b) - dropped >= CSV_DROP) { drop_pages(cv, dropped, (size_t)(p - b)); dropped = (size_t)(p - b); }
    }
    drop_pages(cv, dropped, cv->cb[k + 1]);
done:
    cv->quo[k] = quo; cv->cnt[k] = rows;
}

/* Files with quotes past the header: one sequential pass with the real lexer
 * finds row starts to split at (a quoted field may hold newlines, so no byte
 * offset is safe to start from blind) and counts the rows. */
static void rechunk_quoted(Csv *cv, size_t ds) {
    const char *b = cv->b, *e = b + cv->len, *p = b + ds;
    int nk = cv->nk, k = 0;
    size_t rows = 0, span = cv->len - ds, dropped = ds;
    size_t next = nk > 1 ? ds + span / (size_t)nk : CSV_NONE;
    cv->cb[0] = ds; cv->cr[0] = 0;
    for (;;) {
        p = skip_blank(p, e);
        if (p >= e) break;
        while (k + 1 < nk && (size_t)(p - b) >= next) {
            k++; cv->cb[k] = (size_t)(p - b); cv->cr[k] = rows;
            next = k + 1 < nk ? ds + span / (size_t)nk * (size_t)(k + 1) : CSV_NONE;
        }
        rows++;
        p = lex_row(p, e);
        if ((size_t)(p - b) - dropped >= CSV_DROP) { drop_pages(cv, dropped, (size_t)(p - b)); dropped = (size_t)(p - b); }
    }
    for (k++; k < nk; k++) { cv->cb[k] = cv->len; cv->cr[k] = rows; }
    cv->cb[nk] = cv->len; cv->cr[nk] = rows;
}

/* ---- phase 1: parse ---------------------------------------------------- */

/* Re-reads rows [0,nrows) of chunk k and rewrites column j in mode m (Float,
 * or Sym = the field's offset+1, 0 for empty). Used when a column's mode goes
 * up after some of the chunk's rows were stored in the old mode. */
static void refill(Csv *cv, int k, U j, size_t nrows, int m) {
    const char *b = cv->b, *e = b + cv->len, *p = b + cv->cb[k];
    uint64_t *slot = cv->col[j] + cv->cr[k];
    for (size_t i = 0; i < nrows; i++) {
        p = skip_blank(p, e);
        uint64_t v = null_bits(m);
        for (U f = 0;; f++) {
            const char *s, *t; int esc;
            const char *q = lex_field(p, e, &s, &t, &esc);
            if (f == j && s != t) {
                if (m == CM_SYM) v = (uint64_t)(p - b) + 1;
                else { F d; if (!esc && cell_float(s, t, &d)) memcpy(&v, &d, 8); }
            }
            if (q < e && *q == ',') { p = q + 1; continue; }
            p = row_next(q, e);
            break;
        }
        slot[i] = v;
    }
}

/* Parses the field at p into column j, row `row` (the chunk's li-th row).
 * Returns where the field ended. */
static const char *parse_cell(Csv *cv, int k, U j, const char *p, const char *e, size_t row, size_t li) {
    uint64_t *slot = cv->col[j] + row;
    unsigned char *m = &cv->lm[(size_t)k * cv->nc + j];
    if (p >= e || *p != '"') {
        if (at_delim(p, e)) { *slot = null_bits(*m); return p; }
        if (*m == CM_LONG) {
            L v; int nz;
            const char *q = fast_long(p, e, &v, &nz);
            if (q && at_delim(q, e)) {
                *slot = (uint64_t)v;
                if (nz) cv->uns[(size_t)k * cv->nc + j] = 1;
                return q;
            }
        } else if (*m == CM_FLOAT) {
            F d;
            const char *q = fast_float(p, e, &d);
            if (q && at_delim(q, e)) { memcpy(slot, &d, 8); return q; }
        }
    }
    const char *s, *t; int esc;
    const char *q = lex_field(p, e, &s, &t, &esc);
    if (s == t) { *slot = null_bits(*m); return q; }
    if (*m == CM_LONG) {
        L v;
        if (!esc && cell_long(s, t, &v, &cv->uns[(size_t)k * cv->nc + j])) { *slot = (uint64_t)v; return q; }
        F d;
        if (!esc && cell_float(s, t, &d)) {
            refill(cv, k, j, li, CM_FLOAT); *m = CM_FLOAT;
            memcpy(slot, &d, 8); return q;
        }
        refill(cv, k, j, li, CM_SYM); *m = CM_SYM;
    } else if (*m == CM_FLOAT) {
        F d;
        if (!esc && cell_float(s, t, &d)) { memcpy(slot, &d, 8); return q; }
        refill(cv, k, j, li, CM_SYM); *m = CM_SYM;
    }
    *slot = (uint64_t)(p - cv->b) + 1;
    return q;
}

static void phase1(void *ctx, int k) {
    Csv *cv = (Csv *)ctx;
    U nc = cv->nc;
    const char *b = cv->b, *e = b + cv->len, *ce = b + cv->cb[k + 1], *p = b + cv->cb[k];
    unsigned char *lm = cv->lm + (size_t)k * nc;
    size_t r0 = cv->cr[k], nr = cv->cr[k + 1] - r0, i = 0, dropped = cv->cb[k];
    for (;;) {
        p = skip_blank(p, e);
        if (p >= ce) break;
        if (i >= nr) { cv->bad = 1; return; }
        U j = 0;
        for (;;) {
            const char *q;
            if (j < nc) q = parse_cell(cv, k, j, p, e, r0 + i, i);
            else { const char *s, *t; int esc; q = lex_field(p, e, &s, &t, &esc); }
            j++;
            if (q < e && *q == ',') { p = q + 1; continue; }
            p = row_next(q, e);
            break;
        }
        for (; j < nc; j++) cv->col[j][r0 + i] = null_bits(lm[j]);
        i++;
        if ((size_t)(p - b) - dropped >= CSV_DROP) { drop_pages(cv, dropped, (size_t)(p - b)); dropped = (size_t)(p - b); }
    }
    if (i != nr) cv->bad = 1;
    drop_pages(cv, dropped, cv->cb[k + 1]);
}

/* ---- phase 2: bring every chunk to its column's final mode ------------- */

static void phase2(void *ctx, int k) {
    Csv *cv = (Csv *)ctx;
    size_t n = cv->cr[k + 1] - cv->cr[k];
    if (!n) return;
    for (U j = 0; j < cv->nc; j++) {
        int m = cv->lm[(size_t)k * cv->nc + j], g = cv->gm[j];
        if (m == g) continue;
        if (g == CM_FLOAT && m == CM_LONG && !cv->uns[(size_t)k * cv->nc + j]) {
            /* every cell came through fast_long, so (double)v is exactly the
             * correctly rounded value strtod gives for the same digits */
            uint64_t *s = cv->col[j] + cv->cr[k];
            for (size_t i = 0; i < n; i++) {
                if (s[i] == NLB) s[i] = NFB;
                else { F d = (F)(L)s[i]; memcpy(&s[i], &d, 8); }
            }
        } else refill(cv, k, j, n, g);
    }
}

/* ---- file access ------------------------------------------------------- */

static int load_file(S path, Csv *cv) {
#if CSV_MMAP
    int fd = open(path, O_RDONLY);
    if (fd < 0) { CSV_MSG("csv: cannot open '%s'\n", path); return 0; }
    struct stat st;
    if (fstat(fd, &st) || !S_ISREG(st.st_mode)) { close(fd); CSV_MSG("csv: cannot stat '%s'\n", path); return 0; }
    size_t sz = (size_t)st.st_size;
    long pg = sysconf(_SC_PAGESIZE);
    cv->page = pg > 0 ? (size_t)pg : 4096;
    cv->fsz = sz;
    if (!sz) { close(fd); cv->b = ""; cv->len = 0; return 1; }
    void *m = mmap(0, sz, PROT_READ, MAP_PRIVATE, fd, 0);
    if (m != MAP_FAILED) {
        close(fd);
        cv->b = (const char *)m; cv->len = sz; cv->mapped = 1;
        return 1;
    }
    char *buf = (char *)malloc(sz ? sz : 1);
    size_t got = 0;
    while (buf && got < sz) {
        ssize_t r = read(fd, buf + got, sz - got);
        if (r <= 0) break;
        got += (size_t)r;
    }
    close(fd);
    if (!buf) { CSV_MSG("csv: out of memory reading '%s'\n", path); return 0; }
    cv->b = buf; cv->len = got;
    return 1;
#else
    FILE *fp = fopen(path, "rb");
    if (!fp) { CSV_MSG("csv: cannot open '%s'\n", path); return 0; }
    fseek(fp, 0, SEEK_END);
    long fsz = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if (fsz < 0) { fclose(fp); CSV_MSG("csv: cannot stat '%s'\n", path); return 0; }
    char *buf = (char *)malloc((size_t)fsz + 1);
    if (!buf) { fclose(fp); CSV_MSG("csv: out of memory reading '%s'\n", path); return 0; }
    cv->b = buf; cv->len = fread(buf, 1, (size_t)fsz, fp); cv->fsz = (size_t)fsz;
    fclose(fp);
    return 1;
#endif
}

static void unload_file(Csv *cv) {
#if CSV_MMAP
    if (cv->mapped) { munmap((void *)cv->b, cv->fsz); return; }
    if (cv->fsz) free((void *)cv->b);
#else
    free((void *)cv->b);
#endif
}

/* ---- the reader -------------------------------------------------------- */

typedef struct { const char *s, *t; int esc; } Hdr;

static int csv_cols(S path, A *names_out, A *cols_out) {
    Csv cv;
    memset(&cv, 0, sizeof cv);
    if (!load_file(path, &cv)) return 0;
    const char *b = cv.b;
    Hdr *hdr = 0;
    char *sbuf = 0; size_t scap = 0;
    int ok = 0;

    const char *e = b + cv.len, *p = b;
    if (cv.len >= 3 && (unsigned char)b[0] == 0xEF && (unsigned char)b[1] == 0xBB && (unsigned char)b[2] == 0xBF) p += 3;
    p = skip_blank(p, e);
    if (p >= e) { CSV_MSG("csv: '%s' is empty\n", path); goto out; }

    /* header row */
    U nc = 0, hcap = 16;
    free(hdr);
    hdr = (Hdr *)malloc(hcap * sizeof *hdr);
    if (!hdr) goto oom;
    for (;;) {
        if (nc == hcap) {
            Hdr *g = (Hdr *)realloc(hdr, (size_t)hcap * 2 * sizeof *hdr);
            if (!g) goto oom;
            hdr = g; hcap *= 2;
        }
        const char *q = lex_field(p, e, &hdr[nc].s, &hdr[nc].t, &hdr[nc].esc);
        nc++;
        if (q < e && *q == ',') { p = q + 1; continue; }
        p = row_next(q, e);
        break;
    }
    size_t ds = (size_t)(p - b);
    cv.nc = nc;

    /* chunks: one per thread, starting just after a '\n' */
    size_t span = cv.len - ds;
    int nk = csv_forced_threads > 0 ? csv_forced_threads
           : span >= ((size_t)1 << 20) ? par_thread_count(span) : 1;
    if (nk > PAR_MAX_THREADS) nk = PAR_MAX_THREADS;
    if (nk < 1) nk = 1;
    cv.nk = nk;
    cv.cb[0] = ds;
    for (int k = 1; k < nk; k++) {
        size_t o = ds + span / (size_t)nk * (size_t)k;
        if (o < cv.cb[k - 1]) o = cv.cb[k - 1];
        const char *nl = o < cv.len ? (const char *)memchr(b + o, '\n', cv.len - o) : 0;
        cv.cb[k] = nl ? (size_t)(nl - b) + 1 : cv.len;
    }
    cv.cb[nk] = cv.len;

    par_run(nk, phase0, &cv);

    int quoted = 0;
    for (int k = 0; k < nk; k++) if (cv.quo[k] < cv.len) quoted = 1;
    if (quoted) rechunk_quoted(&cv, ds);
    else {
        cv.cr[0] = 0;
        for (int k = 0; k < nk; k++) cv.cr[k + 1] = cv.cr[k] + cv.cnt[k];
    }
    size_t nr = cv.cr[nk];
    if (nr > 0xffffffffu) { CSV_MSG("csv: '%s' has too many rows\n", path); goto out; }

    /* names first: the reference interns the header before any cell */
    A names = aS(nc);
    I *namev = (I *)_V(names);
    for (U c = 0; c < nc; c++) {
        char *s = field_str(hdr[c].s, hdr[c].t, hdr[c].esc, &sbuf, &scap);
        if (!s) { mr(names); goto oom; }
        namev[c] = (I)sym(s);
    }
    A cols = aA(nc);
    A *colv = _A(cols);
    if (!nr) {
        for (U c = 0; c < nc; c++) colv[c] = aS(0);
        *names_out = names; *cols_out = cols; ok = 1;
        goto out;
    }

    cv.col = (uint64_t **)malloc(nc * sizeof *cv.col);
    cv.lm = (unsigned char *)calloc((size_t)nk * nc, 1);   /* CM_LONG == 0 */
    cv.uns = (unsigned char *)calloc((size_t)nk * nc, 1);
    unsigned char *gm = (unsigned char *)calloc(nc, 1);
    if (!cv.col || !cv.lm || !cv.uns || !gm) { free(gm); mr(names); for (U c = 0; c < nc; c++) colv[c] = aS(0); mr(cols); goto oom; }
    for (U c = 0; c < nc; c++) { colv[c] = aL((U)nr); cv.col[c] = (uint64_t *)_V(colv[c]); }

    par_run(nk, phase1, &cv);
    if (cv.bad) {
        /* cannot happen if phase 0 and phase 1 lex alike; never guess */
        free(gm); mr(names); mr(cols);
        CSV_MSG("csv: internal row-count mismatch on '%s', using the reference reader\n", path);
        free(cv.col); free(cv.lm); free(cv.uns); cv.col = 0; cv.lm = cv.uns = 0;
        free(hdr); free(sbuf);
        unload_file(&cv);
        return ref_cols(path, names_out, cols_out);
    }

    for (U j = 0; j < nc; j++)
        for (int k = 0; k < nk; k++)
            if (cv.cr[k + 1] > cv.cr[k] && cv.lm[(size_t)k * nc + j] > gm[j]) gm[j] = cv.lm[(size_t)k * nc + j];
    cv.gm = gm;
    par_run(nk, phase2, &cv);

    /* symbols last, serially, column by column in row order: the order the
     * reference interns them in */
    for (U j = 0; j < nc; j++) {
        if (gm[j] == CM_FLOAT) { _T(colv[j]) = tF; continue; }
        if (gm[j] != CM_SYM) continue;
        A sv = aS((U)nr);
        I *iv = (I *)_V(sv);
        const uint64_t *off = cv.col[j];
        I empty = 0; int have_empty = 0;
        size_t dropped = 0, hi = 0;
        for (size_t r = 0; r < nr; r++) {
            if (!off[r]) {
                if (!have_empty) { empty = (I)sym(""); have_empty = 1; }
                iv[r] = empty;
                continue;
            }
            const char *f = b + off[r] - 1, *s, *t; int esc;
            const char *q = lex_field(f, e, &s, &t, &esc);
            char *str = field_str(s, t, esc, &sbuf, &scap);
            iv[r] = (I)sym(str ? str : "");
            if ((size_t)(q - b) > hi) hi = (size_t)(q - b);
            if (hi - dropped >= CSV_DROP) { drop_pages(&cv, dropped, hi); dropped = hi; }
        }
        mr(colv[j]);
        colv[j] = sv;
    }
    free(gm);
    *names_out = names; *cols_out = cols; ok = 1;
    goto out;

oom:
    CSV_MSG("csv: out of memory reading '%s'\n", path);
out:
    free(cv.col); free(cv.lm); free(cv.uns);
    free(hdr); free(sbuf);
    unload_file(&cv);
    return ok;
}

A csv_read(S path) {
    A names, cols;
    /* amber 2.3: a file that cannot be read is an 'io ERROR (trappable, and it
     * stops a script), not the generic null `::` -- a script that loaded a
     * missing file used to carry on with a null table. The stderr line saying
     * why (not found, a directory, out of memory) is kept. */
    if (!csv_cols(path, &names, &cols)) return eo0();
    A dict = exc(names, cols);   /* names ! cols  -- the real `!` dyad (a.h) */
    return flp(dict);            /* +dict          -- the real flip verb (a.h) */
}

/* ======================================================================
 * Verification: the new reader against the reference, bit for bit.
 * ====================================================================== */

/* 1 iff the two readers' names and columns are identical: same types, same
 * lengths, same bytes (so -0.0, NaN payloads and symbol indices all count). */
static int same_result(int ok1, A n1, A c1, int ok2, A n2, A c2, S what) {
    if (ok1 != ok2) { fprintf(stderr, "csvx %s: reference ok=%d, reader ok=%d\n", what, ok1, ok2); return 0; }
    if (!ok1) return 1;
    int same = 1;
    if (_n(n1) != _n(n2) || memcmp(_V(n1), _V(n2), (size_t)_n(n1) * 4)) {
        fprintf(stderr, "csvx %s: column names differ\n", what); same = 0;
    }
    for (U c = 0; same && c < _n(c1); c++) {
        A a = _A(c1)[c], z = _A(c2)[c];
        if (_t(a) != _t(z) || _n(a) != _n(z)) {
            fprintf(stderr, "csvx %s: column %u type %d/%d length %u/%u\n", what, c, (int)_t(a), (int)_t(z), _n(a), _n(z));
            same = 0; break;
        }
        size_t w = (size_t)_W(a);
        const unsigned char *pa = (const unsigned char *)_V(a), *pz = (const unsigned char *)_V(z);
        for (U r = 0; r < _n(a); r++)
            if (memcmp(pa + r * w, pz + r * w, w)) {
                fprintf(stderr, "csvx %s: column %u row %u differs\n", what, c, r);
                same = 0; break;
            }
    }
    return same;
}

static int check_file(S path, int threads, S what) {
    A n1 = 0, c1 = 0, n2 = 0, c2 = 0;
    int ok1 = ref_cols(path, &n1, &c1);
    csv_forced_threads = threads;
    int ok2 = csv_cols(path, &n2, &c2);
    csv_forced_threads = 0;
    int same = same_result(ok1, n1, c1, ok2, n2, c2, what);
    if (ok1) { mr(n1); mr(c1); }
    if (ok2) { mr(n2); mr(c2); }
    return same;
}

int csv_check(S path) {
    int same = check_file(path, 0, path);
    if (!same) fprintf(stderr, "csvx: '%s' MISMATCH\n", path);
    return same;
}

/* ---- self-test --------------------------------------------------------- */

static uint64_t st_rng;
static uint64_t st_next(void) {   /* splitmix64: deterministic across platforms */
    uint64_t z = (st_rng += 0x9e3779b97f4a7c15ull);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
    return z ^ (z >> 31);
}
static U st_below(U n) { return (U)(st_next() % n); }

static CO char *const st_odd[] = { "", "-", "+", ".", "-.", "e5", "1e", "1e+", "1.e3", "-0", "+0", "-0.0", "0e999999",
    "1e-320", "4.9e-324", "2.4703282292062327e-324", "1e308", "1.7976931348623157e308", "1e309", "-1e309",
    "9007199254740993", "9007199254740992", "18014398509481985", "123456789012345678", "1234567890123456789",
    "99999999999999999999", "-99999999999999999999", "9223372036854775807", "-9223372036854775808",
    "9223372036854775808", "0x1p3", "0X10", "nan", "NaN", "inf", "-Infinity", " 5", "5 ", "\t7", "1,5", "1_0",
    "00000000000000000000000001", "0.000000000000000000000000001", "1e22", "1e23", "9007199254740993e22",
    "12345678901234567890e-5", "-0000000000000000000", "0.1", "0.3", "2.2250738585072011e-308", "2.2250738585072014e-308", "+.5", "5." };
#define ST_NODD (sizeof st_odd / sizeof *st_odd)

/* A random numeric-looking string, biased to the shapes that matter: the
 * fast paths' edges (18/19/20 digits, 2^53, exponent 22/23, 4/5 exponent
 * digits), subnormals, overflow, signs, zeros, and near-misses. */
static size_t st_number(char *o) {

    U r = st_below(10);
    if (r < 3) { const char *s = st_odd[st_below((U)ST_NODD)]; size_t n = strlen(s); memcpy(o, s, n); return n; }
    size_t n = 0;
    U sg = st_below(4);
    if (sg == 1) o[n++] = '-'; else if (sg == 2) o[n++] = '+';
    U lead = st_below(5) == 0 ? (st_below(4) ? st_below(4) : st_below(24)) : 0;
    for (U i = 0; i < lead; i++) o[n++] = '0';
    U di = st_below(5) == 0 ? st_below(24) : st_below(10);
    for (U i = 0; i < di; i++) o[n++] = (char)('0' + st_below(10));
    if (st_below(2)) {
        o[n++] = '.';
        U df = st_below(5) == 0 ? st_below(24) : st_below(12);
        for (U i = 0; i < df; i++) o[n++] = (char)('0' + st_below(10));
    }
    if (st_below(4) == 0) {
        o[n++] = st_below(2) ? 'e' : 'E';
        U es = st_below(3);
        if (es == 1) o[n++] = '-'; else if (es == 2) o[n++] = '+';
        U ed = st_below(6);
        U ev = st_below(3) == 0 ? st_below(400) : st_below(30);
        char tmp[16]; int tl = sprintf(tmp, "%u", ev);
        if (ed > 0 && (U)tl < ed) for (U i = 0; i < ed - (U)tl; i++) o[n++] = '0';
        if (ed > 0) { memcpy(o + n, tmp, (size_t)tl); n += (size_t)tl; }
    }
    if (st_below(40) == 0) o[n++] = "x .e-"[st_below(5)];
    return n;
}

/* The numeric kernels against strtoll/strtod on `count` strings: the same
 * accept/reject decision, and the same bits when both accept. */
static int st_numbers(long count) {
    char s[128];
    for (long i = 0; i < count; i++) {
        size_t n = st_number(s);
        s[n] = 0;
        if (!n) continue;
        char *end;
        long long rl = strtoll(s, &end, 10); int okl = end != s && !*end && csv_fits(s, n);
        double rd = strtod(s, &end); int okd = end != s && !*end;
        L lv = 0; F fv = 0; unsigned char uns = 0;
        int ml = cell_long(s, s + n, &lv, &uns), mf = cell_float(s, s + n, &fv);
        if (ml != okl || (ml && lv != (L)rl)) { fprintf(stderr, "csv0: Long mismatch on \"%s\"\n", s); return 0; }
        if (mf != okd || (mf && memcmp(&fv, &rd, 8))) { fprintf(stderr, "csv0: Float mismatch on \"%s\"\n", s); return 0; }
        /* fast_long's promise to phase 2: (double) of a safe Long is strtod's answer */
        if (ml && !uns) { F cv = (F)lv; if (!okd || memcmp(&cv, &rd, 8)) { fprintf(stderr, "csv0: Long->Float mismatch on \"%s\"\n", s); return 0; } }
    }
    return 1;
}

typedef struct { const char *s; size_t n; } StFix;
#define FX(lit) { lit, sizeof lit - 1 }
static CO StFix st_fix[] = {
    FX("a,b,c\n1,2,3\n4,5,6\n"),
    FX("a,b\r\n1,2\r\n3,4\r\n"),
    FX("a,b\n1,2"),                       /* no final newline */
    FX(""),                               /* empty file */
    FX("\n\n\r\n"),                       /* blank lines only */
    FX("a,b,c\n"),                        /* header only */
    FX("a,b,c"),                          /* header only, unterminated */
    FX("\xEF\xBB\xBFx,y\n1,2\n"),         /* BOM */
    FX("a,b,c\n1\n2,3,4,5,6\n,,\n7,8\n"), /* ragged rows */
    FX("a,b\n\n1,2\n\n\n3,4\n\r\n5,6\n"), /* blank lines between rows */
    FX("a,b\n1,2\r3,4\r\r5,6\n"),         /* lone CRs */
    FX("a\n\r\r\n1\n"),
    FX("q,n\n\"x,y\",1\n\"he said \"\"hi\"\"\",2\n\"multi\nline\",3\n\"\",4\n"),
    FX("q\n\"ab\"cd,ef\n1\n"),            /* bytes after a closing quote stay in the field: ab"cd */
    FX("x,y\n\"a\"b,c\n1,2\n"),
    FX("x,y\n\"1\"2,3\n4,\"5\" \n6,\"7\"\"\"8\"\"9\r\n\"a\"\"b\"c\"\"d,\"e\"f"),   /* "" after the quote is one quote */
    FX("x,y\n\"a\"\"\"b\n\"\",\"\"\"\n"),
    FX("q,r\n\"unterminated,1\n2,3\n"),
    FX("\"h1\",\"h,2\"\n1,2\n"),          /* quoted header */
    FX("\"multi\nline header\",b\n1,2\n"),
    FX("f\n-0.0\n0\n"),
    FX("f\n-0\n1.5\n"),
    FX("f\n1e-320\n4.9e-324\n1e308\n1e309\n-1e309\n"),
    FX("i\n9007199254740993\n9223372036854775807\n"),
    FX("i\n99999999999999999999\n-99999999999999999999\n"),
    FX("i\n9223372036854775808\n1\n"),       /* past 64 bits: Float */
    FX("i\n1\n-9223372036854775809\n2\n"),
    FX("i\n12345678901234567890\nx\n"),
    FX("i\n1234567890123456789\n-9223372036854775808\n0009223372036854775807\n"),   /* 19 digits; -2^63 (0N) */
    FX("i\n0000000000000000000123\n-0000000000000000000000\n-0009223372036854775808\n1.5\n"),   /* zero-padded */
    FX("i\n-9223372036854775808\n1.5\n"),
    FX("i\n-0000000000000000000\n1.5\n"),     /* 19 digits of -0: strtod's -0.0 */
    FX("i\n9223372036854775807\n-9223372036854775807\n-0\n"),
    FX("i,f\n+5,+1.5\n-7,-.5\n"),
    FX("a,b\n,\n,\n"),                    /* all-empty columns */
    FX("m\n1\n2\n3.5\n4\n"),              /* int then float */
    FX("m\n1\n2\n3\nx\n"),                /* symbol on the last row */
    FX("m\n1.5\n2\n\"3\"\nnan\ninf\n0x10\n"),
    FX("m\n 5\n6 \n"),                    /* whitespace: strtoll takes the leading, not the trailing */
    FX("m\n\" 7\"\n\"\n8\"\n"),
    FX("a,b\n1,2\n3,\"4\"\"\"\n"),
    FX("a,b\n1,2\n3\x00" "4,5\n6,7\n"),   /* NUL: an ordinary byte */
    FX("a\x00" "b,c\n1,2\n"),
    FX("\x00" "a,b\n"),
    FX("x,y\n1,2\n3,\x00" "4\n5,6\n"),
    FX("x,y\n1,2\n\x00\n5,\"6\x00\"\n7,8\x00"),
    FX("s\nab\x00" "c\nab\nab\x00" "d\n"),
    FX("a,b\n1,2\n3,4\x00\x00\x00"),       /* NULs at the end: data too */
    FX("a,b\n1,2\n3,4\n\x00\x00\r\n\x00\n"),
    FX("a,b\n1,2\n\x00\n3,4\n"),          /* a NUL line inside: data */
    FX("a\n1\n\r"),
    FX("s\nAAPL\nMSFT\n\nAAPL\n,\n"),
    FX("x,x\n1,2\n"),                     /* duplicate names */
    FX("a,b\n1,2,\"3,\n4\"\n5,6\n"),      /* quoted newline in a clipped field */
};

static int st_write(S path, const char *s, size_t n) {
    FILE *fp = fopen(path, "wb");
    if (!fp) return 0;
    size_t w = fwrite(s, 1, n, fp);
    fclose(fp);
    return w == n;
}

/* A random CSV built from typed columns, so most columns stay numeric and
 * promotions happen late, in the middle, or in one chunk only. Every string
 * that can end up in a Symbol column comes from a small fixed set: symbols
 * longer than 4 bytes live in one 64 KB table for the life of the process
 * (m.c), and the self-test must not fill it. */
static size_t st_csv(char *o, size_t cap) {
    static const char *term[] = { "\n", "\n", "\n", "\r\n", "\r\n", "\r", "\n\n", "\r\n\r\n", "\r\r\n" };
    size_t n = 0;
    U nc = 1 + st_below(5), nr = st_below(60);
    U kind[8];
    for (U c = 0; c < nc; c++) kind[c] = st_below(7);
    int quotes = st_below(3) == 0;
    for (U c = 0; c < nc; c++) { if (c) o[n++] = ','; n += (size_t)sprintf(o + n, quotes && st_below(2) ? "\"c%u\"" : "c%u", c); }
    o[n++] = '\n';
    for (U r = 0; r < nr && n + 4000 < cap; r++) {
        U cols = st_below(8) == 0 ? st_below(nc + 3) : nc;
        for (U c = 0; c < cols; c++) {
            if (c) o[n++] = ',';
            U k = c < nc ? kind[c] : 5;   /* clipped extras */
            U roll = st_below(100);
            char num[128];
            size_t m;
            if (roll < 8) continue;                                      /* empty */
            if (k == 0) m = (size_t)sprintf(num, "%lld", (long long)(st_next() % 2000000) - 1000000);
            else if (k == 1) m = (size_t)sprintf(num, "%.*f", (int)st_below(10), (double)(st_next() % 100000000) / 997.0 - 50000.0);
            else if (k == 2) { const char *z = st_odd[st_below((U)ST_NODD)]; m = strlen(z); memcpy(num, z, m); }
            else if (k == 6) {   /* random, but only strings strtod takes whole: stays numeric */
                char *end;
                m = st_number(num); num[m] = 0;
                strtod(num, &end);
                if (!m || end == num || *end) m = (size_t)sprintf(num, "%u", st_below(1000));
            }
            else if (k == 3) m = (size_t)sprintf(num, "%lld", (long long)(st_next() % 100));
            else m = (size_t)sprintf(num, "s%u", st_below(20));
            if (k == 3 && r == nr - 1 && roll < 50) m = (size_t)sprintf(num, "late");
            if (k == 0 && roll > 97) m = (size_t)sprintf(num, "%u.5", st_below(9));
            for (size_t i = 0; i < m; i++) if (num[i] == ',' || num[i] == '\n' || num[i] == '\r' || num[i] == '"') num[i] = '_';
            if (quotes && st_below(4) == 0) {
                o[n++] = '"';
                for (size_t i = 0; i < m; i++) { if (num[i] == '_' && st_below(3) == 0) { o[n++] = '"'; o[n++] = '"'; } else o[n++] = num[i]; }
                if (st_below(6) == 0) { o[n++] = '\n'; o[n++] = ','; }
                o[n++] = '"';
            } else { memcpy(o + n, num, m); n += m; }
        }
        const char *t = term[st_below((U)(sizeof term / sizeof *term))];
        size_t tl = strlen(t); memcpy(o + n, t, tl); n += tl;
    }
    return n;
}

int csv_selftest(void) {
    char path[64];   /* A name of this process's own (digest #80) */
#if !defined(wasm)
    snprintf(path, sizeof path, "/tmp/.amber_csv_selftest2.%d.csv", (int)getpid());
#else
    snprintf(path, sizeof path, "/tmp/.amber_csv_selftest2.csv");
#endif
    st_rng = 0x5eedcafe;
    csv_quiet = 1;
    long count = 300000;
    CO char *env = getenv("AMBER_CSV_NUMTEST");
    if (env && *env) count = (long)strtoll(env, 0, 10);
    int ok = st_numbers(count);
    static CO int threads[] = { 1, 2, 3, 5, 8 };
    for (size_t i = 0; ok && i < sizeof st_fix / sizeof *st_fix; i++) {
        if (!st_write(path, st_fix[i].s, st_fix[i].n)) { ok = 0; break; }
        for (size_t t = 0; ok && t < sizeof threads / sizeof *threads; t++) {
            char what[48]; sprintf(what, "fixture %u/%d", (unsigned)i, threads[t]);
            ok = check_file(path, threads[t], what);
        }
    }
    char *buf = (char *)malloc(64 << 10);
    for (int i = 0; ok && buf && i < 300; i++) {
        size_t n = st_csv(buf, 64 << 10);
        if (!st_write(path, buf, n)) { ok = 0; break; }
        char what[48]; sprintf(what, "random %d", i);
        ok = check_file(path, 1 + (int)st_below(9), what);
    }
    if (!buf) ok = 0;
    free(buf);
    if (!ok) fprintf(stderr, "csv0: failing input kept at %s\n", path);
    else remove(path);
    ok = ok && check_file("/nonexistent/.amber_csv_missing.csv", 0, "missing file");
    csv_quiet = 0;
    return ok;
}
