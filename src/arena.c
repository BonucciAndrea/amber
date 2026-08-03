/* arena.c  -  Amber HFT zero-allocation scratchpad allocator.
 * GNU AGPLv3 - see LICENSE and NOTICE.  See arena.h for the contract. */
#include "arena.h"
#include <stdlib.h>

#define ARENA_DEFAULT ((size_t)16u * 1024u * 1024u) /* 16 MB per thread */
#define ARENA_ALIGN   ((size_t)16u)                 /* SIMD-friendly alignment */

/* Overflow blocks: when a single tick asks for more than the slab holds we
 * fall back to malloc but remember the block so reset()/free() can reclaim it.
 * The bookkeeping header is itself 16-byte aligned so the returned payload is
 * aligned like the slab path. */
typedef struct OverflowBlock {
    struct OverflowBlock *next;
    /* payload follows, starting ARENA_ALIGN bytes in */
} OverflowBlock;

static __thread unsigned char *a_base = 0;  /* slab start                */
static __thread size_t         a_cap  = 0;  /* slab capacity in bytes    */
static __thread size_t         a_off  = 0;  /* bump cursor (bytes used)  */
static __thread OverflowBlock  *a_over = 0; /* head of overflow list     */

static size_t align_up(size_t n, size_t a) { return (n + (a - 1)) & ~(a - 1); }

void arena_init(size_t capacity) {
    if (capacity == 0) capacity = ARENA_DEFAULT;
    if (a_base) {
        if (capacity <= a_cap) { arena_reset(); return; } /* fits: just rewind */
        arena_free();                                     /* grow: re-reserve  */
    }
    a_base = (unsigned char *)malloc(capacity);
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
        OverflowBlock *b = (OverflowBlock *)malloc(hdr + bytes);
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
