/* peachpool.c  -  see peachpool.h.
 * GNU AGPLv3 - see LICENSE and NOTICE. */
#if !defined(wasm)
#include "a.h"
#include "peachpool.h"
#include "arena.h"
#include <pthread.h>
#include <stdlib.h>
#include <sys/resource.h>

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
    U err;               /* the lowest index whose f() raised so far; n if none */
    char msg[40];        /* that item's error category, for the re-raise      */
} Job;

/* 2.9.1: a job goes out without a lock. A peach used to cost ~45 us per worker whatever the job (0.3 ms on 8 threads;
 * kdb+ 0.07, l 0.03): every worker took the one pool mutex when it woke and again when it finished, so they queued
 * behind each other, a futex handoff each. Now:
 *  - the parent writes the job, then one 64-bit word w = generation << 32 | shares. A worker that sees a new
 *    generation takes a share by compare-and-swap and then morsels; with no share left it sits the job out. A job of
 *    3 morsels wakes 2 workers, not 7, and a worker late from an older job can't take a share of this one.
 *  - a worker spins a while after a job (peach calls come in runs), then sleeps on its OWN mutex and condvar, so
 *    waking one never waits on another. The parent wakes as many sleepers as there are shares.
 *  - once the parent's own morsels run out, the shares nobody took are taken back (the cursor is past the end, so
 *    they'd find nothing), so it never waits for a worker still waking up. active counts the shares taken and not
 *    done; the last one wakes the parent, only if it went to sleep.
 * w, the sleep flags, active and waiting are seq_cst, so a wake is never lost between a check and a sleep. */
#define PP_SPIN 4000
typedef struct {
    pthread_mutex_t m;
    pthread_cond_t  c;
    int sleeping;
    char pad[64];        /* one worker's flag per cache line */
} Slot;

typedef struct {
    pthread_mutex_t mx;  /* startup and the parent's sleep only */
    pthread_cond_t  cv_done, cv_ready;
    pthread_t      *th;
    Slot           *sl;
    int nth;             /* live worker threads (does NOT count the parent)   */
    int ready;           /* workers that have started (startup)               */
    int shutdown;
    int active;          /* shares taken and not finished                     */
    int waiting;         /* the parent sleeps on cv_done                      */
    unsigned long long w;/* generation << 32 | shares left to take            */
    Job job;
} Pool;

static Pool P;
static pthread_mutex_t init_mx = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t err_mx = PTHREAD_MUTEX_INITIALIZER;   /* taken only when an item fails */
static int P_ready = 0;
#define SC __ATOMIC_SEQ_CST

static inline void pp_relax(void) {
#if defined(__x86_64__) || defined(__i386__)
    __builtin_ia32_pause();
#elif defined(__aarch64__)
    __asm__ __volatile__("yield");
#endif
}

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
            if (i > __atomic_load_n(&j->err, __ATOMIC_RELAXED)) break;   /* an earlier item already failed */
            A item = ii(j->dat, i);
            A v = _1(j->f, item);
            if (!v) {
                /* The failing item with the lowest index records its error
                 * category (the text is in this thread's OWN buffer, gone once
                 * it returns), so the dispatcher re-raises the error serial f'y
                 * would: the first failing item's, whichever worker failed
                 * first (#83 Q16). Items before it still run; items after it
                 * are skipped. */
                pthread_mutex_lock(&err_mx);
                if (i < __atomic_load_n(&j->err, __ATOMIC_RELAXED)) {
                    const char *t = errtext(); size_t k = 0;
                    if (*t == '\'') t++;
                    while (t[k] && t[k] != '\n' && k < sizeof j->msg - 1) { j->msg[k] = t[k]; k++; }
                    j->msg[k] = 0; __atomic_store_n(&j->err, i, __ATOMIC_RELAXED);
                }
                pthread_mutex_unlock(&err_mx);
                break;
            }
            j->out[i] = v;
        }
    }
}

