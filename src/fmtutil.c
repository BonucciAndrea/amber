/* fmtutil.c  -  see fmtutil.h. */
#include "fmtutil.h"
#include <stdio.h>

void fmt_bytes(double bytes, char *buf, size_t buflen) {
    if (bytes >= 1024.0 * 1024.0)
        snprintf(buf, buflen, "%.1f MB", bytes / (1024.0 * 1024.0));
    else if (bytes >= 1024.0)
        snprintf(buf, buflen, "%.1f KB", bytes / 1024.0);
    else
        snprintf(buf, buflen, "%.0f B", bytes);
}
