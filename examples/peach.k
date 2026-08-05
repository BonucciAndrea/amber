/ ============================================================================
/ peach.k  -  parallel-each demo.  Embarrassingly-parallel Monte-Carlo, run
/ serially (f'y) and in parallel (peach[f;y]) so you can SEE the speedup.
/ The same shape covers option pricing, per-symbol backtests, bootstraps, etc.
/   run:  AMBER_THREADS=8 ./amber examples/peach.k     (default: online CPU count)
/ Note on the wasm/browser build: `peach` there falls back to running
/ sequentially (no real fork() in a wasm sandbox -- see peachC's own comment
/ in src/i.c), matching std.k's peach wrapper's documented behavior. NP below
/ is sized to keep the *fallback* serial pass well under a second even at
/ that reduced speed; bump it back up if you're running this natively and
/ want a heavier, more dramatic speedup number.
/ ============================================================================
\l amber.k
\l fin.k
\l std.k

/ one Monte-Carlo pi estimate from np random points  (pure compute, heavy)
mcpi:{[np]x:np?1.0;y:np?1.0;4.0*(+/1.0>(x*x)+y*y)%np}

NP:2000000                                           / points per task
K:16                                                / independent tasks
tasks:K#NP

`0:"Monte-Carlo pi   ",($K)," tasks x ",($NP)," pts   (workers from $AMBER_THREADS, default: online CPU count)\n"
`0:"----------------------------------------------------------------\n"
t0:`t[];sr:avg(mcpi'tasks);      st:round[3]0.001*`t[]-t0     / serial  each
t0:`t[];pr:avg(peach[mcpi;tasks]);pt:round[3]0.001*`t[]-t0    / parallel each
/ st/pt are rounded above: `$` (string-format) pads/truncates to a fixed
/ width by character count, and an un-rounded float can carry enough
/ floating-point noise (e.g. 435.04200000000003) to overflow that width and
/ print garbage -- rounding first keeps the printed string short and exact.
`0:"serial  f'y      : ",(-8$$st)," ms   pi=",($sr),"\n"
`0:"peach[f;y]        : ",(-8$$pt)," ms   pi=",($pr),"\n"
`0:"speedup           : ",($round[3]st%pt),"x\n"
