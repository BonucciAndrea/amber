/* bench/queries/c_bench.c -- native C baseline for bench/run_comparative.py.
 * GNU AGPLv3 - see LICENSE and NOTICE.  Implements bench/SPEC.md verbatim.
 *
 * Build:  gcc -O3 -march=native -o c_bench bench/queries/c_bench.c -lm
 * Run:    ./c_bench <arith|reduce|groupby|join> [runs] [warmup]
 *
 * This is the "how fast could you possibly do it" floor: plain scalar C over
 * flat malloc'd arrays, no library calls in the kernels, -O3 -march=native so
 * the compiler is free to auto-vectorise exactly as it would for any hot loop.
 * Data generation and the join's hash build are OUTSIDE the timed region for
 * every workload, matching what the array engines get (their data is likewise
 * materialised before the clock starts).
 */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define N 10000000L
#define M  1000000L
#define K     1000L
#define G      100L

static double now_ms(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1e3 + t.tv_nsec / 1e6;
}

static int cmp_d(const void *a, const void *b) {
    double x = *(const double *)a, y = *(const double *)b;
    return x < y ? -1 : x > y;
}

/* ---- shared data (SPEC.md §1) ------------------------------------------- */
static long   *h, *a, *b, *kl, *kr, *gk;
static double *x, *y, *vl, *vr;

/* Open-addressed hash for the join's right table. Built OUTSIDE the timer:
 * every other engine also gets its right table materialised before timing. */
#define HB 4096                       /* power of two, > 2*K */
static long hkey[HB];
static double hval[HB];

static void gen(void) {
    h  = malloc(N * sizeof *h);  a = malloc(N * sizeof *a);  b = malloc(N * sizeof *b);
    x  = malloc(N * sizeof *x);  y = malloc(N * sizeof *y);  gk = malloc(N * sizeof *gk);
    kl = malloc(M * sizeof *kl); vl = malloc(M * sizeof *vl);
    kr = malloc(K * sizeof *kr); vr = malloc(K * sizeof *vr);
    if (!h || !a || !b || !x || !y || !gk || !kl || !vl || !kr || !vr) exit(2);

    for (long i = 0; i < N; i++) {
        h[i]  = (262147L * i) % 1048573L;
        a[i]  = h[i] % 1000L;
        b[i]  = h[i] %  997L;
        x[i]  = (double)a[i];
        y[i]  = (double)b[i];
        gk[i] = a[i] % G;
    }
    for (long j = 0; j < K; j++) { kr[j] = (7919L * j) % 1048573L; vr[j] = 2.0 * (double)j; }
    for (long i = 0; i < M; i++) { kl[i] = kr[h[i] % K]; vl[i] = x[i]; }

    for (long s = 0; s < HB; s++) hkey[s] = -1;
    for (long j = 0; j < K; j++) {
        unsigned long s = (unsigned long)kr[j] & (HB - 1);
        while (hkey[s] != -1) s = (s + 1) & (HB - 1);
        hkey[s] = kr[j]; hval[s] = vr[j];
    }
}

static long check(void) {
    long sa = 0, sb = 0;
    for (long i = 0; i < N; i++) { sa += a[i]; sb += b[i]; }
    return sa + 3 * sb;
}

/* ---- kernels (each returns the scalar answer; nothing is elidable) ------ */
static double k_arith(void) {
    double s = 0;
    for (long i = 0; i < N; i++) if (x[i] > 50.0) s += x[i] * 2.5 + y[i];
    return s;
}

static double k_reduce(void) {
    double s = 0, mx = x[0], d = 0;
    for (long i = 0; i < N; i++) s += x[i];
    for (long i = 0; i < N; i++) if (x[i] > mx) mx = x[i];
    for (long i = 0; i < N; i++) d += x[i] * y[i];
    return s + mx + d;
}

static double k_groupby(void) {
    double gs[G];
    for (long g = 0; g < G; g++) gs[g] = 0;
    for (long i = 0; i < N; i++) gs[gk[i]] += x[i];
    double s = 0;
    for (long g = 0; g < G; g++) s += (double)(g + 1) * gs[g];
    return s;
}

static double k_join(void) {
    double s = 0;
    for (long i = 0; i < M; i++) {
        unsigned long p = (unsigned long)kl[i] & (HB - 1);
        while (hkey[p] != kl[i]) p = (p + 1) & (HB - 1);
        s += vl[i] * hval[p];
    }
    return s;
}

int main(int argc, char **argv) {
    const char *id = argc > 1 ? argv[1] : "reduce";
    int runs   = argc > 2 ? atoi(argv[2]) : 5;
    int warmup = argc > 3 ? atoi(argv[3]) : 2;
    if (runs < 1) runs = 1;

    double (*kern)(void) =
        !strcmp(id, "arith")   ? k_arith   :
        !strcmp(id, "reduce")  ? k_reduce  :
        !strcmp(id, "groupby") ? k_groupby :
        !strcmp(id, "join")    ? k_join    : NULL;
    if (!kern) { fprintf(stderr, "unknown bench: %s\n", id); return 2; }

    gen();
    long chk = check();

    double ans = 0;
    for (int w = 0; w < warmup; w++) ans = kern();

    double *t = malloc((size_t)runs * sizeof *t);
    for (int r = 0; r < runs; r++) {
        double t0 = now_ms();
        ans = kern();
        t[r] = now_ms() - t0;
    }
    qsort(t, (size_t)runs, sizeof *t, cmp_d);
    double med = runs & 1 ? t[runs / 2] : 0.5 * (t[runs / 2 - 1] + t[runs / 2]);

    printf("BENCH %s\n", id);
    printf("CHECK %ld\n", chk);
    printf("ANSWER %.17g\n", ans);
    printf("TIME_MS %.4f\n", med);
    return 0;
}