/* a share of generation g: 1 taken, 0 none left or the job moved on */
static int pp_take(unsigned g) {
    unsigned long long w = __atomic_load_n(&P.w, SC);
    while ((unsigned)(w >> 32) == g && (w & 0xffffffffu))
        if (__atomic_compare_exchange_n(&P.w, &w, w - 1, 0, SC, SC)) return 1;
    return 0;
}

/* up to k sleeping workers woken, each under its own lock. The parent wakes two and every worker that takes a share
 * wakes two more while shares are left, so a cold pool wakes as a tree, not one syscall after another */
static void pp_wake(int k) {
    for (int i = 0; i < P.nth && k > 0; i++) {
        Slot *t = &P.sl[i];
        if (!__atomic_load_n(&t->sleeping, SC)) continue;
        pthread_mutex_lock(&t->m); pthread_cond_signal(&t->c); pthread_mutex_unlock(&t->m);
        k--;
    }
}
static void pp_publish(unsigned long long w, int k) { __atomic_store_n(&P.w, w, SC); pp_wake(k); }

static void *worker(void *arg) {
    long idx = (long)arg;
    Slot *me = &P.sl[idx];
    ray_rc_sync = true;                    /* a worker only ever runs peach work */
    par_prng_perturb((W)(idx + 1));        /* its own decorrelated random stream */
    acs((U)(idx + 1));                     /* its own colours for large blocks (m.c) */
    pthread_mutex_lock(&P.mx);
    unsigned seen = (unsigned)(__atomic_load_n(&P.w, SC) >> 32);   /* before pool_init returns, so before any job */
    P.ready++;
    pthread_cond_broadcast(&P.cv_ready);
    pthread_mutex_unlock(&P.mx);
    for (;;) {
        unsigned g = seen;
        for (int s = 0; s < PP_SPIN; s++) {
            g = (unsigned)(__atomic_load_n(&P.w, __ATOMIC_ACQUIRE) >> 32);
            if (g != seen) break;
            pp_relax();
        }
        if (g == seen) {
            pthread_mutex_lock(&me->m);
            __atomic_store_n(&me->sleeping, 1, SC);
            while ((g = (unsigned)(__atomic_load_n(&P.w, SC) >> 32)) == seen) pthread_cond_wait(&me->c, &me->m);
            __atomic_store_n(&me->sleeping, 0, SC);
            pthread_mutex_unlock(&me->m);
        }
        if (__atomic_load_n(&P.shutdown, SC)) { arena_free(); return 0; }   /* this thread's arena (xs sorts in one) goes with it */
        seen = g;
        if (!pp_take(g)) continue;
        if (__atomic_load_n(&P.w, SC) & 0xffffffffu) pp_wake(2);
        run_morsels(&P.job);
        if (__atomic_sub_fetch(&P.active, 1, SC) == 0 && __atomic_load_n(&P.waiting, SC)) {
            pthread_mutex_lock(&P.mx); pthread_cond_signal(&P.cv_done); pthread_mutex_unlock(&P.mx);
        }
    }
}

static unsigned long long pp_next(void) { return (((__atomic_load_n(&P.w, SC) >> 32) + 1) & 0xffffffffull) << 32; }

/* Clean teardown at process exit so LeakSanitizer sees no live threads or a
 * dangling P.th allocation. */
