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

hexpect[14]
hreport"AMBER CORE"
$[FAILN;`exit 1;]
