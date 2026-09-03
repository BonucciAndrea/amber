/* simd.c  -  see simd.h for the contract.
 * GNU AGPLv3 - see LICENSE and NOTICE. */
#include "simd.h"

#if defined(__AVX2__)
  #include <immintrin.h>
  #define AMBER_SIMD_AVX2 1
#elif defined(__aarch64__)
  /* NEON is baseline on aarch64 (Apple Silicon, arm64 Linux); the 128-bit
   * int64x2_t/float64x2_t ops below need A64 NEON, not available on plain
   * 32-bit ARMv7 __ARM_NEON, so this path is intentionally aarch64-only. */
  #include <arm_neon.h>
  #define AMBER_SIMD_NEON 1
#endif

#if defined(__has_attribute)
  #if __has_attribute(vector_size)
    #define AMB_VEC 1
  #endif
#elif defined(__GNUC__)
  #define AMB_VEC 1
#endif

#if defined(__AVX512F__)
  #define VBYTES 64
#elif defined(__AVX2__)
  #define VBYTES 32
#else
  #define VBYTES 16
#endif


const char *simd_backend(void) {
#if !defined(AMB_VEC)
    return "scalar";
#elif VBYTES == 64
    return "vec512";
#elif VBYTES == 32
    return "vec256";
#else
    return "vec128";
#endif
}

/* ==== amber item 6: one definition per kernel, every ISA ==================
 * The AVX2/NEON intrinsic bodies these replace were pinned at 256/128 bits and
 * could not use AVX-512 even when -march=native enabled it, because an
 * intrinsic names its register width explicitly. GCC/Clang vector extensions
 * name the width ONCE, here, from the compile-time target, so one kernel
 * definition lowers to SSE2, AVX2, AVX-512 or NEON as the build allows -- no
 * new dependency, no new build flag, and no per-ISA duplication.
 *
 * __builtin_memcpy of exactly VBYTES into a vector-typed local is the portable
 * UNALIGNED load: it compiles to a single vmovdqu/vmovupd, and unlike a
 * pointer cast to a vector type it is defined behaviour on an unaligned
 * address. src/m.c's payloads are cache-line aligned, but correctness here
 * does not depend on that.
 *
 * On a compiler without vector_size the AMB_VEC guard falls back to the plain
 * scalar loop, which is what the original file's #else branch did. */

#if defined(AMB_VEC)
typedef double   vf64 __attribute__((vector_size(VBYTES)));
typedef int64_t  vi64 __attribute__((vector_size(VBYTES)));
typedef int32_t  vi32 __attribute__((vector_size(VBYTES)));
typedef int16_t  vi16 __attribute__((vector_size(VBYTES)));
typedef int8_t   vi8  __attribute__((vector_size(VBYTES)));
/* The INTEGER kernels compute in this unsigned counterpart, never in vi64 --
 * same reason as AMB_ADDU below and the note in src/2.c: the wrap is
 * deliberate (oZZ() reads the overflow back out of the result's sign bits and
 * the caller re-runs one width wider), and signed overflow is undefined
 * behaviour that UBSan reports and that the optimiser is entitled to assume
 * away, taking the check with it. Two's-complement wrap in the unsigned type
 * is defined and lowers to the identical vpaddq/vpmullq. UBSan caught this on
 * test-euler.k, whose bignum limbs add straight through 2^63. */
typedef uint64_t vu64 __attribute__((vector_size(VBYTES)));

/* vector OP vector */
/* The inner loop is emitted TWICE: once with all three pointers restrict-
 * qualified, once without. simd.h's contract allows out to be pointer-equal to
 * a or b (src/2.c relies on it for the in-place MINE(y) case), so restrict
 * cannot be applied unconditionally -- out==a would then be undefined
 * behaviour. Without it GCC must assume arbitrary overlap and emits a
 * scalar-ish fallback; the original hand-written kernels in 2.c carried RES on
 * every pointer, and dropping it cost ~12% on float multiply (measured on the
 * inner-join cell, where it showed up as a whole-workload regression). */
