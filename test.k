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
t[`wsum;wsum[1 2 3;10 20 30];140.0]
t[`qEdges67;(wsum[1 2;3 4];xbar[2;1.5 3.5];xbar[2;1 3];@ratios[!0];med 5;med 1.5);(11.0;0.0 2.0;0 2;`F;5.0;1.5)]   / Digest #67: as q
t[`wavg;wavg[10 20 30;1 2 3];7%3.0]
t[`cor;0.0001>abs 1.0-cor[1 2 3;2 4 6];1]
t[`cov;0.0001>abs cov[1 2 3;1 2 3]-2%3.0;1]
/ Digest #43: the q-named stats skip nulls (a pair with one, for the two-list ones); #46: cov is centred
t[`sumsNull;   (sums 1 0n 2;sums 1 0N 2;prds 2 0n 3;prds 2 0N 3);   (1 1 3.0;1 1 3;2 2 6.0;2 2 6)]
t[`prdNull;    (prd 2 0n 3;prd 2 0N 3;prd 2 0N 3 2147483648);     (6.0;6;6*2147483648)]
t[`wsumNull;   (wsum[1 2 3.0;1 0n 2];wsum[1 2 3;1 0N 2];wsum[1 2 3;1 0n 2];wsum[1 2 3+2147483648;1 0N 2]);   (7.0;7.0;7.0;`f$7+3*2147483648)]
t[`wavgNull;   (wavg[1 2 3;1 0n 2];wavg[1 2 3;1 0N 2];wavg[1 2 3.0;4 5 6.0]);   (1.75;1.75;32%6)]
t[`svarNull;   (svar 1 2 0n;sdev 1 2 0n);   (0.5;sqrt 0.5)]
t[`covNull;    (cov[1 2 3;1 2 0n];scov[1 2 3;1 2 0n];cor[1 2 3;1 2 0n];cov[1 2 3 0n;0n 2 3 4]);   (0.25;0.5;1.0;0.25)]
t[`scovNoPair; (scov[0n 0n;1 2.0];scov[0#0;0#0];scov[1 0N;0N 2];scov[,1.0;,2.0];svar 0#0.0;svar@,1);   (0n;0n;0n;0n;0n;0n)]   / none or one pair left: 0n, as q (2.5.0-2.7.2: -0.0 for none)
t[`covFar;     (cov[10000000000+0 1 2.0;0 0 1.0];1e-10>abs cor[10000000000+0 1 2.0;0 0 1.0]-sqrt 0.75);   (1%3;1b)]
t[`covKinds;   (cov[1 2 3 4.0;2 4 6 9.0];scov[1 2 3 4;2 4 6 9];cov[(1;2;3.0);1 2 3.0]);   (2.875;11.5%3;2%3)]
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
/ Digest #48: the null is the list's own; #49 all of empty; #50 differ's first item; #51 x vs ""
t[`prevNextTyped;(next `a`b;next 1.5 2.5;prev 1.5 2.5;prev "ab";prev[!0]);(`b`;2.5 0n;0n 1.5;" a";!0)]
t[`allEmpty;(all 0#0;all 1 2;all 1 0;all 0#0.0;any 0#0);1 1 0 1 0]
t[`differNull;(differ 0N 0N 1;differ 0.0 -0.0 0n 0n 1.0;differ `a`a`b;differ (1;1;`a);differ 5);(101b;10101b;101b;101b;1b)]
t[`vsEmpty;("," vs "";"," vs "a,b");(,"";(,"a";,"b"))]
t[`reverse;reverse 1 2 3;3 2 1]
t[`rotate;rotate[2;1 2 3 4 5];3 4 5 1 2]
t[`rotateWhole;(rotate[3;1 2 3];rotate[0;1 2 3];rotate[-3;1 2 3]);3#,1 2 3]   / Digest #47: was ::
/ Digest #38: an atom does what q does, not what the k primitive does
t[`negAfterParams;({[x]-1+x}0 1 2;{[x]-x}0 1 2;{[x;y]-1+x+y}[1;2];{[x]-1.5*x}2);(-1 0 1;0 -1 -2;2;-3.0)]   / Digest #40: was -(1+x)
t[`exceptCharNum;(1 2 3^"a";"ab"^1;(1 2 3)^,"a";except[1 2 3;"a"];except["ab";1 2];"abc"^"b";1 2 3^2);(1 2 3;"ab";1 2 3;1 2 3;"ab";"ac";1 3)]   / Digest #64: nothing removed, as q and ngn/k
t[`exceptMixed;(except[(1;2.5);"a"];except[(0;" ");0];except[(1;"a";`b);"a"];except[(1;"a");1 2];except[(1;"a";2);1];except[(1;`a;2);1];except[("ab";"c");0];except[(`a;"a");0];except[(1 2;"a");1]);((1;2.5);," ";(1;`b);,"a";("a";2);(`a;2);("ab";"c");(`a;"a");(1 2;"a"))]   / digest #64: was 'type
t[`jsonKeys;(`j@1 2!3 4;`j@(1.5 2)!3 4;`j@`a`b!3 4;`j@("ab";"cd")!3 4;5#.[`j@;,(1 2;3 4)!3 4;{x}]);("{\"1\":3,\"2\":4}";"{\"1.5\":3,\"2\":4}";"{\"a\":3,\"b\":4}";"{\"ab\":3,\"cd\":4}";"'type")]   / Digest #60: number keys as strings, list keys 'type, as q
t[`jsonKeyedTable;(`j@(+(,`k)!,1 2)!+(,`v)!,3 4;`j@`a`b!((+(,`k)!,!0)!+(,`v)!,!0;(+(,`k)!,1 2)!+(,`v)!,3 4);`j@(+`a`b!(1 2;`x`y))!+`c`d!(3 4;5 6);`j@(+`a`b!(1 2;`x`y))!+`b`a!(3 4;5 6);{5#.[`j@;,x;{x}]}'((+(,`k)!,1 2)!3 4;(`a;+`a!,1 2)!3 4;(`a;`a`b!1 2)!3 4;(1;::)!3 4;(1;-:)!3 4));("[{\"k\":1,\"v\":3},{\"k\":2,\"v\":4}]";"{\"a\":[],\"b\":[{\"k\":1,\"v\":3},{\"k\":2,\"v\":4}]}";"[{\"a\":1,\"b\":\"x\",\"c\":3,\"d\":5},{\"a\":2,\"b\":\"y\",\"c\":4,\"d\":6}]";"[{\"a\":1,\"b\":\"x\",\"b\":3,\"a\":5},{\"a\":2,\"b\":\"y\",\"b\":4,\"a\":6}]";5#,"'type")]   / digest #60: a keyed table as 0!t, as q (names shared by key and value repeat, as q); table, dict, :: and verb keys 'type, as q
t[`listFnAtom;(prev 5;next 5;raze 5;5#.[distinct;,5;{x}];5#.[group;,5;{x}];distinct `a`b!1 1);(5;5;,5;"'type";"'type";,1)]
t[`listFnAtomRank;{5#.[x;,5;{x}]}'(asc;desc;iasc;idesc;rank;flip;xrank[2];rotate[1];xprev[1]);9#,"'rank"]   / was 5, 'io (desc), 5, 'io (idesc), 5, ,,5, 10, 5, 'type
t[`listFnAtomAsIs;{.[*x;1_x;{5#x}]}'((deltas;5);(ratios;2.0);(fills;0N);(fills;`a);(sublist;2;5);(xbar;2;5.5);(xbar;2 3;5.5));(5;2.0;0N;`a;5;4.0;4 3.0)]   / was 'rank 'rank 'type 'domain ,5 4 4 3
t[`listFnOneItem;(asc@,5;deltas@,5;fills@,0N;sublist[2;,5];rotate[1;,5];xprev[1;,5];deltas `a!5);(,5;,5;,0N;,5;,5;,0N;(,`a)!,5)]   / a one-item list is not an atom
t[`listFnAtomArgs;{5#.[*x;1_x;{x}]}'((rotate;1.5;,5);(rotate;`a;,5);(sublist;`a;5);(sublist;0n;5);(sublist;0N;5);(asc;{x});(iasc;+);(xrank;2;{x}));8#,"'type"]   / a bad count, a function: 'type, as q
t[`listFnAtomFn;{.[*x;1_x;{5#x}]}'((flip;{x});(deltas;{x});(xprev;1;{x}));("'rank";{x};,-1)]   / as q: flip is 'rank, deltas returns it, xprev indexes it
t[`listFnDict;{(fills `a`b`c!0N 3 1;deltas x;ratios x;prev x;differ `a`b`c!1 1 2;sums `a`b`c!1 0N 2;mins `a`b`c!0N 3 1)}`a`b`c!3 1 4;(`a`b`c!0N 3 1;`a`b`c!3 -2 3;`a`b`c!3.0,(1%3),4.0;`a`b`c!0N 3 1;`a`b`c!101b;`a`b`c!1 1 3;`a`b`c!9223372036854775807 3 1)]   / digest #103: by value, as q; was by position: 0N 0N 0N, 0 0 4, 'length, ,0N
t[`listFnDictErr;{5#.[*x;1_x;{x}]}'((flip;`a`b`c!3 1 4);(next;`a`b!1 2);(rotate;1;`a`b`c!3 1 4);(xprev;1;`a`b!1 2);(rotate;1;(+(,`k)!,`x`y)!+(,`a)!,1 2));("'rank";"'type";"'type";"'type";"'type")]   / as q; was a table of one-row columns, ,0N, `b`c`a!1 4 3, 0N 0N, 'nyi
t[`listFnTable;{(fills x;sums x;mins x;flip `a`b!(1;2 3))}@+`a`b!(0N 3 1;1 0N 2);(+`a`b!(0N 3 1;1 1 2);+`a`b!(0 3 4;1 1 3);+`a`b!(9223372036854775807 3 1;1 1 1);+`a`b!(1 1;2 3))]   / by column, as q; fills was a dict of tables
t[`listFnKeyed;{(fills x;deltas fills x;differ x)}(+(,`k)!,`x`y`z)!+`a`b!(3 0N 4;1 1 2);((+(,`k)!,`x`y`z)!+`a`b!(3 3 4;1 1 2);(+(,`k)!,`x`y`z)!+`a`b!(3 0 1;1 0 1);(+(,`k)!,`x`y`z)!111b)]   / by the value table, as q; all were 'nyi
t[`listFnNullFill;{(prds `a`b`c!x;maxs `a`b`c!x;prds@+`a`b!(x;1 0N 2);maxs@+`a`b!(x;1 0N 2))}0N 3 1;(`a`b`c!1 3 3;`a`b`c!-9223372036854775807 3 3;+`a`b!(1 3 3;1 1 2);+`a`b!(-9223372036854775807 3 3;1 1 2))]   / nulls filled in a dict or table, as q; were 0N 0N 0N, 0N 3 3 (dict)
t[`listFnKeyedMore;{(ratios x;prev x;sums x;mins x;5#.[next;,x;{x}];5#.[xprev;(1;x);{x}])}(+(,`k)!,`x`y`z)!+`a`b!(3 0N 4;1 1 2);((+(,`k)!,`x`y`z)!+`a`b!(3.0 0n 0n;1.0 1.0 2.0);(+(,`k)!,`x`y`z)!+`a`b!(0N 3 0N;0N 1 1);(+(,`k)!,`x`y`z)!+`a`b!(3 3 7;1 2 4);(+(,`k)!,`x`y`z)!+`a`b!(3 3 3;1 1 1);"'type";"'type")]   / as q (xprev: q 'length); were 'length, 'nyi, 3 -9223372036854775805 .., 3 0N 0N, 'nyi, 'nyi

/ ---- ordering / set ----
t[`asc;asc 3 1 2;1 2 3]
t[`ascAttr;`at asc 3 1 2;`s]
t[`desc;desc 1 3 2;3 2 1]
t[`iasc;iasc 30 10 20;1 2 0]
t[`rank;rank 30 10 20;2 0 1]
t[`distinct;distinct 1 1 2 3 3;1 2 3]
t[`inMem;in[3 5;1 2 3 4];10b]
t[`inCharInt;*"\n"\.[in;(97;"abc");{x}];"'type"]   / a char and a number are 'type, as in q (issue #20, row 12)
t[`inSymInt;(in[``;0];except[``;0];in[`b`;0];in[0 5 1000000;`$("";"a")];inter[`$("";"a");0]);(00b;``;00b;000b;0#`)]   / a symbol is not a number: the null symbol is not 0 (digest #100: was 11b)
t[`except;except[1 2 3 4;2 4];1 3]
t[`inter;inter[1 2 3 4;2 4 6];2 4]
t[`inGeneric;((1 2;3) in (1 2;3 4);3 in (1 2;3 4);1 2 in (1 2;3);(1;`a) in (1 2;`a);("ab";"c") in ("ab";"cd"));(10b;0b;1b;0b;10b)]   / an item of lower rank than y's items is not found, as q (digest #53)
t[`interExceptGeneric;(inter[(1 2;3);(1 2;5 6)];inter[(1 2;3);(3;5)];except[(1 2;3);(1 2;3 4)];except[(1 2;3);(3;5)]);(,1 2;,3;,3;,1 2)]   / each item of x is one item
t[`interExceptOneItem;{$[`e~*r:.[x;y;{(`e;*"\n"\x)}];r 1;r]}'[(except;inter;except;except;except;except);((1 2 3;(1 2;3));(1 2;(1 2;3));("ab";("ab";"cd"));((1;"a");("ab";"cd"));((1 2;3);,(1 2;3));((1;"a");("ab";1)))];6#,"'type"]   / x one item of y: 'type, as q (where 1b)
t[`interExceptWholeItem;(except[(1 2;3 4);(1 2;(1 2;3 4))];inter[(1 2;3 4);(1 2;(1 2;3 4))]);(,3 4;,1 2)]   / x found whole (#29) still goes item by item, as q
t[`interExceptTypedY;(except[(1 2;3 4);1 2 3];inter[(1 2;3);1 2 3]);((1 2;3 4);,3)]   / a general x against a typed y: item by item too (2.7.2: nested garbage; q: 'type)
t[`inTableRows;{$[`C~@r:.[in;x;{x}];*"\n"\r;r]}'((+`a`b!(1 2;3 4);+`a`b!(1 5;3 6));(+`a`b!(1 2;3 4);+`b`a!(3 4;1 2));(`a`b!1 3;+`a`b!(1 2;3 4));(`b`a!3 1;+`a`b!(1 2;3 4));(`a`b!1 4;+`a`b!(1 2;3 4));(+(,`b)!,3 9;(+(,`a)!,1 2)!+(,`b)!,3 4);(+`a`b!(1 2;3 4);+`a`c!(1 2;3 4));(1;`a`b!1 2);(5;`a`b!1 2));(10b;11b;1b;1b;0b;10b;"'mismatch";1b;0b)]   / rows by column name; a dict y: its values, as q (digest #101: was 'nyi)
t[`exceptInterTable;(except[(1;"a");`x`y!1 2];except[1 2 3;`a`b!"ab"];except[+`a`b!(1 2;3 4);+`a`b!(1 5;3 6)];inter[+`a`b!(1 2;3 4);+`a`b!(1 5;3 6)];except[1 2 3;`a`b!1 2];inter[1 2 3;`a`b!1 2];except[(1;`a);`a`b!(1;`x)]);(,"a";1 2 3;+`a`b!(,2;,4);+`a`b!(,1;,3);,3;1 2;,`a)]   / as q (digest #101: was 'nyi)
t[`exceptInterEmptyGeneric;(except[(1 2;3 4);(1 2;3 4)];except[("ab";"cd");("ab";"cd")];except[(1;`a);(1;`a)];inter[(1 2;3);(5 6;7)];inter[(1 2;3 4);(5 6;7 8)];except[1 2;1 2];except[`a`b;`a`b];except[2024.01.01 2024.01.02;2024.01.01 2024.01.02]);(();();();();();!0;0#`;())]   / none of a general list is (), as q; a typed one keeps its type (digest #102: was 0#,0N 0N)
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
/ Digest #45: a column t lacks takes kt's values as they are; key columns must agree in type
t[`pjNewColKinds;{(+pj[+`k`a!(1 2 3;10 20 30);xkey[`k;+`k`c!(1 3;x)]])`c}'("xy";(10 20;30 40);`p`q);("x",(`c$0),"y";(10 20;!0;30 40);`p``q)]
t[`pjKeyType;.[pj;(+`k`a!(1 2 3;10 20 30);xkey[`k;+`k`b!(`x`y;5 6)]);{5#x}];"'type"]
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

t[`ajKeepsOwn;+aj[,`time;+`sym`time`px!(`a`b`c;0 5 9;1 2 3);+`sym`time`bid!(`x`y;3 7;10 20)];`sym`time`px`bid!(`a`x`y;0 5 9;1 2 3;0N 10 20)]
t[`aj0KeepsOwn;+aj0[`sym`time;+`sym`time`px!(`a`a;0 5;1 2);+`sym`time`bid!(,`a;,3;,10)];`sym`time`px`bid!(`a`a;0 3;1 2;0N 10)]
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
t[`wjFloat;(+wj1[(twF-0.15;twF+0.15);`sym`time;+`sym`time!(`a`a;twF);qwF;,(`c;count;`bid)])`c;2 1]
t[`wjFloatFallback;(+wj1[(twF-0.15;twF+0.15);`sym`time;+`sym`time!(`a`a;twF);qwF;,(`s;{sum x};`bid)])`s;30 30]
t[`wjNullLowFallback;(+wj1[(0N 4 7;1 5 9);`sym`time;+`sym`time!(`a`a`a;1 5 9);+`sym`time`bid!(`a`a`a`a;0 4 5 8;10 20 30 40);,(`s;{sum x};`bid)])`s;10 50 40]   / A null lower bound starts at the first quote, as the kernel does
t[`wjIntTimesFloatWindow;(+wj1[(,4.5;,5.5);`sym`time;+`sym`time!(,`a;,5);+`sym`time`bid!(`a`a`a;3 4 5;10 20 30);,(`s;{sum x};`bid)])`s;,30]   / A float bound over int times, as the kernel
t[`wjNullLowNaNQuote;(+wj[(,0N;,1.5);`sym`time;+`sym`time!(,`a;,1.5);+`sym`time`bid!(`a`a`a;0n 1.0 2.0;10 100 1000);,(`s;{sum x};`bid)])`s;,110]   / a null lower bound over float times with a NaN quote, as the kernel
t[`ajCharHigh;(+aj[`sym`time;+`sym`time!(`a`a`a;`c$66 201 64);+`sym`time`bid!(`a`a;`c$65 200;10 20)])`bid;10 20 0N]   / char times order as unsigned bytes, as in q (issue #17)
t[`ajCharQuoteHigh;(+aj[`sym`time;+`sym`time!(`a`a;`c$66 71);+`sym`time`bid!(`a`a`a;`c$65 70 200;10 20 30)])`bid;10 20]
t[`ajCharHighRev;(+aj[`sym`time;+`sym`time!(`a`a`a;`c$66 201 64);+`sym`time`bid!(`a`a;`c$200 65;20 10)])`bid;10 20 0N]   / the same quotes in the other order
/ Digest #68: wj counts the quote in force at the window's start, as q's wj (the tests around use wj1, the old window)
WQ8:+`sym`time`p!(5#`a;1 3 5 7 9;10 20 30 40 50);WT8:+`sym`time!(,`a;,5)
WF8:{[j;a;b]r:+j[(,a;,b);`sym`time;WT8;WQ8;((`n;#:;`p);(`s;sum;`p))];(r[`n]0;r[`s]0)}
t[`wjPrevailing;(WF8[wj]'[4 3 0 10 0;6 6 6 12 0];WF8[wj1]'[4 3 0 10 0;6 6 6 12 0]);((2 50;2 50;3 60;1 50;0 0);(1 30;2 50;3 60;0 0;0 0))]   / q's answers
t[`wjPrevailingRun;{[j](+j[(,3;,6);`sym`time;WT8;+`sym`time`p!(5#`a;1 3 3 5 9;10 20 25 30 50);,(`s;{+/x};`p)])[`s]0}'(wj;wj1);55 75]   / A run at the start time: wj from its last
/ a window ending before the quote in force at its start is empty in wj, kernel and K alike: q's answers (q's wj1 is 'domain on (6;2) and (5;2))
WQ9:+`sym`time`p!(`a`a`b`b;0 5 1 4;10 20 1 2);WT9:+`sym`time!(`a`a`a`a`a`b`b;7#0);WW9:(6 6 3 -3 0 5 5;2 5 1 -1 5 2 4)
WF9:{[j;a]r:+j[WW9;`sym`time;WT9;WQ9;a];r`n`s}
t[`wjEmptyWindow;(WF9[wj;((`n;count;`p);(`s;sum;`p))];WF9[wj;((`n;#:;`p);(`s;{+/x};`p))];WF9[wj1;((`n;count;`p);(`s;sum;`p))]);((0 1 1 0 2 0 1;0 20 10 0 30 0 2);(0 1 1 0 2 0 1;0 20 10 0 30 0 2);(0 0 0 0 2 0 0;0 0 0 0 30 0 0))]
WT10:+`sym`time!(`a`b`a`b`a`b`a`b`a;!9);WW10:(0 2 3 5 6 5 6 6 7;-1 0 1 2 2 4 5 6 9)
WF10:{[j;a]r:+j[WW10;`sym`time;WT10;WQ9;a];r`n`s}
t[`wjEmptyWindowSorted;(WF10[wj;((`n;count;`p);(`s;sum;`p))];WF10[wj;((`n;#:;`p);(`s;{+/x};`p))]);2#,(0 0 1 0 0 1 1 1 1;0 0 10 0 0 2 20 2 20)]   / time-ordered rows with inverted windows, through the kernel's cursor cache: kernel and K give q's answers
t[`wjCharHigh;{q:+`sym`time`bid!(`a`a`a;`c$65 200 210;10 20 30);t:+`sym`time!(`a`a;`c$66 201);w:(`c$64 199;t`time);((+wj1[w;`sym`time;t;q;,(`mx;max;`bid)])`mx;(+wj1[w;`sym`time;t;q;,(`n;count;`bid)])`n)}[];(10 20;1 1)]   / wj's kernel reads char times unsigned too (as q's wj1)
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
t[`xgroupKeyed;xgroup[`k;+`k`a!(1 2 1;10 20 30)];(+(,`k)!,1 2)!+(,`a)!,(10 30;,20)]   / Digest #57: keyed, as q
/ Digest #52: a column the table lacks is an error named after it
t[`tableFnMissing;{.[x;(`a;+`k`v!(-1 -2;3 4));{2#x}]}'(xasc;xdesc;xkey);3#,"'a"]

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
/ hh is the hour of day, as q's `hh$ (issue #62; it gave all the hours, 100 for 100 hours): a negative time keeps its sign
t[`hhOfDay;hh'(24:00:00.000;23:59:59.999;100:00:00.000;596:31:23.647);0 23 4 20]
t[`hhNeg;hh'0-(01:00:00.000;00:30:00.000;25:30:00.000);-1 0 -1]
t[`hhInts;(hh 3600000 90000000 -3600000 0N;hh 0N;hh 1.5e7);(1 1 -1 0N;0N;4)]
t[`hhVecPaths;(hh 0 3600000 86399999;hh 0 3600000 90000000;hh 0N 3600000;hh 0N 3600000 90000000 0N;hh 3600000 -3600000;hh 0N -3600000;hh 0n 3.6e6);(0 1 23;0 1 1;0N 1;0N 1 1 0N;1 -1;0N -1;0N 1)]
t[`hhNested;(hh (0N 0;0);hh (0N -3600000;-90000000);hh `a`b!(0N -3600000;360000000));((0N 0;0);(0N -1;-1);`a`b!(0N -1;4))]   / a list of lists and a dict keep their shape
t[`thhOfDay;thh (100:00:00.000;0-01:00:00.000;10:35:05.000);4 -1 10]
t[`mm;mm hms[9;30;15];30]
t[`sec;sec hms[9;30;15];15]
/ mm and sec of a negative time keep its sign, as q's `mm$ and `ss$ and as hh (a follow-up to #62; mm -01:30 was 30, sec -01:30:15 was 45); a null stays null
t[`mmSecNeg;(mm 0-01:30:00.000;sec 0-01:30:15.000;mm 0-25:59:59.999;sec 0-25:59:59.999;mm 0-00:00:15.000;sec 0-00:00:00.500);-30 -15 -59 -59 0 0]
t[`mmSecInts;(mm -361815000 -5415123 361815000 0N;sec -361815000 -5415123 361815000 0N;mm 0N;sec 0N;mm 361815000;tmm 0-01:30:00.000;tss 0-01:30:15.000);(-30 -30 30 0N;-15 -15 15 0N;0N;0N;30;-30;-15)]
t[`mmSecNested;(mm (-5415123 0;0);sec (-5415123 0;0);mm `a`b!(-5415123;0);mm `a`b!(-5415123 0;0);mm (0N 0;0);hh (0N 0;0);hh `a`b!(0N -5415123;0));((-30 0;0);(-15 0;0);`a`b!-30 0;`a`b!(-30 0;0);(0N 0;0);(0N 0;0);`a`b!(0N -1;0))]   / lists of lists and dicts, item by item
t[`mmSecFrac;(mm -0.5;sec -0.5;mm -0.5 60000.5;sec -1500.5;hh -0.5);(0;0;0 1;-1;0)]   / a fraction of a ms below 0 is no minute, not 59
t[`mmSecRagged;(mm (5415123 0;,0);mm (5415123 0;-5415123 0 1);mm `a`b!(5415123 0;,0);sec (5415123 0;,0N));((30 0;,0);(30 0;-30 0 0);`a`b!(30 0;,0);(15 0;,0N))]   / lists of lists of any lengths
t[`timeAccessorsFast;(mm 5000000000000 -5000000000000 0N 90000;sec 5000000000000 -5000000000000 0N 1500;mm 00:01:00.000;sec 0-00:00:01.000;year (2026.01.01D23:00;2027.01.01D01:00);dow (2026.03.04;2026.03.05);.[year;,(01:00:00.000;02:00:00.000);{(x?"\n")#x}];year 2026.01.01;dow 2026.03.04;thh 0-01:00:00.000;tmm 01:30:00.000 02:45:00.000);(53 -53 0N 1;20 -20 0N 1;1;-1;2026 2027;`Wed`Thu;"'type";2026;`Wed;-1;30 45)]   / the paths in C (`mm `sec `tjn) and the date atom's: as amber.k's code
/ the time accessors take a general list of times, as a strand of times is (digest #65; they were 'type, in tnum0)
t[`timeAccessorsList;(hh 01:00:00.000 02:30:00.000;mm 01:00:00.000 02:30:00.000;sec 01:00:00.000 02:30:15.000;hh (01:00:00.000;02:30:00.000);mm 0-(01:30:00.000;00:30:00.000);milli 01:00:00.123 02:30:00.456;minute 01:00:00.000 02:30:00.000;second 01:00:00.000 02:30:00.000);(1 2;0 30;0 15;1 2;-30 -30;123 456;60 150;3600 9000)]
t[`timeAccessorsMixed;(.[milli;,(01:00:00.000;2026.01.01);{(x?"\n")#x}];.[hh;,(01:00:00.000;3600000);{(x?"\n")#x}]);2#,"'type"]   / one kind only: times with dates or ints stay 'type
tavc:0   / counts calls of the lambdas in timeAccessorsVerbs
t[`timeAccessorsVerbs;(.[hh;,(+;-);{(x?"\n")#x}];.[milli;,(+;-);{(x?"\n")#x}];.[second;,(::;-:);{(x?"\n")#x}];.[mm;,(+);{(x?"\n")#x}];.[mm;,({x};{y});{(x?"\n")#x}];.[mm;,(+;+);{(x?"\n")#x}];.[hh;,({tavc+:1;x};{tavc+:1;x});{(x?"\n")#x}]);7#,"'type"]   / `u and `v are verb types in Amber, not q's minute and second
t[`timeAccessorsVerbsUncalled;tavc;0]   / hh never calls the lambdas above
t[`timeAccessorsNested;(hh `a`b!01:00:00.000 02:30:00.000;mm (01:30:00.000;(02:00:00.000;03:00:00.000));hh `a`b!(01:00:00.000;(02:00:00.000;03:00:00.000));mm 0#,(::);hh 0#,(::);.[mm;,(01:30:00.000;(02:00:00.000;3600000));{(x?"\n")#x}]);(`a`b!1 2;(30;0 0);`a`b!(1;2 3);!0;!0;"'type")]   / dicts and lists of lists item by item, as q; an empty list is !0
t[`timeAccessorsDeep;(mm (01:30:00.000 02:00:00.000;,03:00:00.000);.[milli;,(01:00:00.000;(2026.01.01;2026.01.02));{(x?"\n")#x}];.[hh;,`a`b!(01:00:00.000;(2026.01.01;2026.01.02));{(x?"\n")#x}]);((30 0;,0);"'type";"'type")]   / lists of any lengths; one kind at every depth
t[`timeAccessorsDates;(milli 2026.01.01 2026.02.01;sec 2026.01.01 2026.02.01;minute 2026.01.01 2026.02.01;second 2026.01.01 2026.02.01;mm `a`b!(2026.01.01;2026.02.01);hh 2026.01.01 2026.02.01;milli 2026.01.01);(497 528;9 9;0 0;9 9;`a`b!0 0;0 0;497)]   / a list of dates is read as its days, as a date atom is (q: 'hh, 'ss; `mm$ gives the month)
/ the date accessors take dates, timestamps and raw days only (digest #104): an int 0N is the null day, as q's `year$0Nd is 0Ni
/ (year 0N was -25252734927764555, dow 0N `Fri); a time, a float, text or a list of dates and ints is 'type, as q's `year$
t[`dateAccessorsNull;(year 0N;month 0N;day 0N;dow 0N;year 0 0N 366;dow 0 0N 1;dparts 0N;month `a`b!0N 59);(0N;0N;0N;`;2000 0N 2001;`Sat``Sun;0N 0N 0N;`a`b!0N 2)]
t[`dateAccessorsType;(.[year;,(2026.01.01;5);{(x?"\n")#x}];.[year;,01:00:00.000;{(x?"\n")#x}];.[month;,1.5;{(x?"\n")#x}];.[dow;,`a;{(x?"\n")#x}];.[day;,"abc";{(x?"\n")#x}];.[year;,(2026.01.01;01:00:00.000);{(x?"\n")#x}];.[dparts;,(1 2;2026.01.01);{(x?"\n")#x}]);7#,"'type"]
t[`dateAccessorsMixed;(year (2026.01.01;2026.01.01D23:00:00);dow (2026.03.04;2026.03.04D23:00:00);day ((2026.03.04;2026.01.01);`a`b!(2026.03.05;2026.03.06));year ());(2026 2026;`Wed`Wed;(4 1;`a`b!5 6);!0)]   / dates with timestamps, item by item
/ in a list of dates or timestamps an int 0N or :: is the null day: D,0N makes one, prev, next and indexing out of range put :: in
/ (they were 'type with the check, wrong years on 2.7.2; q 0N 2026 and 2026 0N); a list of ints with :: is raw days
DAD:2026.01.01 2027.01.01
t[`dateAccessorsNullItems;(year DAD,0N;year prev DAD;year DAD@0 5;dow next DAD;month next 2026.03.04D01:00 2027.05.06D01:00;dow `a`b!(2026.03.04;0N);year ((2026.01.01;0N);::);year (0N;::);year (1 2;::);year DAD@5 6;dow DAD@5 6;year (::;::);year `a`b!(::;::));(2026 2027 0N;0N 2026;2026 0N;`Fri`;5 0N;`a`b!`Wed`;(2026 0N;0N);0N 0N;(2000 2000;0N);0N 0N;``;0N 0N;`a`b!0N 0N)]
/ a table goes column by column, a keyed table on its value, as q's `year$ (a table was 'type with the check, a keyed table 'nyi on 2.7.2)
DAT:+`d`p!(DAD;2026.01.01D01:00 2027.03.01D01:00)
t[`dateAccessorsTables;(year DAT;month DAT;dow DAT;year 0#DAT;.[year;,+`d`x!(DAD;1.5 2.5);{(x?"\n")#x}]);(+`d`p!(2026 2027;2026 2027);+`d`p!(1 1;1 3);+`d`p!(`Thu`Fri;`Thu`Mon);+`d`p!(!0;!0);"'type")]
t[`dateAccessorsKeyed;(.[year;,(+(,`k)!,1 2)!DAT;{(x?"\n")#x}];.[day;,(+(,`k)!,1 2)!+(,`d)!,DAD;{(x?"\n")#x}]);((+(,`k)!,1 2)!+`d`p!(2026 2027;2026 2027);(+(,`k)!,1 2)!+(,`d)!,1 1)]   / the key is kept, as q's
t[`minute;minute hms[9;30;15];570]
t[`second;second hms[9;30;15];34215]
t[`milli;milli hms[9;30;15]+123;123]
t[`stime;stime hms[9;30;15];"09:30:15.000"]
t[`ptime;ptime"09:30:15.123";34215123]
t[`stimeNeg;stime -1800000 -1 1800000;("-00:30:00.000";"-00:00:00.001";"00:30:00.000")]
/ every digit of the hours, so a time of 100 hours or more reads back as itself (issue #62; it printed the last two)
t[`stimeHours;stime 0 3600000 356400000 360000000 2147483647 -2147483647;("00:00:00.000";"01:00:00.000";"99:00:00.000";"100:00:00.000";"596:31:23.647";"-596:31:23.647")]
t[`timeHoursText;($100:00:00.000;`k 50:00:00.000+50:00:00.000;$0-100:00:00.000);("100:00:00.000";"100:00:00.000";"-100:00:00.000")]
t[`timeHoursRead;({x=. $x}'(100:00:00.000;596:31:23.647;99:59:59.999);`i$"T"$$123:45:06.789);(1 1 1;445506789)]
t[`timeHoursGrid;tcell[`time;3600000 360000000];("01:00:00.000";"100:00:00.000")]
t[`timeHoursAtoms;stime (01:00:00.000;100:00:00.000;3600000);("01:00:00.000";"100:00:00.000";"01:00:00.000")]   / a list of time atoms, as tsym gives
t[`stimeMidnightAtom;stime (00:00:00.000;01:00:00.000);("00:00:00.000";"01:00:00.000")]   / midnight is not null (issue #62; it printed as 0N)
t[`ptimeNeg;(ptime"-00:30:00.000";ptime"-01:00:00");-1800000 -3600000]
t[`ptimeList;*"\n"\.[{"T"$("09:30:00.100";"10:00:00.000")};,0;{x}];"'type"]   / a list of strings is 'type, as before (the sign test must not recurse)
t[`timeNegText;`i$"T"$"-00:30:00.000";-1800000]
/ "T"$ of a time past 32 bits of milliseconds is 'limit, as the literal (issue #62; it wrapped)
t[`timeTextLimit;(.[{"T"$x};,"1000:00:00.000";{(x?"\n")#x}];.[{"T"$x};,"596:31:23.648";{(x?"\n")#x}];`i$"T"$"-596:31:23.648";.[ptime;,"1000:00:00.000";{(x?"\n")#x}]);("'limit";"'limit";-2147483648;"'limit")]
t[`ptimeRound;(ptime stime hms[13;45;9])=hms[13;45;9];1]
t[`ptextAsCasts;(ptime"-01:00";pdate"2024.02.29";ptstamp"2026.07.30D09:30:00.123456789";.[pdate;,"2026.02.29";{(x?"\n")#x}]);(-3600000;8825;838719000123456789;"'parse")]   / as "T"$, "D"$ and "P"$ read them (issue #62)
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

/ ---- asc / desc of a dict: the keys go with their values ----
t[`ascDict;        asc `c`a`b!3 1 2;                 `a`b`c!1 2 3]
t[`descDict;       desc `c`a`b!3 1 2;                `c`b`a!3 2 1]
t[`ascDictRepeat;  asc `a`a`b!3 1 2;                 `a`b`a!1 2 3]
t[`descDictTies;   desc `a`b`c`d!1 2 1 2;            `b`d`a`c!2 2 1 1]
t[`ascKeyed;       asc ([k:`x`y`z]v:3 1 2);          ([k:`y`z`x]v:1 2 3)]

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
t[`vsBase89;     (vs[2 8;5];2 9 vs 5);  (0 5;0 5)]
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
t[`wjStamps;       ((+wj1[AJW;`ts;AJWT;AJWQ;,(`c;count;`q)])`c;(+wj1[AJW;`ts;AJWT;AJWQ;,(`s;{+/x};`q)])`s);  (2 1;11 4)]   / {+/x}: wj's pure-K fallback
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
/ ej: null keys, int keys of mixed widths, an empty x; xasc of mixed temporals beside a bool column
t[`inBits;(0101B in 1;(0101B;1) in 1);(0101b;(0101b;1b))]   / Digest #7
t[`ejNullKeys;ej[`s;+`s`n!(``a`;1 2 3);+`s`v!(`a``;10 20 30)];+`s`n`v!(```a``;1 1 2 3 3;20 30 10 20 30)]
t[`ejMixedWidths;ej[`k;+`k`n!(1 2 300;1 2 3);+`k`v!(`i$300 1 7;10 20 30)];+`k`n`v!(1 300;1 3;20 10)]
t[`ejEmptyX;{(!+x;@'(+x)`s`n`v)}ej[`s;+`s`n!(0#`;0#0);+`s`v!(`a`b;1 2)];(`s`n`v;`S`I`I)]
t[`xascTemporalBool;(+`t`b xasc +`t`b`v!((2026.07.30;1;0.5;2026.07.29;2026.07.30D10:00:00.000;0N);101010b;!6))`v;3 0 2 5 1 4]   / As < orders the column (digest #58; it was the rows' order)
/ Digest #58: a generic column mixing temporals with numbers orders as < (>) on it, with one key or two
XG:(2026.07.30D10:00:00.000;0N;1.5;2026.07.30;2026.07.30D09:59:59.999999999;0n;5;-0.0;0.0;2026.07.29);XT:+`g`v!(XG;9 8 7 6 5 4 3 2 1 0)
t[`xascGeneric;((+xasc[`g;XT])`v;(+xdesc[`g;XT])`v;(+xasc[`v`g;XT])`v);(XT[`v]@<XG;XT[`v]@>XG;!10)]
t[`ajsortedStamps; (ajsorted[`ts;AJQ];ajsorted[`ts;+`ts`q!(|AJQ`ts;6 5)]);  1 0]   / sorted temporal quotes are not sorted again
/ a range (!n, a+!n) rises, so ajsorted takes it as sorted and the joins no longer sort the quotes first; the
/ wj kernel then reads a range value column as its items, not as its two ends (q has no ranges: the answers
/ are q's for the same columns as lists, wj's q's wj1, whose windows Amber's wj takes)
t[`ajsortedRange;  ((+aj[`g`time;+`g`time!(!3;!3);+`g`time`b!(!4;!4;10+!4)])`b;ajsorted[`s`time;+`s`time!(`a`a`b`b;!4)];ajsorted[`time;+(,`time)!,3+!4]);  (10 11 12;1b;1b)]
WQ:+`time`q`r!(1 1 1 1 1 1 1 1 3 3 3 3 4;!13;1000000000000+!13)
WW:(1 1 -1 -1 1 -1 -1 1 -1 -1 -1 2 -1;3 3 1 1 3 1 1 3 1 1 1 4 1)
WT:+`time`v!(2 2 0 0 2 0 0 2 0 0 0 3 0;!13)
t[`wjRangeColumn;  (+wj1[WW;`time;WT;WQ;((`c;count;`q);(`z;sum;`q);(`f;first;`q);(`l;last;`r);(`m;max;`r);(`a;avg;`q))])`c`z`f`l`m`a;  (12 12 8 8 12 8 8 12 8 8 8 5 8;66 66 28 28 66 28 28 66 28 28 28 50 28;0 0 0 0 0 0 0 0 0 0 0 8 0;1000000000000+11 11 7 7 11 7 7 11 7 7 7 12 7;1000000000000+11 11 7 7 11 7 7 11 7 7 7 12 7;5.5 5.5 3.5 3.5 5.5 3.5 3.5 5.5 3.5 3.5 3.5 10 3.5)]
t[`wjRangeQuotes;  (+wj[(-1+!4;1+!4);`s`time;+`s`time!(`a`a`a`a;!4);+`s`time`v!(13#`a;!13;!13);((`z;sum;`v);(`f;first;`v);(`m;min;`v))])`z`f`m;  (1 3 6 9;0 0 1 2;0 0 1 2)]
/ wj's windows are as long as the trade table, as in q ('length; an empty trade table was read past its end)
t[`wjWindowLength; {*"\n"\.[wj;x;{x}]}'((WW;`time;0#WT;WQ;,(`z;sum;`q));((0 1;2 3);`time;3#WT;WQ;,(`z;sum;`q));((0 1 2;2 3);`time;3#WT;WQ;,(`z;sum;`q));((0 1;2 3);`time;3#WT;WQ;,(`z;{+/x};`q)));  4#,"'length"]
/ int folds, scans and amends wrap (computed in unsigned arithmetic: a signed overflow in C is undefined)
t[`scanWrap;       (*\0N 0N;+\9223372036854775807 1;*\4294967296 4294967296;9223372036854775806+\1 1);(0N 0;9223372036854775807 0N;4294967296 0;9223372036854775807 0N)]
t[`foldSeedWrap;   (9223372036854775807+/1 2;0N*/0N 3);(-9223372036854775806;0)]
t[`amendWrap;      ({x[0 1]*:-9223372036854775807;x}3#-1099511627776;@[2#9223372036854775807;0 0;+;1];{x[0 1]+:1;x}2#9223372036854775807);(3#-1099511627776;-9223372036854775807 9223372036854775807;0N 0N)]
/ whole-number floats beyond 2^53 are not counting-sorted (their (L) is undefined past 2^63)
t[`sortBigFloats;  {(<x;>x;x@<x)}1e19 1.0000000000000004e19 1e19 1.0000000000000002e19;(0 2 3 1;1 3 0 2;1e19 1e19 1.0000000000000002e19 1.0000000000000004e19)]
t[`gradeBigNeg;    <-1e19 -1.0000000000000004e19 -1e19;1 0 2]
/ raze of dicts joins them into a dict, as , and {x,y}/ do
t[`razeDicts;      (,/(`a`b!1 2;`c`d!3 4);,/(1 2!3 4;5 6!7 8));(`a`b`c`d!1 2 3 4;1 2 5 6!3 4 7 8)]
/ :: in a dict amend is every key, as when indexing
t[`amendDictAll;   (.[`a`b!(1 2;3 4);(::;0);:;9];@[`a`b!1 2;::;+;9];.[`x`y!(`p`q!1 2;`p`q!3 4);(::;`q);*;10]);(`a`b!(9 2;9 4);`a`b!10 11;`x`y!(`p`q!1 20;`p`q!3 40))]
/ a dict as the value through :: on a dict: its values go to the keys in order, as for d[!d] (q matches by key); was a crash
t[`amendDictAllDictValue;(.[+`a`b!(1 2 3;4 5 6);(0;::);:;`a`b!9 8];{t:+`a`b!(1 2 3;4 5 6);t[0;]:`a`b!9 8;t}[];.[`a`b!(`x`y!1 2;`x`y!3 4);(`a;::);:;`x`y!9 8]);(+`a`b!(9 2 3;8 5 6);+`a`b!(9 2 3;8 5 6);`a`b!+`x`y!(9 3;8 4))]
t[`amendDictAllDictNear;(.[+`a`b!(1 2 3;4 5 6);(0;::);:;7];.[+`a`b!(1 2 3;4 5 6);(0;::);+;`a`b!10 20];.[(`a`b!1 4;`a`b!2 5);(0;::);:;`a`b!9 8];.[`x`y!(`p`q!1 2;`p`q!3 4);(::;`q);:;`x`y!10 20];{d:`a`b!(`x`y!1 2;`x`y!3 4);d[`a;]:`x`y!9 8;d}[]);(+`a`b!(7 2 3;7 5 6);+`a`b!(11 2 3;24 5 6);+`a`b!(9 2;8 5);`x`y!+`p`q!(1 3;10 20);`a`b!+`x`y!(9 3;8 4))]
t[`amendDictAllKeyed;*"\n"\.[{.[x;(1;::);:;`a`b!9 8]};,([k:1 2 3]a:4 5 6;b:7 8 9);{x}];"'nyi"]   / a keyed table's row: not yet, as before

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
/ one lane (AMBER_THREADS=1) is a plain serial each, where a global may be set (digest #79)
PS:(,"1")~(`env[])@`$"AMBER_THREADS"
PG:0
t[`peachNoupdate;   .[{`pe({PG::x;x};!50)};,0;{`noupdate}];          $[PS;!50;`noupdate]]
t[`peachGlobalKept; PG;                                              $[PS;49;0]]
/ JSON in peach: the reader's and the writer's positions are per thread (workers shared one, and read or wrote each other's text)
/ Digest #5: a worker recurses as deep as the main thread (its stack was the platform default, 512 KB on macOS)
PF:{$[x>0;1+o x-1;0]}
t[`peachDeep;`pe({PF x};4#2000);4#2000]
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
