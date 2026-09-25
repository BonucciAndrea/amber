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

/* The fast float path relies on one IEEE multiply or divide being correctly
 * rounded, which needs doubles evaluated at double precision (not x87's
 * extended format). The wasm shim's strtod is not the platform one either,
 * so keep that build on strtod alone: its answers then stay whatever they
 * were before. */
#if !defined(wasm) && defined(FLT_EVAL_METHOD) && FLT_EVAL_METHOD == 0
#define CSV_FASTF 1
#else
#define CSV_FASTF 0
#endif

/* The self-test feeds both readers missing and empty files on purpose. */
static int csv_quiet;
#define CSV_MSG(...) do { if (!csv_quiet) fprintf(stderr, __VA_ARGS__); } while (0)

/* ======================================================================
 * Reference reader:the 2.2.0 implementation, unchanged apart from names
 * and returning its two halves instead of the table. Only the self-test
 * and `csvx call it. It is the oracle csv_cols() must match bit for bit.
 * ====================================================================== */

static char *ref_field(char **pp, char *delim) {
    char *p = *pp;
    char *out = p;
    if (*p == '"') {
        char *w = p, *r = p + 1;
        while (*r && !(*r == '"' && r[1] != '"')) {
            if (*r == '"' && r[1] == '"') { *w++ = '"'; r += 2; }
            else *w++ = *r++;
        }
        if (*r == '"') r++;
        *w = 0;
        *delim = *r;
        *pp = r;
        return out;
    }
    while (*p && *p != ',' && *p != '\n' && *p != '\r') p++;
    *delim = *p;
    *p = 0;
    *pp = p;
    return out;
}

