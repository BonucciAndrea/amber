/ bench/scout/engines/ngnk.k - ngn/k side of the scout matrix (core tier).
/ Run:  ngnk bench/scout/engines/ngnk.k <op> <N> <runs> <warmup>
/ Protocol and data model: bench/scout/SCOUT_SPEC.md
/ .
/ ngn/k is the interpreter Amber's core is derived from, so this is the closest
/ possible "same language, no Amber kernels" control: identical syntax, but no
/ msum/mmax/aj/table layer.  The moving-window ops are written with the standard
/ prefix-sum-difference K idiom (which is what Amber's own msumK fallback does);
/ mmax_64 has no O(n) K formulation and is reported n/a rather than measured
/ against the native monotonic-deque kernels with a different complexity class.
/ Globals are UPPERCASE: a lambda binds x/y/z as implicit arguments.

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
Y:0.0+B
CHK:(+/A)+3*+/B

sfa:{[s]n:#s;((s@0)+(s@_n%4)+(s@_n%2)+(s@_(3*n)%4)+(s@n-1))+1e9*+/(1_s)<(-1)_s}
med:{[v]s:v@<v;s@_(#v)%2}
grp:{[g;v]d:=g;+/(1+251!!d)*(+/v@)'. d}
memb:{[v;t]~^t?v}                                 / no `in` verb; ? gives 0N when absent
msumw:{[w;v]s:+\0.0+v;s-(w#0.0),(-w)_s}           / prefix-difference moving sum
mavgw:{[w;v]msumw[w;v]%w&1+!#v}

MJ:1000000

F:0
F:$[OP~"sum_f";          {+/X};F]
F:$[OP~"max_f";          {|/X};F]
F:$[OP~"dot";            {+/X*Y};F]
F:$[OP~"sum_i";          {+/A};F]
F:$[OP~"arith_mask";     {+/(Y+2.5*X)@&X>50};F]
F:$[OP~"sort_f";         {sfa X@<X};F]
F:$[OP~"sort_presorted"; [P::X@<X; {sfa P@<P}];F]
F:$[OP~"grade_i";        [GK::1000&NN; {+/GK#<A}];F]
F:$[OP~"find";           [KR::1048573!7919*!1000; PR::KR@1000!H; {+/KR?PR}];F]
F:$[OP~"member";         [KR::1048573!7919*!1000; {+/memb[H;KR]}];F]
F:$[OP~"distinct";       {d:?A;(1e6*#d)++/d};F]
F:$[OP~"distinct_100k";  [G5::100000!H; {d:?G5;(1e6*#d)++/d}];F]
F:$[OP~"group_10";       [G::10!H;     {grp[G;X]}];F]
F:$[OP~"group_100";      [G::100!H;    {grp[G;X]}];F]
F:$[OP~"group_10k";      [G::10000!H;  {grp[G;X]}];F]
F:$[OP~"group_100k";     [G::100000!H; {grp[G;X]}];F]
F:$[OP~"join_inner";     [KR::1048573!7919*!1000; VR::2.0*!1000; HJ::1048573!262147*!MJ;
                          KL::KR@1000!HJ; VL::0.0+1000!HJ; {+/VL*VR@KR?KL}];F]
F:$[OP~"msum_16";        {+/msumw[16;X]};F]
F:$[OP~"mavg_256";       {+/mavgw[256;X]};F]

run:{[f]ANS::0.0;$[WARM>0;{[g;j]ANS::g 0}[f]'!WARM;];TS::{[g;j]t0:`t[];ANS::g 0;0.001*`t[]-t0}[f]'!RUNS;`0:"BENCH   ",OP;`0:"CHECK   ",$CHK;`0:"ANSWER  ",$0.0+ANS;`0:"TIME_MS ",$med TS}
$[0~F;`0:"SKIP ",OP;run F]
\\
