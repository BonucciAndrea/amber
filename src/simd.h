/* simd.h  -  Amber SIMD-accelerated vector kernels.
 * GNU AGPLv3 - see LICENSE and NOTICE.
 *
 * Standalone, dependency-free kernels over raw C arrays. Every kernel is
 * written as portable C (GCC generic vectors or plain counted loops) and, on
 * x86-64 ELF builds without -march, compiled TWICE by function multiversioning
 * (target_clones "avx2"/"default") so the portable binary runs the AVX2 body
 * on a CPU that has it. None of the kernels require aligned input; all handle
 * any n including 0 unless stated. The Amber-facing glue lives in 2.c/3.c/v.c.
 */
#ifndef AMBER_SIMD_H
#define AMBER_SIMD_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Name of the code path in this build ("vec256-mv" = multiversioned and the
 * running CPU has AVX2, "vec128-mv" = multiversioned, baseline selected). */
const char *simd_backend(void);

/* out[i] = a[i] OP b[i]. out may be pointer-equal to a or b. */
void simd_add_i64(const int64_t *a, const int64_t *b, int64_t *out, size_t n);
void simd_add_f64(const double  *a, const double  *b, double  *out, size_t n);
void simd_sub_i64(const int64_t *a, const int64_t *b, int64_t *out, size_t n);
void simd_sub_f64(const double  *a, const double  *b, double  *out, size_t n);
void simd_mul_i64(const int64_t *a, const int64_t *b, int64_t *out, size_t n);
void simd_mul_f64(const double  *a, const double  *b, double  *out, size_t n);
void simd_div_f64(const double  *a, const double  *b, double  *out, size_t n);
/* out[i] = v - b[i] */
void simd_subs_f64(double  v, const double  *b, double  *out, size_t n);
void simd_subs_i64(int64_t v, const int64_t *b, int64_t *out, size_t n);

/* Reductions (empty input returns 0). */
int64_t simd_sum_i64(const int64_t *a, size_t n);
double  simd_sum_f64(const double  *a, size_t n);
int64_t simd_sum_i32(const int32_t *a, size_t n);
int64_t simd_sum_i16(const int16_t *a, size_t n);
int64_t simd_sum_i8 (const int8_t  *a, size_t n);

/* element-wise, narrow integer widths (wrapping) */
void simd_add_i32(const int32_t *a, const int32_t *b, int32_t *out, size_t n);
void simd_add_i16(const int16_t *a, const int16_t *b, int16_t *out, size_t n);
void simd_add_i8 (const int8_t  *a, const int8_t  *b, int8_t  *out, size_t n);
void simd_sub_i32(const int32_t *a, const int32_t *b, int32_t *out, size_t n);
void simd_sub_i16(const int16_t *a, const int16_t *b, int16_t *out, size_t n);
void simd_sub_i8 (const int8_t  *a, const int8_t  *b, int8_t  *out, size_t n);
void simd_adds_i64(int64_t v, const int64_t *b, int64_t *out, size_t n);
void simd_adds_i32(int32_t v, const int32_t *b, int32_t *out, size_t n);
void simd_adds_i16(int16_t v, const int16_t *b, int16_t *out, size_t n);
void simd_adds_i8 (int8_t  v, const int8_t  *b, int8_t  *out, size_t n);

/* Overflow-CHECKED narrow integer kernels: return 1 if any element
 * overflowed the width (result then unusable, redo one width wider), 0 if
 * every element fits. The check is fused into the single pass. */
int simd_addc_i8  (const int8_t  *a, const int8_t  *b, int8_t  *out, size_t n);
int simd_addc_i16 (const int16_t *a, const int16_t *b, int16_t *out, size_t n);
int simd_addc_i32 (const int32_t *a, const int32_t *b, int32_t *out, size_t n);
int simd_subc_i8  (const int8_t  *a, const int8_t  *b, int8_t  *out, size_t n);
int simd_subc_i16 (const int16_t *a, const int16_t *b, int16_t *out, size_t n);
int simd_subc_i32 (const int32_t *a, const int32_t *b, int32_t *out, size_t n);
int simd_addsc_i8 (int8_t  v, const int8_t  *b, int8_t  *out, size_t n);
int simd_addsc_i16(int16_t v, const int16_t *b, int16_t *out, size_t n);
int simd_addsc_i32(int32_t v, const int32_t *b, int32_t *out, size_t n);
int simd_subsc_i8 (int8_t  v, const int8_t  *b, int8_t  *out, size_t n);   /* v - b */
int simd_subsc_i16(int16_t v, const int16_t *b, int16_t *out, size_t n);
int simd_subsc_i32(int32_t v, const int32_t *b, int32_t *out, size_t n);
int simd_mulc_i8  (const int8_t  *a, const int8_t  *b, int8_t  *out, size_t n);
int simd_mulc_i16 (const int16_t *a, const int16_t *b, int16_t *out, size_t n);
int simd_mulc_i32 (const int32_t *a, const int32_t *b, int32_t *out, size_t n);
int simd_mulsc_i8 (int8_t  v, const int8_t  *b, int8_t  *out, size_t n);
int simd_mulsc_i16(int16_t v, const int16_t *b, int16_t *out, size_t n);
int simd_mulsc_i32(int32_t v, const int32_t *b, int32_t *out, size_t n);

