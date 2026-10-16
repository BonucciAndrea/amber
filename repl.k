#!k
\d repl
`trv 1   / 2.9.0: a trap is silent, as q's; the REPL's own, round each line, still shows the report
prompt:"amber> " /use 0x0720 for emacs integration
(opn;sem;cls):"(;)"
/(opn;sem;cls):("( ";" ; ";" )") /uncomment this line for extra spacing
lines:cols:999
/ 1.9.5: the terminal-size probe must never be able to break the DISPLAY path.
/ `tput -S` writes NOTHING when $TERM is unset -- the normal state of a CI step,
/ a cron job, a docker RUN, or anything else that is not a terminal -- and the
/ old unconditional (lines;cols):: then assigned a 0-element vector to a
/ 2-element target and raised E0103 "Vector length mismatch". fmt calls upd[]
/ before formatting ANY value, so the engine computed the right answer and then
/ failed to PRINT it: `100+`pr7 0` reported a length error instead of 107, and
/ `#`aio[-1]` reported one instead of 7, which read as "the extension did not
/ link" when the extension had linked perfectly.
/ tchk therefore returns a vector that is ALWAYS two usable numbers -- the probe
/ when it answered, the values already in force when it did not -- so the
/ assignment below cannot fail. The probe also latches off after a failure: a
/ missing or mute tput then costs one fork rather than one per result, and
/ cannot repeat its own stderr on every line of a session.
tprobe:1
tchk:{p:`diag 0;v:.[{`I$"\n"\`x(("/usr/bin/env";"tput";"-S");"lines\ncols\n")};,0;(!0)];`diag p;tprobe::(2=#v)&(&/v>0);$[tprobe;v;lines,cols]}
/ 2.0.0: the size now comes from `bi 0 (ln.c's ioctl(TIOCGWINSZ)) -- no fork at
/ all, and it is the SAME probe the editor lays the status bar out with, so the
/ two can never disagree.  The tput path below is kept only as the fallback for a
/ core too old to report indices 6/7.
tsz:{v:.[{`bi 0};,0;!0];$[(7<#v)&(0<v 6)&0<v 7;(v 7;v 6);$[tprobe;tchk[];lines,cols]]}
/ With the status bar up, ln.c reserves the bottom SB_FOOT (4) rows for the
/ footer and output scrolls only in rows 1..h-4, so the height fmt may lay a
/ table out in is FOUR LESS than the terminal's. Using the raw height made a
/ full-height table 4 rows too tall and silently scrolled its own header off the
/ top. Guarded, because upd can run before sbon is defined.
sbfoot:{.[{$[sbon;4;0]};,0;0]}
upd:{v:tsz[];(lines;cols)::((v@0)-sbfoot[];v@1)}
lim:{(x<#y)(x#)/y}
dd:{(x<#y)(,[;".."](x-2)#)/y}
fmt:{upd[];$[(::)~x;"";|/(@x)=`m`M;("\n"/amfmt x),"\n";(1<#x)&|/`m`M`A=@x;mMA x;(dd[cols]`k@lim[cols]x),"\n"]}
fmtx:{$[(::)~x;"";`k[x],"\n"]}
/ 1.9.5: the one call an optional extension is reached through. `ext.NAME` is a
/ root-namespace function an installed package (see src/ext.h and lib/) may
/ define; nothing here requires it to exist. The rich stderr diagnostic is
/ switched off around the call, so a stock install -- where every ext.* is
/ undefined -- stays completely silent instead of painting an error report
/ after every line. Returns 0 when the hook is absent or throws.
exthook:{[f;a]p:`diag 0;r:.[f;,a;0];`diag p;r}
extstr:{$[`C~@r:exthook[x;y];r;""]}
/ error ergonomics: expand the one-word error to descriptive text, keep the core's ^ caret.
edesc:`compile`domain`index`length`nyi`io`parse`rank`stack`type`value`limit!("compile: expression could not be compiled";"domain: value outside the valid domain for this operation";"index: index out of range";"length: operands have mismatched counts";"nyi: not implemented for these types";"io: input/output error";"parse: malformed expression";"rank: wrong number of arguments";"stack: call depth exceeded";"type: wrong type for this operation";"value: undefined name or empty value";"limit: implementation limit reached")
/ 1.9.4: when the rich stderr diagnostic is enabled (the default) the core has
/ ALREADY printed a full report for this error, so echoing the descriptive
/ legacy text here would show the same failure twice in two formats. Print it
/ only when diagnostics are off (`diag 0), so nothing is ever swallowed.
/ `diag with a non-numeric argument reads the setting without changing it.
onerr:{exthook[{ext.err x};x];$[0<`diag(::);;[b:`err[];lns:"\n"\b;h:$[#lns;*lns;""];nm:`$$[(0<#h)&"'"~*h;1_h;h];`0:$[(0<#h)&|/nm=!edesc;("'",edesc nm),"\n","\n"/1_lns;b]]];}
/ \grid clean|rounded|sharp|heavy : set the table border mode (root global GRID, via eval).
gridcmd:{p:" "\x;$[1<#p;[m:`$p 1;$[4>`clean`rounded`sharp`heavy?m;[.:("GRID:`",$m);`1:"grid: ",($m),"\n"];`1:"grid: modes are clean rounded sharp heavy\n"]];`1:"grid: ",($(.:"GRID")),"\n"]}
/ \clear : clear the terminal (scrollback + screen, cursor home).
clearcmd:{`1:,/(((`c$27),"[3J");((`c$27),"[H");((`c$27),"[2J");"\n");$[sbon;sbupd[];]}
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
/ 1.9.5: input comes from `rdl (src/ln.c, src/lnk.c) instead of a raw read on
/ fd 1. `rdl prints the prompt itself and gives the line native editing,
/ history and inline editing when stdin is a terminal; when it is a pipe it
/ degrades to a plain line read with the prompt still printed, which is also
/ why piping a script into the REPL works. This is what removed the need to
/ run Amber under rlwrap -- see the comment block in ./a.
rd:{`rdl x}
/ The qSQL rewriter runs on every typed line, which would silently rewrite the
/ ARGUMENT of an extension command (e.g. an "optimize select px from t" request)
/ before the extension ever saw it. An extension may define ext.raw to claim
/ lines that must be passed through byte-for-byte; every other \-command still
/ gets the rewrite, so \t select ... from t keeps working.
/ \ts is q's "time and space" command.  Amber's timer is \t, and an unrecognised
/ \-word is a SHELL escape -- so a q reflex typed at this prompt ran `ts` through
/ /bin/sh ("ts: not found") and silently never timed anything.  Map the whole
/ \ts family onto \t before anything else looks at the line: \ts EXPR -> \t EXPR
/ and \ts:n EXPR -> \t:n EXPR.
tsfix:{$[(3<#x)&("\\ts"~3#x)&(" "~x 3)|":"~x 3;"\\t",3_x;x]}
qrwx:{$[0<exthook[{ext.raw x};x];x;(.:"qrw")tsfix x]}
/ ---- Claude-Code-style bottom status bar (\sb toggles) -----------------------
/ ON by default (sbon below); \sb turns it off and restores the plain prompt.
/ src/ln.c OWNS the panel -- two fixed bottom rows
/ (a muted bar + a live info line) plus a DECSTBM scroll region above them -- and
/ repaints it on Ctrl-L, \clear, a resize and every keystroke.  Here we only BUILD
/ the two content strings and hand them to the `sbb verb each prompt.  Byte 0x01
/ (sA) toggles the coral accent inside a string.  ln.c's atexit hook resets the
/ scroll region on EVERY exit path (\\, Ctrl-D, a crash), so the terminal is
/ never left corrupted.
sbon:1                                              / status bar is ON by default; \sb toggles it off
sblast:0.0                                          / ms the last evaluated line took
/ single info panel on row h-1 (the input owns row h).  Marker bytes ln.c handles:
/   0x01 toggle amber accent · 0x02 -> live RSS MB · 0x03 split (right-align the
/   rest) · 0x04 -> build kind (native|portable).  repl.k supplies version, the
/   last exec time (sblast, captured in line1) and the reserved-heap size.
sA:`c$1                                             / accent toggle
sM:`c$2                                             / -> RSS in MB (ln.c fills)
sSP:`c$3                                            / left / right split
sB:`c$4                                             / -> build kind (ln.c fills)
/ exec time, always in ms to 3 decimals (a sub-millisecond line reads "0.017 ms").
/ Built from INTEGER tenths (n), not a float multiply: 0.1*818 is 81.80000000000001
/ in IEEE double and $ prints every digit of it, so the bar read "81.80000000000001 ms".
/ Splitting n into _n%1000 and 1000!n gives "81.800" exactly, for every value.
/ pad3 zero-pads the fraction: 9.008 ms is n=9008, whose 1000!n is 8, and a bare
/ $8 would render "9.8" -- three digits are mandatory, not cosmetic.
/ Note also: k has no bare ".1" float literal -- ".1" lexes as the verb "."
/ applied to 1, which YIELDS 1. The original ".1*_0.5+10*sblast" therefore
/ silently reported TEN TIMES the real time (a 23 ms line read "230 ms").
/ sblast is milliseconds already: `sbt[] returns monotonic ms since the line was
/ submitted (ln.c stamps the start at Enter), read in line1 right after the eval.
pad3:{s:$x;((3-#s)#"0"),s}
sbexec:{n:_0.5+1000*sblast;($_n%1000),".",pad3[1000!n]," ms"}
sbpanel:{m:binfo[];rmb:$_0.5+(m@0)%1048576.0;(,sA),"⬡ amber ",(m@4),(,sA),"   ·   exec: ",sbexec[],"   ·   mem: ",(,sM),"/",rmb," MB   ·   [",(,sB),"]",(,sSP),"\\ help   ·   \\\\ exit"}
sbupd:{$[sbon;`sbb(1;sbpanel[];"");]}
sbtoggle:{sbon::~sbon;$[sbon;sbupd[];`sbb(0;"";"")]}
line0:{c:{0x07~*-2#*x}{(l;r):x;(rd prompt;r,,(-2_l))}/(x;());"\n"/(*|c),,*c}
line1:{$[#x;;:0];x:-1_x;exthook[{ext.pre x};x];s0:`sbt[];$[(4<#x)&"\\grid"~5#x;gridcmd x;(5<#x)&"\\clear"~6#x;clearcmd[];(2<#x)&"\\sb"~3#x;sbtoggle[];(1<#x)&"\\\\"~2#x;[`sbb(0;"";"");.:x];(3>#x)&("\\"=*x)&~^(!cmds)?x 1;cmds[x 1]x 1;(4<#x)&"\\ast"~4#x;.[`1:(fmt;fmtx)[" "~*x]@.:;,x;onerr];.[`1:(fmt;fmtx)[" "~*x]@.:;,qrwx x;onerr]];sblast::sblast+`sbt[]-s0;exthook[{ext.post x};x];1}
/ A folded paste arrives as one line with its statements joined by 0x1e (ln.c's
/ read_paste).  Split on it and run each statement through the normal pipeline so
/ a pasted block of many statements evaluates as a unit in a single read.
runbatch:{[s]{line1 line0 (x,"\n")}'(`c$30)\-1_s;1}
/ sblast accumulates each statement's `sbt[] delta (measured tightly around the
/ eval INSIDE line1, both clock reads in k), so a folded paste reports the SUM of
/ its statements' run times.  Reset to 0 here at the start of every input line.
line:{sblast::0;r:$[(|/x=`c$30);runbatch x;line1 line0 x];r}
/ ---- the \ help pages ------------------------------------------------------------------------------------
/ The pages are the text after the block comment at the end of this file, one per section between rules of
/ dashes: the key on its first line (\q, or "\ menu" for the menu), the title on its second. The rest is a
/ light markup: "# HEAD" a heading, "a¦b¦c" a row (its columns line up down the page, the last one wraps
/ under itself), two spaces an example (its " -> " result and " / " comment quieter), "| " a line kept as it
/ is, "~ " a note. Colour follows COLOR and NO_COLOR and a page reads the same without it. Keys are bold in
/ the terminal's own colour so they read on a light screen too, and AMBER_NO_EDIT (the screen reader setting)
/ drops the rules and the logo, which a reader would spell out
hc:0
/ repl.k's functions see only this namespace, so the root's COLOR is read through .: and the two helpers below
/ stand in for amber.k's hvl and ss
henv:{(#k)>(k:!`env[])?`$x}
hcol:{$[~1~.[{.:"COLOR"};,0;0];0;henv"NO_COLOR";0;1]}
hp:0
hvl:{+/~(0+x)<-64}
hss:{[s;d]$[(#d)>#s;!0;&{x~y}[d]'(#d)':s]}
hsg:{[c;s]$[hc&(0<#c)&0<#s;(`c$27),"[",c,"m",s,(`c$27),"[0m";s]}
HA:"1;38;2;225;140;0"
HK:"1"
HX:""
HM:"38;2;140;132;120"
HR:"38;2;120;114;104"
hpad:{[n;s]s,(0|n-hvl[s])#" "}
hwr:{[n;s]$[0=#s;,"";(,"") {[n;r;x]$[(0<#*|r)&n<(hvl[*|r])+1+hvl[x];r,,x;@[r;-1+#r;{$[#x;x," ",y;y]}[;x]]]}[n]/" "\s]}
hcut:{[s;d]p:hss[s;d];$[#p;((*p)#s;(*p)_s);(s;"")]}
hex:{[s](c;r):hcut[s;" -> "];(c;m):hcut[c;" / "];hsg[HX;c],hsg[HM;m,r]}
/ A row: the key, then its words wrapped under themselves. With a third column the words are padded and the
/ example goes after them, on a line of its own if it doesn't fit. A key wider than half the page sits alone
hr1:{[w;k;d;e](,"  ",hsg[HK;k]),{"      ",x}'hwr[w-6;d,$[#e;"  ",e;""]]}
hr2:{[w;kw;k;d]i:4+kw;r:hwr[w-i;d];(,"  ",hsg[HK;hpad[kw;k]],"  ",*r),{[i;x](i#" "),x}[i]'1_r}
hr3:{[w;kw;dw;k;d;e]i:4+kw;l:"  ",hsg[HK;hpad[kw;k]],"  ",hpad[dw;d];$[w<i+dw+2+hvl[e];(l;((i+dw+2)#" "),hex e);,l,"  ",hex e]}
hrow:{[w;kw;dw;f]k:*f;d:$[1<#f;f 1;""];e:$[2<#f;f 2;""];$[kw>_w%2;hr1[w;k;d;e];#e;hr3[w;kw;dw;k;d;e];hr2[w;kw;k;d]]}
hnote:{[w;s]r:hwr[w-4;s];hsg[HM]'(,"  › ",*r),{"    ",x}'1_r}
hline:{[w;l;r;f;kw;dw]$[r;hrow[w;kw;dw;f];0=#l;,"";"# "~2#l;("";"  ",hsg[HA;2_l]);"  "~2#l;,"    ",hex 2_l;"| "~2#l;,"  ",2_l;"~ "~2#l;hnote[w;2_l];{"  ",x}'hwr[w-2;l]]}
hfoot:{$[" "=x 1;"type a topic's key, like \\j   ·   \\\\ leaves Amber";"\\ back to every topic   ·   \\\\ leaves Amber"]}
hpage:{[t]hc::hcol[];hp::henv"AMBER_NO_EDIT";w:50|88&tsz[]1;L:2_t;r:{0<#hss[x;"¦"]}'L;F:{[r;x]$[r;"¦"\x;()]}'[r;L]
 kw:|/0,{$[#x;hvl[*x];0]}'F;dw:|/0,{$[2<#x;hvl[x 1];0]}'F;rl:$[hp;();,"  ",hsg[HR;,/(w-4)#,"─"]]
 `0:("";"  ",$[hp;"amber · ";hsg[HA;"⬡ amber"],hsg[HM;"  ·  "]],t 1),rl,(,/hline[w]'[L;r;F;kw;dw]),(,""),rl,("  ",hsg[HM;hfoot@*t];"");}
$["repl.k"~basename`argv 1;{cmds::@[cmds;x[1]1;:;{y;.[hpage;,x;{`0:,"help: ",$[`C~@x;x;$x]}]}1_x]}'{(&x~\:80#"-")_x:(1+*&x~\:,"/")_-1_x}@0:`argv 1;];
/ `bi 0 (process/build info) can throw 'domain on non-Linux builds (e.g. macOS);
/ trap it so the banner still renders.  Returns (heap;used;arch;compiler).
binfo:{.[{`bi 0};,0;(10485760;0;"arm64";"Darwin";"1.9.5")]}
/ 1.9.5: (m 5) is the compiled-in extension tag from src/ext.h (am_ext_banner),
/ "" on a stock build; ext.tag[] is the optional Amber-level one. Both are
/ absent from a stock install, so the banner is character-for-character 1.9.4's.
extag:{$[5<#x;x 5;""]}
banner:{m:binfo[];"Amber v",(m 4)," (",(m 2),"; ",(m 3),") — Columnar Time-Series Engine",extag[m],extstr[{ext.tag[]};0],"\n"}
/ sbupd BEFORE the banner enters the alternate screen (ln.c) so the banner and
/ the whole session live there, vim-style; \\ / \sb leave it and restore the
/ terminal exactly as it was.
run:{m:binfo[];`1:(`c$27),"[4q";sbupd[];`1:banner[],"Memory: ",($0.1*_0.5+(m 0)%104857.6)," MB heap reserved | \\ for help command, \\\\ to exit  (\\sb toggles the status bar)\n\n";{sbupd[];line@rd prompt}::/`;}
\d .
/ load the Amber library by ABSOLUTE path derived from this script's own location,
/ so it works no matter what the current working directory is.
dir::{$[#p:"/"/-1_"/"\x;p;"."]}`argv 1
{.[{. x^(`c$13)};,x;{}]}'0:dir,"/amber.k";
{.[{. x^(`c$13)};,x;{}]}'0:dir,"/fin.k";
/ v1.5 extended modules (order matters: std -> qsql -> temporal -> sys -> hdb -> ipc)
{.[{. x^(`c$13)};,x;{}]}'0:dir,"/std.k";
{.[{. x^(`c$13)};,x;{}]}'0:dir,"/qsql.k";
{.[{. x^(`c$13)};,x;{}]}'0:dir,"/temporal.k";
{.[{. x^(`c$13)};,x;{}]}'0:dir,"/sys.k";
{.[{. x^(`c$13)};,x;{}]}'0:dir,"/hdb.k";
{.[{. x^(`c$13)};,x;{}]}'0:dir,"/ipc.k";
/ 1.9.5: optional extensions. lib/ext.k does not exist in a stock checkout; an
/ installer (e.g. amber-ai's install.sh) creates it and has it load its own
/ .k files. Read WHOLE-FILE, because extension libraries carry multi-line
/ definitions, and fully trapped with diagnostics off: a missing or broken
/ lib/ext.k leaves the REPL exactly as it was.
{p:`diag 0;.[{. `c$1:x};,x;0];`diag p;}dir,"/lib/ext.k";
$[.[{sum;1};,`;{0}];;`0:"amber: WARNING - amber.k did not load (is it next to repl.k?)\n"]

$[(~"kw"~`argv 0)&~"--non-interactive"~*x;repl.run[];]

/
--------------------------------------------------------------------------------
\ menu
help: type \X to open a topic
# THE LANGUAGE
\0¦types, literals and nulls
\+¦verbs  : + - * % ! & | < > = ~ , ^ # _ $ ? @ .
\'¦adverbs  ' / \ ': /: \:
\:¦reading and writing files
\`¦built-in names  `k `j `csvr `t `env ...
\h¦the whole language on one screen
# THE Q LAYER
\q¦functions: maths, aggregates, moving windows, sets, strings
\j¦tables, keyed tables, joins and qSQL
\z¦dates, times, timestamps and spans
\m¦finance and HFT
# DATA AND SYSTEM
\o¦on disk: splayed and partitioned tables, CSV, JSON
\i¦talking to q: IPC, handlers, timers, logs
\g¦charts, and how tables print
\r¦speed: threads, peach and attributes
\e¦errors and debugging
\k¦every REPL command and editing key
# QUICK START
  gentq 1000
  select n:count i, vwap:sz wavg px by sym from trades
  plot trades`px
--------------------------------------------------------------------------------
\k
REPL commands and editing keys
# COMMANDS
\l file¦load a script
\d ns¦switch namespace (\d . is the root)
\t expr¦time it in ms, \t:n expr runs it n times (\ts works too)
\t n¦call .z.ts every n ms, \t 0 stops it
\p n¦listen for q clients on port n, on this machine (\p 0.0.0.0:n opens it to the network)
\v¦every global: name, type, shape, memory
\f¦the functions defined
\cd dir¦change directory (\cd alone shows it)
\grid mode¦table borders: clean, rounded, sharp, heavy
\clear¦clear the screen
\sb¦status bar on or off
\ast \trace \disasm¦look inside an expression (see \e)
\a¦the licence (AGPLv3)
\cmd¦anything else runs in the shell
\\¦exit
# EDITING
Up Down¦history, kept in ~/.amber_history
Ctrl-A Ctrl-E¦start or end of the line
Ctrl-W Ctrl-U Ctrl-K¦delete a word, to the start, to the end
PgUp PgDn¦scroll back through the session
Ctrl-L¦clear the screen
Ctrl-C¦abandon the line
Ctrl-D¦exit, on an empty line
{ ( [¦left open, the line goes on (...>) until it closes
# STARTING
./a¦this REPL, with every library loaded
amber¦the bare interpreter, no libraries
amber file.k¦run a script
amber file.q¦run a q script, the q library loaded first
amber file.q -p 5000¦serve q clients on port 5000 (options go after the script)
AMBER_THREADS=n¦threads to use, every core by default
AMBER_NO_EDIT=1¦plain line input, for screen readers and simple terminals
--------------------------------------------------------------------------------
\0
types, literals and nulls
~ @x gives the type: a capital for a list, small for an atom. type x gives q's number
# DATA
`I `i¦int¦0  -12  0N  1 2 3  01b (booleans are ints)
`F `f¦float¦1.5  1e9  0n  0w  -0w
`C `c¦char¦"a"  "abc"  0x6263  "\n\t\"\\"
`S `s¦symbol¦`a  `a`b  `"with space"
`A¦general list¦(1;`a;"bc")  ()
`M `m¦table, dict¦`a`b!1 2  +`a`b!(1 2;3 4)
# DATES AND TIMES (see \z)
`D `d¦date¦2026.10.15
`T `t¦time¦09:30:00.000
`N `n¦timestamp¦2026.10.15D09:30:00.000000000
`E `e¦timespan¦0D00:05:00.000000000
`H `h¦minute¦09:30
`J `j¦second¦09:30:15
`Y `y¦month¦2026.10m
# FUNCTIONS
`o¦lambda¦{x+y}  {[a;b]a*b}
`p¦projection¦1+  f[;2]
`q¦composition¦*|:
`r¦derived verb¦+/  2\
`u `v¦verb¦+:  +
`w¦adverb¦'  /:
`x¦external¦a C function
# NULLS
0N 0n `¦int, float, symbol
0Nd 0Nt 0Np 0Nn¦date, time, timestamp, timespan
0Nu 0Nv 0Nm¦minute, second, month
--------------------------------------------------------------------------------
\:
reading and writing files
~ x can be a file descriptor (an int), or a name: `"f.txt" "/path/f.txt" "host:port" ":port"
# VERBS
0:x¦read lines
x 0:y¦write lines
1:x¦read bytes
x 1:y¦write bytes
<s¦open¦fd:<`"f.txt"
>i¦close¦>fd
# THE Q WAY (see \o)
read0 `:f  read1 `:f¦lines, bytes
`:f 0: lines¦write lines
hdel `:f¦delete a file
--------------------------------------------------------------------------------
\+
verbs  : + - * % ! & | < > = ~ , ^ # _ $ ? @ .
~ the arguments: [c]har [i]nt [n]umber [s]ymbol [a]tom [d]ict [f]unction (F takes two) [x y z] anything
# ASSIGNING
var:y¦set¦a:1;a -> 1
var::y¦set a global¦a:1;{a::2}[];a -> 2
(v;..):y¦unpack¦(b;(c;d)):(2 3;4 5);c -> 4
:x¦return¦{:x+1;2}[3] -> 4
$[c;t;f]¦cond¦$[0;`a;`b] -> `b
o[..]¦recur¦{$[x<2;x;+/o'x-1 2]}9 -> 34
[..]¦in order¦[0;1;2;3] -> 3
# ARITHMETIC
::¦self¦(::)12 -> 12
+x¦flip¦+("ab";"cd") -> ("ac";"bd")
N+N¦add¦1 2+3 -> 4 5
-N¦negate¦- 1 2 -> -1 -2
N-N¦subtract¦1-2 3 -> -1 -2
*x¦first¦*`a`b -> `a
N*N¦multiply¦1 2*3 4 -> 3 8
%N¦sqrt¦%25 -> 5.0
N%N¦divide¦2 3%4 -> 0.5 0.75
i!I¦mod¦10!1234 567 -> 4 7
-i!I¦div¦-10!1234 567 -> 123 56
N&N¦min, and¦2&-1 3 -> -1 2
N|N¦max, or¦2|-1 3 -> 2 3
_n¦floor¦_12.34 -12.34 -> 12 -13
# LISTS
!i¦enum¦!3 -> 0 1 2
!I¦odometer¦!2 3 -> (0 0 0 1 1 1;0 1 2 0 1 2)
&I¦where¦&1 0 1 2 -> 0 2 3 3
|x¦reverse¦|"abc" -> "cba"
<X >X¦up, down¦<"abacus" -> 0 2 1 3 5 4
=X¦group¦="abba" -> "ab"!(0 3;1 2)
=i¦unit matrix¦=2 -> (1 0;0 1)
,x¦enlist¦,1 -> ,1
x,y¦join¦0,1 2 -> 0 1 2
#x¦count¦#"abc" -> 3
i#y¦take¦5#"abc" -> "abcab"
i_y¦drop¦2_"abcde" -> "cde"
I_y¦cut¦2 4_"abcde" -> ("cd";,"e")
f#y¦keep where f¦(3>)#1 5 2 -> 1 2
?X¦distinct¦?"abacus" -> "abcus"
X?y¦find (a miss is the count)¦"abcde"?"bfe" -> 1 5 4
i?x¦random¦3?10
@x¦type¦@1 -> `i
# COMPARING
N<N N>N N=N¦less, more, equal¦0 2<1 -> 1 0
x~y¦match¦2 3~2 3 -> 1
~x¦not¦~0 2 -> 1 0
^x¦null¦^1 0N -> 0 1
a^y¦fill¦1^0 0N -> 0 1
# TYPES AND TEXT
$x¦string¦$12 -> "12"
i$C¦pad¦5$"abc" -> "abc  "
s$y¦cast¦`c$97 -> "a"
_c¦lower case¦_"Ab" -> "ab"
x!y¦dict¦`a`b!1 2 -> `a`b!1 2
!d .d¦keys, values¦!`a`b!1 2 -> `a`b
.C¦eval¦."1+2" -> 3
x@y x.y¦apply¦{x+1}@2 -> 3
# AMEND, TRY
@[x;i;f]¦amend¦@[2 3;1;-:] -> 2 -3
@[x;i;F;y]¦amend¦@[2 3;0;+;4] -> 6 3
.[x;i;f]¦drill¦.[(1 2;3 4);1 0;-:] -> (1 2;-3 4)
.[f;args;h]¦try¦.[+;(1;`a);{"E"}] -> "E"
--------------------------------------------------------------------------------
\'
adverbs  ' / \ ': /: \:
# EACH
f'¦each¦#'("abc";3 4 5 6) -> 3 4
x F'¦each pair¦2 3#'"ab" -> ("aa";"bbb")
X'¦binary search¦1 3 5 7 9'8 9 0 -> 3 4 -1
x F/:¦each right¦1 2*/:3 4 -> (3 6;4 8)
x F\:¦each left¦1 2*\:3 4 -> (3 4;6 8)
F':¦each prior¦-':12 13 11 -> 12 1 -2
i':¦windows¦3':"abcde" -> ("abc";"bcd";"cde")
# OVER, SCAN
F/ F\¦fold, running¦+/1 2 3 -> 6
x F/¦with a start¦10+/1 2 3 -> 16
i f/ i f\¦n times¦5(2*)/1 -> 32
f f/¦while¦(100>)(2*)/1 -> 128
f/ f\¦until it settles¦(-2!)\100 -> 100 50 25 12 6 3 1 0
C/ C\¦join, split¦"/"\"a/b" -> (,"a";,"b")
I/ I\¦decode, encode¦2/1 1 0 1 -> 13
# TACIT
(f g)¦hook: x f g x¦(=;|:)"abba" -> 1 1 1 1
(f g h)¦fork: (f x) g (h x)¦(+/;%;#:)1 2 3 6 -> 3.0
--------------------------------------------------------------------------------
\`
built-in names
# DATA
`k x¦as k text¦`k("ab";2 3) -> "(\"ab\";2 3)"
`j?x¦parse JSON¦`j?"[1,2]" -> 1.0 2.0
`p x¦parse k
`csvr x¦a CSV file into a typed table
`ser x  `des b¦Amber's binary format, and back
`hex x¦hexadecimal¦`hex"ab" -> "6162"
# MATHS
`sin `cos `exp `ln `abs¦¦`ln 1 -> 0.0
`pri n¦primes below n¦`pri 20 -> 2 3 5 7 11 13 17 19
`prng[]¦random state: get it, set it, `prng 0 seeds from the clock
# SYSTEM
`t[]¦microseconds since the epoch
`argv[]¦the command line
`env[]¦the environment, as a dict
`x(cmd;in)¦run a program, feed it, get its output
`err"msg"¦raise an error
`diag 0¦plain one-line errors (`diag 1 the full reports)
`exit n¦exit
--------------------------------------------------------------------------------
\h
the whole language on one screen
| : set  return   $[c;t;f] cond   ' each, slide, binary search
| + add  flip                     / fold, join, decode, comment
| - subtract  negate              \ scan, split, encode
| * multiply  first               ': each prior   /: each right   \: each left
| % divide  sqrt
| ! mod, div, dict  enum, keys    0: lines   1: bytes
| & min, and  where               ?[a;i;b] splice
| | max, or  reverse              @[x;i;[f;]y] amend
| < less  up                      .[x;i;[f;]y] drill
| > more  down                    / on its own line: a comment block, to a \ line
| = equal  group
| ~ match  not
| , join  enlist
| ^ fill, without  null
| # take, reshape  count
| _ drop, cut  floor
| $ cast, pad  string
| ? find, random  distinct
| @ apply  type
| . apply  eval, values
~ a lambda has at most 8 arguments, 14 names counting locals, 512 bytes of code; calls nest 2048 deep
--------------------------------------------------------------------------------
\q
functions: maths, aggregates, moving windows, sets, strings
~ two-argument functions sit between their arguments as in q (10 mod 3), or take brackets: mod[10;3]
# MATHS
neg abs signum sqrt exp log¦as in q
floor ceiling round¦round[2;x] keeps 2 decimals
sin cos reciprocal¦
mod div xbar xlog¦remainder, quotient, bucket, log in a base
# AGGREGATES
sum prd min max avg med¦a list to one value
var dev svar sdev¦variance, deviation (s: the sample ones)
wsum wavg¦weighted: w wavg x
cov scov cor¦covariance, correlation
count first last all any¦
# RUNNING AND MOVING
sums prds mins maxs¦running total, product, low, high
deltas ratios differ¦against the item before
prev next xprev¦shift by one, or by n
fills¦nulls take the last value before them
msum mavg mmin mmax mcount¦over the last n: 3 msum x
mdev mvar mprd¦
ema¦exponential moving average: ema[a;x]
# ORDER AND SETS
asc desc iasc idesc¦sorted, or the order that sorts
rank xrank¦place in sort order, or n buckets
bin binr¦last at or below, first at or above, in a sorted list
in except inter union distinct¦
cross raze sublist cut rotate¦
within¦x within (lo;hi)
# EACH
each peach prior¦f each x, f peach x on every core, f prior x
over scan¦f over x, f scan x
# STRINGS
lower upper trim ltrim rtrim¦
ss ssr like¦find, replace, match a pattern (* ? [a-z])
sv vs string¦join, split, to text
# EXAMPLES
  10 mod 3 -> 1
  3 msum 1 2 3 4 -> 1 3 6 9
  2 3 wavg 10 20 -> 16.0
  fills 1 0N 0N 4 -> 1 1 1 4
  1 2 3 4 bin 2 -> 1
  "abracadabra" ss "ra" -> 2 9
--------------------------------------------------------------------------------
\j
tables, keyed tables, joins and qSQL
# BUILD AND LOOK
([]a:1 2;b:`x`y)¦a table, ([k:..]v:..) a keyed one
flip `a`b!(1 2;3 4)¦a table from a dict of columns
show meta cols count¦print it, its types, its columns, its rows
t`a  t 0¦a column, a row as a dict
xkey unkey keys¦`k xkey t sets the key
xcol xcols xasc xdesc¦rename, reorder, sort by columns
insert upsert¦add rows (`t insert x appends in place)
# QSQL, AS IN Q
select¦select [cols] [by grp] from t [where cond]
select[n]¦select[5;>px] from t sorts then takes 5
exec¦a column, or a dict of them
update¦add or change columns (by works too)
delete¦drop rows (where) or columns
fby¦select from t where px=(max;px) fby sym
xbar¦select sum sz by 10 xbar time from t
~ here, in \l files and in .q scripts this is plain q; a .k script run directly writes sel"select ..."
# JOINS
lj ij¦left, inner, on a keyed table
ej pj uj¦equi, plus, union
aj aj0¦as-of: aj[`sym`time;trades;quotes]
wj wj1¦window: wj[w;`sym`time;t;(q;(max;`bid))]
# FUNCTIONAL FORMS
qwhere qselect qby¦the same as functions, for inside lambdas
xgroup ungroup¦nest the rows by key, and back
--------------------------------------------------------------------------------
\z
dates, times, timestamps and spans
# LITERALS
2026.10.15¦date
09:30:00.000¦time of day
2026.10.15D09:30:00.000000000¦timestamp (to the ns)
0D00:05:00.000000000¦timespan
09:30  09:30:15¦minute, second
2026.10m¦month
0Nd 0Nt 0Np 0Nn 0Nu 0Nv 0Nm¦the nulls
# ARITHMETIC
  2026.10.15+1 -> 2026.10.16
  2026.10.16-2026.10.15 -> 1
  2026.10.15D09:30:00.000000000+0D00:05:00.000000000 -> 2026.10.15D09:35:00.000000000
  2026.10.15+09:30:00.000 -> 2026.10.15D09:30:00.000000000
# PARTS AND CASTS
year month day dow¦of a date
hh mm sec milli¦of a time (sec, since ss is string search)
minute second¦since midnight
`date$ `time$ `minute$¦cast, as q
"D"$ "T"$ "P"$ "N"$¦read text, bad text gives the null
.z.p .z.d .z.t¦now: a timestamp, the date, the time
# BARS
0D00:05 xbar t¦5-minute bars of a timestamp
minbar[5;t]¦5-minute bars of a time
~ select last px by 0D00:01 xbar time from t gives 1-minute closes
--------------------------------------------------------------------------------
\m
finance and HFT (fin.k)
# TEST DATA
gentq n¦tables trades and quotes, n trades: random-walk prices, real spreads
genopt n¦a table options: n contracts priced with Black-Scholes, with greeks
# QUOTES
mid spread spreadbps¦of a quote table
micro imbal¦microprice, book imbalance
# TRADES
taq[t;q]¦each trade with the quote it hit
tsign signedvol¦Lee-Ready side, signed volume
effspread notional¦2|px-mid|, px*sz
# PRICE SERIES
ret logret rvol¦returns, log returns, realised volatility
vwap twap¦volume and time weighted price
movavg movsum movmax movmin¦moving windows
ema rollstd¦exponential average, rolling deviation
# TABLES
bars[w;t]¦w-minute OHLCV bars
symstats t¦a summary per symbol
bysym t  symrows[t;g;s]¦each symbol's rows, found at once
sortcol partcol groupcol¦mark a column `s, `p or `g
pt t¦print with times as HH:MM:SS.mmm
--------------------------------------------------------------------------------
\o
on disk: splayed and partitioned tables, CSV, JSON
# TABLES ON DISK (q's layout, memory-mapped)
`:db/t/ set t¦a file per column, symbols in db/sym
t:get `:db/t¦map it back, pages load as they're read
`:f set x  get `:f¦one value in one file
.Q.dpft[d;p;f;t]¦one partition: .Q.dpft[`:db;2026.10.15;`sym;`t]
\l db¦open a partitioned database (loaddb "db" too)
select .. where date=d¦reads only that day, and only the columns named
~ Amber reads the databases q writes, and q reads Amber's
~ a write that dies (kill -9, a power cut, a full disk) leaves each table whole, old or new
# FILES
`csvr "f.csv"¦CSV straight into a typed table
read0 `:f  read1 `:f¦lines, bytes
`:f 0: lines¦write lines
hdel `:f¦delete a file
system "ls"¦run a shell command, its output as lines
# FORMATS
.j.j .j.k¦to JSON, from JSON
-8!x  -9!b¦q's bytes, as on the wire
`ser x  `des b¦Amber's own binary format
--------------------------------------------------------------------------------
\i
talking to q: IPC, handlers, timers, logs
# CONNECTING
\p 5000¦listen on this machine (or start with amber file.q -p 5000)
\p 0.0.0.0:5000¦listen on the network too, which is what q's \p 5000 does
h:hopen 5000¦connect: a port, or `:host:5000
h"1+1"¦run it there and wait for the answer
h(`f;1;2)¦call a function there
neg[h]"x:1"¦send it, don't wait
hclose h¦close the connection
# HANDLERS, AS IN Q
.z.pg .z.ps¦answer a sync, an async message
.z.po .z.pc¦a client connects, disconnects
.z.w¦the handle being served
# WHO GETS IN
.z.pw¦.z.pw:{[u;p] ...} gets the user and password and says yes or no
-u file  -U file¦user:password lines a client has to match (md5 passwords too)
~ a no is 'access at the client. A port open to the network with neither prints a warning
# TIMERS AND LOGS
\t 1000¦call .z.ts every second, \t 0 stops it
h:hopen `:log¦a log: h enlist(`upd;`t;x) appends to it
-11!`:log¦replay it
# CONSOLE
1"text"  -1"line"¦to stdout, 2 and -2 to stderr
~ tested against kdb+ itself, both ways round, with a whole tickerplant
--------------------------------------------------------------------------------
\g
charts, and how tables print
# CHARTS
plot v¦a line, plot (a;b;c) a line per column
plots d¦several series with a legend
xyplot (xs;ys)¦y against a real x axis
scatter (xs;ys)¦points
hist v¦a histogram, hist (v;bins)
barh d  heat d¦bars, or a matrix as coloured cells
candle t¦OHLC candlesticks
spark v¦a chart in one line of text
tplot dplot pplot¦xyplot over a time, date or timestamp axis
chart d¦any of them, from a dict of options
# TABLES
show t¦print it as a grid
\grid mode¦borders: clean, rounded, sharp, heavy
CROWS:20¦rows shown before the ..
PREC:7¦decimals shown, PREC:0N shows every one
COLOR:0¦no colour
--------------------------------------------------------------------------------
\r
speed: threads, peach and attributes
# THREADS
AMBER_THREADS=n¦threads to use, every core by default, 1 for none
f peach x¦f over x on every core, peach[f;x] too
~ big sorts, finds, joins, group sums and float sums split over the threads by themselves
# ATTRIBUTES
`s#x¦sorted: ? and in use binary search
`u#x¦unique: lookups are hashed
`p#x¦parted: each value's rows sit together
`g#x¦grouped: an index of each value's rows
attr x¦which one x has
~ asc and xasc set `s themselves, changing the list drops it
# TIMING
\t:n expr¦ms for n runs
\trace expr¦where the time went: parse, compile, run, print
~ arithmetic inside a lambda runs fused, in one pass over memory
--------------------------------------------------------------------------------
\e
errors and debugging
# LOOKING INSIDE
\ast expr¦its parse tree, nothing runs
\disasm expr¦the bytecode it compiles to, nothing runs
\trace expr¦run it, with timings and scratch memory
\v¦every global: name, type, shape, memory
# CATCHING
.[f;args;h]¦run f, on an error h gets the message
@[f;x;h]¦the same for one argument
`err"msg"¦raise one
`diag 0¦plain one-line errors, `diag 1 the full reports
# THE CODES
E0101 value¦a name that isn't defined
E0102 type¦the wrong type for this operation
E0103 length¦lists of different lengths
E0104 domain¦a value outside what's allowed
E0105 parse¦a bracket or a string left open
E0106 index¦an index past the end
E0107 rank¦the wrong number of arguments
E0108 limit¦too big, too deep, or out of memory
E0109 io¦a file or a socket failed
E0110 stack¦calls nested more than 2048 deep
E0111 compile¦it parses but can't compile
E0112 nyi¦not done for these types yet
E0100¦any other error
\
