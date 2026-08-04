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

const char *simd_backend(void) {
#if defined(AMBER_SIMD_AVX2)
    return "avx2";
#elif defined(AMBER_SIMD_NEON)
    return "neon";
#else
    return "scalar";
#endif
}

/* ---- add -------------------------------------------------------------- */

void simd_add_i64(const int64_t *a, const int64_t *b, int64_t *out, size_t n) {
    size_t i = 0;
#if defined(AMBER_SIMD_AVX2)
    for (; i + 4 <= n; i += 4) {
        __m256i va = _mm256_loadu_si256((const __m256i *)(a + i));
        __m256i vb = _mm256_loadu_si256((const __m256i *)(b + i));
        _mm256_storeu_si256((__m256i *)(out + i), _mm256_add_epi64(va, vb));
    }
#elif defined(AMBER_SIMD_NEON)
    for (; i + 2 <= n; i += 2) {
        int64x2_t va = vld1q_s64(a + i);
        int64x2_t vb = vld1q_s64(b + i);
        vst1q_s64(out + i, vaddq_s64(va, vb));
    }
#endif
    for (; i < n; i++) out[i] = a[i] + b[i]; /* scalar tail (or full fallback) */
}

void simd_add_f64(const double *a, const double *b, double *out, size_t n) {
    size_t i = 0;
#if defined(AMBER_SIMD_AVX2)
    for (; i + 4 <= n; i += 4) {
        __m256d va = _mm256_loadu_pd(a + i);
        __m256d vb = _mm256_loadu_pd(b + i);
        _mm256_storeu_pd(out + i, _mm256_add_pd(va, vb));
    }
#elif defined(AMBER_SIMD_NEON)
    for (; i + 2 <= n; i += 2) {
        float64x2_t va = vld1q_f64(a + i);
        float64x2_t vb = vld1q_f64(b + i);
        vst1q_f64(out + i, vaddq_f64(va, vb));
    }
#endif
    for (; i < n; i++) out[i] = a[i] + b[i];
}

/* ---- multiply ----------------------------------------------------------
 * Neither AVX2 nor NEON has a native 64x64->64 integer multiply instruction
 * (Arm never added one; AVX2's is 32x32->64 only). The i64 path below is
 * therefore: AVX2 does a widened 32x32->64 multiply-and-combine (correct for
 * any product that fits in 64 bits, same constraint plain `int64_t*int64_t`
 * already has in C); NEON extracts each 64-bit lane and multiplies with the
 * scalar unit (still correct and still loads/stores two lanes at a time,
 * just not a single vector-multiply instruction). Both are meaningfully
 * faster than the pure scalar loop because AVX2 batches four multiplies
 * worth of shuffles into one pass and NEON's paired load/store still halves
 * the memory traffic versus scalar. Float64 multiply has a real vector
 * instruction on both ISAs and is fully vectorised below. */

#if defined(AMBER_SIMD_AVX2)
static __m256i mul_i64_avx2(__m256i a, __m256i b) {
    /* (a*b) mod 2^64 via three 32x32->64 partial products:
     * lo(a)*lo(b) + ((lo(a)*hi(b) + hi(a)*lo(b)) << 32) */
    __m256i bswap = _mm256_shuffle_epi32(b, 0xB1);        /* swap hi/lo 32b halves of b */
    __m256i prodlh = _mm256_mullo_epi32(a, bswap);        /* lo(a)*hi(b), hi(a)*lo(b) in each lane pair */
    __m256i zero   = _mm256_setzero_si256();
    __m256i prodlh2 = _mm256_hadd_epi32(prodlh, zero);    /* pack the two cross terms per 64b lane */
    __m256i prodlh3 = _mm256_shuffle_epi32(prodlh2, 0x73);/* move to the high 32 bits of each 64b lane */
    __m256i prodll  = _mm256_mul_epu32(a, b);             /* lo(a)*lo(b), zero-extended to 64b */
    return _mm256_add_epi64(prodll, prodlh3);
}
#endif

