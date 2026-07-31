/ ============================================================================
/ test-ext.k  -  tests for the extended modules (std qsql temporal sys hdb ipc).
/ run:  ./amber test-ext.k
/ ============================================================================
\l amber.k
\l fin.k
\l std.k
\l qsql.k
\l temporal.k
\l sys.k
\l hdb.k
\l ipc.k
r:()
t:{r,:,($x;$[y~z;"ok";"FAIL got<",(`k y),"> exp<",(`k z),">"])}

/ ---- moving / window aggregates ----
t[`mcount;mcount[3;1 2 3 4 5];1 2 3 3 3]
t[`msum;msum[3;1 2 3 4 5];1 3 6 9 12.0]
t[`mavg;mavg[3;1 2 3 4 5];1 1.5 2 3 4.0]
t[`mmin;mmin[3;5 3 4 1 2];5 3 3 1 1]
t[`mmax;mmax[3;1 3 2 5 4];1 3 3 5 5]
t[`mdev0;mdev[2;2 2 2 2];0 0 0 0.0]
t[`mmu;mmu[(1 2;3 4);(5 6;7 8)];(19 22;43 50)]
t[`dot;dot[1 2 3;4 5 6];32]

/ ---- casting / parse / serialize ----
t[`parseEval;eval"2*3+4";14]
t[`serDeser;deser ser(1 2;`a`b);(1 2;`a`b)]
t[`castFloat;cast[`float;1 2 3];1 2 3.0]
t[`castLong;long 1.9 2.9;1 2]
t[`peach;peach[{x*x};1 2 3 4];1 4 9 16]

