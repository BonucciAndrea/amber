/* peachpool.h  -  Amber persistent thread-pool for parallel-each (`peach`).
 * GNU AGPLv3 - see LICENSE and NOTICE.
 *
 * A single persistent pool of POSIX worker threads, created once (lazily, on
 * the first native `peach` call) and reused for every subsequent dispatch --
 * replacing the old fork()+pipe()+ser8/des9 model in src/i.c. Work is handed
 * out in fixed-size morsels through a lock-free atomic ticket cursor; each
 * worker (and the dispatching parent, which also participates) evaluates the
 * K function `f` over its slice and writes the owned result straight into a
 * pre-allocated shared result slot -- no serialisation, no IPC, no copies.
 *
 * Correctness under threads rests on two things done elsewhere:
 *   - scoped atomic refcounting (a.h's ray_rc_sync / RC_INC / RC_DECV), so the
 *     shared `f` and `dat` can be retained/released from several threads; and
 *   - the allocator + symbol-table lock in src/m.c (also gated on ray_rc_sync),
 *     so concurrent object allocation is safe.
 *
 * WASM / no-pthread builds (-Dwasm) compile this file to nothing; src/i.c keeps
 * the plain serial each there, exactly as before.
 */
#ifndef AMBER_PEACHPOOL_H
#define AMBER_PEACHPOOL_H

/* Elements per morsel. Large enough that per-morsel ticket + dispatch overhead
 * is negligible against the work, small enough that load stays balanced across
 * workers even when per-element cost is uneven. */
#define TASK_GRAIN 1024

#endif /* AMBER_PEACHPOOL_H */
