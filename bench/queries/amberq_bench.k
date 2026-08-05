/ ============================================================================
/ bench/queries/amberq_bench.k  -  Amber qSQL implementation of bench/SPEC.md.
/ GNU AGPLv3 - see LICENSE and NOTICE.
/ ----------------------------------------------------------------------------
/ Run:  ./amber bench/queries/amberq_bench.k <arith|reduce|groupby|join> [runs] [warmup]
/ ----------------------------------------------------------------------------
/ QUERY-LAYER row.  Identical data and identical answers to amber_bench.k, but
/ every workload goes through the select/by/where query layer instead of raw
/ array primitives -- so this, not the primitive row, is the fair peer of
/ DuckDB's SQL planner (SPEC.md §4.5).  Publishing only the primitive row would
/ compare Amber's hand-written array code against another engine's SQL.
/ The gap between this row and amber_bench.k IS the query layer's overhead and
/ is meant to be visible.
/ ============================================================================
DIR:{$[#p:"/"/-1_"/"\x;p;"."]}`argv 1
LOAD:{{. x^(`c$13)}'0:x;}
LOAD DIR,"/../../amber.k"
LOAD DIR,"/../../std.k"
LOAD DIR,"/../../qsql.k"

AV:`argv(::)
BID:$[2<#AV;AV 2;"reduce"]
RUNS:$[3<#AV;. AV 3;5]
WARM:$[4<#AV;. AV 4;2]

N:10000000
M:1000000
KK:1000
G:100

ii:!N
hh:1048573!262147*ii
pa:1000!hh
pb:997!hh
px:0.0+pa
py:0.0+pb
gk:G!pa
kr:1048573!7919*!KK
vr:2.0*0.0+!KK
kl:kr KK!M#hh
vl:M#px
CHK:(+/pa)+3*+/pb

/ Tables are built OUTSIDE the timed region, exactly as DuckDB's CREATE TABLE
/ is excluded from its timed SELECT (SPEC.md §4.3).
t:+`x`y`g!(px;py;gk)
tl:+`k`v!(kl;vl)
tr:xkey[`k;+`k`w!(kr;vr)]

kArith:{[q]*(sel"select s:sum (x*2.5)+y from t where x>50.0")`s}
kReduce:{[q]r:sel"select a:sum x, b:max x, c:sum x*y from t";(*r`a)+(*r`b)+*r`c}
/ !(value ...) are the group keys; weight by the key itself, never by position
kGroupby:{[q]r:sel"select s:sum x by g from t";+/(1.0+0.0+(+!r)`g)*(value r)`s}
kJoin:{[q]j:ij[tl;tr];+/j[`v]*j[`w]}
KERN:`arith`reduce`groupby`join!(kArith;kReduce;kGroupby;kJoin)

f:KERN[`$BID]
$[^f;[`0:"unknown bench: ",BID;`exit 2];]
{[j]f[0];}'!WARM;
ts:{[j]t0:`t 0;f[0];(`t 0)-t0}'!RUNS
ans:f[0]
`0:"BENCH ",BID
`0:"CHECK ",$CHK
`0:"ANSWER ",$ans
`0:"TIME_MS ",$0.001*0.0+med ts
