/* parallel.c  -  see parallel.h.
 * GNU AGPLv3 - see LICENSE and NOTICE. */
#include "parallel.h"
#include "simd.h"
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

static int online_cpus(void) {
#if defined(_SC_NPROCESSORS_ONLN)
    long n = sysconf(_SC_NPROCESSORS_ONLN);
    if (n > 0) return (int)n;
#endif
    return 4;
}

int par_thread_count(size_t n) {
    if (n < PAR_THRESHOLD) return 1;
    int t = -1;
    const char *e = getenv("AMBER_THREADS");
    if (e && *e) {
        t = 0;
        for (; *e >= '0' && *e <= '9'; e++) t = t * 10 + (*e - '0');
        if (t < 1) t = online_cpus();
    } else {
        t = online_cpus();
    }
    if (t < 1) t = 1;
    if (t > PAR_MAX_THREADS) t = PAR_MAX_THREADS;
    /* never use more threads than there are elements to hand out */
    if ((size_t)t > n) t = (int)(n ? n : 1);
    return t;
}

/* ---- generic chunk splitter --------------------------------------------
 * Every op below follows the same shape: split [0,n) into `t` contiguous
 * chunks, hand each chunk to a worker thread that calls the matching
 * simd_* kernel on its slice, join, done. `mul`/`add` write straight into
 * the caller's `out`; `sum` collects one partial per thread and combines
 * them serially at the end (t is always small, so that combine is O(t),
 * not a bottleneck). */

typedef struct { const int64_t *a, *b; int64_t *out; size_t lo, hi; int op; } JobI64;
typedef struct { const double  *a, *b; double  *out; size_t lo, hi; int op; } JobF64;
enum { OP_ADD, OP_MUL, OP_SUM };

static void *work_i64(void *arg) {
    JobI64 *j = (JobI64 *)arg;
    size_t n = j->hi - j->lo;
    switch (j->op) {
        case OP_ADD: simd_add_i64(j->a + j->lo, j->b + j->lo, j->out + j->lo, n); break;
        case OP_MUL: simd_mul_i64(j->a + j->lo, j->b + j->lo, j->out + j->lo, n); break;
        case OP_SUM: j->out[j->lo] = simd_sum_i64(j->a + j->lo, n); break; /* out[lo] used as this thread's partial */
    }
    return 0;
}
static void *work_f64(void *arg) {
    JobF64 *j = (JobF64 *)arg;
    size_t n = j->hi - j->lo;
    switch (j->op) {
        case OP_ADD: simd_add_f64(j->a + j->lo, j->b + j->lo, j->out + j->lo, n); break;
        case OP_MUL: simd_mul_f64(j->a + j->lo, j->b + j->lo, j->out + j->lo, n); break;
        case OP_SUM: j->out[j->lo] = simd_sum_f64(j->a + j->lo, n); break;
    }
    return 0;
}

static void run_i64(const int64_t *a, const int64_t *b, int64_t *out, size_t n, int op) {
    int t = par_thread_count(n);
    if (t <= 1) { work_i64(&(JobI64){a, b, out, 0, n, op}); return; }
    pthread_t th[PAR_MAX_THREADS];
    JobI64 jobs[PAR_MAX_THREADS];
    size_t chunk = n / (size_t)t, lo = 0;
    for (int i = 0; i < t; i++) {
        size_t hi = (i == t - 1) ? n : lo + chunk;
        jobs[i] = (JobI64){a, b, out, lo, hi, op};
        pthread_create(&th[i], 0, work_i64, &jobs[i]);
        lo = hi;
    }
    for (int i = 0; i < t; i++) pthread_join(th[i], 0);
}
static void run_f64(const double *a, const double *b, double *out, size_t n, int op) {
    int t = par_thread_count(n);
    if (t <= 1) { work_f64(&(JobF64){a, b, out, 0, n, op}); return; }
    pthread_t th[PAR_MAX_THREADS];
    JobF64 jobs[PAR_MAX_THREADS];
    size_t chunk = n / (size_t)t, lo = 0;
    for (int i = 0; i < t; i++) {
        size_t hi = (i == t - 1) ? n : lo + chunk;
        jobs[i] = (JobF64){a, b, out, lo, hi, op};
        pthread_create(&th[i], 0, work_f64, &jobs[i]);
        lo = hi;
    }
    for (int i = 0; i < t; i++) pthread_join(th[i], 0);
}

void par_add_i64(const int64_t *a, const int64_t *b, int64_t *out, size_t n) { run_i64(a, b, out, n, OP_ADD); }
void par_mul_i64(const int64_t *a, const int64_t *b, int64_t *out, size_t n) { run_i64(a, b, out, n, OP_MUL); }
void par_add_f64(const double  *a, const double  *b, double  *out, size_t n) { run_f64(a, b, out, n, OP_ADD); }
void par_mul_f64(const double  *a, const double  *b, double  *out, size_t n) { run_f64(a, b, out, n, OP_MUL); }

int64_t par_sum_i64(const int64_t *a, size_t n) {
    int t = par_thread_count(n);
    if (t <= 1) return simd_sum_i64(a, n);
    int64_t *partial = (int64_t *)malloc(n * sizeof *partial); /* only indices lo[i] are ever written */
    if (!partial) return simd_sum_i64(a, n);
    run_i64(a, 0, partial, n, OP_SUM); /* b unused for OP_SUM */
    /* combine: partial[lo] holds thread i's sum, at the lo boundary it wrote */
    int64_t total = 0;
    size_t chunk = n / (size_t)t, lo = 0;
    for (int i = 0; i < t; i++) {
        total += partial[lo];
        lo = (i == t - 1) ? n : lo + chunk;
    }
    free(partial);
    return total;
}
double par_sum_f64(const double *a, size_t n) {
    int t = par_thread_count(n);
    if (t <= 1) return simd_sum_f64(a, n);
    double *partial = (double *)malloc(n * sizeof *partial);
    if (!partial) return simd_sum_f64(a, n);
    run_f64(a, 0, partial, n, OP_SUM);
    double total = 0;
    size_t chunk = n / (size_t)t, lo = 0;
    for (int i = 0; i < t; i++) {
        total += partial[lo];
        lo = (i == t - 1) ? n : lo + chunk;
    }
    free(partial);
    return total;
}