/* CT is the type the arithmetic is DONE in: T itself for the float kernels,
 * T's unsigned counterpart for the integer ones. VT is the vector type that
 * matches CT. The load and the store are byte copies either way, so reading a
 * const T* into a VT and storing a VT back through a T* is exact. */
#define AMB_VV_BODY(T, CT, VT, OP, A, B, O)                                    \
        { const size_t L = VBYTES / sizeof(T);                                 \
          size_t i = 0;                                                        \
          for (; i + L <= n; i += L) {                                         \
              VT va, vb, vo;                                                   \
              __builtin_memcpy(&va, (A) + i, VBYTES);                          \
              __builtin_memcpy(&vb, (B) + i, VBYTES);                          \
              vo = va OP vb;                                                   \
              __builtin_memcpy((O) + i, &vo, VBYTES);                          \
          }                                                                    \
          for (; i < n; i++) (O)[i] = (T)((CT)(A)[i] OP (CT)(B)[i]); }
#define AMB_VV(FN, T, CT, VT, OP)                                              \
    static void FN##_r(const T *__restrict a, const T *__restrict b,           \
                       T *__restrict out, size_t n)                            \
        AMB_VV_BODY(T, CT, VT, OP, a, b, out)                                  \
    static void FN##_a(const T *a, const T *b, T *out, size_t n)               \
        AMB_VV_BODY(T, CT, VT, OP, a, b, out)                                  \
    void FN(const T *a, const T *b, T *out, size_t n) {                        \
        if (out == a || out == b) FN##_a(a, b, out, n);                        \
        else                      FN##_r(a, b, out, n);                        \
    }
/* scalar OP vector (broadcast) */
#define AMB_SV(FN, T, CT, VT, OP)                                              \
    void FN(T v, const T *b, T *out, size_t n) {                               \
        const size_t L = VBYTES / sizeof(T);                                   \
        size_t i = 0;                                                          \
        VT vv;                                                                 \
        { size_t k; CT *s = (CT *)&vv; for (k = 0; k < L; k++) s[k] = (CT)v; } \
        for (; i + L <= n; i += L) {                                           \
            VT vb, vo;                                                         \
            __builtin_memcpy(&vb, b + i, VBYTES);                              \
            vo = vv OP vb;                                                     \
            __builtin_memcpy(out + i, &vo, VBYTES);                            \
        }                                                                      \
        for (; i < n; i++) out[i] = (T)((CT)v OP (CT)b[i]);                    \
    }
/* reduction with FOUR independent vector accumulators: 4 x VBYTES in flight,
 * which is what it takes to cover FP-add latency on a modern core. */
#define AMB_SUM(FN, T, CT, VT, ZERO)                                           \
    T FN(const T *__restrict a, size_t n) {                                    \
        const size_t L = VBYTES / sizeof(T);                                   \
        VT s0 = ZERO, s1 = ZERO, s2 = ZERO, s3 = ZERO;                         \
        size_t i = 0;                                                          \
        for (; i + 4 * L <= n; i += 4 * L) {                                   \
            VT v0, v1, v2, v3;                                                 \
            __builtin_memcpy(&v0, a + i,         VBYTES);                      \
            __builtin_memcpy(&v1, a + i + L,     VBYTES);                      \
            __builtin_memcpy(&v2, a + i + 2 * L, VBYTES);                      \
            __builtin_memcpy(&v3, a + i + 3 * L, VBYTES);                      \
            s0 += v0; s1 += v1; s2 += v2; s3 += v3;                            \
        }                                                                      \
        for (; i + L <= n; i += L) {                                           \
            VT v0; __builtin_memcpy(&v0, a + i, VBYTES); s0 += v0;             \
        }                                                                      \
        { CT r = 0; size_t k;                                                  \
          VT t0 = (s0 + s1) + (s2 + s3);                                       \
          const CT *lanes = (const CT *)&t0;                                   \
          for (k = 0; k < L; k++) r += lanes[k];                               \
          for (; i < n; i++) r += (CT)a[i];                                    \
          return (T)r; }                                                       \
    }

