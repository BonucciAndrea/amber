/* bench/scout/engines/c_ref.c - hand-written C reference for the scout matrix.
 *
 * This is the speed-of-light floor AND the source of truth for every ANSWER.
 * Build:  cc -O3 -march=native -o c_ref c_ref.c -lm
 * Run:    ./c_ref <op> <N> <runs> <warmup>
 *
 * Rules (bench/scout/SCOUT_SPEC.md): single threaded, data generated outside the
 * timed region, median of <runs> timed passes after <warmup> untimed ones, and no
 * algorithm may exploit knowledge that is specific to this data. In particular the
 * sorts are generic LSD radix sorts over the full key width with degenerate-pass
 * skipping - a data-independent optimisation every engine is free to implement -
 * not counting sorts tuned to "the values happen to be < 1000".
 */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <time.h>

/* ------------------------------------------------------------------ constants */
#define MOD   1048573LL
#define MUL    262147LL
#define KJOIN     1000LL          /* right-table rows / distinct join keys */
#define MJOIN  1000000LL          /* left-table rows                       */
#define NT     2000000LL          /* table rows for tablesort / qsql       */
#define MQ      200000LL          /* quote rows                            */
#define QP        2000LL          /* quotes per symbol                     */
#define MT     1000000LL          /* trade rows                            */
#define NSYM       100LL

/* ------------------------------------------------------------------ base data */
static int64_t N;
static int64_t *h, *A, *B;
static double  *X, *Y;

static double now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1e3 + ts.tv_nsec * 1e-6;
}

static void *xmalloc(size_t n) {
    void *p = malloc(n);
    if (!p) { fprintf(stderr, "oom %zu\n", n); exit(2); }
    return p;
}

static void gen_base(void) {
    h = xmalloc(N * sizeof *h);
    A = xmalloc(N * sizeof *A);
    B = xmalloc(N * sizeof *B);
    X = xmalloc(N * sizeof *X);
    Y = xmalloc(N * sizeof *Y);
    for (int64_t i = 0; i < N; i++) {
        int64_t hi = (MUL * i) % MOD;
        h[i] = hi; A[i] = hi % 1000; B[i] = hi % 997;
        X[i] = (double)A[i]; Y[i] = (double)B[i];
    }
}

static double checksum(void) {           /* CHECK = sum(a) + 3*sum(b) */
    int64_t sa = 0, sb = 0;
    for (int64_t i = 0; i < N; i++) { sa += A[i]; sb += B[i]; }
    return (double)(sa + 3 * sb);
}

/* ------------------------------------------------------- generic LSD radix sort
 * 8-bit digits over the full 64-bit key. A pass whose histogram puts every key in
 * one bucket is skipped; that is a property of the histogram, not of this dataset.
 */
static void radix_u64(uint64_t *key, uint64_t *tmp, int64_t n) {
    static int64_t cnt[8][256];
    memset(cnt, 0, sizeof cnt);
    for (int64_t i = 0; i < n; i++) {
        uint64_t k = key[i];
        for (int p = 0; p < 8; p++) cnt[p][(k >> (8 * p)) & 0xFF]++;
    }
    uint64_t *src = key, *dst = tmp;
    for (int p = 0; p < 8; p++) {
        if (cnt[p][(src[0] >> (8 * p)) & 0xFF] == n) continue;   /* degenerate */
        int64_t off[256], s = 0;
        for (int d = 0; d < 256; d++) { off[d] = s; s += cnt[p][d]; }
        for (int64_t i = 0; i < n; i++) dst[off[(src[i] >> (8 * p)) & 0xFF]++] = src[i];
        uint64_t *t = src; src = dst; dst = t;
    }
    if (src != key) memcpy(key, src, n * sizeof *key);
}

/* stable radix sort of (key,payload) pairs; payload rides along */
static void radix_kv(uint64_t *key, int64_t *val, uint64_t *ktmp, int64_t *vtmp, int64_t n) {
    static int64_t cnt[8][256];
    memset(cnt, 0, sizeof cnt);
    for (int64_t i = 0; i < n; i++) {
        uint64_t k = key[i];
        for (int p = 0; p < 8; p++) cnt[p][(k >> (8 * p)) & 0xFF]++;
    }
    uint64_t *ks = key, *kd = ktmp; int64_t *vs = val, *vd = vtmp;
    for (int p = 0; p < 8; p++) {
        if (cnt[p][(ks[0] >> (8 * p)) & 0xFF] == n) continue;
        int64_t off[256], s = 0;
        for (int d = 0; d < 256; d++) { off[d] = s; s += cnt[p][d]; }
        for (int64_t i = 0; i < n; i++) {
            int64_t o = off[(ks[i] >> (8 * p)) & 0xFF]++;
            kd[o] = ks[i]; vd[o] = vs[i];
        }
        uint64_t *kt = ks; ks = kd; kd = kt;
        int64_t  *vt = vs; vs = vd; vd = vt;
    }
    if (ks != key) { memcpy(key, ks, n * sizeof *key); memcpy(val, vs, n * sizeof *val); }
}

