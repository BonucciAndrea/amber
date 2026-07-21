#!k
\d repl
banner:"\n"/("";"   █████╗ ███╗   ███╗██████╗ ███████╗██████╗";"  ██╔══██╗████╗ ████║██╔══██╗██╔════╝██╔══██╗";"  ███████║██╔████╔██║██████╔╝█████╗  ██████╔╝";"  ██╔══██║██║╚██╔╝██║██╔══██╗██╔══╝  ██╔══██╗";"  ██║  ██║██║ ╚═╝ ██║██████╔╝███████╗██║  ██║";"  ╚═╝  ╚═╝╚═╝     ╚═╝╚═════╝ ╚══════╝╚═╝  ╚═╝";"  ▒▒ the array language ▒ v1.4 ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒";"     columnar · vectorised · in-memory · s/u/p/g attributes · O(1) index";"     tables · keyed tables · as-of/window joins · fin.k HFT module";"     \\ help    \\\\ quit    show t / bare t renders a grid";"")
prompt:," " /use 0x0720 for emacs integration
(opn;sem;cls):"(;)"
/(opn;sem;cls):("( ";" ; ";" )") /uncomment this line for extra spacing
lines:cols:999
upd:{(lines;cols)::`I$"\n"\`x(("/usr/bin/env";"tput";"-S");"lines\ncols\n")}
lim:{(x<#y)(x#)/y}
dd:{(x<#y)(,[;".."](x-2)#)/y}
fmt:{upd[];$[(::)~x;"";|/(@x)=`m`M;("\n"/amfmt x),"\n";(1<#x)&|/`m`M`A=@x;mMA x;(dd[cols]`k@lim[cols]x),"\n"]}
fmtx:{$[(::)~x;"";`k[x],"\n"]}
mMA:{(P;f;q):((,"!/+(";dct;,")");(("+![";" +(");tbl;")]");(,,"(";lst;,")"))`m`M`A?t:@x
 w:cols-#*P;u:w-#q;h:lines-2
 x:$[h<(`M=t)+#x;,[;,".."](h-1)#f[w;u;h#x];f[w;u;x]]
 ,[;"\n"]@"\n"/@[;-1+#x;,;q]P[!#x],'x}
lst:{[w;u;x](((-1+#x)#w),u)dd'`k'lim[cols]'x}
dct:{[w;u;x]k:(|/#'k)$k:`k'!x;par'(((-1+#x)#w-3),u-3)dd'sem/'+(k;`k'.x)}
tbl:{[w;u;x]h:`k'!x;d:`k''.x;W:(#'h)|/'#''d
 r:,$[`S~@!x;dd[w](""opn),(""sem)/;par@dd[w-2]@sem/]W$'h
 r,par'dd[w-2]'sem/'+@[W;&~^`i`d?_@'.x;-:]$'d}
cell:{$[|/`i`d=@y;-x;x]$z}
par:{opn,x,cls}
cmds:(,"a")!{`1:1:joinpath[dirname`argv 0]"LICENSE";}
basename:{*|"/"\x}
dirname:{$[#x:"/"/-1_"/"\x;x;,"."]}
joinpath:{$[x~,".";y;"/"~*|x;x,y;x,"/",y]}
line0:{c:{0x07~*-2#*x}{(l;r):x;(1:1;r,,(-2_l))}/(x;());"\n"/(*|c),,*c}
line1:{$[#x;;:0];x:-1_x;$[(3>#x)&("\\"=*x)&~^(!cmds)?x 1;cmds[x 1]x 1;.[`1:(fmt;fmtx)[" "~*x]@.:;,x;{`0:`err[]}]];`1:prompt;1}
line:line1@line0@
$["repl.k"~basename`argv 1;{cmds::@[cmds;x[1]1;:;{y;`0:x}2_x]}'{(&x~\:80#"-")_x:(1+*&x~\:,"/")_-1_x}@0:`argv 1;];
run:{`1:banner,prompt;{line@1:`}::/`;}
\d .
/ load the Amber library by ABSOLUTE path derived from this script's own location,
/ so it works no matter what the current working directory is.
dir::{$[#p:"/"/-1_"/"\x;p;"."]}`argv 1
{.[{. x};,x;{}]}'0:dir,"/amber.k";
{.[{. x};,x;{}]}'0:dir,"/fin.k";
$[.[{sum;1};,`;{0}];;`0:"amber: WARNING - amber.k did not load (is it next to repl.k?)\n"]

$[(~"kw"~`argv 0)&~"--non-interactive"~*x;repl.run[];]

/
--------------------------------------------------------------------------------

                 A M B E R   help      ( type  \X  for a topic )

  AMBER q-layer                             core language
  \q  scalars, aggregation, uniform, sets   \0  types
  \j  tables, keyed tables, joins, qSQL     \+  verbs
  \z  temporal, attributes (s/u/p/g), show  \'  adverbs   \`  symbols
  \m  finance / HFT module (fin.k)          \h  core cheat-sheet
  session
  \a  license (AGPLv3)     \l file.k  load a script     \v  variables
  \d ns   namespace (\d .) \t:n expr  time n runs (ms)  \f  functions
  \cd path   change dir    \other     shell command     \\  exit

  quick start:  show t   meta t   sum x   aj[`sym`time;trade;quote]
--------------------------------------------------------------------------------
\0
Types:
list atom
 `A        generic list   ()   ,"ab"   (0;`1;"2";{3};%)
 `I   `i   int            0N -9223372036854775807 01b
 `F   `f   float          -0w -0.0 0.0 0w 1.2e308 0n
 `C   `c   char           "a"   0x6263   "d\0\"\n\r\t"
 `S   `s   symbol         `   `a   `"file.txt"   `b`cd`"ef"
 `M   `m   table&dict     +`a`b!(0 1;"23")   (0#`)!()
      `o   lambda         {1+x*y#z}  {[a;b]+/*/2#,a,b}
      `p   projection     1+   {z}[0;1]   @[;i;;]
      `q   composition    *|:   1+{x*y}@
      `r   derived verb   +/   2\   {y,x}':
      `u   monadic verb   +:   0::
      `v   dyadic  verb   +   0:
      `w   adverb         '   /:
      `x   external func
--------------------------------------------------------------------------------
\:
I/O verbs
  0:x read  lines
x 0:y write lines
  1:x read  bytes
x 1:y write bytes
   <s open          fd:<`"file.txt"
   >i close         >fd

x can be a file descriptor (int) or symbol or string such as
 "file.txt"
 "/path/to/file"
 "host:port"
 ":port"         /host defaults to 127.0.0.1
--------------------------------------------------------------------------------
\+
Verbs:    : + - * % ! & | < > = ~ , ^ # _ $ ? @ . 0: 1:
notation: [c]har [i]nt [n]umber(int|float|char) [s]ymbol [a]tom [d]ict
          [f]unc(monad) [F]unc(dyad) [xyz]any
special:  var:y     set    a:1;a -> 1
          var::y    global a:1;{a::2}[];a -> 2
          (v;..):y  unpack (b;(c;d)):(2 3;4 5);c -> 4
          :x        return {:x+1;2}[3] -> 4
          :[x;y;..] cond   :[0;`a;"\0";`b;`;`c;();`d;`e] -> `e
          o[..]     recur  {:[x<2;x;+/o'x-1 2]}9 -> 34
          [..]      progn  [0;1;2;3] -> 3

::  self      f:(::);f 12 -> 12
 :  right     f:(:);f[1;2] -> 2   "abc":'"d" -> "ddd"
 +x flip      +("ab";"cd") -> ("ac";"bd")
N+N add       1 2+3 -> 4 5
 -N negate    - 1 2 -> -1 -2
N-N subtract  1-2 3 -> -1 -2
 *x first     *`a`b -> `a   *(0 1;"cd") -> 0 1
N*N multiply  1 2*3 4 -> 3 8
 %N sqrt      %25 -> 5.0   %-1 -> 0n
N%N divide    2 3%4 -> 0.5 0.75
 !i enum      !3 -> 0 1 2   !-3 -> -3 -2 -1
 !I odometer  !2 3 -> (0 0 0 1 1 1;0 1 2 0 1 2)
 !d keys      !`a`b!0 1 -> `a`b
 !S ns keys   a.b.c:1;a.b.d:2;!`a`b -> `c`d
x!y dict      `a`b!1 2 -> `a`b!1 2
i!I div       -10!1234 567 -> 123 56
i!I mod       10!1234 567 -> 4 7
 &I where     &3 -> 0 0 0   &1 0 1 4 2 -> 0 2 3 3 3 3 4 4
 &x deepwhere &(0 1 0;1 0 0;1 1 1) -> (0 1 2 2 2;1 0 0 1 2)
N&N min/and   2&-1 3 -> -1 2   0 0 1 1&0 1 0 1 -> 0 0 0 1
 |x reverse   |"abc" -> "cba"   |12 -> 12
N|N max/or    2|-1 3 -> 2 3   0 0 1 1|0 1 0 1 -> 0 1 1 1
 <X ascend    <"abacus" -> 0 2 1 3 5 4
 >X descend   >"abacus" -> 4 5 3 1 0 2
 <s open      fd:<`"/path/to/file.txt"
 >i close     >fd
N<N less      0 2<1 -> 1 0
N>N more      0 1>0 2 -> 0 0
 =X group     ="abracadabra" -> "abrcd"!(0 3 5 7 10;1 8;2 9;,4;,6)
 =i unitmat   =3 -> (1 0 0;0 1 0;0 0 1)
N=N equal     0 1 2=0 1 3 -> 1 1 0
 ~x not       ~(0 2;``a;"a \0";::;{}) -> (1 0;1 0;0 0 1;1;0)
x~y match     2 3~2 3 -> 1   "4"~4 -> 0   0~0.0 -> 0
 ,x enlist    ,0 -> ,0   ,0 1 -> ,0 1   ,`a!1 -> +(,`a)!,,1
x,y concat    0,1 2 -> 0 1 2  "a",1 -> ("a";1)
d,d merge     (`a`b!0 1),`b`c!2 3 -> `a`b`c!0 2 3
 ^x null      ^(" a";0 1 0N;``a;0.0 0n) -> (1 0;0 0 1;1 0;0 1)
a^y fill      1^0 0N 2 3 0N -> 0 1 2 3 1   "b"^" " -> "b"
X^y without   "abracadabra"^"bc" -> "araadara"
 #x length    #"abc" -> 3   #4 -> 1   #`a`b`c!0 1 0 -> 3
i#y take      5#"abc" -> "abcab"   -5#`a`b`c -> `b`c`a`b`c
X#d take keys `c`d`f#`a`b`c`d!1 2 3 4 -> `c`d`f!3 4 0N
I#y reshape   2 3#` -> (```;```)
f#y replicate (3>#:')#(0;2 1 3;5 4) -> (0;5 4)   {2}#"ab" -> "aabb"
 _n floor     _12.34 -12.34 -> 12 -13
 _c lowercase _"Ab" -> "ab"
i_Y drop      2_"abcde" -> "cde"   -2_`a`b`c -> ,`a
X_d drop keys `a`c_`a`b`c!0 1 2 -> (,`b)!,1
I_Y cut       2 4 4_"abcde" -> ("cd";"";,"e")
f_Y weed out  (3>#:')_(0;2 1 3;5 4) -> ,2 1 3
X_i delete    "abcde"_2 -> "abde"
 $x string    $(12;"ab";`cd;+) -> ("12";(,"a";,"b");"cd";,"+")
i$C pad       5$"abc" -> "abc  "   -3$"a" -> "  a"
s$y cast      `c$97 -> "a"   `i$-1.2 -> -1   `$"a" -> `a
s$y int       `I$"-12" -> -12
 ?X distinct  ?"abacus" -> "abcus"
 ?i uniform   ?2 -> 0.6438163747387873 0.8852656305774402 /random
X?y find      "abcde"?"bfe" -> 1 0N 4
i?x roll      3?1000 -> 11 398 293   1?0 -> ,-8164324247243690787
i?x deal      -3?1000 -> 11 398 293 /guaranteed distinct
 @x type      @1 -> `i   @"ab" -> `C   @() -> `A   @(@) -> `v
x@y apply(1)  {x+1}@2 -> 3   "abc"@1 -> "b"   (`a`b!0 1)@`b -> 1
 .S get       a:1;.`a -> 1   b.c:2;.`b`c -> 2
 .C eval      ."1+2" -> 3
 .d values    .`a`b!0 1 -> 0 1
x.y apply(n)  {x*y+1}. 2 3 -> 8   (`a`b`c;`d`e`f). 1 0 -> `d

@[x;y;f]   amend  @["ABC";1;_:] -> "AbC"   @[2 3;1;{-x}] -> 2 -3
@[x;y;F;z] amend  @["abc";1;:;"x"] -> "axc"   @[2 3;0;+;4] -> 6 3
.[x;y;f]   drill  .[("AB";"CD");1 0;_:] -> ("AB";"cD")
.[x;y;F;z] drill  .[("ab";"cd");1 0;:;"x"] -> ("ab";"xd")
.[f;y;f]   try    .[+;1 2;"E:",] -> 3   .[+;1,`;"E:",] -> "E:'type\n"
?[x;y;z]   splice ?["abcd";1 3;"xyz"] -> "axyzd"
--------------------------------------------------------------------------------
\`
Special symbols:
   `j?C parse json   `j?"{\"a\":1,\"b\":[true,\"c\"]}" -> `a`b!(1.0;(1;,"c"))
   `k@x pretty-print `k("ab";2 3) -> "(\"ab\";2 3)"
   `p@C parse k
 `hex@C hexadecimal  `hex"ab" -> "6162"
 `pri@i primes       `pri 20  ->  2 3 5 7 11 13 17 19
   `x@x fork-exec    `x(("/bin/wc";"-l");"a\nbc\nd\n") -> "3\n"
   `t[] current time (microseconds)
`argv[] list of cmd line args (also in global variable x)
 `env[] dict of env variables
`prng[] `prng@I get/set pseudo-random number generator internal state
                     s:`prng[];r:9?0;`prng s;r~9?0 -> 1
        `prng@0 use current time to set state
 `err@C throw error
 `sin@N trigonometry `sin 12.34 -> -0.22444212919135995
 `exp@N exponential  `exp 1 -> 2.7182818284590455
  `ln@N logarithm    `ln 2 -> 0.6931471805599453
`exit@i exit
--------------------------------------------------------------------------------
\'
Adverbs:   ' / \ ': /: \:
   f' each1     #'("abc";3 4 5 6) -> 3 4
 x F' each2     2 3#'"ab" -> ("aa";"bbb")
   X' binsearch 1 3 5 7 9'8 9 0 -> 3 4 -1
   I'[X;..] case 2 0 1 2'[`a`b`c`d;1 2 3 4;"abcd"] -> ("a";`b;3;"d")
   F/ fold      +/1 2 3 -> 6
   F\ scan      +\1 2 3 -> 1 3 6
 x F/ seeded /  10+/1 2 3 -> 16
 x F\ seeded \  10+\1 2 3 -> 11 13 16
 i f/ n-do      5(2*)/1 -> 32
 i f\ n-dos     5(2*)\1 -> 1 2 4 8 16 32
 f f/ while     (1<){:[2!x;1+3*x;-2!x]}/3 -> 1
 f f\ whiles    (1<){:[2!x;1+3*x;-2!x]}\3 -> 3 10 5 16 8 4 2 1
   f/ converge  {1+1.0%x}/1 -> 1.618033988749895
   f\ converges (-2!)\100 -> 100 50 25 12 6 3 1 0
   C/ join      "ra"/("ab";"cadab";"") -> "abracadabra"
   C\ split     "ra"\"abracadabra" -> ("ab";"cadab";"")
   I/ decode    24 60 60/1 2 3 -> 3723   2/1 1 0 1 -> 13
   I\ encode    24 60 60\3723 -> 1 2 3   2\13 -> 1 1 0 1
  i': window    3':"abcdef" -> ("abc";"bcd";"cde";"def")
i f': stencil   3{x,"."}':"abcde" -> ("abc.";"bcd.";"cde.")
  F': eachprior -':12 13 11 17 14 -> 12 1 -2 6 -3
x F': seeded ': 10-':12 13 11 17 14 -> 2 1 -2 6 -3
x F/: eachright 1 2*/:3 4 -> (3 6;4 8)
x F\: eachleft  1 2*\:3 4 -> (3 4;6 8)
--------------------------------------------------------------------------------
\h
: SET      RETURN    :[c;t;f]     COND
+ add      flip
- subtract negate    '  each|slide|bin
* multiply first     /  fold|join |dec|comment
% divide   sqrt      \  scan|split|enc|trace
! mod|dict enum|key  ': eachprior
& min|and  where     /: eachright
| max|or   reverse   \: eachleft
< less     ascend
> more     descend   /
= equal    group     multiline comment
~ match    not       \
, concat   enlist
^ without  null      0: lines i/o
# reshape  length    1: bytes i/o
_ drop|cut floor
$ cast     string
? find|rnd uniq      ?[a;i;b]     splice
@ apply(1) type      @[x;i;[f;]y] amend
. apply(n) eval      .[x;i;[f;]y] drill
grammar:  E:E;e|e e:nve|te| t:n|v v:tA|V n:t[E]|(E)|{E}|N
limits:   8args 16locals 256globals 256bytecode 2048stack
--------------------------------------------------------------------------------
\q
Amber q-layer : scalars, aggregation, uniform, ordering, sets, strings
NOTE two-argument library functions take brackets:  f[x;y]  (no infix user verbs)

SCALAR (monadic, call as  f x)
  neg not null reciprocal sqrt floor ceiling signum abs exp log sin cos
  til enlist string type key value first last reverse distinct group where flip count
DYADIC (brackets)
  mod[x;y]  div[x;y]  xbar[w;x]  xlog[base;x]
AGGREGATION (list -> atom)
  sum prd min max avg med var dev svar sdev  all any
  wsum[x;y] wavg[x;y] cov[x;y] scov[x;y] cor[x;y]
UNIFORM (list -> list)
  sums prds mins maxs deltas ratios differ prev next
ORDER / RANK
  asc desc iasc idesc rank   xrank[n;x] xprev[n;x] rotate[n;x]
  asc and xasc set the `s sorted attribute  ->  O(log n) search
SETS (brackets)
  in[x;y] except[x;y] inter[x;y] union[x;y] raze x sublist[n;x] cross[x;y]
STRINGS
  lower upper trim ltrim rtrim
  ss[s;sub] ssr[s;from;to] sv[d;x] vs[d;x] like[x;pat]   (globs: * and ?)
--------------------------------------------------------------------------------
\j
Amber q-layer : tables, keyed tables, joins, qSQL

BUILD / INSPECT
  t:([]a:1 2;b:3 4)      table literal   ([key:..]col:..) = keyed table
  t:+`a`b!(1 2;3 4)      (also: flip of a column dict)
  show t                 grid    t`a col   t i row-dict   #t rows   cols t   meta t
  istable isdict iskeyed  meta shows column type AND attribute (a)
KEYED TABLES
  xkey[`k;t]  unkey t  keys t  xcols[c;t]  xcolall[nm;t]  xasc[c;t]  xdesc[c;t]
RESTRUCTURE
  xgroup[k;t]  ungroup t  fby[(f;d);g]  insert[t;r]
JOINS  (left operand drives ; c = `key..`time , last col is the sort/time key)
  lj[t;kt] left     ij[t;kt] inner    uj[x;y] union    pj[t;kt] plus   ej[c;x;y] equi
  aj[c;x;y] as-of   aj0[c;x;y] as-of(y time)   asof[t;d] single    wj[w;c;t;q;aggs] window
  wj: w=(begins;ends) aligned to t ; aggs=((name;fn;col);..)
SELECT / GROUP-BY
  qwhere[t;mask]              select rows where mask
  qselect[t; name!funcs]      computed columns (func takes the table)
  qby[t; by; name!funcs]      group aggregate -> keyed table (func takes group sub-table)
  e.g.  qby[trade;`sym; `vwap`n!({wavg[x`sz;x`px]};{#x})]
--------------------------------------------------------------------------------
\z
Amber q-layer : temporal (tick bars), attributes, display

TIME OF DAY  (milliseconds since midnight, like q `time)
  hms[h;m;s]  build a time         minute t  minutes since midnight (q time.minute)
  hh t   hour        (time.hh)      second t  seconds since midnight (q time.second)
  mm t   min of hour (time.mm)      milli t   millisecond
  sec t  sec of min  (time.ss*)     stime t   -> "HH:MM:SS.mmm"    ptime "09:30:00.5" -> ms
  (* the q accessor is time.ss ; here it is sec, since ss = string-search)
BARS  (tick.minute-style bucketing)
  minbar[w;t]  w-minute bar (a time)      bar[w;u;t]  w u-unit bars in ms
  xbar[w;x]    generic bucketing          minbar[5;trade`time] = 5-minute bars
  1-min OHLCV:  qby[+@[+trade;,`time;minbar[1]@]; `sym`time; open/high/low/close/vol]
  full worked example:  \l examples/tick.k
ATTRIBUTES  (implemented in C ; speed up search ; meta shows them in `a`)
  `sa x  set sorted   (`s)   sorted asc     -> binary-search find   O(log n)
  `ua x  set unique   (`u)   distinct keys  -> hash-like membership
  `pa x  set parted   (`p)   grouped runs   -> binary-search find   O(log n)
  `ga x  set grouped  (`g)   any order      -> group index          O(1) slice
  `at x  read the attribute of x   ->  `s / `u / `p / `g  (or ` if none)
  asc / xasc set `s for you.  fin.k: sortcol[c;t] partcol[c;t] groupcol[c;t]
  set an attribute on a table column ; gentq marks time `s and sym `p.
  bench.k  ~1000-2000x find speedup ;  bench-fin.k  O(1) per-symbol slicing
DISPLAY
  show x      grid-print table / keyed table / dict
  tsym[t;c]   show time columns c as HH:MM:SS strings
  amfmt x     -> the grid as a list of lines
--------------------------------------------------------------------------------
\m
Amber finance / HFT module  (fin.k, auto-loaded after amber.k)

DATA GENERATOR
  gentq n     sets globals  trades  and  quotes  (n trades, 2n quotes)
              numeric times ; time is `s sorted , sym is `p parted (see meta)
ATTRIBUTES ON TABLE COLUMNS
  sortcol[c;t] mark col c `s      partcol[c;t] mark `p      groupcol[c;t] mark `g
O(1) PER-SYMBOL INDEX
  gidx x       vector -> value!positions        bysym t   -> sym!row-positions
  symrows[t;g;s]  rows of t for symbol s in O(1)   (g:bysym t)
QUOTE / ORDER-BOOK  (take a quote table)
  mid q   spread q   spreadbps q   micro q (microprice)   imbal q (book imbalance)
TRADE / TAQ  (aj trades to quotes first:  m:taq[trades;quotes])
  taq[tr;qu]  as-of match      tsign m  Lee-Ready sign (+1 buy/-1 sell)
  signedvol m signed volume    effspread m 2|px-mid|    notional m  px*sz
PRICE SERIES  (a numeric vector)
  ret x  logret x  rvol x  vwap t  twap t
ROLLING / MOVING  (front-padded to length)
  movavg[w;x] movsum[w;x] movmax[w;x] movmin[w;x]  ema[a;x]  rollstd[w;x]
AGGREGATION
  bars[w;t]  w-minute OHLCV keyed table     symstats t  per-symbol summary
  pt x       print a table with `time formatted HH:MM:SS.mmm
  worked example:  \l examples/hft.k
\
