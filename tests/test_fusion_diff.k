/ tests/test_fusion_diff.k -- differential oracle for the 2.1 fused paths:
/ compiler idioms (+/x*y family, x@&m, a+s*b, x@<x), membership, group
/ aggregates, distinct, moving windows and the qSQL by-fast-path. Run under an
/ old and a new binary and diff: outputs must be byte-identical.
\l amber.k
\l std.k
\l qsql.k
N:20000
H:1048573!262147*!N
L1:(N?2000000)-1000000
L2:(N?2000000)-1000000
I1:1000000!H
H1:1000!H
G1:100!H
F1:0.0+L1
F2:0.0+L2
X:0.0+H1
Y:0.0+997!H
B1:X>500
B2:997!H
M0:X>50
G:100!H
NX:@[X;3 7 11;:;0n]                 / with NaNs
ZX:@[X;5;:;-0.0]                    / with a negative zero
S1:`$'"s",/:$100!H
sh:{"  ",($@x)," ",$x}
p:{`0:sh x}
pk:{`0:`k x}                          / dicts and tables: the REPL formatter
/ ---- fused reductions --------------------------------------------------------
p[+/X*Y]; p[+/L1*L2]; p[+/X*L1]; p[+/H1*H1]; p[+/F1*F2]; p[+/X*2.5]; p[+/2.5*X]; p[+/0#0.0*0#0.0]
p[+/X=Y]; p[+/X<Y]; p[+/X>Y]; p[+/X<50]; p[+/50<X]; p[+/X=50]; p[+/L1<L2]; p[+/L1<5]; p[+/5<L1]; p[+/L1=L2]
p[+/NX*Y]; p[+/NX<50]; p[+/ZX>0]; p[+/ZX<0]; p[+/ZX=0]; p[+/NX=NX]
p[+/(1 2 3;4 5 6)*(1 1 1;2 2 2)]; p[+/(1 2 3)*4 5 6]; p[+/1 2 3=1 5 3]
p[|/X*Y]; p[&/X*Y]; p[*/1 2 3*1 1 1]
p[+/X*Y+1.0]
p[+/(X*Y)*2]
/ ---- compress and where -----------------------------------------------------
p[+/X@&M0]; p[#X@&M0]; p[X@&M0]; p[L1@&M0]; p[H1@&M0]; p[S1@&M0]; p[(0.0+!5)@&1 0 1 0 1]; p[(!5)@&1 0 2 0 1]
p[+/(Y+2.5*X)@&X>50]
p[&M0]; p[&B1]; p[&1 0 1 1 0]; p[&0 0 0]; p[&100#1]; p[&2 0 1]; p[&1000#0]
p[#&X>500]; p[+/&X>500]
p[(1 2 3)@&1 1 1 1]                  / mask longer than x: null fill
p["abcde"@&1 0 1 0 1]
/ ---- fma ------------------------------------------------------------------
p[Y+2.5*X]; p[Y-2.5*X]; p[2.5*X+Y]; p[Y+X*2.5]; p[Y+2*X]; p[Y-2*X]; p[L1+2.5*L2]; p[L1+3*L2]; p[F1+2.5*F2]; p[NX+2.5*Y]
p[Y+2.5*X-1.0]
/ ---- sort by value -------------------------------------------------------------
p[X@<X]; p[X@>X]; p[L1@<L1]; p[H1@<H1]; p[G1@<G1]; p[I1@<I1]; p[F1@<F1]; p[S1@<S1]; p[NX@<NX]; p[ZX@<ZX]
p[(X@<X)@<X@<X]; p[asc X]; p[desc X]; p[`srt X]; p[`srt L1]; p[`at X@<X]; p[`at asc X]; p[`at X@>X]
p[X@<Y]                             / not the idiom
/ ---- membership ------------------------------------------------------------
KR:1048573!7919*!1000
p[+/H in KR]; p[H in KR]; p[L1 in L2]; p[H1 in 1 2 3]; p[G1 in 50 60]; p[S1 in `s1`s2]; p[5 in 1 2 5]; p[6 in 1 2 5]; p[0N in 0N 1]; p[(0#0) in 1 2]; p[1 2 in 0#0]
p[F1 in F2]; p[1.5 in 1.5 2.5]; p["abc" in "cab"]; p[`a`b in `b`c]
p[in[1 2 3;2 3]]; p[except[1 2 3 4;2 3]]; p[inter[1 2 3 4;2 3 9]]; p[union[1 2 3;3 4]]
/ ---- distinct ---------------------------------------------------------------
p[?100000!H]; p[#?100000!H]; p[?H]; p[#?H]; p[?G1]; p[?S1]; p[?F1]; p[?L1]; p[?1 2 1 2 3]; p[?0N 1 0N]
/ ---- moving windows ----------------------------------------------------------
p[msum[16;X]]; p[mavg[256;X]]; p[mmax[64;X]]; p[mmin[64;X]]; p[msum[3;NX]]; p[mmax[3;NX]]; p[mmax[64;L1]]; p[msum[4;L1]]; p[mvar[8;X]]; p[mdev[8;X]]
p[msum[16;0#0.0]]; p[mmax[1;X]]; p[msum[1;X]]; p[mmax[5;1 2 3]]
/ ---- qSQL by fast path -----------------------------------------------------------
T:([]sym:S1;px:X;sz:B2;g:G)
pk[sel"select sum px by sym from T"]
pk[sel"select sum px,avg sz by sym from T"]
pk[sel"select min px,max px,count px,first px,last px by g from T"]
pk[sel"select sum px by sym from T where sz>250"]
pk[sel"select sum px by sym,g from T"]
pk[sel"select s:sum px*sz by sym from T"]
pk[sel"select sum px by sym from T where sz>2000"]
/ ---- group aggregates ------------------------------------------------------
pk[gsum[G;X]]; pk[gsum[G;L1]]; pk[gsum[G;H1]]; pk[gavg[G;X]]; pk[gmin[G;X]]; pk[gmax[G;X]]; pk[gcount[G]]; pk[gfirst[G;X]]; pk[glast[G;X]]
pk[gsum[S1;X]]; pk[gcount[S1]]; pk[gfirst[S1;S1]]; pk[gsum[100000!H;X]]; pk[gsum[L1;X]]; pk[gsum[G;0#0.0]]
pk[(+/X@)'=G]; pk[{[v;i]+/v i}[X]'=G]
/ self-consistency of the fused group aggregates against the K idioms (each line must print 1)
gsum[G;X]~{[v;i]+/v i}[X]'=G
gsum[S1;X]~{[v;i]+/v i}[X]'=S1
gsum[G;L1]~{[v;i]+/v i}[L1]'=G
gavg[G;X]~{[v;i](+/v i)%#i}[X]'=G
gmin[G;X]~{[v;i]&/v i}[X]'=G
gmax[G;X]~{[v;i]|/v i}[X]'=G
gcount[G]~#:'=G
gfirst[G;X]~{[v;i]*v i}[X]'=G
glast[G;X]~{[v;i]*|v i}[X]'=G
gsum[100000!H;X]~{[v;i]+/v i}[X]'=100000!H
(sel"select sum px by sym from T")~qby[T;`sym;(,`px)!,{sum x`px}]
(sel"select sum px,avg sz by sym from T")~qby[T;`sym;`px`sz!({sum x`px};{avg x`sz})]
(sel"select min px,max px,count px by g from T")~qby[T;`g;`px`px`px!({min x`px};{max x`px};{count x`px})]
(sel"select sum px by sym,g from T")~qby[T;`sym`g;(,`px)!,{sum x`px}]
(sel"select sum px by sym from T where sz>250")~qby[qwhere[T;T[`sz]>250];`sym;(,`px)!,{sum x`px}]
