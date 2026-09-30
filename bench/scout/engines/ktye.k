/ bench/scout/engines/ktye.k - ktye/k side of the scout matrix (core tier).
/ Run:  sh bench/scout/engines/ktye_run.sh ~/opt/bin/ktye-scout bench/scout/engines/ktye.k <op> <N> <runs> <warmup>
/ Protocol and data model: bench/scout/SCOUT_SPEC.md
/ .
/ ktye/k (github.com/ktye/i, release kv.c 2026-03-28) differs from ngn/k where
/ it matters here:
/  - integers are 32-bit (no 64-bit integer type), so the generator runs in
/    float64 (exact: 262147*(N-1) < 2^53) with a one-step fix-up of the
/    floor-division remainder, h is then cast to int (< 2^20); every sum whose
/    total can exceed 2^31 is taken over floats.  sum_i (an int64 sum by
/    definition) is SKIP: +/ over 32-bit ints wraps at N=1e7;
/  - x%y on two ints is integer division, and there is no float mod;
/  - it has no clock, so ktye-scout is kv.c plus one native function now[]
/    (CLOCK_MONOTONIC ms) registered through ktye's own NATIVE extension hook;
/  - $ prints floats with 6 digits, so fmt renders a non-negative float with
/    all integer digits and 9 decimals;
/  - there is no argv access, so ktye_run.sh passes the op as -e main[...];
/  - any error inside -e is fatal, so every lambda here names its argument,
/    and a lambda is applied with f[x], never "f !n" (that parses as dyadic !).
/ mmax_64 uses the van Herk/Gil-Werman block formulation (O(n)), as in
/ growler.k.  Globals are UPPERCASE: a lambda binds x/y/z as implicit arguments.

p9:{-9#"000000000",$x}
fmt:{h:_x%1e9;r:x-1e9*h;$[r<0.;[h-:1;r+:1e9];~r<1e9;[h+:1;r-:1e9];0];l:_r;f:_0.5+1e9*r-l;$[f>999999999;[l+:1;f:0];0];$[l>999999999;[h+:1;l-:1000000000];0];s:$[h;($h),p9 l;$l];$[f;s,".",p9 f;s]}
med:{[v]s:v@<v;s@(#v)%2}
sfa:{[s]n:#s;((s@0)+(s@n%4)+(s@n%2)+(s@(3*n)%4)+(s@n-1))+1e9*0.++/(1_s)<(-1)_s}
grp:{[g;v]d:=g;+/(1.+0.+251!!d)*{[v;i]+/v@i}[v]'.d}
msumw:{[w;v]s:+\0.+v;s-(w#0.),(-w)_s}
mavgw:{[w;v]msumw[w;v]%0.+w&1+!#v}
mmaxw:{[w;v]n:#v;y:((w-1)#-0w),0.+v;m:w*((#y)+w-1)%w;y:y,(m-#y)#-0w;c:+(m%w)^y;p:,/+|\c;s:,/+|(|\|c);(n#s)|n#(w-1)_p}
modh:{[i]p:262147.*i;h:p-1048573.*_p%1048573;h:h-1048573.*~h<1048573.;h:h+1048573.*h<0.;_h}

main:{[op;nn;runs;warm]
 H::modh[!nn];A::1000!H;B::997!H;X::0.+A;Y::0.+B
 CHK::(+/0.+A)+3.*+/0.+B
 NN::nn
 F::0
 $[op~"sum_f";F::{[u]+/X};0]
 $[op~"max_f";F::{[u]|/X};0]
 $[op~"scan_f";F::{[u]s:+\X;(s@0)+(s@NN%2)+(s@NN-1)};0]
 $[op~"dot";F::{[u]+/X*Y};0]
 $[op~"arith_mask";F::{[u]+/(Y+2.5*X)@&X>50};0]
 $[op~"sort_f";F::{[u]sfa[^X]};0]
 $[op~"sort_presorted";[P::^X;F::{[u]sfa[^P]}];0]
 $[op~"grade_i";[GK::1000&NN;F::{[u]+/0.+GK#<A}];0]
 $[op~"find";[KR::_modk[!1000];PR::KR@1000!H;F::{[u]+/0.+KR?PR}];0]
 $[op~"member";[KR::_modk[!1000];F::{[u]+/(#KR)>KR?H}];0]
 $[op~"distinct";F::{[u]d:?A;(1e6*#d)+0.++/0.+d};0]
 $[op~"distinct_100k";[G5::100000!H;F::{[u]d:?G5;(1e6*#d)+0.++/0.+d}];0]
 $[op~"group_10";[G::10!H;F::{[u]grp[G;X]}];0]
 $[op~"group_100";[G::100!H;F::{[u]grp[G;X]}];0]
 $[op~"group_10k";[G::10000!H;F::{[u]grp[G;X]}];0]
 $[op~"group_100k";[G::100000!H;F::{[u]grp[G;X]}];0]
 $[op~"join_inner";[KR::_modk[!1000];VR::2.*!1000;HJ::modh[!1000000];KL::KR@1000!HJ;VL::0.+1000!HJ;F::{[u]+/VL*VR@KR?KL}];0]
 $[op~"msum_16";F::{[u]+/msumw[16;X]};0]
 $[op~"mavg_256";F::{[u]+/mavgw[256;X]};0]
 $[op~"mmax_64";F::{[u]+/mmaxw[64;X]};0]
 $[F~0;`<"SKIP ",op,"\n";bench[op;runs;warm]]}

modk:{[j]p:7919.*j;h:p-1048573.*_p%1048573;h:h-1048573.*~h<1048573.;h+1048573.*h<0.}
bench:{[op;runs;warm]
 a:0.;i:0;while[i<warm;a:F 0;i+:1]
 ts:!0;i:0;while[i<runs;t0:now 0;a:F 0;ts,:,(now 0)-t0;i+:1]
 `<"BENCH   ",op,"\n"
 `<"CHECK   ",(fmt CHK),"\n"
 `<"ANSWER  ",(fmt 0.+a),"\n"
 `<"TIME_MS ",(fmt med ts),"\n"
 0}
