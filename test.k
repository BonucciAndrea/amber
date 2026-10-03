\l amber.k
/ ============================================================================
/ Amber test suite. run:  ./amber test.k
/ ============================================================================
r:()
t:{r,:,($x;$[y~z;"ok";"FAIL got<",($y),"> exp<",($z),">"])}
/ xasc orders a char column as < does: unsigned bytes (issue #17)
t[`xascChars;(+xasc[`c;+`c`v!(0x806100;1 2 3)])`v;3 2 1]

/ amber.k's min and mins of chars give chars (issue #17: & and | of chars give a char)
t[`minChars;(min "ab";mins "bac";max "ab";maxs "abc");("a";"baa";"b";"abc")]

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
t[`inCharInt;*"\n"\.[in;(97;"abc");{x}];"'type"]   / a char and a number are 'type, as in q (issue #20, row 12)
t[`except;except[1 2 3 4;2 4];1 3]
t[`inter;inter[1 2 3 4;2 4 6];2 4]
t[`union;union[1 2 3;3 4 5];1 2 3 4 5]
t[`sublistPos;sublist[3;1 2 3 4 5];1 2 3]
t[`sublistCap;sublist[9;1 2 3];1 2 3]
t[`sublistNeg;sublist[-2;1 2 3 4 5];4 5]
t[`crossN;cross[1 2;10 20];((1;10);(1;20);(2;10);(2;20))]
t[`razeN;raze(1 2;3 4;5);1 2 3 4 5]

/ ---- attributes: sorted-attr find must equal linear find, and be a speedup ----
base:0+{x@<x}200000?1000000          / 0+ strips the `s attribute a value sort now carries (2.1)
q:base@1000?#base
bAttr:`sa base
lin:base?q
bin:bAttr?q
t[`attrCorrect;bin;lin]
t[`attrIsSet;`at bAttr;`s]
t0:`t[];rr0:bAttr?q;t1:`t[]
t2:`t[];rr1:base?q;t3:`t[]
t[`attrSpeedup;(t3-t2)>2*1|t1-t0;1]
/ the four q-style attributes
t[`attrSorted;`at(`sa 1 2 3);`s]
t[`attrUnique;`at(`ua 3 1 2);`u]
t[`attrParted;`at(`pa 1 1 2 2);`p]
t[`attrGrouped;`at(`ga 3 1 2);`g]
t[`partedFindC;(`pa 10 10 20 20 30)?10 20 30 40;0 2 4 0N]

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
t[`ljKeepsOwn;+lj[+`sym`n`px!(`a`b;1 2;0.5 1.0);xkey[`sym;+`sym`n`px!(`a`c;10 30;5.0 7.0)]];`sym`n`px!(`a`b;10 2;5.0 1.0)]
/ a column the table has, of another type in the keyed table, is 'type, as in q (issue #20, row 21), matched or not
t[`ljOtherType;.[lj;(+`k`a!(1 2 3;10 20 30);xkey[`k;+`k`a!(1 3;100 300.0)]);{`err}];`err]
t[`ljOtherTypeNoMatch;.[lj;(+`k`a!(1 2 3;10 20 30);xkey[`k;+`k`a!(5 6;100 300.0)]);{`err}];`err]
t[`ljGenericRight;.[lj;(+`k`a!(1 2 3;10 20 30);xkey[`k;+`k`a!(1 3;(100;`x))]);{`err}];`err]
t[`ljGenericLeft;(+lj[+`k`a!(1 2 3;(10;`y;30));xkey[`k;+`k`a!(1 3;100 300)]])`a;(100;`y;300)]
t[`ljNewColumn;+lj[+`k`a!(1 2 3;10 20 30);xkey[`k;+`k`b!(1 3;100 300.0)]];`k`a`b!(1 2 3;10 20 30;100.0 0n 300.0)]
t[`ljEmptyGeneric;+lj[+`k`a!(1 2 3;10 20 30);xkey[`k;+`k`a!(0#0;())]];`k`a!(1 2 3;10 20 30)]   / an empty generic column matches nothing and is no type clash, as in q
t[`ljAllMatch;(+lj[+`k`a!(1 2;10 20);xkey[`k;+`k`a!(1 2;5 6)]])`a;5 6]
t[`ljAllMatchGeneric;(+lj[+`k`a!(1 2;(10;`y));xkey[`k;+`k`a!(1 2;5 6)]])`a;(5;6)]
t[`ijRows;#ij[lt;kv];3]
t[`uj;(+uj[+`a`b!(1 2;3 4);+`b`c!(5 6;7 8)])`c;0N 0N 7 8]
t[`pj;(+pj[+`s`p!(`a`b`c;1 1 1);xkey[`s;+`s`p!(`a`b;10 20)]])`p;11 21 1]
t[`pjNewCol;(+pj[+`k`a!(1 2 3;10 20 30);xkey[`k;+`k`b!(1 3;100 300)]])`b;100 0 300]
t[`pjNulls;((+pj[+`k`a!(1 2 3;0N 20 30);xkey[`k;+`k`a!(1 3;1 2)]])`a;(+pj[+`k`a!(1 2 3;10 20 30);xkey[`k;+`k`a!(1 3;0N 2)]])`a);(0N 20 32;10 20 32)]
t[`pjNewColFloat;(+pj[+`k`a!(1 2 3;10 20 30);xkey[`k;+`k`b!(1 3;1.5 2.5)]])`b;1.5 0.0 2.5]
t[`ej;#ej[`s;+`s`p!(`a`b`a;1 2 3);+`s`x!(`a`b`d;10 20 30)];3]
t[`ejAllMatches;ej[`s;+`s`n!(`a`b;1 2);+`s`v!(`a`a`b;10 11 20)];+`s`n`v!(`a`a`b;1 1 2;10 11 20)]
t[`ejNone;ej[`s;+`s`n!(`a`b;1 2);+`s`v!(,`c;,9)];+`s`n`v!(0#`;!0;!0)]
t[`ejTwoKeys;ej[`s`k;+`s`k`n!(`a`a;1 2;5 6);+`s`k`v!(`a`a`a;2 2 1;7 8 9)];+`s`k`n`v!(`a`a`a;1 2 2;5 6 6;9 7 8)]
t[`ejUnique;(ej[`s;+`s`n!(`a`b`c`a;1 2 3 4);+`s`v!(`a`c;10 30)];ej[`s`k;+`s`k`n!(`a`a`b;1 2 1;5 6 7);+`s`k`v!(`a`b;1 1;8 9)]);(+`s`n`v!(`a`c`a;1 3 4;10 30 10);+`s`k`n`v!(`a`b;1 1;5 7;8 9))]
t[`ejKeyedY;.[ej;(`s;+`s`n!(`a`b;1 2);(+(,`s)!,`a`c)!+(,`v)!,10 30);{`err}];`err]
t[`ejFloatKeys;ej[`s;+`s`n!(-0.0 0n 1.5;1 2 3);+`s`v!(0.0 0n;10 20)];+`s`n`v!(-0.0 0n;1 2;10 20)]
t[`ejManyMatches;ej[`s;+`s`n!(`a`b`c;1 2 3);+`s`v!(`b`a`b`b`c`b`b`b;1+!8)];+`s`n`v!(`a`b`b`b`b`b`b`c;1 2 2 2 2 2 2 3;2 1 3 4 6 7 8 5)]
t[`ejCharKeys;ej[`s;+`s`n!("ab";1 2);+`s`v!("bba";10 11 20)];+`s`n`v!("abb";1 2 2;20 10 11)]

/ ---- asof / window joins ----
trade:+`sym`time`px!(`a`b`a;3 4 9;100 200 300)
quote:+`sym`time`bid!(`a`a`b`a;1 5 2 8;10 11 20 12)
t[`aj;(+aj[`sym`time;trade;quote])`bid;10 20 12]
t[`aj0time;(+aj0[`sym`time;trade;quote])`time;1 2 8]
t[`wjMax;(+wj[(trade[`time]-2;trade`time);`sym`time;trade;quote;,(`mx;max;`bid)])`mx;10 20 12]
t[`asof;asof[xasc[`sym`time;trade];`sym`time!(`a;6)];(,`px)!,100]

/ ---- native aj kernel (src/a.c): edge cases ----
/ no match: trade before first quote in its group, and a symbol absent from quotes -> null
tradeN:+`sym`time`px!(`a`c;0 5;1 2)
t[`ajNull;(+aj[`sym`time;tradeN;quote])`bid;0N 0N]
/ single-group (time-only) as-of: most recent quote on-or-before each trade time
t[`ajNoGrp;(+aj[`time;+`time`x!(0 4 8;0 0 0);+`time`v!(1 3 5 7;10 20 30 40)])`v;0N 20 40]
t[`ajNaNHidesUnsorted;(+aj[`sym`time;+`sym`time!(`a`a`a;0.5 1.5 3.5);+`sym`time`bid!(`a`a`a;3.0 0n 1.0;10 20 30)])`bid;20 30 10]
/ a lazy range as a time column (!n): the kernels used to see it as a 2-item range
t[`ajRangeTrades;(+aj[`time;+`time`px!(!10;!10);+`time`bid!(0 2 4 6 8;1 2 3 4 5)])`bid;1 1 2 2 3 3 4 4 5 5]
t[`ajRangeQuotes;(+aj[`time;+`time`px!(1 3 5;7 8 9);+`time`bid!(!6;10+!6)])`bid;11 13 15]
t[`wjRangeTime;(+wj[(-1+!5;1+!5);`time;+`time`px!(!5;!5);+`time`bid!(!5;10*!5);,(`s;sum;`bid)])`s;10 30 60 90 70]
t[`ajZeroGroups;ajsorted[`g`time;+`g`time`bid!(0.0 1.0 -0.0;1 2 3;10 20 30)];0b]
/ the row is the one aj gives on the sorted quotes (which zero comes first is issue #15's order)
ajZT:+`g`time!(0.0 -0.0 1.0;5 5 5);ajZQ:+`g`time`bid!(0.0 1.0 -0.0;1 2 3;10 20 30)
t[`ajZeroGroupsRow;(+aj[`g`time;ajZT;ajZQ])`bid;(+aj[`g`time;ajZT;xasc[`g`time;ajZQ]])`bid]
t[`ajSortedZeros;{n:-1*x;(ajsorted[`s`t;+`s`t!(`a`a;n,x)];ajsorted[`s`t;+`s`t!(`a`a;x,n)])}0.0;11b]
/ a NaN time sorts first, as one value: quotes 0.0 1.0 -0n are unsorted, and a trade at -0.0 takes the 0.0 quote, as in q
t[`ajNanQuoteLast;(+aj[`sym`time;+`sym`time!(,`a;,-0.0);+`sym`time`bid!(`a`a`a;0.0 1.0 -0n;10 20 30)])`bid;,10]
t[`ajCharTimes;(+aj[`sym`time;+`sym`time!(`a`a`a;`c$66 201 64);+`sym`time`bid!(`a`a;`c$65 200;10 20)])`bid;(+aj[`sym`time;+`sym`time!(`a`a`a;`c$66 201 64);xasc[`sym`time;+`sym`time`bid!(`a`a;`c$65 200;10 20)]])`bid]
t[`asofLatest;asof[+`sym`time`bid!(`a`a`a`a;0 1 1 0;10 20 30 40);`sym`time!(`a;1)];(,`bid)!,30]

t[`divExact;(div[4417977684374999297;1000000];div[-4417977684374999297;7];div[17;-3];div[17 0N -17;8]);(4417977684374;-631139669196428471;-6;2 0N -3)]
t[`xbarExact;(xbar[1000000;4417977684374999297];xbar[5;4611686018427387903];xbar[5;17 18]);(4417977684374000000;4611686018427387900;15 15)]
t[`barExact;bar[1;1000000;4417977684374999297];4417977684374000000]
t[`divExactVec;(div[4417977684374999297;1000000 1000000];div[4417977684374999297 17;1000000 -7];div[17;5 -5 8 9 -8 -9];div[-17;5 -5 8 9]);(4417977684374 4417977684374;4417977684374 -3;3 -4 2 1 -3 -2;-4 3 -3 -2)]
t[`xbarExactVec;(xbar[1000000 7;4417977684374999297 4417977684374999297];xbar[5 10;17]);(4417977684374000000 4417977684374999297;15 10)]
t[`divVecFloat;div[17.5;2 3];8.0 5.0]
/ q's rules (issue #20, row 10): an int by a float divisor rounds it half away from zero; a float dividend floors and stays a float
t[`divFloatDivisor;(17 div 2.5;17 div 3.5;17 div -2.5;17 div 2.5 -2.5 0n;17 div 0n);(5;4;-6;5 -6 0N;0N)]
t[`divFloatDividend;(17.5 div 2;17.5 div 2.5;-17.5 div 2;17.5 2.5 div 2;@17.5 div 2;0n div 2);(8.0;7.0;-9.0;8.0 1.0;`f;0n)]
t[`divNullDivisor;(17 div 0N;17 18 div 0N;17 div 2 0N);(0N;0N 0N;8 0N)]
t[`xbarNull;(xbar[0N;17];xbar[2;0N];xbar[5;0N];xbar[2;0N 17];xbar[0N 2;17];bar[0N;60;17];bar[5;60;0N]);(0N;0N;0N;0N 16;0N 16;0N;0N)]
t[`xbarFloatNull;xbar[0.5;1.7 0n];1.5 0n]
t[`divFloatNullDivisor;`k@(17.5 div 0N;17.5 2.5 div 2 0N;xbar[0N;17.5]);"(0n;8.0 0n;0n)"]   / a null divisor gives a null for a float dividend too, as in q
t[`divVecExact;(9223372036854775807 -9223372036854775807 0N 7 -7 1700000000123456789 div 1 1 2 0N 2 60000000000);9223372036854775807 -9223372036854775807 0N 0N -4 28333333]   / an int vector divisor with nulls or values past 2^53, exactly
t[`divFloatBelow;(-17.5 div 0;-1e300 div 3;-1e300 1e300 div 3);(0n;0n;0n 9.223372036854776e18)]   / below the int range the floor is null, as in q, atom or list
/ 64-bit nanosecond timestamps (values exceed 2^31 -> proves long/ns path)
t[`ajNs;(+aj[`sym`time;+`sym`time`px!(,`a;,1500000000000;,1);+`sym`time`bid!(`a`a;1000000000000 2000000000000;10 20)])`bid;,10]
/ float times compare as floats (the kernels used to truncate them to ints, so 0.5 and 0.7 tied)
qtF:+`sym`time`bid!(`a`a;0.5 0.7;10 20)
trF:+`sym`time!(`a`a`a;0.6 0.4 1.0)
t[`ajFloat;(+aj[`sym`time;trF;qtF])`bid;10 0N 20]
t[`aj0Float;(+aj0[`sym`time;+`sym`time!(`a`a;0.6 1.0);qtF])`time;0.5 0.7]
t[`ajFloatInt;(+aj[`time;+(,`time)!,0.5 2.5;+`time`v!(1 2;10 20)])`v;0N 20]
/ -0.0 and 0.0 are one time, as in q: a trade at -0.0 takes the last quote at either zero
t[`ajZeroTimes;(+aj[`sym`time;+`sym`time!(`a`a`a`a;-0.0 0.0 0.5 1.0);+`sym`time`bid!(`a`a`a;-0.0 0.0 1.0;10 20 30)])`bid;20 20 20 30]
qwF:+`sym`time`bid!(`a`a`a;0.5 0.7 0.9;10 20 30)
twF:0.6 1.0
t[`wjFloat;(+wj[(twF-0.15;twF+0.15);`sym`time;+`sym`time!(`a`a;twF);qwF;,(`c;count;`bid)])`c;2 1]
t[`wjFloatFallback;(+wj[(twF-0.15;twF+0.15);`sym`time;+`sym`time!(`a`a;twF);qwF;,(`s;{sum x};`bid)])`s;30 30]
t[`wjNullLowFallback;(+wj[(0N 4 7;1 5 9);`sym`time;+`sym`time!(`a`a`a;1 5 9);+`sym`time`bid!(`a`a`a`a;0 4 5 8;10 20 30 40);,(`s;{sum x};`bid)])`s;10 50 40]   / a null lower bound starts at the first quote, as the kernel does
t[`wjIntTimesFloatWindow;(+wj[(,4.5;,5.5);`sym`time;+`sym`time!(,`a;,5);+`sym`time`bid!(`a`a`a;3 4 5;10 20 30);,(`s;{sum x};`bid)])`s;,30]   / a float bound over int times, as the kernel
t[`wjNullLowNaNQuote;(+wj[(,0N;,1.5);`sym`time;+`sym`time!(,`a;,1.5);+`sym`time`bid!(`a`a`a;0n 1.0 2.0;10 100 1000);,(`s;{sum x};`bid)])`s;,110]   / a null lower bound over float times with a NaN quote, as the kernel
t[`ajCharHigh;(+aj[`sym`time;+`sym`time!(`a`a`a;`c$66 201 64);+`sym`time`bid!(`a`a;`c$65 200;10 20)])`bid;10 20 0N]   / char times order as unsigned bytes, as in q (issue #17)
t[`ajCharQuoteHigh;(+aj[`sym`time;+`sym`time!(`a`a;`c$66 71);+`sym`time`bid!(`a`a`a;`c$65 70 200;10 20 30)])`bid;10 20]
t[`ajCharHighRev;(+aj[`sym`time;+`sym`time!(`a`a`a;`c$66 201 64);+`sym`time`bid!(`a`a;`c$200 65;20 10)])`bid;10 20 0N]   / the same quotes in the other order
t[`wjCharHigh;{q:+`sym`time`bid!(`a`a`a;`c$65 200 210;10 20 30);t:+`sym`time!(`a`a;`c$66 201);w:(`c$64 199;t`time);((+wj[w;`sym`time;t;q;,(`mx;max;`bid)])`mx;(+wj[w;`sym`time;t;q;,(`n;count;`bid)])`n)}[];(10 20;1 1)]   / wj's kernel reads char times unsigned too (the max as in q; q counts the quote in force at the window start too)
t[`wjCharFallback;{q:+`sym`time`bid!(`a`a`a;`c$65 128 200;10 20 30);t:+`sym`time!(,`a;,`c$200);w:(,`c$128;t`time);s:(+wj[w;`sym`time;t;q;,(`s;sum;`bid)])`s;v:(+wj[w;`sym`time;t;q;,(`v;{x};`bid)])`v;(s;v)}[];(,50;,20 30)]   / wj's fallback in K reads char times unsigned too, as the kernel
t[`ajCharTradeNumQuotes;(+aj[`sym`time;+`sym`time!(,`a;,`c$250);+`sym`time`v!(`a`a`a;1 100 200;1 2 3)])`v;,0N]   / a char trade time against number quote times stays signed, as ' and ajmK read it
t[`wjCharMax;{q:+`sym`time`c!(`a`a`a;1 2 3;"a",0x80,"b");t:+`sym`time!(,`a;,3);w:(,0;,3);((+wj[w;`sym`time;t;q;,(`m;max;`c)])`m;(+wj[w;`sym`time;t;q;,(`m;min;`c)])`m;(+wj[w;`sym`time;t;q;((`m;max;`c);(`k;{max x};`c))])`m)}[];(,0x80;,"a";,0x80)]   / min and max of a char column give chars, whatever else is aggregated
t[`wjCharFirstMax;{q:+`sym`time`c!(`a`a`a;1 2 3;"a",0x80,"b");t:+`sym`time!(`a`a;3 0);w:(0 -5;3 0);r:+wj[w;`sym`time;t;q;((`f;first;`c);(`m;max;`c);(`n;min;`c))];(r`f;r`m;r`n)}[];(97 0N;0x8000;0x61ff)]   / the kernel keeps first as it was; min and max of chars give chars, an empty window the identity
/ the sortedness check given an atom where a column belongs: "not known sorted", not a crash
t[`ajsAtom;(`ajs(();5;0b);`ajs((5;1 2);1 2;0b);`ajs((1 2;5);1 2;0b);`ajs(();3 2 1;0b));0 0 0 0]

/ ---- `ajs / ajsorted (src/a.c): is a table already in as-of order? ----
/ A WRONG 1b here silently corrupts every join that trusts it, so the predicate
/ is pinned in both directions: it must say yes for an ordered book, and no for
/ each way a book can be out of order.
ajQ0:+`sym`time`bid!(`a`a`b`b;1 3 2 4;10 20 30 40)          / ordered
ajQ1:+`sym`time`bid!(`b`a`b`a;2 1 4 3;30 10 40 20)          / groups interleaved
ajQ2:+`sym`time`bid!(`a`a`b`b;3 1 2 4;10 20 30 40)          / time descends in `a
ajQ3:+`sym`time`bid!(`a`b`a;1 2 3;10 20 30)                 / group `a in two runs
t[`ajsOrdered;   ajsorted[`sym`time;ajQ0];1]
t[`ajsInterleave;ajsorted[`sym`time;ajQ1];0]
t[`ajsTimeDesc;  ajsorted[`sym`time;ajQ2];0]
t[`ajsSplitRun;  ajsorted[`sym`time;ajQ3];0]
t[`ajsNoGroup;   ajsorted[,`time;+`time`v!(1 2 3;10 20 30)];1]
t[`ajsAfterXasc; ajsorted[`sym`time;xasc[`sym`time;ajQ1]];1]
/ and the gate must not change ANY answer: same book, shuffled rows, same join
ajT:+`sym`time`px!(`a`b`a`b;2 3 4 5;1.0 2 3 4)
t[`ajGateAj;(aj[`sym`time;ajT;ajQ0])~aj[`sym`time;ajT;ajQ1];1]
t[`ajGateWj;(wj[(ajT[`time]-2;ajT`time);`sym`time;ajT;ajQ0;,(`mx;max;`bid)])~wj[(ajT[`time]-2;ajT`time);`sym`time;ajT;ajQ1;,(`mx;max;`bid)];1]

/ ---- `wjb / wjbnd (src/a.c): C group-bounds kernel vs the pure-K path ----
/ wjbnd feeds BOTH aj and wj their per-row group slices, so a wrong slice here
/ silently attaches trades to the wrong symbol's quotes. The C kernel is pinned
/ against wjbndK (the original K implementation, kept as the fallback) by
/ differential testing over randomly generated books -- sorted and shuffled,
/ many groups and one group, more groups than rows -- rather than by a handful
/ of fixed cases. `wjb is allowed to DECLINE (return ()), which the harness
/ accepts; what it may never do is return a different answer from K.
wjbChk:{[nq;nt;ns;srt]sy:`$'("s",'$!ns);
  q:+`sym`time`bid!(sy nq?ns;nq?50;nq?100.0); q:$[srt;xasc[`sym`time;q];q];
  t:+`sym`time`px!(sy nt?ns;nt?50;nt?100.0);
  c:`wjb(value (,`sym)#+q;value (,`sym)#+t); k:wjbndK[,`sym;t;q];
  $[0=#c;1b;((c 0)~k 0)&(c 1)~k 1]}
wjbR:()
{[i]wjbR,:wjbChk[100;60;5;1b]}'!12
{[i]wjbR,:wjbChk[100;60;5;0b]}'!12
{[i]wjbR,:wjbChk[300;150;25;1b]}'!8
{[i]wjbR,:wjbChk[20;120;30;1b]}'!8
t[`wjbVsK;&/wjbR;1b]
/ end-to-end: the same book shuffled vs pre-sorted must give the same join
ajEqChk:{[nq;nt;ns]sy:`$'("s",'$!ns);
  q:+`sym`time`bid!(sy nq?ns;nq?50;nq?100.0);
  t:+`sym`time`px!(sy nt?ns;nt?50;nt?100.0); qs:xasc[`sym`time;q];
  w:(t[`time]-3;t`time); ag:((`mx;max;`bid);(`n;count;`bid));
  ((aj[`sym`time;t;q])~aj[`sym`time;t;qs])&(wj[w;`sym`time;t;q;ag])~wj[w;`sym`time;t;qs;ag]}
ajEqR:()
{[i]ajEqR,:ajEqChk[200;120;7]}'!10
{[i]ajEqR,:ajEqChk[60;60;20]}'!8
{[i]ajEqR,:ajEqChk[20;150;30]}'!8
t[`ajShuffleEq;&/ajEqR;1b]
/ a symbol present in trades but absent from quotes -> null slice, null match
t[`wjbMissing;(+aj[`sym`time;+`sym`time`px!(,`zz;,5;,1.0);+`sym`time`bid!(`a`a;1 2;10 20)])`bid;,0N]
/ the `p (parted) attribute lets ajsorted skip the contiguity pass; it must not
/ change the answer for a table that genuinely is parted-but-not-sorted
/ ---- `p (parted) attribute fast path ----
/ A parted-but-NOT-sorted book (groups contiguous, symbols in no particular
/ order) is exactly what `p asserts. ajsorted must accept it either way; with the
/ attribute present it skips the O(n) contiguity pass instead of re-deriving it,
/ which must not change the answer.
ajPq:+`sym`time`bid!(`b`b`a`a;1 3 2 4;30 40 10 20)      / parted, not sorted
ajPt:+`sym`time`px!(`a`b;3 2;1.0 2.0)
t[`ajPartedSeen;ajsorted[`sym`time;ajPq];1]
t[`ajPartedAttr;ajsorted[`sym`time;+(+ajPq),(,`sym)!,`pa (+ajPq)`sym];1]
t[`ajPartedSame;(aj[`sym`time;ajPt;ajPq])~aj[`sym`time;ajPt;xasc[`sym`time;ajPq]];1]
/ `p must NOT rescue a book whose times descend inside a group -- that is pass 2,
/ which the attribute says nothing about and which is therefore still checked.
ajPbad:+`sym`time`bid!(`b`b`a`a;3 1 2 4;30 40 10 20)
t[`ajPartedTimeStill;ajsorted[`sym`time;+(+ajPbad),(,`sym)!,`pa (+ajPbad)`sym];0]

/ ---- HFT arena scratchpad (src/arena.c) : bump / reset / overflow self-test ----
t[`arena;`arn 0;1]

/ ---- Rust-style diagnostics (src/diagnostic.c) : formatter structure self-test ----
t[`diag;`dgn 0;1]

/ ---- SIMD kernels (src/simd.c) : add/mul/sum vs scalar reference, i64+f64 ----
t[`simd;`simd 0;1]

/ ---- bytecode disassembler (src/vm.c) : decodes real b.c-compiled closures ----
t[`vmDisasm;`vmd 0;1]

/ ---- multithreaded vector engine (src/parallel.c) : par vs serial-simd ----
t[`parallel;`para 0;1]

/ ---- native CSV parser (src/csv.c) : shape/type/null round-trip via K primitives ----
t[`csv;`csv0 0;1]

/ ---- AST visualizer (src/ast.c) : \ast tree labels for verbs/adverbs/lambdas/
/ hooks/forks/projections/literal vectors -- no "<X-atom>" placeholder regressions ----
t[`ast;`astt 0;1]

/ ---- qSQL-style ----
st:+`s`v!(`a`b`a`b`a;1 2 3 4 5)
t[`qwhere;(+qwhere[st;st[`v]>2])`v;3 4 5]
t[`qbyTot;(value qby[st;`s;(,`tot)!,{sum x`v}])`tot;9 6]
t[`fby;fby[(sum;10 20 30 40 50);`a`b`a`b`a];90 60 90 60 90]
/ fby on SEVERAL group columns, q's (max;px) fby (sym;ex).  `=` on a list of
/ columns groups the columns, so this used to return one value per COLUMN
/ (2 of them) instead of one per row -- a wrong answer, not an error.
t[`fbyCols;fby[(max;10 20 30 40 50.0);(`a`b`a`b`a;`X`X`Y`Y`X)];50 20 30 40 50.0]
t[`fbyCols2;fby[(sum;1 2 3 4 5);(`a`b`a`b`a;`X`X`Y`Y`X)];6 2 3 4 6]
/ the same grouping written as a table was always right: #t is its row count
t[`fbyTable;fby[(sum;1 2 3 4 5);([]s:`a`b`a`b`a;e:`X`X`Y`Y`X)];6 2 3 4 6]
/ ... and a general list that genuinely has one item per datum is NOT flipped
t[`fbyListSame;fby[(sum;10 20);(`a`b;`a`b)];30 30]
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
\l temporal.k
t[`tsLastNs;pstr 2026.07.30D23:59:59.999999999;"2026.07.30D23:59:59.999999999"]
t[`tsMsFloor;pstr 2140.01.01D00:01:24.374999297;"2140.01.01D00:01:24.374999297"]
t[`tsBefore2000;pstr 1999.12.31D23:59:59.999999999;"1999.12.31D23:59:59.999999999"]
t[`fdivExact;(fdiv[4417977684374999297;1000000];fdiv[-5;10];fdiv[17;8];fdiv[-17;8];fdiv[80;9]);(4417977684374;-1;2;-3;8)]
t[`fdivNegative;fdiv[17;-3];-6]
t[`fdivNull;fdiv[17 0N -17;8];2 0N -3]
t[`fdivFloat;fdiv[17.5;2];8]
t[`fdivVector;fdiv[17;5 6];3 2]
t[`hms;hms[9;30;15];34215000]
t[`hh;hh hms[9;30;15];9]
t[`mm;mm hms[9;30;15];30]
t[`sec;sec hms[9;30;15];15]
t[`minute;minute hms[9;30;15];570]
t[`second;second hms[9;30;15];34215]
t[`milli;milli hms[9;30;15]+123;123]
t[`stime;stime hms[9;30;15];"09:30:15.000"]
t[`ptime;ptime"09:30:15.123";34215123]
t[`stimeNeg;stime -1800000 -1 1800000;("-00:30:00.000";"-00:00:00.001";"00:30:00.000")]
t[`ptimeNeg;(ptime"-00:30:00.000";ptime"-01:00:00");-1800000 -3600000]
t[`ptimeList;*"\n"\.[{"T"$("09:30:00.100";"10:00:00.000")};,0;{x}];"'type"]   / a list of strings is 'type, as before (the sign test must not recurse)
t[`timeNegText;`i$"T"$"-00:30:00.000";-1800000]
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

/ ---- regression: & (where) on a literal empty generic list -> !0 (was ,!0) ----
/ was: &() returned a 1-element list (,!0) instead of the empty int vector !0.
/ fixed in src/v.c whr() RA branch with an empty-guard.  see docs/MISSING.md 14.
t[`whrEmptyGen;&();!0]
t[`whrEmptyGenType;@&();`I]
t[`whrEmptyGenCount;#&();0]
t[`whrEmptyMatchesRange;(&())~&!0;1b]
t[`whrGroupedStillWorks;&(0 1 0;1 0 1);(0 1 1;1 0 2)]

/ ---- infix notation for two-arg library dyads (amber 2.0.0) ----
/ `x f y` now applies the named dyad f infix, identical to the bracket form
/ f[x;y].  The bracket form still works, and definitions (in:{...}) still assign.
t[`infixIn;      2 3 9 in 2 3 4;        in[2 3 9;2 3 4]]
t[`infixInVal;   2 3 9 in 2 3 4;        1 1 0]
t[`infixWithin;  5 within 3 9;          within[5;3 9]]
t[`infixLike;    "hello" like "h*";     like["hello";"h*"]]
t[`infixExcept;  1 2 3 4 5 except 2 4;  except[1 2 3 4 5;2 4]]
t[`infixInter;   1 2 3 inter 2 3 4;     inter[1 2 3;2 3 4]]
t[`infixUnion;   1 2 3 union 3 4;       union[1 2 3;3 4]]
t[`infixSv;      "/" sv ("a";"bc");     sv["/";("a";"bc")]]
t[`infixVs;      "/" vs "a/bc/d";       vs["/";"a/bc/d"]]
t[`infixCross;   1 2 cross 3 4;         cross[1 2;3 4]]
t[`infixBracketStillWorks; in[1 2 3;2 3]; 0 1 1]
t[`infixLj; (([]k:1 2 3;v:`a`b`c) lj ([k:1 2]w:10 20)); lj[([]k:1 2 3;v:`a`b`c);([k:1 2]w:10 20)]]

/ ---- nested/chained brackets: right-to-left evaluation (matches q) ----
/ Amber evaluates a function's bracketed arguments RIGHT-to-LEFT, exactly as
/ q does (verified: `f[mark 1;mark 2;mark 3]` logs 3 2 1 in both).  These
/ pin that order (via side effects) and the nested/chained-bracket results so a
/ parser change can never silently flip them.
RTLOG:()
rtmark:{RTLOG,:x;x}
t[`rtlArgOrder; [RTLOG:(); {[a;b;c]0}[rtmark 1;rtmark 2;rtmark 3]; RTLOG]; 3 2 1]
t[`nestedFnApply;  {x*10}{x+1}2;          30]
t[`nestedFnBrack;  {x*10}[{x+1}[2]];      30]
t[`deepNestFn;     {x*10}{x+1}{x*10}{x+1}1; 210]
t[`chainedIndex;   (10 20 30;40 50 60)[1][2]; 60]
t[`multiIndexEq;   (10 20 30;40 50 60)[1;2]; (10 20 30;40 50 60)[1][2]]
t[`projectThenApply; ({[a;b;c]a+b+c}[10;;30]) 5; 45]

/ ---- symbol grouping fast path (amber 2.0.0): group `=` on a symbol vector
/ groups by interned id, ~47x faster than the old lexical string-sort, and the
/ partition + first-appearance key order are byte-identical to the old result. ----
t[`groupSymKeys;   key(=`c`a`b`a`c);      `c`a`b]
t[`groupSymDict;   =`c`a`b`a`c;           `c`a`b!(0 4;1 3;,2)]
t[`groupSymSingle; =`x`x`x;               (,`x)!,0 1 2]
t[`groupSymEmptyCount; #(=0#`a);          0]
t[`groupSymMatchesInt; value(=`b`a`b`c`a); value(=2 1 2 3 1)]
/ JSON \u escapes: four hex digits, either case; a short or bad one is an error, read no further
t[`jsonHexCase;   `j?"\"\\u004a\\u004A\\u00e9\"";  "JJ",`c$195 169]
t[`jsonHexShort;  {.[`j?;,x;{`err}]}'("\"\\u";"\"\\u12";"\"\\u12\"";"\"\\ud83d\\u");  `err`err`err`err]
t[`jsonHexBad;    {.[`j?;,x;{`err}]}'("\"\\u004G\"";"\"\\uzz41\"";"\"\\u 041\"");  `err`err`err]

/ ---- tacit trains (hooks/forks) + function-list indexing (since 2.0.1) ----
/ A literal verb/lambda list applied to DATA runs as a train; the SAME shape of
/ list indexed by a scalar still INDEXES -- that is the (fmt;fmtx)[" "~*x] REPL
/ formatter dispatch, whose regression these cases exist to prevent recurring.
t[`forkMean;       (+/;%;#) 2 4 6 8 10;        6.0]
t[`forkRange;      (|/;-;&/) 5 2 9 1;          8]
t[`forkStored;     {m:(+/;%;#);m 1 2 3 4 5} 0; 3.0]
t[`hookAppendRev;  (,;|) 1 2 3;                1 2 3 3 2 1]
t[`hookNormSum;    +/(%;+/) 3 1 4 1 5;         1.0]
t[`forkDyadic;     (+;*;-)[10;3];              91]
t[`forkLambdas;    ({x*2};+;{x*10}) 5;         60]
t[`fnListIdx0;     ((+/;|/;&/)[0]) 5 2 9;      16]
t[`fnListIdx1;     ((+/;|/;&/)[1]) 5 2 9;      9]
t[`fnListIdx2;     ((+/;|/;&/)[2]) 5 2 9;      2]
t[`fnListMatchIdx; ((#;+/)[" "~" "]) 1 2 3;    6]
t[`fnListIdxType;  @(#;+/)[0];                 `v]
t[`nonFnListIdx;   @(&/;1;|/)[1];              `i]
t[`fourVerbIdx;    @(+;-;*;%)[2];              `v]
/ a JSON surrogate pair is one character: four bytes of UTF-8
t[`jsonSurrogate;  (`i$`j?"\"\\ud83d\\ude00\"";#`j?"\"a\\ud83d\\ude00\\u00e9\"");(-16 -97 -104 -128;7)]
/ -8!/-9! leave a boxed int on the left alone (it used to be released, freeing a variable's value)
t[`serLeftKept;    {m:(-8 5000000000)0;m!1 2;m!3 4;m}[];  -8]
t[`serLeftKept9;{m:(-9 5000000000)0;m!-8!1 2;m}[];-9]

/ -9! keeps an attribute only when the data keeps it (sort, grade and find trust `s)
t[`deserAttrKept;  (`at@-9!-8!`sa 1000 2000 3000;`at@-9!-8!`pa 3 3 1 1 2;`at@-9!-8!`ua 1 2 3);(`s;`p;`u)]
t[`deserAttrBad;   {c:-8!`sa 1000 2000 3000;c[14 15 18 19]:(-8!3000 2000 1000)14 15 18 19;x:-9!c;(`at@x;<x;x?1000)}[];(`;2 1 0;2)]

/ dates, times and timestamps grade by their values (not their words, nor a timestamp's address)
t[`gradeDates;     (<(1999.12.31;2000.01.01);<(2000.01.01;1999.12.31;2026.07.30);>(1999.12.31;2000.01.01));(0 1;1 0 2;1 0)]
t[`gradeTimes;     <(00:00:00.000-1;00:00:00.000);0 1]
t[`gradeStamps;    (<(2026.07.31D09:00:00.000;2026.07.30D10:00:00.000;2026.07.30D11:00:00.000);{<(x;x+0)}2026.07.30D10:00:00.000;<2026.07.30D10:00:00.000+|!5);(1 2 0;0 1;4 3 2 1 0)]
GTS:(2026.07.30D10:00:00.000;2026.07.30D10:00:00.000+1;2026.07.30D10:00:00.000+0;2026.07.30D10:00:00.000+1)
t[`groupStamps;   (#?GTS;#=GTS);  2 2]   / equal timestamps are one value
GN:(2026.07.30D10:00:00.000;0N;2026.07.30D09:00:00.000;0N;2026.07.30D10:00:00.000+0;2026.07.29;0.5)
t[`gradeStampsMixed; (<GN;>GN;#?GN;#=GN;GN?(?GN));  (5 6 1 3 2 0 4;0 4 2 1 3 6 5;5;5;0 1 2 5 6)]   / with a 0N, a date and a float among them: by kind (d f i n), then by value
XN:+`s`t`v!(`a`b`a`a`b`a`a;GN;!7)
t[`xascStampsMixed;  ((+`s`t xasc XN)`v;(+`s`t xdesc XN)`v;(+`t`s xasc XN)`v);  (5 6 3 2 0 1 4;4 1 0 2 3 6 5;5 6 3 1 2 0 4)]

/ aj and wj on columns of dates, times or timestamps compare their values
AJT:+`ts`v!((2026.07.30D10:00:00.000;2026.07.30D11:00:00.000;2026.07.31D09:00:00.000);1 2 3)
AJQ:+`ts`q!((2026.07.30D08:00:00.000;2026.07.30D10:30:00.000);6 5)
t[`ajStamps;       (+aj[`ts;AJT;AJQ])`q;  6 5 5]
t[`ajDates;        (+aj[`d;+`d`v!((2026.07.30;2026.07.31);1 2);+`d`q!((2026.07.29;2026.07.31);6 5)])`q;  6 5]
t[`ajStampsEmpty;  ((+aj[`ts;AJT;0#AJQ])`q;#aj[`ts;0#AJT;AJQ]);  (0N 0N 0N;0)]
AJW:((2026.07.30D09:00:00.000;2026.07.30D10:30:00.000);(2026.07.30D10:20:00.000;2026.07.30D11:10:00.000))
AJWT:+`ts`v!((2026.07.30D10:00:00.000;2026.07.30D11:00:00.000);1 2)
AJWQ:+`ts`q!((2026.07.30D09:30:00.000;2026.07.30D10:15:00.000;2026.07.30D10:50:00.000);6 5 4)
t[`wjStamps;       ((+wj[AJW;`ts;AJWT;AJWQ;,(`c;count;`q)])`c;(+wj[AJW;`ts;AJWT;AJWQ;,(`s;{+/x};`q)])`s);  (2 1;11 4)]   / {+/x}: wj's pure-K fallback
/ a temporal time column against one of another kind is 'type, as in q
AJD:+`ts`v!((2026.07.30;2026.07.31);1 2)
t[`ajMixedKinds;   *"\n"\.[aj;(`ts;AJD;AJQ);{x}];  "'type"]
t[`ajMixedInt;     *"\n"\.[aj;(`ts;+`ts`v!(10 9708;1 2);AJQ);{x}];  "'type"]
t[`wjMixedKinds;   *"\n"\.[wj;(((2026.07.30;2026.07.30);(2026.07.31;2026.07.31));`ts;AJWT;AJWQ;,(`c;count;`q));{x}];  "'type"]
t[`ajMixedItems;   *"\n"\.[aj;(`ts;+`ts`v!((2026.07.30D10:00:00.000;9708);1 2);AJQ);{x}];  "'type"]   / every item must be of the kind
t[`ajMixedNumbers; (*"\n"\.[aj;(`ts;+`ts`v!(1 2;1 2);+`ts`q!((1;2026.07.30D10:00:00.000);6 5));{x}];*"\n"\.[wj;((0 0;10 10);`ts;+`ts`v!(1 2;1 2);+`ts`q!((1;2026.07.30D10:00:00.000);6 5);,(`c;count;`q));{x}]);  2#,"'type"]   / a column mixing timestamps with numbers, against ints too (q answers for mixed quotes; here the timestamp would be read by its address)
t[`ajStampsUnsorted; (+aj[`ts;AJT;+`ts`q!(|AJQ`ts;5 6)])`q;  6 5 5]   / quotes out of order: sorted by value first
G0:2026.07.30D10:00:00.000
GT:+`g`t`v!((G0;G0+1;G0+0);10 10 11;1 2 3);GQ:+`g`t`q!((G0;G0+1;G0+0;G0+1);9 9 10 10;5 6 7 8)
t[`ajGroupStamps;   (+aj[`g`t;GT;GQ])`q;  7 8 7]   / grouped by a timestamp column: equal timestamps are one group
t[`wjGroupStamps;   ((+wj[(10 10 10;11 11 11);`g`t;GT;GQ;,(`c;count;`q)])`c;(+wj[(10 10 10;11 11 11);`g`t;GT;GQ;,(`s;sum;`q)])`s);  (1 1 1;7 8 7)]
t[`ajTimes;        (+aj[`t;+`t`v!((10:00:00.000;11:00:00.000);1 2);+`t`q!((09:00:00.000;10:30:00.000);6 5)])`q;  6 5]
t[`ajsortedStamps; (ajsorted[`ts;AJQ];ajsorted[`ts;+`ts`q!(|AJQ`ts;6 5)]);  1 0]   / sorted temporal quotes are not sorted again
/ a range (!n, a+!n) rises, so ajsorted takes it as sorted and the joins no longer sort the quotes first; the
/ wj kernel then reads a range value column as its items, not as its two ends (q has no ranges: the answers
/ are q's for the same columns as lists, wj's q's wj1, whose windows Amber's wj takes)
t[`ajsortedRange;  ((+aj[`g`time;+`g`time!(!3;!3);+`g`time`b!(!4;!4;10+!4)])`b;ajsorted[`s`time;+`s`time!(`a`a`b`b;!4)];ajsorted[`time;+(,`time)!,3+!4]);  (10 11 12;1b;1b)]
WQ:+`time`q`r!(1 1 1 1 1 1 1 1 3 3 3 3 4;!13;1000000000000+!13)
WW:(1 1 -1 -1 1 -1 -1 1 -1 -1 -1 2 -1;3 3 1 1 3 1 1 3 1 1 1 4 1)
WT:+`time`v!(2 2 0 0 2 0 0 2 0 0 0 3 0;!13)
t[`wjRangeColumn;  (+wj[WW;`time;WT;WQ;((`c;count;`q);(`z;sum;`q);(`f;first;`q);(`l;last;`r);(`m;max;`r);(`a;avg;`q))])`c`z`f`l`m`a;  (12 12 8 8 12 8 8 12 8 8 8 5 8;66 66 28 28 66 28 28 66 28 28 28 50 28;0 0 0 0 0 0 0 0 0 0 0 8 0;1000000000000+11 11 7 7 11 7 7 11 7 7 7 12 7;1000000000000+11 11 7 7 11 7 7 11 7 7 7 12 7;5.5 5.5 3.5 3.5 5.5 3.5 3.5 5.5 3.5 3.5 3.5 10 3.5)]
t[`wjRangeQuotes;  (+wj[(-1+!4;1+!4);`s`time;+`s`time!(`a`a`a`a;!4);+`s`time`v!(13#`a;!13;!13);((`z;sum;`v);(`f;first;`v);(`m;min;`v))])`z`f`m;  (1 3 6 9;0 0 1 2;0 0 1 2)]
/ wj's windows are as long as the trade table, as in q ('length; an empty trade table was read past its end)
t[`wjWindowLength; {*"\n"\.[wj;x;{x}]}'((WW;`time;0#WT;WQ;,(`z;sum;`q));((0 1;2 3);`time;3#WT;WQ;,(`z;sum;`q));((0 1 2;2 3);`time;3#WT;WQ;,(`z;sum;`q));((0 1;2 3);`time;3#WT;WQ;,(`z;{+/x};`q)));  4#,"'length"]
/ int folds, scans and amends wrap (computed in unsigned arithmetic: a signed overflow in C is undefined)
t[`scanWrap;       (*\0N 0N;+\9223372036854775807 1;*\4294967296 4294967296;9223372036854775806+\1 1);(0N 0;9223372036854775807 0N;4294967296 0;9223372036854775807 0N)]
t[`foldSeedWrap;   (9223372036854775807+/1 2;0N*/0N 3);(-9223372036854775806;0)]
t[`amendWrap;      ({x[0 1]*:-9223372036854775807;x}3#-1099511627776;@[2#9223372036854775807;0 0;+;1];{x[0 1]+:1;x}2#9223372036854775807);(3#-1099511627776;-9223372036854775807 9223372036854775807;0N 0N)]

/ ---- amber 2.2: float literals below 1e-308 (subnormals) ------------------
/ pfu (src/p.c) clamped an out-of-range exponent and returned EARLY, before it
/ advanced the parse cursor -- so `1e-309` left "e-309" in the input and the
/ literal died with 'value, and `1e309` did the same.  Every subnormal double
/ was unwritable, which is a round-trip hole: std.k's text ser/deser is `k
/ followed by eval, so a table holding one serialised to text that could not be
/ read back.  Now the cursor is advanced on both paths, and a small exponent is
/ scaled in two steps instead of answering 0.
t[`litSubnormalParses;  1e-309>0.0;              1b]
t[`litSmallestSubnorm;  5e-324>0.0;              1b]
t[`litSubnormalValue;   1e-320;                  1e-320]
t[`litSubnormalNeg;     (neg 5e-324)<0.0;        1b]
t[`litSubnormalMantissa;1.5e-310>1.0e-310;       1b]
t[`litUnderflowToZero;  1e-400;                  0.0]
t[`litDeepUnderflow;    1e-700;                  0.0]
t[`litOverflowToInf;    1e309;                   0w]
t[`litDeepOverflow;     1e400;                   0w]
t[`litNormalUnchanged;  2.5e2;                   250.0]
t[`litNegExpUnchanged;  2.5e-2;                  0.025]
t[`litExpBoundary;      1e-308>0.0;              1b]
/ the whole literal is consumed: a following token must still parse
t[`litCursorAdvanced;   1e-309 1.5;              1e-309 1.5]
t[`litCursorOverflow;   1e309 1.5;               0w 1.5]
t[`litRoundTrip;        . `k 5e-324;             5e-324]
t[`litRoundTripVec;     . `k (5e-324;1e-320;1.5);(5e-324;1e-320;1.5)]
t[`temporalNullPrint;(pstr 0N;dstr 0N;stime 0N;`k@1707.09.22D00:12:43.145224192);("0N";"0N";"0N";"0N")]

/ ---- 2.3: grade of 8-byte keys whose span fits in 32 bits ----
/ the radix picked its 4-byte kernel from the normalised byte count, so an int64
/ column holding both signs (or crossing 2^31), or tightly clustered floats,
/ graded to an unsorted permutation -- and = (group) fell back onto that grade.
srtd:{[x]g:<x;&/~(1_x g)<-1_x g}
t[`gradeI64Mixed;   srtd (2147483748;-100),400001!7*!300000;        1b]
t[`gradeI64Cross;   srtd 2147283648+400001!7*!300000;               1b]
t[`gradeF64Cluster; srtd 100.0+1e-12*400001!7*!300000;              1b]
t[`groupI64Mixed;   {#!=x}[(2147483748;-100),400001!7*!300000];    #?(2147483748;-100),400001!7*!300000]
/ small n takes the other kernel: 7 int64s straddling 2^31 graded to 1 3 5 6 0 2 4 in 2.2.0
t[`gradeI64Small;   <2147483458 2147483929 2147483995 2147483362 2147483030 2147483293 2147483857; 4 5 3 0 6 1 2]
/ ---- 2.3: symbols beyond the old 64 KB table, interned by hash ----
t[`symCapacity;     {#?`$x}[{"symbol_",$x}'!20000];                 20000]
t[`symRoundTrip;    {x~$`$x}[{"symbol_",$x}'!20000];                1b]
/ ---- 2.3: peach -- globals are read-only in workers; dicts map values ----
PG:0
t[`peachNoupdate;   .[{`pe({PG::x;x};!50)};,0;{`noupdate}];          `noupdate]
t[`peachGlobalKept; PG;                                              0]
/ JSON in peach: the reader's and the writer's positions are per thread (workers shared one, and read or wrote each other's text)
PJ:$%1+!100000
t[`peachJsonRead;  (`pe({`j?PJ x};!#PJ))~{`j?PJ x}'!#PJ;  1b]
t[`peachJsonWrite; (`pe({`j@PJ x};!#PJ))~{`j@PJ x}'!#PJ;  1b]
t[`peachJsonWriteFn; (`pe({`j@(x;{x+y})};!20000))~{`j@(x;{x+y})}'!20000;  1b]   / with a function's text, written through another worker's position
/ the 'noupdate of a global amend in a worker releases only what it owns: the value survives
PV:100 200 300;PH:0
PW:{.[{`pe({.[`PH;();:;PV]};x)};,!8;{x}]}
t[`peachAmendKept;  {PW[];PW[];PW[];b:{x,x}'!100;PV}[];  100 200 300]
t[`peachDict;       `pe({x+1};`a`b!1 2);                             `a`b!2 3]
t[`peachSmallN;     `pe({x*x};!7);                                   {x*x}'!7]
/ workers used to parse over each other (file-static parser state): 'parse, then a segfault
t[`peachEval;       `pe({. "1+",$x};!3000);                         1+!3000]
t[`peachEvalLambda; `pe({. "{x*y}[",($x),";2]"};!2000);             2*!2000]
/ ---- 2.3: float each-prior through the vector kernels ----
t[`eachPriorFsub;   -':1.5 4.0 2.5;                                  1.5 2.5 -1.5]
t[`eachPriorFeq;    =':1.0 1.0 2.0 0n 0n;                            0 1 0 0 1]
t[`eachPriorFmax;   |':3.0 1.0 4.0;                                  3.0 3.0 4.0]
/ a seed at the top of the storage width (127 on bytes; 0N for &', which mnmp flips to
/ the int64 max) came back as seed,seed,..
t[`eachPriorMinNull; &':0N 3 1 2 5 4;                                0N 0N 1 1 2 4]
t[`eachPriorSeedMin; 0N&':3 1 2 5 4;                                 0N 1 1 2 4]
t[`eachPriorSeedTop; 127|':3 1 2 5 4;                                127 3 2 5 5]
t[`eachPriorSeedWide;1000|':3 1 2 5 4;                               1000 3 2 5 5]
/ ---- 2.3: fused (1_x) OP ((-1)_x), +/ of it, and +/(a+s*b)@&(c OP k) ----
SH:3 1 4 1 5 9 2 6;SF:3.5 1.0 4.0 4.0 -2.0
t[`shiftLt;         (1_SH)<(-1)_SH;                                  1 0 1 0 0 1 0]
t[`shiftSum;        +/(1_SF)<(-1)_SF;                                2]
t[`shiftFlip;       ((-1)_SH)-(1_SH);                                2 -3 3 -4 -4 7 -4]
t[`shiftF;          (1_SF)-(-1)_SF;                                  -2.5 3.0 0.0 -6.0]
FX:0.0+!10;FY:10.0+!10
t[`fmsCmp;          +/(FY+2.5*FX)@&FX>5;                             {+/x@&y}[FY+2.5*FX;FX>5]]
t[`fmsCmpLitLeft;   +/(FY-2*FX)@&5<FX;                               {+/x@&y}[FY-2*FX;FX>5]]
/ the comparison-fused forms add in the same order as +/ of the selected values, also where the sum is inexact
CX:-1298990374206.0 1.0 -9005900264370198.0 1298990374206.0;CC:1 1 1 1
t[`fmsCmpExact;`k@(+/CX@&CC>0;+/(CX+0.0*CX)@&CC>0);`k@(+/CX;+/CX)]
t[`maskSumCmp;      +/FY@&FX<3;                                      33.0]

/ ---- report ----
fails:r@&{"FAIL"~4#x}'r[;1]
`0:""
`0:"================ AMBER TEST SUITE ================"
`0:(($#r)," tests run, ",($#fails)," failures")
$[#fails;{`0:"  ",(x 0),": ",x 1}'fails;`0:"ALL TESTS PASSED"];
`0:"================================================="
