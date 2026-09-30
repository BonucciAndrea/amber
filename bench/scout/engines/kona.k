/ bench/scout/engines/kona.k - Kona (open-source K3) side of the scout matrix (core tier).
/ Run:  kona bench/scout/engines/kona.k <op> <N> <runs> <warmup>
/ Protocol and data model: bench/scout/SCOUT_SPEC.md
/ .
/ K3 dialect notes (vs the ngn/k adapter this mirrors):
/  - dyadic ! is x mod y with the modulus on the RIGHT;
/  - ? is not atomic in its right argument, so "find each" is KR?/:PR, and
/    membership is find-each tested against #KR (the ngnk.k formulation).
/    The K3 system verb _lin (list-in) gives the same answer but is ~16x
/    slower in kona (1.6 s vs 0.1 s at N=1e5), so at N=1e7 it cannot finish
/    the default 7 passes inside scout's 900 s timeout;
/  - =x groups indices in first-occurrence order, the same order as ?x;
/  - :[c;t;f] is cond; functions are one-liners; globals are set at top level
/    (if[...]) because an assignment inside a function is local;
/  - the only sub-second clock is _T (gettimeofday, Julian days), so kernel
/    times are 86400000*(_T-t0) ms: wall-clock, not monotonic.  _T is read
/    through the helper now[] because a bare _T written inside a lambda body
/    is evaluated when kona parses that body at call entry, so two reads
/    around a kernel in the same lambda return (almost) the same instant;
/  - \p 0 selects full float print precision (default is 7 digits), and
/    \e 2 makes an error exit(1) instead of opening kona's debug prompt
/    (which would otherwise wait on stdin until the harness timeout).
/  - x%y on two INTEGERS is integer (floor) division in kona (7%2 is 3, where
/    K3 gives 3.5), so the block count is _((#u)+w-1)%w, exact either way.
/ mmax_64 uses the van Herk/Gil-Werman block formulation (O(n), any data/w)
/ as in growler.k, with two kona-specific spellings of the same algorithm:
/ the max-scan is {x|y}\ because kona's primitive |\ over a list of 64 float
/ vectors is ~30x slower than the lambda (215 ms vs 7 ms for 64x15625), and
/ the block results are read back with one gather from the flattened 64-row
/ scan instead of ,/+ (join-over of N/64 rows is quadratic in kona: 16 s at
/ N=1e6).  The script ends with _exit 0 rather than \\ because
/ kona treats the trailing command-line arguments as further scripts to load.
/ Globals are UPPERCASE: a function binds x/y/z as implicit arguments.

\p 0
\e 2
OP:_a 2
NN:. _a 3
RUNS:. _a 4
WARM:. _a 5

I:!NN
H:(262147*I)!1048573
A:H!1000
B:H!997
X:0.0+A
Y:0.0+B
CHK:(+/A)+3*+/B

sfa:{[s]n:#s;((s@0)+(s@_ n%4)+(s@_ n%2)+(s@_ (3*n)%4)+(s@n-1))+1e9*+/(1_ s)<(-1)_ s}
med:{[v]s:v@<v;s@_ (#v)%2}
grp:{[g;v]d:=g;+/(1+(g@*:'d)!251)*+/'v@d}
msumw:{[w;v]s:+\0.0+v;s-(w#0.0),(-w)_ s}
mavgw:{[w;v]msumw[w;v]%w&1+!#v}
mmaxw:{[w;v]n:#v;u:((w-1)#-0i),0.0+v;nb:_((#u)+w-1)%w;u:u,((nb*w)-#u)#-0i;c:+(nb,w)#u;pf:,/{x|y}\c;sf:,/|({x|y}\|c);i:!n;j:i+w-1;(sf@(nb*i!w)+i%w)|pf@(nb*j!w)+j%w}

MJ:1000000

F:0
if[OP~"sum_f";          F:{+/X}]
if[OP~"max_f";          F:{|/X}]
if[OP~"scan_f";         F:{s:+\X;(s@0)+(s@_ NN%2)+(s@NN-1)}]
if[OP~"dot";            F:{+/X*Y}]
if[OP~"sum_i";          F:{+/A}]
if[OP~"arith_mask";     F:{+/(Y+2.5*X)@&X>50}]
if[OP~"sort_f";         F:{sfa X@<X}]
if[OP~"sort_presorted"; P:X@<X; F:{sfa P@<P}]
if[OP~"grade_i";        GK:1000&NN; F:{+/GK#<A}]
if[OP~"find";           KR:(7919*!1000)!1048573; PR:KR@H!1000; F:{+/KR?/:PR}]
if[OP~"member";         KR:(7919*!1000)!1048573; F:{+/(#KR)>KR?/:H}]
if[OP~"distinct";       F:{d:?A;(1e6*#d)++/d}]
if[OP~"distinct_100k";  G5:H!100000; F:{d:?G5;(1e6*#d)++/d}]
if[OP~"group_10";       G:H!10;     F:{grp[G;X]}]
if[OP~"group_100";      G:H!100;    F:{grp[G;X]}]
if[OP~"group_10k";      G:H!10000;  F:{grp[G;X]}]
if[OP~"group_100k";     G:H!100000; F:{grp[G;X]}]
if[OP~"join_inner";     KR:(7919*!1000)!1048573; VR:2.0*!1000; HJ:(262147*!MJ)!1048573; KL:KR@HJ!1000; VL:0.0+HJ!1000; F:{+/VL*VR@KR?/:KL}]
if[OP~"msum_16";        F:{+/msumw[16;X]}]
if[OP~"mavg_256";       F:{+/mavgw[256;X]}]
if[OP~"mmax_64";        F:{+/mmaxw[64;X]}]

warm:{[f;j]f 0}
now:{[j]_T}
timed:{[f;j]t0:now[0];a:f 0;(a;86400000*now[0]-t0)}
run:{[f]w:warm[f]'!WARM;r:timed[f]'!RUNS;`0:,"BENCH   ",OP;`0:,"CHECK   ",5:CHK;`0:,"ANSWER  ",5:0.0+*r[0];`0:,"TIME_MS ",5:med[r[;1]]}
:[0~F;`0:,"SKIP ",OP;run[F]]
_exit 0
