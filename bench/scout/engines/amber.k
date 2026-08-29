/ bench/scout/engines/amber.k - Amber (array-primitive) side of the scout matrix.
/ Run from the repo root:  ./amber bench/scout/engines/amber.k <op> <N> <runs> <warmup>
/ Protocol and data model: bench/scout/SCOUT_SPEC.md
/ .
/ Globals are UPPERCASE on purpose: a lambda binds x/y/z as implicit arguments,
/ so a kernel written {+/x} would sum its own argument, not the data vector.
/ Dispatch is one ASSIGNMENT to F per op: a 23-way $[...] blows Amber's
/ 256-bytecode-per-expression limit, and a bare $[...] statement at top level
/ would echo the kernel lambda ahead of the protocol lines.
\l amber.k
\l std.k
\l qsql.k
DIAG:`diag 0

AV:`argv[]
OP:AV 2
NN:. AV 3
RUNS:. AV 4
WARM:. AV 5

/ ---- base data (outside the timed region) ---------------------------------
I:!NN
H:1048573!262147*I
A:1000!H
B:997!H
X:0.0+A
Y:0.0+B
CHK:(+/A)+3*+/B

/ ---- helpers --------------------------------------------------------------
/ order-statistic checksum + 1e9 * adjacent inversions (SCOUT_SPEC 2)
sfa:{[s]n:#s;((s@0)+(s@_n%4)+(s@_n%2)+(s@_(3*n)%4)+(s@n-1))+1e9*+/(1_s)<(-1)_s}
med:{[v]s:v@<v;s@_(#v)%2}
/ mat: force a RANGE-representation vector (n+!m) into a materialised one.
/ Amber keeps `n+!m` in a compact range form, and the native `aj kernel misreads
/ that form's length -- it returns a 2-element match vector, so aj[] then dies
/ with 'length.  Indexing through an explicit gather materialises it.  This runs
/ outside the timed region, so it costs the benchmark nothing.  The defect and
/ its minimal reproducer are written up in bench/SCOUT_REPORT.md.
mat:{[v]v@!#v}
/ 100 symbols, zero-padded and letter-prefixed: `s00 .. `s99.  Padding makes the
/ LEXICOGRAPHIC order xasc uses agree with the numeric order the C reference
/ sorts by, so tablesort answers are comparable across engines.
SYM:`$'{"s",(-2)#"00",x}'$!100
grp:{[g;v]d:=g;+/(1+251!!d)*(+/v@)'. d}          / group-sum, weighted by key

/ ---- fixed-size workload constants ---------------------------------------
MJ:1000000                                        / join left rows
NT:2000000                                        / table rows
MQ:200000                                         / quote rows
QP:2000                                           / quotes per symbol
MT:1000000                                        / trade rows

/ ---- per-op setup + kernel ------------------------------------------------
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
F:$[OP~"member";         [KR::1048573!7919*!1000; {+/H in KR}];F]
F:$[OP~"distinct";       {d:?A;(1e6*#d)++/d};F]
F:$[OP~"distinct_100k";  [G5::100000!H; {d:?G5;(1e6*#d)++/d}];F]
F:$[OP~"group_10";       [G::10!H;     {grp[G;X]}];F]
F:$[OP~"group_100";      [G::100!H;    {grp[G;X]}];F]
F:$[OP~"group_10k";      [G::10000!H;  {grp[G;X]}];F]
F:$[OP~"group_100k";     [G::100000!H; {grp[G;X]}];F]
F:$[OP~"join_inner";     [KR::1048573!7919*!1000; VR::2.0*!1000; HJ::1048573!262147*!MJ;
                          KL::KR@1000!HJ; VL::0.0+1000!HJ; {+/VL*VR@KR?KL}];F]
F:$[OP~"msum_16";        {+/msum[16;X]};F]
F:$[OP~"mavg_256";       {+/mavg[256;X]};F]
F:$[OP~"mmax_64";        {+/mmax[64;X]};F]
F:$[OP~"qsql_select";    [HT::1048573!262147*!NT; SY::100!HT; PX::0.0+1000!HT;
                          SZ::500!HT; {m:&SZ>250; grp[SY@m;PX@m]}];F]
F:$[OP~"tablesort";      [HT::1048573!262147*!NT; SY::SYM@100!HT; PX::0.0+1000!HT;
                          SZ::500!HT; T::([]sym:SY;px:PX;sz:SZ);
                          {tsa[`sym`px xasc T]}];F]
F:$[OP~"asof";           [QJ::!MQ; QT::mat 1+500*QP!QJ; QB::0.0+1000!QP!QJ;
                          QS::SYM@_QJ%QP; QQ::([]sym:QS;time:QT;bid:QB);
                          HTR::1048573!262147*!MT;
                          TR::([]sym:SYM@100!HTR;time:mat 1000+!MT);
                          {+/(aj[`sym`time;TR;QQ])`bid}];F]

/ Lexicographic sort checksum for `tablesort`.  One line on purpose: a lambda
/ body split across lines in a script file does not parse the way it reads.
tsa:{[r]sp:SYM?r`sym;rp:r`px;n:#rp;iv:+/((1_sp)<(-1)_sp)|((1_sp)=(-1)_sp)&(1_rp)<(-1)_rp;((rp@0)+(rp@_n%4)+(rp@_n%2)+(rp@_(3*n)%4)+(rp@n-1))+1e9*iv}

/ ---- warm-up, timed runs, median -----------------------------------------
run:{[f]ANS::0.0;$[WARM>0;{[g;j]ANS::g 0}[f]'!WARM;];TS::{[g;j]t0:`t[];ANS::g 0;0.001*`t[]-t0}[f]'!RUNS;`0:"BENCH   ",OP;`0:"CHECK   ",$CHK;`0:"ANSWER  ",$0.0+ANS;`0:"TIME_MS ",$med TS}
$[0~F;`0:"SKIP ",OP;run F]
\\
