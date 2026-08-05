\l amber.k
\l tests/harness.k
/ ============================================================================
/ tests/test_matrix.k  -  combinatorial primitive x type x size matrix.
/ GNU AGPLv3 - see LICENSE and NOTICE.
/ ----------------------------------------------------------------------------
/ Rather than hard-coding thousands of literal expectations (which rot), every
/ case asserts an INVARIANT that must hold regardless of the concrete data:
/ shape/type preservation, algebraic identities, agreement between the
/ vectorised C kernel and a scalar K reference, and stability of the result
/ across the sizes that switch Amber between its scalar, SIMD and
/ multi-threaded (PAR_THRESHOLD = 100000) code paths.
/ Amber has no long-typed infinity literal (0W), so empty-input identity
/ elements are taken from the primitives themselves rather than written out.
/ ============================================================================

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
t[`sumBig;   +/1+!100000;      5000050000];
t[`sumParF;  0.5>abs(+/0.0+1+!200000)-20000100000.0; 1b];
t[`mulBig;   +/(1+!100000)*2;  10000100000];

/ ============================ 4. empty / singleton ==========================
t[`emptySum;    +/0#0;        0];
t[`emptyPrd;    */0#0;        1];
t[`emptyCount;  #0#0;         0];
t[`emptyRev;    |0#0;         0#0];
t[`emptyAsc;    <0#0;         0#0];
t[`emptyDist;   ?0#0;         0#0];
t[`emptyDeltas; deltas 0#0;   0#0];
t[`emptyRatios; ratios 0#0;   0#0];
t[`emptySums;   sums 0#0;     0#0];
t[`emptyAvg;    avg 0#0;      0n];
t[`emptyMed;    med 0#0;      0n];
t[`emptyWhere;  &0#0b;        0#0];
t[`emptyGrp;    #=0#0;        0];
t[`emptyCat;    (0#0),0#0;    0#0];
t[`emptyTake;   #0#0#0;       0];
t[`emptyIdx;    (0#0)@0 1;    0N 0N];
tk[`emptyFlip;  "+(0#`)!0#0"];
t[`atomVsOne;   (,1)~1;       0b];
t[`oneCount;    #,1;          1];
t[`oneSum;      +/,7;         7];
t[`oneRev;      |,1;          ,1];
t[`oneAsc;      <,5;          ,0];
t[`oneDist;     ?,3;          ,3];
t[`enlEnl;      #,,1;         1];
t[`oneVsAtom;   (+/,7)~+/7;   1b];

/ ======================== 5. special / null values ==========================
/ NOTE: like kdb+, Amber does NOT propagate the LONG null through arithmetic --
/ 0N is LLONG_MIN and 0N+1 wraps to -0W. Float nulls (0n) DO propagate.
t[`nullAddI;     0N+1;      -9223372036854775807];
t[`nullAddF;     0n+1.0;    0n];
t[`infAdd;       0w+1.0;    0w];
t[`infNeg;       (-0w)+1.0; -0w];
t[`nanFromInf;   (-0w)+0w;  0n];
t[`divZeroF;     1%0;       0w];
t[`divNegZero;   -1%0;      -0w];
t[`zeroOverZero; 0%0;       0n];
t[`nullEqNull;   0N=0N;     1b];
t[`nanEqNan;     0n=0n;     1b];
t[`nullMatch;    0N~0N;     1b];
t[`nanNotNeg;    ($0%0)~"0n"; 1b];
t[`idxOOB;       1 2 3@5;   0N];
t[`idxNeg;       1 2 3@-1;  0N];
t[`idxNull;      1 2 3@0N;  0N];
t[`findMiss;     1 2 3?9;   0N];
t[`findMissS;    `a`b?`z;   0N];
t[`fillNull;     0^1 0N 3;  1 0 3];
t[`nullNoPropSum; ($+/1 0N 3)~$0N+4; 1b];
t[`nullSort;     #<0N 1 0N 2; 4];
t[`nullDistinct; #?0N 0N 1; 2];

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
t[`attrSorted;   `at(`sa 1 2 3);   `s];
t[`attrUnique;   `at(`ua 3 1 2);   `u];
t[`attrParted;   `at(`pa 1 1 2 2); `p];
t[`attrGrouped;  `at(`ga 3 1 2);   `g];
t[`attrNone;     `at 1 2 3;      `];
t[`attrValue;    `sa 1 2 3;      1 2 3];
t[`attrAscSets;  `at asc 3 1 2;  `s];
t[`attrBinFind;  (`sa 1 3 5 7 9)?7; 3];
t[`attrBinMiss;  (`sa 1 3 5 7 9)?4; 0N];
t[`sortBigIdem;  (asc asc 100000#1000 3 7 2)~asc 100000#1000 3 7 2; 1b];
t[`sortBigOrder; &/~0>deltas asc(-1+!50000)|0;                      1b];
t[`sortBigLen;   #asc 100000#7 1 9;                                 100000];
t[`distinctBig;  #?100000#1 2 3;                                    3];
t[`groupBig;     #=100000#1 2 3 4;                                  4];

/ ======================== 7. take / drop / slice ============================
t[`take3;      3#1 2 3 4 5;   1 2 3];
t[`takeNeg;    -3#1 2 3 4 5;  3 4 5];
t[`takeOver;   #5#1 2;        5];
t[`takeZero;   0#1 2 3;       0#0];
t[`drop2;      2_1 2 3 4;     3 4];
t[`dropNeg;    -2_1 2 3 4;    1 2];
t[`dropAll;    9_1 2 3;       0#0];
t[`dropNegAll; -9_1 2 3;      0#0];
t[`sliceLen;   #2_5#!100;     3];
t[`takeDropId; (3#1 2 3 4 5),3_1 2 3 4 5; 1 2 3 4 5];
t[`reshape;    2 3#!6;        (0 1 2;3 4 5)];
t[`reshapeLen; #,/2 3#!6;     6];

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
t[`dictKeys;      !d;      `a`b`c];
t[`dictVals;      . d;     1 2 3];
t[`dictIdx;       d`b;     2];
t[`dictCount;     #d;      3];
t[`dictAdd;       . d+1;   2 3 4];
t[`dictMissing;   d`zz;    0N];
t[`dictRoundtrip; (!d)!. d; d];
t[`emptyDictCount; #(0#`)!0#0; 0];
tk[`emptyDictKeys; "!(0#`)!0#0"];

/ ============================ 10. nested lists ==============================
nl:(1 2;3 4 5;0#0)
t[`nestCount;   #nl;         3];
t[`nestLens;    #'nl;        2 3 0];
t[`nestRaze;    ,/nl;        1 2 3 4 5];
t[`nestFirst;   *nl;         1 2];
t[`nestRev;     |nl;         (0#0;3 4 5;1 2)];
t[`nestEachSum; +/'nl;       3 12 0];
t[`nestDeep;    #,/,/((,1;,2);(,3;,4)); 4];
t[`nestRagged;  #,/(!3;!5);  8];

/ ======================== 11. compound pipelines ============================
p:1+!1000
t[`pipeGrpAgg;   +/. {+/p x}'=3!p;                  +/p];
t[`pipeSortTake; +/5#desc p;                        4990];
t[`pipeScanRed;  |/sums p;                          +/p];
t[`pipeMaskId;   +/p@&p>500;                        (+/p)-+/p@&~p>500];
t[`pipeChain;    +/(2*p@&p>500)%2;                  0.0++/p@&p>500];
t[`pipeFloat;    0.001>abs(+/(0.5+0.0+!5000)%2.0)-0.5*+/0.5+0.0+!5000; 1b];
t[`pipeBig;      +/(1+!100000)@&0=2!1+!100000;      2500050000];
t[`pipeNested;   +/,/{!x}'1 2 3;                    4];

hreport"AMBER COMBINATORIAL MATRIX"
