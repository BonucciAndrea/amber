/* inspect.h  -  Amber rich workspace variable inspector ("\v").
 * GNU AGPLv3 - see LICENSE and NOTICE.
 *
 * Renders the active global workspace as a formatted ASCII table:
 *
 *   +----------+----------------+----------------+-----------+
 *   | Name     | Type           | Shape / Length | Memory    |
 *   +----------+----------------+----------------+-----------+
 *   | prices   | Float Vector   | 10,000,000     | 76.3 MB   |
 *   | t        | Table          | 1,000,000 x 3  | 22.9 MB   |
 *   +----------+----------------+----------------+-----------+
 *
 * This module knows how to classify and measure a single Amber K value
 * (type name, shape/length, deep memory footprint) and how to lay the
 * results out as a table. It deliberately does NOT know how to walk the
 * global symbol table (gk/gv/gn are file-local to m.c) -- the caller
 * (bsv() in m.c) iterates its own globals and feeds each one in via
 * iv_add(); this file only needs a.h for the K-value accessor macros.
 */
#ifndef AMBER_INSPECT_H
#define AMBER_INSPECT_H

/* Requires a.h to already be included by the translation unit (for the `A`
 * K-value type). Not included here directly because a.h has no #include
 * guard and is designed to be pulled in exactly once per .c file. */

#ifdef __cplusplus
extern "C" {
#endif

/* Clear any rows queued from a previous \v invocation. */
void iv_begin(void);

/* Classify K value `x` and queue it as a row named `name`. Function-typed
 * values (to..tx) are skipped -- \v lists data, \f lists functions.
 * `name` is copied, not retained. */
void iv_add(const char *name, A x);

/* Render every queued row as an ASCII table and print it to stdout, then
 * clear the queue. No-op (prints "(empty workspace)") if iv_add() was
 * never called. */
void iv_print(void);

#ifdef __cplusplus
}
#endif

#endif /* AMBER_INSPECT_H */