AMB_VV(simd_add_f64, double,  double,   vf64, +)
AMB_VV(simd_mul_f64, double,  double,   vf64, *)
AMB_VV(simd_add_i64, int64_t, uint64_t, vu64, +)
AMB_VV(simd_mul_i64, int64_t, uint64_t, vu64, *)
AMB_SUM(simd_sum_f64, double,  double,   vf64, (vf64){0})
AMB_SUM(simd_sum_i64, int64_t, uint64_t, vu64, (vu64){0})

#else  /* no vector extensions: plain scalar loops, as before */
#define AMB_SCALAR_VV(FN, T, CT, OP) \
    void FN(const T *a, const T *b, T *out, size_t n) { \
        for (size_t i = 0; i < n; i++) out[i] = (T)((CT)a[i] OP (CT)b[i]); }
AMB_SCALAR_VV(simd_add_f64, double,  double,   +)
AMB_SCALAR_VV(simd_mul_f64, double,  double,   *)
AMB_SCALAR_VV(simd_add_i64, int64_t, uint64_t, +)
AMB_SCALAR_VV(simd_mul_i64, int64_t, uint64_t, *)
double  simd_sum_f64(const double *a, size_t n) {
    double s0=0,s1=0,s2=0,s3=0; size_t m=n&~(size_t)3,i=0;
    for(;i<m;i+=4){s0+=a[i];s1+=a[i+1];s2+=a[i+2];s3+=a[i+3];}
    { double r=(s0+s1)+(s2+s3); for(;i<n;i++)r+=a[i]; return r; }
}
int64_t simd_sum_i64(const int64_t *a, size_t n) {
    uint64_t s0=0,s1=0,s2=0,s3=0; size_t m=n&~(size_t)3,i=0;
    for(;i<m;i+=4){s0+=(uint64_t)a[i];s1+=(uint64_t)a[i+1];s2+=(uint64_t)a[i+2];s3+=(uint64_t)a[i+3];}
    { uint64_t r=(s0+s1)+(s2+s3); for(;i<n;i++)r+=(uint64_t)a[i]; return (int64_t)r; }
}
#endif


/* ==== amber item 3: widened kernel matrix ================================
 * Written as plain restrict-qualified counted loops rather than intrinsics.
 * At -O3 (and especially with -march=native) both GCC and Clang vectorise
 * these to the full native width -- including AVX-512, which the hand-written
 * AVX2 intrinsics above cannot reach. Item 6 replaces the intrinsic kernels
 * with the same portable form. Every loop is unaligned-safe: no assumption is
 * made about the alignment of a/b/out. */

#define AMB_BIN(FN, T, OP)                                                     \
    void FN(const T *a, const T *b, T *out, size_t n) {                        \
        const T *ap = a; const T *bp = b; T *op = out;                         \
        for (size_t i = 0; i < n; i++) op[i] = (T)(ap[i] OP bp[i]);            \
    }

/* Integer add wraps deliberately, matching src/2.c: the caller detects the
 * overflow from the result's sign bits afterwards and re-runs one width
 * wider. Doing the arithmetic in the unsigned counterpart makes that wrap
 * defined behaviour instead of UB the optimiser may delete the check for. */
#define AMB_ADDU(FN, T, UT)                                                    \
    void FN(const T *a, const T *b, T *out, size_t n) {                        \
        const T *ap = a; const T *bp = b; T *op = out;                         \
        for (size_t i = 0; i < n; i++) op[i] = (T)((UT)ap[i] + (UT)bp[i]);     \
    }