static char ***ref_grid(char *data, U *nrows_out, U *ncols_out) {
    /* 2.2.0 counted only '\n' here, but a lone '\r' ends a row too, so a file
     * with old-Mac line endings wrote past this table (heap corruption, then
     * garbage columns). Counting both bytes is a true upper bound. */
    U maxlines = 1;
    for (char *p = data; *p; p++) if (*p == '\n' || *p == '\r') maxlines++;
    char ***rows = (char ***)arena_alloc(maxlines * sizeof(char **));
    U nrows = 0;
    char *p = data;
    if ((unsigned char)p[0] == 0xEF && (unsigned char)p[1] == 0xBB && (unsigned char)p[2] == 0xBF) p += 3;
    U ncols = 0;
    while (*p) {
        if (*p == '\r' && p[1] == '\n') { p += 2; continue; }
        if (*p == '\n') { p++; continue; }
        U cap = ncols ? ncols : 32, cnt = 0;
        char **fields = (char **)arena_alloc(cap * sizeof(char *));
        char delim = 0;
        for (;;) {
            char *f = ref_field(&p, &delim);
            if (cnt >= cap) {
                char **grown = (char **)arena_alloc(cap * 2 * sizeof(char *));
                memcpy(grown, fields, cnt * sizeof(char *));
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
            char **padded = (char **)arena_alloc(ncols * sizeof(char *));
            memcpy(padded, fields, cnt * sizeof(char *));
            for (U i = cnt; i < ncols; i++) padded[i] = (char *)"";
            fields = padded;
        }
        rows[nrows++] = fields;
    }
    *nrows_out = nrows;
    *ncols_out = ncols;
    return rows;
}

enum { COL_LONG, COL_FLOAT, COL_SYM };

static int ref_is_long(const char *s, long long *out) {
    if (!*s) return 1;
    char *end;
    long long v = strtoll(s, &end, 10);
    if (end == s || *end) return 0;
    *out = v;
    return 1;
}
static int ref_is_float(const char *s, double *out) {
    if (!*s) return 1;
    char *end;
    double v = strtod(s, &end);
    if (end == s || *end) return 0;
    *out = v;
    return 1;
}
static int ref_classify(char **rows_col, U nrows) {
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
    char ***rows = ref_grid(data, &nrows_total, &ncols);
    if (nrows_total == 0 || ncols == 0) {
        arena_reset();
        CSV_MSG("csv: '%s' is empty\n", path);
        return 0;
    }
    char **header = rows[0];
    U nrows = nrows_total - 1;
    A names = aS(ncols);
    I *namev = (I *)_V(names);
    for (U c = 0; c < ncols; c++) namev[c] = (I)sym(header[c]);
    A cols = aA(ncols);
    A *colv = _A(cols);
    char **coldata = (char **)arena_alloc((nrows ? nrows : 1) * sizeof(char *));
    for (U c = 0; c < ncols; c++) {
        for (U r = 0; r < nrows; r++) coldata[r] = rows[r + 1][c];
        int kind = nrows ? ref_classify(coldata, nrows) : COL_SYM;
        A vec;
        if (kind == COL_LONG) {
            vec = aL(nrows); L *v = _V(vec);
            for (U r = 0; r < nrows; r++) {
                long long li;
                v[r] = (*coldata[r] && ref_is_long(coldata[r], &li)) ? (L)li : NL;
            }
        } else if (kind == COL_FLOAT) {
            vec = aF(nrows); F *v = _V(vec);
            for (U r = 0; r < nrows; r++) {
                double fv;
                v[r] = (*coldata[r] && ref_is_float(coldata[r], &fv)) ? (F)fv : NF;
            }
        } else {
            vec = aS(nrows); I *v = (I *)_V(vec);
            for (U r = 0; r < nrows; r++) v[r] = (I)sym(coldata[r]);
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
 * terminator; the reference stops at the first NUL byte, so the effective
 * length here is the offset of the first NUL (or the file size).
 *   row start  skip "\n" and "\r\n" (blank lines); a lone "\r" there is a
 *              row holding one empty field
 *   field      '"' opens a quoted field that runs to a '"' not followed by
 *              another '"'; "" inside is a literal quote. Anything else runs
 *              to ',', '\n', '\r' or the end
 *   after it   ',' starts the next field; "\r\n", "\r" or "\n" ends the row;
 *              any other byte (possible only after a closing quote) ends the
 *              row too and is where the next row starts
 * ====================================================================== */

enum { CM_LONG, CM_FLOAT, CM_SYM };           /* ordered: promotion only goes up */
#define NLB ((uint64_t)1 << 63)               /* bits of NL */
#define NFB ((uint64_t)0x7ff8000000000000ull) /* bits of NF (NFL, a.h) */
#define CSV_DROP ((size_t)16 << 20)           /* release mapped input every 16 MB */
#define CSV_NONE ((size_t)-1)                /* no NUL / no quote seen (the wasm libc has no SIZE_MAX) */

typedef struct {
    const char *b;      /* file bytes */
    size_t len;         /* effective length (first NUL or file size) */
    size_t fsz;         /* mapped/read size */
    int mapped;
    size_t page;
    U nc;               /* column count (the header's field count) */
    int nk;             /* chunks */
    size_t cb[PAR_MAX_THREADS + 1];   /* chunk byte bounds */
    size_t cr[PAR_MAX_THREADS + 1];   /* first row of each chunk; cr[nk] = rows */
    size_t nul[PAR_MAX_THREADS], quo[PAR_MAX_THREADS], cnt[PAR_MAX_THREADS];
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

/* Lexes the field starting at p. [*s,*t) is its content with the quotes
 * stripped; *esc says it holds "" pairs still to be collapsed. Returns the
 * position of the byte that ended it (== e at the end). */
static const char *lex_field(const char *p, const char *e, const char **s, const char **t, int *esc) {
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

#if CSV_FASTF
static const double csv_p10[23] = {
    1e0, 1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8, 1e9, 1e10, 1e11,
    1e12, 1e13, 1e14, 1e15, 1e16, 1e17, 1e18, 1e19, 1e20, 1e21, 1e22 };
#endif

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
 * string must be consumed. */
static int libc_num(const char *s, size_t n, int fl, L *lv, F *fv) {
    char sb[128];
    char *buf = n < sizeof sb ? sb : (char *)malloc(n + 1);
    if (!buf) return 0;
    memcpy(buf, s, n); buf[n] = 0;
    char *end; int ok;
    if (fl) { double d = strtod(buf, &end); ok = end != buf && !*end; if (ok) *fv = d; }
    else { long long d = strtoll(buf, &end, 10); ok = end != buf && !*end; if (ok) *lv = (L)d; }
    if (buf != sb) free(buf);
    return ok;
}

/* Non-empty content [s,t) as a Long / Float, or 0 if the reference would
 * reject it. */
static int cell_long(const char *s, const char *t, L *v, unsigned char *uns) {
    int nz;
    if (fast_long(s, t, v, &nz) == t) { if (nz) *uns = 1; return 1; }
    if (libc_num(s, (size_t)(t - s), 0, v, 0)) { *uns = 1; return 1; }
    return 0;
}
static int cell_float(const char *s, const char *t, F *v) {
    if (fast_float(s, t, v) == t) return 1;
    return libc_num(s, (size_t)(t - s), 1, 0, v);
}

static inline uint64_t null_bits(int m) { return m == CM_LONG ? NLB : m == CM_FLOAT ? NFB : 0; }

/* ---- phase 0: find NULs and quotes, count rows ------------------------- */

/* Counts chunk k's rows assuming it holds no quote (true unless quo[] says
 * otherwise, and then the count is discarded). Stops at a NUL. */
static void phase0(void *ctx, int k) {
    Csv *cv = (Csv *)ctx;
    const char *b = cv->b, *p = b + cv->cb[k], *ce = b + cv->cb[k + 1], *fe = b + cv->len;
    size_t rows = 0, nul = CSV_NONE, quo = CSV_NONE, dropped = cv->cb[k];
    while (p < ce) {
        char ch = *p;
        if (ch == '\n') { p++; continue; }
        if (ch == '\r' && p + 1 < fe && p[1] == '\n') { p += 2; continue; }
        if (!ch) { nul = (size_t)(p - b); break; }
        rows++;
        for (; p < ce; p++) {
            ch = *p;
            if (ch == '\n' || ch == '\r') break;
            if (ch == '"') { if (quo == CSV_NONE) quo = (size_t)(p - b); }
            else if (!ch) { nul = (size_t)(p - b); goto done; }
        }
        if (p < ce) {
            if (*p == '\r') { p++; if (p < fe && *p == '\n') p++; }
            else p++;
        }
        if ((size_t)(p - b) - dropped >= CSV_DROP) { drop_pages(cv, dropped, (size_t)(p - b)); dropped = (size_t)(p - b); }
    }
done:
    drop_pages(cv, dropped, cv->cb[k + 1]);
    cv->nul[k] = nul; cv->quo[k] = quo; cv->cnt[k] = rows;
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

restart:;
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
    /* phase 0 below looks for NULs in the data only; the header is ours */
    const char *hn = ds ? (const char *)memchr(b, 0, ds) : 0;
    if (hn) { cv.len = (size_t)(hn - b); goto restart; }

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

    /* the reference stops at the first NUL byte */
    size_t nul = CSV_NONE;
    int kn = 0;
    for (; kn < nk; kn++) if (cv.nul[kn] != CSV_NONE) { nul = cv.nul[kn]; break; }
    if (nul != CSV_NONE) {
        cv.len = nul;
        if (nul < ds) goto restart;  /* inside the header: start over on the shorter file */
        for (int k = 0; k <= nk; k++) if (cv.cb[k] > nul) cv.cb[k] = nul;
        for (int k = kn + 1; k < nk; k++) cv.cnt[k] = 0;
    }
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
    if (!csv_cols(path, &names, &cols)) return au;
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
    "12345678901234567890e-5", "0.1", "0.3", "2.2250738585072011e-308", "2.2250738585072014e-308", "+.5", "5." };
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
    U lead = st_below(5) == 0 ? st_below(4) : 0;
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
        long long rl = strtoll(s, &end, 10); int okl = end != s && !*end;
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
    FX("q\n\"ab\"cd,ef\n1\n"),            /* bytes after a closing quote start a new row */
    FX("q,r\n\"unterminated,1\n2,3\n"),
    FX("\"h1\",\"h,2\"\n1,2\n"),          /* quoted header */
    FX("\"multi\nline header\",b\n1,2\n"),
    FX("f\n-0.0\n0\n"),
    FX("f\n-0\n1.5\n"),
    FX("f\n1e-320\n4.9e-324\n1e308\n1e309\n-1e309\n"),
    FX("i\n9007199254740993\n9223372036854775807\n"),
    FX("i\n99999999999999999999\n-99999999999999999999\n"),
    FX("i,f\n+5,+1.5\n-7,-.5\n"),
    FX("a,b\n,\n,\n"),                    /* all-empty columns */
    FX("m\n1\n2\n3.5\n4\n"),              /* int then float */
    FX("m\n1\n2\n3\nx\n"),                /* symbol on the last row */
    FX("m\n1.5\n2\n\"3\"\nnan\ninf\n0x10\n"),
    FX("m\n 5\n6 \n"),                    /* whitespace: strtoll takes the leading, not the trailing */
    FX("m\n\" 7\"\n\"\n8\"\n"),
    FX("a,b\n1,2\n3,\"4\"\"\"\n"),
    FX("a,b\n1,2\n3\x00" "4,5\n6,7\n"),   /* NUL: the reference stops there */
    FX("a\x00" "b,c\n1,2\n"),
    FX("\x00" "a,b\n"),
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
    CO char *path = "/tmp/.amber_csv_selftest2.csv";
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
