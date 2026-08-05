/* arena.h  -  Amber HFT zero-allocation scratchpad allocator.
 * GNU AGPLv3 - see LICENSE and NOTICE.
 *
 * A thread-local bump ("region") allocator for the transient / intermediate
 * array buffers produced while evaluating a K expression.  During a tick the
 * evaluator carves scratch out of one pre-reserved slab with a single pointer
 * bump (no locking, no syscalls); at the end of the tick arena_reset() rewinds
 * the slab in O(1).  This removes the malloc()/free() calls -- and the latency
 * jitter they cause -- from the hot path.
 *
 * Portable C (POSIX / C99): thread-locality uses the __thread storage class,
 * which both gcc and Apple-Silicon clang accept.  No libc beyond malloc/free.
 */
#ifndef AMBER_ARENA_H
#define AMBER_ARENA_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Reserve the per-thread slab.  capacity==0 selects the 16 MB default.
 * Safe to call more than once: a request that fits the existing slab just
 * rewinds it; a larger request re-reserves. */
void   arena_init(size_t capacity);

/* Bump-allocate `bytes` of 16-byte-aligned scratch.  Never returns NULL for a
 * successful malloc: requests that overflow the slab fall back to a tracked
 * heap block that arena_reset()/arena_free() release, so callers never leak and
 * never see a short allocation mid-tick.  Lazily arena_init()s on first use. */
void  *arena_alloc(size_t bytes);

/* Rewind the slab to empty in O(1) and release any overflow blocks.
 * Call once per tick / evaluation cycle. */
void   arena_reset(void);

/* Release the slab and all overflow blocks back to the system. */
void   arena_free(void);

/* Introspection (bytes currently handed out -- slab bump cursor plus any live
 * overflow blocks -- and slab capacity). */
size_t arena_used(void);
size_t arena_capacity(void);

/* High-water mark of arena_used() since the last arena_reset_peak().
 * arena_reset() deliberately does NOT clear this: a consumer that wants to
 * know how much scratch an evaluation actually touched has to sample it
 * after the evaluation has already rewound the slab (this is exactly what
 * \trace does), so the peak must survive the rewind. Call
 * arena_reset_peak() immediately before the region you want to measure. */
size_t arena_peak(void);
void   arena_reset_peak(void);

#ifdef __cplusplus
}
#endif

#endif /* AMBER_ARENA_H */
