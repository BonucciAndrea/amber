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

/* Scoped scratch: arena_mark() snapshots the bump cursor AND the overflow-list
 * head, arena_release() rewinds to that snapshot -- freeing exactly the
 * overflow blocks taken since the mark and nothing older.
 *
 * arena_reset() alone is a whole-tick rewind, which is the right granularity
 * for "one expression, one scratch lifetime" but the wrong one for a kernel
 * that runs MANY times inside a single expression: `{asc x}'1000#,v` would hold
 * 1000 generations of radix ping-pong buffers live at once, because nothing
 * rewinds until the statement ends. A kernel that marks on entry and releases
 * on exit keeps its peak footprint at one generation regardless of how often it
 * is called, and still costs nothing on the slab fast path (two stores).
 *
 * Marks nest, but must be released in LIFO order -- releasing an outer mark
 * while an inner one is live invalidates the inner scratch. */
typedef struct { size_t off; void *over; } ArenaMark;
ArenaMark arena_mark(void);
void      arena_release(ArenaMark m);

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
