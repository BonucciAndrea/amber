/* inspect.c  -  Amber rich workspace variable inspector ("\v"). See inspect.h.
 *
 * Uses only the parameterized K-value accessor macros from a.h/g.h (_t, _n,
 * _b, _w, _a, _x, _y, _t0, TR, TU, HD) so it never depends on a local
 * variable happening to be named `x` -- every macro call below names its
 * own operand explicitly.
 *
 * Amber, like most K/q dialects, represents an (unkeyed) table as *the same*
 * heap tag as a plain dict (tM): a 2-element {names; values} pair where the
 * values just happen to be a list of equal-length column vectors keyed by
 * symbol names. The distinct `tm` tag shows up for some dict/table variants
 * built through the `!` verb. Since "is this a table or a dict" is a
 * structural question, not a tag question, classify() below inspects the
 * {keys,values} pair directly instead of switching on the tag alone.
 */
#include "a.h"
#include "inspect.h"
#include "fmtutil.h"
#include <stdio.h>
#include <string.h>

#define IV_MAX 4096
typedef struct {
    char name[64];
    char type[24];
    char shape[64]; /* wide enough for comma-formatted "rows x cols" even
                      * when both dimensions run into the billions */
    char mem[16];
} IvRow;

static IvRow g_rows[IV_MAX];
static int   g_nrows = 0;

/* ---- formatting helpers ------------------------------------------------ */

static void fmt_int_commas(long long n, char *buf, size_t buflen) {
    char digits[32];
    int neg = n < 0;
    unsigned long long u = neg ? (unsigned long long)(-n) : (unsigned long long)n;
    int di = 0;
    do { digits[di++] = (char)('0' + u % 10); u /= 10; } while (u);
    size_t bi = 0;
    if (neg && bi + 1 < buflen) buf[bi++] = '-';
    for (int k = di - 1; k >= 0; k--) {
        if (bi + 1 >= buflen) break;
        buf[bi++] = digits[k];
        if (k > 0 && k % 3 == 0) buf[bi++] = ',';
    }
    buf[bi < buflen ? bi : buflen - 1] = 0;
}

static const char *iv_typename(UC t) {
    switch (t) {
        case tA:  return "List";
        case tE:  return "Range";
        case tB:  return "Boolean Vector";
        case tG:  return "Byte Vector";
        case tH:  return "Short Vector";
        case tI:  return "Int Vector";
        case tL:  return "Long Vector";
        case tF:  return "Float Vector";
        case tC:  return "Char Vector";
        case tS:  return "Symbol Vector";
        case tM:  return "Dict";
        case tm:  return "Dict";
        case ti:  return "Int Atom";
        case tl:  return "Long Atom";
        case tf:  return "Float Atom";
        case tc:  return "Char Atom";
        case ts:  return "Symbol Atom";
        case tdt: return "Date Atom";
        case ttm: return "Time Atom";
        case tnp: return "Timestamp Atom";
        default:  return "Value";
    }
}

/* ---- deep memory footprint --------------------------------------------- */
/* Sums the bucket-allocator block size (HD<<bucket) of every heap object
 * reachable from `v`, recursing into ref-holding lists (general lists,
 * dicts, tables -- all stored as arrays of child A values). Packed atoms
 * (int/char/symbol/date/time atoms etc.) own no heap block of their own, so
 * they contribute sizeof(A) -- the tagged word itself.
 *
 * Note: this is a straightforward sum over the reachable graph. Amber's
 * arrays are refcounted/COW, so a value that shares a sub-array with
 * another (already-printed) variable will have that sub-array counted in
 * both -- the same convention most "deep size" tools use. */
static size_t iv_deepsize(A v, int depth) {
    if (_t0(v) || depth > 32) return sizeof(A);
    size_t bucket = (size_t)HD << _b(v);
    size_t raw = (size_t)HD + (((((size_t)_n(v)) << _w(v)) + 7) >> 3);
    size_t bytes = raw > bucket ? raw : bucket;
    if (TR(_t(v))) {
        U n = _n(v);
        for (U i = 0; i < n; i++) bytes += iv_deepsize(_a(v), depth + 1);
    }
    return bytes;
}

/* ---- dict / table classification ---------------------------------------- */

/* first element of a ref-holding array (no reliance on a loop index `i`). */
static A _a1(A v) { return _A(v)[0]; }

