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

hexpect[169]
hreport"AMBER CORE"
$[FAILN;`exit 1;]

