/* arena.c  -  Amber HFT zero-allocation scratchpad allocator.
 * GNU AGPLv3 - see LICENSE and NOTICE.  See arena.h for the contract.
 *
 * Alignment: the slab base and every overflow block are allocated with
 * posix_memalign() at ARENA_ALIGN (64 bytes) -- a full cache line, so one
 * register's width, and a multiple of NEON's 16-byte registers too -- so
 * that bump-allocated scratch handed to src/simd.c's kernels (or to any
 * other 256-bit-SIMD-friendly consumer) starts life aligned, not just
 * "rounded up from whatever malloc happened to return". Plain malloc()
 * only promises `alignof(max_align_t)` (commonly 16 bytes on x86_64/arm64),
 * which is not sufficient for a 32-byte guarantee. posix_memalign() needs
 * `_POSIX_C_SOURCE >= 200112L`, which must be defined before the first
 * system header (<stdlib.h>) is pulled in, same reasoning as the
 * clock_gettime() feature-test fix in trace.c. */
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200112L
#endif
#include "arena.h"
#include <stdlib.h>

#define ARENA_DEFAULT ((size_t)16u * 1024u * 1024u) /* 16 MB per thread */
#define ARENA_ALIGN   ((size_t)64u)                 /* full cache line: AVX2/AVX-512 friendly */

/* Overflow blocks: when a single tick asks for more than the slab holds we
 * fall back to an aligned heap allocation but remember the block so
 * reset()/free() can reclaim it. The bookkeeping header is itself rounded
 * up to ARENA_ALIGN so the returned payload is aligned like the slab path. */
typedef struct OverflowBlock {
    struct OverflowBlock *next;
    size_t                bytes;   /* payload size, so arena_used() can count it */
    /* payload follows, starting ARENA_ALIGN bytes in */
} OverflowBlock;

static __thread unsigned char *a_base = 0;  /* slab start                */
static __thread size_t         a_cap  = 0;  /* slab capacity in bytes    */
static __thread size_t         a_off  = 0;  /* bump cursor (bytes used)  */
static __thread OverflowBlock  *a_over = 0; /* head of overflow list     */
static __thread size_t         a_ovf  = 0;  /* bytes live in overflow blocks */
static __thread size_t         a_peak = 0;  /* high-water mark since last reset_peak */

/* Re-derive the high-water mark after every hand-out. `arena_used()` alone
 * cannot serve as a peak gauge because arena_reset() rewinds it to 0 -- any
 * consumer that samples it after the fact (e.g. \trace) would always read 0.
 * See arena_peak()/arena_reset_peak() in arena.h. */
static void note_peak(void) {
    size_t live = a_off + a_ovf;
    if (live > a_peak) a_peak = live;
}

static size_t align_up(size_t n, size_t a) { return (n + (a - 1)) & ~(a - 1); }

/* posix_memalign() requires size to be a multiple of alignment on some
 * implementations' stricter interpretations and always requires alignment
 * to be a power of two multiple of sizeof(void*); ARENA_ALIGN (32) already
 * satisfies both, but round the request up anyway so every caller gets a
 * block whose length is itself a clean multiple of the SIMD width. */
static void *aligned_xalloc(size_t bytes) {
    void *p = 0;
    size_t rounded = align_up(bytes ? bytes : 1, ARENA_ALIGN);
    if (posix_memalign(&p, ARENA_ALIGN, rounded) != 0) return 0;
    return p;
}

void arena_init(size_t capacity) {
    if (capacity == 0) capacity = ARENA_DEFAULT;
    if (a_base) {
        if (capacity <= a_cap) { arena_reset(); return; } /* fits: just rewind */
        arena_free();                                     /* grow: re-reserve  */
    }
    /* Never drop the overflow list on the floor here: if a previous slab
     * reservation failed (a_base==0) every arena_alloc() re-enters
     * arena_init(), and blindly zeroing a_over would leak every tracked
     * overflow block allocated in between. arena_reset() releases them. */
    arena_reset();
    a_base = (unsigned char *)aligned_xalloc(capacity);
    a_cap  = a_base ? capacity : 0;
    a_off  = 0;
}

void *arena_alloc(size_t bytes) {
    if (bytes == 0) bytes = 1;
    if (!a_base) arena_init(ARENA_DEFAULT);

    size_t off = align_up(a_off, ARENA_ALIGN);
    /* `off + bytes <= a_cap` would wrap for a huge `bytes` (reachable on a
     * 32-bit target such as wasm32, where size_t is 32 bits and an element
     * count * element width can overflow), silently passing the bounds check
     * and handing back a short block. Compare against the remaining space
     * instead -- that form cannot overflow. */
    if (a_base && off <= a_cap && bytes <= a_cap - off) { /* fast path: pointer bump */
        a_off = off + bytes;
        note_peak();
        return a_base + off;
    }

    /* Slow path: slab exhausted (or malloc failed).  Track a heap block so the
     * next arena_reset() frees it -- the caller never leaks and never gets a
     * short allocation. */
    {
        size_t hdr = align_up(sizeof(OverflowBlock), ARENA_ALIGN);
        if (bytes > (size_t)-1 - hdr) return 0;   /* hdr+bytes would wrap */
        OverflowBlock *b = (OverflowBlock *)aligned_xalloc(hdr + bytes);
        if (!b) return 0;
        b->next  = a_over;
        b->bytes = bytes;
        a_over   = b;
        a_ovf   += bytes;
        note_peak();
        return (unsigned char *)b + hdr;
    }
}

/* Scoped rewind -- see arena.h. Overflow blocks are a singly-linked LIFO list
 * (arena_alloc pushes at the head), so "everything allocated since the mark" is
 * exactly the prefix of that list down to the head recorded by the mark. */
ArenaMark arena_mark(void) {
    ArenaMark m;
    m.off  = a_off;
    m.over = (void *)a_over;
    return m;
}

void arena_release(ArenaMark m) {
    while (a_over && (void *)a_over != m.over) {
        OverflowBlock *n = a_over->next;
        a_ovf -= a_over->bytes;
        free(a_over);
        a_over = n;
    }
    /* Defensive: a release that does not find its mark in the list (a caller
     * that released out of LIFO order, or a reset in between) must not leave
     * the cursor pointing past freed scratch. */
    if (m.off <= a_off) a_off = m.off;
}

void arena_reset(void) {
    while (a_over) {
        OverflowBlock *n = a_over->next;
        free(a_over);
        a_over = n;
    }
    a_ovf = 0;
    a_off = 0;
}

void arena_free(void) {
    arena_reset();
    free(a_base);
    a_base = 0;
    a_cap  = 0;
    a_off  = 0;
}

/* Bytes handed out and not yet rewound -- slab bump cursor PLUS any live
 * overflow blocks. Counting only a_off (as this did before) under-reported
 * every allocation that spilled past the slab. */
size_t arena_used(void)     { return a_off + a_ovf; }
size_t arena_capacity(void) { return a_cap; }
size_t arena_peak(void)     { return a_peak; }
void   arena_reset_peak(void) { a_peak = a_off + a_ovf; }