/* float64 -> uint64 preserving total order (IEEE754 trick) */
static inline uint64_t f2o(double d) {
    uint64_t u; memcpy(&u, &d, 8);
    return (u & 0x8000000000000000ULL) ? ~u : (u | 0x8000000000000000ULL);
}
static inline double o2f(uint64_t u) {
    u = (u & 0x8000000000000000ULL) ? (u & 0x7FFFFFFFFFFFFFFFULL) : ~u;
    double d; memcpy(&d, &u, 8); return d;
}

/* ------------------------------------------------------------ open-address hash */
typedef struct { int64_t *k; double *v; int64_t *n; int64_t mask; } Htab;

static Htab ht_new(int64_t cap_hint) {
    int64_t cap = 16; while (cap < cap_hint * 2) cap <<= 1;
    Htab t; t.mask = cap - 1;
    t.k = xmalloc(cap * sizeof *t.k);
    t.v = xmalloc(cap * sizeof *t.v);
    t.n = xmalloc(cap * sizeof *t.n);
    for (int64_t i = 0; i < cap; i++) { t.k[i] = INT64_MIN; t.v[i] = 0.0; t.n[i] = 0; }
    return t;
}
static void ht_free(Htab *t) { free(t->k); free(t->v); free(t->n); }
static inline uint64_t mix(uint64_t z) {
    z ^= z >> 33; z *= 0xff51afd7ed558ccdULL; z ^= z >> 33;
    z *= 0xc4ceb9fe1a85ec53ULL; z ^= z >> 33; return z;
}

/* ---------------------------------------------------------------- sort answer */
static double sort_answer(const double *s, int64_t n) {
    double ord = s[0] + s[n / 4] + s[n / 2] + s[(3 * n) / 4] + s[n - 1];
    int64_t inv = 0;
    for (int64_t j = 1; j < n; j++) if (s[j] < s[j - 1]) inv++;
    return ord + 1e9 * (double)inv;
}

/* ================================================================== kernels */
typedef double (*kern_t)(void);
static void *scratch1, *scratch2, *scratch3, *scratch4;

static double k_sum_f(void)  { double s = 0; for (int64_t i = 0; i < N; i++) s += X[i]; return s; }
static double k_max_f(void)  { double m = X[0]; for (int64_t i = 0; i < N; i++) if (X[i] > m) m = X[i]; return m; }
static double k_dot(void)    { double s = 0; for (int64_t i = 0; i < N; i++) s += X[i] * Y[i]; return s; }
static double k_sum_i(void)  { int64_t s = 0; for (int64_t i = 0; i < N; i++) s += A[i]; return (double)s; }
static double k_arith(void)  { double s = 0; for (int64_t i = 0; i < N; i++) if (X[i] > 50.0) s += Y[i] + 2.5 * X[i]; return s; }

static double k_sort_f(void) {
    uint64_t *k = scratch1, *t = scratch2; double *o = scratch3;
    for (int64_t i = 0; i < N; i++) k[i] = f2o(X[i]);
    radix_u64(k, t, N);
    for (int64_t i = 0; i < N; i++) o[i] = o2f(k[i]);
    return sort_answer(o, N);
}
static double k_sort_pre(void) {
    uint64_t *k = scratch1, *t = scratch2; double *o = scratch3;
    const double *pre = scratch4;
    for (int64_t i = 0; i < N; i++) k[i] = f2o(pre[i]);
    radix_u64(k, t, N);
    for (int64_t i = 0; i < N; i++) o[i] = o2f(k[i]);
    return sort_answer(o, N);
}
static double k_grade_i(void) {
    uint64_t *k = scratch1, *kt = scratch2; int64_t *v = scratch3, *vt = scratch4;
    for (int64_t i = 0; i < N; i++) { k[i] = (uint64_t)A[i]; v[i] = i; }
    radix_kv(k, v, kt, vt, N);
    int64_t lim = N < 1000 ? N : 1000, s = 0;
    for (int64_t j = 0; j < lim; j++) s += v[j];
    return (double)s;
}

