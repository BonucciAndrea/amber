/ ============================================================================
/ tests/test_core.k  -  the core K verbs Amber inherits from ngn/k: reading
/ literals, arithmetic, lists, casts and printing.  GNU AGPLv3 - see LICENSE
/ and NOTICE.
/ ----------------------------------------------------------------------------
/     ./amber tests/test_core.k          (exits non-zero on any failure)
/ ----------------------------------------------------------------------------
/ Each section pins one behaviour, with the cases that used to go wrong. The
/ suite loads only the harness, not the q-style libraries, so it checks the
/ interpreter itself.
/ ============================================================================
DIR:{$[#p:"/"/-1_"/"\x;p;"."]}`argv 1
LOAD:{{. x^(`c$13)}'0:x;}
LOAD DIR,"/harness.k"

/ ---- multiplication widens when the product does not fit ----
t[`mulWidenNeg;(,-50)*-3;,150]
t[`mulWidenPos;(,100)*100;,10000]


/ ---- $[..] with a branch over 255 bytes jumps right (upstream ngn/k 2c4e817b) ----
condLong:{$[x=0;`a;x=1;x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x;x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x]}
t[`condLong;(condLong 0;condLong 1;condLong 2);(`a;111;42)]

/ ---- a dotted name of more than 8 parts is an error (upstream ngn/k aaa99ad7) ----
te[`dottedLimit;"a.b.c.d.e.f.g.h.i"]
tv[`dotted;"a.b.c:1;a.b.c";1]

/ ---- drop an index from a bit-boolean list (upstream ngn/k 02d24691) ----
t[`dropBits;(01B_1;0101B_2;01B_0);(,0;0 1 1;,1)]

/ ---- hex decoding rejects bytes above 127 (upstream ngn/k 387f90c0) ----
te[`hexHigh;"`hex?`c$200 200"]
t[`hexOk;`hex?"414a";"AJ"]

/ ---- joins over 2^32 characters are an error (upstream ngn/k 153a01cf) ----
te[`joinLimit;"s:1048576#\"a\";#\",\"/4096#,s"]
te[`joinLimitStr;"s:1048576#\"a\";#\"ab\"/4096#,s"]
t[`joinSmall;","/("ab";"c");"ab,c"]

/ ---- a long non-ASCII name fits its buffer (upstream ngn/k 60864276) ----
te[`longNameUnset;"фффффффффффффффффффффффффффффф 1"]
tv[`longName;"фффффффффффффффффффффффффффффф:3;фффффффффффффффффффффффффффффф+1";4]

/ ---- an n-ary scan with a count of 2^32 or more is an error (upstream ngn/k cde0b34c) ----
te[`scanCountLimit;"{x+y}\\[4294967296;1;2]"]
t[`scanCount;{x+y}\[3;1;2];1 2 3 5]

/ ---- int!float keeps the int (upstream ngn/k a0f39bc0) ----
n:1000000000000
tv[`modIntFloat;"n!2.5;n!2.5;n";1000000000000]
tv[`modIntFloatBig;"m:4611686018427387904;m!2.5;m!2.5;m!2.5;m";4611686018427387904]
t[`modIntFloatValue;1000000000000!2.5;2.5]


/ ---- a projection given more arguments than it takes is an error, not a buffer overflow ----
te[`projRank;"{[a;b;c;d;e;f;g;h]h}[1;;3][2;4;5;6;7;8;9;10]"]
t[`projFill;({[a;b;c;d;e;f;g;h]h}[1;;3][2;4;5;6;7;8];{x+y+z}[1;;3][2]);8 6]

/ ---- projections that fill every slot: 7- and 8-ary with two gaps (the copy check alone bounds them) ----
t[`proj7gaps;{[a;b;c;d;e;f;g]a+b+c+d+e+f+g}[;;3;4;5;6;7][1;2];28]
t[`proj7dot;{[a;b;c;d;e;f;g]a+b+c+d+e+f+g}[;;3;4;5;6;7] . 1 2;28]
t[`proj8gaps;{[a;b;c;d;e;f;g;h](a;b;c;d;e;f;g;h)}[;;3;4;5;6;7;8][1;2];1 2 3 4 5 6 7 8]
t[`proj8of7slots;{[a;b;c;d;e;f;g;h](a;b;c;d;e;f;g;h)}[1;;3;4;5;6;7][2;8];1 2 3 4 5 6 7 8]
te[`proj7over;"{[a;b;c;d;e;f;g]a+b+c+d+e+f+g}[;;3;4;5;6;7][1;2;3]"]
/ more arguments than 8 slots hold: 'rank, and the arguments are released once
P8:{[a;b;c;d;e;f;g;h]a}[1;;3;4;5;6;7;8]
PX:!4
te[`proj8over;"P8[PX;0]"]
/ the same call again: it would fail if the first had released the arguments
te[`proj8overAgain;"P8[PX;0]"]
t[`proj8overArgs;PX;!4]
P3:{[a;b;c](a;b;c)}[;(,1);(,2)]
PA:,"abc";PB:,"def"
t[`proj3over;.[P3;(PA;PB;PA;PB;PA;PB;PA);{`e}];`e]
t[`proj3overAgain;.[P3;(PA;PB;PA;PB;PA;PB;PA);{`e}];`e]
/ a new allocation, which reuses any memory released twice above
PZ:"hello"
t[`proj3overArgs;(PA;PB);(,"abc";,"def")]

/ ---- a projection with 8 slots called with one argument too many ----
/ The dyadic call copied the extra argument past its 8-slot buffer (ASan:
/ stack-buffer-overflow); it is 'rank, as the n-argument call is.
P1:{[a;b;c;d;e;f;g;h]a}[;2;3;4;5;6;7;8]
te[`proj8each;"1 2 P1'3 4"]
te[`proj8eachLeft;"1 P1\\:2 3"]
te[`proj8eachRight;"1 P1/:2 3"]
te[`proj8dot;"(P1;P1). 1 2"]
t[`proj8one;P1 9;9]


/ ---- the fused x@&m applies a function, as @ does ----
f:{x+1}
t[`compressFn;f@&1 0 1;1 3]
t[`compressSym;`k@&1 0 1;"0 2"]
t[`compressList;10 20 30@&1 0 1;10 30]
t[`compressFnSum;+/f@&1 0 1;4]
t[`compressFnCmp;f@&(1 2 3)>1;2 3]


/ ---- a function past 256 bytes of bytecode: last-use positions and the stack size kept in one byte ----
tv[`bcUses130;"f:. \"{\",(\"+\"/130#,\"x\"),\"}\";f 1";130]
tv[`bcTwoUses;"g:1;f:. \"{x+\",(,/63#,\"g+\"),\"1+x}\";f 1";66]
tv[`bcUnusedArg;"f:. \"{[x;y]x+1+\",(\"+\"/127#,\"x\"),\"+-x}\";f[1;0]";128]
tv[`bcCondUses;"A:\"+\"/64#,\"x\";f:. \"{$[x=0;\",A,\";x=1;\",A,\";1]}\";f 0";0]
te[`bcStackLimit;"I:{\";\"/(\"a\"),(x-1)#\"1\"};g:. \"{[a]((\",(I 96),\");\",(I 160),\")}\""]


/ ---- adding a key to a dict whose values or keys are a range ----
/ Pushing onto a range (!n, a+!n) looped forever.
t[`pushRangeVals;{x[`c]:5;x}`a`b!!2;`a`b`c!0 1 5]
t[`pushRangeJoin;(`a`b!3+!2),`c!5;`a`b`c!3 4 5]
t[`pushRangeKeys;{x[1]:`z;x}(7+!3)!!3;7 8 9 1!(0;1;2;`z)]
t[`pushRangeLambda;@[(-!2)!!2;,1;{x,1}];0 -1 1!(0;1;0N 1)]


/ ---- amending many items squeezes once, and the result is the same ----
/ The amend of values of another type at many indices squeezed the list after
/ every item, so it took time quadratic in the number of indices.
AN:3000
t[`amendManyMixed;@[(2*AN)#1;!AN;:;AN#2.0];(AN#2.0),AN#1]
t[`amendManyMixedType;@@[(2*AN)#1;!AN;:;AN#2.0];`A]
t[`amendManyUniform;@@[AN#1;!AN;:;AN#2.0];`F]
t[`amendManyAssign;{x[!AN]:AN#0.5;x}(2*AN)#1;(AN#0.5),AN#1]
t[`amendManyPlusUniform;@{x[!AN]+:0.5;x}AN#1;`F]
t[`amendManyInnerSqueeze;@*@[AN#,,1;!AN;{@[x;0;:;y]};AN#0.5];`F]
t[`amendManyInner;@[AN#,1 2;!AN;{@[x;0 1;:;y,y]};AN#0.5];AN#,0.5 0.5]
t[`amendManyToVector;@[(AN#1),`a;!AN+1;:;(AN+1)#2];(AN+1)#2]
t[`amendManyLastItem;@@[(AN#1),`a;AN;:;3];`I]
/ code the amend calls out to still sees squeezed lists
AG:{@@[(1;2.5);0;:;1.5]}
t[`amendManyElidedInner;@[3#,(1;2);(!3;::);{AG[]}];3#`F]
t[`amendManyTableInner;@[+`a`b!(1 2;3 4);(!2;::);{AG[]}];2#`F]
t[`amendManyDepth;@[AN#1;(!AN;!AN);:;(AN#0.5;AN#2)];AN#2]

/ ---- amending a dict at many keys finds them all at once ----
/ Each key was found on its own, by a linear search, so this was quadratic.
/ Keys not in the dict are added in order of first use.
t[`dictAmendPresent;@[`a`b`c!1 2 3;`c`a;:;5 6];`a`b`c!6 2 5]
t[`dictAmendRepeat;@[`a`b!1 2;`a`a`b;+;1];`a`b!3 3]
t[`dictAmendNew;@[`a`b!1 2;`c`d`c;:;3 4 5];`a`b`c`d!1 2 5 4]
t[`dictAmendNewAtom;@[`a`b!1 2;`c`a`c;+;1];`a`b`c!2 2 2]
t[`dictAmendNewJoin;@[`a`b!1 2;`a`c;,;1 2];`a`b`c!(1 1;2;,2)]
t[`dictAmendNewMixed;@[`a`b!1 2;`c`d;:;(0.5;`x)];`a`b`c`d!(1;2;0.5;`x)]
t[`dictAmendIntKeys;@[1 2 3!4 5 6;5 0 5;:;7 8 9];1 2 3 5 0!4 5 6 9 8]
t[`dictAmendEmpty;@[(0#`)!!0;`a`b`a;+;1];`a`b!2 1]
t[`dictAmendListKeys;@[(1 2;3 4)!5 6;(1 2;7 8);:;9 10];(1 2;3 4;7 8)!9 6 10]
t[`dictAmendFunc;@[`a`b!1 2;`b`c;{x,9}];`a`b`c!(1;2 9;0N 9)]
t[`dictAmendRangeKeys;@[(!3)!3#1;3 4 3;+;1 2 3];0 1 2 3 4!1 1 1 4 2]
t[`dictAmendRangeVals;@[`a`b!1 2;`c`d`a;:;!3];`a`b`c`d!2 2 0 1]
t[`dictJoinMany;((!AN)!AN#1),(AN+!AN)!AN#2.0;(!2*AN)!(AN#1),AN#2.0]
t[`dictAssignMany;{x[AN+!AN]:AN#2.0;x}(!AN)!AN#1;(!2*AN)!(AN#1),AN#2.0]
t[`dictCountMany;#@[(!0)!!0;(2*AN)#!AN;+;1];AN]
te[`dictAmendLength;"@[`a`b!1 2;`c`d;:;1 2 3]"]

/ ---- temporal atoms: list verbs treat them as atoms; with a list, arithmetic goes item by item ----
/ (the bare interpreter cannot print a date, so `i$ shows the day counts)
t[`dateTake;(`i$3#2026.07.30;`i$1#2026.07.30;`i$-2#10:00:05.000;`i$2#2026.07.30D09:30:00.000000001);(9707 9707 9707;,9707;36005000 36005000;838719000000000001 838719000000000001)]
t[`dateAtomVerbs;(`i$|2026.07.30;`i$*2026.07.30;#2026.07.30);9707 9707 1]
t[`dateGrade;*"\n"\.[.:;,"<2026.07.30";{x}];"'type"]   / open of a temporal atom (monadic <) crashed
t[`dateDrop;*"\n"\.[.:;,"2_2026.07.30";{x}];"'rank"]   / drop from an atom crashed; 'rank as for an int atom
t[`dateIndexOut;((2026.07.30;2026.07.31)5;(`a`b!(2026.07.30;2026.07.31))`c)~(::;::);1]   / out of range: the null of an atom, :: (it was ,::)
t[`datePlusList;(`i$2026.07.30+!3;`i$(!3)+2026.07.30;`i$10:00:05.000+0 1 2;`i$2026.07.30-0 1);(9707 9708 9709;9707 9708 9709;36005000 36005001 36005002;9707 9706)]
t[`datePlusAtom;(`i$2026.07.30+1;2026.07.31-2026.07.30);9708 1]


/ ---- fill and binary search with a temporal atom on the right are 'type, not a crash ----
te[`fillTemporal;"0N^2026.07.30"]
te[`fillTemporalSym;"`a^10:00:05.000"]
te[`fillTemporalList;"0N^(2026.07.30;1)"]
te[`binTemporal;"(1 2 3)'2026.07.30"]
te[`binTemporalFloat;"1.0 2.0'2026.07.30"]
te[`binTemporalGeneric;"(1 2 3)'(2026.07.30;1)"]

/ a temporal atom on the left of fill is 'type too (it was 'nyi), not a fill
t[`fillTemporalLeft;{.[{x^y};(x;y);{(x?"\n")#x}]}[2026.07.30;0N 1];"'type"]

/ ---- the int null converts to the float null (0n), never to a number ----
t[`nullToFloatAdd;(0N+1.5;1.5+0N;1.5-0N;0N*2.0);4#0n]
t[`nullToFloatDiv;(0N%2;2%0N);0n 0n]
t[`nullToFloatDivVec;(0N%2 3;2 3%0N);(0n 0n;0n 0n)]
t[`nullToFloatCast;`f$0N;0n]
t[`nullToFloatEq;0N=0n;1b]
t[`nullToFloatMin;(0N&1.5;0N|1.5);(0n;1.5)]
t[`nullToFloatSum;+/(0N;1.5);0n]
t[`nullToFloatVecOk;0N 1+1.5;0n 2.5]

hexpect[100]
hreport"AMBER CORE"
$[FAILN;`exit 1;]
