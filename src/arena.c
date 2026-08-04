/* arena.c  -  Amber HFT zero-allocation scratchpad allocator.
 * GNU AGPLv3 - see LICENSE and NOTICE.  See arena.h for the contract.
 *
 * Alignment: the slab base and every overflow block are allocated with
 * posix_memalign() at ARENA_ALIGN (32 bytes) -- one AVX2 __m256i/__m256d
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
#define ARENA_ALIGN   ((size_t)32u)                 /* AVX2/NEON-friendly alignment */

/* Overflow blocks: when a single tick asks for more than the slab holds we
 * fall back to an aligned heap allocation but remember the block so
 * reset()/free() can reclaim it. The bookkeeping header is itself rounded
 * up to ARENA_ALIGN so the returned payload is aligned like the slab path. */
typedef struct OverflowBlock {
    struct OverflowBlock *next;
    /* payload follows, starting ARENA_ALIGN bytes in */
} OverflowBlock;

static __thread unsigned char *a_base = 0;  /* slab start                */
static __thread size_t         a_cap  = 0;  /* slab capacity in bytes    */
static __thread size_t         a_off  = 0;  /* bump cursor (bytes used)  */
static __thread OverflowBlock  *a_over = 0; /* head of overflow list     */

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
    a_base = (unsigned char *)aligned_xalloc(capacity);
    a_cap  = a_base ? capacity : 0;
    a_off  = 0;
    a_over = 0;
}

void *arena_alloc(size_t bytes) {
    if (bytes == 0) bytes = 1;
    if (!a_base) arena_init(ARENA_DEFAULT);

    size_t off = align_up(a_off, ARENA_ALIGN);
    if (a_base && off + bytes <= a_cap) {   /* fast path: pure pointer bump */
        a_off = off + bytes;
        return a_base + off;
    }

    /* Slow path: slab exhausted (or malloc failed).  Track a heap block so the
     * next arena_reset() frees it -- the caller never leaks and never gets a
     * short allocation. */
    {
        size_t hdr = align_up(sizeof(OverflowBlock), ARENA_ALIGN);
        OverflowBlock *b = (OverflowBlock *)aligned_xalloc(hdr + bytes);
        if (!b) return 0;
        b->next = a_over;
        a_over  = b;
        return (unsigned char *)b + hdr;
    }
}

void arena_reset(void) {
    while (a_over) {
        OverflowBlock *n = a_over->next;
        free(a_over);
        a_over = n;
    }
    a_off = 0;
}

void arena_free(void) {
    arena_reset();
    free(a_base);
    a_base = 0;
    a_cap  = 0;
    a_off  = 0;
}

size_t arena_used(void)     { return a_off; }
size_t arena_capacity(void) { return a_cap; }
