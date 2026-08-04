\l amber.k
\l std.k
/ ============================================================================
/ demo/bench_showcase.k  -  Mega Demo, part 2: engine speed & parallelism.
/ --
/ Benchmarks Amber's core vector ops at 10,000,000 elements three ways:
/ a forced per-element scalar path (each, `'`), the native vector kernel
/ (elementwise +, *, and the +/ sum reduction -- auto-SIMD in the C kernels),
/ and a multi-core reduction via `peach` (see examples/peach.k for the
/ underlying pattern). Prints a formatted speedup table for each comparison.
/ --
/ (globals below are named `vx`/`vy`, not `x`/`y` -- inside a brace lambda
/ with no declared params, `x`/`y`/`z` are the function's OWN implicit
/ parameters, so a bare `{x+y}` called with `f[]` would NOT see same-named
/ globals; it would just be an unapplied 2-arg projection.)
/ --
/ Row count is the `N` global just below -- edit it directly for a quicker
/ local run (e.g. N:500000 finishes in well under a second on a laptop).
/ --
/ run:  AMBER_THREADS=8 ./amber demo/bench_showcase.k     (default: online CPU count)
/ ============================================================================

N:10000000                             / vector size for the +/* elementwise rounds

hr:{`0:"";`0:"== ",x," ",(60-#x)#"=";}

/ ---- benchmark table: collects op tags + method + ms, printed at the end --
BOP:()                                 / op tags (becomes a symbol vector on first append)
BMD:()                                 / method tags (becomes a symbol vector on first append)
BMS:()                                 / matching elapsed-ms values
bench:{[op;md;nm;f] t0:`t[]; r:f[]; ms:0.001*`t[]-t0;
  BOP,:op; BMD,:md; BMS,:ms;
  pad:(46-#nm)#".";
  msg:nm,pad," ",(-8$$round[1]ms),"ms\n";
  `0:msg;
  r}

`0:"Amber Engine Benchmark Showcase\n"
`0:"vector size: ",($N),"\n"
`0:"------------------------------------------------------------\n"

vx:N?100.0
vy:N?100.0

/ ---- 1. elementwise + : forced scalar each vs native vector kernel --------
hr"1. elementwise +  (",($N)," elements)"
rAdd1:bench[`add;`scalar;"scalar each (vx+'vy)";{vx+'vy}]
rAdd2:bench[`add;`vector;"native vector (vx+vy)";{vx+vy}]
addOK:1e-3>abs((+/rAdd1)-(+/rAdd2))
`0:"   results match: ",($addOK),"\n"

/ ---- 2. elementwise * : forced scalar each vs native vector kernel --------
hr"2. elementwise *  (",($N)," elements)"
rMul1:bench[`mul;`scalar;"scalar each (vx*'vy)";{vx*'vy}]
rMul2:bench[`mul;`vector;"native vector (vx*vy)";{vx*vy}]
mulOK:1e-3>abs((+/rMul1)-(+/rMul2))
`0:"   results match: ",($mulOK),"\n"

/ ---- 3. sum reduction : native +/ vs multithreaded peach chunk-sum --------
hr"3. sum reduction  +/  (",($N)," elements)"
K:8                                     / peach worker chunks
NPc:`i$N%K                             / chunk size, cast back to Long (N%K is a float divide)
chunks:{[i]NPc#(i*NPc)_vx}'!K            / K equal-size slices of vx

rSum1:bench[`sum;`serial;"serial +/vx";{+/vx}]
rSum2:bench[`sum;`peach;"peach chunk-sum (",($K)," workers)";{+/peach[sum;chunks]}]
sumOK:1.0>abs(rSum1-rSum2)

`0:"   serial total: ",($rSum1),"   peach total: ",($rSum2),"   match (~1.0 tol): ",($sumOK),"\n"

/ ---- summary ---------------------------------------------------------------
hr"benchmark summary"
addSp:BMS[0]%BMS[1]
mulSp:BMS[2]%BMS[3]
sumSp:BMS[4]%BMS[5]
summaryT:+`op`method`ms!(BOP;BMD;round[1]BMS)
qq:show summaryT
`0:" \n"
`0:"speedups (baseline / faster method):\n"
`0:"   +   scalar -> native  : ",($round[2]addSp),"x\n"
`0:"   *   scalar -> native  : ",($round[2]mulSp),"x\n"
`0:"   +/  serial -> peach   : ",($round[2]sumSp),"x  (workers from $AMBER_THREADS, default: online CPU count)\n"
`0:"   (a plain +/ sum is already a single fast native pass -- peach's fork/IPC\n"
`0:"    overhead only pays off for genuinely per-task-heavy work, e.g. the\n"
`0:"    Monte-Carlo pi estimate in examples/peach.k.)\n"

`0:"\nBenchmark showcase complete.\n"
