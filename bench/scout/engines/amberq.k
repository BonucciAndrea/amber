/ bench/scout/engines/amberq.k - Amber through the qSQL QUERY LAYER.
/ Run from the repo root:  ./amber bench/scout/engines/amberq.k <op> <N> <runs> <warmup>
/ Protocol and data model: bench/scout/SCOUT_SPEC.md
/ .
/ bench/SPEC.md 4.5: reporting only the array-primitive Amber row would be
/ picking whichever comparison flatters Amber.  This row is the fair peer of
/ DuckDB SQL and of q's `select ... by ... from`; the gap between it and
/ bench/scout/engines/amber.k is the query layer's own overhead.
/ Only the ops that HAVE a query-layer formulation appear here; the rest report
/ SKIP so the runner leaves them blank rather than duplicating the array row.
\l amber.k
\l std.k
\l qsql.k
DIAG:`diag 0

AV:`argv[]
OP:AV 2
NN:. AV 3
RUNS:. AV 4
WARM:. AV 5

I:!NN
H:1048573!262147*I
A:1000!H
B:997!H
X:0.0+A
CHK:(+/A)+3*+/B:997!H
med:{[v]s:v@<v;s@_(#v)%2}
MJ:1000000
NT:2000000

/ gq: run `select s:sum x by g from GT` and weight the result the way
/ SCOUT_SPEC 2 defines the group answer.
gq:{[]r:sel"select s:sum x by g from GT"; +/(1+251!(!r)`g)*(value r)`s}

F:0
F:$[OP~"group_10";    [GT::([]g:10!H;    x:X); gq];F]
F:$[OP~"group_100";   [GT::([]g:100!H;   x:X); gq];F]
F:$[OP~"group_10k";   [GT::([]g:10000!H; x:X); gq];F]
F:$[OP~"group_100k";  [GT::([]g:100000!H;x:X); gq];F]
F:$[OP~"qsql_select"; [HT::1048573!262147*!NT;
                       T::([]sym:100!HT; px:0.0+1000!HT; sz:500!HT);
                       {r:sel"select s:sum px by sym from T where sz>250";
                        +/(1+251!(!r)`sym)*(value r)`s}];F]
F:$[OP~"join_inner";  [KR::1048573!7919*!1000; HJ::1048573!262147*!MJ;
                       RT::xkey[`k;([]k:KR; vr:2.0*!1000)];
                       LT::([]k:KR@1000!HJ; vl:0.0+1000!HJ);
                       {r:ij[LT;RT]; +/(r`vl)*r`vr}];F]

run:{[f]ANS::0.0;$[WARM>0;{[g;j]ANS::g[]}[f]'!WARM;];TS::{[g;j]t0:`t[];ANS::g[];0.001*`t[]-t0}[f]'!RUNS;`0:"BENCH   ",OP;`0:"CHECK   ",$CHK;`0:"ANSWER  ",$0.0+ANS;`0:"TIME_MS ",$med TS}
$[0~F;`0:"SKIP ",OP;run F]
\\
