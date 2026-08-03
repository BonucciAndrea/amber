/* badge.c  -  Amber REPL execution-timer badge ("\timer"). See badge.h. */
#include "badge.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define BADGE_DIM "\x1b[2m"
#define BADGE_RST "\x1b[0m"

static int g_enabled = 1; /* ON by default, per spec */

void badge_set_enabled(int on) { g_enabled = !!on; }
int  badge_enabled(void)       { return g_enabled; }

void badge_begin(TimerBadge *tb) {
    clock_gettime(CLOCK_MONOTONIC, &tb->t0);
    tb->mem = 0;
}

void badge_accumulate(TimerBadge *tb, size_t bytes) {
    tb->mem += bytes;
}

/* Count evaluated lines: newline-separated chunks of `input`. Empty input,
 * or input with no trailing newline, still counts as (at least) one line. */
static size_t count_lines(const char *input, size_t len) {
    if (len == 0) return 1;
    size_t n = 1;
    for (size_t i = 0; i < len; i++) if (input[i] == '\n') n++;
    if (input[len - 1] == '\n') n--; /* trailing newline doesn't start a new line */
    return n ? n : 1;
}

/* Human-scale a byte count as "<value> <unit>" into buf, choosing B/KB/MB. */
static void fmt_bytes(double bytes, char *buf, size_t buflen) {
    if (bytes >= 1024.0 * 1024.0)
        snprintf(buf, buflen, "%.1f MB", bytes / (1024.0 * 1024.0));
    else if (bytes >= 1024.0)
        snprintf(buf, buflen, "%.1f KB", bytes / 1024.0);
    else
        snprintf(buf, buflen, "%.0f B", bytes);
}

void badge_end_print(const TimerBadge *tb, const char *input, size_t input_len) {
    struct timespec t1;
    clock_gettime(CLOCK_MONOTONIC, &t1);
    double elapsed_ns = (double)(t1.tv_sec  - tb->t0.tv_sec)  * 1e9
                       + (double)(t1.tv_nsec - tb->t0.tv_nsec);
    if (elapsed_ns < 0) elapsed_ns = 0; /* clock hiccup guard */

    if (!g_enabled) return;

    size_t lines = count_lines(input, input_len);
    double ns_per_line = elapsed_ns / (double)lines;
    double bytes_per_line = (double)tb->mem / (double)lines;

    char mem_total[24], mem_line[24];
    fmt_bytes((double)tb->mem, mem_total, sizeof mem_total);
    fmt_bytes(bytes_per_line, mem_line, sizeof mem_line);

    char line[160];
    int n = snprintf(line, sizeof line,
        BADGE_DIM "[%s %.2f ms | %s allocated | %.0f ns/line | %s/line]" BADGE_RST "\n",
        "\xE2\x96\x91" /* U+2591 LIGHT SHADE, "dim badge" glyph */,
        elapsed_ns / 1e6, mem_total, ns_per_line, mem_line);
    if (n > 0) fwrite(line, 1, (size_t)n, stderr);
}

void badge_cmd(const char *arg) {
    while (*arg == ' ' || *arg == '\t') arg++;
    if (!*arg) {
        badge_set_enabled(!badge_enabled());
    } else if (!strcasecmp(arg, "on") || !strcmp(arg, "1")) {
        badge_set_enabled(1);
    } else if (!strcasecmp(arg, "off") || !strcmp(arg, "0")) {
        badge_set_enabled(0);
    } else {
        fputs("usage: \\timer [on|off]\n", stdout);
        return;
    }
    printf("timer: %s\n", badge_enabled() ? "on" : "off");
}