/* `v` looks like a table iff it's a 2-element {names;values} pair whose
 * names are symbols and whose values are a non-empty ref-holding list (one
 * entry per column) with a first column we can measure. On success, fills
 * *rows / *cols and returns 1; otherwise returns 0 (plain dict, or something
 * with a nonconforming shape). */
static int iv_as_table(A v, long long *rows, long long *cols) {
    UC t = _t(v);
    if ((t != tM && t != tm) || _t0(v) || _n(v) != 2) return 0;
    A names = _x(v), vals = _y(v);
    if (_t0(names) || _t(names) != tS) return 0;
    if (_t0(vals) || !TR(_t(vals)) || _n(vals) == 0) return 0;
    if (_n(vals) != _n(names)) return 0;
    A c0 = _a1(vals);
    if (_t0(c0)) return 0;
    *cols = (long long)_n(names);
    *rows = (long long)_n(c0);
    return 1;
}

static void iv_shape(A v, char *buf, size_t buflen) {
    UC t = _t(v);
    long long rows, cols;
    if ((t == tM || t == tm) && iv_as_table(v, &rows, &cols)) {
        char r[24], c[24];
        fmt_int_commas(rows, r, sizeof r);
        fmt_int_commas(cols, c, sizeof c);
        snprintf(buf, buflen, "%s x %s", r, c);
    } else if (t == tM || t == tm) {
        A keys = _x(v);
        long long nk = _t0(keys) ? 1 : (long long)_n(keys);
        char tmp[24];
        fmt_int_commas(nk, tmp, sizeof tmp);
        snprintf(buf, buflen, "%s keys", tmp);
    } else if (TU(t) || t > tm) {
        snprintf(buf, buflen, "atom");
    } else {
        char tmp[24];
        fmt_int_commas(_t0(v) ? 1 : (long long)_n(v), tmp, sizeof tmp);
        snprintf(buf, buflen, "%s", tmp);
    }
}

/* ---- public API ---------------------------------------------------------- */

void iv_begin(void) { g_nrows = 0; }

void iv_add(const char *name, A v) {
    if (g_nrows >= IV_MAX) return;
    UC t = _t(v);
    if (TU(t)) return; /* functions: \f's job, not \v's */
    IvRow *r = &g_rows[g_nrows++];
    snprintf(r->name, sizeof r->name, "%s", name);
    long long rows, cols;
    snprintf(r->type, sizeof r->type, "%s",
        (t == tM || t == tm) && iv_as_table(v, &rows, &cols) ? "Table" : iv_typename(t));
    iv_shape(v, r->shape, sizeof r->shape);
    fmt_bytes((double)iv_deepsize(v, 0), r->mem, sizeof r->mem);
}

void iv_print(void) {
    if (!g_nrows) { fputs("(empty workspace)\n", stdout); return; }

    static const char *H_NAME = "Name", *H_TYPE = "Type",
                       *H_SHAPE = "Shape / Length", *H_MEM = "Memory";
    size_t wn = strlen(H_NAME), wt = strlen(H_TYPE),
           ws = strlen(H_SHAPE), wm = strlen(H_MEM);
    for (int i = 0; i < g_nrows; i++) {
        size_t n = strlen(g_rows[i].name);   if (n > wn) wn = n;
        size_t t = strlen(g_rows[i].type);   if (t > wt) wt = t;
        size_t s = strlen(g_rows[i].shape);  if (s > ws) ws = s;
        size_t m = strlen(g_rows[i].mem);    if (m > wm) wm = m;
    }

    char rule[512];
    snprintf(rule, sizeof rule, "+-%.*s-+-%.*s-+-%.*s-+-%.*s-+\n",
        (int)wn, "--------------------------------------------------------------",
        (int)wt, "--------------------------------------------------------------",
        (int)ws, "--------------------------------------------------------------",
        (int)wm, "--------------------------------------------------------------");

    fputs(rule, stdout);
    printf("| %-*s | %-*s | %-*s | %-*s |\n",
        (int)wn, H_NAME, (int)wt, H_TYPE, (int)ws, H_SHAPE, (int)wm, H_MEM);
    fputs(rule, stdout);
    for (int i = 0; i < g_nrows; i++)
        printf("| %-*s | %-*s | %-*s | %-*s |\n",
            (int)wn, g_rows[i].name, (int)wt, g_rows[i].type,
            (int)ws, g_rows[i].shape, (int)wm, g_rows[i].mem);
    fputs(rule, stdout);

    g_nrows = 0;
}