static int64_t *KR; static double *VR;
static void gen_join_keys(void) {
    KR = xmalloc(KJOIN * sizeof *KR); VR = xmalloc(KJOIN * sizeof *VR);
    for (int64_t j = 0; j < KJOIN; j++) { KR[j] = (7919 * j) % MOD; VR[j] = 2.0 * j; }
}
static double k_find(void) {
    const int64_t *probe = scratch1; int64_t s = 0;
    Htab t = *(Htab *)scratch2;
    for (int64_t i = 0; i < N; i++) {
        uint64_t p = mix((uint64_t)probe[i]) & t.mask;
        while (t.k[p] != probe[i]) p = (p + 1) & t.mask;
        s += t.n[p];
    }
    return (double)s;
}
static double k_member(void) {
    Htab t = *(Htab *)scratch2; int64_t c = 0;
    for (int64_t i = 0; i < N; i++) {
        uint64_t p = mix((uint64_t)h[i]) & t.mask;
        while (t.k[p] != INT64_MIN) { if (t.k[p] == h[i]) { c++; break; } p = (p + 1) & t.mask; }
    }
    return (double)c;
}
static int64_t DIST_MOD;
static double k_distinct(void) {
    Htab t = ht_new(DIST_MOD < N ? DIST_MOD : N);
    int64_t cnt = 0, sum = 0;
    for (int64_t i = 0; i < N; i++) {
        int64_t key = DIST_MOD == 1000 ? A[i] : h[i] % DIST_MOD;
        uint64_t p = mix((uint64_t)key) & t.mask;
        while (t.k[p] != INT64_MIN && t.k[p] != key) p = (p + 1) & t.mask;
        if (t.k[p] == INT64_MIN) { t.k[p] = key; cnt++; sum += key; }
    }
    ht_free(&t);
    return 1e6 * (double)cnt + (double)sum;
}

static int64_t GC;
static double k_group(void) {
    Htab t = ht_new(GC);
    for (int64_t i = 0; i < N; i++) {
        int64_t key = h[i] % GC;
        uint64_t p = mix((uint64_t)key) & t.mask;
        while (t.k[p] != INT64_MIN && t.k[p] != key) p = (p + 1) & t.mask;
        t.k[p] = key; t.v[p] += X[i];
    }
    double s = 0;
    for (int64_t p = 0; p <= t.mask; p++)
        if (t.k[p] != INT64_MIN) s += (double)(1 + t.k[p] % 251) * t.v[p];
    ht_free(&t);
    return s;
}

static double k_join(void) {
    const int64_t *kl = scratch1; const double *vl = scratch3;
    Htab t = *(Htab *)scratch2; double s = 0;
    for (int64_t i = 0; i < MJOIN; i++) {
        uint64_t p = mix((uint64_t)kl[i]) & t.mask;
        while (t.k[p] != INT64_MIN) {
            if (t.k[p] == kl[i]) { s += vl[i] * t.v[p]; break; }
            p = (p + 1) & t.mask;
        }
    }
    return s;
}

static int64_t MW;
static double k_msum(void) {
    double *o = scratch1, run = 0, s = 0;
    for (int64_t i = 0; i < N; i++) {
        run += X[i]; if (i >= MW) run -= X[i - MW];
        o[i] = run; s += run;
    }
    return s;
}
static double k_mavg(void) {
    double *o = scratch1, run = 0, s = 0;
    for (int64_t i = 0; i < N; i++) {
        run += X[i]; if (i >= MW) run -= X[i - MW];
        int64_t c = (i + 1) < MW ? (i + 1) : MW;
        o[i] = run / (double)c; s += o[i];
    }
    return s;
}
static double k_mmax(void) {                      /* monotonic index deque, O(n) */
    int64_t *dq = scratch2; double *o = scratch1;
    int64_t head = 0, tail = 0; double s = 0;
    for (int64_t i = 0; i < N; i++) {
        while (tail > head && X[dq[tail - 1]] <= X[i]) tail--;
        dq[tail++] = i;
        if (dq[head] <= i - MW) head++;
        o[i] = X[dq[head]]; s += o[i];
    }
    return s;
}

