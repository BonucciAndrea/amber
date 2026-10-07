/ ============================================================================
/ tests/test_disk.k  -  the on-disk layer: column files, splayed and partitioned tables.
/ GNU AGPLv3 - see LICENSE and NOTICE.
/ ----------------------------------------------------------------------------
/ `wcol / `rcol (src/dsk.c) write and map one column file; hdb.k builds splayed tables, the sym file, set/get,
/ Q.dpft and loaddb on them, and qsql.k hands a partitioned table to hdb.k's qpsel. Everything is written under
/ a fresh directory in /tmp and removed at the end.
/ Run it from anywhere:  ./amber tests/test_disk.k
/ ============================================================================
DIR:{$[#p:"/"/-1_"/"\x;p;"."]}`argv 1
LOAD:{{. x^(`c$13)}'0:x;}
LOAD DIR,"/../amber.k"
LOAD DIR,"/../std.k"
LOAD DIR,"/../qsql.k"
LOAD DIR,"/../temporal.k"
LOAD DIR,"/../sys.k"
LOAD DIR,"/../hdb.k"
LOAD DIR,"/harness.k"
T:"/tmp/amber_disk_",$`t[]   / the microsecond clock: the seed is fixed, so a random name was the same in every run
rm:{`x(("/bin/rm";"-rf";x);"");}
rm T

/ ---- one column file: every kind back as it went in --------------------------
rt:{[tag;v]f:T,"/c/",$tag;`wcol(f;v;-1;0);r:`rcol f;t[tag;(v~r)&(@v)~@r;1b]}
rt[`colFloat;1000?1.0]
rt[`colLong;-500+1000?1000]
rt[`colBig;1000?2000000000]
rt[`colShort;1000?30000]
rt[`colByte;1000?100]
rt[`colBool;1000?2=1000?2]
rt[`colChar;"hello world"]
rt[`colEmpty;0#0.0]
/ an attribute byte (7) that names none of the four is 'format: it was copied in, and `at read "\0supg" past its end
x:`wcol(T,"/c/att";1 2 3;-1;0);AB:1:T,"/c/att"
ra:{(T,"/c/att2") 1:(7#AB),(`c$x),8_AB;.[{`at@`rcol x};,T,"/c/att2";{*"\n"\x}]}
t[`colAttrByte;ra'5 200 4;("'format";"'format";`g)]
rt[`colNested;(1 2;"ab";`x`y)]
rt[`colAtom;42]
rt[`colDict;`a`b!1 2]
rt[`colTable;([]a:1 2 3;b:`x`y`z)]
rt[`colSyms;`a`bb`cccccc]
rt[`colNulls;0N 1 0N]
rt[`colFloats;0n 1.5 -0w 0w -0.0]
rt[`colDates;2026.01.01 2026.01.02 2026.01.03]
v:asc 1000?1000;`wcol(T,"/c/s";v;-1;0);t[`colKeepsSorted;attr `rcol T,"/c/s";`s]
dom:`ibm`msft`aapl;sy:100?dom;x:`wcol(T,"/c/e";dom?sy;#dom;0)
t[`colEnum;`rcol(T,"/c/e";dom);sy]
t[`colEnumRaw;`rcol T,"/c/e";dom?sy]
te[`colEnumShort;"`rcol(T,\"/c/e\";2#dom)"]
big:1000000?1.0;`wcol(T,"/c/big";big;-1;0);r:`rcol T,"/c/big"
t[`colMapped;r;big]
r[0]:-1.0
t[`colWritePrivate;(`rcol T,"/c/big")0;big 0]
te[`colMissing;"`rcol T,\"/c/nope\""]
(T,"/c/junk") 1:"not a column file at all, just some text that is long enough"
te[`colNotOurs;"`rcol T,\"/c/junk\""]
/ a file that opens but is shorter than the header page is 'format, as other damage; 'io is a file that cannot be
/ opened or read, and a directory (#94 Q14)
x:`wcol(T,"/c/ff";1 2 3;-1;0);(T,"/c/gg") 1:4000#1:T,"/c/ff";(T,"/c/hh") 1:""
rc:{.[{`rcol x};,T,"/c/",x;{*"\n"\x}]}
t[`colShort;rc'("gg";"hh";"junk";"nope";"");("'format";"'format";"'format";"'io";"'io")]
t[`fszMissing;`fsz T,"/c/nope";-1]
t[`fszDir;`fsz T,"/c";-2]
t[`ldir;(`ldir T)1;()]

/ ---- values, splayed tables, the sym file ------------------------------------
dt:([]sym:`a`b`c`a;px:1 2 3 4.0;n:10 20 30 40;s:("ab";"c";"";"def"))
t[`dsetDget;dget dset[T,"/one";dt];dt]
x:splay[T,"/db/t";dt]
t[`splayDload;dload T,"/db/t";dt]
t[`symFile;dget T,"/db/sym";`a`b`c]
x:splay[T,"/db/t2";([]sym:`d`a`e;v:1 2 3)]
t[`symGrows;dget T,"/db/sym";`a`b`c`d`e]
t[`oldStillReads;dload T,"/db/t";dt]
/ a set that fails partway leaves the old table and the sym file: every file is written before any is renamed
/ (#94 Q12). A directory where s's temporary file goes makes its write fail, after a's and the sym file's
h:`$":",T,"/db/f/";x:h set ([]a:1 2;s:`x`y);x:mkd T,"/db/f/s.tmp";z:dget T,"/db/sym"
x:.[set;(h;([]a:!5;s:5#`zp`zq));{*"\n"\x}]
t[`splayFailOld;(x;.[get;,h;{*"\n"\x}];z~dget T,"/db/sym";`ldir T,"/db/f");("'io";([]a:1 2;s:`x`y);1;(,"s.tmp";(".d";,"a";,"s")))]
wa:{x:`wcol((T,"/c/w1";1 2;-1;0);(T,"/c/w2";`k;-1;0));(x;`rcol T,"/c/w1";`rcol T,"/c/w2")}
tv[`wcolAll;"wa[]";((T,"/c/w1";T,"/c/w2");1 2;`k)]   / a list of quadruples: all written, then renamed
wd:{x:.[{`wcol((T,"/c/w3";1 2;-1;0);(T,"/c/w3";3 4;-1;0))};,0;{*"\n"\x}];(x;`fsz T,"/c/w3";`fsz T,"/c/w3.tmp")}
tv[`wcolDup;"wd[]";("'domain";-1;-1)]   / a path named twice: nothing is written
/ set of a keyed table to a splay is 'type, as q's set (it wrote the unkeyed table), and so is Q.dpft of one (q: 'nyi); nothing
/ is written. One file keeps a keyed table whole
kt:([k:1 2]v:3 4)
t[`splayKeyed;(.[set;(`$":",T,"/db/k/";kt);{*"\n"\x}];`fsz T,"/db/k";.[Q.dpft;(T,"/hdbk";2026.01.01;`v;`kt);{*"\n"\x}];`fsz T,"/hdbk");("'type";-1;"'type";-1)]
x:splay[T,"/db/k2";kt]
t[`setKeyedFile;(get(`$":",T,"/kf") set kt;dload T,"/db/k2");(kt;([]k:1 2;v:3 4))]   / Amber's splay still writes it unkeyed
x:mkd T,"/old";(T,"/old/x") 1:`k 1 2 3
t[`oldTextFile;dget T,"/old/x";1 2 3]
h:`$":",T,"/db/u/"
x:h set dt
t[`setSplays;get`$":",T,"/db/u";dt]
x:(`$":",T,"/f") set 1 2 3
t[`setFile;get`$":",T,"/f";1 2 3]
x:`zzdisk set 7
t[`setGlobal;get`zzdisk;7]
/ get reads only the binary format: a text file or table is 'format and never run (#94 Q11). dget and dload still
/ read the files Amber wrote before 2.7, by running them
x:mkd T,"/old/tt";(T,"/old/tt/.d") 1:`k@,`a;(T,"/old/tt/a") 1:"zzrun:1;5 6";(T,"/old/run") 1:"zzrun:42;1+2"
ge:{.[get;,`$":",T,x;{*"\n"\x}]}
t[`getText;(ge"/old/run";ge"/old/tt";ge"/old/nope";.[.:;,`zzrun;{"unset"}]);("'format";"'format";"'io";"unset")]
x:dget T,"/old/run";y:dload T,"/old/tt"
t[`getTextDget;(x;y);(3;+(,`a)!,5 6)]

