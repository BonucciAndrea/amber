/* simd.c  -  see simd.h for the contract.
 * GNU AGPLv3 - see LICENSE and NOTICE. */
#include "simd.h"

#if defined(__AVX2__)
  #define AMBER_SIMD_AVX2 1
#endif

#if defined(__has_attribute)
  #if __has_attribute(vector_size)
    #define AMB_VEC 1
  #endif
#elif defined(__GNUC__)
  #define AMB_VEC 1
#endif

/* ==== amber 2.1: function multiversioning ==================================
 * The default ./build.sh binary carries no -march flag so that it runs on any
 * x86-64. That used to pin every kernel in this file to SSE2 (two doubles per
 * register). With target_clones GCC emits an AVX2 body AND a baseline body for
 * each kernel and an ifunc resolver picks the AVX2 one at load time on a CPU
 * that has it -- the portable binary is now as fast as the native one on
 * every kernel that lives here, and still runs on a 2005 Opteron. The
 * mechanism needs an ELF platform with ifunc support (Linux glibc, FreeBSD);
 * Mach-O and wasm get the plain compile, exactly as before. The native build
 * (-march=native) is unaffected: its default clone already has AVX2. */
#if defined(__x86_64__) && defined(__ELF__) && defined(__GNUC__) && !defined(wasm) && !defined(__AVX2__)
  #define AMB_MV __attribute__((target_clones("avx2","default")))
#else
  #define AMB_MV
#endif
/* The vector width is fixed at 32 bytes on x86-64: on an SSE2 target GCC
 * simply splits a 32-byte vector operation into two 16-byte ones (locals only,
 * nothing crosses a function boundary, so no ABI concern), and inside the AVX2
 * clone the same source lowers to one ymm operation. Elsewhere the natural
 * width of the target is used. */
#if defined(__x86_64__)
  #define VBYTES 32
#elif defined(__AVX512F__)
  #define VBYTES 64
#elif defined(__AVX2__)
  #define VBYTES 32
#else
  #define VBYTES 16
#endif
/* Never let the optimiser contract a*b+c into an FMA inside the fused
 * reductions: their contract is bit-identical results to the unfused
 * `x*y` followed by `+/`, which rounds the product before adding. */
#if defined(__GNUC__) && !defined(__clang__)
  #define AMB_NOFMA __attribute__((optimize("-ffp-contract=off")))
#else
  #define AMB_NOFMA
#endif

const char *simd_backend(void) {
#if !defined(AMB_VEC)
    return "scalar";
#elif defined(__x86_64__) && defined(__ELF__) && defined(__GNUC__) && !defined(wasm) && !defined(__AVX2__)
    return __builtin_cpu_supports("avx2") ? "vec256-mv" : "vec128-mv";
#elif VBYTES == 64
    return "vec512";
#elif VBYTES == 32
    return "vec256";
#else
    return "vec128";
#endif
}

#if defined(AMB_VEC)
typedef double   vf64 __attribute__((vector_size(VBYTES)));
typedef int64_t  vi64 __attribute__((vector_size(VBYTES)));
typedef uint64_t vu64 __attribute__((vector_size(VBYTES)));

/* vector OP vector. The body is emitted twice, restrict and non-restrict:
 * simd.h allows out to be pointer-equal to a or b (src/2.c relies on it for the
 * in-place MINE(y) case). */
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
    static inline __attribute__((always_inline)) void FN##_r(                  \
            const T *__restrict a, const T *__restrict b, T *__restrict out, size_t n) \
        AMB_VV_BODY(T, CT, VT, OP, a, b, out)                                  \
    static inline __attribute__((always_inline)) void FN##_a(                  \
            const T *a, const T *b, T *out, size_t n)                          \
        AMB_VV_BODY(T, CT, VT, OP, a, b, out)                                  \
    AMB_MV void FN(const T *a, const T *b, T *out, size_t n) {                 \
        if (out == a || out == b) FN##_a(a, b, out, n);                        \
        else                      FN##_r(a, b, out, n);                        \
    }