static int64_t *TSYM, *TSZ; static double *TPX;
static void gen_table(void) {
    TSYM = xmalloc(NT * sizeof *TSYM); TSZ = xmalloc(NT * sizeof *TSZ);
    TPX  = xmalloc(NT * sizeof *TPX);
    for (int64_t i = 0; i < NT; i++) {
        int64_t hi = (MUL * i) % MOD;
        TSYM[i] = hi % 100; TPX[i] = (double)(hi % 1000); TSZ[i] = hi % 500;
    }
}
static double k_qsql(void) {
    double acc[100]; for (int j = 0; j < 100; j++) acc[j] = 0.0;
    for (int64_t i = 0; i < NT; i++) if (TSZ[i] > 250) acc[TSYM[i]] += TPX[i];
    double s = 0;
    for (int j = 0; j < 100; j++) s += (double)(1 + j % 251) * acc[j];
    return s;
}
static double k_tablesort(void) {
    uint64_t *k = scratch1, *kt = scratch2; int64_t *v = scratch3, *vt = scratch4;
    for (int64_t i = 0; i < NT; i++) {
        k[i] = ((uint64_t)TSYM[i] << 44) | (uint64_t)(int64_t)TPX[i];
        v[i] = i;
    }
    radix_kv(k, v, kt, vt, NT);
    double ord = TPX[v[0]] + TPX[v[NT / 4]] + TPX[v[NT / 2]]
               + TPX[v[(3 * NT) / 4]] + TPX[v[NT - 1]];
    int64_t inv = 0;
    for (int64_t j = 1; j < NT; j++) {
        int64_t s0 = TSYM[v[j - 1]], s1 = TSYM[v[j]];
        if (s1 < s0 || (s1 == s0 && TPX[v[j]] < TPX[v[j - 1]])) inv++;
    }
    return ord + 1e9 * (double)inv;
}

static int64_t *QSYM, *QTIME, *TSY, *TTIME; static double *QBID;
static void gen_asof(void) {
    QSYM = xmalloc(MQ * sizeof *QSYM); QTIME = xmalloc(MQ * sizeof *QTIME);
    QBID = xmalloc(MQ * sizeof *QBID);
    for (int64_t j = 0; j < MQ; j++) {
        QSYM[j] = j / QP; QTIME[j] = 500 * (j % QP) + 1;
        QBID[j] = (double)((j % QP) % 1000);
    }
    TSY = xmalloc(MT * sizeof *TSY); TTIME = xmalloc(MT * sizeof *TTIME);
    for (int64_t i = 0; i < MT; i++) {
        int64_t hi = (MUL * i) % MOD;
        TSY[i] = hi % 100; TTIME[i] = 1000 + i;
    }
}
static double k_asof(void) {
    double s = 0;
    for (int64_t i = 0; i < MT; i++) {
        int64_t base = TSY[i] * QP, lo = 0, hi2 = QP;
        while (lo < hi2) {
            int64_t mid = (lo + hi2) / 2;
            if (QTIME[base + mid] <= TTIME[i]) lo = mid + 1; else hi2 = mid;
        }
        s += QBID[base + lo - 1];
    }
    return s;
}

/* ================================================================== driver */
static int cmpd(const void *a, const void *b) {
    double x = *(const double *)a, y = *(const double *)b;
    return x < y ? -1 : x > y ? 1 : 0;
}

