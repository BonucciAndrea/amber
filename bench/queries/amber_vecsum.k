/ amber_vecsum.k -- Amber-specific vector sum (bench id: vecsum).
/ Companion to k_vecsum.k (the idiomatic ngn/k script run for the "K" row);
/ this file is what actually runs for the "Amber" row of the comparative
/ table, per bench/run_comparative.py's bench_amber().

/ Workload: sum the integers 0..9,999,999 (`+/!10000000`).

/ What was tried, and why the plain form won:
/   This engine exposes real multithreading to K scripts via `peach` (see
/   std.k / examples/peach.k, backed by src/parallel.c's worker pool), so
/   the obvious "engine-level" idea is to chunk the range into N pieces and
/   sum each chunk on its own worker: `+/peach[{+/x};(K,n%K)#!n]`. Measured
/   head-to-head on this machine (nproc=2): the naive single-threaded
/   `+/!10000000` runs in ~3-6ms wall-clock, while the peach-chunked version
/   costs ~30-130ms -- worker-fork/IPC overhead alone is 10-40x the entire
/   cost of the reduction itself. `+/` over a freshly-materialized, flat,
/   contiguous int range is already a single tight pass with no allocation
/   churn beyond the range vector itself, so there is no headroom for
/   threading to recover at this problem size; peach exists for genuinely
/   heavy per-task work (Monte Carlo, per-symbol backtests), not a 10M-elem
/   integer fold. Sum of a contiguous integer range is also exact and
/   order-independent, so correctness wasn't the blocker here -- raw
/   throughput was, and the direct form wins it outright.
/ Result: kept the same direct, contiguous, single-pass form as k_vecsum.k.
/ Values (0..9,999,999) are exact in int64, so this matches k_vecsum.k's
/ output bit-for-bit: 49999995000000.
+/!10000000
