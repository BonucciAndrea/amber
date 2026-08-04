/* csv.c  -  see csv.h.
 * GNU AGPLv3 - see LICENSE and NOTICE.
 *
 * Table shape verified interactively before writing this parser:
 *   amber> d:`a`b!(1 2 3;10 20 30)
 *   amber> t:+d
 *   amber> @t                 / `M  -- same tag a `([]a:..;b:..)` literal gets
 *   amber> meta t             / renders a real column/type/attribute table
 *   amber> qwhere[t;t[`a]>1]  / qSQL functional forms work on it unmodified
 * so csv_read() builds exactly that: `exc(names,cols)` (the `!` dyad, a.h)
 * to make the {names;values} dict, then `flp()` (a.h) to flip it into a
 * table -- the identical two calls `names!values` followed by `+` make at
 * the prompt. Neither of those two calls is touched or reimplemented here.
 */
#include "a.h"
#include "arena.h"
#include "csv.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- pass 1: split the raw file into NUL-terminated fields in place ---- */

/* Advances *pp past one field starting at *pp, NUL-terminating it in place
 * and returning a pointer to its start. The delimiter that ended the field
 * (',', '\n', '\r', or 0 at end-of-file) is written to *delim -- the caller
 * MUST consult *delim rather than re-reading **pp, because the unquoted
 * path below overwrites that very byte with a NUL to terminate the field
 * (so **pp reads back as 0 regardless of what the real delimiter was).
 * Handles a double-quoted field, collapsing "" to a literal ". */
static char *read_field(char **pp, char *delim) {
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
        *delim = *r;   /* not mutated on this path: safe to read directly */
        *pp = r;
        return out;
    }
    while (*p && *p != ',' && *p != '\n' && *p != '\r') p++;
    *delim = *p;       /* capture before the NUL-terminate below clobbers it */
    *p = 0;
    *pp = p;
    return out;
}

/* Splits `data` (mutated in place) into a row/field grid. nrows_out and
 * ncols_out are set; returns an arena-allocated char*** — rows[r][c] — valid until
 * the next arena_reset(). Every row is padded/clipped to the header's
 * column count. Returns 0 (and leaves *nrows==0) on an empty file. */
