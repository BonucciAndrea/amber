/* peachpool.c  -  see peachpool.h.
 * GNU AGPLv3 - see LICENSE and NOTICE. */
#if !defined(wasm)
#include "a.h"
#include "peachpool.h"
#include <pthread.h>
#include <stdlib.h>

/* ---- one live job at a time --------------------------------------------
 * peach dispatches are serialised: only the parent thread publishes a job,
 * and a `peach` reached from inside a worker (nested peach) falls back to a
 * serial each in src/i.c (it sees ray_rc_sync already set), so this single
 * shared Job is never contended for publication. */
typedef struct {
    A f, dat;            /* borrowed from the caller (peachC owns/frees them) */
    A *out;              /* result slots [0,n), pre-zeroed by the dispatcher  */
    U n, grain;
    U cursor;            /* next morsel start; grabbed via __atomic_fetch_add */
    volatile int err;    /* set by the first worker whose f() raised          */
} Job;

typedef struct {
    pthread_mutex_t mx;
    pthread_cond_t  cv_work, cv_done, cv_ready;
    pthread_t      *th;
    int nth;             /* live worker threads (does NOT count the parent)   */
    int gen;             /* bumped once per dispatch; workers wake on a change */
    int active;          /* workers still processing the current generation   */
    int ready;           /* workers that have parked at least once (startup)  */
    int shutdown;
    Job job;
} Pool;

static Pool P;
static pthread_mutex_t init_mx = PTHREAD_MUTEX_INITIALIZER;
static int P_ready = 0;

/* Grab morsels until the input is exhausted. Runs on every worker AND on the
 * dispatching parent. Per-index work is byte-for-byte the serial each step:
 * ii(dat,i) makes the owned element, _1(f,item) applies f (consuming item and
 * returning 0 on error), and the owned result lands in its own slot. */
static void run_morsels(Job *j) {
    for (;;) {
        U t = __atomic_fetch_add(&j->cursor, j->grain, __ATOMIC_RELAXED);
        if (t >= j->n) break;
        U hi = t + j->grain; if (hi > j->n) hi = j->n;
        for (U i = t; i < hi; i++) {
            if (j->err) break;                 /* another slice already failed */
            A item = ii(j->dat, i);
            A v = _1(j->f, item);
            if (!v) { j->err = 1; break; }
            j->out[i] = v;
        }
    }
}

static void *worker(void *arg) {
    long idx = (long)arg;
    ray_rc_sync = true;                    /* a worker only ever runs peach work */
    par_prng_perturb((W)(idx + 1));        /* its own decorrelated random stream */
    pthread_mutex_lock(&P.mx);
    int mygen = P.gen;
    /* Announce we have latched our baseline generation and are about to park.
     * pool_init() blocks until every worker has done this, so the FIRST dispatch
     * (which bumps gen under mx) cannot slip past a worker that hadn't yet
     * chosen its baseline -- the lost-wakeup that would otherwise hang the join.
     * Later dispatches are safe without help: a worker between finishing a job
     * and re-parking re-tests gen and picks up the new generation directly. */
    P.ready++;
    pthread_cond_broadcast(&P.cv_ready);
    for (;;) {
        while (P.gen == mygen && !P.shutdown) pthread_cond_wait(&P.cv_work, &P.mx);
        if (P.shutdown) { pthread_mutex_unlock(&P.mx); return 0; }
        mygen = P.gen;
        pthread_mutex_unlock(&P.mx);

        run_morsels(&P.job);

        pthread_mutex_lock(&P.mx);
        if (--P.active == 0) pthread_cond_broadcast(&P.cv_done);
    }
}

/* Clean teardown at process exit so LeakSanitizer sees no live threads or a
 * dangling P.th allocation. */
