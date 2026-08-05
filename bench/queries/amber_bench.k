/ ============================================================================
/ bench/queries/amber_bench.k  -  Amber implementation of bench/SPEC.md.
/ GNU AGPLv3 - see LICENSE and NOTICE.
/ ----------------------------------------------------------------------------
/ Run:  ./amber bench/queries/amber_bench.k <arith|reduce|groupby|join> [runs] [warmup]
/ Self-locating: derives the repo root from `argv 1, so it runs from any cwd.
/ ----------------------------------------------------------------------------
/ ARRAY-PRIMITIVE row.  This is Amber's fair peer to ngn/k, CBQN, J and Uiua:
/ idiomatic array code, no query layer.  bench/queries/amberq_bench.k is the
/ separate qSQL row, the fair peer of DuckDB SQL (SPEC.md §4.5).
/ ----------------------------------------------------------------------------
/ NOT USED, DELIBERATELY: the previous suite's `+/!10000000`.  Amber constant-
/ folds a sum over a RANGE into the closed form n(n-1)/2 (src/3.c, arf), so
/ that benchmark measured an O(1) formula against everyone else's O(n) loop.
/ Every vector below is materialised before the clock starts.
/ ============================================================================
DIR:{$[#p:"/"/-1_"/"\x;p;"."]}`argv 1
LOAD:{{. x^(`c$13)}'0:x;}
LOAD DIR,"/../../amber.k"

/ `argv with an index wraps around modularly, so the element count has to come
/ from the whole list: `argv(::) returns it (binary; script; user args...).
AV:`argv(::)
BID:$[2<#AV;AV 2;"reduce"]
RUNS:$[3<#AV;. AV 3;5]
WARM:$[4<#AV;. AV 4;2]

N:10000000
M:1000000
KK:1000
G:100

/ ---- data (SPEC.md §1); every intermediate stays under 2^53 ---------------
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

/ ---- kernels: each returns the scalar answer ------------------------------
kArith:{[q]+/((px*2.5)+py)@&px>50.0}
kReduce:{[q](+/px)+(|/px)++/px*py}
/ NOTE: !(=v) returns the distinct keys in FIRST-APPEARANCE order, not sorted,
/ so the weights must be taken from the group keys themselves. Weighting by a
/ positional 1..G vector silently pairs each group sum with the wrong weight --
/ caught by the cross-engine ANSWER check against the C and NumPy references.
kGroupby:{[q]d:=gk;+/(1.0+0.0+!d)*{[s]+/px s}'. d}
/ Real key lookup: kr is sparse and unsorted, so `?` performs a genuine
/ search per left key -- it cannot degrade into an array index (SPEC.md §2).
kJoin:{[q]+/vl*vr kr?kl}
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
