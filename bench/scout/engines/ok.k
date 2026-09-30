/ bench/scout/engines/ok.k - oK side of the scout matrix (core tier).
/ Run:  node bench/scout/engines/ok_driver.js bench/scout/engines/ok.k <op> <N> <runs> <warmup>
/ Protocol and data model: bench/scout/SCOUT_SPEC.md
/ .
/ oK has no clock primitive, so ok_driver.js times the kernel: this script only
/ builds the data and binds F (0 when the op is not implemented) and CHK.
/ Formulations follow ngnk.k op for op (oK is a k6-era dialect with the same
/ ?-find returning 0N, =-group dict and right-atomic x!y mod); mmax_64 uses the
/ van Herk/Gil-Werman block formulation as in growler.k, reading the block
/ results back with one gather from the flattened 64-row scans instead of ,/+
/ (oK's join-over is quadratic in the number of pieces: 257 s at N=1e6).
/ oK's & (where) is also built by join-over, so arith_mask is slow.  oK has no [a;b]
/ blocks or global :: assignment, so per-op setup is one top-level $[] per
/ global.  All oK numbers are doubles; every value here stays below 2^53.
/ oK does not read exponent literals (1e9 lexes as 1 followed by a name e9).
/ Globals are UPPERCASE: a lambda binds x/y/z as implicit arguments.

OP:x 0
NN:. x 1

I:!NN
H:1048573!262147*I
A:1000!H
B:997!H
X:0.0+A
Y:0.0+B
CHK:(+/A)+3*+/B

sfa:{[s]n:#s;((s@0)+(s@_n%4)+(s@_n%2)+(s@_(3*n)%4)+(s@n-1))+1000000000*+/(1_s)<(-1)_s}
grp:{[g;v]d:=g;+/(1+251!!d)*{[v;i]+/v@i}[v]'. d}
memb:{[v;t]~^t?v}
msumw:{[w;v]s:+\0.0+v;s-(w#0.0),(-w)_s}
mavgw:{[w;v]msumw[w;v]%w&1+!#v}
mmaxw:{[w;v]n:#v;y:((w-1)#-0w),0.0+v;nb:-_-(#y)%w;y:y,((nb*w)-#y)#-0w;c:+(nb;w)#y;pf:,/|\c;sf:,/|(|\|c);i:!n;j:i+w-1;(sf@(nb*w!i)+_i%w)|pf@(nb*w!j)+_j%w}

MJ:1000000
KEYS:|/OP~/:("find";"member";"join_inner")
KR:$[KEYS;1048573!7919*!1000;0]
PR:$[OP~"find";KR@1000!H;0]
P:$[OP~"sort_presorted";X@<X;0]
GK:1000&NN
G5:$[OP~"distinct_100k";100000!H;0]
GC:$[OP~"group_10";10;OP~"group_100";100;OP~"group_10k";10000;OP~"group_100k";100000;0]
G:$[GC>0;GC!H;0]
VR:$[OP~"join_inner";2.0*!1000;0]
HJ:$[OP~"join_inner";1048573!262147*!MJ;0]
KL:$[OP~"join_inner";KR@1000!HJ;0]
VL:$[OP~"join_inner";0.0+1000!HJ;0]

F:0
F:$[OP~"sum_f";          {+/X};F]
F:$[OP~"max_f";          {|/X};F]
F:$[OP~"scan_f";         {s:+\X;(s@0)+(s@_NN%2)+(s@NN-1)};F]
F:$[OP~"dot";            {+/X*Y};F]
F:$[OP~"sum_i";          {+/A};F]
F:$[OP~"arith_mask";     {+/(Y+2.5*X)@&X>50};F]
F:$[OP~"sort_f";         {sfa X@<X};F]
F:$[OP~"sort_presorted"; {sfa P@<P};F]
F:$[OP~"grade_i";        {+/GK#<A};F]
F:$[OP~"find";           {+/KR?PR};F]
F:$[OP~"member";         {+/memb[H;KR]};F]
F:$[OP~"distinct";       {d:?A;(1000000*#d)++/d};F]
F:$[OP~"distinct_100k";  {d:?G5;(1000000*#d)++/d};F]
F:$[GC>0;                {grp[G;X]};F]
F:$[OP~"join_inner";     {+/VL*VR@KR?KL};F]
F:$[OP~"msum_16";        {+/msumw[16;X]};F]
F:$[OP~"mavg_256";       {+/mavgw[256;X]};F]
F:$[OP~"mmax_64";        {+/mmaxw[64;X]};F]
0
