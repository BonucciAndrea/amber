/* test_parallel.c  -  standalone correctness test for src/parallel.{h,c}.
 * GNU AGPLv3 - see LICENSE and NOTICE.
 *
 * Build:
 *   cc -O2 -Wall -Wextra -std=c99 -pthread -Isrc src/simd.c src/parallel.c tests/test_parallel.c \
 *      -o /tmp/test_parallel && /tmp/test_parallel
 */
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200112L /* setenv/unsetenv need this under strict -std=c99 */
#endif
#include "parallel.h"
#include <stdio.h>
#include <stdlib.h>

static int fails = 0;

static void chk_i64(const char *what, int64_t got, int64_t want) {
    if (got != want) { fprintf(stderr, "FAIL %s: got %lld want %lld\n", what, (long long)got, (long long)want); fails++; }
}
static void chk_f64(const char *what, double got, double want, double eps) {
    double d = got - want; if (d < 0) d = -d;
    if (d > eps) { fprintf(stderr, "FAIL %s: got %.17g want %.17g\n", what, got, want); fails++; }
}

/* n values chosen to straddle PAR_THRESHOLD (currently 100000) on both
 * sides, plus a couple of small/edge sizes. */
static const size_t SIZES[] = {0, 1, 100, 99999, 100000, 100001, 250003, 1000037};

static void run_case(size_t n, int force_threads) {
    if (force_threads > 0) {
        char buf[32]; snprintf(buf, sizeof buf, "%d", force_threads);
        setenv("AMBER_THREADS", buf, 1);
    } else {
        unsetenv("AMBER_THREADS");
    }

    size_t cap = (n ? n : 1);
    int64_t *ai = malloc(cap * sizeof *ai), *bi = malloc(cap * sizeof *bi), *oi = malloc(cap * sizeof *oi);
    double  *af = malloc(cap * sizeof *af), *bf = malloc(cap * sizeof *bf), *of = malloc(cap * sizeof *of);
    if (!ai || !bi || !oi || !af || !bf || !of) { fprintf(stderr, "OOM\n"); exit(1); }
    for (size_t i = 0; i < n; i++) {
        ai[i] = (int64_t)i * 3 - 7; bi[i] = (int64_t)(n - i) + 1;
        af[i] = (double)i * 0.75 - 2.5; bf[i] = (double)(n - i) * 0.1;
    }

    int64_t sref_i = 0; for (size_t i = 0; i < n; i++) sref_i += ai[i];
    double  sref_f = 0; for (size_t i = 0; i < n; i++) sref_f += af[i];

    char tag[96];
    int actual_t = par_thread_count(n);
    snprintf(tag, sizeof tag, "n=%zu threads=%d(env=%s)", n, actual_t, force_threads > 0 ? "set" : "auto");

    par_add_i64(ai, bi, oi, n);
    for (size_t i = 0; i < n; i++) if (oi[i] != ai[i] + bi[i]) { fprintf(stderr, "FAIL add_i64 %s idx %zu\n", tag, i); fails++; break; }

    par_mul_i64(ai, bi, oi, n);
    for (size_t i = 0; i < n; i++) if (oi[i] != ai[i] * bi[i]) { fprintf(stderr, "FAIL mul_i64 %s idx %zu\n", tag, i); fails++; break; }

    chk_i64(tag, par_sum_i64(ai, n), sref_i);

    par_add_f64(af, bf, of, n);
    for (size_t i = 0; i < n; i++) if (of[i] != af[i] + bf[i]) { fprintf(stderr, "FAIL add_f64 %s idx %zu\n", tag, i); fails++; break; }

    par_mul_f64(af, bf, of, n);
    for (size_t i = 0; i < n; i++) if (of[i] != af[i] * bf[i]) { fprintf(stderr, "FAIL mul_f64 %s idx %zu\n", tag, i); fails++; break; }

    chk_f64(tag, par_sum_f64(af, n), sref_f, 1e-6 * (double)(n + 1));

    free(ai); free(bi); free(oi); free(af); free(bf); free(of);
}

int main(void) {
    for (size_t k = 0; k < sizeof(SIZES) / sizeof(*SIZES); k++) {
        run_case(SIZES[k], 0);   /* auto thread count */
        run_case(SIZES[k], 1);   /* forced serial */
        run_case(SIZES[k], 8);   /* forced 8 threads, even on a small machine */
    }
    if (fails) { fprintf(stderr, "%d PARALLEL TEST(S) FAILED\n", fails); return 1; }
    printf("ALL PARALLEL TESTS PASSED (%zu sizes x 3 thread configs)\n", sizeof(SIZES) / sizeof(*SIZES));
    return 0;
}
