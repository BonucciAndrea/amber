/ ============================================================================
/ tests/test_matrix.k  -  combinatorial primitive x type x size matrix.
/ GNU AGPLv3 - see LICENSE and NOTICE.
/ ----------------------------------------------------------------------------
/ Run it from anywhere:
/     ./amber tests/test_matrix.k
/     /path/to/amber /path/to/amber/tests/test_matrix.k
/ Exits non-zero if any case fails.
/ ----------------------------------------------------------------------------
/ Rather than hard-coding thousands of literal expectations (which rot), every
/ case asserts an INVARIANT that must hold regardless of the concrete data:
/ shape/type preservation, algebraic identities, agreement between the
/ vectorised C kernel and a scalar K reference, and stability of the result
/ across the sizes that switch Amber between its scalar, SIMD and
/ multi-threaded (PAR_THRESHOLD = 100000) code paths.
/ Amber has no long-typed infinity literal (0W), so empty-input identity
/ elements are taken from the primitives themselves rather than written out.
/ ----------------------------------------------------------------------------
/ Standalone cases use tv[tag;"expr";want], which evaluates the expression
/ INSIDE the trap. The plain t[tag;got;want] form evaluates `got` in the
/ caller, so one throwing expression would abort the whole file and print no
/ report at all -- which is exactly what a broken engine makes it do.
/ ============================================================================
DIR:{$[#p:"/"/-1_"/"\x;p;"."]}`argv 1
LOAD:{{. x^(`c$13)}'0:x;}
LOAD DIR,"/../amber.k"
LOAD DIR,"/harness.k"
/ ---- generators: one builder per type, parameterised by length -------------
gI:{$[x;1+!x;0#0]}                        / Long vector, all positive
gJ:{$[x;(!x)-_x%2;0#0]}                   / Long vector spanning negatives
gF:{$[x;0.5+0.0+!x;0#0.0]}                / Float vector
gB:{$[x;0=2!!x;0#0b]}                     / Boolean vector
gC:{$[x;`c$97+26!!x;""]}                  / Char vector
gS:{$[x;`$'{"s",x}'$!x;0#`]}              / Symbol vector
gN:{$[x;enlist'!x;()]}                    / Nested list
sizes:0 1 10 100000
small:0 1 10

/ ============================ 1. shape invariants ===========================
{[n]
 x:gI n; y:gJ n;
 t[`$"len+.",$n; #x+y; n];
 t[`$"len-.",$n; #x-y; n];
 t[`$"len*.",$n; #x*y; n];
 t[`$"len%.",$n; #x%y; n];
 t[`$"len&.",$n; #x&y; n];
 t[`$"len|.",$n; #x|y; n];
 t[`$"len<.",$n; #x<y; n];
 t[`$"len=.",$n; #x=y; n];
 t[`$"len,.",$n; #x,y; 2*n];
 t[`$"lenrev.",$n; #|x; n];
 t[`$"lenasc.",$n; #<x; n];
 }'sizes;

/ ---- the same shape invariants for every other element type ---------------
{[n]
 t[`$"lenF.",$n; #(gF n)+gF n; n];
 t[`$"lenB.",$n; #(gB n)|gB n; n];
 t[`$"lenC.",$n; #|gC n;       n];
 t[`$"lenS.",$n; #<gS n;       n];
 t[`$"lenN.",$n; #|gN n;       n];
 t[`$"catC.",$n; #(gC n),gC n; 2*n];
 t[`$"catS.",$n; #(gS n),gS n; 2*n];
 t[`$"catN.",$n; #(gN n),gN n; 2*n];
 t[`$"distS.",$n; #?(gS n),gS n; n];
 t[`$"distC.",$n; (?(gC n),gC n)~?gC n; 1b];
 }'small;

/ ============================ 2. algebraic identities =======================
{[n]
 x:gI n; y:gJ n; f:gF n;
 t[`$"addcomm.",$n;   x+y;      y+x];
 t[`$"mulcomm.",$n;   x*y;      y*x];
 t[`$"addassoc.",$n;  (x+y)+x;  x+y+x];
 t[`$"addid.",$n;     x+0;      x];
 t[`$"mulid.",$n;     x*1;      x];
 t[`$"subself.",$n;   x-x;      n#0];
 t[`$"negneg.",$n;    - -x;     x];
 t[`$"distrib.",$n;   x*y+x;    (x*y)+x*x];
 t[`$"fadd0.",$n;     f+0.0;    f];
 t[`$"revrev.",$n;    |(|x);    x];
 t[`$"minmaxabs.",$n; (x&y)|x;  x];
 t[`$"demorgan.",$n;  ~(x>0)|y>0; (~x>0)&~y>0];
 }'sizes;

/ ================== 3. vector kernel vs scalar K reference ==================
/ `each` over an empty vector yields the generic empty list (), not !0, so the
/ elementwise cross-checks start at n=1.
{[n]
 x:gI n; y:gJ n; f:gF n; g:gF n;
 t[`$"kadd.",$n;   x+y;     {x+y}'[x;y]];
 t[`$"kmul.",$n;   x*y;     {x*y}'[x;y]];
 t[`$"ksub.",$n;   x-y;     {x-y}'[x;y]];
 t[`$"kfad.",$n;   f+g;     {x+y}'[f;g]];
 t[`$"kfml.",$n;   f*g;     {x*y}'[f;g]];
 t[`$"klt.",$n;    x<y;     {x<y}'[x;y]];
 t[`$"ksum.",$n;   +/x;     {x+y}/[0;x]];
 t[`$"kmax.",$n;   |/x;     {x|y}/[*x;x]];
 t[`$"kmin.",$n;   &/x;     {x&y}/[*x;x]];
 t[`$"kscan.",$n;  sums x;  {x+y}\[0;x]];
 }'1 10;

/ ------- SIMD / multi-thread boundary: identical answers at every size ------
{[n]
 x:gI n; y:gJ n;
 t[`$"bigAddSum.",$n;  +/x+y;   (+/x)++/y];
 t[`$"bigMulComm.",$n; +/x*y;   +/y*x];
 t[`$"bigLen.",$n;     #x+y;    n];
 t[`$"bigMaxIsLast.",$n; |/x;   x@-1+#x];
 }'99999 100000 100001 250000;
tv[`sumBig;"+/1+!100000";5000050000];
tv[`sumParF;"0.5>abs(+/0.0+1+!200000)-20000100000.0";1b];
tv[`mulBig;"+/(1+!100000)*2";10000100000];

/ ============================ 4. empty / singleton ==========================
tv[`emptySum;"+/0#0";0];
tv[`emptyPrd;"*/0#0";1];
tv[`emptyCount;"#0#0";0];
tv[`emptyRev;"|0#0";0#0];
tv[`emptyAsc;"<0#0";0#0];
tv[`emptyDist;"?0#0";0#0];
tv[`emptyDeltas;"deltas 0#0";0#0];
tv[`emptyRatios;"ratios 0#0";0#0];
tv[`emptySums;"sums 0#0";0#0];
tv[`emptyAvg;"avg 0#0";0n];
tv[`emptyMed;"med 0#0";0n];
tv[`emptyWhere;"&0#0b";0#0];
tv[`emptyGrp;"#=0#0";0];
tv[`emptyCat;"(0#0),0#0";0#0];
tv[`emptyTake;"#0#0#0";0];
tv[`emptyIdx;"(0#0)@0 1";0N 0N];
tk[`emptyFlip;  "+(0#`)!0#0"];
tv[`atomVsOne;"(,1)~1";0b];
tv[`oneCount;"#,1";1];
tv[`oneSum;"+/,7";7];
tv[`oneRev;"|,1";,1];
tv[`oneAsc;"<,5";,0];
tv[`oneDist;"?,3";,3];
tv[`enlEnl;"#,,1";1];
tv[`oneVsAtom;"(+/,7)~+/7";1b];

/ ======================== 5. special / null values ==========================
/ NOTE: like kdb+, Amber does NOT propagate the LONG null through arithmetic --
/ 0N is LLONG_MIN and 0N+1 wraps to -0W. Float nulls (0n) DO propagate.
tv[`nullAddI;"0N+1";-9223372036854775807];
tv[`nullAddF;"0n+1.0";0n];
tv[`infAdd;"0w+1.0";0w];
tv[`infNeg;"(-0w)+1.0";-0w];
tv[`nanFromInf;"(-0w)+0w";0n];
tv[`divZeroF;"1%0";0w];
tv[`divNegZero;"-1%0";-0w];
tv[`zeroOverZero;"0%0";0n];
tv[`nullEqNull;"0N=0N";1b];
tv[`nanEqNan;"0n=0n";1b];
tv[`nullMatch;"0N~0N";1b];
tv[`nanNotNeg;"($0%0)~\"0n\"";1b];
tv[`idxOOB;"1 2 3@5";0N];
tv[`idxNeg;"1 2 3@-1";0N];
tv[`idxNull;"1 2 3@0N";0N];
tv[`findMiss;"1 2 3?9";0N];
tv[`findMissS;"`a`b?`z";0N];
tv[`fillNull;"0^1 0N 3";1 0 3];
tv[`nullNoPropSum;"($+/1 0N 3)~$0N+4";1b];
tv[`nullSort;"#<0N 1 0N 2";4];
tv[`nullDistinct;"#?0N 0N 1";2];

/ ==================== 6. sort / distinct / group / index ====================
{[n]
 x:(gJ n)@(-1+!n)|0;
 t[`$"sortIdem.",$n;       asc asc x;         asc x];
 t[`$"sortLen.",$n;        #asc x;            #x];
 t[`$"descIsRevAsc.",$n;   desc x;            |asc x];
 t[`$"gradeApply.",$n;     x@<x;              asc x];
 t[`$"sortStable.",$n;     asc x;             x@<x];
 t[`$"distinctIdem.",$n;   ??x;               ?x];
 t[`$"distinctSubset.",$n; &/1,in[?x;x];        1b];
 t[`$"grpKeys.",$n;        asc(?!=x);         asc(?x)];
 t[`$"grpTotal.",$n;       +/#'. =x;          #x];
 t[`$"idxIdentity.",$n;    x@!#x;             x];
 t[`$"whereBool.",$n;      #&x>0;             +/x>0];
 }'small;
/ attributes: Amber spells these `sa/`ua/`pa/`ga (setter) and `at (getter),
/ NOT kdb's `s#/`u#/`p#/`g# -- see README "Attributes".
tv[`attrSorted;"`at(`sa 1 2 3)";`s];
tv[`attrUnique;"`at(`ua 3 1 2)";`u];
tv[`attrParted;"`at(`pa 1 1 2 2)";`p];
tv[`attrGrouped;"`at(`ga 3 1 2)";`g];
tv[`attrNone;"`at 1 2 3";`];
tv[`attrValue;"`sa 1 2 3";1 2 3];
tv[`attrAscSets;"`at asc 3 1 2";`s];
tv[`attrBinFind;"(`sa 1 3 5 7 9)?7";3];
tv[`attrBinMiss;"(`sa 1 3 5 7 9)?4";0N];
tv[`sortBigIdem;"(asc asc 100000#1000 3 7 2)~asc 100000#1000 3 7 2";1b];
tv[`sortBigOrder;"&/~0>deltas asc(-1+!50000)|0";1b];
tv[`sortBigLen;"#asc 100000#7 1 9";100000];
tv[`distinctBig;"#?100000#1 2 3";3];
tv[`groupBig;"#=100000#1 2 3 4";4];

/ ======================== 7. take / drop / slice ============================
tv[`take3;"3#1 2 3 4 5";1 2 3];
tv[`takeNeg;"-3#1 2 3 4 5";3 4 5];
tv[`takeOver;"#5#1 2";5];
tv[`takeZero;"0#1 2 3";0#0];
tv[`drop2;"2_1 2 3 4";3 4];
tv[`dropNeg;"-2_1 2 3 4";1 2];
tv[`dropAll;"9_1 2 3";0#0];
tv[`dropNegAll;"-9_1 2 3";0#0];
tv[`sliceLen;"#2_5#!100";3];
tv[`takeDropId;"(3#1 2 3 4 5),3_1 2 3 4 5";1 2 3 4 5];
tv[`reshape;"2 3#!6";(0 1 2;3 4 5)];
tv[`reshapeLen;"#,/2 3#!6";6];

/ ==================== 8. type mismatches must ERROR, not crash ==============
te[`symPlusInt;   "`a`b+1 2"];
tk[`charPlusInt;  "\"ab\"+1 2"];   / Amber allows char arithmetic
te[`symMinus;     "-`a`b"];
te[`symSqrt;      "sqrt`a"];
te[`lenMismatch;  "1 2 3+1 2"];
te[`lenMismatch2; "(,1)+1 2 3"];
te[`badReshape;   "(-1 -1)#1 2 3"];
te[`symDivide;    "`a%2"];
te[`applyNonFn;   "1 2 3[`a]"];
tk[`symMatch;     "`a`b~`a`b"];
tk[`symSort;      "asc`c`a`b"];
tk[`symDistinct;  "?`a`a`b"];
tk[`symGroup;     "=`a`a`b"];
tk[`charSort;     "asc\"cab\""];
tk[`mixedList;    "(1;2.0;`a;\"c\")"];
tk[`charCat;      "\"ab\",\"cd\""];
tk[`symCat;       "`a,`b"];

/ ============================ 9. dictionaries ===============================
d:`a`b`c!1 2 3
tv[`dictKeys;"!d";`a`b`c];
tv[`dictVals;". d";1 2 3];
tv[`dictIdx;"d`b";2];
tv[`dictCount;"#d";3];
tv[`dictAdd;". d+1";2 3 4];
tv[`dictMissing;"d`zz";0N];
tv[`dictRoundtrip;"(!d)!. d";d];
tv[`emptyDictCount;"#(0#`)!0#0";0];
tk[`emptyDictKeys; "!(0#`)!0#0"];

/ ============================ 10. nested lists ==============================
nl:(1 2;3 4 5;0#0)
tv[`nestCount;"#nl";3];
tv[`nestLens;"#'nl";2 3 0];
tv[`nestRaze;",/nl";1 2 3 4 5];
tv[`nestFirst;"*nl";1 2];
tv[`nestRev;"|nl";(0#0;3 4 5;1 2)];
tv[`nestEachSum;"+/'nl";3 12 0];
tv[`nestDeep;"#,/,/((,1;,2);(,3;,4))";4];
tv[`nestRagged;"#,/(!3;!5)";8];

/ ======================== 11. compound pipelines ============================
p:1+!1000
tv[`pipeGrpAgg;"+/. {+/p x}'=3!p";+/p];
tv[`pipeSortTake;"+/5#desc p";4990];
tv[`pipeScanRed;"|/sums p";+/p];
tv[`pipeMaskId;"+/p@&p>500";(+/p)-+/p@&~p>500];
tv[`pipeChain;"+/(2*p@&p>500)%2";0.0++/p@&p>500];
tv[`pipeFloat;"0.001>abs(+/(0.5+0.0+!5000)%2.0)-0.5*+/0.5+0.0+!5000";1b];
tv[`pipeBig;"+/(1+!100000)@&0=2!1+!100000";2500050000];
tv[`pipeNested;"+/,/{!x}'1 2 3";4];

/ Guard against a silently skipped block (see tests/harness.k): this is the
/ exact number of assertions this file is expected to record. Bump it when
/ you add or remove cases -- if it ever disagrees, some of them stopped running.
hexpect[308]
hreport"AMBER COMBINATORIAL MATRIX"
$[FAILN;`exit 1;]