static void pool_shutdown(void) {
    if (!P_ready) return;
    __atomic_store_n(&P.shutdown, 1, SC);
    pp_publish(pp_next(), P.nth);   /* every one, to see shutdown */
    for (int i = 0; i < P.nth; i++) pthread_join(P.th[i], 0);
    free(P.th); P.th = 0; free(P.sl); P.sl = 0; P_ready = 0;
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
        pthread_cond_init(&P.cv_done, 0);
        pthread_cond_init(&P.cv_ready, 0);
        P.w = 0; P.active = 0; P.waiting = 0; P.ready = 0; P.shutdown = 0; P.nth = 0;
        P.th = (pthread_t *)malloc(sizeof(pthread_t) * (want ? want : 1));
        P.sl = (Slot *)calloc(want ? want : 1, sizeof(Slot));
        if (P.sl) for (int i = 0; i < want; i++) { pthread_mutex_init(&P.sl[i].m, 0); pthread_cond_init(&P.sl[i].c, 0); }
        if (P.th && P.sl) {
            /* Digest #5: a worker gets the main thread's stack size. The VM allows every thread the same
             * depth (2048), and the default for other threads can be far smaller (512 KB on macOS), so
             * deep recursion in a worker crashed the process where the main thread ran it fine. */
            pthread_attr_t at; int ha = pthread_attr_init(&at) == 0;
            if (ha) { struct rlimit rl; size_t ss = (size_t)8 << 20;
                if (getrlimit(RLIMIT_STACK, &rl) == 0 && rl.rlim_cur != RLIM_INFINITY && rl.rlim_cur > ss) ss = (size_t)rl.rlim_cur;
                if (ss > ((size_t)64 << 20)) ss = (size_t)64 << 20;
                if (pthread_attr_setstacksize(&at, ss)) { pthread_attr_destroy(&at); ha = 0; } }
            for (int i = 0; i < want; i++) {
                if (pthread_create(&P.th[i], ha ? &at : 0, worker, (void *)(long)i) == 0) P.nth++;
                else break;               /* run with however many we got */
            }
            if (ha) pthread_attr_destroy(&at);
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

    mgive();                                      /* its free blocks to the depot, where the workers find them (m.c) */
    ray_rc_sync = true;                           /* parent joins the parallel scope */

    P.job.f = f; P.job.dat = dat; P.job.out = out;
    /* amber 2.3: the morsel shrinks with the input. A fixed 1,024-item grain
     * handed the WHOLE of any job under 1,025 items to whichever thread took
     * the first ticket, so `f peach x` over a few hundred heavy items (a
     * search per config, a simulation per seed) ran on one core. Now each
     * lane gets ~4 morsels (so uneven items still balance), never more than
     * TASK_GRAIN items per morsel and never fewer than 1. */
    { U g = n / (U)(nw * 4); if (g < 1) g = 1; if (g > TASK_GRAIN) g = TASK_GRAIN;
      P.job.grain = g; }
    P.job.n = n; P.job.cursor = 0; P.job.err = n; P.job.msg[0] = 0;
    /* the parent takes morsels too: a share for each morsel past the first, at most one per worker */
    U mo = (n + P.job.grain - 1) / P.job.grain;
    int k = P.nth; if ((U)k > mo - 1) k = (int)(mo - 1);
    __atomic_store_n(&P.active, k, SC);
    pp_publish(pp_next() | (unsigned)k, k < 2 ? k : 2);

    run_morsels(&P.job);                          /* parent pulls its share */

    /* the cursor is past the end: shares nobody took would find nothing, so they're taken back */
    { unsigned long long w = __atomic_load_n(&P.w, SC); U left;
      while ((left = (U)(w & 0xffffffffu)) && !__atomic_compare_exchange_n(&P.w, &w, w & ~0xffffffffull, 0, SC, SC)) ;
      if (left) __atomic_sub_fetch(&P.active, (int)left, SC); }
    for (int s = 0; s < PP_SPIN && __atomic_load_n(&P.active, SC) > 0; s++) pp_relax();
    if (__atomic_load_n(&P.active, SC) > 0) {
        pthread_mutex_lock(&P.mx);
        __atomic_store_n(&P.waiting, 1, SC);
        while (__atomic_load_n(&P.active, SC) > 0) pthread_cond_wait(&P.cv_done, &P.mx);
        __atomic_store_n(&P.waiting, 0, SC);
        pthread_mutex_unlock(&P.mx);
    }

    int err = P.job.err < n;
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

/* The error category the first failing item of the LAST dispatch raised, or "". */
const char *peach_errmsg(void) { return P.job.msg; }

#endif /* !wasm */
