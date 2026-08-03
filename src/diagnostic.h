/* diagnostic.h  -  Amber Rust-style visual diagnostic error reports.
 * GNU AGPLv3 - see LICENSE and NOTICE.
 *
 * A Span pins a byte range inside a source string, together with the 1-based
 * line/column it starts on.  report_diagnostic() renders a Span (plus optional
 * secondary spans) as an ANSI-coloured, gutter-aligned report in the style of
 * the Rust compiler:
 *
 *   error[E0104]: Vector length mismatch
 *     --> test.k:12:8
 *      |
 *   12 |   prices + sizes
 *      |   ^^^^^^   ^^^^^
 *      |
 *      = help: Both vectors must have matching lengths for element-wise `+`.
 *
 * The renderer is pure C99 and writes into a caller-supplied buffer, so it can
 * feed a terminal, a string returned to the interpreter, or a test assertion.
 */
#ifndef AMBER_DIAGNOSTIC_H
#define AMBER_DIAGNOSTIC_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char *src;   /* full source-code string (NUL-terminated)     */
    uint32_t    start; /* start byte offset of the span                */
    uint32_t    end;   /* end byte offset (exclusive)                  */
    uint32_t    line;  /* 1-based line number of `start`               */
    uint32_t    col;   /* 1-based column of `start`                    */
} Span;

/* Build a Span for [start,end) in src, computing line/col from start. */
Span span_at(const char *src, uint32_t start, uint32_t end);

/* Render a diagnostic into buf (always NUL-terminated; returns the number of
 * bytes written, excluding the terminator, or the length it would have needed
 * had buf been large enough -- like snprintf).
 *   code       e.g. "E0104"        (shown as error[E0104])
 *   title      one-line message
 *   file       path shown in the `-->` locator (may be NULL -> "<amber>")
 *   primary    the main span; its line supplies the shown source line
 *   secondary  extra spans underlined on the same line (may be NULL)
 *   nsecondary count of secondary spans
 *   help       optional `= help:` note (may be NULL)
 *   color      non-zero -> emit ANSI SGR colour codes                       */
size_t report_diagnostic(char *buf, size_t buflen,
                         const char *code, const char *title,
                         const char *file, Span primary,
                         const Span *secondary, size_t nsecondary,
                         const char *help, int color);

/* Convenience: format with color and write to stderr. */
void report_diagnostic_stderr(const char *code, const char *title,
                              const char *file, Span primary,
                              const Span *secondary, size_t nsecondary,
                              const char *help);

#ifdef __cplusplus
}
#endif

#endif /* AMBER_DIAGNOSTIC_H */