void simd_mul_i64(const int64_t *a, const int64_t *b, int64_t *out, size_t n) {
    size_t i = 0;
#if defined(AMBER_SIMD_AVX2)
    for (; i + 4 <= n; i += 4) {
        __m256i va = _mm256_loadu_si256((const __m256i *)(a + i));
        __m256i vb = _mm256_loadu_si256((const __m256i *)(b + i));
        _mm256_storeu_si256((__m256i *)(out + i), mul_i64_avx2(va, vb));
    }
#elif defined(AMBER_SIMD_NEON)
    for (; i + 2 <= n; i += 2) {
        int64x2_t va = vld1q_s64(a + i);
        int64x2_t vb = vld1q_s64(b + i);
        out[i]     = vgetq_lane_s64(va, 0) * vgetq_lane_s64(vb, 0);
        out[i + 1] = vgetq_lane_s64(va, 1) * vgetq_lane_s64(vb, 1);
    }
#endif
    for (; i < n; i++) out[i] = a[i] * b[i];
}

void simd_mul_f64(const double *a, const double *b, double *out, size_t n) {
    size_t i = 0;
#if defined(AMBER_SIMD_AVX2)
    for (; i + 4 <= n; i += 4) {
        __m256d va = _mm256_loadu_pd(a + i);
        __m256d vb = _mm256_loadu_pd(b + i);
        _mm256_storeu_pd(out + i, _mm256_mul_pd(va, vb));
    }
#elif defined(AMBER_SIMD_NEON)
    for (; i + 2 <= n; i += 2) {
        float64x2_t va = vld1q_f64(a + i);
        float64x2_t vb = vld1q_f64(b + i);
        vst1q_f64(out + i, vmulq_f64(va, vb));
    }
#endif
    for (; i < n; i++) out[i] = a[i] * b[i];
}

/* ---- sum reduction ------------------------------------------------------
 * Reduction order differs from a naive left-to-right scalar sum (the SIMD
 * paths sum lanes independently, then combine), so float sums may differ
 * from a scalar reference in the last ULP or two -- expected and harmless
 * for the same reason `+/` over a parallel reduction in any vector engine
 * does. Integer sums are exact regardless of order (mod 2^64 wraparound is
 * identical either way). */

int64_t simd_sum_i64(const int64_t *a, size_t n) {
    size_t i = 0;
    int64_t acc = 0;
#if defined(AMBER_SIMD_AVX2)
    __m256i vacc = _mm256_setzero_si256();
    for (; i + 4 <= n; i += 4)
        vacc = _mm256_add_epi64(vacc, _mm256_loadu_si256((const __m256i *)(a + i)));
    int64_t lanes[4];
    _mm256_storeu_si256((__m256i *)lanes, vacc);
    acc = lanes[0] + lanes[1] + lanes[2] + lanes[3];
#elif defined(AMBER_SIMD_NEON)
    int64x2_t vacc = vdupq_n_s64(0);
    for (; i + 2 <= n; i += 2)
        vacc = vaddq_s64(vacc, vld1q_s64(a + i));
    acc = vgetq_lane_s64(vacc, 0) + vgetq_lane_s64(vacc, 1);
#endif
    for (; i < n; i++) acc += a[i];
    return acc;
}

double simd_sum_f64(const double *a, size_t n) {
    size_t i = 0;
    double acc = 0.0;
#if defined(AMBER_SIMD_AVX2)
    __m256d vacc = _mm256_setzero_pd();
    for (; i + 4 <= n; i += 4)
        vacc = _mm256_add_pd(vacc, _mm256_loadu_pd(a + i));
    double lanes[4];
    _mm256_storeu_pd(lanes, vacc);
    acc = (lanes[0] + lanes[1]) + (lanes[2] + lanes[3]);
#elif defined(AMBER_SIMD_NEON)
    float64x2_t vacc = vdupq_n_f64(0.0);
    for (; i + 2 <= n; i += 2)
        vacc = vaddq_f64(vacc, vld1q_f64(a + i));
    acc = vgetq_lane_f64(vacc, 0) + vgetq_lane_f64(vacc, 1);
#endif
    for (; i < n; i++) acc += a[i];
    return acc;
}
