/ bench/suite.k - Amber side of the BENCHMARKS.md section 2 suite.
/ Run from the repo root, stdin closed (otherwise the REPL waits after the script):
/   AMBER_THREADS=1 taskset -c 3 ./amber bench/suite.k < /dev/null
/ Or just ./bench/run_suite.sh, which also runs bench/suite.py and bench/render.py.
/ Prints one line per workload:  RESULT <workload> amber <median_ms> <answer>
/ .
/ Data: closed formulas, no RNG, the same in bench/suite.py (keep the two in step).
/   i   = 0..N-1          h  = (2654435761*i) mod 100000      (ints, 100k distinct)
/   px  = 0.001*h         g  = h mod G,  G in 10 1000 100000   (group keys)
/   table: sym = S<(7919*i) mod 5000>, time = +\ 1+(7919*i) mod 5, px
/   inner join: right k = (104729*j) mod 10000019, rv = 1+j (j<1000, sparse, unsorted)
/               left  k = right k at h mod 1000, lv = 1+i mod 7
/   as-of: trades sym = S<(7919*i) mod 100>, time = +\ 1+(7919*i) mod 7
/          quotes sym = S<(104729*j) mod 100>, time = +\ 1+(104729*j) mod 3,
/                 bid = (7907*j) mod 1000; times strictly increasing, so exactly one answer
/ Timing: 2 warm-ups, then 5 batches of n reps (n so a batch is >= ~20 ms);
/ the figure is the median of the 5 batch means.  Answers are computed from the
/ last warm-up result, outside the clock.
/ Globals are UPPERCASE: a lambda binds x/y/z itself, and lambdas see no locals.
\l amber.k
\l std.k

/ ---- harness ---------------------------------------------------------------
batch:{[f;n]t0:`t[];{[f;a;j]f 0;a}[f]/[0;!n];0.001*(`t[]-t0)%n}
med5:{(x@<x)@2}
reps:{[f]t0:`t[];f 0;1|-_-20000%1|`t[]-t0}
bench:{[id;f;ck]r:f 0;r:f 0;a:ck r;r:0;n:reps f;ts:{[f;n;j]batch[f;n]}[f;n]'!5;`0:"RESULT ",id," amber ",($med5 ts)," ",$a}
/ fresh-input variant: every call gets a new input from mk, built off the clock.
/ Needed where the op caches something on its input (asc marks a sorted vector `s
/ in place, so a repeated asc of the same vector is O(1) and would time nothing).
one:{[mk;f]c:mk 0;t0:`t[];f c;`t[]-t0}
batchx:{[mk;f;n]0.001*({[mk;f;a;j]a+one[mk;f]}[mk;f]/[0;!n])%n}
benchx:{[id;mk;f;ck]r:f mk 0;r:f mk 0;a:ck r;r:0;n:1|-_-20000%1|one[mk;f];ts:{[mk;f;n;j]batchx[mk;f;n]}[mk;f;n]'!5;`0:"RESULT ",id," amber ",($med5 ts)," ",$a}

/ ---- checksums (identical definitions in suite.py) --------------------------
idn:{x}
tot:{+/x}
W:{+/x*1009!!#x}                                   / position-weighted sum: checks order
grk:{(+/(1+251!!x)*. x)}                           / group dict -> sum (1+key mod 251)*groupsum
dck:{(#x)++/x}                                     / distinct: count + sum
ckT1:{p:x`px;(W p)++/p*1013!x`time}                / 1-key sort: tie-insensitive
ckT2:{p:x`px;+/(1009!!#p)*p+1013!x`time}           / unique keys: full row order
ckJ:{+/(x`lv)*x`rv}
ckA:{sum x`bid}                                    / as-of: unmatched trades are null, skipped

/ ---- base data, N = 1M ------------------------------------------------------
NN:1000000
I:!NN
H:100000!2654435761*I
PX:0.001*H
PS:0.001*asc H                                     / sorted px WITHOUT the `s attribute asc would set
G10:10!H
G1K:1000!H

/ headline-only workloads
bench["sum";{+/PX};idn]
bench["filter";{+/PX>50};idn]
bench["distinct";{?H};dck]

/ 2.1 moving windows
bench["msum100";{msum[100;PX]};tot]
bench["mavg100";{mavg[100;PX]};tot]
bench["mdev100";{mdev[100;PX]};tot]
bench["mmin100";{mmin[100;PX]};tot]
bench["mmax100";{mmax[100;PX]};tot]
bench["mavg10";{mavg[10;PX]};tot]
bench["mavg1000";{mavg[1000;PX]};tot]
bench["mmin10";{mmin[10;PX]};tot]
bench["mmin1000";{mmin[1000;PX]};tot]

/ 2.2 sort and grade
bench["sortf";{asc PX};W]
bench["sorti";{asc H};W]
bench["gradef";{iasc PX};{W PX@x}]
benchx["presorted";{PS*1.0};{asc x};W]               / fresh unmarked copy per call
PS:0

/ 2.5 group-by vs cardinality
bench["grp10";{gsum[G10;PX]};grk]
bench["grp1k";{gsum[G1K;PX]};grk]
bench["grp100k";{gsum[H;PX]};grk]
G10:0;G1K:0

/ 2.3 table sort, 1M rows x 3 cols
T:([]sym:(`$"S",'$!5000)@5000!7919*I;time:+\1+5!7919*I;px:PX)
bench["tsort_px";{xasc[,`px;T]};ckT1]
bench["tsort_st";{xasc[`sym`time;T]};ckT2]
bench["tdesc_st";{xdesc[`sym`time;T]};ckT2]
T:0

/ 2.4 inner join: keyed right table built before the clock
RJ:!1000
L:([]k:(10000019!104729*RJ)@1000!H;lv:1+7!I)
KT:xkey[`k;([]k:10000019!104729*RJ;rv:1+RJ)]
bench["ij";{ij[L;KT]};ckJ]
L:0;KT:0;I:0;H:0;PX:0

/ 2.4 as-of, 100 syms: every engine gets the same time-ordered trades and quotes
/ (tick-capture order).  aj regroups the quotes by sym inside the clock; handing it
/ a book already xasc'd by `sym`time instead is ~5x faster at 500k x 1M.
AS:`$"S",'$!100
mkt:{[n]i:!n;([]sym:AS@100!7919*i;time:+\1+7!7919*i)}
mkq:{[n]j:!n;([]sym:AS@100!104729*j;time:+\1+3!104729*j;bid:0.0+1000!7907*j)}
TR:mkt 50000
QU:mkq 100000
bench["asof50k";{aj[`sym`time;TR;QU]};ckA]
TR:mkt 500000
QU:mkq 1000000
bench["asof500k";{aj[`sym`time;TR;QU]};ckA]
TR:0;QU:0

/ 2.6 scaling, Amber only: 100k / 1M / 10M rows
scale:{[n;tag]H::100000!2654435761*!n;PX::0.001*H;G10::10!H;H::0;bench["sc_sum_",tag;{+/PX};idn];bench["sc_sort_",tag;{asc PX};W];bench["sc_mavg_",tag;{mavg[100;PX]};tot];bench["sc_grp_",tag;{gsum[G10;PX]};grk];PX::0;G10::0}
scale[100000;"100k"]
scale[1000000;"1m"]
scale[10000000;"10m"]
\\