/* scalar OP vector (broadcast): out = v OP b */
#define AMB_SV(FN, T, CT, VT, OP)                                              \
    AMB_MV void FN(T v, const T *b, T *out, size_t n) {                        \
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
/* reduction with FOUR independent vector accumulators */
#define AMB_SUM(FN, T, CT, VT)                                                 \
    AMB_MV T FN(const T *__restrict a, size_t n) {                             \
        const size_t L = VBYTES / sizeof(T);                                   \
        VT s0 = {0}, s1 = {0}, s2 = {0}, s3 = {0};                             \
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
/* fused sum of products: the SAME accumulator structure as AMB_SUM over the
 * lane-wise products, so `+/x*y` fused is bit-identical to `x*y` then `+/`
 * (product rounded per lane, then the identical summation tree). */
#define AMB_DOT(FN, T, CT, VT)                                                 \
    AMB_MV AMB_NOFMA T FN(const T *__restrict a, const T *__restrict b, size_t n) { \
        const size_t L = VBYTES / sizeof(T);                                   \
        VT s0 = {0}, s1 = {0}, s2 = {0}, s3 = {0};                             \
        size_t i = 0;                                                          \
        for (; i + 4 * L <= n; i += 4 * L) {                                   \
            VT a0, a1, a2, a3, b0, b1, b2, b3;                                 \
            __builtin_memcpy(&a0, a + i,         VBYTES);                      \
            __builtin_memcpy(&a1, a + i + L,     VBYTES);                      \
            __builtin_memcpy(&a2, a + i + 2 * L, VBYTES);                      \
            __builtin_memcpy(&a3, a + i + 3 * L, VBYTES);                      \
            __builtin_memcpy(&b0, b + i,         VBYTES);                      \
            __builtin_memcpy(&b1, b + i + L,     VBYTES);                      \
            __builtin_memcpy(&b2, b + i + 2 * L, VBYTES);                      \
            __builtin_memcpy(&b3, b + i + 3 * L, VBYTES);                      \
            a0 = a0 * b0; a1 = a1 * b1; a2 = a2 * b2; a3 = a3 * b3;            \
            s0 += a0; s1 += a1; s2 += a2; s3 += a3;                            \
        }                                                                      \
        for (; i + L <= n; i += L) {                                           \
            VT a0, b0; __builtin_memcpy(&a0, a + i, VBYTES);                   \
            __builtin_memcpy(&b0, b + i, VBYTES); a0 = a0 * b0; s0 += a0;      \
        }                                                                      \
        { CT r = 0; size_t k;                                                  \
          VT t0 = (s0 + s1) + (s2 + s3);                                       \
          const CT *lanes = (const CT *)&t0;                                   \
          for (k = 0; k < L; k++) r += lanes[k];                               \
          for (; i < n; i++) { CT p = (CT)a[i] * (CT)b[i]; r += p; }           \
          return (T)r; }                                                       \
    }

AMB_VV(simd_add_f64, double,  double,   vf64, +)
AMB_VV(simd_sub_f64, double,  double,   vf64, -)
AMB_VV(simd_mul_f64, double,  double,   vf64, *)
AMB_VV(simd_div_f64, double,  double,   vf64, /)
AMB_VV(simd_add_i64, int64_t, uint64_t, vu64, +)
AMB_VV(simd_sub_i64, int64_t, uint64_t, vu64, -)
AMB_VV(simd_mul_i64, int64_t, uint64_t, vu64, *)
AMB_SV(simd_subs_f64, double,  double,   vf64, -)
AMB_SV(simd_subs_i64, int64_t, uint64_t, vu64, -)
AMB_SUM(simd_sum_f64, double,  double,   vf64)
AMB_SUM(simd_sum_i64, int64_t, uint64_t, vu64)
AMB_DOT(simd_dot_f64, double,  double,   vf64)
AMB_DOT(simd_dot_i64, int64_t, uint64_t, vu64)

#else  /* no vector extensions: plain scalar loops */
#define AMB_SCALAR_VV(FN, T, CT, OP) \
    void FN(const T *a, const T *b, T *out, size_t n) { \
        for (size_t i = 0; i < n; i++) out[i] = (T)((CT)a[i] OP (CT)b[i]); }
#define AMB_SCALAR_SV(FN, T, CT, OP) \
    void FN(T v, const T *b, T *out, size_t n) { \
        for (size_t i = 0; i < n; i++) out[i] = (T)((CT)v OP (CT)b[i]); }
AMB_SCALAR_VV(simd_add_f64, double,  double,   +)
AMB_SCALAR_VV(simd_sub_f64, double,  double,   -)
AMB_SCALAR_VV(simd_mul_f64, double,  double,   *)
AMB_SCALAR_VV(simd_div_f64, double,  double,   /)
AMB_SCALAR_VV(simd_add_i64, int64_t, uint64_t, +)
AMB_SCALAR_VV(simd_sub_i64, int64_t, uint64_t, -)
AMB_SCALAR_VV(simd_mul_i64, int64_t, uint64_t, *)
AMB_SCALAR_SV(simd_subs_f64, double,  double,   -)
AMB_SCALAR_SV(simd_subs_i64, int64_t, uint64_t, -)
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
AMB_NOFMA double simd_dot_f64(const double *a, const double *b, size_t n) {
    double s0=0,s1=0,s2=0,s3=0; size_t m=n&~(size_t)3,i=0;
    for(;i<m;i+=4){s0+=a[i]*b[i];s1+=a[i+1]*b[i+1];s2+=a[i+2]*b[i+2];s3+=a[i+3]*b[i+3];}
    { double r=(s0+s1)+(s2+s3); for(;i<n;i++)r+=a[i]*b[i]; return r; }
}
int64_t simd_dot_i64(const int64_t *a, const int64_t *b, size_t n) {
    uint64_t s0=0,s1=0,s2=0,s3=0; size_t m=n&~(size_t)3,i=0;
    for(;i<m;i+=4){s0+=(uint64_t)a[i]*(uint64_t)b[i];s1+=(uint64_t)a[i+1]*(uint64_t)b[i+1];
                   s2+=(uint64_t)a[i+2]*(uint64_t)b[i+2];s3+=(uint64_t)a[i+3]*(uint64_t)b[i+3];}
    { uint64_t r=(s0+s1)+(s2+s3); for(;i<n;i++)r+=(uint64_t)a[i]*(uint64_t)b[i]; return (int64_t)r; }
}
#endif


/* ==== element-wise integer kernels (auto-vectorised plain loops) =========== */
/* Integer add wraps deliberately; doing the arithmetic in the unsigned
 * counterpart makes that wrap defined behaviour. */
#define AMB_ADDU(FN, T, UT)                                                    \
    AMB_MV void FN(const T *a, const T *b, T *out, size_t n) {                 \
        const T *ap = a; const T *bp = b; T *op = out;                         \
        for (size_t i = 0; i < n; i++) op[i] = (T)((UT)ap[i] + (UT)bp[i]);     \
    }
#define AMB_ADDSU(FN, T, UT)                                                   \
    AMB_MV void FN(T v, const T *b, T *out, size_t n) {                        \
        const T *bp = b; T *op = out; UT uv = (UT)v;                           \
        for (size_t i = 0; i < n; i++) op[i] = (T)(uv + (UT)bp[i]);            \
    }
AMB_ADDU(simd_add_i32, int32_t, uint32_t)
AMB_ADDU(simd_add_i16, int16_t, uint16_t)
AMB_ADDU(simd_add_i8,  int8_t,  uint8_t)
AMB_ADDSU(simd_adds_i64, int64_t, uint64_t)
AMB_ADDSU(simd_adds_i32, int32_t, uint32_t)
AMB_ADDSU(simd_adds_i16, int16_t, uint16_t)
AMB_ADDSU(simd_adds_i8,  int8_t,  uint8_t)

/* ==== amber 2.1: overflow-CHECKED integer kernels ==========================
 * src/2.c used to run the add and then a SECOND full pass (oZZ) reading x, y
 * and the result to look for sign anomalies. The check here is folded into the
 * same loop: for r = a+b the sign of r differs from the sign of both a and b
 * exactly when the add overflowed, i.e. (r^a)&(r^b) has its top bit set; for
 * r = a-b it is (a^b)&(a^r). The accumulator is an OR over the vector, so it
 * vectorises, and only the top bit of it is inspected. Multiplication is
 * checked by computing the product one width up and testing that it fits.
 * Every kernel returns 1 when SOME element overflowed (the caller then redoes
 * the operation one width wider, as before) and 0 otherwise. Exactly n
 * elements are examined -- padding past n is never read. */
#define AMB_ADDC(FN, T, UT, SIGN)                                              \
    AMB_MV int FN(const T *a, const T *b, T *out, size_t n) {                  \
        const T *ap = a; const T *bp = b; T *op = out; UT acc = 0;             \
        for (size_t i = 0; i < n; i++) {                                       \
            UT x = (UT)ap[i], y = (UT)bp[i], r = x + y;                        \
            acc |= (r ^ x) & (r ^ y); op[i] = (T)r; }                          \
        return (acc & (UT)SIGN) != 0;                                          \
    }
#define AMB_SUBC(FN, T, UT, SIGN)                                              \
    AMB_MV int FN(const T *a, const T *b, T *out, size_t n) {                  \
        const T *ap = a; const T *bp = b; T *op = out; UT acc = 0;             \
        for (size_t i = 0; i < n; i++) {                                       \
            UT x = (UT)ap[i], y = (UT)bp[i], r = x - y;                        \
            acc |= (x ^ y) & (x ^ r); op[i] = (T)r; }                          \
        return (acc & (UT)SIGN) != 0;                                          \
    }
#define AMB_ADDSC(FN, T, UT, SIGN)                                             \
    AMB_MV int FN(T v, const T *b, T *out, size_t n) {                         \
        const T *bp = b; T *op = out; UT acc = 0, x = (UT)v;                   \
        for (size_t i = 0; i < n; i++) {                                       \
            UT y = (UT)bp[i], r = x + y;                                       \
            acc |= (r ^ x) & (r ^ y); op[i] = (T)r; }                          \
        return (acc & (UT)SIGN) != 0;                                          \
    }
#define AMB_SUBSC(FN, T, UT, SIGN)   /* out = v - b */                          \
    AMB_MV int FN(T v, const T *b, T *out, size_t n) {                         \
        const T *bp = b; T *op = out; UT acc = 0, x = (UT)v;                   \
        for (size_t i = 0; i < n; i++) {                                       \
            UT y = (UT)bp[i], r = x - y;                                       \
            acc |= (x ^ y) & (x ^ r); op[i] = (T)r; }                          \
        return (acc & (UT)SIGN) != 0;                                          \
    }
#define AMB_MULC(FN, T, WT)  /* WT: a signed type twice as wide as T */         \
    AMB_MV int FN(const T *a, const T *b, T *out, size_t n) {                  \
        const T *ap = a; const T *bp = b; T *op = out; int bad = 0;            \
        for (size_t i = 0; i < n; i++) {                                       \
            WT p = (WT)ap[i] * (WT)bp[i]; bad |= (p != (WT)(T)p); op[i] = (T)p; } \
        return bad;                                                            \
    }
#define AMB_MULSC(FN, T, WT)                                                   \
    AMB_MV int FN(T v, const T *b, T *out, size_t n) {                         \
        const T *bp = b; T *op = out; int bad = 0; WT x = (WT)v;               \
        for (size_t i = 0; i < n; i++) {                                       \
            WT p = x * (WT)bp[i]; bad |= (p != (WT)(T)p); op[i] = (T)p; }      \
        return bad;                                                            \
    }
AMB_ADDC (simd_addc_i8,   int8_t,  uint8_t,  0x80u)
AMB_ADDC (simd_addc_i16,  int16_t, uint16_t, 0x8000u)
AMB_ADDC (simd_addc_i32,  int32_t, uint32_t, 0x80000000u)
AMB_SUBC (simd_subc_i8,   int8_t,  uint8_t,  0x80u)
AMB_SUBC (simd_subc_i16,  int16_t, uint16_t, 0x8000u)
AMB_SUBC (simd_subc_i32,  int32_t, uint32_t, 0x80000000u)
AMB_ADDSC(simd_addsc_i8,  int8_t,  uint8_t,  0x80u)
AMB_ADDSC(simd_addsc_i16, int16_t, uint16_t, 0x8000u)
AMB_ADDSC(simd_addsc_i32, int32_t, uint32_t, 0x80000000u)
AMB_SUBSC(simd_subsc_i8,  int8_t,  uint8_t,  0x80u)
AMB_SUBSC(simd_subsc_i16, int16_t, uint16_t, 0x8000u)
AMB_SUBSC(simd_subsc_i32, int32_t, uint32_t, 0x80000000u)
AMB_MULC (simd_mulc_i8,   int8_t,  int32_t)
AMB_MULC (simd_mulc_i16,  int16_t, int32_t)
AMB_MULC (simd_mulc_i32,  int32_t, int64_t)
AMB_MULSC(simd_mulsc_i8,  int8_t,  int32_t)
AMB_MULSC(simd_mulsc_i16, int16_t, int32_t)
AMB_MULSC(simd_mulsc_i32, int32_t, int64_t)
/* 64-bit subtraction with a scalar on the RIGHT is x + (-v); the scalar-left
 * form is simd_subs_i64 above. Plain wrapping 64-bit sub kernels: */
AMB_MV void simd_sub_i32(const int32_t *a, const int32_t *b, int32_t *out, size_t n) {
    for (size_t i = 0; i < n; i++) out[i] = (int32_t)((uint32_t)a[i] - (uint32_t)b[i]); }
AMB_MV void simd_sub_i16(const int16_t *a, const int16_t *b, int16_t *out, size_t n) {
    for (size_t i = 0; i < n; i++) out[i] = (int16_t)((uint16_t)a[i] - (uint16_t)b[i]); }
AMB_MV void simd_sub_i8(const int8_t *a, const int8_t *b, int8_t *out, size_t n) {
    for (size_t i = 0; i < n; i++) out[i] = (int8_t)((uint8_t)a[i] - (uint8_t)b[i]); }

/* ==== integer reductions ================================================== */
/* Four independent accumulators, same justification as src/3.c's sumF. */
#define AMB_RED(FN, T, OP)                                                     \
    AMB_MV T FN(const T *a, size_t n) {                                        \
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
/* Narrow integer sums into a 64-bit total. Blocks of 8192 elements are summed
 * in a 32-bit lane accumulator (a block cannot overflow it: 8192*32767 < 2^31),
 * then folded into the 64-bit total, so the whole loop runs on 8/16-lane
 * vector adds instead of 64-bit ones. */
#define AMB_SUMN(FN, T)                                                        \
    AMB_MV int64_t FN(const T *a, size_t n) {                                  \
        int64_t tot = 0; size_t i = 0;                                         \
        while (i < n) {                                                        \
            size_t e = n - i > 8192 ? i + 8192 : n; int32_t s = 0;             \
            for (; i < e; i++) s += (int32_t)a[i];                             \
            tot += s; }                                                        \
        return tot; }
AMB_SUMN(simd_sum_i8,  int8_t)
AMB_SUMN(simd_sum_i16, int16_t)
AMB_MV int64_t simd_sum_i32(const int32_t *a, size_t n) {
    int64_t s0 = 0, s1 = 0, s2 = 0, s3 = 0; size_t m = n & ~(size_t)3, i = 0;
    for (; i < m; i += 4) { s0 += a[i]; s1 += a[i+1]; s2 += a[i+2]; s3 += a[i+3]; }
    { int64_t r = (s0 + s1) + (s2 + s3); for (; i < n; i++) r += a[i]; return r; }
}

/* ==== amber 2.1: float min/max at vector width ============================
 * 16 independent lanes (four vectors' worth) updated with the plain
 * `x > m ? x : m` pattern, which GCC lowers to maxpd/vmaxpd exactly (a NaN
 * operand makes the compare false, so the running value is kept, which is what
 * the scalar code did too); NaN presence is accumulated in parallel so the
 * caller can fall back to the total-order path when it matters. */
#define AMB_MMF(FN, CMP)                                                       \
    AMB_MV double FN(const double *a, size_t n, int *sawnan) {                 \
        enum { LN = 16 };                                                      \
        double m[LN]; int nl[LN]; size_t i = 0, k;                             \
        for (k = 0; k < LN; k++) { m[k] = a[0]; nl[k] = 0; }                   \
        for (; i + LN <= n; i += LN)                                           \
            for (k = 0; k < LN; k++) {                                         \
                double x = a[i + k];                                           \
                nl[k] |= (x != x);                                             \
                m[k] = x CMP m[k] ? x : m[k]; }                                \
        { double r = m[0]; int nn = 0;                                         \
          for (k = 0; k < LN; k++) { nn |= nl[k]; r = m[k] CMP r ? m[k] : r; } \
          for (; i < n; i++) { double x = a[i]; nn |= (x != x); r = x CMP r ? x : r; } \
          *sawnan = nn; return r; }                                            \
    }
AMB_MMF(simd_max_f64, >)
AMB_MMF(simd_min_f64, <)

/* ==== amber item 4: direct float comparison ============================== */
#define AMB_BADBITS(u) (((((u) & 0x7fffffffffffffffULL) > 0x7ff0000000000000ULL) \
                       | ((u) == 0x8000000000000000ULL)) ? 1 : 0)
static inline uint64_t amb_bits(double d) {
    uint64_t u;
    __builtin_memcpy(&u, &d, sizeof u);
    return u;
}
#if defined(AMB_VEC)
  #define AMB_GUARD_DECL                                                       \
      const size_t L = VBYTES / sizeof(double);                                \
      vi64 gacc = {0}, gabs, gexp, gnz;                                        \
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

AMB_MV void simd_cmps_f64(const double *a, double v, unsigned char *out, size_t n, int op, int *bad) {
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

AMB_MV void simd_cmpv_f64(const double *a, const double *b, unsigned char *out, size_t n, int op, int *bad) {
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

/* ==== amber 2.1: fused compare-and-count ==================================
 * +/x<y, +/x>y, +/x=y without the byte vector. op: 0 <, 1 >, 2 =. The float
 * forms report NaN / -0.0 operands through *bad exactly like the compare
 * kernels above, and the caller then takes the unfused path. */
AMB_MV int64_t simd_cntcmps_f64(const double *a, double v, size_t n, int op, int *bad) {
    int64_t c = 0; int b = AMB_BADBITS(amb_bits(v)) ? 1 : 0; size_t i = 0;
    AMB_GUARD_DECL
    if (op == 0) { for (; i + AMB_GUARD_BLOCK <= n; i += AMB_GUARD_BLOCK) { size_t k; AMB_GUARD_STEP(a, i) for (k = 0; k < AMB_GUARD_BLOCK; k++) c += (a[i+k] < v); }
                   for (; i < n; i++) { b |= AMB_BADBITS(amb_bits(a[i])); c += (a[i] < v); } }
    else if (op == 1) { for (; i + AMB_GUARD_BLOCK <= n; i += AMB_GUARD_BLOCK) { size_t k; AMB_GUARD_STEP(a, i) for (k = 0; k < AMB_GUARD_BLOCK; k++) c += (a[i+k] > v); }
                   for (; i < n; i++) { b |= AMB_BADBITS(amb_bits(a[i])); c += (a[i] > v); } }
    else {         for (; i + AMB_GUARD_BLOCK <= n; i += AMB_GUARD_BLOCK) { size_t k; AMB_GUARD_STEP(a, i) for (k = 0; k < AMB_GUARD_BLOCK; k++) c += (a[i+k] == v); }
                   for (; i < n; i++) { b |= AMB_BADBITS(amb_bits(a[i])); c += (a[i] == v); } }
    AMB_GUARD_DONE(b)
    *bad = b; return c;
}
AMB_MV int64_t simd_cntcmpv_f64(const double *a, const double *q, size_t n, int op, int *bad) {
    int64_t c = 0; int b = 0; size_t i = 0;
    AMB_GUARD_DECL
    if (op == 0) { for (; i + AMB_GUARD_BLOCK <= n; i += AMB_GUARD_BLOCK) { size_t k; AMB_GUARD_STEP(a, i) AMB_GUARD_STEP(q, i) for (k = 0; k < AMB_GUARD_BLOCK; k++) c += (a[i+k] < q[i+k]); }
                   for (; i < n; i++) { b |= AMB_BADBITS(amb_bits(a[i])) | AMB_BADBITS(amb_bits(q[i])); c += (a[i] < q[i]); } }
    else if (op == 1) { for (; i + AMB_GUARD_BLOCK <= n; i += AMB_GUARD_BLOCK) { size_t k; AMB_GUARD_STEP(a, i) AMB_GUARD_STEP(q, i) for (k = 0; k < AMB_GUARD_BLOCK; k++) c += (a[i+k] > q[i+k]); }
                   for (; i < n; i++) { b |= AMB_BADBITS(amb_bits(a[i])) | AMB_BADBITS(amb_bits(q[i])); c += (a[i] > q[i]); } }
    else {         for (; i + AMB_GUARD_BLOCK <= n; i += AMB_GUARD_BLOCK) { size_t k; AMB_GUARD_STEP(a, i) AMB_GUARD_STEP(q, i) for (k = 0; k < AMB_GUARD_BLOCK; k++) c += (a[i+k] == q[i+k]); }
                   for (; i < n; i++) { b |= AMB_BADBITS(amb_bits(a[i])) | AMB_BADBITS(amb_bits(q[i])); c += (a[i] == q[i]); } }
    AMB_GUARD_DONE(b)
    *bad = b; return c;
}
AMB_MV int64_t simd_cntcmps_i64(const int64_t *a, int64_t v, size_t n, int op) {
    int64_t c = 0;
    if (op == 0)      for (size_t i = 0; i < n; i++) c += (a[i] < v);
    else if (op == 1) for (size_t i = 0; i < n; i++) c += (a[i] > v);
    else              for (size_t i = 0; i < n; i++) c += (a[i] == v);
    return c;
}
AMB_MV int64_t simd_cntcmpv_i64(const int64_t *a, const int64_t *q, size_t n, int op) {
    int64_t c = 0;
    if (op == 0)      for (size_t i = 0; i < n; i++) c += (a[i] < q[i]);
    else if (op == 1) for (size_t i = 0; i < n; i++) c += (a[i] > q[i]);
    else              for (size_t i = 0; i < n; i++) c += (a[i] == q[i]);
    return c;
}

/* ==== amber 2.1: fused scalar*vector+vector ===============================
 * out = a + s*b (sub=0) or a - s*b (sub=1), product rounded before the add
 * (no FMA contraction) so the result matches `s*b` then `a+` bit for bit.
 * out may alias a or b. */
AMB_MV AMB_NOFMA void simd_fma_f64(const double *a, double s, const double *b, double *out, size_t n, int sub) {
    if (sub) for (size_t i = 0; i < n; i++) { double p = s * b[i]; out[i] = a[i] - p; }
    else     for (size_t i = 0; i < n; i++) { double p = s * b[i]; out[i] = a[i] + p; }
}

/* ==== amber 2.1: where / compress on a byte mask ===========================
 * The mask is one byte per element, 0 or 1. Both kernels are branch-free
 * (an unconditional store and a masked cursor advance), which is bound by the
 * store port at roughly one element per cycle. *bad is set when a mask byte is
 * neither 0 nor 1, in which case the caller discards the output and takes the
 * general path (`&` replicates by count). The cursor advances by (m!=0), so
 * the output can never run past n. */
#define AMB_CMPRS(FN, T)                                                       \
    AMB_MV size_t FN(const T *src, const unsigned char *m, T *out, size_t n, int *bad) { \
        size_t k = 0; unsigned char acc = 0;                                   \
        for (size_t i = 0; i < n; i++) {                                       \
            unsigned char mi = m[i]; acc |= mi;                                \
            out[k] = src[i]; k += (mi != 0); }                                 \
        *bad = acc > 1; return k;                                              \
    }
AMB_CMPRS(simd_compress_8,  int8_t)
AMB_CMPRS(simd_compress_16, int16_t)
AMB_CMPRS(simd_compress_32, int32_t)
AMB_CMPRS(simd_compress_64, int64_t)
AMB_MV size_t simd_where_i32(const unsigned char *m, int32_t *out, size_t n, int *bad) {
    size_t k = 0; unsigned char acc = 0;
    for (size_t i = 0; i < n; i++) { unsigned char mi = m[i]; acc |= mi; out[k] = (int32_t)i; k += (mi != 0); }
    *bad = acc > 1; return k;
}
AMB_MV size_t simd_where_i16(const unsigned char *m, int16_t *out, size_t n, int *bad) {
    size_t k = 0; unsigned char acc = 0;
    for (size_t i = 0; i < n; i++) { unsigned char mi = m[i]; acc |= mi; out[k] = (int16_t)i; k += (mi != 0); }
    *bad = acc > 1; return k;
}
/* ==== amber 2.1: float range scan for the counting sort / distinct ==========
 * One vectorised pass: min, max, whether the vector is non-decreasing, and
 * whether every element is a "plain integer": finite, integral, within 2^53
 * and not -0.0 (tested on the bit pattern, since the build's -fno-signed-zeros
 * lets the compiler fold a signbit test on a value that compares equal to 0).
 * Returns 1 when the vector qualifies for an integer-keyed algorithm. */
AMB_MV int simd_frange_f64(const double *a, size_t n, double *mn, double *mx, int *sorted) {
    const double lim = 9007199254740992.0;
    double lo = a[0], hi = a[0]; int bad = 0, ns = 0; size_t i;
    uint64_t nz = 0;
    for (i = 0; i < n; i++) {
        double u = a[i]; uint64_t b; __builtin_memcpy(&b, &u, 8);
        bad |= !(u >= -lim && u <= lim);           /* NaN compares false -> bad */
        bad |= (__builtin_floor(u) != u);
        nz  |= (b == 0x8000000000000000ULL);
        lo = u < lo ? u : lo; hi = u > hi ? u : hi;
    }
    for (i = 1; i < n; i++) ns |= (a[i] < a[i - 1]);
    if (bad || nz) return 0;
    *mn = lo; *mx = hi; *sorted = !ns; return 1;
}

/* ==== amber 2.1: sum of the elements selected by a 0/1 byte mask ==========
 * +/x@&m without the compressed vector: one pass over x and m. Same
 * accumulator tree as simd_sum (lane-wise masked add), so the f64 result is
 * bit-identical to compress-then-sum on the same elements. *bad=1 when a mask
 * byte exceeds 1 (the caller then takes the general path). */
AMB_MV AMB_NOFMA double simd_masksum_f64(const double *a, const unsigned char *m, size_t n, int *bad) {
    double s[16]; size_t i = 0, k; unsigned char acc = 0;
    for (k = 0; k < 16; k++) s[k] = 0;
    for (; i + 16 <= n; i += 16)
        for (k = 0; k < 16; k++) { unsigned char mi = m[i + k]; acc |= mi; s[k] += mi ? a[i + k] : 0.0; }
    { double r = 0; for (k = 0; k < 16; k++) r += s[k];
      for (; i < n; i++) { unsigned char mi = m[i]; acc |= mi; r += mi ? a[i] : 0.0; }
      *bad = acc > 1; return r; }
}
AMB_MV int64_t simd_masksum_i64(const int64_t *a, const unsigned char *m, size_t n, int *bad) {
    uint64_t r = 0; unsigned char acc = 0;
    for (size_t i = 0; i < n; i++) { unsigned char mi = m[i]; acc |= mi; r += mi ? (uint64_t)a[i] : 0; }
    *bad = acc > 1; return (int64_t)r;
}

/* sum and OR of a byte mask in one pass (sizing + validity for `&`). */
AMB_MV int64_t simd_masksum_u8(const unsigned char *m, size_t n, unsigned *orv) {
    int64_t s = 0; unsigned o = 0; size_t i = 0;
    while (i < n) { size_t e = n - i > 65536 ? i + 65536 : n; unsigned s32 = 0; unsigned char oo = 0;
        for (; i < e; i++) { s32 += m[i]; oo |= m[i]; }
        s += s32; o |= oo; }
    *orv = o; return s;
}
