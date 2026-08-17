/* simd.h  -  Amber SIMD-accelerated vector kernels.
 * GNU AGPLv3 - see LICENSE and NOTICE.
 *
 * Standalone, dependency-free kernels for element-wise add/multiply and
 * sum-reduction over raw `int64_t`/`double` arrays. Each kernel auto-selects,
 * at COMPILE time via preprocessor guards, one of three code paths:
 *
 *   - AVX2   (x86_64, when the compiler defines __AVX2__ -- e.g. gcc/clang
 *             with `-mavx2`)
 *   - NEON   (ARM64 / Apple Silicon, when __ARM_NEON or __aarch64__ is
 *             defined -- NEON is baseline on aarch64, no extra flag needed)
 *   - scalar (portable C99 fallback, used whenever neither of the above is
 *             available, e.g. a plain -O2 x86_64 build without -mavx2, or a
 *             32-bit / non-Arm/non-x86 target)
 *
 * Every kernel operates on plain C arrays (no dependency on Amber's `A`
 * value type or arena), so this file can be unit-tested in complete
 * isolation (see tests/test_simd.c) and reused as a general-purpose
 * library. The Amber-facing glue (building/reading `A` vectors, the `` `simd``
 * self-test builtin) lives in a.c/m.c, not here.
 *
 * Alignment: `arena_alloc()` (arena.h) now hands out 32-byte-aligned
 * scratch by default, which is the natural width for one AVX2 `__m256i`/
 * `__m256d` register (4 x int64 or 4 x double) and is also friendly to
 * NEON's 128-bit registers (any 32-byte-aligned address is 16-byte-aligned
 * too). None of the kernels below *require* aligned input -- they use
 * unaligned loads/stores throughout, since Amber's bucket-allocated heap
 * vectors (mb()/aV() in m.c) are not guaranteed 32-byte aligned -- but
 * arena-backed scratch buffers get the alignment for free when a caller
 * wants to build one directly.
 */
#ifndef AMBER_SIMD_H
#define AMBER_SIMD_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Human-readable name of the code path compiled into this build ("avx2",
 * "neon", or "scalar"). Handy for a self-test / benchmark report. */
const char *simd_backend(void);

/* out[i] = a[i] + b[i]  for i in [0,n).  Any of a/b/out may alias so long as
 * they are pointer-equal (out==a and/or out==b is fine); partial overlap
 * between distinct pointers is undefined, same as memcpy. */
void simd_add_i64(const int64_t *a, const int64_t *b, int64_t *out, size_t n);
void simd_add_f64(const double  *a, const double  *b, double  *out, size_t n);

/* out[i] = a[i] * b[i]  for i in [0,n). */
void simd_mul_i64(const int64_t *a, const int64_t *b, int64_t *out, size_t n);
void simd_mul_f64(const double  *a, const double  *b, double  *out, size_t n);

/* Reduction: returns the sum of a[0..n).  Empty input (n==0) returns 0. */
int64_t simd_sum_i64(const int64_t *a, size_t n);
double  simd_sum_f64(const double  *a, size_t n);


/* ---- amber item 3: widened kernel matrix --------------------------------
 * The six original kernels covered 2 of Amber's 5 numeric widths and 3 of its
 * operations, and nothing in src/2.c or src/3.c called them. These extend the
 * matrix so the primitive dispatch has a kernel for every width it dispatches
 * on. All are unaligned-safe and handle any n, including 0. */

/* element-wise vector OP vector, narrow integer widths */
void simd_add_i32(const int32_t *a, const int32_t *b, int32_t *out, size_t n);
void simd_add_i16(const int16_t *a, const int16_t *b, int16_t *out, size_t n);
void simd_add_i8 (const int8_t  *a, const int8_t  *b, int8_t  *out, size_t n);
void simd_div_f64(const double  *a, const double  *b, double  *out, size_t n);

/* scalar OP vector (the alL/aiI/ahH/agG family in src/2.c) */
void simd_adds_i64(int64_t v, const int64_t *b, int64_t *out, size_t n);
void simd_adds_i32(int32_t v, const int32_t *b, int32_t *out, size_t n);
void simd_adds_i16(int16_t v, const int16_t *b, int16_t *out, size_t n);
void simd_adds_i8 (int8_t  v, const int8_t  *b, int8_t  *out, size_t n);

/* reductions beyond sum. The i64 min/max cannot be auto-vectorised by GCC on
 * baseline x86-64 (SSE2 has no 64-bit signed compare), which is exactly why
 * src/3.c hand-unrolls them; these give the vector form where the ISA has it. */
int64_t simd_max_i64(const int64_t *a, size_t n);   /* n>0 */
int64_t simd_min_i64(const int64_t *a, size_t n);   /* n>0 */

/* Float min/max WITH a NaN escape. K's `|/` over floats currently routes
 * through of1()/of0(), which materialises a whole order-preserving integer
 * copy of the vector. A direct double max avoids that, but only when no NaN is
 * present -- NaN ordering is the one thing the of1() domain defines and IEEE
 * maxsd does not. *sawnan is set to 1 when any NaN was seen, in which case the
 * return value is meaningless and the caller must use the old path. */
double simd_max_f64(const double *a, size_t n, int *sawnan);
double simd_min_f64(const double *a, size_t n, int *sawnan);

/* Fused dot product: sum(a[i]*b[i]) without materialising the product. */
double  simd_dot_f64(const double  *a, const double  *b, size_t n);
int64_t simd_dot_i64(const int64_t *a, const int64_t *b, size_t n);


/* ---- amber item 4: direct float comparison ------------------------------
 * `x>50.0` in src/2.c's arif() folds the WHOLE float vector into an
 * order-preserving 64-bit integer domain with of1() (a fully materialised
 * copy), compares there, and throws the copy away. Measured at 10M elements:
 * float compare 29.9 ms vs integer compare 3.0 ms for the same shape.
 *
 * IEEE comparison on the doubles themselves gives the identical answer EXCEPT
 * for two operand classes, both of which these kernels detect in the same pass
 * and report through *bad, whereupon the caller falls back to the unchanged
 * of1() path:
 *   NaN        -- of1() orders NaN above every real; IEEE makes every
 *                 comparison against it false.
 *   -0.0       -- of1() places -0.0 strictly below +0.0; IEEE says they are
 *                 equal, so `0.0 > -0.0` differs between the two.
 * Both are tested on the raw bit pattern, not with isnan()/signbit(), because
 * the build passes -fno-signed-zeros and -fno-math-errno and the optimiser is
 * entitled to fold the float-domain forms away.
 *
 * op: 0 = "a < b", 1 = "a > b". out[] is one BYTE per element, 0 or 1,
 * matching what cmpZZ() produces today. */
void simd_cmps_f64(const double *a, double v, unsigned char *out, size_t n, int op, int *bad);
void simd_cmpv_f64(const double *a, const double *b, unsigned char *out, size_t n, int op, int *bad);

#ifdef __cplusplus
}
#endif
#endif /* AMBER_SIMD_H */
