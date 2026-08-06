/ ============================================================================
/ peach.k  -  parallel-each demo.  Embarrassingly-parallel Monte-Carlo, run
/ serially (f'y) and in parallel (peach[f;y]) so you can SEE the speedup.
/ The same shape covers option pricing, per-symbol backtests, bootstraps, etc.
/   run:  AMBER_THREADS=8 ./amber examples/peach.k     (default: online CPU count)
/ peach dispatches 1,024-element morsels across a persistent POSIX thread pool
/ (src/peachpool.c); each worker draws its own decorrelated random stream, so
/ the parallel pi estimate is an independent Monte-Carlo run, not a replay of
/ the serial one (expect it to agree to ~3 digits, not bit-for-bit).
/ Note on the wasm/browser build: there are no threads in a wasm sandbox, so
/ `peach` there evaluates sequentially (see peachC in src/i.c), matching
/ std.k's peach wrapper. NP below is sized to keep even that serial fallback
/ well under a second; bump it up on a many-core native box for a heavier,
/ more dramatic speedup number.
/ ============================================================================
\l amber.k
\l fin.k
\l std.k

/ one Monte-Carlo pi estimate from np random points  (pure compute, heavy)
mcpi:{[np]x:np?1.0;y:np?1.0;4.0*(+/1.0>(x*x)+y*y)%np}

NP:10000000                                           / points per task
K:16                                                / independent tasks
tasks:K#NP

`0:"Monte-Carlo pi   ",($K)," tasks x ",($NP)," pts   (workers from $AMBER_THREADS, default: online CPU count)\n"
`0:"----------------------------------------------------------------\n"
t0:`t[];sr:avg(mcpi'tasks);      st:0.001*`t[]-t0     / serial  each
t0:`t[];pr:avg(peach[mcpi;tasks]);pt:0.001*`t[]-t0    / parallel each
`0:"serial  f'y      : ",($st)," ms   pi=",($sr),"\n"
`0:"peach[f;y]        : ",($pt)," ms   pi=",($pr),"\n"
`0:"speedup           : ",($st%pt),"x   (bandwidth-bound: see note below)\n"

/ ---- compute-bound, allocation-heavy scaling ------------------------------
/ The Monte-Carlo above sweeps million-element vectors, so on most boxes it is
/ limited by memory bandwidth (the RAM bus wall), not by cores -- its speedup
/ tops out well below the core count no matter how good the allocator is.
/ THIS second benchmark is the case the lock-free per-thread allocator
/ (thread-local bkt[] in src/m.c) is built for: every task iterates a lambda
/ over a small, cache-resident vector, allocating a short-lived temporary each
/ step. Allocation is hammered, but the working set stays in L1/L2, so it scales
/ with cores instead of hitting the bandwidth wall. Under the OLD global
/ allocator lock this ran SLOWER in parallel than serially; now it is near-linear.
`0:"\ncompute-bound, alloc-heavy (near-linear on cores)\n"
`0:"----------------------------------------------------------------\n"
cwork:{[s]+/{1.0+x*0.5}/[300;1000#0.0+s]}
CN:6000
t0:`t[];cs:cwork'!CN;         cst:0.001*`t[]-t0        / serial  each
t0:`t[];cp:peach[cwork;!CN];  cpt:0.001*`t[]-t0        / parallel each
`0:"serial  f'y      : ",($cst)," ms\n"
`0:"peach[f;y]        : ",($cpt)," ms\n"
`0:"speedup           : ",($cst%cpt),"x   (results match: ",($cs~cp),")\n"
