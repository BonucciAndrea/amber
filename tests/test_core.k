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

hexpect[15]
hreport"AMBER CORE"
$[FAILN;`exit 1;]