/ ---- qSQL ----
tb:([]sym:`a`b`a`b`a;px:10 20 11 21 12.0;sz:100 200 300 100 200)
t[`selWhere;(sel"select px from tb where sym=`a")`px;10 11 12.0]
t[`selBy;(unkey sel"select v:sum sz by sym from tb")`v;600 300]
t[`exec;exq"exec px from tb where sym=`b";20 21.0]
t[`updNoBy;(upd"update px:px+100 from tb where sym=`b")`px;10 120 11 121 12.0]
t[`updBy;(upd"update mp:max px by sym from tb")`mp;12 21 12 21 12.0]
t[`delRows;#del"delete from tb where px<15";2]
t[`delCols;cols del"delete sz from tb";`sym`px]

/ ---- temporal ----
t[`ymd0;ymd2d[2000;1;1];0]
t[`ymd2024;ymd2d[2024;1;15];8780]
t[`leap;ymd2d[2020;2;29];7364]
t[`d2ymd;d2ymd 8780;2024 1 15]
t[`dstr;dstr 8780;"2024.01.15"]
t[`pdate;pdate"2024.01.15";8780]
t[`dow0;dow 0;`Sat]
t[`pstr;pstr tstamp[8780;hms[9;30;0]];"2024.01.15D09:30:00.000"]

/ ---- system namespaces ----
t[`zdToday;`M=@meta([]a:1 2);0]           / (placeholder true/false shape check)
t[`Qf;Q.f[2;3.14159];"3.14"]
t[`Qdd;Q.dd[`a;`b];`$"a.b"]
t[`jjDict;j.j`a`b!(1;"x");"{\"a\":1,\"b\":\"x\"}"]
t[`jkParse;(j.k"[1,2,3]")~1 2 3.0;1]

/ ---- on-disk (hdb) ----
dt:([]sym:`a`b`c;px:1 2 3.0)
t[`dsetGet;dt~dget dset["/tmp/amber_ext_one";dt];1]
x:splay["/tmp/amber_ext_sp";dt]
t[`splayLoad;dt~dload"/tmp/amber_ext_sp";1]
x:partsave["/tmp/amber_ext_pdb";dt;`sym]
t[`partRows;#partload"/tmp/amber_ext_pdb";3]

/ ---- in-process tickerplant ----
x:u.def[`trd;([]sym:0#`;px:0#0.0)]
TC::0
x:u.sub[`trd;{[nm;d]TC::TC+#d}]
x:u.pub[`trd;([]sym:`a`b;px:10 20.0)]
x:u.pub[`trd;([]sym:`c`d`e;px:30 40 50.0)]
t[`tickRows;#u.get`trd;5]
t[`tickCb;TC;5]

/ ---- C-kernel window join (wj): standard reducers run in C ----
trade:+`sym`time`px!(`a`b`a;3 4 9;100 200 300)
quote:+`sym`time`bid`ask!(`a`a`b`a;1 5 2 8;10 11 20 12;15 16 25 18)
t[`wjClast;&/(+wj[(trade[`time]-2;trade`time);`sym`time;trade;quote;,(`b;last;`bid)])[`b]=10 20 12;1b]
t[`wjCmax; &/(+wj[(trade[`time]-2;trade`time);`sym`time;trade;quote;,(`m;max;`ask)])[`m]=15 25 18;1b]
t[`wjCmin; &/(+wj[(trade[`time]-2;trade`time);`sym`time;trade;quote;,(`m;min;`bid)])[`m]=10 20 12;1b]
t[`wjCsum; &/(+wj[(trade[`time]-2;trade`time);`sym`time;trade;quote;,(`s;sum;`bid)])[`s]=10 20 12;1b]
t[`wjCcnt; &/(+wj[(trade[`time]-2;trade`time);`sym`time;trade;quote;,(`n;count;`bid)])[`n]=1 1 1;1b]
t[`wjCavg; &/1e-9>abs(+wj[(trade[`time]-2;trade`time);`sym`time;trade;quote;,(`a;avg;`bid)])[`a]-10 20 12;1b]
t[`wjCnogrp;&/(+wj[(trade[`time]-2;trade`time);,`time;trade;quote;,(`m;max;`bid)])[`m]=20 20 12;1b]
/ C wj agrees with interpreted wjK on random data (bit-exact reducers)
gentq 400
wAGG:((`hi;max;`ask);(`lo;min;`bid);(`nq;count;`bid))
wCr:+wj[(trades[`time]-60000;trades`time);`sym`time;trades;quotes;wAGG]
wKr:+wjK[(trades[`time]-60000;trades`time);`sym`time;trades;quotes;wAGG]
t[`wjCK;&/{[a;b;c]&/(a[c]=b c)|(^a[c])&^b c}[wCr;wKr]'`hi`lo`nq;1b]
/ ---- C-kernel EMA ----
t[`ema0;ema[0.5;1 2 3 4];1.0 1.5 2.25 3.125]
t[`ema1;ema[1.0;1 2 3];1.0 2 3]
t[`emaEmpty;ema[0.5;0#0.0];0#0.0]
/ ---- temporal arithmetic ----
t[`dateRT;pdate dstr ymd2d[2026;7;30];ymd2d[2026;7;30]]
t[`dow;dow ymd2d[2000;1;1];`Sat]
t[`year;year ymd2d[2026;7;30];2026]
t[`timeAdd;stime[ptime["10:00:00.000"]+ptime["00:00:05.000"]];"10:00:05.000"]
t[`tsRT;tsdate tstamp[ymd2d[2026;7;30];ptime"09:30:00.000"];ymd2d[2026;7;30]]
/ ---- native temporal types: literals, arithmetic, casts, accessors ----
t[`litDateT;type 2026.07.30;`d]
t[`litTimeT;type 10:00:05.000;`t]
t[`litTsT;type 2026.07.30D09:30:00.000;`n]
t[`timeAddLit;`i$10:00:00.000+00:00:05.000;36005000]
t[`dateDiff;2026.07.30-2026.07.25;5]
t[`datePlus;`i$2026.07.30+5;9712]
t[`dateCmp;$[2026.07.25<2026.07.30;1;0];1]
t[`tsArith;`i$2026.07.30D09:30:00.000+1000000000;838719001000000000]
t[`castD;type "D"$"2026.07.30";`d]
t[`castT;`i$"T"$"10:00:05.000";36005000]
t[`yearA;year 2026.07.30;2026]
t[`monthA;month 2026.07.30;7]
t[`dayA;day 2026.07.30;30]
t[`dowA;dow 2026.07.30;`Thu]
t[`thhA;thh 10:35:05.000;10]
t[`tmmA;tmm 10:35:05.000;35]

/ ---- terminal charting (braille canvas + candlesticks) ----
t[`plotRows;+/"\n"=`plt (1.0*!30;40;10);10]              / H=10 -> 10 rows
t[`plotNonEmpty;0<#`plt 1.0*!20;1b]                      / canvas generated
t[`plotBraille;|/0>`plt 1.0*!30;1b]                      / contains UTF-8 multibyte (braille/box) chars
t[`candleRows;+/"\n"=`cdl (1.0 2 3;3.0 4 5;0.0 1 2;2.0 3 4);15]  / default H=15
/ ---- Arrow C Data Interface : pointer round-trips ----
t[`arrowNum;([]a:1 2 3;b:1.5 2.5 3.5)~arrow.import arrow.export ([]a:1 2 3;b:1.5 2.5 3.5);1b]
t[`arrowSym;([]s:`a`b`c;v:10 20 30)~arrow.import arrow.export ([]s:`a`b`c;v:10 20 30);1b]
t[`arrowWide;([]x:100000 200000 300000;y:9000000000 9000000001 9000000002)~arrow.import arrow.export ([]x:100000 200000 300000;y:9000000000 9000000001 9000000002);1b]
t[`arrowRange;([]v:0+!50)~arrow.import arrow.export ([]v:0+!50);1b]
t[`arrowPtrs;{[t]p:arrow.export t;n:#p;arrow.import p;n}[([]a:1 2 3)];2]  / export -> 2 C ABI addresses

fails:r@&{"FAIL"~4#x}'r[;1]
`0:"";`0:"============ EXTENDED TEST SUITE ============"
`0:(($#r)," tests run, ",($#fails)," failures")
$[#fails;{`0:"  ",(x 0),": ",x 1}'fails;`0:"ALL EXTENDED TESTS PASSED"];
`0:"============================================="
