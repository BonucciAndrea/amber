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
T:"/tmp/amber_disk_",$*1?999999999
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
x:mkd T,"/old";(T,"/old/x") 1:`k 1 2 3
t[`oldTextFile;dget T,"/old/x";1 2 3]
h:`$":",T,"/db/u/"
x:h set dt
t[`setSplays;get`$":",T,"/db/u";dt]
x:(`$":",T,"/f") set 1 2 3
t[`setFile;get`$":",T,"/f";1 2 3]
x:`zzdisk set 7
t[`setGlobal;get`zzdisk;7]

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
x:loaddb T,"/hdb"
t[`reload;#sel"select from trade";12]

rm T
hexpect[51]
hreport"AMBER ON-DISK"
$[FAILN;`exit 1;]