int main(int argc, char **argv) {
    if (argc < 2) { fprintf(stderr, "usage: c_ref <op> [N] [runs] [warmup]\n"); return 2; }
    const char *op = argv[1];
    N       = argc > 2 ? strtoll(argv[2], 0, 10) : 10000000;
    int runs   = argc > 3 ? atoi(argv[3]) : 5;
    int warmup = argc > 4 ? atoi(argv[4]) : 2;

    gen_base();
    double chk = checksum();

    kern_t kern = 0;
    if      (!strcmp(op, "sum_f"))  kern = k_sum_f;
    else if (!strcmp(op, "max_f"))  kern = k_max_f;
    else if (!strcmp(op, "dot"))    kern = k_dot;
    else if (!strcmp(op, "sum_i"))  kern = k_sum_i;
    else if (!strcmp(op, "arith_mask")) kern = k_arith;
    else if (!strcmp(op, "sort_f")) {
        scratch1 = xmalloc(N * 8); scratch2 = xmalloc(N * 8); scratch3 = xmalloc(N * 8);
        kern = k_sort_f;
    } else if (!strcmp(op, "sort_presorted")) {
        scratch1 = xmalloc(N * 8); scratch2 = xmalloc(N * 8); scratch3 = xmalloc(N * 8);
        double *pre = xmalloc(N * 8);
        uint64_t *k = xmalloc(N * 8), *t = xmalloc(N * 8);
        for (int64_t i = 0; i < N; i++) k[i] = f2o(X[i]);
        radix_u64(k, t, N);
        for (int64_t i = 0; i < N; i++) pre[i] = o2f(k[i]);
        free(k); free(t); scratch4 = pre;
        kern = k_sort_pre;
    } else if (!strcmp(op, "grade_i")) {
        scratch1 = xmalloc(N * 8); scratch2 = xmalloc(N * 8);
        scratch3 = xmalloc(N * 8); scratch4 = xmalloc(N * 8);
        kern = k_grade_i;
    } else if (!strcmp(op, "find") || !strcmp(op, "member")) {
        gen_join_keys();
        Htab *t = xmalloc(sizeof *t); *t = ht_new(KJOIN);
        for (int64_t j = 0; j < KJOIN; j++) {
            uint64_t p = mix((uint64_t)KR[j]) & t->mask;
            while (t->k[p] != INT64_MIN) p = (p + 1) & t->mask;
            t->k[p] = KR[j]; t->v[p] = VR[j]; t->n[p] = j;
        }
        scratch2 = t;
        if (!strcmp(op, "find")) {
            int64_t *probe = xmalloc(N * sizeof *probe);
            for (int64_t i = 0; i < N; i++) probe[i] = KR[h[i] % KJOIN];
            scratch1 = probe; kern = k_find;
        } else kern = k_member;
    } else if (!strcmp(op, "distinct"))      { DIST_MOD = 1000;   kern = k_distinct; }
    else if (!strcmp(op, "distinct_100k"))   { DIST_MOD = 100000; kern = k_distinct; }
    else if (!strncmp(op, "group_", 6)) {
        GC = !strcmp(op, "group_10")  ? 10 : !strcmp(op, "group_100") ? 100
           : !strcmp(op, "group_10k") ? 10000 : 100000;
        kern = k_group;
    } else if (!strcmp(op, "join_inner")) {
        gen_join_keys();
        Htab *t = xmalloc(sizeof *t); *t = ht_new(KJOIN);
        for (int64_t j = 0; j < KJOIN; j++) {
            uint64_t p = mix((uint64_t)KR[j]) & t->mask;
            while (t->k[p] != INT64_MIN) p = (p + 1) & t->mask;
            t->k[p] = KR[j]; t->v[p] = VR[j];
        }
        scratch2 = t;
        int64_t *kl = xmalloc(MJOIN * sizeof *kl); double *vl = xmalloc(MJOIN * sizeof *vl);
        for (int64_t i = 0; i < MJOIN; i++) {
            int64_t hi = (MUL * i) % MOD;
            kl[i] = KR[hi % KJOIN]; vl[i] = (double)(hi % 1000);
        }
        scratch1 = kl; scratch3 = vl; kern = k_join;
    } else if (!strcmp(op, "msum_16")) { MW = 16;  scratch1 = xmalloc(N * 8); kern = k_msum; }
    else if (!strcmp(op, "mavg_256"))  { MW = 256; scratch1 = xmalloc(N * 8); kern = k_mavg; }
    else if (!strcmp(op, "mmax_64"))   { MW = 64;  scratch1 = xmalloc(N * 8);
                                         scratch2 = xmalloc(N * 8); kern = k_mmax; }
    else if (!strcmp(op, "qsql_select")) { gen_table(); kern = k_qsql; }
    else if (!strcmp(op, "tablesort")) {
        gen_table();
        scratch1 = xmalloc(NT * 8); scratch2 = xmalloc(NT * 8);
        scratch3 = xmalloc(NT * 8); scratch4 = xmalloc(NT * 8);
        kern = k_tablesort;
    } else if (!strcmp(op, "asof")) { gen_asof(); kern = k_asof; }
    else { printf("SKIP %s\n", op); return 0; }

    double ans = 0;
    for (int i = 0; i < warmup; i++) ans = kern();
    double *t = xmalloc(runs * sizeof *t);
    for (int i = 0; i < runs; i++) {
        double t0 = now_ms(); ans = kern(); t[i] = now_ms() - t0;
    }
    qsort(t, runs, sizeof *t, cmpd);

    printf("BENCH   %s\n", op);
    printf("CHECK   %.0f\n", chk);
    printf("ANSWER  %.17g\n", ans);
    printf("TIME_MS %.6f\n", t[runs / 2]);
    return 0;
}
