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

typedef struct { void (*fn)(void *, int); void *ctx; int i; } RunJob;
static void *run_job(void *arg) { RunJob *j = (RunJob *)arg; j->fn(j->ctx, j->i); return 0; }

/* par_run's own threads: the way it always worked, and the fallback when the pool is taken */
static void par_run_spawn(int t, void (*fn)(void *ctx, int i), void *ctx) {
    pthread_t th[PAR_MAX_THREADS];
    RunJob jobs[PAR_MAX_THREADS];
    int started[PAR_MAX_THREADS];
    for (int i = 1; i < t; i++) {
        jobs[i] = (RunJob){fn, ctx, i};
        started[i] = pthread_create(&th[i], 0, run_job, &jobs[i]) == 0;
        if (!started[i]) fn(ctx, i);
    }
    fn(ctx, 0);
    for (int i = 1; i < t; i++) if (started[i]) pthread_join(th[i], 0);
}

#if !defined(wasm)
/* ---- amber 2.5: the persistent pool ---------------------------------------
 * Workers 1..pl_n, made once and kept. A call publishes a job as one 64-bit word (generation << 8 | t) after fn,
 * ctx and the share counter are set (release). The t shares are not tied to threads: the caller and every worker
 * awake take the next share from pl_next (generation << 16 | next index, by compare-and-swap, so a worker late
 * from an older job can never take one of this job's) until none is left. A worker that wakes late, or was
 * preempted -- with every CPU busy something always is -- finds nothing to do instead of making the call wait a
 * scheduler tick for its share. The kernels still get share i of t, as before. The caller returns once pl_done
 * says all t ran; a share still running keeps it from publishing the next job, so fn and ctx are read only while
 * they are this job's. Workers spin a while after a job (kernels come in runs), then sleep on a condvar; the
 * word and pl_sleep are both seq_cst, so a publish never misses a worker on its way to sleep. */
#define PL_SPIN 20000
static pthread_mutex_t pl_busy = PTHREAD_MUTEX_INITIALIZER;   /* one par_run on the pool at a time */
static pthread_mutex_t pl_m = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t pl_go = PTHREAD_COND_INITIALIZER, pl_fin = PTHREAD_COND_INITIALIZER;
static int pl_n, pl_done, pl_sleep, pl_wait;
static uint64_t pl_word, pl_next, pl_g0[PAR_MAX_THREADS];
static void (*pl_fn)(void *, int);
static void *pl_ctx;

static inline void pl_relax(void) {
#if defined(__x86_64__) || defined(__i386__)
    __builtin_ia32_pause();
#elif defined(__aarch64__)
    __asm__ __volatile__("yield");
#endif
}
/* take and run shares of job g (t shares) until there are none */
static void pl_take(uint64_t g, int t) {
    for (;;) {
        uint64_t c = __atomic_load_n(&pl_next, __ATOMIC_ACQUIRE);
        if (c >> 16 != g || (int)(c & 0xffff) >= t) return;
        if (!__atomic_compare_exchange_n(&pl_next, &c, c + 1, 0, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE)) continue;
        pl_fn(pl_ctx, (int)(c & 0xffff));
        if (__atomic_add_fetch(&pl_done, 1, __ATOMIC_SEQ_CST) == t && __atomic_load_n(&pl_wait, __ATOMIC_SEQ_CST)) {
            pthread_mutex_lock(&pl_m); pthread_cond_signal(&pl_fin); pthread_mutex_unlock(&pl_m);
        }
    }
}
static void *pl_worker(void *arg) {
    int k = (int)(intptr_t)arg;
    uint64_t seen = pl_g0[k];                                     /* not the word now: the job may be out already */
    for (;;) {
        uint64_t w = 0; int s;
        for (s = 0; s < PL_SPIN; s++) {
            w = __atomic_load_n(&pl_word, __ATOMIC_ACQUIRE);
            if (w >> 8 != seen) break;
            pl_relax();
        }
        if (s == PL_SPIN) {
            pthread_mutex_lock(&pl_m);
            __atomic_add_fetch(&pl_sleep, 1, __ATOMIC_SEQ_CST);
            while ((w = __atomic_load_n(&pl_word, __ATOMIC_SEQ_CST)) >> 8 == seen) pthread_cond_wait(&pl_go, &pl_m);
            __atomic_sub_fetch(&pl_sleep, 1, __ATOMIC_SEQ_CST);
            pthread_mutex_unlock(&pl_m);
        }
        seen = w >> 8;
        pl_take(seen, (int)(w & 255));
    }
    return 0;
}
static void pl_atfork_child(void) {                               /* the child has none of the workers */
    pl_n = 0; pl_done = 0; pl_sleep = 0; pl_wait = 0;
    pthread_mutex_init(&pl_busy, 0); pthread_mutex_init(&pl_m, 0);
    pthread_cond_init(&pl_go, 0); pthread_cond_init(&pl_fin, 0);
}
void par_run(int t, void (*fn)(void *ctx, int i), void *ctx) {
    if (t < 1) t = 1;
    if (t > PAR_MAX_THREADS) t = PAR_MAX_THREADS;
    if (t == 1) { fn(ctx, 0); return; }
    if (pthread_mutex_trylock(&pl_busy)) { par_run_spawn(t, fn, ctx); return; }
    { static int at; if (!at) { at = 1; pthread_atfork(0, 0, pl_atfork_child); } }
    uint64_t g = __atomic_load_n(&pl_word, __ATOMIC_RELAXED) >> 8;
    while (pl_n < t - 1) {                                        /* more workers, told the current generation */
        pthread_t th; pthread_attr_t a; pthread_attr_init(&a); pthread_attr_setdetachstate(&a, PTHREAD_CREATE_DETACHED);
        pl_g0[pl_n + 1] = g;
        int r = pthread_create(&th, &a, pl_worker, (void *)(intptr_t)(pl_n + 1));
        pthread_attr_destroy(&a);
        if (r) break;
        pl_n++;
    }
    if (pl_n < 1) { pthread_mutex_unlock(&pl_busy); par_run_spawn(t, fn, ctx); return; }
    g++;
    pl_fn = fn; pl_ctx = ctx;
    __atomic_store_n(&pl_done, 0, __ATOMIC_RELAXED);
    __atomic_store_n(&pl_next, g << 16, __ATOMIC_RELAXED);
    __atomic_store_n(&pl_word, g << 8 | (uint64_t)t, __ATOMIC_SEQ_CST);
    if (__atomic_load_n(&pl_sleep, __ATOMIC_SEQ_CST)) { pthread_mutex_lock(&pl_m); pthread_cond_broadcast(&pl_go); pthread_mutex_unlock(&pl_m); }
    pl_take(g, t);                                                /* the caller takes shares too */
    int s;
    for (s = 0; s < PL_SPIN && __atomic_load_n(&pl_done, __ATOMIC_ACQUIRE) < t; s++) pl_relax();
    if (__atomic_load_n(&pl_done, __ATOMIC_ACQUIRE) < t) {
        pthread_mutex_lock(&pl_m);
        __atomic_store_n(&pl_wait, 1, __ATOMIC_SEQ_CST);
        while (__atomic_load_n(&pl_done, __ATOMIC_SEQ_CST) < t) pthread_cond_wait(&pl_fin, &pl_m);
        __atomic_store_n(&pl_wait, 0, __ATOMIC_SEQ_CST);
        pthread_mutex_unlock(&pl_m);
    }
    pthread_mutex_unlock(&pl_busy);
}
#else
void par_run(int t, void (*fn)(void *ctx, int i), void *ctx) {
    if (t < 1) t = 1;
    if (t > PAR_MAX_THREADS) t = PAR_MAX_THREADS;
    par_run_spawn(t, fn, ctx);
}
#endif