#define AMB_ADDSU(FN, T, UT)                                                   \
    void FN(T v, const T *b, T *out, size_t n) {                               \
        const T *bp = b; T *op = out; UT uv = (UT)v;                           \
        for (size_t i = 0; i < n; i++) op[i] = (T)(uv + (UT)bp[i]);            \
    }

AMB_ADDU(simd_add_i32, int32_t, uint32_t)
AMB_ADDU(simd_add_i16, int16_t, uint16_t)
AMB_ADDU(simd_add_i8,  int8_t,  uint8_t)
AMB_BIN (simd_div_f64, double, /)

AMB_ADDSU(simd_adds_i64, int64_t, uint64_t)
AMB_ADDSU(simd_adds_i32, int32_t, uint32_t)
AMB_ADDSU(simd_adds_i16, int16_t, uint16_t)
AMB_ADDSU(simd_adds_i8,  int8_t,  uint8_t)

/* Four independent accumulators, same justification as src/3.c's sumF: it
 * breaks the loop-carried dependence so the vectoriser can issue one wide
 * min/max per group instead of a serialised cmov chain. */
#define AMB_RED(FN, T, OP)                                                     \
    T FN(const T *a, size_t n) {                                               \
        const T *p = a;                                                        \
        T r0 = p[0], r1 = p[0], r2 = p[0], r3 = p[0];                          \
        size_t m = n & ~(size_t)3, i = 0;                                      \
        for (; i < m; i += 4) {                                                \
            T v0 = p[i], v1 = p[i+1], v2 = p[i+2], v3 = p[i+3];                \
            r0 = OP(r0, v0); r1 = OP(r1, v1);                                  \
            r2 = OP(r2, v2); r3 = OP(r3, v3);                                  \
        }                                                                      \
        { T r = OP(OP(r0, r1), OP(r2, r3));                                    \
          for (; i < n; i++) r = OP(r, p[i]);                                  \
          return r; }                                                          \
    }
#define AMB_MAX(x, y) ((x) > (y) ? (x) : (y))
#define AMB_MIN(x, y) ((x) < (y) ? (x) : (y))

AMB_RED(simd_max_i64, int64_t, AMB_MAX)
AMB_RED(simd_min_i64, int64_t, AMB_MIN)

/* NaN is detected with v!=v rather than isnan() so no <math.h> is needed and
 * -fno-math-errno cannot elide it. The check is folded into the same pass, so
 * a NaN-free vector -- the overwhelmingly common case -- pays one extra
 * compare per element and no second traversal. */
double simd_max_f64(const double *a, size_t n, int *sawnan) {
    const double *p = a;
    double r0 = p[0], r1 = p[0], r2 = p[0], r3 = p[0];
    int nan0 = 0, nan1 = 0, nan2 = 0, nan3 = 0;
    size_t m = n & ~(size_t)3, i = 0;
    for (; i < m; i += 4) {
        double v0 = p[i], v1 = p[i+1], v2 = p[i+2], v3 = p[i+3];
        nan0 |= v0 != v0; nan1 |= v1 != v1; nan2 |= v2 != v2; nan3 |= v3 != v3;
        r0 = AMB_MAX(r0, v0); r1 = AMB_MAX(r1, v1);
        r2 = AMB_MAX(r2, v2); r3 = AMB_MAX(r3, v3);
    }
    { double r = AMB_MAX(AMB_MAX(r0, r1), AMB_MAX(r2, r3));
      int nn = nan0 | nan1 | nan2 | nan3;
      for (; i < n; i++) { double v = p[i]; nn |= v != v; r = AMB_MAX(r, v); }
      *sawnan = nn; return r; }
}
double simd_min_f64(const double *a, size_t n, int *sawnan) {
    const double *p = a;
    double r0 = p[0], r1 = p[0], r2 = p[0], r3 = p[0];
    int nan0 = 0, nan1 = 0, nan2 = 0, nan3 = 0;
    size_t m = n & ~(size_t)3, i = 0;
    for (; i < m; i += 4) {
        double v0 = p[i], v1 = p[i+1], v2 = p[i+2], v3 = p[i+3];
        nan0 |= v0 != v0; nan1 |= v1 != v1; nan2 |= v2 != v2; nan3 |= v3 != v3;
        r0 = AMB_MIN(r0, v0); r1 = AMB_MIN(r1, v1);
        r2 = AMB_MIN(r2, v2); r3 = AMB_MIN(r3, v3);
    }
    { double r = AMB_MIN(AMB_MIN(r0, r1), AMB_MIN(r2, r3));
      int nn = nan0 | nan1 | nan2 | nan3;
      for (; i < n; i++) { double v = p[i]; nn |= v != v; r = AMB_MIN(r, v); }
      *sawnan = nn; return r; }
}

