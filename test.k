\l amber.k
/ ============================================================================
/ Amber test suite. run:  ./amber test.k
/ ============================================================================
r:()
t:{r,:,($x;$[y~z;"ok";"FAIL got<",($y),"> exp<",($z),">"])}

/ ---- scalars / aggregations ----
t[`sum;sum 1 2 3 4;10]
t[`sumEmpty;sum 0#0;0]
t[`prd;prd 1 2 3 4;24]
t[`avg;avg 1 2 3 4;2.5]
t[`min;min 3 1 2;1]
t[`max;max 3 1 2;3]
t[`med;med 1 2 3 4 5;3.0]
t[`medEven;med 1 2 3 4;2.5]
t[`dev;dev 2 4 4 4 5 5 7 9;2.0]
t[`svar;svar 2 4 4 4 5 5 7 9;4.571428571428571]
t[`wsum;wsum[1 2 3;10 20 30];140]
t[`wavg;wavg[10 20 30;1 2 3];7%3.0]
t[`cor;0.0001>abs 1.0-cor[1 2 3;2 4 6];1]
t[`cov;0.0001>abs cov[1 2 3;1 2 3]-2%3.0;1]
t[`signum;signum -3 0 5;-1 0 1]
t[`abs;abs -3 4 -5;3 4 5]
t[`floor;floor 2.7 -2.3;2 -3]
t[`ceiling;ceiling 2.1 -2.9;3 -2]
t[`sqrt;sqrt 9 16;3 4.0]
t[`mod;mod[7;3];1]
t[`div;div[7;3];2]
t[`xbar;xbar[10;0 5 12 23];0 0 10 20]

/ ---- uniform ----
t[`sums;sums 1 2 3;1 3 6]
t[`prds;prds 1 2 3 4;1 2 6 24]
t[`deltas;deltas 1 3 6 10;1 2 3 4]
t[`differ;differ 1 1 2 2 3;10101b]
t[`prev;prev 1 2 3;0N 1 2]
t[`next;next 1 2 3;2 3 0N]
t[`reverse;reverse 1 2 3;3 2 1]
t[`rotate;rotate[2;1 2 3 4 5];3 4 5 1 2]

/ ---- ordering / set ----
t[`asc;asc 3 1 2;1 2 3]
t[`ascAttr;`at asc 3 1 2;`s]
t[`desc;desc 1 3 2;3 2 1]
t[`iasc;iasc 30 10 20;1 2 0]
t[`rank;rank 30 10 20;2 0 1]
t[`distinct;distinct 1 1 2 3 3;1 2 3]
t[`inMem;in[3 5;1 2 3 4];10b]
t[`except;except[1 2 3 4;2 4];1 3]
t[`inter;inter[1 2 3 4;2 4 6];2 4]
t[`union;union[1 2 3;3 4 5];1 2 3 4 5]
t[`sublistPos;sublist[3;1 2 3 4 5];1 2 3]
t[`sublistCap;sublist[9;1 2 3];1 2 3]
t[`sublistNeg;sublist[-2;1 2 3 4 5];4 5]
t[`crossN;cross[1 2;10 20];((1;10);(1;20);(2;10);(2;20))]
t[`razeN;raze(1 2;3 4;5);1 2 3 4 5]

/ ---- attributes: sorted-attr find must equal linear find, and be a speedup ----
base:{x@<x}200000?1000000
q:base@1000?#base
bAttr:`sa base
lin:base?q
bin:bAttr?q
t[`attrCorrect;bin;lin]
t[`attrIsSet;`at bAttr;`s]
t0:`t[];rr0:bAttr?q;t1:`t[]
t2:`t[];rr1:base?q;t3:`t[]
t[`attrSpeedup;(t3-t2)>2*1|t1-t0;1]

/ ---- tables / keyed tables ----
tt:+`s`p`v!(`a`b`c;10 20 30;1.5 2.5 3.5)
t[`istable;istable tt;1]
t[`cols;cols tt;`s`p`v]
t[`rowCount;#tt;3]
t[`colAccess;tt`p;10 20 30]
kt:xkey[`s;tt]
t[`iskeyed;iskeyed kt;1]
t[`keys;keys kt;,`s]
t[`keyedCols;cols kt;`s`p`v]
t[`unkey;unkey kt;tt]
t[`xcols;cols xcols[`v;tt];`v`s`p]
t[`meta;(!meta tt)`c;`s`p`v]

/ ---- joins ----
lt:+`s`p!(`a`b`c`a;10 20 30 40)
kv:xkey[`s;+`s`x!(`a`b;100 200)]
t[`lj;(+lj[lt;kv])`x;100 200 0N 100]
t[`ijRows;#ij[lt;kv];3]
t[`uj;(+uj[+`a`b!(1 2;3 4);+`b`c!(5 6;7 8)])`c;0N 0N 7 8]
t[`pj;(+pj[+`s`p!(`a`b`c;1 1 1);xkey[`s;+`s`p!(`a`b;10 20)]])`p;11 21 1]
t[`ej;#ej[`s;+`s`p!(`a`b`a;1 2 3);+`s`x!(`a`b`d;10 20 30)];3]

/ ---- asof / window joins ----
trade:+`sym`time`px!(`a`b`a;3 4 9;100 200 300)
quote:+`sym`time`bid!(`a`a`b`a;1 5 2 8;10 11 20 12)
t[`aj;(+aj[`sym`time;trade;quote])`bid;10 20 12]
t[`aj0time;(+aj0[`sym`time;trade;quote])`time;1 2 8]
t[`wjMax;(+wj[(trade[`time]-2;trade`time);`sym`time;trade;quote;,(`mx;max;`bid)])`mx;10 20 12]
t[`asof;asof[xasc[`sym`time;trade];`sym`time!(`a;6)];(,`px)!,100]

/ ---- qSQL-style ----
st:+`s`v!(`a`b`a`b`a;1 2 3 4 5)
t[`qwhere;(+qwhere[st;st[`v]>2])`v;3 4 5]
t[`qbyTot;(value qby[st;`s;(,`tot)!,{sum x`v}])`tot;9 6]
t[`fby;fby[(sum;10 20 30 40 50);`a`b`a`b`a];90 60 90 60 90]
t[`xgroupUngroup;ungroup xgroup[`s;st];+`s`v!(`a`a`a`b`b;1 3 5 2 4)]

/ ---- strings ----
t[`lower;lower"HeLLo";"hello"]
t[`upper;upper"HeLLo";"HELLO"]
t[`ltrim;ltrim"  ab ";"ab "]
t[`rtrim;rtrim"  ab ";"  ab"]
t[`trim;trim"  ab ";"ab"]
t[`ss;ss["abracadabra";"ra"];2 9]
t[`ssr;ssr["abracadabra";"ra";"XX"];"abXXcadabXX"]
t[`svJoin;sv["/";("a";"bc";"d")];"a/bc/d"]
t[`svDecode;sv[10;1 2 3];123]
t[`likeStar;like["abcde";"a*e"];1]
t[`likeQ;like["abcde";"a?e"];0]
t[`likeMid;like["abcde";"*cd*"];1]
t[`likeVec;like[("cat";"dog";"cab");"c*"];101b]

/ ---- table literals  ([]col:..;..) and ([key:..]col:..) ----
t[`tlitCols;cols([]sym:`a`b;px:1 2);`sym`px]
t[`tlitType;@([]sym:`a`b;px:1 2);`M]
t[`tlitData;(([]sym:`a`b;px:1 2))`px;1 2]
t[`tlitExpr;(([]x:1+!3;y:{x*x}1 2 3))`y;1 4 9]
t[`tlitKeyed;iskeyed([sym:`a`b]px:1 2);1]
t[`tlitKeyCols;keys([sym:`a`b]px:1 2);,`sym]
t[`tlitRows;#([]a:1 2 3 4);4]

/ ---- temporal ----
t[`hms;hms[9;30;15];34215000]
t[`hh;hh hms[9;30;15];9]
t[`mm;mm hms[9;30;15];30]
t[`sec;sec hms[9;30;15];15]
t[`minute;minute hms[9;30;15];570]
t[`second;second hms[9;30;15];34215]
t[`milli;milli hms[9;30;15]+123;123]
t[`stime;stime hms[9;30;15];"09:30:15.000"]
t[`ptime;ptime"09:30:15.123";34215123]
t[`ptimeRound;(ptime stime hms[13;45;9])=hms[13;45;9];1]
t[`minbar;minbar[5;hms[9;37;0]];hms[9;35;0]]
t[`minbarVec;minute minbar[1;hms[9;30;0]+60000*0 1 2];570 571 572]
t[`bar5m;bar[5;60000;hms[9;37;30]];hms[9;35;0]]

/ ---- coverage: remaining scalar / monadic / conversion ----
t[`neg;neg 1 2 -3;-1 -2 3]
t[`notf;not 0 1 0;101b]
t[`nullf;null 0N 1;10b]
t[`recip;reciprocal 2 4;0.5 0.25]
t[`expf;0.001>abs 2.718281828-exp 1;1]
t[`logf;log 1;0.0]
t[`sinf;sin 0;0.0]
t[`cosf;cos 0;1.0]
t[`til;til 5;0 1 2 3 4]
t[`enlist;#enlist 5;1]
t[`string;string 42;"42"]
t[`typef;type 1 2 3;`I]
t[`keyf;key`a`b!1 2;`a`b]
t[`valuef;value`a`b!1 2;1 2]
t[`first;first 10 20 30;10]
t[`last;last 10 20 30;30]
t[`group;value group`a`b`a;(0 2;,1)]
t[`where;where 0 1 0 1 1;1 3 4]
t[`flipf;flip(1 2;3 4);(1 3;2 4)]
t[`count;count 1 2 3 4;4]
t[`xlog;0.001>abs 3.0-xlog[2;8];1]
t[`round;round[2;3.14159];3.14]

/ ---- coverage: remaining aggregation / uniform / order ----
t[`var;var 1 2 3 4 5;2.0]
t[`sdev;0.001>abs 2.13809-sdev 2 4 4 4 5 5 7 9;1]
t[`scov;0.001>abs 2.0-scov[1 2 3;2 4 6];1]
t[`allT;all 1 1 1;1b]
t[`allF;all 1 0 1;0b]
t[`anyT;any 0 0 1;1b]
t[`anyF;any 0 0 0;0b]
t[`mins;mins 3 1 4 1 5;3 1 1 1 1]
t[`maxs;maxs 3 1 4 1 5;3 3 4 4 5]
t[`ratios;ratios 1 2 4 8;1 2 2 2.0]
t[`idesc;idesc 30 10 20;0 2 1]
t[`xrank;xrank[2;10 20 30 40];0 0 1 1]
t[`xprev;xprev[2;1 2 3 4 5];0N 0N 1 2 3]

/ ---- coverage: remaining table / select / temporal ----
t[`isdict;isdict`a`b!1 2;1]
t[`rowsf;rows([]a:1 2;b:3 4);(1 3;2 4)]
t[`atr;(atr[([]a:1 2 3;b:4 5 6);0 2])`a;1 3]
t[`xcolall;cols xcolall[`x`y;([]a:1 2;b:3 4)];`x`y]
t[`xdesc;(xdesc[`p;([]s:`a`b`c;p:1 3 2)])`p;3 2 1]
t[`qselect;(qselect[([]a:1 2 3);(,`s)!,{sum x`a}])`s;,6]
t[`tsym;(tsym[([]tc:hms[9;30;0]);`tc])`tc;,"09:30:00.000"]
t[`sv;sv["/";("a";"bc";"d")];"a/bc/d"]
t[`vs;#vs["/";"a/bc/d"];3]

/ ---- report ----
fails:r@&{"FAIL"~4#x}'r[;1]
`0:""
`0:"================ AMBER TEST SUITE ================"
`0:(($#r)," tests run, ",($#fails)," failures")
$[#fails;{`0:"  ",(x 0),": ",x 1}'fails;`0:"ALL TESTS PASSED"];
`0:"================================================="
