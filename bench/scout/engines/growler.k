/ bench/scout/engines/growler.k - growler/k side of the scout matrix (core tier).
/ Run:  growler bench/scout/engines/growler.k <op> <N> <runs> <warmup>
/ Protocol and data model: bench/scout/SCOUT_SPEC.md
/ .
/ growler/k (codeberg.org/growler/k) is the maintained fork of ngn/k, so this is
/ ngnk.k with the same formulations op for op; the growler-vs-ngn/k pair isolates
/ what the fork's kernel work buys.  No msum/mmax/aj/table layer: the moving sums
/ use the prefix-sum-difference K idiom, and mmax_64 uses the van Herk/Gil-Werman
/ block formulation (reshape into rows of w, max-scan each block forwards and
/ backwards across the flipped matrix, combine), which is O(n) like the C
/ reference's monotonic deque and works for any data and any w.
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
/ van Herk/Gil-Werman moving max: pad w-1 -inf on the left (shrinking start
/ windows) and to a multiple of w on the right; c is the w x (m%w) flipped block
/ matrix, so |\c is the per-block prefix max and |(|\|c) the suffix max.
mmaxw:{[w;v]n:#v;y:((w-1)#-0w),0.0+v;m:w*-_-(#y)%w;y:y,(m-#y)#-0w;c:+(0N;w)#y;p:,/+|\c;s:,/+|(|\|c);(n#s)|n#(w-1)_p}

MJ:1000000

F:0
F:$[OP~"sum_f";          {+/X};F]
F:$[OP~"max_f";          {|/X};F]
F:$[OP~"scan_f";         {s:+\X;(s@0)+(s@_NN%2)+(s@NN-1)};F]
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
F:$[OP~"mmax_64";        {+/mmaxw[64;X]};F]

run:{[f]ANS::0.0;$[WARM>0;{[g;j]ANS::g 0}[f]'!WARM;];TS::{[g;j]t0:`t[];ANS::g 0;0.001*`t[]-t0}[f]'!RUNS;`0:"BENCH   ",OP;`0:"CHECK   ",$CHK;`0:"ANSWER  ",$0.0+ANS;`0:"TIME_MS ",$med TS}
$[0~F;`0:"SKIP ",OP;run F]
\\
