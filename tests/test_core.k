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

/ ---- fold / slash with generic list operand (closes #13) ----
/ a generic list takes as many arguments as its nesting levels, folding and indexing at depth.
/ empty fold gives 0N as in ngn/k (note: q gives ())
t[`genericListFold;(1 2;3 4)/0 1;2]
t[`genericListFoldEmpty;(,(1;2))/!0;0N]
t[`mixedRankFold;(1;2 3)/0;0]
t[`simpleVectorDecode;2 2 2/1 0 1;5]

/ ---- a failed indexed assignment leaves the variable as it was ----
IA:{[set;e]. set;r:.[.:;,e;{`err}];(r;. *":"\set)}
t[`amendKeepIndex;IA["A1:,0";"A1[1]:0"];(`err;,0)]
t[`amendKeepModify;IA["A2:1 2 3";"A2[5]+:1"];(`err;1 2 3)]
t[`amendKeepNegative;IA["A3:1 2 3";"A3[-1]:9"];(`err;1 2 3)]
t[`amendKeepLength;IA["A4:1 2 3";"A4[0 1]:7 8 9"];(`err;1 2 3)]
t[`amendKeepSome;IA["A5:1 2 3";"A5[0 5]:9"];(`err;1 2 3)]
t[`amendKeepDeep;IA["A6:(1 2;3 4)";"A6[0;5]:9"];(`err;(1 2;3 4))]
t[`amendKeepElided;IA["A7:(1 2;3 4)";"A7[;5]:9"];(`err;(1 2;3 4))]
t[`amendStillWorks;(IA["A8:1 2 3";"A8[0 1]+:1"];IA["A9:(1 2;3 4)";"A9[;1]:9"]);((::;2 3 3);(::;(1 9;3 9)))]
t[`amendByNameKeep;(IA["B1:1 2 3";".[`B1;,5;:;9]"];IA["B2:1 2 3";".[`B2;,0 1;:;7 8 9]"];IA["B3:(1 2;3 4)";".[`B3;0 5;:;9]"]);((`err;1 2 3);(`err;1 2 3);(`err;(1 2;3 4)))]
t[`amendByNameWorks;{.[`B4;,1;:;9];.[`B4;,0;+;10];B4}B4:1 2 3;11 9 3]

/ ---- a function just over the bytecode limit is 'limit ----
/ The last of cpl's limit checks marked the error before making it, and
/ segfaulted on a function two bytes short of filling the buffer, even
/ trapped (510 bytes when the buffer was 512; 2046 at 2048).
/ (statements, not one long chain of verbs, which the parser's nesting limit refuses first)
FN:{"{",(";"/x#,"x"),"}"}
/ first, before any other error (an earlier error hid it)
t[`fnBytes2046Msg;6#.[.:;,FN 1023;{x}];"'limit"]
t[`fnUnderLimit;#$. FN 1022;2045]   / 1022 statements (2044 bytes) compile; 2045 is the length of its text
te[`fnBytes2046;". FN 1023"]
t[`fnBytes2046Trap;.[.:;,FN 1023;{`t}];`t]
te[`fnBytes2048;". FN 1024"]

/ ---- a list expression of up to 255 globals compiles (the 512-byte bytecode buffer stopped near 165) ----
G:5;L:{";"/x#,y}
t[`listGlobals255;#. "(",(L[255;"G"]),")";255]
t[`listGlobalsSum254;#. "(",(L[254;"G+1"]),")";254]
t[`listGlobalsLambda254;#(. "{(",(L[254;"G+x"]),")}")1;254]

/ ---- an impossible size is 'limit, one error for all of them (issue #10) ----
ER:{.[.:;,x;{(x?"\n")#x}]}
t[`joinLimitError;ER"s:1048576#\"a\";#\",\"/4096#,s";"'limit"]
t[`scanCountLimitError;ER"{x+y}\\[4294967296;1;2]";"'limit"]

/ ---- floats order by value: the two zeros are one value, and so are the NaNs (issue #15, option A) ----
FZ:-0.0 0.0;FN:0n -0n
t[`floatLessZeros;(FZ[0]<FZ[1];FZ[1]>FZ[0];FN[0]<FN[1];FN[1]>FN[0]);0 0 0 0]
t[`floatGradeZeros;(<-0.0 0.0;<0.0 -0.0;>-0.0 0.0;<-0n 0n;<0n -0n);(0 1;0 1;0 1;0 1;0 1)]
t[`floatGradeMixed;<3 0.0 1 -0.0 -1 -0n 0n;5 6 4 1 3 2 0]
t[`floatBinSearch;(0.0 0.0'-0.0;0.0 1.0'-0.0;-0.0 0.0'0.0);1 0 1]
t[`floatFusedCount;(+/(1.0 -0.0 2.0)<0;+/(1.0 -0.0 2.0)>0);0 2]
t[`floatSortMatchesGrade;{(x@<x)~x g:<x}0.0 -0.0 0n -0n 1.5 -0.0;1]
t[`floatGradeTable;<+`a`b!(0.0 -0.0 0.0 -0.0;4 3 2 1);3 2 1 0]
t[`floatSortedAttr;.[{`sa x};,0.0 -0.0 1.0;{`err}];0.0 -0.0 1.0]

/ ---- squeezing an int vector at the top of int and long (UBSan reported a signed overflow) ----
t[`sqzTopLong;9223372036854775807 1;9223372036854775807 1]
t[`sqzTopInt;@2147483647 1;`I]

/ ---- date and time literals with fields out of range are 'parse, as q refuses them (issue #18) ----
TL:{.[{`i$. x};,x;{`$(x?"\n")#x}]}
t[`litDateBad;TL'("2026.02.29";"1900.02.29";"2026.13.01";"2026.00.10";"2026.04.31");5#`$"'parse"]
t[`litTimeBad;TL'("10:61:00.000";"10:00:60.000";"2026.07.30D10:60:00");3#`$"'parse"]
t[`litStampDateBad;TL"2026.02.30D10:00:00";`$"'parse"]
t[`litStampHour24;TL"2026.07.30D24:00:00.000";`i$2026.07.31D00:00:00.000]
t[`litStampHour99;TL"2026.07.30D99:00:00.000";`i$2026.08.03D03:00:00.000]
t[`litTemporalGood;TL'("2024.02.29";"2000.02.29";"2026.04.30";"24:00:00.000";"99:00:00.000");8825 59 9616 86400000 356400000]

/ ---- float literals past 2^63: the integer digits no longer wrap ----
t[`floatLitBig;`k@12345678901234567890.0;"1.2345678901234567e19"]
t[`floatLit2e64;`k@18446744073709551616.0;"1.8446744073709552e19"]
t[`floatLit2e63;`k@'(9223372036854775808.0;-9223372036854775808.0);("9.223372036854776e18";"-9.223372036854776e18")]
t[`floatLitExp;`k@99999999999999999999e0;"1e20"]
t[`floatLitCast;`k@`F$"12345678901234567890";"1.2345678901234567e19"]
t[`floatLitStrand;`k@1.0 18446744073709551616;"1.0 1.8446744073709552e19"]


/ ---- float literals past the power table ----
/ A long fraction indexed the table of powers of ten past its end; integer digits,
/ which now count in the exponent, would too past 327 of them.
t[`floatDigits328;`F$(328#"1"),".0";0w]
t[`floatDigits330Neg;`F$"-",(330#"1");-0w]
t[`floatLongFraction;`F$"0.",(308#"0"),"1";1e-309]
t[`floatLongFraction2;`F$"0.",(400#"0"),"1";0.0]
t[`floatDigitsLiteral;. (331#"1"),".0";0w]
/ a zero with a large exponent is zero
t[`floatZeroBigExp;0e400;0.0]
t[`floatZeroBigExpF;`F$"0.0e400";0.0]
t[`floatZeroBigExpNeg;`k@-0e400;"-0.0"]
t[`floatOneBigExp;1e400;0w]

/ ---- a range is the vector it stands for, near the int limits too ----
t[`rangeDropNearMax;#2_9223372036854775803+!3;1]
t[`rangeDropNearMaxItems;2_9223372036854775803+!3;,9223372036854775805]
t[`rangeDropBackNearMax;-2_9223372036854775803+!3;,9223372036854775803]
t[`rangeDropAll;#5_9223372036854775803+!3;0]
t[`rangeDropHuge;#9223372036854775807_!3;0]
t[`rangePlusPastMax;9223372036854775806+!3;9223372036854775806 9223372036854775807 0N]
t[`rangeMinusPastMin;(-9223372036854775807+!3)-1;0N -9223372036854775807 -9223372036854775806]
t[`rangePlusPastMaxDrop;(1_9223372036854775806+!3;-1_9223372036854775806+!3);(9223372036854775807 0N;9223372036854775806 9223372036854775807)]

/ ---- take: a count of 2^32 or more is 'limit (it was cut to 32 bits) ----
t[`takeLimit;.[.:;,"4294967297#!3";{(x?"\n")#x}];"'limit"]
t[`takeLimitNeg;.[.:;,"-4294967297#!3";{(x?"\n")#x}];"'limit"]
t[`takeLimitCount;.[.:;,"#4294967297#!100";{(x?"\n")#x}];"'limit"]
t[`takeBig;#100000#!3;100000]
t[`takeCycle;5#1 2;1 2 1 2 1]


/ ---- counts: a list has at most 2^32-1 items; beyond is 'limit, never a wrapped count ----
t[`tilBig;(#!3000000000;*|!3000000000;#!-3000000000;*!-3000000000);3000000000 2999999999 3000000000 -3000000000]
t[`tilLimit;.[.:;,"#!1000000000000000";{(x?"\n")#x}];"'limit"]
t[`tilLimitNeg;.[.:;,"#!-4294967296";{(x?"\n")#x}];"'limit"]
t[`reshapeLimit;.[.:;,"2 4294967297#0";{(x?"\n")#x}];"'limit"]
t[`rollLimit;.[.:;,"#4294967297?1.0";{(x?"\n")#x}];"'limit"]
t[`tilDropBig;#3000000000_!3000000001;1]
t[`windowsSmall;3':!5;(0 1 2;1 2 3;2 3 4)]

/ reshape: a shape whose product passes 2^32, even by wrapping past 2^64, is 'limit; 0N with a long count keeps it
t[`reshapeWrapLimit;*"\n"\.[.:;,"3 6148914691236517206#!5";{x}];"'limit"]
t[`reshapeHugeLimit;*"\n"\.[.:;,"65536 65536 65536 65536#0";{x}];"'limit"]
t[`reshapeNullLongCount;(0N 4294967297#!3;0N 4294967296#!3);(,0 1 2;,0 1 2)]


/ ---- rolls of chars and of full-range ints check their count too ----
t[`rollCharsLimit;.[.:;,"#4294967298?0x00";{(x?"\n")#x}];"'limit"]
t[`rollFullLimit;.[.:;,"#4294967298?-1";{(x?"\n")#x}];"'limit"]
t[`rollCharsNeg;.[.:;,"#-4294967294?\" \"";{(x?"\n")#x}];"'domain"]
t[`rollCharsSmall;#5?" ";5]


t[`rollNullChars;0N?" ";""]

/ ---- where of counts that overflow or pass 2^32 is 'limit (as upstream ngn/k 4b1dd07e, d5099539 check; issue #10) ----
te[`whereOverflow;"#&9223372036854775807 9223372036854775807 2"]
t[`whereLimitSum;.[.:;,"#&4294967297 0";{(x?"\n")#x}];"'limit"]
te[`whereLimitOne;"#&,4294967297"]
t[`whereLimitError;.[.:;,"#&,4294967297";{(x?"\n")#x}];"'limit"]
t[`whereOverflowLimit;.[.:;,"#&9223372036854775807 9223372036854775807 2";{(x?"\n")#x}];"'limit"]
t[`whereNegative;.[.:;,"&1 -1";{(x?"\n")#x}];"'domain"]
t[`whereSmall;(&1 0 2;&0#0);(0 2 2;!0)]
t[`whereWrapNegative;*"\n"\.[.:;,"&9223372036854775807 1";{x}];"'limit"]   / a total that wraps negative was 'domain

/ ---- numeric literals: 0N among floats, and boolean tokens among numbers ----
tv[`litNullFloat;"0N 1.5";0n 1.5]
tv[`litFloatNull;"1.5 0N";1.5 0n]
tv[`litNegNullFloat;"-0N 1.5";0n 1.5]
t[`litNegNullBits;(-8!-0N 1.5)~-8!0n 1.5;1]   / the same bits as 0n (-0n has the sign bit)
tv[`litIntBool;"1 0b";1 0]
tv[`litIntBools;"1 01b";1 0 1]
tv[`litNullBools;"0N 1b 01b";0N 1 0 1]
tv[`litFloatBool;"1.5 1b";1.5 1.0]
tv[`litFloatBools;"1.5 01b 2";1.5 0.0 1.0 2.0]
tv[`litPlain;"(1 2;1.5 2;0N 1;0n 1.5)";(1 2;1.5 2.0;0N 1;0n 1.5)]


/ ---- a bool token must end the token: 1b.5 is not 1b then .5 ----
te[`boolDot;"{b:7;1 1b.5}[]"]
te[`boolDotAlone;"{b:7;01b.1}[]"]
t[`boolStrandStill;1 01b;1 0 1]

/ ---- scans with a float seed, or of floats with -, are float scans ----
t[`scanSeedAdd;1.5+\1 2;2.5 4.5]
t[`scanSeedMul;1.5*\1 2;1.5 3.0]
t[`scanSeedSub;1.5-\1 2;0.5 -1.5]
t[`scanSeedChars;1.5+\"ab";98.5 196.5]
t[`scanSeedZero;0.0+\1 2;1.0 3.0]
t[`scanSubFloat;-\1.5 2.5;1.5 -1.0]
t[`scanSubFloatSeed;1.5-\1.5 2.5;0.0 -2.5]
t[`scanSeedLong;((1.5+\!100000)~{x+y}\[1.5;!100000];(-\0.5+!100000)~{x-y}\0.5+!100000);11b]
t[`scanOthersOk;(1.5&\1 2;1.5|\1 2;1+\1.5 2;+\1 2 3;-\1 2 3);(1.0 1.0;1.5 2.0;2.5 4.5;1 3 6;1 -1 -4)]


/ ---- =\ of floats compares values, as {x=y}\ does (it read the bits as ints) ----
t[`eqScanFloat;=\1.0 1.0 1.0;(1.0;1;1)]
t[`eqScanFloatSeed;1.0=\1 1 2;1 1 0]
t[`eqScanFloatAsLambda;(=\0.5 0.5 1.5)~{x=y}\0.5 0.5 1.5;1]

/ ---- = on floats compares values: -0.0 equals 0.0, and a NaN equals any NaN ----
t[`eqZeros;{a:-1*0.0;(a=0.0;0.0=a;a=0;0=a)}[];1 1 1 1]
t[`eqZerosVec;{a:(-1*0.0),1;(a=0.0 1;0.0=a;(,*a)=,0.0)}[];(1 1;1 0;,1)]
t[`eqNans;{(x=-x;(0%0)=x;x=x;x=0.0)}0n;1 1 1 0]
t[`eqPlain;(1.5=1.5;1.5=2.5;1 2.5=1.0 2.5;1=1.0);(1;0;1 1;1)]
t[`eqAgreesMatch;{(x=y)~x~y}[-0.0;0.0];1b]
t[`paZerosOneValue;.[{`at(`pa x)};,0.0 -0.0 0.0;{`err}];`p]   / `pa: both zeros are one value, so the run is one

/ ---- find and distinct on floats: every NaN is one value, as for ~ and = ----
/ n is 0n and -n the NaN with the sign bit, the spelling x86-64 computes (0%0)
t[`findNan;{(x,1.0)?-x}0n;0]
t[`findNanNeg;{((-x),1.0)?x}0n;0]
t[`findNanEach;{(1.0,x)?(-x),x}0n;1 1]
t[`findNanMiss;{1.0 2.0?-x}0n;0N]
t[`distinctNan;{(#?x,(-x),1.0;#?x,1.0,-x)}0n;2 2]
t[`distinctNanLength;{#?3000#x,(-x),1.0}0n;2]
t[`findZeroStill;1.0 -0.0?0.0;1]
t[`paNansOneValue;*"\n"\.[{`at(`pa x)};,0n 1.0 -0n;{x}];"'p-fail"]   / `pa: every NaN is one value, in two runs here

/ ---- distinct of a generic list keeps the first of matching items ----
/ (compared as text, since ~ takes -0.0 for 0.0)
t[`distinctFirstPos;`k@?(0;0.0;-1*0.0);"(0;0.0)"]
t[`distinctFirstNeg;`k@?(0;-1*0.0;1.0-1.0);"(0;-0.0)"]
t[`distinctFirstNan;?(1;0n;-0n;0n);(1;0n)]
t[`distinctGenPlain;?(1;"a";1;`b;"a");(1;"a";`b)]

/ ---- group of floats: -0.0 and 0.0 (and NaNs) are one key, indices ascending, first key seen ----
t[`groupZeros;(=0.0 -0.0;=-0.0 0.0 1.5 0.0);((,0.0)!,0 1;-0.0 1.5!(0 1 3;,2))]
t[`groupNans;=0n 1.0 -0n;0n 1.0!(0 2;,1)]
t[`groupFloatPlain;=1.5 2.5 1.5;1.5 2.5!(0 2;,1)]
t[`groupGenericZeros;`k@=(0;0.0;-1*0.0);"(0;0.0)!(,0;1 2)"]

/ ---- literals of any length (they were cut at 512 numbers or chars, 256 symbols) ----
t[`litLongInts;#. " "/600#,"1";600]
t[`litLongFloats;+/. " "/600#,"1.5";900.0]
t[`litLongBools;. " "/(600#,"1"),,"0101b";(600#1),0 1 0 1]
t[`litLongString;#. "\"",(1300#"ab\\n"),"\"";975]
t[`litLongSyms;#?. ,/"`s",/:$!700;700]
t[`litLongSymsOrder;. ,/"`s",/:$!300;`$"s",/:$!300]
/ ---- a list expression of 256 or more items is 'limit, not 'compile ----
t[`compileLimit;*"\n"\.[.:;,"a:1;(",(";"/256#,"a"),")";{x}];"'limit"]
/ the other limits of the bytecode are 'limit too: a $[..] branch over 255 bytes, more than 8 names
/ assigned at once, 9 or more arguments, 16 or more locals (14 and 15 were 'limit already; 13 compile)
t[`limitCondJump;*"\n"\.[.:;,"{$[x;",("+"/130#,"x"),";0]}";{x}];"'limit"]
t[`limitAssignNames;*"\n"\.[.:;,"(a;b;c;d;e;f;g;h;i):!9";{x}];"'limit"]
t[`limitArgs;*"\n"\.[.:;,"{x}[1;2;3;4;5;6;7;8;9]";{x}];"'limit"]
t[`limitLocals;*"\n"\.[.:;,("{",(";"/(,'"abcdefghijklmnopq"),\:":1"),"}");{x}];"'limit"]
t[`limitsInside;({$[x;1;0]}1;(a;b):1 2;{x+y}[1;2];{a:1;b:2;a+b}[]);(1;1 2;3;3)]
t[`compileUnderLimit;. "a:1;#(",(";"/100#,"a"),")";100]


/ ---- a string that ends in a backslash is 'parse (the read past the text shows only under ASan, with piped input as in #22) ----
t[`strTrailingBackslash;.[.:;,"\"a\\";{(x?"\n")#x}];"'parse"]

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

/ ---- a missing or failing temporal formatter is an error, not the end of the process or a crash ----
te[`fmtMissing;"$2026.07.30"]
te[`fmtMissingK;"`k@2026.07.30"]
/ a projection, composition or derived verb holding a date, and JSON, pass the error up too
te[`fmtMissingProj;"`k@{x+y}[2026.07.30]"]
te[`fmtMissingDerived;"`k@(2026.07.30+)"]
te[`fmtMissingString;"$({x+y}[2026.07.30])"]
te[`fmtMissingJson;"`j@2026.07.30"]
te[`fmtMissingJsonList;"`j@(1;2026.07.30)"]
t[`fmtMissingGrade;<({x+y}[2026.07.30];{x+y}[2026.07.31]);0 1]
tv[`fmtFails;"dstr::{x+`a};.[{$x};,2026.07.30;{`caught}]";`caught]
te[`fmtFailsList;"dstr::{x+`a};`k@(2026.07.30;1)"]


te[`fmtFailsProj;"dstr::{x+`a};`k@{x+y}[2026.07.30]"]
te[`fmtFailsJson;"dstr::{x+`a};`j@`a`b!(1;2026.07.30)"]

/ ---- a temporal item keeps its type in a list ----
/ There is no temporal vector: pushing a date onto an int list made it an int.
t[`joinDateType;@'1,2026.07.30;`i`d]
t[`joinTimeType;@'100000 2,12:00:00.000;`i`i`t]
t[`eachDateType;@'(1;0)+(2;2026.07.30);`i`d]
t[`eachDateValue;2026.07.30=*|(1;0)+(2;2026.07.30);1]
t[`eachStampType;@'(5000000000;1)+(2;2026.07.30D00:00:00.0);`i`n]
t[`joinIntsStay;@1 2,3;`I]

/ ---- the int null converts to the float null (0n), never to a number ----
t[`nullToFloatAdd;(0N+1.5;1.5+0N;1.5-0N;0N*2.0);4#0n]
t[`nullToFloatDiv;(0N%2;2%0N);0n 0n]
t[`nullToFloatDivVec;(0N%2 3;2 3%0N);(0n 0n;0n 0n)]
t[`nullToFloatCast;`f$0N;0n]
t[`nullToFloatEq;0N=0n;1b]
t[`nullToFloatMin;(0N&1.5;0N|1.5);(0n;1.5)]
t[`nullToFloatSum;+/(0N;1.5);0n]
t[`nullToFloatVecOk;0N 1+1.5;0n 2.5]

/ ---- dict arithmetic: a null value is a value; only a missing key takes the identity ----
t[`dictNullKept;((`a!0N)+`a!5;(`a!0n)+`a!5.0;(`a!0n)%`a!2.0);((,`a)!,-9223372036854775803;(,`a)!,0n;(,`a)!,0n)]
t[`dictNullKeptMixed;(`a`b!0N 1)+`b`c!5 6;`a`b`c!0N 6 6]
t[`dictMissing;((`a!1)+`b!2;(`a!1)-`b!2;(`a!1)*`b!2);(`a`b!1 2;`a`b!1 -2;`a`b!1 2)]
t[`dictMissingMinMax;((`a!1)&`b!2;(`a!1)|`b!2;(`a!1.5)&`b!2.5);(`a`b!1 2;`a`b!1 2;`a`b!1.5 2.5)]
t[`dictCommon;(`a`b!1 2)+`b`c!10 20;`a`b`c!1 12 20]


/ ---- a key on one side only takes the identity atom; the other values are left as they are ----
t[`dictMissListVal;(`a`b!1 2)+`c!,1 2;`a`b`c!(1;2;1 2)]
t[`dictMissOrder;((`a`b!(1 2;3))+`c!5;(`a`b!(3;1 2))+`c!5);(`a`b`c!(1 2;3;5);`a`b`c!(3;1 2;5))]
t[`dictMissLonger;(`a`b!(1 2 3;3))+`a`c!(5;6 7);`a`b`c!(6 7 8;3;6 7)]
/ a temporal value: no crash; types only, as no formatter is loaded
t[`dictDateAdd;@'.((,`a)!,2026.07.30)+(,`a)!,1;,`d]
t[`dictDateAddValue;2026.07.31=*.((,`a)!,2026.07.30)+(,`a)!,1;1]
t[`dictTyped;((`a!1.5)+`b!2.5;(`a`b!1 2)-`b`c!10 20);(`a`b!1.5 2.5;`a`b`c!1 -8 -20)]
/ a dict with untyped empty keys fills nothing ('length, as 2.3.1); typed empty keys fill, as in q
t[`dictEmptyUntyped;*"\n"\.[.:;,"(`a`b!1 2)+()!()";{x}];"'length"]
t[`dictEmptyTyped;(`a`b!1 2)+(0#`)!!0;`a`b!1 2]
t[`dictEmptyVals;(((0#`)!())-`a`b!1 2;(`a`b!1 2)+(0#`)!());(`a`b!-1 -2;`a`b!1 2)]   / empty values () filled nothing: `a`b!(!0;!0)

/ ---- the fused masked sums equal the unfused expression, bit for bit ----
fu:{[x;m]`k@+/x@&m};un:{[x;m]i:&m;d:x@i;`k@+/d}
t[`maskSumOrder;fu[-1298990374206.0 1.0 -9005900264370198.0 1298990374206.0;1 1 1 1];un[-1298990374206.0 1.0 -9005900264370198.0 1298990374206.0;1 1 1 1]]
mv:{x#1e16 1.0 -1e16 3.0 0.1 7e15 -0.3}
t[`maskSumLong;{fu[mv x;y]~un[mv x;y]}'[5 17 37 100;(5#1;17#1 0 1;37#1 1 0 1 0;100#0 1 1)];1 1 1 1]
fa:{[a;b;m]`k@+/(a+0.1*b)@&m};ua:{[a;b;m]c:a+0.1*b;i:&m;d:c@i;`k@+/d}
t[`maskFmaNoContract;fa[2#-0.30000000000000004;2#3.0;1 1];"0.0"]
t[`maskFmaOrder;{fa[x;y;z]~ua[x;y;z]}'[0.3*!'17 37;0.7*1+!'17 37;(17#1 1 0;37#0 1)];1 1]

/ cases that show the old order on x86 as well as arm64 (the four-item ones above are exact on x86)
MD:1%1+!1000
t[`maskSumX86;`k@{[x;m]+/x@&m}[MD;1000#1 0];`k@{[x;m]{+/x}x@&m}[MD;1000#1 0]]
t[`maskFmaX86;`k@{[a;m]+/(a+0.1*a)@&m}[MD;1000#0 1 1];`k@{[a;m]{+/x}(a+0.1*a)@&m}[MD;1000#0 1 1]]
t[`maskCmpX86;`k@{[x;c]+/x@&c>0}[MD;1000#1 -1];`k@{[x;c]{+/x}x@&c>0}[MD;1000#1 -1]]
/ an int atom on the left is 'type, as unfused (the fused sum read it as a vector)
t[`maskSumIntAtom;*"\n"\.[.:;,"+/0@&0 1";{x}];"'type"]
t[`maskFmaIntAtom;*"\n"\.[.:;,"+/(1+2*3)@&0 1";{x}];"'type"]
t[`maskCmpIntAtom;*"\n"\.[.:;,"+/0@&(0 1)>0";{x}];"'type"]

/ ---- -/ of floats is the left fold x0-x1-.. ----
t[`minusFoldInf;(-/0w 1.0;-/-0w 5.0;{x-y}/0w 1.0);0w -0w 0w]
t[`minusFoldBig;-/1e308 1.0;1e308]
t[`minusFoldSeeded;(0w-/1.0 2.0;1.5-/0.5 0.25;2-/0.5 0.25);0w 0.75 1.25]
t[`minusFoldZero;`k@(-/,0.0;-/0.0 0.0;0.0-/!0;-/0#0.0);"0.0 0.0 0.0 0.0"]
t[`minusFoldInts;(-/1 2 3;-/,5;10-/1 2;-/!0);(-4;5;7;0)]

/ ---- chars order as unsigned bytes, the same in <, >, grade and generic lists ----
t[`charGradeUp;<0x008061;0 2 1]
t[`charGradeDown;>0x008061;1 2 0]
t[`charLess;(0x80<"a";"a"<0x80;0x80>"a");0 1 1]
t[`charLessVec;("b",(`c$200),"a")<"b";0 0 1]
t[`charAgreesGeneric;(<0x8061)~2#<(0x80;"a";`b);1b]
/ the fused neighbour compare, which `sa checks sortedness with, orders chars as < does
t[`charShiftCmp;{((1_x)<-1_x;+/(1_x)<-1_x)}0x806100;(1 1;2)]
t[`charSortedAttr;(@`sa "ab",0x80;.[{`sa x};,0x80ff0061;{`fail}]);(`C;`fail)]
t[`charBinNumber;("abc"'-5 0 98 300;0x006180'-1 100 200 128 127);(-1 -1 1 2;-1 1 2 2 1)]   / a number against chars: as a byte 0 to 255, below them if negative
t[`charBinNarrow;0x006180'(100 1;01b;100.0);(1 0;0 0;1)]   / byte-width ints, bools and floats too
t[`charFirstGrade;(*<0x8061;*>0x80ff00ff;{*<x}"b",0x80,"a");1 1 2]   / *< and *> (the fused first of a grade) order chars as < does
t[`charLessInt;((`c$200)<300;(`c$200)>199;(`c$200)<0;(`c$200)=-56);1 0 1 1]
t[`charEachPrior;(<':0x00800061;>':0x00800061;0x80<':0x0061);(0 0 1 0;0 1 0 1;1 0)]
t[`charSortFused;{(x@<x;x@>x)}0x800061;(0x006180;0x806100)]
t[`charSortAgrees;{(x@<x)~x@(<x)@!#x}0x80ff00617a;1b]
t[`charBin;(0x006180'0x70;0x006180'0x90;0x006180'"a";0x006180'0x00);1 2 1 0]
t[`charBinVec;0x006180'0x7090;1 2]
t[`intLessUnchanged;(1 2<2 1;-1<1;<2 1 3);(1 0;1b;1 0 2)]


/ ---- binary search on a char list compares chars as unsigned bytes, inside a list too ----
t[`binCharInList;(," ")'(0x80;"");(0;!0)]
t[`binCharInList2;(" ",0x80)'(0xff;"");(1;!0)]
t[`binIntInList;1 3 5'(2;4 6);(0;1 2)]


/ ---- the fused compare idioms order chars as the unfused ones do (unsigned among chars) ----
t[`fusedCharCount;+/0x806162>"a";2]
t[`fusedCharWhere;&0x806162>"a";0 2]
t[`fusedCharVec;+/0x8061<"ab";1]
t[`fusedCharVecVec;{c:0x806162;d:0x618081;(+/c<d;+/b:c<d)}[];2 2]
t[`fusedCharCompress;{s:1 2;x:0x8061;s@&x<"b"}[];,2]
t[`fusedCharInt;+/0x806162<100;3]


/ ---- folds and scans of chars agree with the verbs: arithmetic reads chars as ints, < > unsigned (& |: next section) ----
t[`charFoldOne;(%/,"a";|/,"a";+/,"a";*/,"a");(97;"a";97;97)]
t[`charScan;(+\"ab";|\"ab";&\"ba";%\,"a");(97 195;"ab";"ba";,97)]
t[`charFoldSeed;("a"+/!0;"a"%/!0;"a"+/();0x80</!0);97 97 97 128]
t[`charFoldLess;(</"ab";</0x0080;<\0x800061);(1;1;128 0 1)]
t[`charFoldSum;(+/"ab";1+/"ab");195 196]


/ ---- a seeded < > fold or scan of chars goes item by item, as the verb does ----
t[`charFoldSeedItems;"a"</,0x80;1]
t[`charFoldSeedItemsGt;"a">/,0x80;0]
t[`charFoldSeedAsScan;("a"</0x8081)=*|"a"<\0x8081;1]
t[`charFoldIntSeed;0</"ab";1]
/ a seeded < > fold or scan with chars applies the verb item by item, so it is
/ the lambda fold and the last of the scan, for atom, vector and dict items
t[`charFoldAtomItem;"a"</0x80;"a"<0x80]
t[`charScanAtomItem;"a"<\0x80;"a"<0x80]
t[`charFoldVecSeedAtom;"ab"</0x80;1 1]
t[`charFoldVecSeedScan;(0x8081</,0x00)~*|0x8081<\,0x00;1]
t[`charFoldDictItems;("a"</`a`b!0x8080)~*|. "a"<\`a`b!0x8080;1]
/ & and | with a char seed give a char over a char atom, a dict of chars, a list of strings, and none
t[`charMinSeedAtom;("a"&/0x80;"a"|\0x80);("a";0x80)]
t[`charMinSeedDict;"a"&/`a`b!"bc";"a"]
t[`charMinSeedLists;"z"&/(,"b";,"a");,"a"]
t[`charMinSeedEmpty;"a"&/!0;"a"]
/ each-prior with a number seed: chars against each other unsigned, & | of two chars a char, as {x f y}': gives
t[`charEachPriorIntSeed;(0<':0x8000;0>':0x0080);(1 1;0 1)]
t[`charEachPriorIntSeedMin;`k@0&':0x8061;"(-128;\"a\")"]
/ a dict or list seed holding chars goes item by item through & and |, as the verb does
t[`charMinSeedDictItems;((`a`b!"a",0x80)&/,0x80;("ab";"cd")&/,"b";("ab";"cd")|\,0x80);(`a`b!"a",0x80;("ab";"bb");,(0x8080;0x8080))]
/ amending a dict of chars at a new key with & or | starts from the char identity (0xff, 0x00)
t[`charAmendIdentity;(@[`a!,"a";`c;|;"x"];@[`a!,"a";`c;&;0x80]);(`a`c!"ax";`a`c!"a",0x80)]
t[`charFoldIntSeedVerb;(0</,0x80)~0<0x80;1]
t[`charFoldLambda;("a"</0x61618080)~{x<y}/["a";0x61618080];1]
t[`charScanEmpty;("a"+\"";0<\"");("";"")]


/ ---- & and | of two chars compare them as unsigned bytes and give a char, as q; with a number they stay numeric ----
t[`charMinMax;(0x80&"a";0x80|"a";"ab"&"ba";"a"&"b";"ab"&0x80);("a";0x80;"aa";"a";"ab")]
t[`charMinMaxNumber;(1&"a";0x80&0;("a";1)&"b");(1;-128;("a";1))]
t[`charMinMaxFold;(&/"ab";|/"a",`c$200;&/0x80c3;&/"";|/"";"a"&/"bc";0&/"ab");("a";0xc8;0x80;0xff;0x00;"a";0)]
t[`charMinMaxScan;(&\"bac";|\"bac";&':"bac";|':"bac";"z"&\"bac");("baa";"bbc";"baa";"bbc";"baa")]

/ ---- JSON: floats outside the int range, infinities, NaN ----
t[`jsonBigFloat;(`j@1e20;`j@-1e20;`j@1e22;`j@1.5;`j@3.0;`j@-2.0);("1e20";"-1e20";"1e22";"1.5";,"3";"-2")]
t[`jsonInfNan;(`j@0w;`j@-0w;`j@0n;`j@1.0 0w -0w 0n);("null";"null";"null";"[1,null,null,null]")]
t[`jsonLiterals;`j?"[1,null,2]";(1.0;0n;2.0)]
t[`jsonRoundTrip;`j?`j@1e20 -1.5 2.0;1e20 -1.5 2.0]


/ ---- JSON true and false read as 1 and 0 ----
t[`jsonBools;`j?"[true,false,null]";(1;0;0n)]
t[`jsonDocExample;`j?"{\"a\":1,\"b\":[true,\"c\"]}";`a`b!(1.0;(1;,"c"))]
/ :: is written as null, as in q, so writing and reading agree (issue #20, row 24)
t[`jsonNullOut;(`j@(::);`j@(1;::);`j@`a`b!(1;::));("null";"[1,null]";"{\"a\":1,\"b\":null}")]

/ ---- encoding with no digits is empty, and checks y as one digit does (issue #20, row 9) ----
t[`encodeEmpty;((!0)\5;(!0)\6 7;(!0)\(3 4;5);10 10\123);(!0;0#,0N 0N;0#,(0N 0N;0N);2 3)]
t[`encodeEmptyFloat;@(!0)\5.5;`F]
t[`encodeEmptyChar;*"\n"\.[.:;,"(!0)\\\"a\"";{x}];"'type"]
t[`encodeOneDigitChar;(,10)\"a";,7]
t[`encodeEmptySym;*"\n"\.[.:;,"(!0)\\`a";{x}];"'type"]

/ ---- float to int: 0n is 0N; out of range, floor gives 0W above and 0N below, a cast 0W and -0W (as q) ----
t[`floorNull;(_0n;_0w;_-0w);0N 9223372036854775807 0N]
t[`floorVec;_0n 0w -0w 1.5 -1.5;0N 9223372036854775807 0N 1 -2]
t[`floorBig;_1e300 -1e300;9223372036854775807 0N]
t[`castNull;(`i$0n;`i$0w;`i$-0w);0N 9223372036854775807 -9223372036854775807]
t[`castVec;`i$0n 0w -0w 2.9 -2.9;0N 9223372036854775807 -9223372036854775807 2 -2]
t[`castFloat;(`i$1e19;`i$-1e19);9223372036854775807 -9223372036854775807]
t[`floorPlain;_2.5 -2.5 0.0;2 -3 0]

/ ---- find between chars and numbers is 'type, as in q (issue #20, row 12) ----
t[`findCharHayIntAtom;*"\n"\.[.:;,"\"abc\"?97";{x}];"'type"]
t[`findCharHayIntVec;*"\n"\.[.:;,"\"abc\"?98 99";{x}];"'type"]
t[`findCharHayFloat;*"\n"\.[.:;,"\"abc\"?97.0";{x}];"'type"]
t[`findIntHayCharAtom;*"\n"\.[.:;,"1 2 97?\"a\"";{x}];"'type"]
t[`findIntHayCharVec;*"\n"\.[.:;,"1 2 97?\"ab\"";{x}];"'type"]
t[`findWideHayChar;*"\n"\.[.:;,"1000 2000 97?\"a\"";{x}];"'type"]
t[`findMixedNeedles;*"\n"\.[.:;,"\"abc\"?(97;\"b\")";{x}];"'type"]
t[`findCharChar;"abc"?"cb";2 1]
t[`findIntInt;1 2 97?97 1;2 0]
t[`findGeneric;((1;"a")?"a";(1;"a")?97);1 0N]
t[`findNeedleKept;{n:98 99;.[{"abc"?x};,n;{x}];n}[];98 99]

/ ---- integer literals: from 2^63 in magnitude 'limit, as q refuses them (issue #20, row 7) ----
tv[`litIntMax;"9223372036854775807 -9223372036854775807 000000000000000000001";9223372036854775807 -9223372036854775807 1]
te[`litInt2p63;"9223372036854775808"]
te[`litInt2p63Neg;"-9223372036854775808"]
te[`litIntWrap;"18446744073709551615"]
te[`litIntLimit;"18446744073709551616"]
/ a literal that fails for another reason (here, too many numbers, a cap the literal-limits fix removes; with it the case
/ checks the ordinary path) leaves nothing behind for the next
t[`litIntAfterFail;{.[.:;,"99999999999999999999 ",(" "/600#,"1");{`e}];. "1+1"}[];2]
te[`litIntLimitNeg;"-18446744073709551616"]
te[`litIntLimitVec;"1 18446744073709551617"]
te[`litIntLimitBig;"99999999999999999999"]

/ ---- `I$ of digits beyond int64 is 0N, like any text that is not an int ----
t[`parseIntBig;`I$"99999999999999999999";0N]
t[`parseIntMaxPlus1;`I$"9223372036854775808";0N]
t[`parseIntMax;`I$"9223372036854775807";9223372036854775807]
t[`parseIntMin;`I$"-9223372036854775807";-9223372036854775807]
t[`parseIntZeros;`I$"0000000000000000000000012";12]
t[`parseIntVec;`I$("12";"99999999999999999999";"x");12 0N 0N]


/ ---- JSON escapes of control characters ----
t[`jsonCtl;(`j@`c$1;`j@`c$65 1;`j@`c$1 65;`j@`c$31 127);("\"\\u0001\"";"\"A\\u0001\"";"\"\\u0001A\"";"\"\\u001f\\u007f\"")]


/ ---- JSON writes NUL as \u0000 ----
t[`jsonNul;(`j@"a\0b";`j@"\0");("\"a\\u0000b\"";"\"\\u0000\"")]


/ ---- JSON output is sized right (upstream ngn/k fdf2ec5f) ----
t[`jsonLongNeg;(`j@-3908768282852987948;`j@(-3908768282852987948;1));("-3908768282852987948";"[-3908768282852987948,1]")]
t[`jsonCharDict;(`j@`a`b!"xy";`j@(`a!,"x";1));("{\"a\":\"x\",\"b\":\"y\"}";"[{\"a\":\"x\"},1]")]

/ a large int: Jl compared it with the null by subtracting, which overflowed (UBSan)
t[`jsonLongInt;(`j@3000000000;`j@(3000000000;1));("3000000000";"[3000000000,1]")]

/ ---- a symbol that is not an identifier prints quoted, with its bytes as they are ----
t[`symDigitFirst;`k@`$"1a";"`\"1a\""]
t[`symUtf8;`k@`$`c$195 169;"`\"",(`c$195 169),"\""]
t[`symIdent;`k@`abc;"`abc"]
t[`symList;`k@`$("1a";"b");"`\"1a\"`b"]
tv[`symBack;"{x~. `k@x}@`$(\"1a\";\"a b\";\"\";\"a\";((`c$1),\"x\");(\"a\",`c$233))";1b]


/ ---- a Cyrillic lead byte takes the next byte only if it continues the character ----
/ (at the end of the text it stepped over the terminator and the lexer ran on past it)
tv[`pidEnd;"$.(\"`\",(`c$208),(`c$0),\";42\")";,`c$208]
tv[`pidCyrillic;"$.(\"`\",`c$208 184 208 176)";`c$208 184 208 176]
tv[`pidLoneBack;"{x~. `k@x}@`$((\"a\",`c$208);(\"b\",(`c$208),\"`\");(`c$208 208);(`c$208 184))";1b]

/ ---- `s$ of a char is the one-char symbol, for every byte ----
t[`charToSym80;(`s$0x80)~`$,0x80;1b]
t[`charToSymFF;(`s$0xff)~`$,0xff;1b]
t[`charToSymAll;(`s$'`c$!256)~`$,'`c$!256;1b]
t[`charToSymA;`s$"a";`a]

/ ---- dict arithmetic: keys of different ranks and keyed tables are errors, not crashes ----
t[`dictMixedRank;*"\n"\.[.:;,"(`a`b!1 2)+((1 2;3 4)!5 6)";{x}];"'domain"]
t[`keyedPlusKeyed;*"\n"\.[.:;,"a:(+(,`a)!,1 2 3)!4 5 6;b:(+(,`a)!,2 3 4)!10 20 30;a+b";{x}];"'nyi"]
t[`keyedPlusKeyedCols;*"\n"\.[.:;,"((+(,`a)!,1 2)!3 4)+((+(,`b)!,1 2)!3 4)";{x}];"'domain"]   / key tables with different columns do not join
t[`dictListKeys;((1 2;3 4)!5 6)+(1 2;3 4)!7 8;(1 2;3 4)!12 14]
t[`keyedPlusAtom;((+(,`a)!,1 2 3)!4 5 6)+1;(+(,`a)!,1 2 3)!5 6 7]

/ the higher rank on the left crashed
t[`dictMixedRankL;*"\n"\.[.:;,"((1 2;3 4)!5 6)+`a`b!1 2";{x}];"'domain"]

/ a dict with no keys is not checked: two empty ones still add, as group counts from ()!() do
t[`dictEmptyBoth;`k@((()!())+#'=0#`;(#'=0#`)+()!());"(()!0#,!0;(0#`)!!0)"]

/ ---- an error with a computed message reads its text before releasing it ----
t[`errComputedText;*"\n"\.[{`err "ab",x};,"cd";{x}];"'abcd"]

/ ---- take, drop, cut and reshape on bit-boolean lists (01B), as upstream ngn/k ----
t[`bitTake;(0#01B;1#01B;-1#01B;5#01B);(!0;,0;,1;0 1 0 1 0)]
t[`bitTakeNeg;-5#0110B;0 0 1 1 0]
t[`bitDrop;(1_01B;0_01B;-1_01B);(,1;!2;,0)]
t[`bitCut;0 1_0110B;(,0;1 1 0)]
t[`bitReshape;3 0N#0110B;(,0;,1;1 0)]
t[`bitLong;+/70#0110B;35]
t[`bitLongDrop;`k@65_70#0110B;"1 1 0 0 1"]


/ ---- bit lists from take and drop match their values ----
/ Take, drop, cut and reshape leave the bits after the last item as they were;
/ match (and so find, distinct, group and dict keys) and -8! ignore them.
t[`bitTakeMatch;(2#1111B)~11B;1]
t[`bitDropMatch;(-2_1111B)~11B;1]
t[`bitCutMatch;(*0 2_1111B)~11B;1]
t[`bitReshapeMatch;(3 0N#111111B)~(11B;11B;11B);1]
t[`bitFind;(,11B)?2#1111B;0]
t[`bitDistinct;#?(11B;2#1111B);1]
t[`bitGroup;#=(11B;2#1111B);1]
t[`bitDictKey;((11B;00B)!1 2)2#1111B;1]
t[`bitSer;(-8!2#1111B)~-8!11B;1]
t[`bitSerRound;(-9!-8!2#1111B)~11B;1]
t[`bitNotMatch;(2#1011B)~11B;0]

/ appending one bit clears it first: take and drop leave stale bits after the last item
t[`bitAppend;((2#1111B),0B;{r:2#1111B;r,:0B;r,:0B;r}[];`k@(2#1111B),0);(1 1 0;1 1 0 0;"1 1 0")]

/ where reads only the list's own bits (take, drop and ~ leave others after the last item; each
/ was written past the result, which a release build does not show in the values)
t[`bitWhereDrop;&-4_0000111100001111B;4 5 6 7]
t[`bitWhereNot;&~01B;!1]
/ ~ flips whole words, so on 2.3.1 match read its stale bits too
t[`bitNotMatch2;(~01B)~10B;1]
/ indexing by a range crashed
t[`bitRange;(1111B@!2;0110B@1+!2);(1 1;1 1)]

/ ---- tail calls reuse the frame: no depth limit, and nothing freed twice ----
/ A call whose result is the function's result runs in the caller's frame, so
/ tail recursion of any depth works (it stopped at 2048 with 'stack).
tcCnt:{$[x;o x-1;0]}
t[`tailSelf;tcCnt 1000000;0]
tcName:{$[x=0;y;tcName[x-1;y+1]]}
t[`tailByName;tcName[100000;0];100000]
tcThen:{$[x>0;o x-1;`done]}
t[`tailThenBranch;tcThen 50000;`done]
tcBlk:{[x]y:x-1;$[y<0;`blk;[z:y;o z]]}
t[`tailInBlock;tcBlk 50000;`blk]
tcEv:{$[x;tcOd x-1;1b]};tcOd:{$[x;tcEv x-1;0b]}
t[`tailMutual;(tcEv 100000;tcEv 99999);10b]
tcAcc:{[n;s]$[n;o[n-1;s,n];s]}
t[`tailAccum;+/tcAcc[5000;!0];12502500]
tcBig:{[a;b;c;d;e;f;g;h]$[a;o[a-1;b;c;d;e;f;g;h+1];h]}
t[`tailEightArgs;tcBig[100000;0;0;0;0;0;0;0];100000]
/ a tail call to a function that needs a bigger frame, a projection, a wrong count
tcFac:{$[x<2;1;x*o x-1]}
t[`nonTailStillWorks;tcFac 20;2432902008176640000]
te[`nonTailStillLimited;"tcFac 5000"]
tcPr:{x+y};tcPj:{$[x;o x-1;tcPr 1]}
t[`tailToProjection;(tcPj 10)2;3]
tcRk:{$[x;o[x-1;2];0]}
te[`tailWrongArity;"tcRk 5"]
/ the shape that found a double free: repeated calls through a tail call, then others
tcD:-tcB:*/32#2;tcP:18(3*)\1
tcCarry:{(tcB!x)+0 :':tcD!x,:0}
tcPow:{$[18>y;tcCarry x*tcP y;o[tcCarry x*tcP 18;y-18]]}
tcLoop:{tcCarry (tcP 18!y)*(-18!y){tcCarry x*tcP 18}/x}
t[`tailPowMatchesLoop;(tcPow[,1;500];tcPow[,1;500];tcPow[,1;100000]);(tcLoop[,1;500];tcLoop[,1;500];tcLoop[,1;100000])]
t[`tailAfterwards;(tcCnt 3;tcName[3;0];tcCarry 5 7);(0;3;5 7 0)]

/ ---- nesting deeper than the parser's limit is 'limit, not a crash ----
t[`nestParens300;. (300#"("),"1",300#")";1]
te[`nestParensDeep;(20000#"("),"1",20000#")"]
te[`nestLambdasDeep;(20000#"{"),20000#"}"]
te[`nestIndexDeep;"a",(20000#"["),20000#"]"]
te[`nestChainDeep;(,/20000#,"1+"),"1"]
te[`nestAdverbsDeep;"1",,/20000#,"+/"]
te[`stringBackslashEnd;"\"a\\"]

/ ---- 2.4.1: reverse, a seeded :': and m! by a power of two in one pass ----
t[`revBytes;|"abcdefghijk";"kjihgfedcba"]
t[`revWidths;(|1 2 3;|1 2 300000;|1 2 5000000000;|!0);(3 2 1;300000 2 1;5000000000 2 1;!0)]
t[`revShared;{v:x;(|v;x)}"abcdefghij";("jihgfedcba";"abcdefghij")]
t[`seedShift;(0 :':5 6 7;300 :':1 2 3;"x" :':"abc");(0 5 6;300 1 2;"xab")]
t[`modPow2Shared;{(4294967296!x;256!x;x)}5000000000 -7;(705032704 4294967289;0 249;5000000000 -7)]

/ ---- a number before `-` and a float literal (no space) is not read as a float: the strand ends at the `-` ----
t[`strandMinusFloat;`k@'(0-0.5<1;1 2-0.0<0;0-0.0~0;{0-0.0<0}[]);("-1";"1 2";,"0";,"0")]   / 2.4.1: -1.0, 1.0 2.0, 0.0, 0.0
t[`strandMinusFloatParse;`k@`p"0-0.5<1";"(-;0;(<;0.5;1))"]
t[`strandSpaceFloat;`k@'(1 -0.5;1 2.5;1 -0.5<1);("1.0 -0.5";"1.0 2.5";"0 1")]   / with a space it is one strand, floats if any item is
t[`strandMinusKinds;`k@'(1 -2-0.5<1;01b-0.5<1);("0 -3";"-1 0")]   / a spaced strand or a boolean before the - (2.4.1: floats)
tv[`strandMinusUses;"(\"abc\"@2-0.5<1;!3-0.5<1)";("b";0 1)]   / as an index and a count (2.4.1: 'type)
t[`strandMinusIntKinds;`k@'(0N-0.5<1;1-0xef;0-1 0.5<1);("9223372036854775807";"18";"0 -1")]   / an int null wraps, hex is an int, a spaced strand after the - (2.4.1: floats)
t[`strandMinusBigInt;ER"99999999999999999999-0.5";"'limit"]   / an int literal past 64 bits is 'limit, as on its own (2.4.1 read it as a float here)
t[`strandMinusDate;((1-2024.01.01)~1976.01.02;(`i$1-12:00:00.000)~-43199999);11b]   / an int minus a date or a time (2.4.1: the int read as a float and its bits used, 1976.01.01 and -12:00:00.000, -43200000 ms)

/ ---- assigning a table's cell by row amends just that column ----
/ t[i;c]:y made a list of every row and a table of it again, so it took time in proportion to the rows.
/ That rebuild drops each column's attribute and makes a list of dicts with the same keys a table.
raT:+`a`b`c!(1 2 3;4.5 5 6;`x`y`z)
t[`rowAssign;{x[1;`a]:9;x}raT;+`a`b`c!(1 9 3;4.5 5 6;`x`y`z)]
t[`rowAssignType;{x[1;`a]:9.5;x}raT;+`a`b`c!((1;9.5;3);4.5 5 6;`x`y`z)]
t[`rowAssignList;{x[0;`c]:1 2;x}raT;+`a`b`c!(1 2 3;4.5 5 6;(1 2;`y;`z))]
t[`rowAssignPlus;({x[2;`b]+:1;x}raT;{x[1;`a]-:1;x}raT);(+`a`b`c!(1 2 3;4.5 5 7;`x`y`z);+`a`b`c!(1 1 3;4.5 5 6;`x`y`z))]
t[`rowAssignFunc;.[raT;(1;`c);{`q}];+`a`b`c!(1 2 3;4.5 5 6;`x`q`z)]
t[`rowAssignShared;{u:x;x[1;`a]:9;(x;u)}raT;(+`a`b`c!(1 9 3;4.5 5 6;`x`y`z);raT)]
t[`rowAssignDeep;.[+`a`b!(1 2;(3 4;5 6));(1;`b;0);:;9];+`a`b!(1 2;(3 4;9 6))]
t[`rowAssignSymJoin;{x[1;`b],:0;x}[+`a`b!(1 2;`x`y)];+`a`b!(1 2;(`x;(`y;0)))]
t[`rowAssignDupName;{x[0;`a]:9;x}[+`a`a!(1 2;3 4)];+`a`a!(9 2;3 4)]
t[`rowAssignNewCol;{x[1;`d]:9;x}raT;(`a`b`c!(1;4.5;`x);`a`b`c`d!(2;5.0;`y;9);`a`b`c!(3;6.0;`z))]
t[`rowAssignAttrs;(`at'. +{x[1;`a]:9;x}[+`a`b!(`sa 1 2 3;`ua 4 5 6)];`at'. +{x[`a;1]:9;x}[+`a`b!(`sa 1 2 3;`ua 4 5 6)]);(``;``u)]
t[`rowAssignDictsTable;{x[0;`b]:9;x}{x[`a;2]:`x`y!7 8;x}[+`a`b!((`x`y!1 2;`x`y!3 4;`z!,5);1 2 3)];+`a`b!(+`x`y!(1 3 7;2 4 8);9 2 3)]
t[`rowAssignOneDict;({x[0;`a]:`x`y!1 2;x}[+`a`b!(,1;,2)];@'. +{x[`a;0]:`x`y!1 2;x}[+`a`b!(,1;,2)]);(+`a`b!(+`x`y!(,1;,2);,2);`A`I)]
te[`rowAssignIndex;".[raT;(3;`a);:;9]"]
te[`rowAssignTypeErr;".[raT;(1;`a);+;`x]"]
/ ---- a nested assignment at many keys finds them all at once ----
/ d[k;i]:y found each key on its own, by a linear search, so this was quadratic. The keys still go in
/ order and the values squeeze as they did, after each atom and not at the end.
nkD:`a`b`c!(1 2 3;4 5 6;7 8 9)
t[`nestKeys;{x[`c`a;1]:0;x}nkD;`a`b`c!(1 0 3;4 5 6;7 0 9)]
t[`nestKeysList;.[nkD;(`c`a;1);:;10 20];`a`b`c!(1 20 3;4 5 6;7 10 9)]
t[`nestKeysRepeat;.[nkD;(`a`a`b;0);+;1];`a`b`c!(3 2 3;5 5 6;7 8 9)]
t[`nestKeysNew;{x[`c`d;0]:5;x}nkD;`a`b`c`d!(1 2 3;4 5 6;5 8 9;5 0N 0N)]
t[`nestKeysDeep;.[`a`b!(2 2#!4;2 2#4+!4);(`b`a;1;0);:;9];`a`b!((0 1;9 3);(4 5;9 7))]
t[`nestKeysNoSqueeze;@.{x[`a`b;`y]:9;x}`a`b!(`x`y!1 2;(,`x)!,3);`A]
t[`nestKeysInt;.[1 2 3!(1 2;3 4;5 6);(3 1;0);:;7 8];1 2 3!(8 2;3 4;7 6)]
t[`nestKeysChar;.["ab"!(1 2;3 4);("ba";1);,;5];"ab"!((1;2 5);(3;4 5))]
t[`nestKeysDup;.[`a`a`b!(1 2;3 4;5 6);(`a`b;0);:;9];`a`a`b!(9 2;3 4;9 6)]
t[`nestKeysTable;.[+`a`b!(1 2;3 4);(`b`a;1);:;9];+`a`b!(1 9;3 9)]
t[`nestKeysShared;{u:x;x[`a`b;0]:9;(x;u)}nkD;(`a`b`c!(9 2 3;9 5 6;7 8 9);nkD)]
t[`nestKeysDictVal;.[nkD;(`a`b;1);:;`a`b!8 9];`a`b`c!(1 8 3;4 9 6;7 8 9)]
t[`nestKeysMany;{r:.[x!(#x)#,1 2 3;(x;1);+;!#x];(r`k0;r`k1999;#r)}`$"k",'$!2000;(1 2 3;1 2001 3;2000)]
te[`nestKeysAtoms;".[`a`b!1 2;(`a`b;0);:;9]"]
te[`nestKeysLength;".[nkD;(`a`b;0);:;5 6 7]"]

/ ---- a 'stack or 'rank error gives back the call depth it took ----
tcDeep:{$[x>0;1+o x-1;0]}
t[`stackErrorsTrapped;{.[tcDeep;,x;{-1}]}'1000#3000;1000#-1]
tv[`stackDepthAfterErrors;"tcDeep 1500";1500]
tcRank:{x+y}
t[`rankErrorsTrapped;{.[tcRank;1 2 3;{-1}]}'!1000;1000#-1]
tv[`stackDepthAfterRankErrors;"tcDeep 1500";1500]

/ ---- & and | of float atoms next to minus the smallest normal double ----
t[`minMaxNegMinNormal;{(x&x;x|x;0.0&x;1.0&x;-1.0|x;-0.0&x;0n|x)}@-2.2250738585072014e-308;7#-2.2250738585072014e-308]
t[`minMaxNegMinNormalPair;{(x&y;x|y;y&x;y|x)}[-2.225073858507201e-308;-2.225073858507202e-308];-2.225073858507202e-308 -2.225073858507201e-308 -2.225073858507202e-308 -2.225073858507201e-308]

/ ---- an empty index takes the prototype from the list's first item ----
/ (an empty index made by n_n#1 has a stale 1 past its end)
t[`emptyIdxStale;{x:(1 2;"ab");?{[x;n]`k x@n_n#1}[x]'1+!20}[];,"0#,0N 0N"]
t[`emptyIdxStaleDict;{x:(`c`d!3 4;`x);?{[x;n](x@n_n#1)~0#x}[x]'1+!20}[];,1]
/ the stale slot made ~ of two such results depend on what ran before
t[`emptyIdxMatch;{d:`a`a0!(,0#,,();0#,,());ks:`a`a0;ks_d;{[d;w]((!d)@w)!(.d)@w}[d;&^ks?!d];(ks_d)~{[d;w]((!d)@w)!(.d)@w}[d;&^ks?!d]}[];1]
t[`emptyIdxEmptyList;(`k@()@!0;`k@(0#,,())@!0);("()";"0#,,()")]

/ ---- an elided amend, or a lambda's each, with a keyed table is 'nyi, not a crash ----
ktA:(+(,`k)!,1 2)!+(,`v)!,3 4
te[`amendKeyedEmptyTable;"@[0#+`a`b!(1 2;3 4);::;:;ktA]"]
te[`amendKeyedTable;"@[+`a`b!(1 2;3 4);::;:;ktA]"]
te[`amendKeyedInts;"@[!0;::;:;ktA]"]
te[`amendKeyedList;"@[();::;:;ktA]"]
te[`amendKeyedPlus;"@[1 2;::;+;ktA]"]
te[`eachKeyedThree;"{x+y+z}'[ktA;ktA;ktA]"]
t[`amendDictElided;(@[1 2;::;:;`a`b!1 2];@[1 2;::;+;`a`b!3 4]);(`a`b!1 2;`a`b!4 6)]

/ ---- each over dicts whose keys cannot be joined is 'domain ----
/ The values line up on the union of the keys; keys of another type are an
/ error, which leaked the union built so far (the loop takes that path many
/ times; the leak itself shows only in the process's memory use).
t[`eachDictsUnion;({x,y}'[`a`b!1 2;`b`c!3 4];{x,y,z}'[`a!1;`b!2;`a`c!3 4]);(`a`b`c!(1 0N;2 3;0N 4);`a`b`c!(1 0N 3;0N 2 0N;0N 0N 4))]
t[`eachDictList;{x,y}'[`a`b!1 2;3 4];`a`b!(1 3;2 4)]
t[`eachDictsKeyTypes;(.[{x,y}';(`a`b!1 2;1 2!3 4);{x}];.[{x,y,z}';(`a`b!1 2;`c`d!1 2;1 2!3 4);{x}]);2#,"'domain\n"]
t[`eachDictsKeyTypesLoop;+/{.[{x,y,z}';(`a`b!1 2;`c`d!1 2;(x+1 2)!3 4);{1}]}'!10000;10000]

/ ---- an int index refused by the check is released once ----
/ (a 0N, or an int beyond 32 bits, is not packed in the pointer, so a second release freed it)
t[`amendKeepHeapIndex;{f:{L5[0N;`a]:7};L5::(1 2;3 4);r:{.[x;,0;{`err}]}'20#f;(?r;.[.:;,"L5";{`gone}])}[];(,`err;(1 2;3 4))]
t[`amendKeepHeapIndexByName;{V7::1 2 3;f:{.[`V7;0N;+;(1 2;3 4)]};r:{.[x;,0;{`err}]}'20#f;(?r;V7)}[];(,`err;1 2 3)]
t[`amendKeepHeapIndexWide;{f:{L8[4294967296;`a]:7};L8::(1 2;3 4);r:{.[x;,0;{`err}]}'20#f;(?r;.[.:;,"L8";{`gone}])}[];(,`err;(1 2;3 4))]

/ ---- an amend by name with a symbol list as the name, refused by the check, does not crash ----
G1:1 2 3;N1.v:4 5 6
t[`amendByNameListRefused;{r:{.[x;,0;{`err}]}'20#({.[,`G1;,5;:;9]};{.[`N1`v;,0 1;:;7 8 9]});(?r;G1;N1.v;. "1+1")}[];(,`err;1 2 3;4 5 6;2)]   / the name was released twice

/ ---- float literals, `F$ and JSON numbers are correctly rounded (issue #5) ----
/ fb gives a float's 64 bits, high byte first, so these check the exact double read
fb:{8#|-8!x}
t[`floatRound1e308;fb 1e308;0x7fe1ccf385ebc8a0]
t[`floatRoundTieEven;fb 9007199254740993.0;0x4340000000000000]   / 2^53+1, halfway: ties to even
t[`floatRoundMax;fb 1.7976931348623157e308;0x7fefffffffffffff]   / the largest double, not 0w
t[`floatRound1em308;fb 1e-308;0x000730d67819e8d2]
t[`floatRoundSubMax;fb `F$"2.2250738585072011e-308";0x000fffffffffffff]   / the largest subnormal
t[`floatRoundSubMin;fb 5e-324;0x0000000000000001]
t[`floatRoundHalfMinUp;fb `F$"2.4703282292062328e-324";0x0000000000000001]   / just over half the smallest subnormal
t[`floatRoundHalfMinDown;fb `F$"2.4703282292062327e-324";0x0000000000000000]
t[`floatRound2e53Exact;fb 1801439850948199.0;0x431999999999999c]   / below 2^53, but its digits with the .0 are not: the old reader rounded twice
t[`floatRoundBig;fb 4.3501401714533576e18;0x43ce2f66b80d512f]
/ more than 780 significant digits: the ones past that still decide a tie (here the digit 801 places after the point)
t[`floatRoundLongTie;fb `F$"9007199254740993.",800#"0";0x4340000000000000]
t[`floatRoundLongSticky;fb `F$"9007199254740993.",(800#"0"),"1";0x4340000000000001]
t[`floatRoundLongInt;fb `F$"9007199254740993",(790#"0"),"1e-791";0x4340000000000001]
t[`floatRoundEmptyExp;(1.5e;1e-);1.5 1.0]   / an exponent with no digits counts as 0, as before
/ JSON numbers go through the same reader
t[`jsonRound;fb'`j?"[0.3,2.5e-5,0.123456789]";(0x3fd3333333333333;0x3efa36e2eb1c432d;0x3fbf9add3739635f)]
t[`jsonRoundMax;fb `j?"1.7976931348623157e308";0x7fefffffffffffff]
t[`jsonRoundSub;fb `j?"5e-324";0x0000000000000001]   / was 'parse (q: these bits too)
t[`jsonRoundHuge;`j?"[1e309,-1e309,1e-400]";(0w;-0w;0.0)]   / were 'parse; q: 0w -0w 0
t[`jsonRoundExp;`j?"-0.5E+2";-50.0]
te[`jsonRoundNoExpDigits;"`j?\"[1e,2]\""]   / an exponent needs a digit, as in q
te[`jsonRoundTwoSigns;"`j?\"1e+-5\""]   / one sign, as in q (the old reader read about 1e-5)
t[`floatRoundLongOffset;fb `F$"0.",(400000#"0"),"17976931348623157e400309";0x7fefffffffffffff]   / a long exponent against a long offset (17 digits)
t[`floatRoundLongDigitsOffset;fb `F$"0.",(200000#"0"),(26#"1"),"e200030";0x45f6704f4fab27ec]   / 26 digits, past Clinger's step: decided from the first 19 digits, with the long exponent and the long offset summed
t[`floatRoundHugeExp;`k@(1e9999999999999999999;1e-9999999999999999999;`F$"1e9999999999999999999";`F$"-1e-9999999999999999999");"0w 0.0 0w -0.0"]   / an exponent past 64 bits saturates (it wrapped, reading backwards)
t[`jsonRoundInRange;`j?"1000e-310";1e-307]   / was 'parse: only the exponent was past 308
t[`jsonRoundLong;fb'`j?"[3.1000000000114159265358979,-0.1000000000000000055511151231257827021181583404541015625,0.000000000000000000000000123456789012345678901]";(0x4008cccccccd3137;0xbfb999999999999a;0x3ac31aa094e075f3)]   / more than 19 digits: decided by the first 19
t[`jsonRoundLongTie;fb'`j?"[9007199254740993.0000000000000000000,9007199254740993.000000000000000000001]";(0x4340000000000000;0x4340000000000001)]   / 2^53+1 in 35 and 37 digits: a tie (to even), and just past one
t[`floatRound20Nines;fb `F$"9999999999999999999.9";0x43e158e460913d00]   / 20 digits: the first 19, plus one, make 10^19

hexpect[563]
hreport"AMBER CORE"
$[FAILN;`exit 1;]