/ ---- 0: and 1: of a path ---------------------------------------------------------
ER:{[f;a].[f;a;{*"\n"\x}]};N0:`c$0;x:mkd T,"/io";P0:T,"/io/ab";P0 1:"kept"
/ #94 Q16: a path holding a NUL is 'domain (open() took the name before it: "ab\0cd" 1: y wrote ab)
t[`pathNul;(1:P0;ER[{1:x};,P0,N0,"zz"];ER[{0:x};,P0,N0];ER[{x 1:"nul"};,P0,N0,"cd"]);("kept";"'domain";"'domain";"'domain")]   / items run from the right
t[`pathNulVerbs;(1:P0;ER[{`csvr x};,P0,N0];ER[{`fsz x};,P0,N0];ER[{`ldir x};,T,N0];ER[{`rcol x};,P0,N0];ER[{`wcol(x;1 2;-1;0)};,P0,N0,"cd"];ER[{`x(x;"")};,("/bin/echo";"a",N0)]);(,"kept"),6#,"'domain"]
/ . of a string reads it as a C string, so \l and \cd (and a shell command) of a path holding a NUL took the name before it
/ (that line is refused before it runs, as the lines before it run; a \ that starts a line inside a string is no command,
/ and code holding a NUL is cut there, as before)
LD:0;(T,"/io/ld.k")0:,"LD:1"
t[`pathNulText;(LD;ER[.:;,"\\l ",T,"/io/ld.k",N0,"zz"];ER[.:;,"LD:2\n\\l ",T,"/io/ld.k",N0];ER[.:;,"\\cd ",T,N0];ER[.:;,"\"a\n\\tb\"",N0]);(2;"'domain";"'domain";"'domain";"a\n\tb")]
/ a script's \l and \cd lines and a line on stdin reach the parser the same way: the script stops with 'domain at that line
SA:0;LD:0;(T,"/io/s1.k")0:("SA:1";"\\l ",T,"/io/ld.k",N0,"zz");(T,"/io/s2.k")0:("SA:2";"\\cd ",T,N0,"zz")
t[`pathNulScript;(SA;LD;ER[.:;,"\\l ",T,"/io/s1.k"];ER[.:;,"\\l ",T,"/io/s2.k"]);(1;0;"";"")]   / a script's error is printed where it stops, so the trap sees ""
t[`pathNulStdin;`x(("/bin/sh";"-c";"exec \"$0\" 2>/dev/null";`argv 0);"LD:0\n\\l ",T,"/io/ld.k",N0,"zz\nLD\n");"0\n"]
/ x 0: y and x 1: y whose path does not open ('domain for a NUL, 'io for a missing directory) free y: \m reported it
/ (in a process of its own, as \m reports the running script's own text)
t[`pathNulFree;`x(("/bin/sh";"-c";"exec \"$0\"";`argv 0);"y:\"abc\";z:(\"ab\";\"cd\");.[{\"ab\\000cd\" 1: x};,y;{x}];.[{\"ab\\000cd\" 0: x};,z;{x}];.[{\"/nonexistent/dir/f\" 1: x};,y;{x}];.[{\"/nonexistent/dir/f\" 0: x};,z;{x}];\n\\m\n");""]

/ ---- a partitioned database ------------------------------------------------
trade:([]sym:`ibm`msft`ibm`aapl;px:10 20 11 30.0;sz:100 200 300 400)
x:{[d]Q.dpft[T,"/hdb";d;`sym;`trade]}'2026.01.02 2026.01.03 2026.01.05
t[`loaddb;loaddb T,"/hdb";,`trade]
t[`ptCols;cols sel"select from trade";`date`sym`px`sz]
t[`ptRows;#sel"select from trade";12]
t[`ptPrune;#sel"select from trade where date=2026.01.03";4]
t[`ptPruneWithin;#sel"select from trade where date within 2026.01.02 2026.01.03";8]
t[`ptByDate;(+value sel"select sum sz by date from trade")`sz;1000 1000 1000]
t[`ptByDateSym;(+value sel"select sum sz by date,sym from trade where date>2026.01.02")`sz;400 400 200 400 400 200]
t[`ptBySym;(+value sel"select sum sz by sym from trade where date within 2026.01.02 2026.01.03")`sz;800 800 400]
t[`ptExec;exq"exec px from trade where date=2026.01.05,sym=`ibm";10 11.0]
te[`ptUpdate;"upd\"update px:0 from trade\""]
te[`ptDelete;"del\"delete from trade where px>0\""]
t[`ptab;#ptab[`trade;()];12]
t[`ptabSome;#ptab[`trade;,2026.01.03];4]
t[`ptSortedSym;attr (sel"select from trade where date=2026.01.02")`sym;`p]
trade2:([]px:1.5 2.5;sym:`z`y;sz:1 2);x:Q.dpft[T,"/hdb2";2026.01.03;`sym;`trade2]
t[`dpftOrder;(`rcol T,"/hdb2/2026.01.03/trade2/.d";`rcol T,"/hdb2/2026.01.03/trade2/px");(`sym`px`sz;2.5 1.5)]   / the parted column first, as q's .Q.dpft
x:loaddb T,"/hdb"
t[`reload;#sel"select from trade";12]

/ ---- \l of a script that loads itself ---------------------------------------
/ each level nested the loader on the C stack with no limit and crashed (SIGSEGV); now 'stack after 512 levels, as q's
/ 'stack after 500. Run in a process of its own, so a crash cannot stop the suite: the child prints the trapped error
/ (printed already, so "") and how many levels ran (511 under the script's own load)
x:mkd T,"/l";(T,"/l/rec.k") 0:("n+:1";"\\l ",T,"/l/rec.k")
(T,"/l/top.k") 0:("n:0";"r:.[.:;,\"\\\\l ",T,"/l/rec.k\";{x}]";"`0:(`k r),\"/\",$n")
t[`loadSelf;`x((`argv[]0;T,"/l/top.k");"");"\"\"/511\n"]
(T,"/l/cnd.k") 0:("m+:1";"$[m<300;. \"\\\\l ",T,"/l/cnd.k\";]")
m:0;x:.[.:;,"\\l ",T,"/l/cnd.k";{x}]
t[`loadNested300;m;300]   / a load that stops is not limited before 512

/ ---- 1: past 32 bits: file sizes and offsets are 64-bit ----------------------
/ they were cut to 32 bits: an offset of 2^32+k read at k, a negative one a page below 2^32 mapped far past the end
/ (SIGBUS on Linux, so in a process of its own), and a file of 2 to 4 GiB read as ""
x:mkd T,"/big";F1:T,"/big/ff";F1 1:"hello world"
rd:{.[{1:x};,x;{*"\n"\x}]}
t[`readOffset4G;rd(F1;4294967297 3);""]   / past the end, as 1:(F1;20 3); it was "ell"
t[`readOffsetNeg;rd'((F1;-1 3);(F1;-9223372036854775807 1));("'io";"'io")]   / the second was ,"e"
t[`readOffsetNegPage;`x(,`argv[]0;"`0:.[{1:x};,(\"",F1,"\";-4096 3);{*\"\\n\"\\x}]\n");"'io\n"]
/ big files, sparse: only where the file system keeps them so (a 64 MiB probe takes no blocks), or they would be written
dd:{[f;n;s]`x(("/bin/dd";"of=",f;"bs=1";"seek=",$n;$[#s;"conv=notrunc";"count=0"]);s);f}
SP:64>. *"\t"\`x(("/usr/bin/du";"-sk";dd[T,"/big/probe";67108864;""]);"")
S2:$[SP;dd[T,"/big/s2g";2147483648;""];""];S4:$[SP;dd[dd[T,"/big/s4g";4294967301;""];4294967296;"XY"];""]
t[`readSize2G;$[SP;#1:S2;2147483648];2147483648]   / it was 0
t[`readSize4G;$[SP;rd S4;"'limit"];"'limit"]   / a vector's count is 32 bits; it was 5
t[`readOffset4GBig;$[SP;rd(S4;4294967296 2);"XY"];"XY"]   / it read at 0
rm T,"/big"

/ ---- x 1: y over the file y was read from: written from its start, not truncated first (Linux: y's pages, a map of
/ the file, were zeros by the time they were copied, and the file was all NULs; "bb" 1: get`:bb was SIGBUS) ----
/ WSL 1 is a Windows kernel: it will not cut or rewrite a file while a map of it is open, so these writes are 'io there
/ (the file stays as it was); they run where the kernel is Linux. Trapped, so a refusal fails a test, not the suite
LK:~like[.[{`x(("/usr/bin/uname";"-r");"")};,0;{""}];"*Microsoft*"];wo:{[f;y]$[LK;.[{x 1:y};(f;y);{0}];0];}
W:T,"/w";W 1:"original";wo[W;1:W]
t[`writeOwnRead;1:W;"original"]
v:1:W;wo[W;v]
t[`writeOwnReadKept;(v;1:W);2#,"original"]
B:T,"/bb";(`$":",B)set 5000#"abc";$[LK;wo[B;get`$":",B];B 1:5000#"abc"]
t[`writeOwnGet;1:B;5000#"abc"]

rm T
hexpect[78]
hreport"AMBER ON-DISK"
$[FAILN;`exit 1;]
