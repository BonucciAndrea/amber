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

#endif /* AMBER_CSV_H */
