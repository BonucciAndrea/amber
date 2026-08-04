/* test_simd.c  -  standalone correctness test for src/simd.{h,c}.
 * GNU AGPLv3 - see LICENSE and NOTICE.
 *
 * Depends on nothing but simd.h/simd.c -- does not link against the Amber
 * interpreter, so it can run on any machine that has a C99 compiler,
 * including CI runners that never build the full interpreter.
 *
 * Build (from the project root):
 *   cc -O2 -Wall -Wextra -std=c99 -Isrc src/simd.c tests/test_simd.c -o /tmp/test_simd && /tmp/test_simd
 *   cc -O2 -mavx2 -Wall -Wextra -std=c99 -Isrc src/simd.c tests/test_simd.c -o /tmp/test_simd_avx2 && /tmp/test_simd_avx2
 *
 * Exits 0 and prints "ALL SIMD TESTS PASSED" on success, exits 1 on the
 * first mismatch (also driven from `./amber test.k` via the `` `simd``
 * builtin -- see the "simd" assertions there for the in-interpreter check).
 */
#include "simd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* GCC's -Wmaybe-uninitialized cannot prove a/b are always fully written
 * before use across the n==0 fast-empty-path in the helpers below (they
 * always are -- malloc+fill precedes every call); silence it narrowly
 * rather than restructuring working test code around a false positive. */
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif

static int fails = 0;

static void chk_i64(const char *what, int64_t got, int64_t want) {
    if (got != want) {
        fprintf(stderr, "FAIL %s: got %lld want %lld\n", what, (long long)got, (long long)want);
        fails++;
    }
}
static void chk_f64(const char *what, double got, double want) {
    /* exact compare for add/mul (no reordering within a single scalar-vs-
     * kernel call at matching indices); sum reduction uses an epsilon since
     * SIMD lane reduction associates differently than a linear scalar sum. */
    if (got != want) {
        fprintf(stderr, "FAIL %s: got %.17g want %.17g\n", what, got, want);
        fails++;
    }
}
static void chk_f64_eps(const char *what, double got, double want, double eps) {
    double d = got - want; if (d < 0) d = -d;
    if (d > eps) {
        fprintf(stderr, "FAIL %s: got %.17g want %.17g (diff %.3g > eps %.3g)\n", what, got, want, d, eps);
        fails++;
    }
}

/* sizes chosen to exercise: empty, single element, a partial AVX2 lane (< 4),
 * a partial NEON lane (< 2), an exact multiple of 4, an exact multiple of 4
 * plus a remainder, and a "large" size representative of real vector work. */
static const size_t SIZES[] = {0, 1, 2, 3, 4, 5, 7, 8, 15, 16, 1000, 100003};

static void test_add_mul_i64(size_t n) {
    size_t cap = (n ? n : 1) * sizeof(int64_t) + 8;
    int64_t *a = malloc(cap), *b = malloc(cap);
    int64_t *out = malloc(cap), *ref = malloc(cap);
    if (!a || !b || !out || !ref) { fprintf(stderr, "OOM\n"); exit(1); }
    for (size_t i = 0; i < n; i++) {
        /* mix of small, large, negative and zero values */
        a[i] = (int64_t)(i * 7 - 123) * ((i % 3) ? 1 : -1);
        b[i] = (int64_t)((n - i) * 3 + 5) * ((i % 5 == 0) ? -1 : 1);
    }
    simd_add_i64(a, b, out, n);
    for (size_t i = 0; i < n; i++) ref[i] = a[i] + b[i];
    for (size_t i = 0; i < n; i++) { char w[64]; snprintf(w, sizeof w, "add_i64[n=%zu][%zu]", n, i); chk_i64(w, out[i], ref[i]); }

    simd_mul_i64(a, b, out, n);
    for (size_t i = 0; i < n; i++) ref[i] = a[i] * b[i];
    for (size_t i = 0; i < n; i++) { char w[64]; snprintf(w, sizeof w, "mul_i64[n=%zu][%zu]", n, i); chk_i64(w, out[i], ref[i]); }

    int64_t sref = 0; for (size_t i = 0; i < n; i++) sref += a[i];
    char w[64]; snprintf(w, sizeof w, "sum_i64[n=%zu]", n);
    chk_i64(w, simd_sum_i64(a, n), sref);

    /* in-place aliasing: out==a must still be correct */
    memcpy(out, a, n * sizeof *a);
    simd_add_i64(out, b, out, n);
    for (size_t i = 0; i < n; i++) ref[i] = a[i] + b[i];
    for (size_t i = 0; i < n; i++) { char ww[80]; snprintf(ww, sizeof ww, "add_i64_inplace[n=%zu][%zu]", n, i); chk_i64(ww, out[i], ref[i]); }

    free(a); free(b); free(out); free(ref);
}

static void test_add_mul_f64(size_t n) {
    size_t cap = (n ? n : 1) * sizeof(double) + 8;
    double *a = malloc(cap), *b = malloc(cap);
    double *out = malloc(cap), *ref = malloc(cap);
    if (!a || !b || !out || !ref) { fprintf(stderr, "OOM\n"); exit(1); }
    for (size_t i = 0; i < n; i++) {
        a[i] = (double)i * 1.5 - 10.25;
        b[i] = (double)(n - i) * 0.25 + (i % 2 ? -3.0 : 3.0);
    }
    simd_add_f64(a, b, out, n);
    for (size_t i = 0; i < n; i++) ref[i] = a[i] + b[i];
    for (size_t i = 0; i < n; i++) { char w[64]; snprintf(w, sizeof w, "add_f64[n=%zu][%zu]", n, i); chk_f64(w, out[i], ref[i]); }

    simd_mul_f64(a, b, out, n);
    for (size_t i = 0; i < n; i++) ref[i] = a[i] * b[i];
    for (size_t i = 0; i < n; i++) { char w[64]; snprintf(w, sizeof w, "mul_f64[n=%zu][%zu]", n, i); chk_f64(w, out[i], ref[i]); }

    double sref = 0; for (size_t i = 0; i < n; i++) sref += a[i];
    char w[64]; snprintf(w, sizeof w, "sum_f64[n=%zu]", n);
    /* SIMD reduction associates differently than the naive left-to-right
     * scalar sum above; allow a relative epsilon proportional to n. */
    chk_f64_eps(w, simd_sum_f64(a, n), sref, 1e-6 * (double)(n + 1));

    free(a); free(b); free(out); free(ref);
}

int main(void) {
    printf("simd backend under test: %s\n", simd_backend());
    for (size_t k = 0; k < sizeof(SIZES) / sizeof(*SIZES); k++) {
        test_add_mul_i64(SIZES[k]);
        test_add_mul_f64(SIZES[k]);
    }
    if (fails) {
        fprintf(stderr, "%d SIMD TEST(S) FAILED\n", fails);
        return 1;
    }
    printf("ALL SIMD TESTS PASSED (%zu size cases x i64/f64 x add/mul/sum)\n",
           sizeof(SIZES) / sizeof(*SIZES));
    return 0;
}