/* ---- 2.5 (exp): blocked float sum/dot, parallel float min/max ------------------------------------------ */
typedef struct { const double *a, *b; size_t n, nb; double *part; int mx; int nan[PAR_MAX_THREADS];
                 double mm[PAR_MAX_THREADS]; int t; size_t next; } PBS;
static void pbs_job(void *c_, int i) {
    PBS *c = (PBS *)c_; (void)i;
    for (;;) {
        size_t k = __atomic_fetch_add(&c->next, 1, __ATOMIC_RELAXED);
        if (k >= c->nb) break;
        size_t lo = k * PBS_BLOCK, m = c->n - lo < PBS_BLOCK ? c->n - lo : PBS_BLOCK;
        c->part[k] = c->b ? simd_dot_f64(c->a + lo, c->b + lo, m) : simd_sum_f64(c->a + lo, m);
    }
}
static double pbs(const double *a, const double *b, size_t n) {
    if (n < PBS_MIN) return b ? simd_dot_f64(a, b, n) : simd_sum_f64(a, n);
    PBS c; c.a = a; c.b = b; c.n = n; c.nb = (n + PBS_BLOCK - 1) / PBS_BLOCK; c.next = 0;
    double stack[512];
    c.part = c.nb <= 512 ? stack : (double *)malloc(c.nb * sizeof(double));
    if (!c.part) return b ? simd_dot_f64(a, b, n) : simd_sum_f64(a, n);
    int t = par_thread_count(n);
    if ((size_t)t > c.nb) t = (int)c.nb;
    if (t < 2) pbs_job(&c, 0); else par_run(t, pbs_job, &c);
    double r = simd_sum_f64(c.part, c.nb);
    if (c.part != stack) free(c.part);
    return r;
}
double par_bsum_f64(const double *a, size_t n) { return pbs(a, 0, n); }
double par_bdot_f64(const double *a, const double *b, size_t n) { return pbs(a, b, n); }

static void pmm_job(void *c_, int i) {
    PBS *c = (PBS *)c_;
    size_t ch = c->n / (size_t)c->t, lo = (size_t)i * ch, hi = i == c->t - 1 ? c->n : lo + ch;
    int s = 0;
    c->mm[i] = c->mx ? simd_max_f64(c->a + lo, hi - lo, &s) : simd_min_f64(c->a + lo, hi - lo, &s);
    c->nan[i] = s;
}
double par_mm_f64(const double *a, size_t n, int mx, int *sawnan) {
    int t = n < PBS_MIN ? 1 : par_thread_count(n);
    if (t < 2) return mx ? simd_max_f64(a, n, sawnan) : simd_min_f64(a, n, sawnan);
    PBS c; c.a = a; c.b = 0; c.n = n; c.mx = mx; c.t = t;
    par_run(t, pmm_job, &c);
    double r = c.mm[0]; int s = c.nan[0];
    for (int i = 1; i < t; i++) { s |= c.nan[i]; if (mx ? c.mm[i] > r : c.mm[i] < r) r = c.mm[i]; }
    if (s || r == 0.0) return mx ? simd_max_f64(a, n, sawnan) : simd_min_f64(a, n, sawnan);   /* NaN rules, zero's sign */
    *sawnan = 0;
    return r;
}
