/ bench/primitives.k - primitive-level and small-input benchmarks: the cases bench/suite.k leaves out.
/ Run from the repo root, stdin closed (otherwise the REPL waits after the script):
/   AMBER_THREADS=1 ./amber bench/primitives.k < /dev/null             every case
/   ./amber bench/primitives.k amend.dict print < /dev/null             only these families
/   ./amber bench/primitives.k -list [selectors]                        the case ids, nothing run
/   ./amber bench/primitives.k -src [selectors]                         each case as a one-case block (below)
/ A selector picks the ids it is a whole-component prefix of: amend picks amend.*, amend.dict.sym picks
/ amend.dict.sym.*, amend.list.set.name.10 that case alone (not .10k); a trailing "." is ignored.  A
/ selector that starts with "." names a size: ./amber bench/primitives.k amend .1 .10 runs the amends of
/ 1 and 10 items (sizes and prefixes combine: any prefix, and any size; cases with no size always pass).
/ Prints one line per case:  RESULT <id> amber <median_ms> <answer>  (bench/suite.k's form), and at the end
/   CHECKED <n> cases: <m> mismatches or errors
/ A case whose answer differs from its expected value prints MISMATCH, one that fails prints ERROR (the run
/ goes on); both count in the last line.
/ .
/ What it times: amends of every kind (by name and by value; lists, dicts with symbol and int keys, tables by
/ column, row and cell, keyed tables; a key or column added; @[x;i;f;y] and .[x;i;f;y]; nested), indexing and
/ find, cats and small joins, adverbs, dict and table arithmetic, small sorts and grades, small qSQL and
/ avg/var/dev, printing (`k@, $, the display formatter), calling lambdas and projections, . of a string.
/ Sizes 1, 10, 100, 10k and 1m where they make sense: the small ones show fixed per-call costs (an extra
/ check, a lost inline, a library call), which large inputs hide.  Ids: family.kind.op[.form].size, where
/ form is name (the amend goes through a variable: L[i]:v, .[`L;..], a global) or val (on a value:
/ @[L;..], a local copy), and size is the item count (1 10 100 10k 1m); cases with no size have none.
/ .
/ Data: closed formulas, no RNG (as bench/SPEC.md).  N is the size, M:_N%2 the middle index,
/   H = 100000!2654435761*!N (ints),  K = `k0`k1.. (symbols),  T = +`a`b!(!N;10+!N),  F = 0.001*H.
/ Each case is (id stem; sizes; setup; body; check; expected), all but the sizes K source text.  Per size:
/ set N and M, run the setup, call {body} once, and compare check (an atom; R is that call's result) with
/ expected (computed by another route: a closed formula or a different primitive).  Then run the setup again
/ and time {body} with suite.k's harness: 2 warm-ups, 5 batches of n calls (n so a batch is >= ~20 ms,
/ from 1000 calls when one takes under 200 us), the median of the 5 batch means.  Below 1000 items the body is repeated 10 times in one lambda (and the
/ time divided by 10), so the harness's own per-call cost does not swamp the case.  By-name bodies change
/ a global in place each call; the timed state is the setup's, carried along (L[M]+:1 keeps adding).
/ -src prints a prelude (the \l lines and the check helpers) and then, per case,
/   /@ <id>   N:<n>;M:<m>   <setup>   out["<id>";K;{<body, repeated as timed>}]
/ (one line each), for runners that time one case per process: the runner defines out and K (out times
/ K calls of the lambda).  Case data are UPPERCASE globals other than N M R and the helpers W WS WT (the
/ harness's own names are lowercase; sel, upd, ... are qsql.k's).  Total: 541 cases, about 65 s on an
/ Apple M2 (-list counts a selection).
args:x
\l amber.k
\l std.k
\l qsql.k
COLOR:0