static void pool_shutdown(void) {
    if (!P_ready) return;
    pthread_mutex_lock(&P.mx);
    P.shutdown = 1; P.gen++;
    pthread_cond_broadcast(&P.cv_work);
    pthread_mutex_unlock(&P.mx);
    for (int i = 0; i < P.nth; i++) pthread_join(P.th[i], 0);
    free(P.th); P.th = 0; P_ready = 0;
}

/* Lazily build the pool the first time peach runs natively. `nw_total` is the
 * desired concurrency INCLUDING the parent, so nw_total-1 worker threads are
 * spawned; the parent makes up the difference. Sized once (AMBER_THREADS is
 * fixed per process); later, smaller jobs simply leave some workers idle. */
static void pool_init(int nw_total) {
    pthread_mutex_lock(&init_mx);
    if (!P_ready) {
        int want = nw_total - 1; if (want < 0) want = 0;
        pthread_mutex_init(&P.mx, 0);
        pthread_cond_init(&P.cv_work, 0);
        pthread_cond_init(&P.cv_done, 0);
        pthread_cond_init(&P.cv_ready, 0);
        P.gen = 0; P.active = 0; P.ready = 0; P.shutdown = 0; P.nth = 0;
        P.th = (pthread_t *)malloc(sizeof(pthread_t) * (want ? want : 1));
        if (P.th) {
            for (int i = 0; i < want; i++) {
                if (pthread_create(&P.th[i], 0, worker, (void *)(long)i) == 0) P.nth++;
                else break;               /* run with however many we got */
            }
        }
        /* Wait until every spawned worker has parked before returning, so the
         * first dispatch cannot outrun a still-starting worker. */
        pthread_mutex_lock(&P.mx);
        while (P.ready < P.nth) pthread_cond_wait(&P.cv_ready, &P.mx);
        pthread_mutex_unlock(&P.mx);
        atexit(pool_shutdown);
        P_ready = 1;
    }
    pthread_mutex_unlock(&init_mx);
}

/* Apply f to each of dat's n items across nw-way parallelism (nw counts the
 * parent). Returns the assembled result -- identical to serial f'dat -- or 0 if
 * any worker's f() raised. f and dat are borrowed. Caller guarantees n>=2 and
 * nw>=2 and that we are NOT already inside a peach scope. */
A peach_pool(A f, A dat, U n, I nw) {
    pool_init((int)nw);

    A *out = (A *)calloc((size_t)n, sizeof(A));   /* zeroed: unfilled slots stay 0 */
    if (!out) {                                   /* OOM: let the caller go serial */
        A u = aA0(n); for (U i = 0; i < n; i++) { A v = _1(f, ii(dat, i)); if (!v) { mr(u); return 0; } u = psh(u, v); } return sqz(u);
    }

    ray_rc_sync = true;                           /* parent joins the parallel scope */

    pthread_mutex_lock(&P.mx);
    P.job.f = f; P.job.dat = dat; P.job.out = out;
    P.job.n = n; P.job.grain = TASK_GRAIN; P.job.cursor = 0; P.job.err = 0;
    P.active = P.nth;                             /* each worker decrements once */
    P.gen++;
    pthread_cond_broadcast(&P.cv_work);
    pthread_mutex_unlock(&P.mx);

    run_morsels(&P.job);                          /* parent pulls its share */

    pthread_mutex_lock(&P.mx);
    while (P.active > 0) pthread_cond_wait(&P.cv_done, &P.mx);
    pthread_mutex_unlock(&P.mx);

    int err = P.job.err;
    ray_rc_sync = false;                          /* back to serial before assembling */

    if (err) {
        for (U i = 0; i < n; i++) if (out[i]) mr(out[i]);
        free(out);
        return 0;
    }
    /* Assemble one general list and squeeze it -- exactly what serial each does,
     * so the result is bit-identical (typed vector when uniform, list otherwise). */
    A r = aA(n); A *e = (A *)_V(r);
    for (U i = 0; i < n; i++) e[i] = out[i];
    AN(n, r); free(out);
    return sqz(r);
}

#endif /* !wasm */
