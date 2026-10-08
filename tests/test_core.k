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
IK:{[set;e]. set;r:.[.:;,e;{`err}];(r;.[.:;,*":"\set;{`lost}])}   / as IA, but a variable the amend deleted reads as `lost, not a 'value that stops the suite
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
/ a list of more than 255 constants inside another list folds to one constant, as one alone does (ngn/k); it was 'limit
t[`nestedConstList;{.[.:;,x;{x}]}'("#((",(L[300;"1"]),");0)";"(0;(0;(",(L[300;"2"]),")))";"{((",(L[300;"3"]),");x)}[4]");(2;(0;(0;300#2));(300#3;4))]

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
t[`litDateBad;TL'("2026.02.29";"1900.02.29";"2026.13.01";"2026.00.10";"2026.04.31";"18446744073709551616.02.29");6#`$"'parse"]
t[`litTimeBad;TL'("10:61:00.000";"10:00:60.000";"2026.07.30D10:60:00");3#`$"'parse"]
t[`litStampDateBad;TL"2026.02.30D10:00:00";`$"'parse"]
t[`litStampHour24;TL"2026.07.30D24:00:00.000";`i$2026.07.31D00:00:00.000]
t[`litStampHour99;TL"2026.07.30D99:00:00.000";`i$2026.08.03D03:00:00.000]
t[`litTemporalGood;TL'("2024.02.29";"2000.02.29";"2026.04.30";"24:00:00.000";"99:00:00.000");8825 59 9616 86400000 356400000]
/ a time is 32 bits of milliseconds: a literal past 596:31:23.647 is 'limit, as an int literal past 64 bits (issue #62; it wrapped)
t[`litTimeLimit;TL'("596:31:23.648";"1000:00:00.000";"18446744073709551616:00:00.000");3#`$"'limit"]
t[`litTimeTop;TL'("596:31:23.647";"00000000000000000001:00:00.000");2147483647 3600000]
t[`mktLimit;(.[{`mkt x};,2147483648;{(x?"\n")#x}];`i$`mkt -2147483648);("'limit";-2147483648)]
/ a date is 32 bits of days: a literal past 5881610.07.11, and `mkd past 32 bits, is 'limit, as "D"$ (they wrapped:
/ 100000000.01.01 read as -5832990.07.06)
t[`litDateLimit;TL'("5881610.07.12";"9999999.12.31";"100000000.01.01";"18446744073709551616.01.01");4#`$"'limit"]
t[`litDateTop;TL'("5881610.07.11";"00000000000000000002026.07.30");2147483647 9707]
t[`mkdLimit;(.[{`mkd x};,2147483648;{(x?"\n")#x}];.[{`mkd x};,-2147483649;{(x?"\n")#x}];`i$`mkd -2147483648);("'limit";"'limit";-2147483648)]
/ a timestamp is 64 bits of nanoseconds: a literal past 2292.04.10D23:47:16.854775807, or before 1707.09.22D00:12:43.145224192
/ (0N's), is 'limit, as "P"$ (digest #93; it wrapped: 2300.01.01D00:00:00.000 read as 1715.06.13D00:25:26.290448384)
t[`litStampLimit;TL'("2300.01.01D00:00:00.000";"2292.04.10D23:47:16.854775808";"1707.09.22D00:12:43.145224191";"5881610.07.11D00:00:00";"2000.01.01D18446744073709551617:00:00");5#`$"'limit"]
t[`litStampTop;TL'("2292.04.10D23:47:16.854775807";"1707.09.22D00:12:43.145224193";"1707.09.22D00:12:43.145224192";"2000.01.01D2562047:47:16.854775807");9223372036854775807 -9223372036854775807 0N 9223372036854775807]
/ a minute, second, month or day of 20 digits is 'parse, as in "T"$, "D"$ and "P"$ (it wrapped: 10:18446744073709551617:00.000 read as 10:01:00.000)
t[`litFieldDigits;TL'("10:18446744073709551617:00.000";"10:00:18446744073709551617.000";"2000.01.01D10:18446744073709551617:00";"2000.01.01D10:00:18446744073709551617";"2000.18446744073709551617.01";"2000.01.18446744073709551617";"2000.18446744073709551617.01D00:00");7#`$"'parse"]
t[`litFieldZeros;TL'("10:0000000000000000000000001:00.000";"0002000.0000000000000000000001.0000000000000000000002";"2000.01.01D0000000000000000000000001:00");36060000 1 3600000000000]
/ `mkd, `mkt and `mkp take an int atom: of anything else they read its bits (issue #62)
t[`mkTemporalType;ER'("`mkd {x}";"`mkt 5.5";"`mkp 0x05";"`mkd 1 2");4#,"'type"]
/ "T"$, "D"$ and "P"$ read the text as the literal reader does, and refuse text that is not a time, a date or a timestamp,
/ and fields out of range, with 'parse (issue #62; they rolled over: 10:61 was 11:01, 2026.02.29 was 2026.03.01;
/ "T"$"abc" was midnight, and "D"$"abc" a date that changed from run to run); past the type's range 'limit
TX:{.[{`i$x$y};(x;y);{(x?"\n")#x}]}
t[`timeTextBad;TX["T"]'("10:61:00.000";"10:00:60.000";"abc";"**:00:00.000";"";"10:-5:00";"--10:00";"10:00:00.000:00";"10:00:00.5x";"10 :00";"10:";":10";"- 10:00";"10:00 x";"   ");15#,"'parse"]
t[`timeTextGood;TX["T"]'("10";"10:00";"10:00:00.";"10.5";"-10:00";"23:59:59.999";"99:00";"1:2:3";"-596:31:23.648";"596:31:23.647";"00000000000000000001:00");36000000 36000000 36000000 36000500 -36000000 86399999 356400000 3723000 -2147483648 2147483647 3600000]
t[`timeTextLimit;TX["T"]'("596:31:23.648";"-596:31:23.649";"1000:00:00.000";"99999999999999999999999");4#,"'limit"]
t[`dateTextBad;TX["D"]'("2026.02.29";"1900.02.29";"2026.13.01";"2026.00.10";"2026.04.31";"abc";"2026.01";"2026.01.01.01";"2026-01-01";"";"2026.1.x";"-1.02.29";"--1.01.01";"18446744073709551616.02.29");14#,"'parse"]
t[`dateTextGood;(TX["D"]'("2024.02.29";"2026.1.1";"2000.01.01";"-4.02.29";"0.02.29");TX["D";"99999999.01.01"]);(8825 9497 0 -731887 -730426;"'limit")]
t[`stampTextBad;TX["P"]'("2026.07.30D10:61:00.000";"2026.02.29D10:00:00.000";"abc";"2026.07.30D-01:00:00.000";"2026.07.30x10:00";"2026.07.30D10:00:00.5x";"2026.07.30D10:");7#,"'parse"]
t[`stampTextGood;TX["P"]'("2026.07.30";"2026.07.30D";"2026.07.30D24:00:00.000";"2026.07.30D99:00";"2026.07.30D09:30:00.123456789";"1707.09.22D00:12:43.145224192");838684800000000000 838684800000000000 838771200000000000 839041200000000000 838719000123456789 0N]
t[`stampTextLimit;TX["P"]'("2026.07.30D2562047:00";"-1.01.01D10:00");2#,"'limit"]
t[`temporalTextType;(TX["T";"5"];TX["T";("10:00";"11:00")];TX["D";`a];TX["P";1]);4#,"'type"]
/ blanks around the text are skipped, as q skips them: spaces, tabs and carriage returns, not newlines; not inside the text
t[`temporalTextBlanks;(TX["T"]'(" 10:00";"10:00 ";" \t-10:00\r");TX["D"]'(" 2026.1.1";"2026.1.1\t");TX["P"]'(" 2026.07.30D10:00 ";"2026.07.30D ";"2026.07.30 "));(36000000 36000000 -36000000;9497 9497;838720800000000000 838684800000000000 838684800000000000)]
t[`temporalTextBlanksBad;(TX["T"]'("10:00\n";"\n10:00");TX["D"]'("2026 .1.1";"  ");TX["P"]'("2026.07.30 D10:00";"2026.07.30D 10:00";"  "));(2#,"'parse";2#,"'parse";3#,"'parse")]
/ ^ of midnight, 2000.01.01 and the epoch timestamp is 0, as q's null (issue #62; they were taken for nulls). A date or a time
/ has no null of its own; a timestamp's is 0N's nanoseconds, which prints as 0N
t[`nullTemporalZero;(^00:00:00.000;^2000.01.01;^2000.01.01D00:00:00.000000000;^(00:00:00.000;2000.01.01;01:00:00.000));(0;0;0;0 0 0)]
t[`nullStamp;(^1707.09.22D00:12:43.145224192;^`mkp 0N;^(`mkp 0N;2000.01.01D00:00:00.000000001));(1;1;1 0)]

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

/ ---- `prng n replays the stream whatever was drawn before it: a full-range roll (and so a char roll or a roll
/ of a power of two) left its buffered word behind, and the next draw after the reseed took it ----
t[`prngReplayFullRoll;{2?0;`prng 5;c:3?1.0;`prng 5;c~3?1.0}[];1]
t[`prngReplayEach;{p:("2?0";"3?-1";"3?256";"3?65536";"3?4294967296";"3?\" \"";"5?0x00";"3?100";"3?100000";"3?10000000000";"1?1.0";"-3?10";"-3?10000000000";"0N?10";"3?`a`b`c";"-2?`a`b`c";"3?\"a\"")
 d:("3?1.0";"3?10";"3?256";"3?100000";"3?10000000000";"3?0";"-3?10";"-3?10000000000";"0N?5";"3?`a`b`c";"-2?`a`b`c";"3?\"a\"";"2?\" \"")
 ,/{[d;x]{[x;y].x;`prng 5;c:.y;`prng 5;c~.y}[x]'d}[d]'p}[];(17*13)#1b]

/ ---- a full-range roll uses up its words (#83 Q5): the next draw does not reuse the roll's last word (a float
/ draw was its low 52 bits), and saving and restoring `prng[] around any draw replays it ----
t[`prngRollFresh;{r:4?0;f:*1?1.0;(4503599627370496!r 3)~_f*4503599627370496}[];0]
t[`prngRestoreFullRoll;{2?0;s:`prng[::];c:3?1.0;`prng s;c~3?1.0}[];1]
t[`prngRestoreEach;{p:("2?0";"3?-1";"3?256";"3?65536";"3?4294967296";"3?\" \"";"5?0x00";"3?100";"3?100000";"3?10000000000";"1?1.0";"-3?10";"-3?10000000000";"0N?10";"3?`a`b`c";"-2?`a`b`c";"3?\"a\"";"0?0")
 d:("3?1.0";"3?10";"3?256";"3?100000";"3?10000000000";"3?0";"-3?10";"-3?10000000000";"0N?5";"3?`a`b`c";"-2?`a`b`c";"3?\"a\"";"2?\" \"")
 ,/{[d;x]{[x;y].x;s:`prng[::];c:.y;`prng s;c~.y}[x]'d}[d]'p}[];(18*13)#1b]

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
/ a bool token run into an int before a float strand (no space) is a noun of its own that the strand indexes, as before an
/ int strand and in ngn/k; run into a float it stays 'value, as in ngn/k, which reads each token of a strand by itself
tv[`boolRunFloat;"(ER'(\"01b1 2.5\";\"01B1 0w\");`k@{x}[101b1 2 32.5+])";(2#,"'type";"1 0 11.0 2.0 32.5+")]   / 2.7.2: 'value; 101b1 2 3+ is "1 0 11 2 3+"
t[`boolRunFloatTok;ER'("01b2.5";"01b0w";"01b0n";"01b1e2";"01b1f";"1b1.5");6#,"'value"]

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

/ |/ and &/ of floats give 0.0 for a zero, as | & and the scans do (the SIMD kernel kept a lane's zero), threaded too
t[`foldZeros;1%(|/-0.0 0.0;|/0.0 -0.0;&/-0.0 0.0;&/0.0 -0.0;|/-0.0 -0.0;|/20#-0.0 0.0;&/20#0.0 -0.0;|/,-0.0);8#0w]
t[`foldZerosBig;1%(|/2000000#-0.0;&/2000000#-0.0 0.0);0w 0w]
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

/ ---- an int atom filling floats gives floats, as q (2.7.1's fast path; 2.7.0 and ngn/k: (1.5;0)) ----
t[`fillIntFloats;(0^0n;0^1.5 0n;0^(1;0n);1b^1.5 0n;0N^1.5 0n;0^`a`b!1.5 0n);(0.0;1.5 0.0;(1;0.0);1.5 1.0;1.5 0n;`a`b!1.5 0.0)]
/ other fills of another type still make a generic list, as k; a null int atom becomes the fill
t[`fillOtherType;(1^0N;0.5^0N;0.5^1 0N;0x00^1.5 0n;`a^1.5 0n);(1;0.5;(1;0.5);(1.5;0x00);(1.5;`a))]

/ ---- an int atom filling float nulls gives its value as a float, -2147483648 too (it gave 0n) ----
t[`fillFloatMinI32;(-2147483648^0n;-2147483648^0n 1.5;-2147483648^(0n;`a);-2147483648^`a`b!0n 1);(-2147483648.0;-2147483648 1.5;(-2147483648.0;`a);`a`b!-2147483648 1.0)]
t[`fillFloatIntNull;(0N^0n;0N^0n 1.5;-2147483647^0n;-2147483649^0n);(0n;0n 1.5;-2147483647.0;-2147483649.0)]

/ ---- a missing or failing temporal formatter is an error, not the end of the process or a crash ----
/ a projection, composition or derived verb holding a date, and JSON, pass the error up too
/ ---- the core prints a temporal atom itself, with no library loaded (temporal.k's dstr and pstr, amber.k's stime) or with one ----
/ (it was 'value) the same text as the library's, and it reads back
tv[`fmtMissing;"$2026.07.30";"2026.07.30"]
tv[`fmtMissingK;"`k@'(2026.07.30;12:34:56.789;2026.07.30D01:02:03.000000004;0001.01.01;1707.09.22D00:12:43.145224192)";("2026.07.30";"12:34:56.789";"2026.07.30D01:02:03.000000004";"1.01.01";"0N")]
tv[`fmtMissingEdges;"`k@'(`mkd -2147483648;`mkd 2147483647;`mkt -1;`mkt 359999999;`mkp -1;`mkp 1)";("-5877611.06.22";"5881610.07.11";"-00:00:00.001";"99:59:59.999";"1999.12.31D23:59:59.999999999";"2000.01.01D00:00:00.000000001")]
tv[`fmtMissingHours;"`k@'(100:00:00.000;596:31:23.647)";("100:00:00.000";"596:31:23.647")]   / every digit of the hours (#62)
tv[`fmtMissingBack;"{x~. `k@x}'(2026.07.30;12:34:56.789;2026.07.30D01:02:03.000000004;(2026.07.30;10:00:00.000);100:00:00.000)";1 1 1 1 1]
/ a projection, composition or derived verb holding a date, and JSON, print it too
tv[`fmtMissingProj;"`k@{x+y}[2026.07.30]";"{x+y}[2026.07.30;]"]
tv[`fmtMissingDerived;"`k@(2026.07.30+)";"2026.07.30+"]
tv[`fmtMissingString;"$({x+y}[2026.07.30])";"{x+y}[2026.07.30;]"]
tv[`fmtMissingJson;"`j@2026.07.30";"\"2026.07.30\""]
tv[`fmtMissingJsonList;"`j@(1;2026.07.30)";"[1,\"2026.07.30\"]"]
t[`fmtMissingGrade;<({x+y}[2026.07.30];{x+y}[2026.07.31]);0 1]
/ ---- printing a temporal does not call dstr, stime or pstr: a program's own, even a failing one, changes nothing (issue #18) ----
tv[`fmtFails;"dstr::{x+`a};.[{$x};,2026.07.30;{`caught}]";"2026.07.30"]
tv[`fmtFailsList;"dstr::{x+`a};`k@(2026.07.30;1)";"(2026.07.30;1)"]
tv[`fmtOwnFormatters;"dstr::{\"d\"};stime::{\"t\"};pstr::{\"p\"};`k@'(2026.07.30;12:34:56.789;2026.07.30D01:02:03.000000004;1707.09.22D00:12:43.145224192)";("2026.07.30";"12:34:56.789";"2026.07.30D01:02:03.000000004";"0N")]


tv[`fmtFailsProj;"dstr::{x+`a};`k@{x+y}[2026.07.30]";"{x+y}[2026.07.30;]"]
tv[`fmtFailsJson;"dstr::{x+`a};`j@`a`b!(1;2026.07.30)";"{\"a\":1,\"b\":\"2026.07.30\"}"]

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
/ A dict with no keys on one side gives the other one, as q and ngn/k (it was 'length, digest #24)
t[`dictEmptyUntyped;((`a`b!1 2)+()!();((1 2;3 4)!5 6)+()!();(()!())-`a`b!1 2;(()!())+()!());(`a`b!1 2;(1 2;3 4)!5 6;`a`b!-1 -2;()!())]
t[`dictEmptyTyped;(`a`b!1 2)+(0#`)!!0;`a`b!1 2]
t[`dictEmptyCompare;((`a`b!1 2)<()!();(`a`b!1 2)>()!();(`a`b!1 2)=()!();(()!())<`a`b!1 2;(()!())>`a`b!1 2;(()!())=`a`b!1 2;(`a`b!1.5 2)<()!();(()!())<`a`b!1.5 2);(`a`b!0 0;`a`b!1 1;`a`b!0 0;`a`b!1 1;`a`b!0 0;`a`b!0 0;`a`b!0 0;`a`b!1 1)]   / as with (0#`)!!0: a key on one side meets a null; it was 'length
t[`dictEmptyCompareVals;(((0#`)!())<`a`b!1 2;(`a`b!1 2)=(0#`)!();(`a`b!1 2)<(0#,`x)!();((1 2;3 4)!5 6)<()!();(()!())<()!();((0#`)!())<(0#`)!());(`a`b!1 1;`a`b!0 0;`a`b!0 0;(1 2;3 4)!0 0;()!();(0#`)!())]   / () values and generic keys of any prototype: it was `a`b!(!0;!0)
t[`dictEmptyListVals;((`a`b!(1 2;3 4))+()!();(`a`b!(1 2;3 4))|()!();(`a`b!+(,`c)!,1 2)+()!();(`a`b!(1 2;3 4))<()!();(()!())<`a`b!(1 2;3 4);(`a`b!+(,`c)!,1 2)<()!();(`a`b!1 2)<(0#`)!0#,1 2;(`a`b!+(,`c)!,1 2)+(0#`)!0#,1 2);(`a`b!(1 2;3 4);`a`b!(1 2;3 4);`a`b!+(,`c)!,1 2;`a`b!(0 0;0 0);`a`b!(1 1;1 1);`a`b!+(,`c)!,0 0;`a`b!0 0;`a`b!+(,`c)!,1 2)]   / as x f 0#x; it was 'length or (0 0;0 0)
t[`dictEmptyBothCompare;`k@(((0#`)!"")<(0#`)!"";((0#`)!0#0n)<(0#`)!0#0n;(()!())<(0#`)!!0;((0#`)!!0)<()!();((0#`)!())<(0#`)!!0;(()!0#0n)<()!0#0n;(()!0#0n)<()!();(()!())<()!0#0n;(()!0#0n)<(0#`)!();((0#`)!())<()!0#0n);"((0#`)!!0;(0#`)!!0;(0#`)!!0;(0#`)!!0;(0#`)!!0;()!!0;()!!0;()!!0;(0#`)!!0;(0#`)!!0)"]   / two empty dicts: typed ones as before, generic keys or values take the other's typed 0# in either order (+ - * % & | ,' are dkey's)
lk:(1 2;3 4)!5 6
t[`dictEmptyGenericKeys;(lk+(0#`)!!0;lk+(0#`)!();((0#`)!!0)+lk;((0#`)!!0)-lk;lk<(0#`)!!0;((0#`)!!0)<lk;lk<()!();lk,'(0#`)!!0;((0#`)!!0),'lk);((1 2;3 4)!5 6;(1 2;3 4)!5 6;(1 2;3 4)!5 6;(1 2;3 4)!-5 -6;(1 2;3 4)!0 0;(1 2;3 4)!1 1;(1 2;3 4)!0 0;(1 2;3 4)!5 6;(1 2;3 4)!5 6)]   / it was (1 2;3 4;`)!5 6 0N, 'type or (1 2;3 4)!(0 0;0 0)
t[`dictEmptyVals;(((0#`)!())-`a`b!1 2;(`a`b!1 2)+(0#`)!());(`a`b!-1 -2;`a`b!1 2)]   / empty values () filled nothing: `a`b!(!0;!0)
/ Two empty dicts: the values combine as lists do, (!0)+0#0n is 0#0n, whichever dict is on the left (x's came back)
t[`dictEmptyBothTyped;`k'{(x+y;y+x;y-x;y*x;y%x;y&x;y|x)}[(0#`)!0#0n;(0#`)!!0];7#,"(0#`)!0#0n"]
t[`dictEmptyBothEach;`k'(((0#`)!!0),'(0#`)!0#0n;(!0),'0#0n);("(0#`)!0#,(0N;0n)";"0#,(0N;0n)")]
t[`dictEmptyBothProto;`k'(((0#`)!0#0n)+(0#`)!0#,1 2;((0#`)!0#,1 2)+(0#`)!0#0n);2#,"(0#`)!0#,0n 0n"]   / 0#,1 2 is not ()
t[`dictEmptyBothType;*"\n"\.[.:;,"((0#`)!0#`)+(0#`)!!0";{x}];"'type"]   / as (0#`)+!0 and q
t[`dictEmptyBothKeys;`k'(((0#`)!!0)+(!0)!0#0n;((!0)!0#0n)+(0#`)!!0);("(0#`)!0#0n";"(!0)!0#0n")]   / x's keys, as for keys of different types with values (q: 'type)
/ ()!() gives the other dict, either way round, as q; () values alone take the list rule, () keys give way to y's
t[`dictEmptyBothUntyped;`k'{(x+y;y+x;x-y;y-x)}[()!();(0#`)!!0];4#,"(0#`)!!0"]
t[`dictEmptyBothUntypedSym;`k'{(x+y;y+x)}[()!();(0#`)!()];2#,"(0#`)!()"]   / q: (`symbol$())!() both ways
t[`dictEmptyBothUntypedVals;`k'{(x+y;y+x)}[(0#`)!();(0#`)!0#0n];2#,"(0#`)!0#,0#0n"]   / as ()+0#0n
t[`dictEmptyBothUntypedType;*"\n"\.[.:;,"((0#`)!())+(0#`)!0#`";{x}];"'type"]   / as ()+0#` (q: (`symbol$())!())
t[`dictEmptyBothUntypedKeys;`k'{(x+y;y+x)}[()!0#0n;(0#`)!!0];2#,"(0#`)!0#0n"]
t[`dictEmptyBothUntypedEach;`k'(((0#`)!()),'(0#`)!0#0n;((0#`)!0#0n),'(0#`)!();(()!()),'(0#`)!!0;((0#`)!!0),'()!();(()!0#0n),'(0#`)!!0);(2#,"(0#`)!0#,,0n"),(2#,"(0#`)!!0"),,"(0#`)!0#,(0n;0N)"]   / q: (`symbol$())!()

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
/ ---- binary search on symbols, in <'s order, as q's bin; a float list with a symbol is 'type, as an int list (#20 row 13) ----
tv[`binSyms;"(`a`c`e'`b;`a`c`e'`e`f`a`A;(0#`)'`a;`a`c`e'(`b;`d`f))";(0;2 2 0 -1;-1;(0;1 2))]
tv[`binSymsMany;"{(x'y)~{-1++/~y<x}[x]'y}[{x@<x}`$\"x\",/:$!3000;`$\"x\",/:$-50+!3100]";1b]
t[`binFloatSym;5#.[{(,0.5)'x};,`a;{x}];"'type"]


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
/ Digest #7: bits looked for are widened to bytes (they were read as ints: wrong rows, reads past the end)
t[`findBits;((,1)?0101010101B;(,0)?0101b;1 0?0101b;(,1)?0#01b);(0N 0 0N 0 0N 0 0N 0 0N 0;0 0N 0 0N;1 0 1 0;!0)]
t[`exceptBits;(0101010101B^1;11B^1;0101010101B^0;11B^0);(00000b;0#01b;11111b;11b)]
t[`exceptMixed;((0;" ")^0;(0;" ")^" ";(1;"a";`b)^"a";(1;"a")^1 2;(1;"a")^(1;"a");(1;"a";2)^1;(1;"a")^!0);(," ";,0;(1;`b);,"a";!0;("a";2);(1;"a"))]   / digest #64: a general list's chars against numbers are kept, not 'type
/ a range is its items: except between a range and chars removes nothing, as for the ints written out (2.7.2: 'type)
tv[`exceptRangeChars;"((!3)^\"a\";(1+!3)^\"a\";(!1)^\"\";(!3)^\"ab\")";(0 1 2;1 2 3;,0;0 1 2)]
tv[`exceptCharsRange;"(\"abc\"^!3;\"abc\"^1+!3)";("abc";"abc")]
tv[`exceptGenericRange;"((1;\"a\")^!3;(1;\"a\";2)^1+!3)";(,"a";,"a")]   / a general list's numbers go, its chars stay
t[`exceptRangeNums;((!5)^3+!5;(!5)^!2;(!5)^1 3);(0 1 2;2 3 4;0 2 4)]   / the range fast paths (y covers an end of x) still run
/ Digest #6: > leaves stdin, stdout and stderr open (a failed write with no stderr to report it on looped or crashed)
t[`closeStdHandles;(7#.[{[d]>0};,0;{x}];7#.[{[d]>1};,0;{x}];7#.[{[d]>2};,0;{x}]);3#,"'domain"]
/ an int past 32 bits is no descriptor: close() took its low half, so >0N or >4294967296 closed stdin
t[`closeWideInt;(7#.[{[d]>0N};,0;{x}];7#.[{[d]>4294967296};,0;{x}];7#.[{[d]>-4294967296};,0;{x}]);3#,"'domain"]
t[`aimRange;(7#.[{[d]`aim[!5]};,0;{x}];7#.[{[d]`aim[1000+!3]};,0;{x}]);2#,"'length"]   / Digest #11: a range is its items, not its two stored ends (read as addresses)
t[`modBigFloat;(7!1e19;5!-0w;5!0w;7!-7.0;7!8.5;1%3!-0.0);(3.0;0n;0n;0.0;1.5;-0w)]   / Digest #15: the float remainder is exact, 0 to x (infinities: 0n)
t[`findEmptyGeneric;(-8!1.0 2.0?();-8!1000000 2?();5#.[{[d]1.0 2.0?"a"};,0;{x}]);(-8!1 2?();-8!1 2?();"'type")]   / Digest #30
t[`literalPoolType;(4_5#(-8!0110B;0 1 1 0)0;4_5#{(-8!0110B;0 1 1 0)}[]0);2#,,0x03]   / Digest #36: a bit list stays a bit list next to an equal int list
t[`dropSqueeze;({4_5#-8!-1_(0;1;())}[];(0 1)~-1_(0;1;());-1_(1;"a";2.5));(,0x04;1b;(1;"a"))]   / Digest #28: a negative drop squeezes in place too
t[`jsonNegZero;1%(`j?"-0";`j?"[-0,-0.0,0]");(-0w;-0w -0w 0w)]   / -0 keeps its sign
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
/ ---- a base of 8 or 9 divides like any other: its K text's (-8)! and (-9)! were serialise and deserialise ----
t[`encodeBase89;(2 8\5;2 9\5;16 8\20;8 8 8\511;-8 -9\100;2 8\5 6);(0 5;0 5;2 4;7 7 7;5 1;(0 0;5 6))]
t[`encodeAtomBase89;(8\100;9\100;-8\100;-9\-100);(1 4 4;1 2 1;2 4 4;1 8 3 8)]

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
/ a dict's or a table's values are checked too (they gave nulls), with a dict haystack as well
t[`findCharHayDict;.[.:;,"\"ab\"?`a`b!1 2";{*"\n"\x}];"'type"]
t[`findIntHayDict;.[.:;,"1 2?`a`b!\"ab\"";{*"\n"\x}];"'type"]
t[`findCharHayTable;.[.:;,"\"ab\"?([]a:1 2)";{*"\n"\x}];"'type"]
t[`findIntHayTable;.[.:;,"1 2?([]a:\"ab\")";{*"\n"\x}];"'type"]
t[`findCharHayKeyed;.[.:;,"\"ab\"?([k:`a`b]v:1 2)";{*"\n"\x}];"'type"]
t[`findDictHayDict;.[.:;,"(`a`b!1 2)?`a`b!\"ab\"";{*"\n"\x}];"'type"]
t[`findDictDeep;.[.:;,"\"ab\"?`a`b!(1;2)";{*"\n"\x}];"'type"]
t[`findListOfDicts;.[.:;,"\"ab\"?(`a`b!1 2;`c`d!3 4)";{*"\n"\x}];"'type"]
t[`findEmptyDict;.[.:;,"\"ab\"?(0#`)!!0";{*"\n"\x}];"'type"]
t[`findDictSame;("ab"?`a`b!"ba";1 2?`a`b!2 1;"ab"?([]a:"ba"));(`a`b!1 0;`a`b!1 0;+(,`a)!,1 0)]

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
/ blanks (space, tab, CR) round the digits are skipped and a leading + is read, as q (#20 row 4)
t[`parseIntBlanks;`I$(" 12";"+12";"  +12  ";"\t12\r";" -12";" +9223372036854775807";"++12";"-+12";"--12";"+-+-12");12 12 12 12 -12 9223372036854775807 12 -12 12 12]
t[`parseIntBlanksBad;`I$("+ 12";"+";" ";"\n12";"1 2";" +9223372036854775808";"-";"+-";"- 12");9#0N]


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

/ ---- `$ trims the blanks (space, tab, CR) round the text, as q; a JSON key keeps them (#20 row 3) ----
t[`symCastTrim;`$(" a ";" ";"\ta\r";"a b";"a\n");(`a;`;`a;`$"a b";`$"a\n")]
t[`symCastTrimChar;`$" ";`]
t[`symJsonKeyBlanks;`k@!`j?"{\" a \":1}";",`\" a \""]
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

/ ---- dict arithmetic: keys of different ranks are errors, not crashes; keys that are tables add over their union ----
t[`dictMixedRank;*"\n"\.[.:;,"(`a`b!1 2)+((1 2;3 4)!5 6)";{x}];"'domain"]
t[`keyedPlusKeyed;{a:(+(,`a)!,1 2 3)!4 5 6;b:(+(,`a)!,2 3 4)!10 20 30;a+b}[];(+(,`a)!,1 2 3 4)!4 15 26 30]
t[`keyedPlusKeyedCols;*"\n"\.[.:;,"((+(,`a)!,1 2)!3 4)+((+(,`b)!,1 2)!3 4)";{x}];"'domain"]   / key tables with different columns do not join
t[`dictListKeys;((1 2;3 4)!5 6)+(1 2;3 4)!7 8;(1 2;3 4)!12 14]
t[`keyedPlusAtom;((+(,`a)!,1 2 3)!4 5 6)+1;(+(,`a)!,1 2 3)!5 6 7]

/ the higher rank on the left crashed
t[`dictMixedRankL;*"\n"\.[.:;,"((1 2;3 4)!5 6)+`a`b!1 2";{x}];"'domain"]

/ a dict with no keys is not checked: two empty ones still add, as group counts from ()!() do
t[`temporalStrand;(2026.01.01 2026.01.02;12:00:00.000 13:00:00.000;#2026.01.01 2026.01.02 2026.01.03;1+2026.01.01 2026.01.02;5#.[.:;,"2026.01.01 12:00:00.000";{x}])~(2026.01.01+0 1;12:00:00.000+0 3600000;3;2026.01.02+0 1;"'type");1]   / Digest #65: a strand of one kind is a list, as q
t[`groupMixedRank;(=(1;"ab");=(1 2;1);=(1;"ab";1);(1;"ab")?"ab";((1;"ab")!10 20)"ab";5#.[?;(("ab";"cd");1);{x}]);((1;"ab")!(,0;,1);(1 2;1)!(,0;,1);(1;"ab")!(0 2;,1);1;20;"'rank")]   / Digest #29: as q
t[`dictEmptyBoth;`k'((()!())+#'=0#`;(#'=0#`)+()!());2#,"(0#`)!!0"]
/ Digest #23 #25 #26: a key on one side passes its value through; x's repeated keys stay
t[`dictOneSide;((`a`b!1 2)|`c!,1 2;(`a`b!(1 2 3;3))|`a`c!(5;6 7);(`a`b!0x0102)|`b`c!0x0304;(`a`b!0x0102)&`b`c!0x0304);(`a`b`c!(1;2;1 2);`a`b`c!(5 5 5;3;6 7);`a`b`c!0x010304;`a`b`c!0x010204)]
t[`dictRepeatKey;((`a`a!1 2)+`a!10;(`a`a!1 2)*`b!3);(`a`a!11 2;`a`a`b!1 2 3)]
t[`dictEachJoin;(`a`b!1 2),'(1 2!3 4);(`a;`b;1;2)!1 2 3 4]
t[`dictSameKeys;((`a`b!1 2)+`a`b!10 20;(`a`b!1 2)|`a`b!0 5;(`a`b!1 2.0)%`a`b!2 4);(`a`b!11 22;`a`b!1 5;`a`b!0.5 0.5)]
t[`dictOneSideDiv;((`a!1)%`b!2;(`a`b!1 2)%`b`c!4 8;(`a!1.5)%`b!2);(`a`b!1 2;`a`b`c!(1;0.5;8);`a`b!(1.5;2))]   / % passes a one-sided value through, as q (it took 1%y: `a`b!1.0 0.5)
t[`dictDisjointTypes;.[.:;,"((`a!1)|`b!`y;(`a!1)&`b!`y;(`a!1)%`b!`y;(`a!`x)|(,`b)!,1 2;((0#`)!!0)|(,`)!,`a;(`a`b!1 2)%`c!`y;(`a!2026.01.01)+`b!2026.01.02;(`a!12:00)|`b!13:00)";{x}];(`a`b!(1;`y);`a`b!(1;`y);`a`b!(1;`y);`a`b!(`x;1 2);(,`)!,`a;`a`b`c!(1;2;`y);`a`b!2026.01.01 2026.01.02;`a`b!12:00 13:00)]   / no key on both: no value meets another, so the types need not meet, as ngn/k (it was 'type; q passes the dates, times and (`a!`x)|(,`b)!,1 2 through, and is 'type for the int/symbol ones)
t[`dictDisjointShared;*"\n"\.[.:;,"(`a`b!1 2)|`b`c!`x`y";{x}];"'type"]   / a key on both still meets the verb
t[`dictDisjointNeg;.[.:;,"`i$(`a!2026.01.01)-`b!2026.01.02";{x}];`a`b!9497 -9498]   / a right-only value is negated, a date too, as q

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
/ Digest #27: temporal arithmetic outside the documented cases is 'type (issue #18), a null int 'domain, x:y is y
t[`temporalKept;(2026.07.30+1;1+2026.07.30;2026.07.30-1;2026.07.30-2026.07.29;10:00:00.000+00:00:01.000;2026.07.30&2026.07.29;2026.07.30<2026.07.31)~(2026.07.31;2026.07.31;2026.07.29;1;10:00:01.000;2026.07.29;1);1]
t[`temporalType;ER'("2026.07.30+2.3";"2026.07.30*2";"2026.07.30%2";"2!2026.07.30";"2026.07.30+2026.07.30");5#,"'type"]
t[`temporalNull;ER"2026.07.30+0N";"'domain"]
/ a date, time or timestamp against a float compares its stored number with the float rounded half away from zero, as q
/ casts it (the float's bits were read as an int: 2026.07.30<2.5 was 1, 2026.07.30=9707.0 0, 2026.07.30<0n 1); 0n is
/ below all, -0w 0w and floats past the int range compare as they are
t[`dateFloatCmp;(2026.07.30<2.5;2026.07.30=9707.0;2026.07.30<0n;0n<2026.07.30;2026.07.30=0n;2026.07.30<0w;2026.07.30>-0w;2026.07.30<9707.5;2026.07.30>9706.5;2.5<2026.07.30;9707.0=2026.07.30;2026.07.30=9707.4;2026.07.30<9707.4;9706.5<2026.07.30;2026.07.30=9706.6;1999.12.31=-0.5;2026.07.30=9707.5;1999.12.31=-1.5;1999.12.30=-1.5;2026.07.30<1e100;2026.07.30>-1e100;-1e100<2026.07.30;2026.07.30=1e10);0 1 0 1 0 1 1 1 0 1 1 1 0 0 1 1 0 0 1 1 1 1 0]
t[`timeFloatCmp;(12:00:00.000<2.5;12:00:00.000=43200000.0;0n<12:00:00.000;12:00:00.000=0n;12:00:00.000<0w;-0w<12:00:00.000;12:00:00.000=43200000.4;12:00:00.000=43199999.5;12:00:00.000>43199999.5;43200000.4>12:00:00.000;12:00:00.000<1e30);0 1 1 0 1 1 1 1 0 0 1]
t[`timestampFloatCmp;(2026.07.30D12:00:00.000000000=838728000000000000.0;2026.07.30D12:00:00.000000000<0w;2000.01.01D00:00:00.000000001=1.0;0n<2000.01.01D00:00:00.000000000;2000.01.01D00:00:00.000000000>0.5;2000.01.01D00:00:00.000000001=1.4;2000.01.01D00:00:00.000000001=0.5;2026.07.30D00:00:00.000000001=838684800000000000.0;2026.07.30D00:00:00.000000001>838684800000000000.0;838684800000000000.0<2026.07.30D00:00:00.000000001;2026.07.30D00:00:00.000000001<1e19;2026.07.30D00:00:00.000000001>-1e19;2026.07.30D00:00:00.000000001<9.3e18);1 1 1 1 0 1 1 0 1 1 1 1 1]
t[`temporalFloatListCmp;(2026.07.30=9707.4 9706.4;9707.4 9706.4=2026.07.30;2026.07.30 2026.07.31=9707.0;2026.07.30 2026.07.31=9707.4 9708.4;9707.0 0n>2026.07.30;0n 0w<12:00:00.000;0n 0.5<2000.01.01D00:00:00.000000000;2026.07.30D00:00:00.000000001>838684800000000000.0 1.4)~(1 0;1 0;1 0;1 1;0 0;1 0;1 0;1 1);1]
/ a general list of temporals against an atom or another list compares in one loop (tcl): the same answers as item by item,
/ and the item-by-item path where an item is another type (the float here), the lengths differ, or the list is empty
t[`temporalListLoop;((2026.07.30;2026.07.31;2026.08.01)<2026.07.31;9707<(2026.07.30;2026.07.31);(2026.07.30;2026.07.31)=(2026.07.31;2026.07.31);(2026.07.30;2026.07.31)>9707;(2026.07.30D00:00:00.000000001;2026.07.30D00:00:00.000000002)=2026.07.30D00:00:00.000000002;(12:00:00.000;13:00:00.000)<(13:00:00.000;12:00:00.000));(1 0 0;0 1;0 1;0 1;0 1;1 0)]
t[`temporalListLoopBack;((2026.07.30;2026.07.31;2.5)=2026.07.30;(2026.07.30;2026.07.31;9707.0)=2026.07.30;(0#,2026.07.30)=2026.07.30;@(2026.07.30;2026.07.31)=2026.07.30;(2026.07.30;2026.07.31;2026.07.30)=(2026.07.30;2026.07.31;2.5);9707<(2026.07.30;2026.07.31;2.5));(1 0 0;1 0 1;!0;`I;1 1 0;0 1 0)]
t[`temporalListLoopLength;ER"(2026.07.30;2026.07.31)=(2026.07.30;2026.07.31;2026.07.30)";"'length"]
/ two kinds compare as q (#83 Q9; their stored numbers were compared, days with ms or ns: 2026.07.30=12:00:00.000 was 0,
/ 2026.07.30<12:00:00.000 1): a date and a time are 'type, a date against a timestamp is its midnight, exactly (no date
/ wraps past the timestamp range), and a timestamp against a time is its time of day to the millisecond, as `time$
t[`temporalKindsType;ER'("2026.07.30=12:00:00.000";"2026.07.30<12:00:00.000";"12:00:00.000>2026.07.30";"(2026.07.30;2026.07.31)=12:00:00.000");4#,"'type"]
t[`dateStampCmp;(2026.07.30=2026.07.30D00:00:00.000000000;2026.07.30D00:00:00.000000000=2026.07.30;2026.07.30<2026.07.30D00:00:00.000000001;2026.07.30=2026.07.30D12:00:00.000000000;2026.07.30>2026.07.29D23:59:59.999999999;2026.07.30D12:00:00.000000000>2026.07.30;1707.09.21<1707.09.22D00:12:43.145224193;2292.04.11>2292.04.10D23:47:16.854775807;5881610.07.11>2026.07.30D00:00:00.000000000;2000.01.01=2000.01.01D00:00:00.000000000);1 1 1 0 1 1 1 1 1 1]
t[`timeStampCmp;(12:00:00.000=2026.07.30D12:00:00.000000000;2026.07.30D12:00:00.000999999=12:00:00.000;12:00:00.000<2026.07.30D12:00:00.001000000;12:00:00.000<2026.07.30D00:00:00.000000000;36:00:00.000=2000.01.02D12:00:00.000000000;11:59:59.999=1999.12.31D11:59:59.999999999;2026.07.30D13:00:00.000000000>12:00:00.000);1 1 1 0 0 1 1]
t[`temporalKindsList;((2026.07.30;2026.07.31)=2026.07.30D00:00:00.000000000;2026.07.30D12:00:00.000000000<(11:00:00.000;13:00:00.000);(2026.07.30;12:00:00.000)=(2026.07.30D00:00:00.000000000;2026.07.30D12:00:00.000000000))~(1 0;0 1;1 1);1]
/ the null timestamp is below every date and every time and equal to none, as q's 0Np (its stored ns were read as a date or a time)
t[`temporalKindsNull;((`mkp 0N)=00:12:43.145;(`mkp 0N)<00:00:00.001;(`mkp 0N)>00:00:00.001;00:00:00.001>(`mkp 0N);00:00:00.001<(`mkp 0N);00:00:00.001=(`mkp 0N);1700.01.01>(`mkp 0N);1700.01.01<(`mkp 0N);1700.01.01=(`mkp 0N);(`mkp 0N)<1700.01.01;(`mkp 0N)>1700.01.01;(`mkd -2147483648)>(`mkp 0N);(`mkp 0N)<`mkt -2147483648;1707.09.21<(`mkp 0N);1707.09.21<1707.09.22D00:12:43.145224193);0 1 0 1 0 0 1 0 0 1 0 1 1 0 1]
/ a general list of dates with another kind in it leaves the one-loop path (tcl) for item by item, which follows q (#83 Q9)
t[`temporalListLoopKinds;(ER"(2026.07.30;2026.07.31;12:00:00.000)=2026.07.30";(2026.07.30;2026.07.31;2026.07.30D00:00:00.000000000)=2026.07.30);("'type";1 0 1)]
/ a - before a time literal is part of it, as q and as -10 for an int (#83 Q8; it was the verb on the time, 'type, so a
/ negative time's text did not read back), down to -596:31:23.648 as "T"$ reads; a strand of times takes one; dates keep the verb
tv[`timeNegLiteral;"(`i$-10:00:00.000;`i$-00:00:00.001;`i$-596:31:23.648;`i$-00:00:00.000;@-10:00:00.000;-10:00~0-10:00:00.000)";(-36000000;-1;-2147483648;0;`t;1)]
tv[`timeNegStrand;"((10:00:00.000 -10:00:00.000)~(10:00:00.000;0-10:00:00.000);(-10:00:00.000 -11:00:00.000 12:00:00.000)~(0-10:00:00.000;0-11:00:00.000;12:00:00.000))";1 1]
/ -x of a time negates it, as q (it was 'type); -596:31:23.648 has no negative in 32 bits, so it is 'limit
tv[`timeNeg;"({-x}10:00:00.000;-(0-10:00:00.000);-(10:00:00.000;0-01:00:00.000);-00:00:00.000)~(0-10:00:00.000;10:00:00.000;(0-10:00:00.000;01:00:00.000);00:00:00.000)";1]
t[`timeNegErrors;ER'("-596:31:23.649";"-(-596:31:23.648)";"-1000:00:00.000";"-10:60:00.000";"-2026.07.30";"{-x}2026.07.30D00:00:00.000000000");("'limit";"'limit";"'limit";"'parse";"'type";"'type")]
/ a date plus or minus an int past 32 bits of days is 'limit, as the literal (it wrapped: 2026.07.30+2147483647 was -5877584.01.18)
t[`dateArithLimit;ER'("2026.07.30+2147483647";"2147483647+2026.07.30";"5881610.07.11+1";"(`mkd -2147483648)-1";"2026.07.30+9223372036854775807";"2026.07.30+1 2147483647");6#,"'limit"]
t[`dateArithTop;`i$(5881610.07.10+1;2026.07.30-2147483647;(`mkd -2147483647)-1;2026.07.30+-9707);2147483647 -2147473940 -2147483648 0]
t[`temporalAmend;@[2#2026.07.30;0;:;2026.08.01]~(2026.08.01;2026.07.30);1]
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

/ ---- an empty list whose prototype is :: (no dates left, or a list of functions) prints as 0#,(::), which reads back ----
/ (0#,:: read back as a function: 0#, composed with ::)
t[`emptyProtoNil;`k@'(0#2026.01.02 2026.01.03;0#({x};1);(0#2026.01.02 2026.01.03;1);,0#12:00:00.000 13:00:00.000);("0#,(::)";"0#,(::)";"(0#,(::);1)";",0#,(::)")]
t[`emptyProtoNilBack;{x~. `k@x}'(0#2026.01.02 2026.01.03;0#({x};1);(0#2026.01.02 2026.01.03;1);([]));1 1 1 1]

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

/ ---- an assignment whose verb fails leaves the variable as it was ----
/ (where each level is one place that is there, each level puts back the item it took out; #: _: @: .: and a table's row read the item first)
t[`amendVerbFailsKeep;(IA["F1:1 2 3";"F1[1]+:`a"];IA["F2:+`a`b!(1 2 3;4 5 6)";"F2[1;`a]+:`x"];IA["F3:+`a`b!(1 2 3;4 5 6)";"F3[`a;1]+:`x"];IA["F19:+`a`b!(1 2 3;4 5 6)";"F19[1]+:`x"];IA["F31:!5";"F31[2]+:`q"]);((`err;1 2 3);(`err;+`a`b!(1 2 3;4 5 6));(`err;+`a`b!(1 2 3;4 5 6));(`err;+`a`b!(1 2 3;4 5 6));(`err;!5))]
t[`amendVerbFailsKeepNested;(IA["F4:`a`b!(1 2;3 4)";"F4[`b]+:`x"];IA["F5:`a`b!(1 2;3 4)";"F5[`a;0]+:`x"];IA["F6:(1 2;(3;4 5))";"F6[1;1;0]+:`x"];IA["F39:`a`b!(1 2;3 4)";"F39[`a;5]#:1"]);((`err;`a`b!(1 2;3 4));(`err;`a`b!(1 2;3 4));(`err;(1 2;(3;4 5)));(`err;`a`b!(1 2;3 4)))]
t[`amendVerbFailsKeepByName;(IA["F7:1 2 3";".[`F7;,1;+;`a]"];IA["F8:`a`b!(1 2;3 4)";".[`F8;(`a;0);{x+`z}]"];IA["F9:+`a`b!(1 2;3 4)";".[`F9;(`a;1);{x+y+`z};1]"]);((`err;1 2 3);(`err;`a`b!(1 2;3 4));(`err;+`a`b!(1 2;3 4)))]   / by name, where :: was left in it
t[`amendVerbStillWorks;(IA["F10:1 2 3";"F10[1]+:10"];IA["F11:+`a`b!(1 2;3 4)";"F11[1;`a]*:5"];IA["F12:`a`b!(1 2;3 4)";"F12[`b;0],:9"];IA["F13:(1 2;(3;4 5))";".[`F13;(1;1;0);-:]"];IA["F20:+`a`b!(1 2;3 4)";"F20[1]+:10"]);((::;1 12 3);(::;+`a`b!(1 10;3 4));(::;`a`b!(1 2;(3 9;4)));2#,(1 2;(3;-4 5));(::;+`a`b!(1 12;3 14)))]
t[`amendByNameFiveArgs;{r:IA["F14:(1 2;3 4)";".[`F14;1 0;{(x;y;z)};10;1]"];s:IA["F15:1 2";".[`F15;,0;{[a;b;c;d]a};10;1;2]"];e:*"\n"\.[.:;,".[`F14;0;+;1;2]";{x}];(r;s;e;F14;F15)}[];((`err;(1 2;3 4));(`err;1 2);"'nyi";(1 2;3 4);1 2)]   / five or more arguments: 'nyi, before the global is touched
t[`amendByNameVerbSetsGlobal;{FA::1 2 3;r:.[{.[`FA;,1;{FA[0]:1;x+1}]};,0;{`t}];FB::1 2 3;s:.[{.[`FB;,1;{FB::99;x+`a}]};,0;{`t}];(r;FA;s;FB)}[];(`t;1 2 3;`t;99)]   / a verb that assigns into the global, then fails: the global is put back, or keeps what the verb set it to
t[`amendVerbDictOfDicts;(IA["F16:`a`b!(`x`y!1 2;`x`y!3 4)";"F16[`b],:`q"];IA["F17:`a`b!(`x`y!1 2;`x`y!3 4)";"F17[`b;`x]+:10"];IA["F18:`a`b!(`x`y!1 2;`x`y!3 4)";"F18[`b;`x]+:`z"]);((::;`a`b!(`x`y!1 2;(`x`y!3 4;`q)));(::;`a`b!(`x`y!1 2;`x`y!13 4));(`err;`a`b!(`x`y!1 2;`x`y!3 4)))]   / values that are a table (dicts of the same keys): as a list of the dicts
t[`amendVerbFunctionSetsGlobal;({r:IA["FG:+`a`b!(!1000;1000#{FG::0;x+1})";"FG[`b;3]@:5"];(*r;r[1;`b;3];#r 1)}[];IA["FH:(!1000;(2;{FH::0;x+1}))";"FH[1;1]@:5"][1;1;1];{r:IA["FI:+`a`b!(!1000;1000#{FI::0;x+1})";"FI[3;`b]@:5"];(*r;r[1;`b;3];#r 1)}[]);((::;6;1000);6;(::;6;1000))]   / a function item called by @: sets the global (it was freed, then read): the assignment is made on the global as it was
t[`amendVerbFunctionSetsGlobal2;({r:IA["FJ:(!1000),{FJ::!3;x+1}";"FJ[1000]@:5"];(*r;r[1;1000];#r 1)}[];IA["FK:(1;(2;{FK::0;x+1}))";"FK[1;1]@:5"];IA["FL:`a`b!(1;{FL::!3;x+1})";"FL[`b].:,5"];{FS::(1;{FS::7 8;x+1});r:FS[1]@:5;(r;FS)}[];{FU::(1;(2;{FU::(1;{FU::7;x});x+1}));r:FU[1;1]@:5;(r;FU)}[]);((::;6;1001);(::;(1;2 6));(::;`a`b!1 6);(6;1 6);(6;(1;2 6)))]   / and with .:, and with the assignment's value used
t[`amendVerbFunctionSetsGlobalFails;(IA["FM:(1;(2;{FM::!3;x+`a}))";"FM[1;1]@:5"];IA["FW:(1;(2;{FW[0]:9;x+`a}))";"FW[1;1]@:5"];IA["FP:(1;(2;{FP[0]:9;x+1}))";"FP[1;1]@:5"]);((`err;0 1 2);(`err;(9;(2;{FW[0]:9;x+`a})));(::;(1;2 6)))]   / sets or amends it, then fails: what the function set is kept; amends it, then succeeds: the assignment is made on the global as it was
t[`amendVerbFunctionKeepsGlobal;{r:(IA["FQ:(1;(2;{x+1}))";"FQ[1;1]@:5"];IA["FR:+`a`b!(!3;3#{x+1})";"FR[`b;1]@:5"]);s:{FT::(1;(2;{x+1}));r:FT[1;1]@:5;(r;FT)}[];(r[0];*r 1;FR[1;`b];FR`a;s)}[];((::;(1;2 6));::;6;0 1 2;(6;(1;2 6)))]   / a function item that leaves the global alone: amended as before
t[`amendVerbFailsKeepRowBelow;(IA["F21:+`a`b!(1 2 3;(1 2;3 4;5 6))";"F21[1;`b;0]+:`x"];IA["F22:+`a`b!(1 2 3;(1 2;3 4;5 6))";".[`F22;(1;`b;0);+;`x]"];IA["F23:`t`u!(+`a`b!(1 2 3;(1 2;3 4;5 6));0)";"F23[`t;1;`b;0]+:`x"];IA["F24:`t`u!(+`a`b!(1 2 3;(1 2;3 4;5 6));0)";".[`F24;(`t;1;`b;0);{x+`z}]"]);((`err;+`a`b!(1 2 3;(1 2;3 4;5 6)));(`err;+`a`b!(1 2 3;(1 2;3 4;5 6)));(`err;`t`u!(+`a`b!(1 2 3;(1 2;3 4;5 6));0));(`err;`t`u!(+`a`b!(1 2 3;(1 2;3 4;5 6));0)))]   / a table's row with more than one level below it
t[`amendVerbRowBelowWorks;(IA["F25:+`a`b!(1 2 3;(1 2;3 4;5 6))";"F25[1;`b;0]+:10"];IA["F26:+`a`b!(1 2 3;(1 2;3 4;5 6))";".[`F26;(1;`b;0);,;9]"];IA["F27:`t`u!(+`a`b!(1 2 3;(1 2;3 4;5 6));0)";"F27[`t;1;`b;0]-:1"];IA["F30:+`a`b!(1 2 3;(`p`q!1 2;`p`q!3 4;`p`q!5 6))";"F30[1;`b;`r]+:100"]);((::;+`a`b!(1 2 3;(1 2;13 4;5 6)));2#,+`a`b!(1 2 3;(1 2;(3 9;4);5 6));(::;`t`u!(+`a`b!(1 2 3;(1 2;2 4;5 6));0));(::;+`a`b!(1 2 3;(`p`q!1 2;`p`q`r!3 4 100;`p`q!5 6))))]   / (a key not there below: added, as before)
t[`amendVerbResultFits;(IA["F32:+`a`b!(1 2 3;4 5 6)";"F32[`a],:5"];IA["F33:+`a`b!(1 2 3;4 5 6)";"F33[`a]#:2"];IA["F34:+`a`b!(1 2 3;4 5 6)";"F34[`a]@:0 1"];IA["F35:+`a`b!(1 2 3;4 5 6)";".[`F35;,`a;,;5]"];IA["F36:`x`y!(+`a`b!(1 2 3;4 5 6);0)";"F36[`x;`a],:5"];IA["F37:+(,`a)!,1 2 3";"F37[`a],:5"]);((`err;+`a`b!(1 2 3;4 5 6));(`err;+`a`b!(1 2 3;4 5 6));(`err;+`a`b!(1 2 3;4 5 6));(`err;+`a`b!(1 2 3;4 5 6));(`err;`x`y!(+`a`b!(1 2 3;4 5 6);0));(::;+(,`a)!,1 2 3 5))]   / a verb whose result does not fit a table's column ('length): the variable is kept; one column can change its count
t[`amendVerbFailsKeepThroughSymbol;{GK::1 2 3;F28::`a`b!(1;`GK);r:.[.:;,"F28[`b;0]+:`x";{`err}];GL::1 2 3;F29::`a`b!(1;`GL);s:.[.:;,"F29[`b;0]+:10";{`err}];GM::+`a`b!(1 2 3;4 5 6);F38::(1;`GM);u:.[.:;,"F38[1;`a],:5";{`err}];(r;GK;s;GL;F29;u;GM)}[];(`err;1 2 3;::;11 2 3;`a`b!(1;11 2 3);`err;+`a`b!(1 2 3;4 5 6))]   / a symbol on the way names a global, amended by name: that global is kept (the variable is still lost)
/ (a list of indices, an elided level, a list of keys: d4 amends a copy, the variable held; then as above)
t[`amendVerbFunctionSetsGlobalList;.[{(IA["FV1:(!1000),2#{FV1::!3;x+1}";"FV1[1000 1001]@:5"];IA["FV2:(1;({FV2::!3;x+1};{x+2}))";"FV2[1;]@:5"];IA["FV3:`a`b!({FV3::!3;x+1};{x+2})";"FV3[`a`b]@:5"];{r:IA["FV4:+`a`b!(!3;3#{FV4::0;x+1})";"FV4[`b;0 1]@:5"];(*r;2#r[1;`b])}[];{FV5::(1;({FV5[0]:7;x+1};{x}));r:FV5[1;0 1]@:5;(r;FV5)}[])};,0;{x}];((::;(!1000),6 6);(::;(1;6 7));(::;`a`b!6 7);(::;6 6);(6 5;(1;6 5)))]   / (main: 'type, and the variable lost)
t[`amendVerbFunctionSetsGlobalListFails;.[{(IA["FV6:(!3),2#{FV6::!3;x+`a}";"FV6[3 4]@:5"];{r:IA["FV7:(!3),({x+1};{x+`a})";"FV7[3 4]@:5"];(*r;`k r 1)}[];{r:IA["FV8:(1;({FV8[0]:7;x+1};2))";"FV8[1;0 0]@:5"];(*r;r[1;0];r[1;1;1])}[])};,0;{x}];((`err;0 1 2);(`err;"(0;1;2;{x+1};{x+`a})");(`err;7;2))]   / sets or amends it, then fails: what the function set is kept; else the variable as it was
t[`amendVerbFunctionSetsGlobalWhole;.[{(+/{FV9::{FV9::!1000;x+1};FV9@:5;FV9}'!3;{FV10::{FV10::!3;x>1};FV10#:1 2 3;FV10}[];IA["FV11:{FV11::!3;x+`a}";"FV11@:5"])};,0;{x}];(18;2 3;(`err;0 1 2))]   / f@:y, f a global that sets itself (main read it freed)
t[`amendVerbFunctionSetsGlobalTake;.[{({`k@IA["FV12:(0;(1;(2;{FV12::0;x>1})))";"FV12[1;1;1]#:1 2 3"]1}[];{`k@IA["FV13:(0;(1;(2;{FV13::0;x>1})))";"FV13[1;1;1]_:1 2 3"]1}[];{`k@IA["FV14:(0;(1;({FV14::0;x>1};{x>2})))";"FV14[1;1;0 1]#:(1 2 3;1 2 3)"]1}[];{`k@IA["FV15:(0;(1;({FV15::0;x>1};{x>2})))";"FV15[1;1;0 1]_:(1 2 3;1 2 3)"]1}[];{FV16::(!1000),2#{FV16::!3;x+1};FV16[1000 1001]@:5;FV16::0;!30}[])};,0;{x}];("(0;(1;(2;2 3)))";"(0;(1;(2;,1)))";"(0;(1;(2 3;,3)))";"(0;(1;(,1;1 2)))";!30)]   / #: and _: (main: a third item, and memory reused)
t[`amendVerbDotDeep;.[{N:{(x;0)}/[100;0];FV17::(N;N);FV17[0 1].:(,1;,1);r:FV17;FV17::0;r};,0;{x}];0 0]   / .: on items 100 levels deep: the look for a function in them stops at 64 levels, and holds the variable

/ ---- an assignment into a symbol list below a column, key or index amends the list ----
/ (a symbol list there was taken as the name of a global to amend, as . takes one: 'type, and the variable was lost)
t[`amendSymbolColumn;(IA["S1:+`a`b!(1 2 3;`x`y`z)";"S1[`b;1]:`q"];IA["S2:+`a`b!(1 2 3;`x`y`z)";"S2[`b;1 2]:`q`r"];IA["S3:+`a`b!(1 2 3;`x`y`z)";"S3[`b;1]:7"];IA["S20:+`a`b!(1 2 3;`x`y`z)";"S20[`b;1],:`w"]);((::;+`a`b!(1 2 3;`x`q`z));(::;+`a`b!(1 2 3;`x`q`r));(::;+`a`b!(1 2 3;(`x;7;`z)));(::;+`a`b!(1 2 3;(`x;`y`w;`z))))]
t[`amendSymbolListInDict;(IA["S4:`a`b!(1 2;`x`y`z)";"S4[`b;1]:`q"];IA["S5:`a`b!(1 2;`x`y`z)";"S5[`b;0 1],:`s"];IA["S6:`a`b!(`x`y;`z`w)";"S6[`a`b;1]:`q"];IA["S21:`x`y!(`a`b;`c`d)";"S21[`z;0]:`q"]);((::;`a`b!(1 2;`x`q`z));(::;`a`b!(1 2;(`x`s;`y`s;`z)));(::;`a`b!(`x`q;`z`q));(::;`x`y`z!(`a`b;`c`d;`q`)))]   / and a key to add, whose value is the first value nulled
t[`amendSymbolListInList;(IA["S7:(`a`b;`c`d)";"S7[1;0]:`q"];IA["S8:(`a`b;`c`d)";"S8[;1]:`q"];IA["S9:((`a`b;`c);`d)";"S9[0;0;1]:`q"]);((::;(`a`b;`q`d));(::;(`a`q;`c`q));(::;((`a`q;`c);`d)))]
t[`amendSymbolListLocal;{d:`a`b!(1 2;`x`y`z);d[`b;1]:`q;e:d;e[`b;0 2]:`r`s;f:d;f[`b;]:`t;(d;e;f)}[];(`a`b!(1 2;`x`q`z);`a`b!(1 2;`r`q`s);`a`b!(1 2;`t`t`t))]
t[`amendSymbolListDot;(.[(`x`y;`z);(0;1);:;`q];.[`a`b!(1 2;`x`y`z);(`b;1);:;`q];IA["S0:`a`b!(1 2;`x`y`z)";".[`S0;(`b;1);:;`q]"];IA["S22:`a`b!(1 2;`x`y`z)";".[`S22;(`b;2);^:]"]);((`x`q;`z);`a`b!(1 2;`x`q`z);2#,`a`b!(1 2;`x`q`z);2#,`a`b!(1 2;(`x;`y;0)))]   / the value form, and by name: the name is the global, the list below it data
t[`amendSymbolListError;(IA["S23:(1;`x`y`z)";"S23[1;5]:`q"];IA["S24:(1;`x`y`z)";"S24[1;0 1]:`q`r`s"];{{*"\n"\.[.:;,x;{x}]}'(".[`a`b!(1 2;`x`y`z);(`b;5);:;`q]";".[`a`b!(1 2;`x`y`z);(`b;0 1);:;`q`r`s]")}[]);((`err;(1;`x`y`z));(`err;(1;`x`y`z));("'index";"'length"))]   / a bad index or count: the variable is kept; in the value form the error is the data's ('index, 'length), not 'type
SG:1 2 3;SH:4 5 6;SX:1 2 3;SV:(`SX`y;2);N2.b:1 2 3;N3.v:1 2 3
t[`amendSymbolListNamesNone;{r:(IA["S14:`x`y!(1;`N2`b)";"S14[`y;1]:9"];IA["S15:`x`y!(1;,`SG)";"S15[`y;0]:9"]);.[`N3`v;,1;:;9];(r;N2.b;SG;N3.v)}[];(((::;`x`y!(1;(`N2;9)));(::;`x`y!(1;,9)));1 2 3;1 2 3;1 9 3)]   / a global by the list's name is not amended (main amended it); . by name still is
t[`amendSymbolListItems;{r:(IA["S16:`a`b!(1;`SG`SH)";"S16[`b;;0]:9"];IA["S17:`a`b!(1;`SG`SH)";"S17[`b;0 1;0]:9"];IA["S18:(1;`SG`SH)";"S18[1;;0]:9"];IA["S19:+`a`b!(1 2;`SG`SH)";"S19[`b;;0]:9"];IA["S27:`a`b!(1;`SG`SH)";"S27[`b;0;0]@:{x}"];IA["S28:`a`b!(1;`SG`SH)";"S28[`b;0;0]#:1"]);(r;SG;SH)}[];(((`err;`a`b!(1;`SG`SH));(`err;`a`b!(1;`SG`SH));(`err;(1;`SG`SH));(`err;+`a`b!(1 2;`SG`SH));(`err;`a`b!(1;`SG`SH));(`err;`a`b!(1;`SG`SH)));1 2 3;4 5 6)]   / its items are data too: an index below one is 'type, and the globals they name are not amended
t[`amendSymbolListItemsError;{e:{r:.[{. x;`ok};,x;{x}];$[`ok~r;r;*"\n"\r]}'("S16[`b;;0]:9";"S18[1;0;0]+:9";".[`S18;(1;::;0);:;9]";".[`SV;(0 1;::;1.0);{x+1}]");(e;S16;S18;SG;SH;SX;SV)}[];(4#,"'type";`a`b!(1;`SG`SH);(1;`SG`SH);1 2 3;4 5 6;1 2 3;(`SX`y;2))]   / (SV, at a list of indices, is kept: .[`v;i;f] is checked)
t[`amendSymbolListBelowRow;(IA["S25:+`a`b!(1 2 3;(`x`y;`z`w;`u`v))";"S25[1;`b;0],:`q"];IA["S26:+`a`b!(1 2 3;(`x`y;`z`w;`u`v))";"S26[1;`b;0]+:1"]);((::;+`a`b!(1 2 3;(`x`y;(`z`q;`w);`u`v)));(`err;+`a`b!(1 2 3;(`x`y;`z`w;`u`v))))]   / below a table's row, with a verb: read first, as other data

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

/ ---- flip: an atom value is enlisted; bit lists; empty lists of tables (upstream ngn/k 7bf994cd) ----
t[`flipAtom;`k@+(,`a)!,{x};"+(,`a)!,,{x}"]
t[`flipCols;`k@+`a`b!(1 2;3 4);"+`a`b!(1 2;3 4)"]
t[`flipEmptyTable;(`k@([]);#!+([]);#. +([]));("+(0#`)!()";0;0)]   / ([]) is the empty table (issue #19); its values were 0#,:: (ngn/k enlists them)
tv[`emptyTable;"(#([]);!([]);([])~+(0#`)!();([]),([]a:1 2);([]a:1 2),([]))";(0;0#`;1;+(,`a)!,1 2;+(,`a)!,1 2)]   / no columns and no rows, as agreed on #19 (q's ([]) is an empty dict): #([]) was 1 and both joins were 'domain
t[`flipEmptyTables;(`k@+0#,(+`a`b!(1 2;3 4));`k@+0#,,`a`b!1 2);("(0#,`a`b!0N 0N;0#,`a`b!0N 0N)";",0#,`a`b!0N 0N")]   / empty lists of tables: the prototype was read before it was set; empty lists of dicts, as in ngn/k
t[`flipBits;(+(01B;01B);+(101B;010B));((0 0;1 1);(1 0;0 1;1 0))]   / lists of bit booleans, read as bits
t[`flipBits8;*+8#,11B;8#1]   / each column was read as 8-byte words of the lists

/ ---- odometer of a list of dicts uses their values (upstream ngn/k d99849c6) ----
t[`tilDicts;!(`a`b!1 2;`c`d!3 4);(1 1 2 2;3 4 3 4)]
t[`tilLists;!(1 2;3 4);(1 1 2 2;3 4 3 4)]
t[`tilDictLast;!(,1;,2;,3;,4;5 6;`a`b!7 8);(1 1 1 1;2 2 2 2;3 3 3 3;4 4 4 4;5 5 6 6;7 8 7 8)]   / a dict after the first four items
t[`tilAtomDict;.[!:;,(1;`a`b!3 4);{`e}];`e]   / an atom item still fails, with a dict beside it
t[`tilKeyed;`k@!((+(,`k)!,1 2)!+(,`v)!,3 4;1 2);"(+(,`v)!,3 3 4 4;1 2 1 2)"]   / a keyed table uses its value table

/ ---- an empty general list whose prototype is a dict stays a list, not a table (upstream ngn/k d99849c6) ----
t[`takeEmptyDictFirst;`k@'(0#((,`a)!,0;0);0#(1 2!3 4;0);0#(`a`b!1 2;`c`d!3 4));("0#,(,`a)!,0N";"0#,1 2!0N 0N";"0#,`a`b!0N 0N")]   / these were empty tables, the second with int columns
t[`takeEmptyDictShape;*0#(`a`b!(0;0 0);0N);`a`b!(0N;0N 0N)]
t[`takeEmptyTable;`k@'(0#(`a`b!1 2;`a`b!3 4);0#,`a`b!1 2);("+`a`b!(!0;!0)";"+`a`b!(!0;!0)")]
t[`dropEmptyDictFirst;`k@2_((,`a)!,0;1 2);"0#,(,`a)!,0N"]
/ reshaping such a list is 'nyi, as reshaping a table is (it read a table as rows of a list: crashes, heap addresses, a loop)
t[`reshapeEmptyDicts;4#.[.:;,"2 0#0#(`a`b!1 2;0)";{x}];"'nyi"]
t[`reshapeDroppedDicts;4#.[.:;,"2 0N#2_(`a`b!1 2;3)";{x}];"'nyi"]
t[`reshapeAmendedDicts;4#.[.:;,"2 2#@[(`a`b!1 2;0);1;:;`a`b!3 4]";{x}];"'nyi"]
t[`reshapeMixedDicts;4#.[.:;,"1 0#(1 2!3 4;`a`b!(1;`c`d!3 4))";{x}];"'nyi"]
/ an amend of an empty table that reaches no row gives the table back; one that reaches the null row (an elided index, :: anywhere in it) makes the table from what it gives
t[`amendEmptyTable;`k@'(@[0#+`a`b!(1 2;3 4);!0;:;()];.[0#+`a`b!(1 2;3 4);(!0;`a);:;()];@[0#+`a`d!(1 2;(`x`y!1 2;`x`y!3 4));!0;:;()]);("+`a`b!(!0;!0)";"+`a`b!(!0;!0)";"+`a`d!(!0;+`x`y!(!0;!0))")]
t[`amendEmptyElided;`k@'(@[0#+`a`b!(1 2;3 4);::;{x,`c!9}];@[0#+`a`b!(1 2;3 4);::;{1#x}];@[0#+`a`b!(1 2;3 4);(!0;::);{x,`c!9}]);("+`a`b`c!(!0;!0;!0)";"+(,`a)!,!0";"+`a`b`c!(!0;!0;!0)")]
/ an @ amend of an empty table at rows and then columns is 'type, as in ngn/k (2.4.1 gave the table)
t[`amendEmptyMixed;5#.[.:;,"@[0#+`a`b!(1 2;3 4);(!0;`a);{x}]";{x}];"'type"]

/ ---- take of keys from a dict whose keys are lists or a table (upstream ngn/k 5f50f8e3, d3b72007) ----
t[`takeListKey;4#.[.:;,"(1 2)#(1 2;3 4)!5 6";{x}];"'nyi"]   / one list key, not a list of them (it gave 1 2!5, two keys and one value)
t[`takeListKeys;(1 2;3 4)#(1 2;3 4)!5 6;(1 2;3 4)!5 6]
t[`takeSymKeys;`a`c#`a`b`c!1 2 3;`a`c!1 3]
t[`takeEmptyDict;`k@'(`a`b#()!();1 2#()!();(,`a)#()!();(0#`)#()!());("`a`b!\"\"";"1 2!\"\"";"(,`a)!\"\"";"(0#`)!\"\"")]   / the empty dict ()!() takes any keys, as on 2.4.1 (ngn/k: 'nyi)
/ a keyed table's keys are a table: reshape or a key list one rank down is 'nyi (they crashed)
t[`reshapeKeyed;4#.[.:;,"2 2#((+(,`k)!,1 2)!+(,`v)!,3 4)";{x}];"'nyi"]
t[`reshapeKeyedNull;4#.[.:;,"0N 2#((+(,`k)!,1 2)!+(,`v)!,3 4)";{x}];"'nyi"]
t[`reshapeKeyedEmpty;4#.[.:;,"2 0#=0#,`a`b!1 2";{x}];"'nyi"]
t[`takeKeyedSyms;4#.[.:;,"(,`x)#((+(,`k)!,1 2)!+(,`v)!,3 4)";{x}];"'nyi"]
/ first or last of an enlisted keyed table, or one passed by ., is the keyed table: the empty one's values were a list of dicts
t[`firstEnlistKeyed;{x~/:(*,x;*|,x;.[{x};,x])}'{(0#x;x)}(+(,`k)!,1 2)!+(,`n)!,3 4;(1 1 1;1 1 1)]
/ keys of the other kind (chars against numbers): the lookup is 'type, as find is since 2.4.0, and the take passes it up (it crashed, even under a trap)
t[`takeKeysCharsNums;(5#.[{"ab"#x};,1 2!3 4;{x}];5#.[{1 2#x};,"xy"!3 4;{x}]);("'type";"'type")]
/ a failed lookup releases only the dict: the key list belongs to the caller (shared here, used again)
t[`takeKeysFailShared;{k:"ab",();k2:1 2,!0;r:{5#.[#;(x;y);{x}]}[k]'4#,1 2!3 4;r2:{5#.[#;(x;y);{x}]}[k2]'4#,((1 2;3 4);(5 6;7 8))!9 10;(r;r2;k;k2)}[];(4#,"'type";4#,"'rank";"ab";1 2)]

/ ---- an amend that leaves no column of a table a list fills each column to the table's row count (an empty table stays empty), as update does; a list still replaces a column ----
t[`tableAtomColumns;({t:+(,`a)!,1 2 3;t[`a]:9;t}[];@[+(,`a)!,1 2 3;`a;:;9];@[+(,`a)!,1 2 3;`a;+/];{t:+`a`b!(1 2 3;4 5 6);t[`a`b]:9;t}[];@[+`a`b!(1 2 3;4 5 6);`a`b;:;(::;::)];@[+`a`b!(1 2 3;4 5 6);`a`b;:;(+;-)]);(+(,`a)!,9 9 9;+(,`a)!,9 9 9;+(,`a)!,6 6 6;+`a`b!(9 9 9;9 9 9);+`a`b!((::;::;::);(::;::;::));+`a`b!((+;+;+);(-;-;-)))]   / one row, before
tv[`tableAtomColumnsTypes;"@[+`a`b!(1 2 3;`x`y`z);`a`b;:;(9;`q)]";+`a`b!(9 9 9;`q`q`q)]   / atoms of two types: 'length, before
t[`tableAtomColumnsGlobal;(IA["TA1:+(,`a)!,1 2 3";"TA1[`a]:9"];IA["TA2:+(,`a)!,1 2 3";".[`TA2;,`a;:;9]"];IA["TA3:+`a`b!(1 2 3;4 5 6)";"TA3[`a`b]:9"];IA["TA4:`x`y!(+(,`a)!,1 2 3;5)";"TA4[`x;`a]:9"]);((::;+(,`a)!,9 9 9);(+(,`a)!,9 9 9;+(,`a)!,9 9 9);(::;+`a`b!(9 9 9;9 9 9));(::;`x`y!(+(,`a)!,9 9 9;5)))]
t[`tableAtomColumnsEmpty;({t:+(,`a)!,!0;t[`a]:9;t}[];{t:0#+`a`b!(1 2;3 4);t[`a`b]:9;t}[];@[0#+(,`a)!,1 2;`a;:;`x];{t:+(0#`)!();t[`a]:9;t}[]);(+(,`a)!,!0;+`a`b!(!0;!0);+(,`a)!,0#`;+(,`a)!,!0)]
tv[`tableAtomColumnsTypesEmpty;"@[0#+`a`b!(1 2;`x`y);`a`b;:;(9;`q)]";+`a`b!(!0;0#`)]
t[`tableAtomColumnsNone;({t:+(0#`)!();t[0#`]:9;t}[];@[+(0#`)!();0#`;:;9];@[+(0#`)!();0#`;{x}];IA["TN1:+(0#`)!()";"TN1[0#`]:9"]);(+(0#`)!();+(0#`)!();+(0#`)!();(::;+(0#`)!()))]   / no column: the table as it was
t[`tableListColumnStill;({t:+(,`a)!,1 2 3;t[`a]:7 8;t}[];{t:+(,`a)!,1 2 3;t[`a]:,9;t}[];{t:+`a`b!(1 2 3;4 5 6);t[`a]:9;t}[];{t:+(,`a)!,1 2 3;t[`b]:9;t}[];{t:+`a`b!(1 2 3;4 5 6);t[`a`b]:(9;10 11);t}[]);(+(,`a)!,7 8;+(,`a)!,,9;+`a`b!(9 9 9;4 5 6);+`a`b!(1 2 3;9 9 9);+`a`b!(9 9;10 11))]   / as before
TE:0#+`a`b!(1 2;3 4)
t[`tableElidedEmpty;(.[TE;(::;`a);:;9];.[TE;(::;`a);{x}];.[TE;(::;`a;0);:;9];{t:TE;t[;`a]:9;t}[];.[(TE;1);(0;::;`a);:;9];.[`x`y!(TE;1);(`x;::;`a);:;9];.[+`a`b!(1 2;3 4);(::;`a);:;9]);(TE;TE;TE;TE;(TE;1);`x`y!(TE;1);+`a`b!(9 9;3 4))]   / every row of an empty table: the table, not ()
t[`tableElidedEmptyAtoms;(.[TE;(::;`a);:;`x];.[TE;(::;`a);:;!0];.[TE;(::;`a);+;()];.[TE;(::;`a);:;0#TE];.[TE;(::;`a);!0];{t:TE;t[;`a]:"x";t}[]);6#,TE]   / f and y atoms or empty
t[`tableElidedEmptyLength;{*"\n"\.[.:;,x;{x}]}'(".[TE;(::;`a);:;1 2]";".[TE;(::;`a);+;,1]";".[TE;(::;`a);:;`a!1]";".[TE;(::;`a);1 2]";".[TE;(::;`a);(1 2);9]";"{t:TE;t[;`a]:1 2;t}[]";".[(TE;1);(0;::;`a);:;1 2]";".[+`a`b!(1 2;3 4);(::;`a);:;1 2 3]");8#,"'length"]   / a list or dict with items: 'length, as before

/ ---- a failed assignment below a dict key, to a table, or into an atom leaves the variable ----
t[`amendKeepDictIndex;(IA["D1:`a`b!(1 2;3 4)";"D1[`a;7]:9"];IA["D2:`a`b!(1 2;3 4)";"D2[`a`b;5]:9"];IA["D3:`a`b!(1 2;3 4)";"D3[`a;7]+:9"]);3#,(`err;`a`b!(1 2;3 4))]
t[`amendKeepDictElided;IA["D4:`a`b!(1 2;3 4)";"D4[;5]:9"];(`err;`a`b!(1 2;3 4))]
t[`amendKeepDictLength;(IA["D5:`a`b!(1 2;3 4)";"D5[`a`b]:1 2 3"];IA["D6:`a`b!(1 2;3 4)";"D6[`a;0 1]:7 8 9"]);2#,(`err;`a`b!(1 2;3 4))]
t[`amendKeepSymbolIndex;IA["D7:`a`b!(1 2;3 4)";"D7[`a;`x]:9"];(`err;`a`b!(1 2;3 4))]   / a symbol indexes no list: 'type
t[`amendKeepTableLength;(IA["T1:+`a`b!(1 2;3 4)";"T1[`a]:5 6 7"];IA["T2:+`a`b!(1 2;3 4)";"T2[`c]:5 6 7"]);2#,(`err;+`a`b!(1 2;3 4))]
t[`amendKeepTableIndex;(IA["T3:+`a`b!(1 2;3 4)";"T3[5]:`a`b!9 9"];IA["T4:+`a`b!(1 2;3 4)";"T4[`a;5]:9"];IA["T5:+`a`b!(1 2;3 4)";"T5[5;`a]:9"]);3#,(`err;+`a`b!(1 2;3 4))]
t[`amendKeepAtom;(IA["V1:5";"V1[0]:9"];IA["V2:`a`b!(1;2)";"V2[`a;0]:9"]);((`err;5);(`err;`a`b!1 2))]   / an index into an atom: 'type
t[`amendKeepTemporalAtom;(IA["V6:2020.01.01";"V6[0]:9"];IA["D22:`a`b!(2020.01.01;2)";"D22[`a;0]:9"]);((`err;2020.01.01);(`err;`a`b!(2020.01.01;2)))]
t[`amendKeepListDeepLength;IA["L1:(1 2;3 4)";"L1[0;0 1]:7 8 9"];(`err;(1 2;3 4))]
t[`amendDictStillWorks;(IA["D8:`a`b!(1 2;3 4)";"D8[`z;0]:9"];IA["D9:`a`b!(1 2;3 4)";"D9[`c]:5 6 7"];IA["D0:`a`b!(1 2;3 4)";"D0[`a;0]:9"]);((::;`a`b`z!(1 2;3 4;9 0N));(::;`a`b`c!(1 2;3 4;5 6 7));(::;`a`b!(9 2;3 4)))]
t[`amendTableStillWorks;(IA["T6:+`a`b!(1 2;3 4)";"T6[`a]:9"];IA["T7:+`a`b!(1 2;3 4)";"T7[0;`a]:9"];IA["T8:+`a`b!(1 2;3 4)";"T8[`z;0]:9"]);((::;+`a`b!(9 9;3 4));(::;+`a`b!(9 2;3 4));(::;+`a`b`z!(1 2;3 4;9 0N)))]
t[`amendTablesBelowList;(IA["L2:(+`a`b!(1 2;3 4);+`a`b!(1 2;3 4))";"L2[0 1;`c]:(5 6;7 8)"];IA["T10:+`a!,1 2 3";"T10[`a]:7 8"]);((::;(+`a`b`c!(1 2;3 4;5 6);+`a`b`c!(1 2;3 4;7 8)));(::;+(,`a)!,7 8))]   / a column below a list index; the only column may take another count
t[`amendByNameKeepDict;(IA["D11:`a`b!(1 2;3 4)";".[`D11;(`a;7);:;9]"];IA["T9:+`a`b!(1 2;3 4)";".[`T9;,`a;:;7 8 9]"];IA["V3:5";".[`V3;,0;:;9]"]);((`err;`a`b!(1 2;3 4));(`err;+`a`b!(1 2;3 4));(`err;5))]   / by name too
t[`amendByNameWhole;(IA["V4:5";".[`V4;::;:;9]"];IA["V5:5";".[`V5;::;+;1]"]);((9;9);(6;6))]   / :: as the index replaces the whole value
t[`amendTableOtherVerbs;(IA["T11:+`a`b!(1 2;3 4)";"T11[`a]~:5 6 7"];IA["T12:+`a`b!(1 2;3 4)";".[`T12;,`a;{y@x};10 20 30]"];IA["T13:+`a`b!(1 2;3 4)";".[`T13;,`c;{2#y};5 6 7]"];IA["D12:`x!,+`a`b!(1 2;3 4)";"D12[`x;`a]~:5 6 7"]);((::;+`a`b!(0 0;3 4));2#,+`a`b!(20 30;3 4);2#,+`a`b`c!(1 2;3 4;5 6);(::;(,`x)!,+`a`b!(0 0;3 4)))]   / a verb other than : gets the whole column, so only its result needs the row count
t[`amendEmptySymbolIndex;(IA["L3:1 2 3";"L3[0#`]:9"];IA["D13:`a`b!(1 2;3 4)";"D13[`a;0#`]:9"]);((::;1 2 3);(::;`a`b!(1 2;3 4)))]   / no symbols, nothing to assign
t[`amendDictElidedFirst;(IA["D14:`a`b!(1 2;,3)";"D14[;1]:9"];IA["D15:`a`b!(1 2;3 4)";"D15[;0]:9"];IA["D16:`x`y!(`a`b!(1 2;,3);0)";"D16[`x;;1]:9"]);((`err;`a`b!(1 2;,3));(::;`a`b!(9 2;9 4));(`err;`x`y!(`a`b!(1 2;,3);0)))]   / an elided level on a dict is every key, as when indexing: each value is checked, and one too short is 'index with the variable kept
t[`amendDictMissingKey;(IA["D19:`a`b!(1 2 3;4 5)";"D19[`z;2]:9"];IA["D20:`a`b!(1 2 3;4 5)";"D20[`z;5]:9"];IA["D21:`a`b!(4 5;1 2 3)";"D21[`z`b;2]:9"]);((::;`a`b`z!(1 2 3;4 5;0N 0N 9));(`err;`a`b!(1 2 3;4 5));(`err;`a`b!(4 5;1 2 3)))]   / a missing key is added with the first value nulled, and that is what is indexed
XS:1 2 3
t[`amendThroughSymbol;{r:IA["D18:`a`b!(1 2;`XS)";"D18[`b;0]:9"];(r;XS)}[];((::;`a`b!(1 2;9 2 3));9 2 3)]   / a symbol atom at a level names a global, which the assignment amends (as in ngn/k), so it is not refused

/ ---- an assignment along one index per level is checked and made in one walk: the same values as before ----
t[`amendWalkNested;(IA["W1:`a`b!(1 2;`x`y!(3 4;5 6))";"W1[`b;`y;1]:9"];IA["W2:`a`b!(1 2;`x`y!(3 4;5 6))";"W2[`b;`x;0]+:10"];IA["W3:(1 2;(3;4 5))";"W3[1;1;0]:9"];IA["W12:`a`b!(1 2;`x`y!(3 4;5 6))";".[`W12;(`b;`x;0);-:]"]);((::;`a`b!(1 2;`x`y!(3 4;5 9)));(::;`a`b!(1 2;`x`y!(13 4;5 6)));(::;(1 2;(3;9 5)));2#,`a`b!(1 2;`x`y!(-3 4;5 6)))]
t[`amendWalkNewKey;(IA["W4:`a`b!(1 2;3 4)";"W4[`z]+:1"];IA["W5:`a`b!(1 2;3 4)";"W5[`z],:`q"];IA["W6:+`a`b!(1 2;3 4)";"W6[`z]+:1"];IA["W7:`a`b!(1 2;3 4)";".[`W7;`z;+;1]"];IA["W11:(`a`b!(1 2;3 4);5)";"W11[0;`z;1]:9"]);((::;`a`b`z!(1 2;3 4;1 1));(::;`a`b`z!(1 2;3 4;,`q));(::;+`a`b`z!(1 2;3 4;1 1));2#,`a`b`z!(1 2;3 4;1 1);(::;(`a`b`z!(1 2;3 4;0N 9);5)))]   / a key that is not there is added, with the first value nulled
t[`amendWalkTable;(IA["W8:+`a`b!(1 2;3 4)";"W8[`a;1]+:1.5"];IA["W9:+`a`b!(1 2;3 4)";"W9[1;`a]:9"];IA["W10:`a`b!(`x`y!1 2;`x`y!3 4)";"W10[`a;`x]:9"];IA["W13:+`a`b!(1 2;3 4)";"W13[0;`b]:5 6 7"]);((::;+`a`b!((1;3.5);3 4));(::;+`a`b!(1 9;3 4));(::;`a`b!+`x`y!(9 3;2 4));(::;+`a`b!(1 2;(5 6 7;4))))]   / a dict whose values are dicts of the same keys holds them as a table; a row then a column amends that column
t[`amendWalkShared;{p:(1 2;3 4);q:p 1;p[1;0]:9;d:`a`b!(1 2;3 4);e:d`a;d[`a;0]:9;s:+`a`b!(1 2;3 4);u:s`a;s[`a;0]:9;(p;q;d;e;s;u)}[];((1 2;9 4);3 4;`a`b!(9 2;3 4);1 2;+`a`b!(9 2;3 4);1 2)]   / an item amended in place is not one held elsewhere
t[`amendWalkResult;{p:(1 2;3 4);r:p[1;0]+:5;(r;p)}[];(8;(1 2;8 4))]

/ ---- where the walk stops short below a key, or meets a list of keys above the last level, the assignment goes on from there: the same values as before, and where it fails (X6, Y8, Y9) the same error, with the variable now kept ----
t[`amendWalkStopShort;(IA["X1:`a`b`c!(1 2 3;4 5 6;7 8 9)";"X1[`c;0 1]:5 6"];IA["X2:`a`b`c!(1 2 3;4 5 6;7 8 9)";"X2[`c;]:5"];IA["X3:+`a`b!(1 2 3;4 5 6)";"X3[`b;0 1]:7 8"];IA["X5:`a`b`c!(1 2 3;4 5 6;7 8 9)";".[`X5;(`c;0 1);:;5 6]"];IA["X6:`a`b`c!(1 2 3;4 5 6;7 8 9)";"X6[`c;0 1 5]:5"]);((::;`a`b`c!(1 2 3;4 5 6;5 6 9));(::;`a`b`c!(1 2 3;4 5 6;5 5 5));(::;+`a`b!(1 2 3;7 8 6));2#,`a`b`c!(1 2 3;4 5 6;5 6 9);(`err;`a`b`c!(1 2 3;4 5 6;7 8 9)))]
XH:1 2 3
t[`amendWalkStopSymbol;{r:IA["X4:`a`b!(1 2;`XH)";"X4[`b;0 1]:7 8"];(r;XH)}[];((::;`a`b!(1 2;7 8 3));7 8 3)]   / below a key, a symbol names a global, which is amended by name
t[`amendWalkKeyList;(IA["Y1:`a`b`c!(1 2 3;4 5 6;7 8 9)";"Y1[`b`c;0]:7 8"];IA["Y2:`a`b`c!(1 2 3;4 5 6;7 8 9)";"Y2[`b`b;0]+:1"];IA["Y3:`a`b`c!(1 2 3;4 5 6;7 8 9)";"Y3[`b`z;0]:7"];IA["Y4:+`a`b!(!5;!5)";"Y4[`a`b;0]:7"];IA["Y5:`a`b!(`x`y!(1 2;3 4);`x`y!(5 6;7 8))";"Y5[`a;`x`y;0]:9"];IA["Y6:`a`b`c!(1 2 3;4 5 6;7 8 9)";".[`Y6;(`b`c;0);-:]"];IA["Y7:`x`y!(`a`b!(1 2;3 4);`a`b!(5 6;7 8))";"Y7[`x`y;`b]:(7 8;9 9)"];IA["Y8:`a`b`c!(1 2 3;4 5 6;7 8 9)";"Y8[`b`c;0 9]:7"];IA["Y9:`a`b!(`x`y!(1 2;3 4);`x`y!(5 6;7 8))";"Y9[`a;`x`y;5]:9"]);((::;`a`b`c!(1 2 3;7 5 6;8 8 9));(::;`a`b`c!(1 2 3;6 5 6;7 8 9));(::;`a`b`c`z!(1 2 3;7 5 6;7 8 9;7 0N 0N));(::;+`a`b!(7 1 2 3 4;7 1 2 3 4));(::;`a`b!+`x`y!((9 2;5 6);(9 4;7 8)));2#,`a`b`c!(1 2 3;-4 5 6;-7 8 9);(::;`x`y!+`a`b!((1 2;5 6);(7 8;9 9)));(`err;`a`b`c!(1 2 3;4 5 6;7 8 9));(`err;`a`b!+`x`y!((1 2;5 6);(3 4;7 8))))]   / found once, at the places the check found; a key not there is added, as before
t[`amendFunctionInVariable;({r:IA["F1:(1;(2;{FR::F1[1;0];x>1}))";"F1[1;1]#:1 2 3"];(r;FR)}[];{r:IA["F3:(1;(2;{FR::F3[1;0];x>1}))";"F3[1;1]_:1 2 3"];(r;FR)}[];IA["F2:(0;(1;(2;{F2[1;0]:5;x>1})))";"F2[1;1;1]#:1 2 3"]);(((::;(1;(2;2 3)));2);((::;(1;(2;,1)));2);(::;(0;(1;(2;2 3)))))]   / f#y and f_y call f, here an item of the variable, which sees the variable whole, and may amend it
XW:`a`b!(1 2;3 4)
t[`amendSelfByName;{r:{.[.:;,x;{`err}]}'("SV1:`a`b!(`SV1;1 2);SV1[`a;`b]:9";"SV2:`a`b!(`SV2;1 2);SV2[`a;`a]:9";"SG:`a`b!(`SV3;5 6);SV3:`a`b!(`SG;1 2);SV3[`a;`a;`b]:9";"SV4:`a`b!(`SV4;1 2);SV4[`a;`z]:9");XW[`a;0]:9;(r;{.[.:;,x;{`lost}]}'("SV1";"SV2";"SV3";"SG";"SV4");XW;+/!1000)}[];(4#`err;(`a`b!(`SV1;1 2);`a`b!(`SV2;1 2);`a`b!(`SG;1 2);::;`a`b!(`SV4;1 2));`a`b!(9 2;3 4);499500)]   / a symbol below a key names a global, amended by name, that names the variable again: 'type, and the variable kept, as the assignment amends a copy of it (on main, lost and freed twice; the missing key it added, to a dict it could not print). SG, amended by name on the way, is lost, as an amend by name that fails leaves its global

t[`amendSelfBelowStop;{r:{.[.:;,x;{`err}]}'("SD1:(0;((`SD1;0);(`SD1;1)));SD1[1;0 1;0;1]:9";"SD2:(0;`a`b!(`SD2;`SD2));SD2[1;`a`b;1]:9";"SD3:(0;+`a`b!(`SD3`SD3;1 2));SD3[1;0;`a;1]:9";"SD4:(0;((`SD4;0);(`SD4;1)));SD4[1;;0;1]:9";"SD5:(0;((`SD5;0);(`SD5;1)));SD5[1;0 1;0;1]+:9";"SE:(0;((`SF;0);(`SF;1)));SF:(`SE;1);SE[1;0 1;0;0;1]:9");XW[`a;0]:8;(r;{.[.:;,x;{`lost}]}'("SD1";"SD2";"SD3";"SD4";"SD5";"SE";"SF");XW;+/!1000)}[];(6#`err;((0;((`SD1;0);(`SD1;1)));(0;`a`b!`SD2`SD2);(0;+`a`b!(`SD3`SD3;1 2));(0;((`SD4;0);(`SD4;1)));(0;((`SD5;0);(`SD5;1)));(0;((`SF;0);(`SF;1)));::);`a`b!(8 2;3 4);499500)]   / an amend by name below where the walk stopped (a list of indices, an elided level, a list of keys, a row) that names the variable, directly or through another global: 'type, and the variable kept, as the assignment amends a held copy of it (the check finds the symbol); SF, amended by name on the way, is ::, as an amend by name that fails leaves its global. Before, the variable was lost (and on main it read freed memory or crashed)
t[`amendThroughSymKeeps;.[{(IK["TS1:(1 2;`TSZ1);TSZ1:5";"TS1[1;0]:9"];IK["TS2:(1 2;`TSQ2)";"TS2[1;0]:9"];IK["TS3:(1 2;`TSZ3);TSZ3:5";"TS3[1;0]+:9"];IK["TS4:(1 2;`TSZ4);TSZ4:5";".[`TS4;1 0;:;9]"];IK["TS5:(1 2;(3;`TSZ5));TSZ5:5";"TS5[1;1;0]:9"];IK["TS6:`a`b!(1 2;`TSZ6);TSZ6:5";"TS6[`b;0]+:9"];IK["TS7:(1 2;`TSZ7);TSZ7:1 2";"TS7[1;0]+:`a"];IK["TS8:(1 2;`TSZ8);TSZ8:1 2";"TS8[1;0]:9"])};,0;{x}];((`err;(1 2;`TSZ1));(`err;(1 2;`TSQ2));(`err;(1 2;`TSZ3));(`err;(1 2;`TSZ4));(`err;(1 2;(3;`TSZ5)));(`err;`a`b!(1 2;`TSZ6));(`err;(1 2;`TSZ7));(::;(1 2;9 2)))]   / an amend through a symbol naming another global, which fails there: the error, and the variable kept (before, deleted, or :: by name); one that works, as before
t[`amendSelfByNameList;.[{r:{.[.:;,x;{`err}]}'("VS1:(1;(2;`VS1));VS1[1;1;0]:9";"VS2:(1;(2;`VS2));VS2[1;1;1;0]:9";"VS3:(`VS3;1 2);VS3[0;0;1]:9";"VS4:(1;(2;`VS4));VS4[1;1;0]+:9";"VS5:(1;(2;`VS6));VS6:(3;`VS5);VS5[1;1;1;0]:9");$[5#`err~r;(`k'(VS1;VS2;VS3;VS4;VS5);`k@!30);r]};,0;{x}];(("(1;(2;`VS1))";"(1;(2;`VS2))";"(`VS3;1 2)";"(1;(2;`VS4))";"(1;(2;`VS6))");`k@!30)]   / and in a list, where the walk meets the symbol (on main, V corrupt: reading it crashed); through another global too
t[`amendSelfByNameTable;.[{r:.[.:;,"VS7:+`a`b!((`VS7;1);1 2);VS7[0;`a;0]:9";{`err}];(r;`k VS7;`k@!30)};,0;{x}];(::;"+`a`b!(((9;`a`b!1 2);1);1 2)";`k@!30)]   / a table's row, then a symbol, by d4 from the first level: the amend by name is made, on the table as it was, held (on main, VS7 became addresses, and memory was reused)
t[`amendTableItemsAtIndex;(IA["TI1:+`a`b!((1 2;`TI1);(5 6;7 8))";"TI1[0;`a;0]:9"];IA["TI2:+`a`b!((1 2;3 4);(`TI2;7 8))";"TI2[0;`a`b;0]:9"];IA["TI3:+`a`b!((1 2;3 4);(5 6;7 8))";"TI3[1;`c;0]:9"];IA["TI4:+`a`b!(`x`TI4;1 2)";"TI4[1;`a;0]:9"];IA["TI5:+`a`b!((1 2;3 4);(5 6;7 8))";"TI5[;`a;0]:9"]);((::;+`a`b!((9 2;`TI1);(5 6;7 8)));(::;+`a`b!((9 2;3 4);((9;`a`b!(3 4;7 8));7 8)));(::;(`a`b!(1 2;5 6);`a`b`c!(3 4;7 8;9 0N)));(::;+`a`b!((`x;(9;`a`b!(`TI4;2)));1 2));(::;+`a`b!((9 2;9 4);(5 6;7 8))))]   / a table's rows, then columns by name: only those items are looked at (a name in another row is not met; one at the index is, and the amend by name is made on the table as it was, held; a column not there has no items)
t[`amendSelfByNameFirst;.[{r:(IA["VF1:(`VF1;`VF1)";"VF1[0 1;0]:9"];IA["VF2:(`VF2;1 2)";"VF2[;0]:9"];IA["VF3:(1;(2;`VF3))";"VF3[1 1;1;0]:9"]);(r;`k@!30)};,0;{x}];(((::;((9;`VF1);(9;`VF1)));(::;((9;1 2);9 2));(::;(1;(2;(9;(2;`VF3))))));`k@!30)]   / a list of indices or an elided level at the first level, by d4: the amend by name of the variable is made, on it as it was, and its result replaces the symbol, as on 2.7.2 (the third overflowed the stack there)
t[`amendSelfByNameFunction;.[{VW1::(1;`WW1);WW1::(1;{.[`VW1;,0;:;7];x+1});VW1[1;1]@:5;(VW1;WW1;`k@!30)};,0;{x}];((1;1 6);1 6;`k@!30)]   / a function that @: calls through a symbol amends the variable by name: made, on the copy d4 does not use, and the assignment's result replaces it, as on 2.7.2
t[`amendSelfByNameFails;.[{r:{.[.:;,x;{`err}]}'("VE1:(`WE1;`WE1);WE1:(`VE1;1);VE1[0 1;0;0;1]:9";"VE2:(`WE2;`WE2);WE2:(`VE2;1);VE2[0 1;0;0;1]@:9");(r;VE1;WE1;VE2;`k@!30)};,0;{x}];(2#`err;`WE1`WE1;::;`WE2`WE2;`k@!30)]   / held from the first level, the amend by name of the variable is made and fails: 'type, the variable put back as it was, and the global amended on the way :: (on 2.7.2 the variable was lost)
t[`amendFailKeepsIndexType;.[{(IK["K1:1 2 3";"K1[1.0]:9"];IK["K2:1 2 3";"K2[\"a\"]:9"];IK["K3:(1 2;3 4)";"K3[0;1.0]:9"];IK["K4:`a`b!(1 2;3 4)";"K4[`c;1.0]:9"];IK["K5:(0#`)!()";"K5[`a;`b]:9"];IK["K6:1 2!(3 4;5 6)";"K6[0;`b]:9"];IK["K7:+`a`b!(1 2 3;4 5 6)";"K7[`a]:`x`y!7 8"];IK["K8:+(0#`)!()";"K8[0]:9"];IK["K9:`a`b!(`x`y!1 2;`x`y!3 4)";"K9[`a`b;`x;0]:9"];{K10::1 2 3;r:.[{K10[1.0]:9};,0;{`err}];(r;.[.:;,"K10";{`lost}])}[];IK["K11:1 2 3";".[`K11;,1.0;:;9]"])};,0;{x}];((`err;1 2 3);(`err;1 2 3);(`err;(1 2;3 4));(`err;`a`b!(1 2;3 4));(`err;(0#`)!());(`err;1 2!(3 4;5 6));(`err;+`a`b!(1 2 3;4 5 6));(`err;+(0#`)!());(`err;`a`b!(`x`y!1 2;`x`y!3 4));(`err;1 2 3);(`err;1 2 3))]   / an index that indexes nothing there, a key not there (added, the first value nulled; none: an empty list), keys not symbols, a dict as a column, a table with no columns, dicts of the same keys: the check finds the error, and the variable is kept (2.7.2: lost)
t[`amendFailKeepsCount;.[{(IK["L1:(1 2 3;4 5 6)";"L1[0 1;1]:1 2 3"];IK["L2:(1 2 3;4 5 6)";"L2[;1]:1 2 3"];IK["L3:(1 2 3;4 5 6)";"L3[]:1 2 3"];IK["L4:(1 2 3;4 5 6)";"L4[0;]:1 2"];IK["L5:`a`b!(1 2;3 4)";"L5[`a`b;0]:1 2 3"];IK["L6:+`a`b!(1 2;3 4)";"L6[0 1;`a]:1 2 3"];IK["L7:((1 2;3 4);(5 6;7 8))";"L7[0;0 1;0]:1 2 3"];IK["L8:(1 2 3;4 5 6)";"L8[0 1;1]+:1 2 3"];IK["L9:(1 2 3;4 5 6)";"L9[0 1;0 1]:(1 2;3 4 5)"])};,0;{x}];{(`err;x)}'((1 2 3;4 5 6);(1 2 3;4 5 6);(1 2 3;4 5 6);(1 2 3;4 5 6);`a`b!(1 2;3 4);+`a`b!(1 2;3 4);((1 2;3 4);(5 6;7 8));(1 2 3;4 5 6);(1 2 3;4 5 6))]   / a value that does not conform below an index list or an elided level: 'length before anything is touched (2.7.2: lost)
t[`amendFailKeepsTable;.[{(IK["N1:+`a`b!(1 2 3;4 5 6)";"N1[;`a]:9 8"];IK["N2:+`a`b!(1 2 3;4 5 6)";"N2[;`a`b]:(9 8 7;6 5 4)"];IK["N3:+`a`b!((1 2;3 4;5 6);4 5 6)";"N3[;`a;0]:9 8"];IK["N4:+`a`b!(1 2 3;4 5 6)";"N4[`a`b]:(7 8 9;1 2)"];IK["N5:+`a`b!(1 2 3;4 5 6)";"N5[`a`c]:(7 8 9;1 2)"];IK["N6:+`a`b!(1 2 3;4 5 6)";"N6[`c;5]:9"];IK["N7:+`a`b!(1 2 3;4 5 6)";"N7[`c;]:9 8"];IK["N8:+(0#`)!()";"N8[`a;0]:9"];IK["N9:+`a`b!(1 2 3;4 5 6)";"N9[;`a]+:9 8"];IK["N10:+`a`b!(1 2 3;4 5 6)";"N10[`c`d;5]:9 8"];IK["N11:+`a`b!(1 2 3;4 5 6)";"N11[`c`d;`x]:9 8"];IK["N12:+(0#`)!()";"N12[`a`b;0]:9 8"];IK["N13:+`a`b!(!0;!0)";"N13[`c`d;0]:9 8"])};,0;{x}];{(`err;x)}'(+`a`b!(1 2 3;4 5 6);+`a`b!(1 2 3;4 5 6);+`a`b!((1 2;3 4;5 6);4 5 6);+`a`b!(1 2 3;4 5 6);+`a`b!(1 2 3;4 5 6);+`a`b!(1 2 3;4 5 6);+`a`b!(1 2 3;4 5 6);+(0#`)!();+`a`b!(1 2 3;4 5 6);+`a`b!(1 2 3;4 5 6);+`a`b!(1 2 3;4 5 6);+(0#`)!();+`a`b!(!0;!0))]   / a table: every row, columns of other counts, a column or columns to add with an index below, a table with no columns by a key: the error before anything is touched (2.7.2: lost)
t[`amendFailKeepsStillWorks;({.[.:;,x;{`$1_(x?"\n")#x}]}'("M1:1 2 3;M1[1.0]:9";"M2:1 2 3;M2[\"a\"]:9";"M3:(1 2;3 4);M3[0 1;5]:1 2 3");IK["M4:(1 2 3;4 5 6)";"M4[0 1;1]:7 8"];IK["M5:`a`b!(1 2;3 4)";"M5[`a`c;0]:9 8"];IK["M6:1 2!(3 4;5 6)";"M6[1;0]:9"];IK["M7:+`a`b!(1 2;3 4)";"M7[;`a]:7 8"];IK["M8:1 2 3";"M8[(0 1;2)]:(4 5;6)"];IK["M9:!200";"M9[\"a\"]:0"][1;97];IK["M10:0#`a`b!(1 2;3 4)";"M10[`c;0]:9"];IK["M11:(0#`)!0#,1 2";"M11[`a`b;0]:9 8"]);(`type`index`length;(::;(1 7 3;4 8 6));(::;`a`b`c!(9 2;3 4;8 0N));(::;1 2!(9 4;5 6));(::;+`a`b!(7 8;3 4));(::;4 5 6);0;(::;(,`c)!,9 0N);(::;`a`b!(9 0N;8 0N)))]   / the error is the amend's own (a count first); and what fits is assigned as before (a char indexes by its code; a key added to a dict with no keys gets its values' prototype nulled)
t[`amendFailKeepsEveryKey;(IK["EK1:`a`b!1 2";"EK1[]:1 2 3"];IK["EK2:`a`b!(1 2;3 4)";"EK2[;0]:7 8 9"];IK["EK3:`a`b!(`x`y!1 2;`x`y!3 4)";"EK3[`a;]:7 8 9"];IK["EK4:(1;`a)!(1 2;3 4 5)";"EK4[;2]:9"];IK["EK5:+`a`b!(1 2 3;4 5 6)";"EK5[0;]:9 8 7"];IK["EK6:`a`b!(`x`y!(1 2;3);`x`y!(4 5;6))";"EK6[`a`b;;0]:9 8"];IK["EK7:`a`b!(1 2;3 4)";"EK7[;0]:7 8"]);((`err;`a`b!1 2);(`err;`a`b!(1 2;3 4));(`err;`a`b!(`x`y!1 2;`x`y!3 4));(`err;(1;`a)!(1 2;3 4 5));(`err;+`a`b!(1 2 3;4 5 6));(`err;`a`b!(`x`y!(1 2;3);`x`y!(4 5;6)));(::;`a`b!(7 2;8 4)))]   / an elided level on a dict is every key, so its values are checked as a list elided there (any keys), and on a table's row every column: the error before anything is touched (lost before)
t[`amendVerbFailKeyNotSym;.[{(IK["VK1:1 3 5!10 20 30";"VK1[3]+:`a"];IK["VK2:1 3 5!10 20 30";".[`VK2;,3;+;`a]"];IK["VK3:1 3 5!10 20 30";".[`VK3;3;{x+y+`a};1]"];IK["VK4:1 3 5!10 20 30";".[`VK4;,3;{`a+x}]"];IK["VK5:\"abc\"!1 2 3";"VK5[\"b\"]-:`a"];IK["VK6:(1;`a;\"x\")!1 2 3";"VK6[`a]*:`q"];IK["VK7:1 2!(`a`b!1 2;`a`b!3 4)";"VK7[1]+:`q"];IK["VK8:1 3 5!10 20 30";"VK8[3]+:1"];IK["VK9:1 3 5!10 20 30";"VK9[3]+:1 2"];IK["VK10:1 3 5!10 20 30";"VK10[7]+:1"];IK["VK11:1 3 5!({x};{x};{x})";"VK11[7]@:1"];IK["VK12:1 3 5!({x};{x};{x})";"VK12[7].:,1"];IK["VK13:1 3 5!({x};{x};{x})";"VK13[7]_:1"];IK["VK14:1 3 5!({x};2;3)";"VK14[7]@:1"];IK["VK15:1 3 5!(1;2;{x})";"VK15[7]@:`q"])};,0;{x}];((`err;1 3 5!10 20 30);(`err;1 3 5!10 20 30);(`err;1 3 5!10 20 30);(`err;1 3 5!10 20 30);(`err;"abc"!1 2 3);(`err;(1;`a;"x")!1 2 3);(`err;1 2!(`a`b!1 2;`a`b!3 4));(::;1 3 5!10 21 30);(::;1 3 5!(10;21 22;30));(::;1 3 5 7!10 20 30 1);(`err;1 3 5!({x};{x};{x}));(`err;1 3 5!({x};{x};{x}));(`err;1 3 5!({x};{x};{x}));(`err;1 3 5!({x};2;3));(`err;1 3 5!(1;2;{x})))]   / a verb that fails at a key of a dict whose keys are not symbols: the error, and the dict kept, as for symbol keys (before, deleted, or :: by name); one that works, as before
t[`amendName3Arg;.[{(IK["N3A1:1 2 3";".[`N3A1;,1.0;{x}]"];IK["N3A2:1 2 3";".[`N3A2;,\"a\";{x}]"];IK["N3A3:1 2 3";".[`N3A3;,5;{x}]"];IK["N3A4:`a`b!(1 2 3;4 5 6)";".[`N3A4;(`a;5);{x}]"];IK["N3A5:(1 2;3 4)";".[`N3A5;(0;`a);{x}]"];IK["N3A6:1 2 3";".[`N3A6;,1;{x*10}]"];IK["N3A7:1 2 3";".[`N3A7;,1;{x+`a}]"])};,0;{x}];((`err;1 2 3);(`err;1 2 3);(`err;1 2 3);(`err;`a`b!(1 2 3;4 5 6));(`err;(1 2;3 4));(1 20 3;1 20 3);(`err;1 2 3))]   / .[`v;i;f], the three-argument form, is checked as the four-argument one is: an index that fails, and v is kept (before, ::); f failing, or working, as before
t[`amendVerbFailList;.[{(IK["VL1:1 2 3";"VL1[0 1]+:`q"];IK["VL2:`a`b!1 2";"VL2[`a`b]+:`q"];IK["VL3:+`a`b!(1 2 3;4 5 6)";"VL3[`a`b]+:`q"];IK["VL4:1 2 3";".[`VL4;,0 1;+;`q]"];IK["VL5:1 2 3";"VL5[0 1 2 0]+:(1;2.5;\"a\";`q)"];IK["VL6:1 3 5!10 20 30";"VL6[1 3]+:`a"];IK["VL7:(1;`a;\"x\")";"VL7[0 1]+:(1;2)"];IK["VL8:`a`b!1 2";".[`VL8;,`a`b;{x+y};(1;`q)]"];IK["VL9:1 2 3";"VL9[0 0 2]+:(1.5;2;3)"];IK["VL10:`a`b`c!1 2 3";"VL10[`c`a]+:10 20"];IK["VL11:+`a`b!(1 2;3 4)";"VL11[`b`a]*:(2;1.5)"];IK["VL12:`a`b!1 2";"VL12[`a`c]+:5"];IK["VL13:1 2 3";".[`VL13;,1 0;{$[x=1;`q;x]+1}]"];IK["VL14:`a`b!(1 2;3 4)";"VL14[`a`b]+:(9;`q)"];IK["VL15:1 2 3";".[`VL15;,0 1;{x*2}]"];IK["VL16:(1 2;3 4)";"VL16[1;0 1]+:(1;`a)"];IK["VL17:`a`b!(1 2;`x`y!3 4)";"VL17[`b;`x`y]+:(1;`q)"];IK["VL18:(1 2;3 4)";".[`VL18;(1;0 1);{x+`q}]"];IK["VLZ19:(1 2;3 4);VL19:(0;`VLZ19)";"VL19[1;1;0 1]+:(1;`a)"])};,0;{x}];((`err;1 2 3);(`err;`a`b!1 2);(`err;+`a`b!(1 2 3;4 5 6));(`err;1 2 3);(`err;1 2 3);(`err;1 3 5!10 20 30);(`err;(1;`a;"x"));(`err;`a`b!1 2);(::;(4.5;2;6));(::;`a`b`c!21 2 13);(::;+`a`b!(1.5 3.0;6 8));(::;`a`b`c!6 2 5);(`err;1 2 3);(`err;`a`b!(1 2;3 4));(2 4 3;2 4 3);(`err;(1 2;3 4));(`err;`a`b!(1 2;`x`y!3 4));(`err;(1 2;3 4));(`err;(1 2;3 4)))]   / a verb that fails part way along a list of indices, of keys, of columns (at the first level, or the last below one index per level): the error, and the variable as it was, its type too (before, deleted, or :: by name); by name with three arguments too; one that works, as before (a repeated index takes the result before it)

/ ---- `ejx (y's keys;x's keys): the rows of an equi-join, (rows of x;rows of y) ----
/ Each row of x in order, with every row of y that has its key, in y's order (amber.k's
/ ej). Keys compare as find does: -0.0 is 0.0, NaN is NaN, null is null, integers of any
/ width by value. y with a repeated key takes another path, with more than four rows to a
/ key in ejxRepeats. Keys it does not take (chars here) give () and ej's K path.
t[`ejxUnique;`ejx(`a`b`c;`c`d`a`c);(0 2 3;2 0 2)]
t[`ejxRepeats;`ejx(1 2 1 1 1 1 2;1 3 2);(0 0 0 0 0 2 2;0 2 3 4 5 1 6)]
t[`ejxFloat;`ejx(0.0 0n 1.5;-0.0 0n 2.0 0.0);(0 1 3;0 1 0)]
t[`ejxWide;`ejx(5000000000 -7 0N;0N 3 5000000000 -7);(0 2 3;2 0 1)]
t[`ejxWidths;`ejx(1 2 300;300 1 70000);(0 1;2 0)]
t[`ejxEmpty;(`ejx(!0;1 2);`ejx(1 2;!0));2#,(!0;!0)]
t[`ejxOther;`ejx("ab";"ba");()]

/ ---- +/ of a range is the sum of its items, wrapped as int addition is ----
t[`rangeSumOne;+/9223372036854775803+!1;9223372036854775803]
t[`rangeSumWrap;+/4611686018427387904+!2;-9223372036854775807]
t[`rangeSumThree;+/9223372036854775800+!3;9223372036854775787]
t[`rangeSumSmall;+/-5+!1005;499485]
t[`rangeSumTil;+/!10;45]

/ ---- x!y for a large modulus x (the atom path added x to the remainder and overflowed) ----
t[`modLarge;(9223372036854775807!1;9223372036854775807!-1;5000000000000000000!4000000000000000000);1 9223372036854775806 4000000000000000000]
t[`modAgreesVec;(9223372036854775807!1 -1)~(9223372036854775807!1),9223372036854775807!-1;1b]
t[`modSmall;(7!2;7!-2;2!-7;-2!7;-2!-7;0!5);2 5 1 3 -4 5]

/ ---- each over an enumeration beyond 2^31 (upstream ngn/k 4dbf859f) ----
t[`eachEnum;{x}'2147483646+!4;2147483646 2147483647 2147483648 2147483649]
t[`eachEnumStr;$2147483647+!2;("2147483647";"2147483648")]

/ ---- a sparse deal from over 2^31 stays in range (upstream ngn/k ea79e42a) ----
tv[`dealLarge;"r:-1000?5000000000;(~0>&/r;5000000000>|/r;1000=#?r)";1 1 1]

/ ---- 0! of chars gives ints, as 1! does (upstream ngn/k 60d587e0) ----
t[`modZeroChars;(0!"ab";1!"ab";0!"a");(97 98;0 0;97)]

/ ---- dividing chars by a negative int gives ints, as 0! and every other ! do ----
t[`divChars;-2!"ab";48 49]
t[`divCharsOne;-1!"ab";97 98]
t[`divCharsHigh;-2!0x8081;-64 -64]
t[`divCharsEmpty;@-2!"";`I]

/ ---- a float prints as the shortest text that is nearest to it ----
/ (the values are built exactly, without reading a float literal)
t[`fltPrint2m24;`k@1%16777216.0;"5.960464477539063e-8"]
t[`fltPrint2p89;`k@*/89#2.0;"6.189700196426902e26"]
t[`fltPrint2p62a;`k@`f$4350140171453357568;"4.3501401714533576e18"]
t[`fltPrint2p61;`k@`f$2305844108725321728;"2.3058441087253217e18"]
t[`fltPrintSmall;`k@0.5 0.25 1.5 100.0;"0.5 0.25 1.5 100.0"]

/ ---- indexing with a table gives a table (upstream ngn/k b87b7d27) ----
t[`indexTable;1 2 3@+`c`e!(0 1;2 0);+`c`e!(1 2;3 1)]
t[`indexDict;1 2 3@`c`e!(0 1;2 0);`c`e!(1 2;3 1)]

/ ---- empty generic lists order by prototype, so the order agrees with ~ ----
t[`distinctEmpties;?(1;0#,1 2;();0#,1 2);(1;0#,0N 0N;())]
t[`distinctEmptiesCount;#?(();0#,1 2;();0#,1 2;0#,"ab");3]
t[`gradeEmpties;<(0#,1 2;();0#,1 2);1 0 2]
t[`gradeEmptiesSorted;{s:x@<x;(<s)~!#s}@(0#,1 2;();0#,1 2;();0#,"ab");1b]

/ ---- a dict whose keys are an empty generic list prints so it reads back ----
t[`dictKeysEmptyText;`k@(0#,,1)!();"(0#,,0N)!()"]
t[`dictKeysEmptyBack;{x~. `k@x}@(0#,,1)!();1b]
t[`dictKeysEmptyParen;`k@()!();"()!()"]
t[`dictKeysVec;`k@(`a`b!1 2);"`a`b!1 2"]

/ ---- a one-item range as dict keys prints in parentheses, so it reads back ----
t[`rangeKeyPrint;`k@(!1)!,2;"(,0)!,2"]
t[`rangeKeyPrint5;`k@(5+!1)!,2;"(,5)!,2"]
t[`rangeKeyRoundTrip;(. `k@(!1)!,2)~(!1)!,2;1]
t[`rangeKeyTwo;`k@(!2)!3 4;"0 1!3 4"]

/ ---- an empty generic list joined to an empty list keeps its prototype; only () gives way (2.7) ----
t[`joinEmptyGenericProto;`k@'((0#,1 2),0#,"ab";(0#,,0N),0#,,();(0#,1 2),();(),0#,"ab");("0#,0N 0N";"0#,,0N";"0#,0N 0N";"0#,\"  \"")]
t[`joinEmptyGenericDictCmp;`k@'(((0#,1 2)!())=(0#`)!!0;((0#,1 2)!0#0n)<(0#`)!!0);2#,"(0#`)!!0"]

/ ---- a train (a list of two or three functions) whose element fails passes the error on: the hook or fork applied the next element to the error (a crash) ----
t[`trainElementError;{*"\n"\.[.:;,x;{x}]}'("({`a+x};{`b+x})@!0";"({`a+x};{`b+x}) 2";"({`b+x};-;+) 5";"(+;-;{`b+x}) 5";"(-;{`b+x})[1;2]";"({`b+y};-;+)[1;2]";"(+;-;{`b+y})[1;2]");7#,"'type"]
/ ---- a train owns its arguments as _1/_2 do: a failing element must not free the borrowed left argument (hook or fork); a heap int argument (0N, a long) was freed by the hook-or-index test ----
t[`trainLeftArgKept;{a:1+!3;{.[{(-;{`b+x})[x;2]};,x;{x}]}[a]'!3;{.[{(+;-;{`b+y})[x;2]};,x;{x}]}[a]'!3;a}[];1 2 3]
t[`trainHeapIntArg;({#?{(+;-) 0N+0*x}'!x}2000;{#?{[v;i](+;-) v}[x]'!200}2000000000000000000);1 1]
t[`trainHeapIntErr;({#?{[v;i].[{(+;{`b+x}) x};,v;{x}]}[x]'!200}0N;#?{.[{({`a+x};{`b+x}) x};,0N;{x}]}'!100);1 1]

/ ---- ?[c;a;b] with c of 0s and 1s is q's vector conditional, with no library loaded ----
tv[`vcondAtoms;"?[1 0 1;1;9]";1 9 1]
tv[`vcondList;"?[1 0 1;1 2 3;9]";1 9 3]
tv[`vcondEmpty;"?[!0;0;0]";!0]
tv[`vcondSplice;"?[2 3 4;(0;1);9]";9 3 4]
/ the core's lambda uses no global name: a user's isat (or the harness's own t) is not called
tv[`vcondOwnIsat;"isat:{1b};?[1 0 1;1 2 3;9]";1 9 3]

/ ---- a parse does not depend on the text parsed before it: just past a lambda's [params] a - starts a number,
/ and where an earlier text's [params] ended was kept, so a later text in the same memory (the REPL's line
/ buffer, or a string . reuses) read ppeV-1 there as ppeV applied to -1 ----
ppeV:5 6
t[`ppeStale;?{."{[ab]ab}",x;."(ppeV-1)",x}'(!100)#\:" ";,4 5]

/ ---- a keyed table is looked up by key, as q: the value row, a row of nulls for a missing key (issue #19) ----
kL:(+(,`k)!,1 2 3)!+(,`v)!,10 20 30
kM:(+`a`b!(1 2;`x`y))!+(,`v)!,5 6
t[`keyedAt;(kL 2;kL@3;kL[1];kL 99;kL(,`k)!,2);((,`v)!,20;(,`v)!,30;(,`v)!,10;(,`v)!,0N;(,`v)!,20)]
/ a table of keys, or a list of keys (each a list of one value per column), gives a table; one such list is one key
t[`keyedAtKeys;(kL[+(,`k)!,2 9];kL[(,1;,99)];kL[,2];kL());(+(,`v)!,20 0N;+(,`v)!,10 0N;(,`v)!,20;+(,`v)!,!0)]
t[`keyedAtCol;(kL[2;`v];kL[99;`v];kL[(,1;,2);`v];kL[+(,`k)!,3 1;`v]);(20;0N;10 20;30 10)]
t[`keyedMulti;(kM(1;`x);kM(2;`z);kM((1;`x);(2;`y));kM`a`b!(2;`y);kM[+`a`b!(2 1 9;`y`x`x)];kM[`a`b!(1;`x);`v]);((,`v)!,5;(,`v)!,0N;+(,`v)!,5 6;(,`v)!,6;+(,`v)!,6 5 0N;5)]
t[`keyedSymTemporal;{s:(+(,`s)!,`a`b)!+(,`v)!,1.5 2.5;d:(+(,`d)!,2020.01.01 2020.01.02)!+(,`v)!,1 2;n:(+(,`n)!,2020.01.01D10:00:00 2020.01.01D11:00:00)!+(,`v)!,1 2;(s`b;s`c;d 2020.01.02;d 2020.01.05;n 2020.01.01D11:00:00)}[];((,`v)!,2.5;(,`v)!,0n;(,`v)!,2;(,`v)!,0N;(,`v)!,2)]
t[`keyedNullRow;((+(,`k)!,1 2)!+`v`w!(3 4;`a`b))9;`v`w!(0N;`)]
t[`keyedEmpty;(((+(,`k)!,!0)!+(,`v)!,!0)1;((+(,`k)!,!0)!+(,`v)!,!0)[+(,`k)!,1 2]);((,`v)!,0N;+(,`v)!,0N 0N)]
/ a row or table of keys goes by column name, as q; a char looked up in strings is a key not there (q: a null row too)
t[`keyedByName;(kM`b`a!(`y;2);kM[+`b`a!(`y`x;2 1)];(+`a`b!(1 2;3 4))?`b`a!4 2;{s:(+(,`s)!,("ab";"cd"))!+(,`v)!,1 2;((s"a")`v;(s"cd")`v;(s[,"ab"])`v)}[]);((,`v)!,6;+(,`v)!,6 5;1;0N 2 1)]
/ a key of the wrong length is 'length, as in q; a row or table with columns that differ is 'domain (q: 'mismatch)
t[`keyedAtErr;{*"\n"\.[.:;,x;{x}]}'("kL 1 2";"kM 1";"kL`a`b!1 2";"kL[+(,`j)!,1]");("'length";"'length";"'domain";"'domain")]
/ find of a row, or of a table's rows, in a table; and a keyed table's find gives the key
t[`tableFind;((+`a`b!(1 2;3 4))?`a`b!2 4;(+`a`b!(1 2;3 4))?+`a`b!(2 9;4 1);(+(,`a)!,1 2)?2;kL?(,`v)!,20;kL _ 1);(1;1 0N;1;(,`k)!,2;(+(,`k)!,2 3)!+(,`v)!,20 30)]
/ more columns: each row's places in the columns, numbered column by column, find what a scan of the rows finds
t[`keyedMultiMany;{b:13!i:!91;a:7!i;q:13!j:!200;p:7!j;((+`a`b!(a;b))?+`a`b!(p;q);(+(a;b))?+(p;q);(+`a`b!(a;b))?`a`b!(3;5))}[];{b:13!i:!91;a:7!i;q:13!j:!200;p:7!j;((+(a;b))?+(p;q);(+(a;b))?+(p;q);(+(a;b))?(3;5))}[]]

/ ---- , of two keyed tables is q's upsert; an amend at a keyed table's keys adds the keys not there, as q (issue #19) ----
t[`keyedUpsert;(kL,([k:3 4]v:33 44);kL,([k:,3]v:,5);kL,([k:2 2 5]v:1 2 3);([k:!0]v:!0),([k:2 1]v:7 8));(([k:1 2 3 4]v:10 20 33 44);([k:1 2 3]v:10 20 5);([k:1 2 3 5]v:10 2 30 3);([k:2 1]v:7 8))]
/ y's rows of keys x lacks are each added, a key given twice too, as q (an amend at keys adds a key once, as q)
t[`keyedUpsertNew;(kL,([k:5 5]v:1 2);kL,([k:2 5 5]v:7 8 9);kL,([k:3 3]v:1 2));(([k:1 2 3 5 5]v:10 20 30 1 2);([k:1 2 3 5 5]v:10 7 30 8 9);([k:1 2 3]v:10 20 2))]
/ the key and value columns go by name; columns that differ are 'domain (q: 'mismatch), a dict 'type (q: 'type or 'length)
t[`keyedUpsertByName;({k:([k:1 2]v:3 4;w:`a`b);k,([k:2 3]w:`z`y;v:9 9)}[];kM,([b:`x`z;a:1 3]v:7 8));(([k:1 2 3]v:3 9 9;w:`a`z`y);([a:1 2 3;b:`x`y`z]v:7 6 8))]
t[`keyedUpsertUnkeyed;{*"\n"\.[.:;,x;{x}]}'("kL,([])";"([]),kL";"kL,0#([]a:1 2)";"kL,([]a:1 2)";"([]k:4 5;v:1 2),kL";"(0#kL),([]k:1 2;v:3 4)");6#,"'type"]   / a keyed table and an unkeyed one, either way, empty or not: 'type, as q
t[`keyedJoinEmpty;(kL,();(),kL;(0#kL),();(),0#kL;kL,0#,1 2;{d:(+(,`a)!,1 2)!3 4;(d,();(),d)}[]);(kL;kL;0#kL;0#kL;kL;2#,(+(,`a)!,1 2)!3 4)]   / () is no rows: a keyed table joined to it, either way, is the keyed table, as q (and a dict keyed by a table)
t[`keyedJoinTypedEmpty;{$[`C~@r:.[.:;,x;{x}];*"\n"\r;""]}'("kL,!0";"kL,\"\"";"kL,0#`";"(!0),kL";"\"\",kL";"(!0),(+(,`a)!,1 2)!3 4");6#,"'type"]   / a typed empty list: 'type, as q
/ a dict whose keys are a table and values a list stays 'nyi in an amend at keys and in , (it was not a keyed table)
t[`keyedListValues;{*"\n"\.[.:;,x;{x}]}'(("dL:(+(,`a)!,1 2)!3 4;",)'("dL[,1]:9";"dL[,3]:9";"@[dL;,1;:;9]";"dL,(+(,`a)!,,3)!,5";"dL,(+(,`a)!,,3)!5"),("kL,(+(,`k)!,,3)!,5";"{d:(+(,`a)!,1 2)!3 4;d[,1]:9;d}[]"));7#,"'nyi"]
t[`keyedUpsertErr;{*"\n"\.[.:;,x;{x}]}'("kL,`k`v!4 40";"kL,(,`v)!,5";"kL,([k:,3]w:,1)";"kL,([j:,3]v:,1)");("'type";"'type";"'domain";"'domain")]
t[`keyedAmend;(@[kL;1;:;,7];@[kL;5;:;,7];.[kL;(9;`v);:;99];@[kL;1;+;1];@[kL;1;:;5];@[kL;(,5;,5);:;(,1;,2)]);(([k:1 2 3]v:7 20 30);([k:1 2 3 5]v:10 20 30 7);([k:1 2 3 9]v:10 20 30 99);([k:1 2 3]v:11 20 30);([k:1 2 3]v:5 20 30);([k:1 2 3 5]v:10 20 30 2))]
t[`keyedAmendCols;{k:([k:1 2]v:3 4;w:`a`b);(@[k;2;:;(99;`z)];@[k;9;:;(98;`y)];@[k;(,1;,8);:;((1;`m);(2;`n))];@[k;([]k:1 8);:;([]v:5 6;w:`r`s)])}[];(([k:1 2]v:3 99;w:`a`z);([k:1 2 9]v:3 4 98;w:`a`b`y);([k:1 2 8]v:1 4 2;w:`m`b`n);([k:1 2 8]v:5 4 6;w:`r`b`s))]
t[`keyedAmendLocal;{k:kL;k[3]:,99;k[4]:,98;k[2;`v]:5;k[7;`v]:6;k[1]+:,1;m:kM;m[(3;`q)]:,51;(k;m)}[];(([k:1 2 3 4 7]v:11 5 99 98 6);([a:1 2 3;b:`x`y`q]v:5 6 51))]
tv[`keyedAmendGlobal;"kG:kL;kG[3]:,99;kG[4]:,98;kG,:([k:1 8]v:0 0);kG";([k:1 2 3 4 8]v:0 20 99 98 0)]
t[`keyedAmendErr;{*"\n"\.[.:;,x;{x}]}'("@[([k:1 2]v:3 4;w:`a`b);1;:;5]";"@[kL;1 9;+;1]");("'length";"'length")]

/ ---- a failed amend by name of a keyed table keeps it, as q: it is put back (issue #19) ----
KG:([k:1 2 3]v:10 20 30;w:`a`b`c)
t[`amendKeyedKeep;(IK["KG1:KG";"KG1[1 9]:7 8"];IK["KG2:KG";"KG2[1]:5"];IK["KG3:KG";"KG3[1]:`z!,77"];IK["KG4:KG";"KG4[`a`b`c]:1"];IK["KG5:KG";"KG5[::]:9"];IK["KG6:KG";"KG6[]:9"];IK["KG7:KG";"KG7[1;`zz;0]:1"];IK["KG8:KG";".[`KG8;,9;-:]"];IK["KG9:KG";"KG9[1]+:`x"];IK["KG10:KG";".[`KG10;,1;:;5]"]);10#,(`err;KG)]
/ and one that works assigns it, by key, a key added, by column, elided, or by .[`v;..]
t[`amendKeyedByName;(IK["KH1:KG";"KH1[3]:(99;`z)"];IK["KH2:KG";"KH2[4]:(98;`y)"];IK["KH3:KG";"KH3[3;`v]:99"];IK["KH4:KG";"KH4[;`v]:9"];IK["KH5:KG";"KH5[;`v]+:1"];IK["KH6:KG";".[`KH6;(::;`v);:;9]"];IK["KH7:KG";".[`KH7;,3;:;(1;`q)]"])[;1];(([k:1 2 3]v:10 20 99;w:`a`b`z);([k:1 2 3 4]v:10 20 30 98;w:`a`b`c`y);([k:1 2 3]v:10 20 99;w:`a`b`c);([k:1 2 3]v:9 9 9;w:`a`b`c);([k:1 2 3]v:11 21 31;w:`a`b`c);([k:1 2 3]v:9 9 9;w:`a`b`c);([k:1 2 3]v:10 20 1;w:`a`b`q))]
/ a keyed table in a dict or list is amended by key below the first level too
t[`amendKeyedNested;(IK["KN1:`a`b!(KG;1)";"KN1[`a;3]:(99;`z)"];IK["KN2:`a`b!(KG;1)";"KN2[`a;4;`v]:7"])[;1];(`a`b!(([k:1 2 3]v:10 20 99;w:`a`b`z);1);`a`b!(([k:1 2 3 4]v:10 20 30 7;w:`a`b`c`);1))]
/ a list of keyed tables (a table whose column names are a table) is amended as before, and fails with the error it gave
KL:([k:1 2 3]v:10 20 30)
t[`amendKeyedTableList;(IK["L1:(KL;KL)";"L1[1]:5"];IK["L2:(KL;KL)";"L2[0]+:1"];IK["L3:3#,KL";"L3[0]:KL"];IK["L4:(KL;KL)";".[`L4;,0;:;5]"];{l:(KL;KL);l[0]:5;l}[]);((::;(KL;5));(::;(1+KL;KL));(::;3#,KL);((5;KL);(5;KL));(5;KL))]
t[`amendKeyedListKeep;(IK["DL1:(+(,`a)!,1 2)!3 4";"DL1[,1]:9"];IK["DL2:(+(,`a)!,1 2)!3 4";"DL2[,3]+:1"]);2#,(`err;(+(,`a)!,1 2)!3 4)]   / keys a table, values a list: 'nyi, and kept
t[`amendKeyedTableListErr;{*"\n"\.[.:;,x;{x}]}'("L5:(KL;KL);L5[;3]:,1";"L6:(KL;KL);L6[0 1;3]:,1";"L7:(KL;KL);L7[5;3]:,1");("'length";"'length";"'index")]
t[`amendKeyedTableListKeep;(IK["KL1:(KL;KL)";"KL1[5]:1"];IK["KL2:(KL;KL)";"KL2[0 1]:1 2 3"];IK["KL3:(KL;KL)";"KL3[;3]:,1"];IK["KL4:(KL;KL)";".[`KL4;(5;3);:;,1]"];IK["KL7:(KL;KL)";"KL7[;3]+:,1"];IK["KL8:3#,KL";"KL8[5]:KL"]);(5#,(`err;(KL;KL))),,(`err;3#,KL)]   / and is checked as the list of its rows: the error before anything is touched, which is kept
t[`amendKeyedTableListKeepErr;{*"\n"\.[.:;,x;{x}]}'("KL9:(KL;KL);KL9[5]:1";"KL9[0 1]:1 2 3";"KL9[;3]:,1";".[`KL9;(5;3);:;,1]";"KL9[1.5]:1";"KL9[0 5]:1");("'index";"'length";"'length";"'index";"'type";"'index")]   / with the error the amend gives
t[`amendKeyedTableListEmpty;{K0::0#KL;(IK["KE1:(K0;K0)";"KE1[5]:1"];IK["KE3:(K0;K0)";"KE3[0 1]:1 2 3"];IK["KE4:(K0;K0)";".[`KE4;(5;3);:;,1]"];IK["KE5:3#,K0";"KE5[5]:K0"])}[];(3#,(`err;(0#KL;0#KL))),,(`err;3#,0#KL)]   / empty keyed tables, which have no columns: their rows as the amend reads them (blw, from the columns' prototype)

/ ---- + - * % & | of two keyed tables over the union of the keys, as q (issue #19): a key on both sides gets the verb,
/ row by row; one on one side passes through as dicts' do (+ - * with the identity: - negates y's), x's keys first ----
kA:([k:1 2 3]v:4 5 6;w:1.5 2.5 3.5);kB:([k:2 3 4]v:10 20 30;w:10 20 30.0)
t[`keyedArithCols;{x[kA;kB]}'(+;-;*;&;|);(([k:1 2 3 4]v:4 15 26 30;w:1.5 12.5 23.5 30);([k:1 2 3 4]v:4 -5 -14 -30;w:1.5 -7.5 -16.5 -30);([k:1 2 3 4]v:4 50 120 30;w:1.5 25 70 30.0);([k:1 2 3 4]v:4 5 6 30;w:1.5 2.5 3.5 30);([k:1 2 3 4]v:4 10 20 30;w:1.5 10 20 30.0))]
t[`keyedArithRight;(kB-kA;kA+kA;kA+([k:2 5]w:1 2.0;v:7 8));(([k:2 3 4 1]v:5 14 30 -4;w:7.5 16.5 30 -1.5);([k:1 2 3]v:8 10 12;w:3 5 7.0);([k:1 2 3 5]v:4 12 6 8;w:1.5 3.5 3.5 2))]
/ % passes a key on one side through as for dicts (q: 'type for an int column); types combine as lists do (q too)
t[`keyedArithTypes;(([k:1 2]v:1 2)%([k:2 3]v:2 4);([k:1 2]v:1 2)+([k:2 3]v:5.5 6));(([k:1 2 3]v:(1;1.0;4));([k:1 2 3]v:(1;7.5;6.0)))]
/ two key columns by name, a repeated key of x (the first gets y's row), empty tables, +: on a variable
t[`keyedArithKeys;(([a:1 2;b:`x`y]v:1 2;w:3 4)+([b:`y`z;a:2 3]w:10 20;v:5 6);([k:1 2 2]v:1 2 3)+([k:2 3]v:10 20);([k:0#0]v:0#0)+([k:2 1]v:7 8);kL-([k:0#0]v:0#0);{k:kL;k+:([k:3 4]v:1 1);k}[]);(([a:1 2 3;b:`x`y`z]v:1 7 6;w:3 14 20);([k:1 2 2 3]v:1 12 3 20);([k:2 1]v:7 8);kL;([k:1 2 3 4]v:10 20 31 1))]
/ a dict keyed by a table with list values, as q; other key columns 'domain (q: 'mismatch); other value columns 'domain (q: the union of the columns, 0 for + -, nulls for * & |, 'type for %); a table of values on one side only 'type, as q
t[`keyedArithLists;((([]a:1 2)!(1 2;3 4))+([]a:2 3)!(10 20;30 40);(([]a:1 2 3)!4 5 6)-([]a:2 3 4)!10 20 30);((+(,`a)!,1 2 3)!(1 2;13 24;30 40);(+(,`a)!,1 2 3 4)!4 -5 -14 -30)]
t[`keyedArithErr;{*"\n"\.[.:;,x;{x}]}'("kL+([j:2 3]v:5 6)";"kL+([k:2 3]w:5 6)";"kL+(([]k:2 3)!5 6)";"(([]k:2 3)!5 6)*kL";"([k:1 2]v:1 2)|([k:3 4]v:`a`b)");("'domain";"'domain";"'type";"'type";"'type")]

/ ---- a list holding :: is data, not a train, so applying it indexes it, as in ngn/k (#83 Q2); a list of functions is still a train ----
t[`trainNullIndexed;`k'((::;::)@0 1;(::;::;-:)@0 1;(::;-:)@0 1;(-:;::)@1 0;(::;::)[0 1]);("(::;::)";"(::;::)";"(::;-:)";"(::;-:)";"(::;::)")]
t[`trainNullFromIndex;{D:({x};{y};1);`k'(D@5 6;(D@5 6)@0 1)}[];2#,"(::;::)"]
t[`trainNullAtom;(`k(::;::)@5;(::;::)[1;2];*"\n"\.[.:;,"1 (::;::) 2";{x}]);("::";2;"'type")]
t[`trainOfFunctions;((+;-)/[1;2];(+;-;*)[5;2];(-:;%:)@4);(0;-3;2.0)]


/ ---- ?[c;a;b] with c of 0s and 1s: an a or b that is a list of another length is 'length, as q, not a splice (#83 Q3) ----
t[`vcondLength;{*"\n"\.[{`k@. x};,x;{x}]}'("?[1 0 1;1 2;9]";"?[1 0 1;9;1 2]";"?[1 0 1;(1;2);9]";"?[0 1 1 0;1 2;5]";"?[!0;1 2;5]";"?[1 0 1;\"ab\";\"xyz\"]");6#,"'length"]
tv[`vcondSpliceOther;"?[0 1 2;1 2;9]";0 9 2]


/ ---- JSON writes a monadic verb as its text, as q and as a dyadic verb (#83 Q4); it wrote true, which reads back as 1 ----
t[`jsonMonadText;(`j@(-:);`j@(#:);`j@(-:;1);`j@`a`b!(-:;+);`j@(::;*:));("\"-:\"";"\"#:\"";"[\"-:\",1]";"{\"a\":\"-:\",\"b\":\"+\"}";"[null,\"*:\"]")]
t[`jsonMonadBack;`j?`j@(-:);"-:"]


/ ---- an empty item in parentheses is ::, as in ngn/k (#83 Q6); it was 'parse. ( ) is (), as () is, and an empty argument in [] is still elided ----
t[`parenEmptyItem;{*"\n"\.[{`k@. x};,x;{x}]}'("(1;;2)";"(;)";"(1;)";"(;1)";"(;;)";"(1;(;))";"{(;x)}3";"(1;\n2)";"( )");("(1;::;2)";"(::;::)";"(1;::)";"(::;1)";"(::;::;::)";"(1;(::;::))";"(::;3)";"(1;::;2)";"()")]
t[`parenEmptyKept;{*"\n"\.[{`k@. x};,x;{x}]}'("()";"(1;2)";"{x+y}[1;]2";"[1;;2]";"(1;2";"(1;;2");("()";"1 2";,"3";,"2";"'parse";"'parse")]

/ ---- the allocator's large blocks (64 KB up; src/m.c colours them): vectors the parsers grow an item at a time
/ from 32 KB to 1 MB keep their contents; coloured blocks built in peach workers and freed on the main thread, and a
/ coloured list kept live, leave \m clean
t[`allocGrowFloats;{(#x;+/x;x~0.0+!131072)}`j?"[",(","/$!131072),"]";(131072;8589869056.0;1b)]
t[`allocGrowChars;{(#x;+/`i$x;-12#x)}@,/$!200000;(1088890;56866720;"199998199999")]
t[`allocPeachFree;`x(("/bin/sh";"-c";"AMBER_THREADS=4 exec \"$0\"";`argv 0);"p:`pe({0.5*!x};40000*1+!8);s:+/'p;q:`pe({,/$!x};20000*1+!8);c:#'q;b:20000 2#!40000;p:0;q:0\n\\m\n`0:\" \"/$s,c,+/+/b\n");"399990000.0 1599980000.0 3599970000.0 6399960000.0 9999950000.0 14399940000.0 19599930000.0 25599920000.0 88890 188890 288890 388890 488890 608890 728890 848890 799980000\n"]   / in a process of its own, as \m reports the running script's own text

/ ---- leaks: \m (the heap check) prints a !refc line for each object counted more than it is held; it reads stdin, so each runs in a process of its own ----
LK:{`x(("/bin/sh";"-c";"exec \"$0\"";`argv 0);x,"\n\\m\n")}
t[`leakTableLiteral;LK"([]a:1 2 3);\n.[.:;,\"([a:1 2]b:1 2;c:1 2\";{x}];\n.[.:;,\"([]a:1 2;3)\";{x}];";""]   / an unkeyed literal's empty key group, and the parts parsed before a 'parse
t[`leakGroupFloat;LK"x:1.0 2.0 1.0;=x;\n=1.0 2.0;\n=`a`b!1.0 2.0;";""]   / group of floats held its argument once too many
t[`leakDistinctFloat;LK"?0.5*!5000;\nx:0.5*!5000;?x;";""]   / distinct of a float vector past a page held its argument once too many
t[`leakSortedDictLookup;LK"(`s#`a`c!1 2)`b;\n(`s#`a`c!1 2)`b`d;\n(`s#\"ac\"!1 2)\"b\";";""]   / a `s dict's symbol or char keys were held once too many by a lookup

hexpect[920]
hreport"AMBER CORE"
$[FAILN;`exit 1;]