/* min/max */
int64_t simd_max_i64(const int64_t *a, size_t n);   /* n>0 */
int64_t simd_min_i64(const int64_t *a, size_t n);   /* n>0 */
/* Float min/max WITH a NaN escape: *sawnan=1 means a NaN was present and the
 * returned value must not be used (caller falls back to the total-order path). */
double simd_max_f64(const double *a, size_t n, int *sawnan);   /* n>0 */
double simd_min_f64(const double *a, size_t n, int *sawnan);   /* n>0 */

/* Fused dot product, bit-identical to `x*y` then simd_sum (same tree). */
double  simd_dot_f64(const double  *a, const double  *b, size_t n);
int64_t simd_dot_i64(const int64_t *a, const int64_t *b, size_t n);

/* Direct float comparison producing one byte (0/1) per element; op: 0 = "<",
 * 1 = ">". *bad=1 when a NaN or -0.0 operand was seen (result then unusable). */
void simd_cmps_f64(const double *a, double v, unsigned char *out, size_t n, int op, int *bad);
void simd_cmpv_f64(const double *a, const double *b, unsigned char *out, size_t n, int op, int *bad);

/* Fused compare-and-count: +/x<y etc. op: 0 <, 1 >, 2 =. */
int64_t simd_cntcmps_f64(const double *a, double v, size_t n, int op, int *bad);
int64_t simd_cntcmpv_f64(const double *a, const double *b, size_t n, int op, int *bad);
int64_t simd_cntcmps_i64(const int64_t *a, int64_t v, size_t n, int op);
int64_t simd_cntcmpv_i64(const int64_t *a, const int64_t *b, size_t n, int op);

/* out = a + s*b (sub=0) or a - s*b (sub=1); product rounded before the add. */
void simd_fma_f64(const double *a, double s, const double *b, double *out, size_t n, int sub);

/* Where / compress over a 0/1 byte mask. Return the number of elements
 * written; *bad=1 when a mask byte exceeded 1 (output then unusable). */
size_t simd_compress_8 (const int8_t  *src, const unsigned char *m, int8_t  *out, size_t n, int *bad);
size_t simd_compress_16(const int16_t *src, const unsigned char *m, int16_t *out, size_t n, int *bad);
size_t simd_compress_32(const int32_t *src, const unsigned char *m, int32_t *out, size_t n, int *bad);
size_t simd_compress_64(const int64_t *src, const unsigned char *m, int64_t *out, size_t n, int *bad);
size_t simd_where_i32(const unsigned char *m, int32_t *out, size_t n, int *bad);
size_t simd_where_i16(const unsigned char *m, int16_t *out, size_t n, int *bad);
int64_t simd_masksum_u8(const unsigned char *m, size_t n, unsigned *orv);
/* +/x@&m in one pass; *bad=1 when a mask byte exceeds 1. */
double  simd_masksum_f64(const double *a, const unsigned char *m, size_t n, int *bad);
int64_t simd_masksum_i64(const int64_t *a, const unsigned char *m, size_t n, int *bad);
/* amber 2.2: +/(a +- s*b)@&m in one pass -- no full-width intermediate for the
 * arithmetic. Bit-identical to simd_fma_f64 followed by simd_masksum_f64
 * (product rounded before the add, same 16-lane accumulation order).
 * sub=0 for a+s*b, sub=1 for a-s*b. *bad=1 when a mask byte exceeds 1. */
double  simd_masksum_fma_f64(const double *a, double s, const double *b,
                             const unsigned char *m, size_t n, int sub, int *bad);
/* Float range scan: 1 if all finite, integral, |x|<=2^53 and no -0.0; then
 * *mn/*mx hold the min/max and *sorted whether the vector is non-decreasing. */
int simd_frange_f64(const double *a, size_t n, double *mn, double *mx, int *sorted);
/* amber 2.2: the two halves simd_frange_f64 is now composed of, exposed so a
 * caller that does not always need the integrality verdict can skip the
 * second pass. Both run at memory bandwidth; the fused loop did not.
 *   frange0   min, max, non-decreasing, and *special (the flags below).
 *             Needs n >= 1, always fills every out-parameter, never fails.
 *             *sorted may only be trusted when *special is 0 -- IEEE `<`
 *             calls -0.0 and 0.0 equal and every NaN unordered, so with
 *             either present "non-decreasing" is not the collation order.
 *   fintegral 1 when every element is integral and |x| <= 2^53 (NaN and the
 *             infinities fail that); -0.0 is integral and is frange0's to
 *             report, not this one's. n may be 0.
 * simd_frange_f64(a,n,...) == frange0(...) && !special && fintegral(a,n). */
#define SIMD_FR_NAN  1
#define SIMD_FR_NEGZ 2
void simd_frange0_f64(const double *a, size_t n, double *mn, double *mx,
                      int *sorted, int *special);
int  simd_fintegral_f64(const double *a, size_t n);

#ifdef __cplusplus
}
#endif
#endif /* AMBER_SIMD_H */
