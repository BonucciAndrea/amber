/* csv.h  -  Amber fast native CSV parser.
 * GNU AGPLv3 - see LICENSE and NOTICE.
 * Requires a.h to already be included by the translation unit.
 *
 * Parses a CSV file straight into a genuine Amber table -- the same value
 * shape `([]col:vals;...)` literal syntax produces (a flipped {names;
 * values} dict, tag `tM`; verified against `@`/`meta`/`qselect`/`qwhere`
 * before this was written, see csv.c's header comment) -- with per-column
 * type inference (Long, Float, or Symbol) instead of leaving every column
 * as text.
 *
 * The file is mapped (or read into one buffer where mmap is unavailable),
 * split at row boundaries into one chunk per thread (AMBER_THREADS, see
 * parallel.h; files under 1 MB use one), and parsed straight into the final
 * column vectors: no per-field table, no second pass over the cells. Peak
 * memory is about the columns themselves; parsed input pages are handed
 * back to the kernel as the reader goes. See csv.c for the phases.
 *
 * Parsing rules (a practical RFC 4180 subset, not a full implementation):
 *   - fields are comma-separated; rows are separated by "\n" or "\r\n"
 *   - a field may be double-quoted ("like this"); a doubled quote ("")
 *     inside a quoted field is an escaped literal quote
 *   - the first row is the header (column names)
 *   - a column is typed Long if every non-empty cell parses as a whole
 *     number, Float if every non-empty cell parses as a number (with a
 *     decimal point/exponent, or the Long check failed), otherwise Symbol
 *   - an empty cell becomes that column's null: 0N (Long), 0n (Float), or
 *     the empty symbol `` ` `` (Symbol)
 *   - rows longer/shorter than the header are clipped/padded with nulls
 *     rather than raising an error, so one malformed line doesn't abort
 *     an otherwise-good file
 */
#ifndef AMBER_CSV_H
#define AMBER_CSV_H

#include <stdint.h>   /* uint64_t in csv_fast (a.h does not bring it on every platform) */
#include <float.h>    /* FLT_EVAL_METHOD */

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
#if CSV_FASTF && defined(__SIZEOF_INT128__)
#define CSV_EL 1
#else
#define CSV_EL 0
#endif

/* \csvr "path.csv" or `csvr[path]: read the file at `path` and return a
 * table (same shape as `([]col:vals;...)`). Returns the generic null atom
 * (au) and prints a message to stderr if the file cannot be opened. */
A csv_read(S path);

/* `csvx "path.csv": reads the file with both csv_read() and the 2.2.0
 * reference reader kept in csv.c and returns 1 iff the two agree bit for bit
 * (names, types, lengths, every byte of every column); the first difference
 * is printed to stderr. */
int csv_check(S path);

/* `csv0's C half: the number parsers against strtoll/strtod on random and
 * edge-case strings (AMBER_CSV_NUMTEST=n sets how many), then csv_check()
 * over a fixture battery and random files at 1..9 forced chunks. 1 = pass. */
int csv_selftest(void);

/* The unsigned decimal at [s,t), correctly rounded (the Eisel-Lemire method on its
 * first 19 significant digits where that decides it, else strtod); the literal and
 * JSON readers call it when csv_fast, csv_span or csv_long cannot decide. */
F csv_float(const char *s, const char *t);

/* For a number of more than 19 digits, w its first 19 (leading zeros among them) and q the
 * power of ten that scales them: 1 and *v, correctly rounded, when w*10^q and (w+1)*10^q
 * round to the same double (the Eisel-Lemire method), else 0 and the caller asks csv_float. */
int csv_span(uint64_t w, long long q, F *v);

/* csv_span for a reader that has scanned a number of more than 19 digits: i integer digits at
 * b, the fraction's at f, the number 0.d1d2d3... times 10^e. */
int csv_long(const char *b, long long i, const char *f, long long e, F *v);

#if CSV_FASTF
static const double csv_p10[23] = {
    1e0, 1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8, 1e9, 1e10, 1e11,
    1e12, 1e13, 1e14, 1e15, 1e16, 1e17, 1e18, 1e19, 1e20, 1e21, 1e22 };
#endif
#if CSV_EL
/* w*10^q, correctly rounded, for w of at most 19 digits: the Eisel-Lemire method (src/csv.c), for
 * what Clinger's fast path cannot take. Out of line, so the readers' short path stays small. */
F csv_elf(uint64_t w, long long q);
#endif

/* w*10^dx when Clinger's fast path takes it (*v, 1): w itself when dx is 0, else one exact
 * IEEE multiply or divide, for w <= 2^53 and |dx| <= 22. 0 otherwise, and where it is not built. */
static inline int csv_cl(uint64_t w, long long dx, F *v) {
#if CSV_FASTF
    if (w <= ((uint64_t)1 << 53)) {
        if (!dx) { *v = (double)w; return 1; }   /* an integer: exact, no power of ten */
        if (dx >= -22 && dx <= 22) { *v = dx < 0 ? (double)w / csv_p10[-dx] : (double)w * csv_p10[dx]; return 1; }
    }
#endif
    (void)w; (void)dx; (void)v;
    return 0;
}

/* w*10^dx, correctly rounded, for w of at most 19 digits (*v, 1): csv_cl inline, else the
 * Eisel-Lemire method out of line (csv_elf); 0 only where neither is built (wasm, or no 128-bit
 * integers), and then the caller asks csv_float. Inline and small, so that the JSON reader
 * takes Clinger's path without a call. */
static inline int csv_fast(uint64_t w, long long dx, F *v) {
    if (csv_cl(w, dx, v)) return 1;
#if CSV_EL
    *v = csv_elf(w, dx); return 1;
#elif CSV_FASTF
    if (!w) { *v = 0.0; return 1; }
#endif
    return 0;
}

#endif /* AMBER_CSV_H */
