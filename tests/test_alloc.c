/* test_alloc.c  -  src/m.c's colouring of large blocks.
 * GNU AGPLv3 - see LICENSE and NOTICE.
 *
 * Linked against the whole interpreter like tests/test_ast.c (0.c built with
 * -Dldstatic; see run_tests.sh), so it calls the allocator itself: an() to
 * allocate, mr() to free (it returns the block m0() pushed on its free list),
 * aa() to resize. A payload's colour, in 64-byte lines, is the spare header
 * byte at -31; its block (HD<<b bytes) starts HD plus the colour before it. A
 * coloured payload's header gives its class as one less; the block's start
 * keeps the true one.
 *
 *   1. large payloads are 64-byte aligned, and their colours vary mod 4 KB
 *      (the smaller period, so the check holds for either value of CP); a
 *      coloured payload leaves at least 32 bytes after it in its block;
 *   2. a freed coloured block goes back on its free list uncoloured (mr returns
 *      the block's own payload address), and the next allocation of that class
 *      takes the same block with the next colour;
 *   3. small blocks are never coloured;
 *   4. a vector grown an item at a time from class 10 to class 13 keeps its
 *      contents, never writes past its block, and is copied at most 4 times:
 *      once a class, and once more if its first block was coloured (a coloured
 *      payload's header gives half its block; the copies are uncoloured). The
 *      same vector shrunk and grown again by large steps keeps them too.
 *
 * Checks 1 and 2 fail on a build that does not colour; 3 and 4 pass there too
 * (3 copies there). Check 2 needs a build without -DDBG, where mr() returns the
 * freed block (with DBG it returns 0). */
#include "a.h"
#include <stdio.h>

static int fails = 0;
#define LP(x) ((L*)(x))
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("FAIL " __VA_ARGS__); printf("\n"); } } while (0)

static W col(A x) { return (W)_cl(x) << 6; }                 /* bytes the payload was moved */
static W blk(A x) { return x - HD - col(x); }                /* the block's start */
static U bcls(A x) { return _b(x) + !!col(x); }               /* the block's class */

int main(void) {
    kinit();
    /* 1. alignment and spread */
    {
        enum { K = 16 };
        A v[K]; int seen[64] = {0}, distinct = 0;
        for (int i = 0; i < K; i++) {
            v[i] = an(100000 + 1000 * i, tL);                 /* 800 KB: class 14 */
            CHECK(!(v[i] & 63), "payload %d not 64-byte aligned: %#llx", i, (unsigned long long)v[i]);
            CHECK(col(v[i]) + HD + ((W)_n(v[i]) << 3) <= (W)HD << bcls(v[i]), "payload %d runs past its block", i);
            CHECK(bcls(v[i]) == 14 && *(UC *)(blk(v[i]) + HD - 32) == 14,
                  "payload %d: class %u, its block's start %u", i, bcls(v[i]), *(UC *)(blk(v[i]) + HD - 32));
            int s = (int)((v[i] >> 6) & 63);
            distinct += !seen[s]; seen[s] = 1;
        }
        CHECK(distinct >= K / 2, "only %d distinct offsets mod 4 KB among %d large payloads", distinct, K);
        for (int i = 0; i < K; i++) mr(v[i]);
    }
    /* 1b. a coloured payload leaves 32 bytes after it in its block (the gathers write up to 31 past n) */
    {
        int short_ = 0, coloured = 0;
        for (int k = 0; k < 64; k++) {
            U n = (U)((262144 - 64) / 8) - (U)k;              /* longs that (nearly) fill a class-12 block */
            A x = an(n, tL);
            if (col(x)) { coloured++; short_ += blk(x) + (HD << bcls(x)) - (x + ((W)n << 3)) < 32; }
            mr(x);
        }
        CHECK(!short_, "%d of %d coloured payloads left under 32 bytes after them", short_, coloured);
    }
    /* 2. free and reuse */
    {
        A x = an(20000, tL);                                  /* 160 KB: class 12 */
        W b = blk(x), c = col(x);
        A f = mr(x);
        CHECK(f == b + HD, "the freed block is not uncoloured: m0 gave %#llx, block %#llx",
              (unsigned long long)f, (unsigned long long)b);
        A y = an(20000, tL);
        CHECK(blk(y) == b, "the next class-12 allocation did not reuse the freed block");
        CHECK(col(y) != c, "the reused block got the same colour (%llu)", (unsigned long long)c);
        mr(y);
    }
    /* 3. small blocks */
    {
        for (int n = 1; n < 4000; n += 97) {
            A x = an(n, tL);
            CHECK(!col(x), "a small block (%d longs) was coloured", n);
            mr(x);
        }
    }
    /* 4. growth across classes 10-13 */
    {
        U n0 = 4100, n1 = 60000;                              /* 32.8 KB (class 10) to 480 KB (class 13) of longs */
        A x = an(n0, tL);
        for (U i = 0; i < n0; i++) LP(x)[i] = (L)i * 7;
        int moves = 0, bad = 0, past = 0;
        for (U n = n0 + 1; n <= n1; n++) {
            A y = aa(n, x);
            moves += y != x; x = y;
            LP(x)[n - 1] = (L)(n - 1) * 7;
            past += col(x) + HD + ((W)n << 3) > (W)HD << bcls(x);
        }
        for (U i = 0; i < n1; i++) bad += LP(x)[i] != (L)i * 7;
        CHECK(!bad, "%d items changed while growing an item at a time", bad);
        CHECK(!past, "%d appends ran past the block", past);
        CHECK(moves <= 4, "grown from class 10 to 13 an item at a time, the vector was copied %d times", moves);
        U sz[] = {9000, 70000, 5000, 33000, 60000};
        for (int k = 0; k < 5; k++) {
            U m = sz[k];
            if (m <= _n(x)) _n(x) = m;
            else { U o = _n(x); x = aa(m, x); for (U i = o; i < m; i++) LP(x)[i] = (L)i * 7; }
        }
        bad = 0;
        for (U i = 0; i < _n(x); i++) bad += LP(x)[i] != (L)i * 7;
        CHECK(!bad && _n(x) == 60000, "%d items changed in the resize round trip", bad);
        mr(x);
    }
    if (fails) { printf("%d ALLOC TEST(S) FAILED\n", fails); return 1; }
    printf("alloc: all passed\n");
    return 0;
}
