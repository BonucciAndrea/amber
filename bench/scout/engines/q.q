/ bench/scout/engines/q.q - kdb+/q side of the scout matrix (also runs on PeachQ).
/ Run:  q bench/scout/engines/q.q <op> <N> <runs> <warmup> -q -s 0 < /dev/null
/ Protocol and data model: bench/scout/SCOUT_SPEC.md
/ .
/ Globals are UPPERCASE on purpose: q binds x/y/z as a lambda's implicit
/ arguments, so a kernel written {sum x} would project over its own argument
/ instead of summing the data vector.  Every kernel is niladic ({[] ...}) for
/ the same reason.  Note the comment line above is "/ ." and not a bare "/":
/ a lone slash opens a multi-line comment block and silences the whole script.
\P 0                                            / full round-trip float printing

op:.z.x 0; N:"J"$.z.x 1; RUNS:"J"$.z.x 2; WARM:"J"$.z.x 3;

/ ---- base data (outside the timed region) ---------------------------------
I:til N;
H:(262147*I) mod 1048573;
A:H mod 1000;
B:H mod 997;
X:"f"$A;
Y:"f"$B;
CHK:(sum A)+3*sum B;

/ ---- helpers --------------------------------------------------------------
sfa:{[s] n:count s;
  ((s 0)+(s n div 4)+(s n div 2)+(s (3*n) div 4)+s n-1)+1e9*sum (1_s)<-1_s};
grp:{[g;v] d:group g; sum (1+(key d) mod 251)*"f"$sum each v value d};
pad:{-2#"00",x};
SYM:`$("s",/:pad each string til 100);         / `s00 .. `s99, lexicographic = numeric

MJ:1000000; NT:2000000; MQ:200000; QP:2000; MT:1000000;

/ ---- per-op setup + kernel ------------------------------------------------
F:0N;
if[op~"sum_f";           F:{[]sum X}];
if[op~"max_f";           F:{[]max X}];
if[op~"dot";             F:{[]sum X*Y}];
if[op~"sum_i";           F:{[]"f"$sum A}];
if[op~"arith_mask";      F:{[]sum (Y+2.5*X) where X>50}];
if[op~"sort_f";          F:{[]sfa asc X}];
if[op~"sort_presorted";  P::asc X; F:{[]sfa asc P}];
if[op~"grade_i";         GK::1000&N; F:{[]"f"$sum GK#iasc A}];
if[op~"find";            KR::(7919*til 1000) mod 1048573; PR::KR H mod 1000;
                         F:{[]"f"$sum KR?PR}];
if[op~"member";          KR::(7919*til 1000) mod 1048573; F:{[]"f"$sum H in KR}];
if[op~"distinct";        F:{[]d:distinct A; (1e6*count d)+sum d}];
if[op~"distinct_100k";   G5::H mod 100000; F:{[]d:distinct G5; (1e6*count d)+sum d}];
if[op~"group_10";        G::H mod 10;     F:{[]grp[G;X]}];
if[op~"group_100";       G::H mod 100;    F:{[]grp[G;X]}];
if[op~"group_10k";       G::H mod 10000;  F:{[]grp[G;X]}];
if[op~"group_100k";      G::H mod 100000; F:{[]grp[G;X]}];
if[op~"join_inner";      KR::(7919*til 1000) mod 1048573; VR::2.0*til 1000;
                         HJ::(262147*til MJ) mod 1048573;
                         KL::KR HJ mod 1000; VL::"f"$HJ mod 1000;
                         F:{[]sum VL*VR KR?KL}];
if[op~"msum_16";         F:{[]sum 16 msum X}];
if[op~"mavg_256";        F:{[]sum 256 mavg X}];
if[op~"mmax_64";         F:{[]sum 64 mmax X}];
if[op~"qsql_select";     HT::(262147*til NT) mod 1048573;
                         T::([]sym:SYM HT mod 100; px:"f"$HT mod 1000; sz:HT mod 500);
                         F:{[]r:select s:sum px by sym from T where sz>250;
                            sum (1+(SYM?(key r)`sym) mod 251)*(value r)`s}];
if[op~"tablesort";       HT::(262147*til NT) mod 1048573;
                         T::([]sym:SYM HT mod 100; px:"f"$HT mod 1000; sz:HT mod 500);
                         F:{[]r:`sym`px xasc T; sp:SYM?r`sym; rp:r`px; n:count rp;
                            ((rp 0)+(rp n div 4)+(rp n div 2)+(rp (3*n) div 4)+rp n-1)
                            +1e9*sum ((1_sp)<-1_sp) or ((1_sp)=-1_sp) and (1_rp)<-1_rp}];
if[op~"asof";            qj::til MQ;
                         / quotes are generated already sorted by (sym,time); the
                         / `p# attribute on sym is the documented, idiomatic kdb+
                         / setup for aj and is applied outside the timed region.
                         / Without it this op takes ~47 s instead of ~90 ms.
                         Q0::([]sym:SYM qj div QP;
                              time:1+500*qj mod QP; bid:"f"$(qj mod QP) mod 1000);
                         / PeachQ has no `p# attribute yet, so ask for it and fall
                         / back to the plain (already sorted) table if it is refused.
                         QUOTE::@[{update `p#sym from x};Q0;{[e;t]t}[;Q0]];
                         HTR::(262147*til MT) mod 1048573;
                         TRADE::([]sym:SYM HTR mod 100; time:1000+til MT);
                         F:{[]sum (aj[`sym`time;TRADE;QUOTE])`bid}];

/ ---- warm-up, timed runs, median -----------------------------------------
if[null F; -1 "SKIP ",op; exit 0];
ANS:0f;
do[WARM; ANS:F[]];
ts:{[f;j] t0:.z.p; ANS::f[]; 1e-6*"f"$.z.p-t0}[F] each til RUNS;
-1 "BENCH   ",op;
-1 "CHECK   ",string CHK;
-1 "ANSWER  ",string "f"$ANS;
-1 "TIME_MS ",string asc[ts] RUNS div 2;
exit 0;
