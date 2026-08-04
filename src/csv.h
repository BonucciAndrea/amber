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
 * The file's bytes and the transient per-field pointer table are read
 * through arena_alloc() (arena.h) -- one bump-allocated scratch region,
 * released with a single arena_reset() when parsing finishes -- rather
 * than a malloc() per field/row, matching how src/a.c's ajc() (the as-of
 * join kernel) uses the arena for its own transient work. Only the final
 * typed column vectors (built with aL()/aF()/aS()) and the table they are
 * assembled into are real, persistent, refcounted heap values.
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

#endif /* AMBER_CSV_H */