double simd_dot_f64(const double *a, const double *b, size_t n) {
    const double *x = a; const double *y = b;
    double s0 = 0, s1 = 0, s2 = 0, s3 = 0;
    size_t m = n & ~(size_t)3, i = 0;
    for (; i < m; i += 4) {
        s0 += x[i]   * y[i];   s1 += x[i+1] * y[i+1];
        s2 += x[i+2] * y[i+2]; s3 += x[i+3] * y[i+3];
    }
    { double r = (s0 + s1) + (s2 + s3);
      for (; i < n; i++) r += x[i] * y[i];
      return r; }
}
int64_t simd_dot_i64(const int64_t *a, const int64_t *b, size_t n) {
    const int64_t *x = a; const int64_t *y = b;
    uint64_t s0 = 0, s1 = 0, s2 = 0, s3 = 0;   /* wrap is defined, matches src/2.c */
    size_t m = n & ~(size_t)3, i = 0;
    for (; i < m; i += 4) {
        s0 += (uint64_t)x[i]   * (uint64_t)y[i];
        s1 += (uint64_t)x[i+1] * (uint64_t)y[i+1];
        s2 += (uint64_t)x[i+2] * (uint64_t)y[i+2];
        s3 += (uint64_t)x[i+3] * (uint64_t)y[i+3];
    }
    { uint64_t r = (s0 + s1) + (s2 + s3);
      for (; i < n; i++) r += (uint64_t)x[i] * (uint64_t)y[i];
      return (int64_t)r; }
}

/* ==== amber item 4: direct float comparison ============================== */

/* Bit-pattern predicates. u is the raw IEEE-754 encoding.
 *   NaN  : exponent all ones AND mantissa non-zero, i.e. (u & ~sign) > +inf
 *   -0.0 : the sign bit alone */
#define AMB_BADBITS(u) (((((u) & 0x7fffffffffffffffULL) > 0x7ff0000000000000ULL) \
                       | ((u) == 0x8000000000000000ULL)) ? 1 : 0)

static uint64_t amb_bits(double d) {
    uint64_t u;
    __builtin_memcpy(&u, &d, sizeof u);   /* the portable, strict-aliasing-safe bitcast */
    return u;
}

/* The guard is the expensive part when the data is CACHE-RESIDENT: at 10M
 * elements the loop waits on memory and four extra scalar ALU ops per element
 * are free, but at 1M everything is in L3 and they showed up as a 21%
 * regression on the bit-mask cell. Doing the guard on VECTOR lanes -- one
 * and/compare/or group per L elements instead of per element -- removes that,
 * while the output loop keeps its scalar source form, which the compiler
 * already vectorises into compare+pack. */
