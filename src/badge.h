/* badge.h  -  Amber REPL execution-timer badge ("\timer").
 * GNU AGPLv3 - see LICENSE and NOTICE.
 *
 * Self-contained module (no Amber internals, no a.h) that measures how long
 * one REPL evaluation cycle took and how much arena memory it consumed, then
 * prints a dim, single-line badge after the result:
 *
 *   [o 0.42 ms | 1.2 MB allocated | 420 ns/line | 120 B/line]
 *
 * Wall-clock time comes from CLOCK_MONOTONIC (POSIX clock_gettime); memory
 * comes from the caller sampling arena_used() (see arena.h) before/after
 * evaluation and handing the delta to badge_accumulate(). Per-line figures
 * divide by the number of lines in the *input* string that was evaluated.
 *
 * ON by default; toggle at runtime with `\timer`, `\timer on`, `\timer off`.
 */
#ifndef AMBER_BADGE_H
#define AMBER_BADGE_H

#include <stddef.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    struct timespec t0;      /* CLOCK_MONOTONIC timestamp at badge_begin()    */
    size_t          mem;     /* bytes of arena consumption accumulated so far */
} TimerBadge;

/* Runtime on/off switch. Default is ON. */
void badge_set_enabled(int on);
int  badge_enabled(void);

/* Start timing one evaluation cycle. */
void badge_begin(TimerBadge *tb);

/* Add `bytes` (an arena_used() delta for one sub-step of the cycle) to the
 * running memory total. Safe to call zero or more times between
 * badge_begin() and badge_end_print(). */
void badge_accumulate(TimerBadge *tb, size_t bytes);

/* Stop timing, compute per-line metrics against `input` (the raw text that
 * was evaluated, `input_len` bytes, not necessarily NUL-terminated) and --
 * if enabled -- print the badge in dim ANSI to stderr. Safe to call
 * unconditionally regardless of the enabled flag. */
void badge_end_print(const TimerBadge *tb, const char *input, size_t input_len);

/* Handler for the `\timer` REPL command. `arg` is the (possibly empty,
 * trimmed, NUL-terminated) text after `\timer`: "" toggles, "on" enables,
 * "off" disables. Prints a one-line confirmation to stdout. */
void badge_cmd(const char *arg);

#ifdef __cplusplus
}
#endif

#endif /* AMBER_BADGE_H */
