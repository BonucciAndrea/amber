/ ============================================================================
/ tests/test_sort_window.k  -  batch 2: radix grade + O(n) moving windows.
/ GNU AGPLv3 - see LICENSE and NOTICE.
/ ----------------------------------------------------------------------------
/ Run it from anywhere:
/     ./amber tests/test_sort_window.k
/ Exits non-zero if any case fails.
/ ----------------------------------------------------------------------------
/ Every case is DIFFERENTIAL: the new C kernels are checked against the exact
/ code they replaced, not against hand-written expectations.
/   * a single-vector grade is checked against  <,'x , which enlists each
/     element and so forces `<` down the generic boxed-row comparison sort
/     (ascA/qA in src/o.c) -- the pre-batch-2 collation, including where 0n and
/     0w land among the finite floats.
/   * a multi-column grade is checked against  <+cols , which IS the expression
/     xasc used before `xs existed.
/   * every moving window is checked against the K definition it replaced,
/     still present in std.k as msumK/mavgK/mvarK/mminK/mmaxK.
/ ----------------------------------------------------------------------------
/ Style rules from tests/harness.k apply: "f[a;b]" never "f [a;b]", every
/ each-block ends with ";" so the run does not print its own result list, and
/ no line is ever just "/" (that opens a block comment and swallows the file).
/ ============================================================================
DIR:{$[#p:"/"/-1_"/"\x;p;"."]}`argv 1
LOAD:{{. x^(`c$13)}'0:x;}
LOAD DIR,"/../amber.k"
LOAD DIR,"/../fin.k"
LOAD DIR,"/../std.k"
LOAD DIR,"/harness.k"

/ ---- reference grades ------------------------------------------------------
refv:{<,'x}                               / boxed-row comparison sort (old path)
refd:{>,'x}                               / ... descending
refm:{<+x}                                / old xasc grade over a column list

/ ---- generators ------------------------------------------------------------
gpos:{$[x;x?1000;0#0]}                    / small non-negative longs (many ties)
gneg:{$[x;(x?2000)-1000;0#0]}             / longs spanning zero
gbig:{$[x;(x?1000000000000000)-500000000000000;0#0]}   / wide 64-bit range
gint:{$[x;`i$(x?2000)-1000;0#0]}          / 32-bit ints
gflt:{$[x;(x?2000.0)-1000.0;0#0.0]}       / doubles spanning zero
gchr:{$[x;`c$97+x?26;""]}                 / chars
gsym:{$[x;`$'{"s",x}'$x?40;0#`]}          / symbols (name collation matters)
sizes:0 1 2 3 17 64 255 256 257 1000 5000
wins:1 2 3 5 17 64 999 1999

/ ============================ 1. single-vector grade ========================
{[n]
 t[`$"gradePos.",$n;  <v; refv v:gpos n];
 t[`$"gradeNeg.",$n;  <v; refv v:gneg n];
 t[`$"gradeBig.",$n;  <v; refv v:gbig n];
 t[`$"gradeInt.",$n;  <v; refv v:gint n];
 t[`$"gradeFlt.",$n;  <v; refv v:gflt n];
 t[`$"gradeChr.",$n;  <v; refv v:gchr n];
 t[`$"dscNeg.",$n;    >v; refd v:gneg n];
 t[`$"dscFlt.",$n;    >v; refd v:gflt n];
 }'sizes;

/ ---- floats: the special values have to collate exactly as they used to ----
fspec:0n,0w,(-0w),0.0,1.0,-1.0,2.5,-2.5,1e308,-1e308
t[`gradeSpecial;    <fspec;    refv fspec]
t[`gradeSpecialRep; <10#fspec; refv 10#fspec]
nmix:@[gflt 500;&0.1>?500;:;0n]
t[`gradeNullMix;    <nmix;     refv nmix]
srtd:nmix@<nmix
t[`sortIdempotent;  <srtd; !#srtd]

/ ---- already-sorted input takes the identity fast path; it must BE identity -
t[`gradeSortedAsc;  <1 2 3 4 5;      !5]
t[`gradeSortedTies; <1 1 2 2 3;      !5]
t[`gradeSortedFlt;  <0.0+!1000;      !1000]
t[`gradeSortedNeg;  <(-500)+!1000;   !1000]

/ ============================ 2. multi-column grade =========================
mk1:{([]a:gsym x;b:gpos x;c:gflt x)}
mk2:{([]a:gpos x;b:gneg x)}
mk3:{([]a:gchr x;b:gsym x;c:gint x;d:gflt x)}
/ `~` compares type/length/payload and ignores the `s attribute xasc sets, so
/ the native result and the old `<+cols` result compare directly as tables.
xchk:{[tag;t;c]hpush[tag;(xasc[c;t])~t@refm value c#+t;"FAIL xasc disagrees with <+cols"];}
ychk:{[tag;t;c]hpush[tag;(xdesc[c;t])~t@>+(+t)c;"FAIL xdesc disagrees with >+cols"];}
{[n]
 t1:mk1 n; t2:mk2 n; t3:mk3 n;
 xchk[`$"xascSymLong.",$n;    t1; `a`b];
 xchk[`$"xascSymLongFlt.",$n; t1; `a`b`c];
 xchk[`$"xascSingle.",$n;     t1; ,`b];
 xchk[`$"xascFloatKey.",$n;   t1; `c`a];
 xchk[`$"xascTwoLong.",$n;    t2; `a`b];
 xchk[`$"xascFour.",$n;       t3; `a`b`c`d];
 ychk[`$"xdescLong.",$n;      t2; `a`b];
 ychk[`$"xdescSym.",$n;       t1; `a`b];
 ychk[`$"xdescFlt.",$n;       t1; ,`c];
 }'sizes;

/ ---- the `s attribute survives, sorting is idempotent, aj still agrees ------
t[`xascAttr;         `at(xasc[`a`b;mk1 100])`a; `s]
sx:xasc[`a`b;mk1 500]
t[`xascSortedIdem;   xasc[`a`b;sx];             sx]
t[`ajsAfterNativeXasc; ajsorted[`a`b;xasc[`a`b;mk1 400]]; 1]

/ ---- types `xs does not handle must fall back, not misbehave ---------------
ft:([]b:0=2!!5;p:5 1 4 2 3;s:`e`a`d`b`c;n:(1 2;,3;();4 5 6;,7))
cn:,`n
cb:,`b
t[`xascNestedFallback; xasc[cn;ft]; ft@<+(+ft)cn]
t[`xascBoolFallback;   xasc[cb;ft]; ft@<+(+ft)cb]
t[`xdescNestedFallback;xdesc[cn;ft];ft@>+(+ft)cn]
t[`xascKeyedUnkeyed;   (xasc[,`v;unkey([k:3 1 2]v:`c`a`b)])`v; `a`b`c]
t[`xascEmptyTable;     #xasc[,`p;([]p:0#0;q:0#`)]; 0]
t[`xascOneRow;         (xasc[,`p;([]p:,5;q:,`a)])`p; ,5]
/ a key column that is entirely constant must leave the rows in original order
t[`xascStableConst;    (xasc[,`p;([]p:5 5 5 5;q:`d`c`b`a)])`q; `d`c`b`a]

/ ============================ 3. moving windows =============================
wchk:{[tag;f;g;w;v]hpush[tag;(f[w;v])~g[w;v];"FAIL window kernel disagrees with its K reference"];}
/ The sum family over FLOATS is compared with a tolerance, not with `~`: a
/ running difference and a prefix-sum difference are both exact only in exact
/ arithmetic, and they round differently. (The kernel is the more accurate of
/ the two -- see msumDrift below -- so exact agreement would be the wrong
/ thing to demand.) Integer-valued input is compared exactly, by msumL.
wtol:{[tag;f;g;w;v]hpush[tag;1e-6>|/|(f[w;v])-g[w;v];"FAIL window kernel drifts from its K reference"];}
{[w]
 vf:gflt 2000; vl:gneg 2000; vi:gint 2000;
 wtol[`$"msum.",$w;  msum; msumK; w; vf];
 wtol[`$"mavg.",$w;  mavg; mavgK; w; vf];
 wchk[`$"mmin.",$w;  mmin; mminK; w; vf];
 wchk[`$"mmax.",$w;  mmax; mmaxK; w; vf];
 wchk[`$"msumL.",$w; msum; msumK; w; vl];
 wchk[`$"mminL.",$w; mmin; mminK; w; vl];
 wchk[`$"mmaxL.",$w; mmax; mmaxK; w; vl];
 wchk[`$"mminI.",$w; mmin; mminK; w; vi];
 wchk[`$"mmaxI.",$w; mmax; mmaxK; w; vi];
 }'wins;

/ ---- the literal expectations from test-ext.k, unchanged -------------------
t[`msumLit; msum[3;1 2 3 4 5];  1 3 6 9 12.0]
t[`mavgLit; mavg[3;1 2 3 4 5];  1 1.5 2 3 4.0]
t[`mminLit; mmin[3;5 3 4 1 2];  5 3 3 1 1]
t[`mmaxLit; mmax[3;1 3 2 5 4];  1 3 3 5 5]
t[`mdevLit; mdev[2;2 2 2 2];    0 0 0 0.0]

/ ---- window edge cases -----------------------------------------------------
t[`msumW1;    msum[1;1 2 3 4.0];  1 2 3 4.0]
t[`mminW1;    mmin[1;5 3 4 1 2];  5 3 4 1 2]
t[`msumWide;  msum[1000;1 2 3.0]; 1 3 6.0]
t[`mmaxWide;  mmax[1000;1 3 2];   1 3 3]
/ (K has no <= operator: "no element is negative" is written |/x<0 ~ 0b.)
t[`mdevNonNeg; |/(mdev[7;gflt 500])<0; 0b]
t[`mvarNonNeg; |/(mvar[7;gflt 500])<0; 0b]
t[`msumEmpty; #msum[3;0#0.0]; 0]
t[`mminEmpty; #mmin[3;0#0];   0]

/ ---- nulls are ABSENT, not contagious: 0n must not poison the whole tail ----
t[`msumNull; msum[2;1 0n 3 4.0]; 1 1 3 7.0]
t[`mavgNull; mavg[2;1 0n 3 4.0]; 1 1 3 3.5]
t[`mminNull; mmin[2;1 0n 3 4.0]; 1 1 3 3.0]
t[`mmaxNull; mmax[2;1 0n 3 4.0]; 1 1 3 4.0]
t[`mAllNull; ^*mavg[2;0n 0n];    1b]

/ ---- drift: the running difference must stay tight to the prefix-sum answer -
dv:1000000+gflt 200000
t[`msumDrift; 1e-3>|/|(msum[50;dv])-msumK[50;dv]; 1b]

hexpect[296]
hreport"AMBER SORT / WINDOW DIFFERENTIAL"
$[FAILN;`exit 1;]
