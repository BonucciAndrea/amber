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

#ifdef __cplusplus
}
#endif
#endif /* AMBER_SIMD_H */
