/* fmtutil.h  -  small shared formatting helpers for Amber's REPL diagnostic
 * commands (\v, \trace). GNU AGPLv3 - see LICENSE and NOTICE.
 *
 * fmt_bytes() was previously duplicated near-verbatim in inspect.c and
 * trace.c; it now lives here once so both report memory the same way.
 */
#ifndef AMBER_FMTUTIL_H
#define AMBER_FMTUTIL_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Human-scale a byte count as "<value> <unit>" into buf, choosing B/KB/MB. */
void fmt_bytes(double bytes, char *buf, size_t buflen);

#ifdef __cplusplus
}
#endif

#endif /* AMBER_FMTUTIL_H */