/ ---- harness ---------------------------------------------------------------
batch:{[f;n]t0:`t[];{[f;a;j]f 0;a}[f]/[0;!n];0.001*(`t[]-t0)%n}
med5:{(x@<x)@2}
tus:{[f;n]t0:`t[];{[f;a;j]f 0;a}[f]/[0;!n];`t[]-t0}                 / microseconds for n calls
reps:{[f]d:tus[f;1];d:$[d<200;0.001*tus[f;1000];d];1|-_-20000%d|0.01}   / calls per ~20 ms batch
same:{$[x~y;1b;~&/(@'(x;y))in`i`f;0b;~(abs x-y)>1e-9*1|abs y]}
mode:$[#args;$["-src"~*args;`src;"-list"~*args;`list;`run];`run]
sels:$[mode~`run;args;1_args]
szs:1_'sels@&"."=*'sels                                                / .10 .1m: sizes
pre:{$[(#x)&"."=last x;-1_x;x]}'sels@&~"."=*'sels                     / amend.dict: id prefixes
pfx:{[id;p]$[(#p)>#id;0b;~p~(#p)#id;0b;(#p)=#id;1b;"."=id@#p]}
pick:{[id;n]$[0=#pre;1b;|/pfx[id]'pre]&$[(0=#szs)|0=n;1b;|/(lab n)~/:szs]}
lab:{$[x<1000;$x;x<1000000;($_x%1000),"k";($_x%1000000),"m"]}
nc:0;nb:0;cid:"";tstart:`t[]
inst:{[id;n;su;bo;ck;ex]N::n;M::_n%2;. su;R::(. "{",bo,"}") 0;a:. ck;e:. ex;R::0;nc::nc+1;$[same[a;e];0;[nb::nb+1;`0:"MISMATCH ",id," got ",(`k@a)," expected ",`k@e]];. su;u:$[n<1000;10;1];f:. "{",(";"/u#,bo),"}";f 0;f 0;k:reps f;ts:{[f;k;j]batch[f;k]}[f;k]'!5;`0:"RESULT ",id," amber ",($(med5 ts)%u)," ",$a}
src:{[id;n;su;bo]u:$[n<1000;10;1];`0:("/@ ",id;"N:",($n),";M:",$_n%2;su;"out[\"",id,"\";K;{",(";"/u#,bo),"}]")}
go:{[id;n;su;bo;ck;ex]$[mode~`list;`0:id;mode~`src;src[id;n;su;bo];[cid::id;.[inst;(id;n;su;bo;ck;ex);{nb::nb+1;`0:"ERROR ",cid," ",(x?"\n")#x}]]];}
bench:{[st;sz;su;bo;ck;ex]{[st;su;bo;ck;ex;n]id:$[n;st,".",lab n;st];$[pick[id;n];go[id;n;su;bo;ck;ex];0]}[st;su;bo;ck;ex]'sz;}

/ ---- check helpers (also printed by -src) ----------------------------------
helpers:("W:{+/x*1009!!#x}";"WS:{W `i$x}";"WT:{c:+x;+/{[c;k]W c k}[c]'!c}")
{. x}'helpers;
$[mode~`src;`0:("/ bench/primitives.k cases (-src ",(" "/1_args),"): one block per case; the runner defines out and K";"\\l amber.k";"\\l std.k";"\\l qsql.k";"COLOR:0"),helpers;0];
s5:1 10 100 10000 1000000
s4:1 10 100 10000
s3:1 10 100
sq:10 100
fx:,0

/ ---- amend: lists ----------------------------------------------------------
bench["amend.list.set.name";s5;"L:!N";"L[M]:7";"W L";"W(M#!N),7,(M+1)_!N"]
bench["amend.list.plus.name";s5;"L:!N";"L[M]+:1";"W L";"W(M#!N),(M+1),(M+1)_!N"]
bench["amend.list.set.val";s5;"L:!N";"l:L;l[M]:7;l";"W R";"W(M#!N),7,(M+1)_!N"]
bench["amend.list.at.val";s5;"L:!N";"@[L;M;+;1]";"W R";"W(M#!N),(M+1),(M+1)_!N"]
bench["amend.list.atlambda.val";s4;"L:!N";"@[L;M;{x+1}]";"W R";"W(M#!N),(M+1),(M+1)_!N"]
bench["amend.list.dot.name";s5;"L:!N";".[`L;,M;:;7]";"W L";"W(M#!N),7,(M+1)_!N"]
bench["amend.list.dot.val";s5;"L:!N";".[L;,M;:;7]";"W R";"W(M#!N),7,(M+1)_!N"]
bench["amend.list.dotlambda.name";s4;"L:!N";".[`L;,M;{x+y};1]";"W L";"W(M#!N),(M+1),(M+1)_!N"]
bench["amend.list.idx.name";s4;"L:!N;I:?0,M";"L[I]+:1";"W L";"W(!N)+(!N)in I"]
bench["amend.list.all.name";s5;"L:!N";"L[]+:1";"W L";"W 1+!N"]
bench["amend.nested.set.name";s4;"LL:N#,1 2 3";"LL[M;1]:7";"+/W'LL";"5+8*N"]
bench["amend.nested.dot.name";s4;"LL:N#,1 2 3";".[`LL;(M;1);+;1]";"+/W'LL";"1+8*N"]
bench["amend.nested.dot.val";s4;"LL:N#,1 2 3";".[LL;(M;1);+;1]";"+/W'R";"1+8*N"]

/ ---- amend: dicts (symbol keys, int keys, a key added, nested) ---------------
bench["amend.dict.sym.set.name";s5;"K:`$\"k\",'$!N;D:K!!N;KM:K M";"D[KM]:7";"W value D";"W(M#!N),7,(M+1)_!N"]
bench["amend.dict.sym.plus.name";s5;"K:`$\"k\",'$!N;D:K!!N;KM:K M";"D[KM]+:1";"W value D";"W(M#!N),(M+1),(M+1)_!N"]
bench["amend.dict.sym.set.val";s5;"K:`$\"k\",'$!N;D:K!!N;KM:K M";"d:D;d[KM]:7;d";"W value R";"W(M#!N),7,(M+1)_!N"]
bench["amend.dict.sym.at.val";s5;"K:`$\"k\",'$!N;D:K!!N;KM:K M";"@[D;KM;+;1]";"W value R";"W(M#!N),(M+1),(M+1)_!N"]
bench["amend.dict.sym.dot.name";s5;"K:`$\"k\",'$!N;D:K!!N;KM:K M";".[`D;,KM;+;1]";"W value D";"W(M#!N),(M+1),(M+1)_!N"]
bench["amend.dict.sym.dot.val";s5;"K:`$\"k\",'$!N;D:K!!N;KM:K M";".[D;,KM;:;7]";"W value R";"W(M#!N),7,(M+1)_!N"]
bench["amend.dict.sym.add.name";s5;"D0:(`$\"k\",'$!N)!!N";"D::D0;D[`new]:7";"(#D)+W value D";"(N+1)+W(!N),7"]
bench["amend.dict.sym.add.val";s5;"D0:(`$\"k\",'$!N)!!N";"d:D0;d[`new]:7;d";"(#R)+W value R";"(N+1)+W(!N),7"]
bench["amend.dict.sym.addplus.name";s4;"D0:(`$\"k\",'$!N)!!N";"D::D0;D[`new]+:1";"(#D)+W value D";"(N+1)+W(!N),1"]
bench["amend.dict.int.set.name";s5;"DI:(2*!N)!!N";"DI[2*M]:7";"W value DI";"W(M#!N),7,(M+1)_!N"]
bench["amend.dict.int.plus.name";s5;"DI:(2*!N)!!N";"DI[2*M]+:1";"W value DI";"W(M#!N),(M+1),(M+1)_!N"]
bench["amend.dict.int.dot.val";s5;"DI:(2*!N)!!N";".[DI;,2*M;+;1]";"W value R";"W(M#!N),(M+1),(M+1)_!N"]
bench["amend.dict.int.add.name";s5;"DI0:(2*!N)!!N";"DI::DI0;DI[-1]:7";"(#DI)+W value DI";"(N+1)+W(!N),7"]
bench["amend.dict.nested.set.name";s4;"K:`$\"k\",'$!N;DN:`a`b!(K!!N;K!!N);KM:K M";"DN[`b;KM]:7";"W value DN`b";"W(M#!N),7,(M+1)_!N"]
bench["amend.dict.nested.dot.name";s4;"K:`$\"k\",'$!N;DN:`a`b!(K!!N;K!!N);KM:K M";".[`DN;(`b;KM);+;1]";"W value DN`b";"W(M#!N),(M+1),(M+1)_!N"]
bench["amend.dict.nested.dot.val";s4;"K:`$\"k\",'$!N;DN:`a`b!(K!!N;K!!N);KM:K M";".[DN;(`b;KM);+;1]";"W value R`b";"W(M#!N),(M+1),(M+1)_!N"]

/ ---- amend: tables (cell, column, row; a column added) and keyed tables -----
bench["amend.table.cell.colrow.name";s5;"T:+`a`b!(!N;10+!N)";"T[`a;M]:7";"WT T";"(W(M#!N),7,(M+1)_!N)+W 10+!N"]
bench["amend.table.cell.rowcol.name";s5;"T:+`a`b!(!N;10+!N)";"T[M;`a]:7";"WT T";"(W(M#!N),7,(M+1)_!N)+W 10+!N"]
bench["amend.table.cell.plus.name";s5;"T:+`a`b!(!N;10+!N)";"T[`a;M]+:1";"WT T";"(W(M#!N),(M+1),(M+1)_!N)+W 10+!N"]
bench["amend.table.cell.dot.name";s5;"T:+`a`b!(!N;10+!N)";".[`T;(M;`a);:;7]";"WT T";"(W(M#!N),7,(M+1)_!N)+W 10+!N"]
bench["amend.table.cell.dot.val";s5;"T:+`a`b!(!N;10+!N)";".[T;(M;`a);:;7]";"WT R";"(W(M#!N),7,(M+1)_!N)+W 10+!N"]
bench["amend.table.col.set.name";s5;"T:+`a`b!(!N;10+!N);V:N#7";"T[`a]:V";"WT T";"(W N#7)+W 10+!N"]
bench["amend.table.col.plus.name";s5;"T:+`a`b!(!N;10+!N)";"T[`a]+:1";"WT T";"(W 1+!N)+W 10+!N"]
bench["amend.table.col.at.val";s5;"T:+`a`b!(!N;10+!N);V:N#7";"@[T;`a;:;V]";"WT R";"(W N#7)+W 10+!N"]
bench["amend.table.col.dot.name";s5;"T:+`a`b!(!N;10+!N)";".[`T;,`a;+;1]";"WT T";"(W 1+!N)+W 10+!N"]
bench["amend.table.col.add.name";s5;"T0:+`a`b!(!N;10+!N);V:N#7";"T::T0;T[`c]:V";"(#cols[T])+WT T";"3+(W@!N)+(W 10+!N)+W N#7"]
bench["amend.table.col.add.val";s5;"T0:+`a`b!(!N;10+!N);V:N#7";"@[T0;`c;:;V]";"(#cols[R])+WT R";"3+(W@!N)+(W 10+!N)+W N#7"]
bench["amend.table.col.addloop.name";s4;"T0:+`a`b!(!N;10+!N);V:N#7;CS:`c`d`e`f";"T::T0;{T[x]:V}'CS";"(#cols[T])+WT T";"6+(W@!N)+(W 10+!N)+4*W N#7"]
bench["amend.table.cols2.plus.name";s4;"T:+`a`b!(!N;10+!N)";"T[`a`b]+:1";"WT T";"(W 1+!N)+W 11+!N"]
bench["amend.table.row.set.name";s4;"T:+`a`b!(!N;10+!N);RW:`a`b!7 8";"T[M]:RW";"WT T";"(W(M#!N),7,(M+1)_!N)+W(M#10+!N),8,(M+1)_10+!N"]
bench["amend.table.row.at.val";s4;"T:+`a`b!(!N;10+!N);RW:`a`b!7 8";"@[T;M;:;RW]";"WT R";"(W(M#!N),7,(M+1)_!N)+W(M#10+!N),8,(M+1)_10+!N"]
bench["amend.ktable.set.name";s4;"KT:xkey[`k;([]k:!N;v:10+!N)]";"KT[M;`v]:7";"W(value KT)`v";"W(M#10+!N),7,(M+1)_10+!N"]
bench["amend.ktable.row.name";s4;"KT:xkey[`k;([]k:!N;v:10+!N)]";"KT[M]:(,`v)!,7";"W(value KT)`v";"W(M#10+!N),7,(M+1)_10+!N"]
bench["amend.ktable.add.name";s4;"KT0:xkey[`k;([]k:!N;v:10+!N)]";"KT::KT0;KT[N;`v]:7";"(#KT)+W(value KT)`v";"(N+1)+W(10+!N),7"]

/ ---- index and find ---------------------------------------------------------
bench["index.list.atom";s5;"L:!N";"L M";"R";"M"]
bench["index.list.idx10";s5;"L:!N;I10:N!7919*!10";"L I10";"W R";"W I10"]
bench["index.dict.sym.atom";s5;"K:`$\"k\",'$!N;D:K!!N;KM:K M";"D KM";"R";"M"]
bench["index.dict.sym.idx10";s5;"K:`$\"k\",'$!N;D:K!!N;I10:N!7919*!10;KI:K I10";"D KI";"W R";"W I10"]
bench["index.dict.int.atom";s5;"DI:(2*!N)!!N;DM:2*M";"DI DM";"R";"M"]
bench["index.table.col";s5;"T:+`a`b!(!N;10+!N)";"T[`a]";"W R";"W@!N"]
bench["index.table.row";s5;"T:+`a`b!(!N;10+!N)";"T M";"W value R";"10+M"]
bench["index.table.cell";s5;"T:+`a`b!(!N;10+!N)";"T[M;`b]";"R";"10+M"]
bench["index.ktable.row";s4;"KT:xkey[`k;([]k:!N;v:10+!N)]";"KT M";"*value R";"10+M"]
bench["find.list.int.atom";s4;"H:100000!2654435761*!N;HM:H M";"H?HM";"R";"*&H=HM"]
bench["find.list.int.idx10";s4;"H:100000!2654435761*!N;HI:H N!7919*!10";"H?HI";"W R";"W@{*&H=x}'HI"]
bench["find.list.sym.atom";s4;"K:`$\"k\",'$!N;KM:K M";"K?KM";"R";"M"]
bench["find.in.small";s4;"H:100000!2654435761*!N;HI:H N!7919*!10";"HI in H";"+/R";"10"]
bench["find.in.big";s4;"H:100000!2654435761*!N;HI:H N!7919*!10";"H in HI";"+/R";"+/{|/x=HI}'H"]

/ ---- cats and joins ---------------------------------------------------------
bench["cat.list.atom";s5;"L:!N";"L,7";"W R";"(W@!N)+7*1009!N"]
bench["cat.list.list";s4;"L:!N";"L,L";"W R";"W(2*N)#!N"]
bench["cat.dict.disjoint";s4;"D:(`$\"k\",'$!N)!!N;E:(`$\"j\",'$!N)!!N";"D,E";"(#R)+W value R";"(2*N)+W(2*N)#!N"]
bench["cat.dict.same";s4;"D:(`$\"k\",'$!N)!!N;E:(`$\"k\",'$!N)!10+!N";"D,E";"(#R)+W value R";"N+W 10+!N"]
bench["cat.table.row.val";s4;"T:+`a`b!(!N;10+!N);RW:`a`b!7 8";"T,RW";"(#R)+WT R";"(N+1)+(W(!N),7)+W(10+!N),8"]
bench["cat.table.row.name";s4;"T:+`a`b!(!N;10+!N);RW:`a`b!7 8";"X::T;X,:RW";"(#X)+WT X";"(N+1)+(W(!N),7)+W(10+!N),8"]
bench["cat.table.table";s4;"T:+`a`b!(!N;10+!N)";"T,T";"(#R)+WT R";"(2*N)+(W(2*N)#!N)+W(2*N)#10+!N"]
bench["cat.table.insert";s4;"T:+`a`b!(!N;10+!N)";"insert[T;(7;8)]";"(#R)+WT R";"(N+1)+(W(!N),7)+W(10+!N),8"]
bench["join.lj";s3;"TL:([]k:N!7*!N;w:!N);KT:xkey[`k;([]k:!N;v:10+!N)]";"lj[TL;KT]";"(#R)+WT R";"N+(W N!7*!N)+(W@!N)+W 10+N!7*!N"]
bench["join.ij";s3;"TL:([]k:N!7*!N;w:!N);KT:xkey[`k;([]k:!N;v:10+!N)]";"ij[TL;KT]";"(#R)+WT R";"N+(W N!7*!N)+(W@!N)+W 10+N!7*!N"]
bench["join.ej";s3;"TL:([]k:N!7*!N;w:!N);TR:([]k:!N;v:10+!N)";"ej[`k;TL;TR]";"(#R)+WT R";"N+(W N!7*!N)+(W@!N)+W 10+N!7*!N"]
bench["join.uj";s3;"TL:([]k:N!7*!N;w:!N);TR:([]k:!N;v:10+!N)";"uj[TL;TR]";"(#R)+(#cols[R])+W R`k";"(2*N)+3+W(N!7*!N),!N"]

/ ---- adverbs, with a primitive and with a lambda ----------------------------
bench["adverb.each.prim";s4;"LL:N#,1 2 3";"#'LL";"+/R";"3*N"]
bench["adverb.each.lambda";s4;"L:!N";"{x+1}'L";"W R";"W 1+!N"]
bench["adverb.over.prim";s3;"H:100000!2654435761*!N";"+/H";"R";"{x+y}/H"]
bench["adverb.over.lambda";s3;"H:100000!2654435761*!N";"{x+y}/H";"R";"+/H"]
bench["adverb.scan.prim";s3;"H:100000!2654435761*!N";"+\\H";"W R";"W@{x+y}\\H"]
bench["adverb.scan.lambda";s3;"H:100000!2654435761*!N";"{x+y}\\H";"W R";"W@+\\H"]
bench["adverb.prior.prim";s3;"H:100000!2654435761*!N";"-':H";"W R";"W H-0,-1_H"]
bench["adverb.prior.lambda";s3;"H:100000!2654435761*!N";"{x-y}':H";"W 1_R";"W 1_H-0,-1_H"]
bench["adverb.eachleft.prim";s3;"H:100000!2654435761*!N;Q:1 2 3";"H+\\:Q";"+/W'R";"(3*+/H)+8*N"]
bench["adverb.eachleft.lambda";s3;"H:100000!2654435761*!N;Q:1 2 3";"H{x+y}\\:Q";"+/W'R";"(3*+/H)+8*N"]
bench["adverb.eachright.prim";s3;"H:100000!2654435761*!N;Q:1 2 3";"Q+/:H";"+/W'R";"(3*+/H)+8*N"]
bench["adverb.eachboth.prim";s3;"H:100000!2654435761*!N";"H+'H";"W R";"W 2*H"]
bench["adverb.eachboth.lambda";s3;"H:100000!2654435761*!N";"H{x+y}'H";"W R";"W 2*H"]

/ ---- dict and table arithmetic ----------------------------------------------
bench["arith.dict.plus.same";s4;"K:`$\"k\",'$!N;D:K!!N;E:K!10+!N";"D+E";"W value R";"W 10+2*!N"]
bench["arith.dict.plus.union";s4;"D:(`$\"k\",'$!N)!!N;F:(`$\"j\",'$!N)!!N";"D+F";"(#R)+W value R";"(2*N)+W(2*N)#!N"]
bench["arith.dict.minus";s4;"K:`$\"k\",'$!N;D:K!!N;E:K!10+!N";"E-D";"W value R";"W N#10"]
bench["arith.dict.times.atom";s4;"D:(`$\"k\",'$!N)!!N";"D*2";"W value R";"W 2*!N"]
bench["arith.dict.int.plus";s4;"DI:(2*!N)!!N";"DI+DI";"W value R";"W 2*!N"]
bench["arith.table.eq";s4;"T:+`a`b!(!N;10+!N);U:+`a`b!(!N;10+!N)";"T=U";"WT R";"2*W N#1"]
bench["arith.table.match";s4;"T:+`a`b!(!N;10+!N);U:+`a`b!(!N;10+!N)";"T~U";"R";"1b"]
bench["arith.table.plus";s4;"T:+`a`b!(!N;10+!N)";"T+T";"WT R";"(W 2*!N)+W 2*10+!N"]
bench["arith.ktable.plus";s3;"KT:xkey[`k;([]k:!N;v:10+!N)]";"KT+KT";"W(value R)`v";"W 2*10+!N"]

/ ---- small sorts and grades (suite.k has the 1M ones) ------------------------
bench["sort.asc.int";s4;"H:100000!2654435761*!N";"asc H";"W R";"W H@<H"]
bench["sort.asc.float";s4;"F:0.001*100000!2654435761*!N";"asc F";"W R";"W F@<F"]
bench["sort.asc.sym";s4;"K:`$\"k\",'$100!100000!2654435761*!N";"asc K";"WS@,/$R";"WS@,/$K@<K"]
bench["sort.desc.int";s4;"H:100000!2654435761*!N";"desc H";"W R";"W H@>H"]
bench["sort.grade.int";s4;"H:100000!2654435761*!N";"<H";"W H R";"W asc H"]
bench["sort.grade.desc.int";s4;"H:100000!2654435761*!N";">H";"W H R";"W desc H"]
bench["sort.grade.str";s4;"K:`$\"k\",'$100!100000!2654435761*!N;SL:$K";"<SL";"WS@,/$K R";"WS@,/$asc K"]
bench["sort.grade.mixed";s3;"MX:N#(1;`a;2.5;\"x\";3)";"<MX";"W asc R";"W@!N"]
bench["sort.asc.dict";s3;"D:(`$\"k\",'$!N)!100000!2654435761*!N";"asc D";"W value R";"W asc value D"]
bench["sort.xasc.table";s4;"TT:([]a:100000!2654435761*!N;b:!N)";"xasc[,`a;TT]";"W R`a";"W asc TT`a"]

/ ---- small qSQL (the string forms and the functional form), avg/var/dev ------
bench["qsql.select.where";sq;"H:100000!2654435761*!N;QT:([]k:`a`b`c`d@4!!N;v:H;w:0.5*H)";"sel\"select from QT where v>50000\"";"(#R)+W R`v";"(+/H>50000)+W H@&H>50000"]
bench["qsql.select.cols";sq;"H:100000!2654435761*!N;QT:([]k:`a`b`c`d@4!!N;v:H;w:0.5*H)";"sel\"select v,x:2*w from QT\"";"W R`x";"W H"]
bench["qsql.select.by";sq;"H:100000!2654435761*!N;QT:([]k:`a`b`c`d@4!!N;v:H;w:0.5*H)";"sel\"select s:sum v by k from QT\"";"+/(value R)`s";"+/H"]
bench["qsql.select.bywhere";sq;"H:100000!2654435761*!N;QT:([]k:`a`b`c`d@4!!N;v:H;w:0.5*H)";"sel\"select s:sum v by k from QT where v>50000\"";"+/(value R)`s";"+/H@&H>50000"]
bench["qsql.select.func";sq;"H:100000!2654435761*!N;QT:([]k:`a`b`c`d@4!!N;v:H;w:0.5*H)";"qwhere[QT;QT[`v]>50000]";"(#R)+W R`v";"(+/H>50000)+W H@&H>50000"]
bench["qsql.exec.where";sq;"H:100000!2654435761*!N;QT:([]k:`a`b`c`d@4!!N;v:H;w:0.5*H)";"exq\"exec v from QT where k=`a\"";"W R";"W H@&0=4!!N"]
bench["qsql.exec.by";sq;"H:100000!2654435761*!N;QT:([]k:`a`b`c`d@4!!N;v:H;w:0.5*H)";"exq\"exec sum v by k from QT\"";"+/R";"+/H"]
bench["qsql.update.where";sq;"H:100000!2654435761*!N;QT:([]k:`a`b`c`d@4!!N;v:H;w:0.5*H)";"upd\"update v:2*v from QT where k=`a\"";"W R`v";"W H*1+0=4!!N"]
bench["qsql.update.by";sq;"H:100000!2654435761*!N;QT:([]k:`a`b`c`d@4!!N;v:H;w:0.5*H)";"upd\"update m:max v by k from QT\"";"+/R`m";"+/{|/H@&x=4!!N}'4!!N"]
bench["qsql.update.new";sq;"H:100000!2654435761*!N;QT:([]k:`a`b`c`d@4!!N;v:H;w:0.5*H)";"upd\"update z:v+1 from QT\"";"W R`z";"W H+1"]
bench["qsql.delete.where";sq;"H:100000!2654435761*!N;QT:([]k:`a`b`c`d@4!!N;v:H;w:0.5*H)";"del\"delete from QT where v>50000\"";"(#R)+W R`v";"(+/~H>50000)+W H@&~H>50000"]
bench["qsql.delete.col";sq;"H:100000!2654435761*!N;QT:([]k:`a`b`c`d@4!!N;v:H;w:0.5*H)";"del\"delete w from QT\"";"(#cols[R])+W R`v";"2+W H"]
bench["agg.avg.int";s3;"AI:1+!N";"avg AI";"R";"0.5*N+1"]
bench["agg.avg.float";s3;"AF:0.5*1+!N";"avg AF";"R";"0.25*N+1"]
bench["agg.avg.mixed";s3;"AG:{$[2!x;x;0.0+x]}'1+!N";"avg AG";"R";"0.5*N+1"]
bench["agg.var.int";s3;"AI:1+!N";"var AI";"R";"((N*N)-1)%12"]
bench["agg.var.float";s3;"AF:0.5*1+!N";"var AF";"R";"((N*N)-1)%48"]
bench["agg.dev.int";s3;"AI:1+!N";"dev AI";"R";"sqrt((N*N)-1)%12"]
bench["agg.dev.float";s3;"AF:0.5*1+!N";"dev AF";"R";"sqrt((N*N)-1)%48"]

/ ---- printing: `k@ (k text), $ (each to a string), amfmt (the display formatter) ----
bench["print.k.int";s4;"H:100000!2654435761*!N";"`k@H";"WS R";"WS $[N=1;\",\",`k@*H;\" \"/{`k@x}'H]"]
bench["print.k.float";s4;"F:0.001*100000!2654435761*!N";"`k@F";"WS R";"WS $[N=1;\",\",`k@*F;\" \"/{`k@x}'F]"]
bench["print.k.str";s4;"C:N#\"abc def\"";"`k@C";"WS R";"WS $[N=1;\",\";\"\"],\"\\\"\",C,\"\\\"\""]
bench["print.k.bytes";2 10 256 10000;"B:N#0x00ff7f80";"`k@B";"WS R";"WS $[N=1;\",\";\"\"],\"0x\",,/N#(\"00\";\"ff\";\"7f\";\"80\")"]
bench["print.k.sym";s4;"K:`$\"k\",'$!N";"`k@K";"WS R";"WS $[N=1;\",\";\"\"],,/\"`\",'$K"]
bench["print.k.dict";s3;"D:(`$\"k\",'$!N)!100000!2654435761*!N";"`k@D";"WS R";"WS $[N=1;\"(\",(`k@!D),\")\";`k@!D],\"!\",`k@value D"]
bench["print.k.table";s3;"T:+`a`b!(!N;10+!N)";"`k@T";"WS R";"WS \"+\",`k@+T"]
bench["print.str.int";s4;"H:100000!2654435761*!N";"$H";"WS@,/R";"WS@,/{`k@x}'H"]
bench["print.str.float";s4;"F:0.001*100000!2654435761*!N";"$F";"WS@,/R";"WS@,/{`k@x}'F"]
bench["print.str.sym";s4;"K:`$\"k\",'$!N";"$K";"WS@,/R";"WS@,/{1_`k@x}'K"]
bench["print.show.int";s3;"H:100000!2654435761*!N";"amfmt H";"WS R";"WS `k@H"]
bench["print.show.dict";s3;"D:(`$\"k\",'$!N)!100000!2654435761*!N";"amfmt D";"#R";"$[N>20;21;N]"]
bench["print.show.table";s3;"T:+`a`b!(!N;10+!N)";"amfmt T";"#R";"$[N>20;26;5+N]"]

/ ---- calling: lambdas, a projection, . of a string -------------------------
bench["call.lambda.monad";fx;"F1:{x+1}";"F1 7";"R";"8"]
bench["call.lambda.dyad";fx;"F2:{x*y}";"F2[3;4]";"R";"12"]
bench["call.lambda.triad";fx;"F3:{x+y+z}";"F3[1;2;3]";"R";"6"]
bench["call.lambda.inline";fx;"";"{x+1}7";"R";"8"]
bench["call.proj";fx;"F2:{x*y};P:F2[3]";"P 4";"R";"12"]
bench["call.eval.expr";fx;"";". \"1+2\"";"R";"3"]
bench["call.eval.lambda";fx;"";"(. \"{x+y}\")[1;2]";"R";"3"]
bench["call.eval.assign";fx;"";". \"EV:5\"";"EV";"5"]

ms:_(`t[]-tstart)%1000
$[mode~`run;`0:"CHECKED ",($nc)," cases: ",($nb)," mismatches or errors (",($ms)," ms)";0];
\\
