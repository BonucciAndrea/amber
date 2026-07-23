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

fails:r@&{"FAIL"~4#x}'r[;1]
`0:"";`0:"============ EXTENDED TEST SUITE ============"
`0:(($#r)," tests run, ",($#fails)," failures")
$[#fails;{`0:"  ",(x 0),": ",x 1}'fails;`0:"ALL EXTENDED TESTS PASSED"];
`0:"============================================="
