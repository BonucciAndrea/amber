/ ============================================================================
/ peach.k  -  parallel-each demo.  Embarrassingly-parallel Monte-Carlo, run
/ serially (f'y) and in parallel (peach[f;y]) so you can SEE the speedup.
/ The same shape covers option pricing, per-symbol backtests, bootstraps, etc.
/   run:  AMBER_THREADS=8 ./amber examples/peach.k     (default 4 workers)
/ ============================================================================
\l amber.k
\l fin.k
\l std.k

/ one Monte-Carlo pi estimate from np random points  (pure compute, heavy)
mcpi:{[np]x:np?1.0;y:np?1.0;4.0*(+/1.0>(x*x)+y*y)%np}

NP:20000000                                          / points per task
K:16                                                / independent tasks
tasks:K#NP

`0:"Monte-Carlo pi   ",($K)," tasks x ",($NP)," pts   (workers from $AMBER_THREADS, default 4)\n"
`0:"----------------------------------------------------------------\n"
t0:`t[];sr:avg(mcpi'tasks);      st:0.001*`t[]-t0     / serial  each
t0:`t[];pr:avg(peach[mcpi;tasks]);pt:0.001*`t[]-t0    / parallel each
`0:"serial  f'y      : ",(-8$$st)," ms   pi=",($sr),"\n"
`0:"peach[f;y]        : ",(-8$$pt)," ms   pi=",($pr),"\n"
`0:"speedup           : ",($st%pt),"x\n"
