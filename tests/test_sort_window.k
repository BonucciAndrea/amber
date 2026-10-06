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
t[`msumLit; msum[3;1 2 3 4 5];  1 3 6 9 12]
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
/ (mmin and mmax take 0n as the smallest value, as & and | do: q's answers)
t[`msumNull; msum[2;1 0n 3 4.0]; 1 1 3 7.0]
t[`mavgNull; mavg[2;1 0n 3 4.0]; 1 1 3 3.5]
t[`mminNull; mmin[2;1 0n 3 4.0]; 1 0n 0n 3.0]
t[`mmaxNull; mmax[2;1 0n 3 4.0]; 1 1 3 4.0]
t[`mAllNull; ^*mavg[2;0n 0n];    1b]

/ ---- drift: the running difference must stay tight to the prefix-sum answer -
dv:1000000+gflt 200000
t[`msumDrift; 1e-3>|/|(msum[50;dv])-msumK[50;dv]; 1b]

/ ---- amber 2.2: distinct on a FLOAT column, across all three kernels --------
/ ?x on floats picks a table by the value range: a byte LUT up to 64K, a bitmap
/ up to 2^26, else an open-addressed hash.  The hash arm used to be sized to 2n
/ slots before the first probe -- 16*2n bytes, 320 MB for a 10M column -- even
/ when the column held a handful of distinct values; it now grows with the
/ distinct COUNT, as the integer arm already did.  These pin the ANSWER (first-
/ appearance order, which is `?`'s contract) on each arm, including the sparse
/ one that exercises the growth and its rehash.
/ firstapp: distinct by definition -- keep an element when this is the FIRST
/ index at which it occurs.  Independent of every kernel under test.
firstapp:{x@&(x?x)=!#x}
dbase:gint 60000
dsmall:`f$dbase                             / span 2000       -> byte LUT
dbm:`f$1000*dbase                           / span 2e6        -> bitmap
dwide:`f$1000000*dbase                      / span 2e9        -> hash, with growth
dfrac:0.5+`f$gint 5000                      / non-integral    -> generic path
t[`distFloatLUT;    ?dsmall;               firstapp dsmall]
t[`distFloatBitmap; ?dbm;                  firstapp dbm]
t[`distFloatHash;   ?dwide;                firstapp dwide]
t[`distFloatCount;  #?dwide;               #?dbase]
t[`distFloatStable; (?dwide)~?dwide;       1b]
t[`distFloatFrac;   ?dfrac;                firstapp dfrac]
/ amber 2.2: -0.0 and 0.0 are ONE value for `?` and for find, as they already
/ were for `=`, `~`, `in` and `=` (group), and as they are in q. This case used
/ to expect 3. The old answer was not merely a different convention, it was not
/ a function of the input at all: `?` reached find for a short vector and `~':`
/ for a long one, so the same values gave 2 distinct at length 2 and 1 at
/ length 600. The length sweep below is the real regression test.
t[`distFloatNegZero;  #?(0.0;-0.0;1.0;0.0); 2]
t[`distNZfirst;       ?(0.0;-0.0);          ,0.0]      / the FIRST spelling is kept
t[`distNZfirstRev;    ?(-0.0;0.0);          ,-0.0]
/ the same values at every length that changes the path taken
t[`distNZlen2;        #?2#-0.0 0.0;         1]
t[`distNZlen600;      #?600#-0.0 0.0;       1]
t[`distNZlen4000;     #?4000#-0.0 0.0;      1]
/ ... and with a non-integral value present, so the integer-keyed path declines
t[`distNZfrac3;       #?(-0.0;0.0;1.5);     2]
t[`distNZfrac4002;    #?4002#-0.0 0.0 1.5;  2]
/ find agrees, whichever spelling is stored and whichever is looked up
t[`findNZa;           (-0.0 0.0)?0.0;       0]
t[`findNZb;           (-0.0 0.0)?-0.0;      0]
t[`findNZc;           (0.0 -0.0)?-0.0;      0]
/ NaN still matches by bit pattern, which is q's answer too
t[`findNaN;           (0n;1.0)?0n;          0]
t[`distNaN;           #?(0n;0n;1.0);        2]
/ and the collation is UNCHANGED: -0.0 still sorts before 0.0 (a tie-break now,
/ since they compare equal -- q's asc is stable here instead, see MISSING.md)
t[`ascNZ;             asc(0.0;-0.0);        -0.0 0.0]
t[`groupNZ;           #=(0.0;-0.0);         1]

/ the int32 minimum stored in a 32-bit column is an ordinary int (Amber has no int32 null)
t[`msumI32Min;  msum[2;-2147483648 70000 1];  -2147483648 -2147413648 70001]
t[`mavgI32Min;  mavg[2;-2147483648 70000 1];  (msum[2;-2147483648 70000 1])%1 2 2.0]

/ q's answers (digest #44): msum of ints is ints, mcount counts the non-nulls,
/ and a window longer than the list, an empty list or a range is no error
t[`msumInt;     msum[2;1 2 3];         1 3 5]
t[`msumIntNull; msum[2;1 0N 2];        1 1 2]
t[`msumLong;    msum[2;100000 2 3];    100000 100002 5]
t[`msumRange;   msum[2;!5];            0 1 3 5 7]
t[`msumEmptyR;  msum[1;!0];            !0]
t[`msumShort;   msum[10;1 2 3];        1 3 6]
t[`mavgShort;   mavg[3;1 2];           1 1.5]
t[`mavgRange;   mavg[3;1000+!3];       1000 1000.5 1001]
t[`mavgEmptyR;  mavg[1;!0];            0#0.0]
t[`mdevShort;   mdev[3;1 2];           0 0.5]
t[`mcountNull;  mcount[2;1 0n 2];      1 1 1]
t[`mcountNullL; mcount[2;1 0N 2 3];    1 1 1 2]
t[`mcountShort; mcount[10;1 2 3];      1 2 3]
t[`mcountEmpty; mcount[1;!0];          !0]
t[`mcountSym;   mcount[2;`a``b];       1 1 1]
/ a window of 0 or less is empty, as q's prefix difference gives: no sum,
/ no count, 0n for the average and deviation
t[`msumW0;      msum[0;1 2 3];         0 0 0]
t[`msumWNeg;    msum[-1;1 2 3 4 5];    0 0 0 0 0]
t[`msumWNegF;   msum[-2;1 2 3.0];      0 0 0.0]
t[`mavgW0;      mavg[0;1 2 3];         0n 0n 0n]
t[`mavgWNeg;    mavg[-1;1 2 3 4 5];    5#0n]
t[`mcountW0;    mcount[0;1 2 3];       0 0 0]
t[`mcountWNeg;  mcount[-1;1 2 3 4 5];  0 0 0 0 0]
t[`mdevW0;      mdev[0;1 2 3];         0n 0n 0n]
t[`mdevWNeg;    mdev[-1;1 2 3 4 5];    5#0n]
t[`mdevWNeg1;   mdev[-1;,7];           ,0n]
/ a dict is taken by its values, a table by its columns (q)
t[`msumDict;    msum[2;`a`b`c!1 2 3];  `a`b`c!1 3 5]
t[`mavgDict;    mavg[2;`a`b`c!1 2 3];  `a`b`c!1 1.5 2.5]
t[`mcountDict;  mcount[2;`a`b`c!1 2 3];`a`b`c!1 2 2]
t[`mdevDict;    mdev[2;`a`b`c!1 2 3];  `a`b`c!0 0.5 0.5]
t[`msumTable;   msum[2;+`a`b!(1 2 3;4 5 6.0)];  +`a`b!(1 3 5;4 9 11.0)]
/ with nulls: a table goes by its columns, a dict or keyed table by its
/ values, through the kernel (nulls absent), as q
t[`msumTabN;    msum[2;+`a`b!(1 0N 3;4 0n 6.0)];    +`a`b!(1 1 3;4 4 6.0)]
t[`mavgTabN;    mavg[2;+`a`b!(1 0N 3;4 0n 6.0)];    +`a`b!(1 1 3.0;4 4 6.0)]
t[`mcountTabN;  mcount[2;+`a`b!(1 0N 3;4 0n 6.0)];  +`a`b!(1 1 1;1 1 1)]
t[`mdevTabN;    mdev[2;+`a`b!(1 0N 3;4 0n 6.0)];    +`a`b!(0 0 0.0;0 0 0.0)]
t[`msumDictN;   msum[2;`a`b`c!1 0N 3];              `a`b`c!1 1 3]
t[`mavgDictN;   mavg[2;`a`b`c!1 0N 3];              `a`b`c!1 1 3.0]
t[`mcountDictN; mcount[2;`a`b`c!1 0N 3];            `a`b`c!1 1 1]
t[`mdevDictN;   mdev[2;`a`b`c!1 0N 3];              `a`b`c!0 0 0.0]
t[`msumKeyN;    msum[2;(+(,`k)!,1 2 3)!+`a`b!(1 0N 3;4 0n 6.0)];   (+(,`k)!,1 2 3)!+`a`b!(1 1 3;4 4 6.0)]
t[`mavgKeyN;    mavg[2;(+(,`k)!,1 2 3)!+`a`b!(1 0N 3;4 0n 6.0)];   (+(,`k)!,1 2 3)!+`a`b!(1 1 3.0;4 4 6.0)]
t[`mcountKeyN;  mcount[2;(+(,`k)!,1 2 3)!+`a`b!(1 0N 3;4 0n 6.0)]; (+(,`k)!,1 2 3)!+`a`b!(1 1 1;1 1 1)]
t[`mdevKeyN;    mdev[2;(+(,`k)!,1 2 3)!+`a`b!(1 0N 3;4 0n 6.0)];   (+(,`k)!,1 2 3)!+`a`b!(0 0 0.0;0 0 0.0)]
t[`msumDictE;   msum[2;(0#`)!0#0];                  (0#`)!0#0]
/ a general list (and a dict of mixed values): nulls add 0, as in a vector
/ (q keeps 0N there for msum, 1 1 3f for mavg and 0 0 0f for mdev)
t[`msumGenN;    msum[2;(1;0N;3.0)];                 (1;1;3.0)]
t[`mavgGenN;    mavg[2;(1;0n;3)];                   1 1 3.0]
t[`mdevGenN;    mdev[2;(1;0N;3.0)];                 0 0 0.0]
t[`mavgDictMix; mavg[2;`a`b`c!(1;0N;3.0)];          `a`b`c!1 1 3.0]
/ a bool list (bytes in Amber) sums to ints, as q's does
t[`msumBool;    msum[2;1011b];         1 1 1 2]
/ a window past 2^32 is a window past the list (it was cut to 32 bits on wasm32)
t[`msumHugeW;   msum[4294967297;1 2 3];   1 3 6]
t[`mcountHugeW; mcount[4294967296;1 2 3]; 1 2 3]
t[`mminHugeW;   mmin[4294967296;3 1 2];   3 1 1]
/ mmin is null when its window holds a null, mmax when the window is all
/ null, and -0W (-0w) then in the first w-1 points, as q's (x-1)|':/y gives
t[`mmaxNullL;   mmax[3;0N 0N 0N 0N 1];      (1+0N;1+0N;0N;0N;1)]
t[`mmaxNullF;   mmax[3;0n 0n 0n 0n 1.0];    -0w -0w 0n 0n 1.0]
t[`mmaxNullMix; mmax[2;0n 5 0n 0n 3.0];     -0w 5 5 0n 3.0]
t[`mminNullF;   mmin[2;0n 1 0n 3 4.0];      0n 0n 0n 0n 3.0]
t[`mminNullL;   mmin[3;3 1 0N 4 5 6 7];     3 1 0N 0N 0N 4 5]
t[`mminNullF3;  mmin[3;0n 0n 1.0];          0n 0n 0n]
/ a window of 0 or less, or an atom, gives x back, as q's (x-1)&':/y does;
/ a dict is taken by its values
t[`mminW0;      mmin[0;1 2];                1 2]
t[`mmaxW0;      mmax[0;1 2.0];              1 2.0]
t[`mminWNeg;    mmin[-1;1 2 3 4 5];         1 2 3 4 5]
t[`mmaxWNeg;    mmax[-2;3 1 2.0];           3 1 2.0]
t[`mminDict;    mmin[2;`a`b`c!3 1 2];       `a`b`c!3 1 1]
t[`mmaxDict;    mmax[2;`a`b`c!3 1 2];       `a`b`c!3 3 2]
t[`mminAtom;    mmin[2;5];                  5]
t[`mmaxAtom;    mmax[0;5.0];                5.0]
t[`mmaxHugeW;   mmax[4294967296;0N 0N 1];   (1+0N;1+0N;1)]
/ a dict, table or keyed table goes through the kernel too: nulls, and the
/ -0W (-0w) of an all-null window in the first w-1 points, as q
t[`mminTabN;    mmin[2;+`a`b!(1 0N 3;4 0n 6.0)];    +`a`b!(1 0N 0N;4 0n 0n)]
t[`mmaxTabN;    mmax[2;+`a`b!(0N 0N 3;0n 0n 6.0)];  +`a`b!((1+0N;0N;3);-0w 0n 6.0)]
t[`mminDictN;   mmin[2;`a`b`c!1 0N 3];              `a`b`c!1 0N 0N]
t[`mmaxDictN;   mmax[2;`a`b`c!0N 0N 1];             `a`b`c!(1+0N;0N;1)]
t[`mmaxDictNF;  mmax[2;`a`b`c!0n 0n 1.0];           `a`b`c!-0w 0n 1.0]
t[`mminKeyN;    mmin[2;(+(,`k)!,1 2 3)!+`a`b!(1 0N 3;4 0n 6.0)];   (+(,`k)!,1 2 3)!+`a`b!(1 0N 0N;4 0n 0n)]
t[`mmaxKeyN;    mmax[2;(+(,`k)!,1 2 3)!+`a`b!(0N 0N 3;0n 0n 6.0)]; (+(,`k)!,1 2 3)!+`a`b!((1+0N;0N;3);-0w 0n 6.0)]
t[`mminDictE;   mmin[2;(0#`)!0#0];                  (0#`)!0#0]
t[`mmaxDictE;   mmax[2;(0#`)!0#0.0];                (0#`)!0#0.0]

hexpect[394]
hreport"AMBER SORT / WINDOW DIFFERENTIAL"
$[FAILN;`exit 1;]
