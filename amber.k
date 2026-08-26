/ ============================================================================
/ Amber standard library  -  the q/kdb+ vocabulary of the Amber array language.
/ (c) 2026 the Amber authors.  GNU AGPLv3 (see LICENSE and NOTICE).
/ One definition per line (an Amber statement is a line).  Loaded at startup.
/ ============================================================================

/ ---- scalar / monadic primitives ------------------------------------------
neg:{-x}
not:{~x}
null:{^x}
reciprocal:{1%x}
sqrt:{%x}
floor:{_x}
ceiling:{-_-x}
signum:{(x>0)-x<0}
abs:{x*signum x}
exp:{`exp x}
log:{`ln x}
sin:{`sin x}
cos:{`cos x}
til:{!x}
enlist:{,x}
string:{$x}
type:{@x}
key:{!x}
value:{. x}
first:{*x}
last:{*|x}
reverse:{|x}
distinct:{?x}
group:{=x}
where:{&x}
flip:{+x}
count:{#x}
mod:{y!x}
div:{_x%y}
xbar:{x*_y%x}
xlog:{(`ln y)%`ln x}
round:{[d;x]p:*/d#10;(_0.5+x*p)%p}

/ ---- aggregations ----------------------------------------------------------
sum:{$[0=#x;$[`F=@x;0.0;0];+/x]}       / empty: keep the float type (+/ already does)
prd:{$[0=#x;$[`F=@x;1.0;1];*/x]}       / empty: keep the float type
min:{&/x}
max:{|/x}
avg:{(+/x)%#x}
wsum:{+/x*y}
wavg:{(+/x*y)%+/x}
var:{avg{x*x}x-avg x}
dev:{sqrt var x}
svar:{(n*var x)%-1+n:#x}
sdev:{sqrt svar x}
cov:{avg[x*y]-avg[x]*avg y}
scov:{(n*cov[x;y])%-1+n:#x}
cor:{cov[x;y]%dev[x]*dev y}
med:{$[0=#x;0n;[a:x@<x;n:#a;m:_n%2;$[2!n;0.0+a m;avg a@m-1 0]]]}
all:{~|/x=0}
any:{~&/x=0}
sums:{+\x}
prds:{*\x}
mins:{&\x}
maxs:{|\x}
deltas:{$[#x;x-0,-1_x;x]}
ratios:{$[#x;x%1,-1_x;x]}
differ:{~~':x}
prev:{-1_0N,x}
next:{1_x,0N}

/ ---- ranking / ordering ----------------------------------------------------
rank:{<<x}
iasc:{<x}
idesc:{>x}
asc:{`srt x}
desc:{x@>x}
xrank:{_y*x%#y:<<y}
xprev:{y[(!#y)-x]}
rotate:{$[0=#y;y;n:(#y)!x;(n_y),n#y]}

/ ---- membership / set ops --------------------------------------------------
in:{~^y?x}
/ within[x;(lo;hi)] -- inclusive range test, the q verb of the same name.  Used
/ by every time-range query (Grafana panels, amberd clients) and previously
/ missing, so those queries died on an undefined name.  Bracket form only: user
/ verbs are never infix here (see the REPL's \h note), so it is within[t;(a;b)],
/ not  t within (a;b).
within:{[x;r](~x<r[0])&~x>r[1]}
except:{x@&~in[x;y]}
inter:{x@&in[x;y]}
union:{?x,y}
raze:{,/x}
sublist:{n:(abs x)&#y;$[x<0;(0-n)#y;n#y]}
cross:{,/x,/:\:y}

/ ---- table predicates & basics ---------------------------------------------
istable:{`M=@x}
isdict:{`m=@x}
iskeyed:{$[`m=@x;`M=@!x;0]}
cols:{$[iskeyed x;(!!x),!.x;!x]}
keys:{$[iskeyed x;!!x;0#`]}
rows:{+. x}
atr:{[t;i]+(cols t)!{y@x}[i]'. t}
xkey:{[k;t]k:k,();d:+t;(+k#d)!+(except[cols t;k])#d}
unkey:{$[iskeyed x;+(+!x),+.x;x]}
xcolall:{[nm;t]+nm!. t}
xcols:{[c;t]c:c,();+(c,except[cols t;c])#+t}
/ xasc / xdesc: `xs (src/a.c) grades the table's key columns with a stable
/ multi-column LSD radix sort straight off the columns' raw C arrays -- no
/ per-row K objects, no boxed comparisons -- and returns () when it meets a
/ column type it does not handle, in which case the original `<+(+t)c` grade
/ below still runs.  Symbol columns are collated BY NAME (via a dense rank of
/ the DISTINCT ids), which is the collation `<` on a symbol vector gives and the
/ one ajsorted's contiguity note assumes.  Descending is the same sort with
/ complemented keys, which reproduces `>`'s "ties in ascending index order".
xasc:{[c;t]c:c,();o:`xs(value c#+t;0b);o:$[#o;o;<+(+t)c];t:t@o;d:+t;+d,(1#c)!`sa'd@1#c}
xdesc:{[c;t]c:c,();o:`xs(value c#+t;1b);o:$[#o;o;>+(+t)c];t@o}
meta:{d:+unkey x;xkey[`c;+`c`t`a!(!d;{@*x}'value d;{`at x}'value d)]}
insert:{[t;r]t,r}

/ ---- joins -----------------------------------------------------------------
lj:{[t;kt]yk:rows[!kt];xk:+. (keys kt)#+t;i:yk?xk;vc:cols value kt;+(+t),vc!{y@x}[i]'value value kt}
/ ij: the probe used to box every left row AND every right key row (rows[...])
/ purely to feed `?`.  Both sides are now rank-encoded column-wise -- jointly,
/ over one shared code space -- so the lookup is a native vector `?` on a flat
/ integer vector.  Single-key joins (the common case) skip the encode entirely
/ and probe the raw key column.
ij:{[t;kt]kc:keys kt;kd:+!kt;td:+t;yc:kd kc;xc:td kc;p:$[1=#kc;(*yc;*xc);qcard[yc,'xc]<qEXACT;[e:qenc yc,'xc;n:#*yc;(n#e;n _ e)];(rows[!kt];+. kc#td)];i:(p 0)?p 1;j:&~^i;+(+atr[t;j]),(cols value kt)!{y@x}[i@j]'value value kt}
gc:{[t;n;ot;c]$[in[c;cols t];(+t)c;n#0#(+ot)c]}
uj:{[x;y]c:distinct cols[x],cols y;+c!{[x;y;c]gc[x;#x;y;c],gc[y;#y;x;c]}[x;y]'c}
pj:{[t;kt]yk:rows[!kt];xk:+. (keys kt)#+t;i:(#yk)^yk?xk;vt:value kt;vc:cols vt;+(+t),vc!{[i;t;vt;c]((+t)c)+((vt c),0)@i}[i;t;vt]'vc}
ej:{[c;x;y]ij[x;xkey[c;y]]}

/ ---- asof join (aj/aj0) : y must contain matching + time cols -----------------
/ ajm: as-of match indices.  Native path: the C `aj kernel does a branch-free
/ lower_bound over each group's sorted ns-timestamp slice (src/a.c), eliminating
/ the per-row K bin.  wjbnd supplies the [base,end) group slice per trade row.
/ y must be xasc'd on c (aj does this) so each group's time column is ascending.
/ ---- as-of ordering predicate ------------------------------------------------
/ ajsorted[c;t] -> 1b if table `t` is ALREADY in the order aj/wj need, so the
/ xasc below can be skipped.  aj used to re-sort its right-hand table on every
/ call; on a 2M-row book that is ~1.1s EVEN WHEN THE BOOK IS ALREADY SORTED,
/ which is the normal case for anything coming out of a tick store or an on-disk
/ partition.  `ajs (src/a.c) answers the question in one O(n) pass.
/ It checks the two properties the join machinery actually relies on -- the time
/ column non-decreasing within each run of equal group keys, and each group key
/ confined to ONE contiguous run -- using equality only, so it is independent of
/ how xasc happens to collate symbols.  It errs towards 0b (sort it) for anything
/ it cannot prove, so a false 1b -- the only answer that could corrupt a join --
/ is not reachable.
ajsorted:{[c;t]c:c,();tc:*|c;g:-1_c;d:+t;gc:value g#d;g0:*gc;`ajs(gc;d tc;$[1=#gc;in[`at g0;`s`p];0b])}
/ ajord: xasc only when it will actually change something.  Marks the result `s
/ so the attribute survives for callers that inspect it (meta/`at).
ajord:{[c;t]$[ajsorted[c;t];t;xasc[c;t]]}
ajmK:{[c;x;y]c:c,();g:-1_c;tc:*|c;yt:(+y)tc;xt:(+x)tc;yg:$[#g;=rows[g#+y];(,0)!,!#y];xg:$[#g;=rows[g#+x];(,0)!,!#x];{[yt;xt;yg;xg;acc;gk]yi:yg gk;xi:xg gk;b:(yt yi)'(xt xi);@[acc;xi;:;(yi,0N)@b]}[yt;xt;yg;xg]/[(#x)#0N;!xg]}  / pure-K reference
ajm:{[c;x;y]c:c,();g:-1_c;tc:*|c;b:wjbnd[g;x;y];`aj((+y)tc;(+x)tc;b 0;b 1)}
aj:{[c;x;y]c:c,();y:ajord[c;y];m:ajm[c;x;y];vc:except[cols y;c];+(+x),vc!{[m;yd;c](yd c)@m}[m;+y]'vc}
aj0:{[c;x;y]c:c,();y:ajord[c;y];m:ajm[c;x;y];vc:except[cols y;-1_c];+(+x),vc!{[m;yd;c](yd c)@m}[m;+y]'vc}
asof:{[t;d]c:!d;g:-1_c;tc:*|c;td:+t;msk:$[#g;&/({[td;d;c]td[c]=d c}[td;d]'g);(#t)#1b];i:&msk&~td[tc]>d tc;$[#i;(except[cols t;c])#t[*|i];()]}

/ ---- window join wj : w=(begins;ends) aligned to t rows -----------------------
/ VECTORISED window lookup: quotes are xasc'd on c, so within each group the time
/ column is ascending -> the [w0,w1] slice is found with two bin (') searches
/ instead of an O(group) scan per row (same technique that made aj ~6x faster).
/ Result keeps the t-side `time` column, so it auto-renders HH:MM:SS.mmm.
/ window join.  w=(begins;ends) aligned to t rows; aggs=list of (name;fn;col).
/ Standard reducers (first last min max sum avg count) run in the C kernel `wj:
/ vectorised binary-search range probe + contiguous slice sweep.  Arbitrary agg
/ functions fall back to wjK (per-row K).  Result keeps the t-side `time column,
/ so it auto-renders HH:MM:SS.mmm.
wjcode:{$[x~first;0;x~last;1;x~min;2;x~max;3;x~sum;4;x~avg;5;x~count;6;x~{#x};6;-1]}
/ wjbndK: the original pure-K group-bounds path. Retained verbatim as the
/ fallback for anything `wjb declines (a quote table that is not contiguously
/ grouped, an unsupported key type, >16 key columns) so no input loses support.
wjbndK:{[g;t;q]gdi:=rows[g#+q];bas:*:'value gdi;len:#:'value gdi;ix:(!gdi)?rows[g#+t];(bas ix;(bas+len)ix)}
/ wjbnd: per-trade-row [base,end) slice into the grouped quote table.
/ The K path above builds one boxed row-tuple per quote row AND per trade row
/ (`rows` is `+.`) just to give `=` and `?` something to hash -- 3M throwaway
/ objects on a 1M x 2M join, measured at ~968ms. `wjb (src/a.c) walks the quote
/ side's run boundaries and probes once per trade row, allocating nothing
/ per row; it returns () if it cannot handle the input, and we fall back.
wjbnd:{[g;t;q]$[#g;[b:`wjb(value g#+q;value g#+t);$[#b;b;wjbndK[g;t;q]]];((#t)#0;(#t)#(#q))]}
wjK:{[w;c;t;q;aggs]c:c,();g:-1_c;tc:*|c;q:ajord[c;q];qd:+q;qt:qd tc;qg:$[#g;=rows[g#+q];(,0)!,!#q];tg:$[#g;rows[g#+t];(#t)#0];res:{[qt;qg;tg;w0;w1;qd;aggs;i]gk:tg i;p:$[in[gk;!qg];qg gk;0#0];sel:$[#p;{[gt;p;lo;hi]a:gt'lo-1;p@(a+1)+!0|(gt'hi)-a}[qt p;p;w0 i;w1 i];0#0];{[qd;sel;a](a 1)(qd a 2)@sel}[qd;sel]'aggs}[qt;qg;tg;w 0;w 1;qd;aggs]'!#t;+(+t),(aggs[;0])!+res}
wj:{[w;c;t;q;aggs]c:c,();g:-1_c;tc:*|c;q:ajord[c;q];qd:+q;cs:wjcode'aggs[;1];$[|/cs<0;wjK[w;c;t;q;aggs];[b:wjbnd[g;t;q];+(+t),(aggs[;0])!`wj(qd tc;qd aggs[;2];cs;w 0;w 1;b 0;b 1)]]}

/ ---- qSQL-style select / group-by --------------------------------------------
qwhere:{[t;m]t@&m}
/ qselect: each aggregate's result is joined onto () so an ATOM becomes a
/ 1-row column. Without that, `select a:sum px, b:max sz from t` built the
/ dict `a`b!(210.0;6) and flipping it raised 'length -- the single-aggregate
/ case only appeared to work because the each-result happened to be a 1-item
/ list whose single item was the atom. (),x leaves genuine vectors untouched.
qselect:{[t;a]+(!a)!{[t;f](),f t}[t]'value a}
/ ---- raw-column grouping: the qSQL fast path --------------------------------
/ qby/xgroup/ij used to group with  =rows[cols#+t] .  rows:{+. x} flips the
/ column dict into ONE BOXED K OBJECT PER ROW, so a 10M-row group-by allocated
/ 10M transient objects in the refcounted heap and then grouped *those*: 7.9s of
/ the 8.4s a `select .. by .. from ..` took, against 87ms for the native vector
/ group-by over the same data.  Nothing about the query needed those tuples --
/ they existed only to be hashed and discarded.  The helpers below hand the RAW
/ column vectors straight to the C kernel's vector `=` (group), `?` (find) and
/ `@` (index) primitives instead.
/ qenc: fold a list of columns into ONE dense integer key per row, rank-encoding
/ each column (`?x` distinct, `u?x` find) and mixing radix-style.  Distinct
/ tuples get distinct codes, so `=` over the result groups EXACTLY as grouping
/ the tuples would -- without ever materialising a tuple.
qenc:{[c]{[a;v]u:?v;(a*#u)+u?v}/[(#*c)#0;c]}
/ qcard: product of per-column cardinalities as a float, so the radix mix can be
/ checked against the exactly-representable integer range; past 2^53 we fall
/ back to the old boxed-row path rather than risk a collision.
qcard:{[c]{x*y}/[1.0;{1.0*#?x}'c]}
qEXACT:9007199254740992.0
/ qgrp[t;b] -> (key-value columns;group row-index vectors).  Key order is
/ first-appearance, identical to the =rows[...] this replaces.  Key VALUES come
/ from indexing the raw columns with each group's first row (`*:'`) -- one
/ gather per by-column, instead of a flip over every row in the table.
qgrp:{[t;b]d:+t;bc:d b;g:$[1=#b;=*bc;qcard[bc]<qEXACT;=qenc bc;=rows[b#+t]];iv:value g;(bc@\:*:'iv;iv)}
qby:{[t;b;a]b:b,();r:qgrp[t;b];kt:+b!r 0;iv:r 1;subs:atr[t]'iv;vt:+(!a)!{[subs;f]f'subs}[subs]'value a;kt!vt}
xgroup:{[k;t]k:k,();r:qgrp[t;k];kt:+k!r 0;vc:except[cols t;k];+(+kt),vc!{[t;iv;c]((+t)c)@/:iv}[t;r 1]'vc}
ungroup:{[x]t:unkey x;d:+t;v:value d;m:{`A=@x}'v;cnt:#:'*v@&m;+(!d)!{[cnt;m;c]$[m;raze c;raze cnt#'c]}[cnt]'[m;v]}
fby:{[fc;g]f:fc 0;d:fc 1;gi:=g;iv:value gi;agg:{[f;d;i]f d@i}[f;d]'iv;@[(#g)#*agg;raze iv;:;raze(#:'iv)#'agg]}

/ ---- strings ---------------------------------------------------------------
lower:{_x}
upper:{c:0+x;`c$c-32*(c>96)&c<123}
ltrim:{(+/&\x=" ")_x}
rtrim:{(0-+/&\|x=" ")_x}
trim:{ltrim rtrim x}
/ ss: string-search, s for p, -> match-start positions (empty if none/p longer than s).
/ Guarded explicitly for (#s)<#p: without this, "!1+(#s)-#p" is "!0" (0 candidate
/ starts), "each" over it correctly gives the generic empty list (), but "&" (where)
/ applied to a literal, untyped empty list () mishandles it -- a real bug in the C
/ `whr` kernel's generic-list branch (src/v.c's X1(whr,...), RA case), which is
/ designed for nested/grouped input and returns a spurious 1-element ",!0" instead
/ of a clean empty "!0" when x is simply an empty () rather than a nested list. That
/ single wrong value then propagates through qsql.k's qsplit/sel/exq/upd/del (every
/ one of them calls ss on whatever's left after stripping the leading keyword, and
/ "select from t"/"exec ... from t" with no where/by clause is exactly the case
/ where the remaining separator being searched for, like " where ", is longer than
/ what's left) -- so this one-line ss fix is what actually fixes "select from t"
/ and friends. See docs/MISSING.md for the underlying whr bug, tracked separately.
ss:{[s;p]$[(#s)<#p;!0;&{[s;p;i]p~(#p)#i_s}[s;p]'!1+(#s)-#p]}
ssr:{[s;f;t]t/f\s}
sv:{x/y}
vs:{x\y}
lk1:{[s;p]$[~#p;~#s;"*"=*p;$[#s;lk1[s;1_p]|lk1[1_s;p];lk1[s;1_p]];~#s;0;("?"=*p)|(*p)=*s;lk1[1_s;1_p];0]}
like:{[x;p]$[`C=@x;lk1[x;p];lk1[;p]'x]}

/ ---- temporal : time-of-day held as milliseconds since midnight (like q time)
/ q's  time.minute / time.hh / time.ss  become  minute t / hh t / ss t  in Amber.
hms:{[h;m;s]1000*s+(60*m)+3600*h}                  / build a time from h m s
hh:{_(x%3600000)}                                  / hour of day      (q time.hh)
mm:{60!_(x%60000)}                                 / minute of hour   (q time.mm)
sec:{60!_(x%1000)}                                 / second of minute (q time.ss; ss=string-search)
milli:{1000!x}                                     / millisecond      (q time.ms)
minute:{_(x%60000)}                                / minutes since midnight (q time.minute)
second:{_(x%1000)}                                 / seconds since midnight (q time.second)
z2:{-2$"0",$x}
z3:{-3$"00",$x}
stime1:{(z2 hh x),":",(z2 mm x),":",(z2 sec x),".",z3 milli x}
stime:{$[|/(@x)=`i`j;stime1 x;stime1'x]}           / time -> "HH:MM:SS.mmm"
ptime:{p:"."\x;t:`I$":"\*p;m:$[1<#p;`I$3#(p 1),"000";0];(3600000*t 0)+(60000*t 1)+(1000*t 2)+m}
bar:{[w;u;t]s:w*u;s*_(t%s)}                        / w u-unit bars: bar[5;60000;t] = 5-min bars(ms)
minbar:{[w;t]60000*xbar[w;minute t]}               / w-minute bar as a time (ms)
tsym:{[t;c]+@[+t;c,();stime]}                      / render time cols c of table t as HH:MM:SS strings
/ gentq (trade/quote generator) lives in fin.k, which loads after amber.k.

/ ---- table visualization (q-style grids) -----------------------------------
amcells:{$[`A=@x;{$[`C=@x;x;`k x]}'x;$x]}
/ temporal columns auto-render (no tsym): a column whose NAME is in TCOLS/DCOLS and
/ whose type is integer prints as HH:MM:SS.mmm / YYYY.MM.DD.  Name-based, so it survives
/ every table op (select/aj/group-by/sort keep column names).  Set TCOLS:0#` to disable.
TCOLS:`time`ts`t                                   / time-of-day columns  (ms since midnight)  -> HH:MM:SS.mmm
DCOLS:,`date                                       / date columns (days, temporal.k epoch)     -> YYYY.MM.DD
tnum:{in[@x;`I`L`i`j`l`H`G]}                        / integer-typed column?
tcell:{[nm;col]$[in[nm;TCOLS]&tnum col;stime col;in[nm;DCOLS]&tnum col;dstr'col;(~^PREC)&|/`f`F`e=@col;$round[PREC;col];amcells col]}
/ ---- ANSI syntax highlighting (subtle, dark-terminal) -----------------------
/ Colour is applied AFTER width padding (ANSI is zero visual width); vlen strips
/ ANSI before every width/underline computation so grids stay aligned.  COLOR:0 off.
COLOR:1                                            / master toggle
CSI:(`c$27),"["                                    / ANSI control sequence introducer
RST:CSI,"0m"                                       / reset
CT:`num`sym`str`time`null`hdr`bool`flt`sep!("38;5;45";"38;5;214";"38;5;114";"38;5;141";"90";"1;38;5;231";"38;5;205";"38;5;79";"90")  / 256-colour SGR codes (null/sep = faint grey)
PAL:("38;5;39";"38;5;208";"38;5;84";"38;5;213";"38;5;220";"38;5;51";"38;5;141";"38;5;203";"38;5;190";"38;5;87";"38;5;222";"38;5;159";"38;5;171";"38;5;120")  / vivid per-column cycle
NULLS:("0N";"0n";"0w";"-0w";"0Nn";,"`")            / null sentinels rendered faint grey (empty cell too)
PREC:7                                             / float precision (decimals) in grids; PREC:0N = full precision
comma:{$[4>#x;x;(comma -3_x),",",-3#x]}            / thousands separators on a digit string
vstrip:{p:((`c$27))\x;,/(1#p),{(1+x?"m")_x}'1_p}   / drop ANSI escape sequences
vlen:{#vstrip x}                                    / visible (printed) width
cwrap:{[c;s]$[COLOR&#c;(CSI,c,"m"),s,RST;s]}        / wrap s in SGR code c
atcol:{$[`b=@x;CT`bool;|/`f`F`e=@x;CT`flt;|/`i`I`j`l`L`h`H`G=@x;CT`num;|/`s`S=@x;CT`sym;|/`c`C=@x;CT`str;""]}  / value type -> colour
palN:{PAL@(#PAL)!x}                                 / n-th column colour (cycles the palette)
isnul:{t:x^" ";$[#t;|/t~/:NULLS;1]}                 / null token? (empty/blank cell counts)
ccell:{[cc;s]cwrap[$[isnul s;CT`null;cc];s]}        / colour a padded cell (null override)
trt:{[nm;col]$[(in[nm;TCOLS])|in[nm;DCOLS];1;|/`i`I`j`l`L`h`H`G`f`F`e=@col]}  / right-align numeric/temporal cols
amblk:{[t;c]cl:{[t;x](+t)x}[t]'c;sc:tcell'[c;cl];h:$c;w:(#:'h)|{$[#x;|/#:'x;0]}'sc;cc:palN'!#c;rt:trt'[c;cl];pc:{[w;h;sc;cc;rt;i]sw:$[rt i;neg w i;w i];(,cwrap[CT`hdr;sw$h i]),ccell[cc i]'sw$'sc i}[w;h;sc;cc;rt]'!#c;+pc}
CROWS:20                                           / Q-style: show first CROWS rows of a grid then "..";  set CROWS:n to change
nrows:{#*.+x}                                      / row count of a table
captab:{c:cols x;+c!(CROWS#)'(+x)c}                / first CROWS rows (caps BEFORE formatting, so huge tables render instantly)
aftr:{[nr;nc]cwrap[CT`null;"[",(comma($nr))," rows x ",($nc)," cols]"]}  / dimmed size footer
/ ---- Unicode table border modes (\grid clean|rounded|sharp|heavy) -----------
GRID:`clean                                        / border mode; \grid sets it
/ each entry: (horiz;vert;tl;tmid;tr;lmid;mid;rmid;bl;bmid;br)
GB:`rounded`sharp`heavy!(("─";"│";"╭";"┬";"╮";"├";"┼";"┤";"╰";"┴";"╯");("─";"│";"┌";"┬";"┐";"├";"┼";"┤";"└";"┴";"┘");("━";"┃";"┏";"┳";"┓";"┣";"╋";"┫";"┗";"┻";"┛"))
brep:{,/x#,y}                                       / repeat a (multi-byte) char x times
/ box render: W=visible col widths, rows=(header;data..) of coloured padded cells, tr=truncated?
box:{[W;rows;g;tr]G:GB g;segs:{,/(x+2)#,y}[;G 0]'W;hl:{[a;m;b;s]cwrap[CT`sep;a,(m/s),b]};vb:cwrap[CT`sep;G 1];mkrow:{[vb;cs]vb,(vb/{" ",x," "}'cs),vb}[vb];body:mkrow'1_rows;body:$[tr;body,,mkrow({[w]cwrap[CT`sep;w$".."]}'W);body];(hl[G 2;G 3;G 4;segs];mkrow(*rows);hl[G 5;G 6;G 7;segs]),body,,hl[G 8;G 9;G 10;segs]}
amtab:{[t]nr:nrows t;tr:CROWS<nr;u:$[tr;captab t;t];rows:amblk[u;cols u];b:$[GRID~`clean;[lns:{" "/x}'rows;l:(1#lns),(,cwrap[CT`sep;(vlen(*lns))#"-"]),1_lns;$[tr;l,,"..";l]];box[{vlen x}'(*rows);rows;GRID;tr]];b,,aftr[nr;#cols t]}
amkeyed:{[kt]t:unkey kt;nr:nrows t;tr:CROWS<nr;t:$[tr;captab t;t];kc:keys kt;vc:except[cols t;kc];kb:amblk[t;kc];vb2:amblk[t;vc];b:$[GRID~`clean;[kr:{" "/x}'kb;vr:{" "/x}'vb2;lns:(kr,\:"| "),'vr;l:(1#lns),(,cwrap[CT`sep;(vlen(*lns))#"-"]),1_lns;$[tr;l,,"..";l]];[rows:kb,'vb2;box[{vlen x}'(*rows);rows;GRID;tr]]];b,,aftr[nr;#cols kt]}
amdict:{[d]tr:CROWS<#!d;kk:$[tr;CROWS#!d;!d];vv:$[tr;CROWS#value d;value d];ks:$kk;vs:`k'vv;kw:|/#:'ks;b:{[kw;ks;vs;vv;i](cwrap[palN i;kw$ks i]),"| ",ccell[atcol vv@i;vs i]}[kw;ks;vs;vv]'!#ks;$[tr;b,,"..";b]}
amfmt:{$[iskeyed x;amkeyed x;`M=@x;amtab x;`m=@x;amdict x;`k x]}
show:{`0:amfmt x;}

repl.amfmt:amfmt
