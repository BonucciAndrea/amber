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

/ ---- adding a key to a dict whose values or keys are a range ----
/ Pushing onto a range (!n, a+!n) looped forever.
t[`pushRangeVals;{x[`c]:5;x}`a`b!!2;`a`b`c!0 1 5]
t[`pushRangeJoin;(`a`b!3+!2),`c!5;`a`b`c!3 4 5]
t[`pushRangeKeys;{x[1]:`z;x}(7+!3)!!3;7 8 9 1!(0;1;2;`z)]
t[`pushRangeLambda;@[(-!2)!!2;,1;{x,1}];0 -1 1!(0;1;0N 1)]

hexpect[6]
hreport"AMBER CORE"
$[FAILN;`exit 1;]