static char ***split_grid(char *data, U *nrows_out, U *ncols_out) {
    /* count raw lines first (cheap upper bound for the row-pointer array) */
    U maxlines = 1;
    for (char *p = data; *p; p++) if (*p == '\n') maxlines++;

    char ***rows = (char ***)arena_alloc(maxlines * sizeof(char **));
    U nrows = 0;
    char *p = data;

    /* skip a leading UTF-8 BOM if present */
    if ((unsigned char)p[0] == 0xEF && (unsigned char)p[1] == 0xBB && (unsigned char)p[2] == 0xBF) p += 3;

    U ncols = 0;
    while (*p) {
        if (*p == '\r' && p[1] == '\n') { p += 2; continue; }
        if (*p == '\n') { p++; continue; }

        /* one row: gather fields until end-of-line. `delim` is the real
         * delimiter each read_field() call consumed, captured before that
         * byte in `data` gets overwritten with a NUL -- see read_field(). */
        U cap = ncols ? ncols : 32, cnt = 0;
        char **fields = (char **)arena_alloc(cap * sizeof(char *));
        char delim = 0;
        for (;;) {
            char *f = read_field(&p, &delim);
            if (cnt >= cap) {
                char **grown = (char **)arena_alloc(cap * 2 * sizeof(char *));
                memcpy(grown, fields, cnt * sizeof(char *));
                fields = grown; cap *= 2;
            }
            fields[cnt++] = f;
            if (delim == ',') { p++; continue; }
            break; /* '\n', '\r', or end of file */
        }
        if (delim == '\r') { p++; if (*p == '\n') p++; }
        else if (delim == '\n') p++;

        if (!ncols) { ncols = cnt; }
        /* pad/clip this row to `ncols` fields */
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

/* ---- pass 2: per-column type inference + typed vector construction ---- */

enum { COL_LONG, COL_FLOAT, COL_SYM };

static int cell_is_long(const char *s, long long *out) {
    if (!*s) return 1; /* empty cell: still compatible, becomes null later */
    char *end;
    long long v = strtoll(s, &end, 10);
    if (end == s || *end) return 0;
    *out = v;
    return 1;
}
static int cell_is_float(const char *s, double *out) {
    if (!*s) return 1;
    char *end;
    double v = strtod(s, &end);
    if (end == s || *end) return 0;
    *out = v;
    return 1;
}

static int classify_column(char **rows_col, U nrows) {
    int could_long = 1, could_float = 1;
    for (U r = 0; r < nrows; r++) {
        long long li; double fv;
        if (could_long && !cell_is_long(rows_col[r], &li)) could_long = 0;
        if (could_float && !cell_is_float(rows_col[r], &fv)) could_float = 0;
        if (!could_long && !could_float) break;
    }
    if (could_long) return COL_LONG;
    if (could_float) return COL_FLOAT;
    return COL_SYM;
}

/* rows is the full grid; picks out column `c` into `buf` (arena scratch). */
static char **column_of(char ***rows, U nrows, U ncols, U c) {
    char **buf = (char **)arena_alloc(nrows * sizeof(char *));
    for (U r = 0; r < nrows; r++) buf[r] = rows[r][c];
    (void)ncols;
    return buf;
}

A csv_read(S path) {
    FILE *fp = fopen(path, "rb");
    if (!fp) { fprintf(stderr, "csv: cannot open '%s'\n", path); return au; }
    fseek(fp, 0, SEEK_END);
    long fsz = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if (fsz < 0) { fclose(fp); fprintf(stderr, "csv: cannot stat '%s'\n", path); return au; }

    arena_reset();
    char *data = (char *)arena_alloc((size_t)fsz + 1);
    size_t got = fread(data, 1, (size_t)fsz, fp);
    fclose(fp);
    data[got] = 0;

    U nrows_total, ncols;
    char ***rows = split_grid(data, &nrows_total, &ncols);
    if (nrows_total == 0 || ncols == 0) {
        arena_reset();
        fprintf(stderr, "csv: '%s' is empty\n", path);
        return au;
    }
    char **header = rows[0];
    U nrows = nrows_total - 1; /* data rows, excluding the header */

    A names = aS(ncols);
    I *namev = (I *)_V(names);
    for (U c = 0; c < ncols; c++) namev[c] = (I)sym(header[c]);

    A cols = aA(ncols);
    A *colv = _A(cols);
    for (U c = 0; c < ncols; c++) {
        char **coldata = column_of(rows + 1, nrows, ncols, c);
        int kind = nrows ? classify_column(coldata, nrows) : COL_SYM;
        A vec;
        if (kind == COL_LONG) {
            vec = aL(nrows); L *v = _V(vec);
            for (U r = 0; r < nrows; r++) {
                long long li;
                v[r] = (*coldata[r] && cell_is_long(coldata[r], &li)) ? (L)li : NL;
            }
        } else if (kind == COL_FLOAT) {
            vec = aF(nrows); F *v = _V(vec);
            for (U r = 0; r < nrows; r++) {
                double fv;
                v[r] = (*coldata[r] && cell_is_float(coldata[r], &fv)) ? (F)fv : NF;
            }
        } else {
            vec = aS(nrows); I *v = (I *)_V(vec);
            for (U r = 0; r < nrows; r++) v[r] = (I)sym(coldata[r]);
        }
        colv[c] = vec;
    }

    A dict = exc(names, cols);   /* names ! cols  -- the real `!` dyad (a.h) */
    A tbl = flp(dict);           /* +dict          -- the real flip verb (a.h) */
    arena_reset();
    return tbl;
}