#if defined(AMB_VEC)
  #define AMB_GUARD_DECL                                                       \
      const size_t L = VBYTES / sizeof(double);                                \
      vi64 gacc = (vi64){0}, gabs, gexp, gnz;                                  \
      { size_t k; int64_t *pa = (int64_t *)&gabs, *pe = (int64_t *)&gexp,      \
                          *pn = (int64_t *)&gnz;                               \
        for (k = 0; k < L; k++) {                                              \
            pa[k] = (int64_t)0x7fffffffffffffffLL;                             \
            pe[k] = (int64_t)0x7ff0000000000000LL;                             \
            pn[k] = (int64_t)0x8000000000000000ULL; } }
  #define AMB_GUARD_STEP(P, I)                                                 \
      { vi64 u; __builtin_memcpy(&u, (P) + (I), VBYTES);                       \
        gacc |= ((u & gabs) > gexp) | (u == gnz); }
  #define AMB_GUARD_DONE(OUT)                                                  \
      { size_t k; int64_t *g = (int64_t *)&gacc; int any = 0;                  \
        for (k = 0; k < L; k++) any |= (g[k] != 0);                            \
        (OUT) |= any; }
  #define AMB_GUARD_BLOCK  L
#else
  #define AMB_GUARD_DECL   const size_t L = 1; int gsc = 0;
  #define AMB_GUARD_STEP(P, I) { gsc |= AMB_BADBITS(amb_bits((P)[I])); }
  #define AMB_GUARD_DONE(OUT) { (OUT) |= gsc; }
  #define AMB_GUARD_BLOCK  L
#endif

void simd_cmps_f64(const double *a, double v, unsigned char *out, size_t n, int op, int *bad) {
    const double *p = a;
    unsigned char *o = out;
    int b = AMB_BADBITS(amb_bits(v)) ? 1 : 0;
    size_t i = 0;
    AMB_GUARD_DECL
    if (op) {
        for (; i + AMB_GUARD_BLOCK <= n; i += AMB_GUARD_BLOCK) {
            size_t k;
            AMB_GUARD_STEP(p, i)
            for (k = 0; k < AMB_GUARD_BLOCK; k++) o[i + k] = (unsigned char)(p[i + k] > v);
        }
        for (; i < n; i++) { b |= AMB_BADBITS(amb_bits(p[i])); o[i] = (unsigned char)(p[i] > v); }
    } else {
        for (; i + AMB_GUARD_BLOCK <= n; i += AMB_GUARD_BLOCK) {
            size_t k;
            AMB_GUARD_STEP(p, i)
            for (k = 0; k < AMB_GUARD_BLOCK; k++) o[i + k] = (unsigned char)(p[i + k] < v);
        }
        for (; i < n; i++) { b |= AMB_BADBITS(amb_bits(p[i])); o[i] = (unsigned char)(p[i] < v); }
    }
    AMB_GUARD_DONE(b)
    *bad = b;
}

void simd_cmpv_f64(const double *a, const double *b, unsigned char *out, size_t n, int op, int *bad) {
    const double *p = a; const double *q = b;
    unsigned char *o = out;
    int bb = 0;
    size_t i = 0;
    AMB_GUARD_DECL
    if (op) {
        for (; i + AMB_GUARD_BLOCK <= n; i += AMB_GUARD_BLOCK) {
            size_t k;
            AMB_GUARD_STEP(p, i) AMB_GUARD_STEP(q, i)
            for (k = 0; k < AMB_GUARD_BLOCK; k++) o[i + k] = (unsigned char)(p[i + k] > q[i + k]);
        }
        for (; i < n; i++) { bb |= AMB_BADBITS(amb_bits(p[i])) | AMB_BADBITS(amb_bits(q[i]));
                             o[i] = (unsigned char)(p[i] > q[i]); }
    } else {
        for (; i + AMB_GUARD_BLOCK <= n; i += AMB_GUARD_BLOCK) {
            size_t k;
            AMB_GUARD_STEP(p, i) AMB_GUARD_STEP(q, i)
            for (k = 0; k < AMB_GUARD_BLOCK; k++) o[i + k] = (unsigned char)(p[i + k] < q[i + k]);
        }
        for (; i < n; i++) { bb |= AMB_BADBITS(amb_bits(p[i])) | AMB_BADBITS(amb_bits(q[i]));
                             o[i] = (unsigned char)(p[i] < q[i]); }
    }
    AMB_GUARD_DONE(bb)
    *bad = bb;
}
