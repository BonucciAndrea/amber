/* parallel.h  -  Amber multithreaded vector engine.
 * GNU AGPLv3 - see LICENSE and NOTICE.
 *
 * Splits large element-wise add/multiply and sum-reduction across POSIX
 * threads, handing each thread's chunk to src/simd.c's kernels so the
 * result is both multi-core AND per-core-vectorised. Below a size
 * threshold (PAR_THRESHOLD, 100,000 elements) every function here just
 * calls the single-threaded simd_* kernel directly -- thread creation/join
 * overhead would dominate a small vector's actual work, so there is no
 * benefit (and real cost) to parallelising it.
 *
 * This is a standalone library, like simd.h: no dependency on Amber's `A`
 * value type or arena, operates on plain C arrays, and is unit-tested in
 * isolation (tests/test_parallel.c). The `` `par`` self-test/benchmark
 * builtin (a.c) is the Amber-facing glue, same pattern as `` `simd``.
 *
 * Thread count: AMBER_THREADS env var if set (same variable `peach`/std.k
 * already uses for process-level parallelism, kept consistent rather than
 * inventing a second knob), else the number of online CPUs
 * (sysconf(_SC_NPROCESSORS_ONLN)), capped at PAR_MAX_THREADS. AMBER_THREADS=1
 * forces serial (single-threaded simd_* only, still vectorised).
 */
#ifndef AMBER_PARALLEL_H
#define AMBER_PARALLEL_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PAR_THRESHOLD   100000  /* below this element count, run single-threaded */
#define PAR_MAX_THREADS 64      /* hard cap regardless of AMBER_THREADS/CPU count */

/* Number of worker threads a call of the given size `n` would actually use
 * (1 below PAR_THRESHOLD, otherwise env/CPU-derived and capped). Exposed so
 * callers/tests/benchmarks can report what happened without guessing. */
int par_thread_count(size_t n);

void par_add_i64(const int64_t *a, const int64_t *b, int64_t *out, size_t n);
void par_add_f64(const double  *a, const double  *b, double  *out, size_t n);
void par_mul_i64(const int64_t *a, const int64_t *b, int64_t *out, size_t n);
void par_mul_f64(const double  *a, const double  *b, double  *out, size_t n);

int64_t par_sum_i64(const int64_t *a, size_t n);
double  par_sum_f64(const double  *a, size_t n);

#ifdef __cplusplus
}
#endif
#endif /* AMBER_PARALLEL_H */
