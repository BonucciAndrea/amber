\l amber.k
\l fin.k
\l std.k
\l qsql.k
\l temporal.k
\l sys.k
\l ipc.k
/ ============================================================================
/ examples/extended.k  -  a tour of the v1.5 modules.  Also a smoke test.
/ run:  ./amber examples/extended.k
/ Browser/wasm-safe: unlike an earlier version of this file, it no longer
/ touches hdb.k's on-disk functions (splay/dload/partsave), which shell out
/ to a real "mkdir" and write real files -- neither exists in a wasm sandbox,
/ so calling them there is a hard crash (a WebAssembly trap), not a
/ catchable K error. See examples/hdb_demo.k for that tour, native-only.
/ ============================================================================
hr:{`0:"";`0:"== ",x," ",(56-#x)#"=";}

hr"1. qSQL template (qsql.k)"
zz:gentq 2000
`0:"select vwap by symbol, where px>100:"
show sel"select vwap:round[4]wavg[sz;px],n:#px by sym from trades where px>100"
`0:"update: add a notional column (first 3 rows):"
show 3#upd"update notl:px*sz from trades"
`0:"delete small trades, count remaining:"
`0:"  ",($#del"delete from trades where sz<200")," of ",($#trades)

hr"2. vectorised moving / window aggregates (std.k)"
p:trades[`px]@where trades[`sym]=`AAPL
`0:"px (first 6):        "," "/$6#round[2]p
`0:"mavg[5] (5..10):     "," "/$5_11#round[2]mavg[5;p]
`0:"mmax[5] (5..10):     "," "/$5_11#round[2]mmax[5;p]
`0:"mdev[5] (last 4):    "," "/$-4#round[4]mdev[5;p]

hr"3. date & timestamp types (temporal.k)"
d:ymd2d[2024;1;15]
`0:"date 2024.01.15  = ",($d)," (days since 2000.01.01)"
`0:"dstr            = ",dstr d
`0:"day of week     = ",$dow d
`0:"timestamp       = ",pstr tstamp[d;hms[9;30;0]]
`0:"z.d today       = ",dstr z.d[]

hr"4. system namespaces + JSON (sys.k)"
`0:"Q.f[2;3.14159]  = ",Q.f[2;3.14159]
`0:"j.j of a table  = ",j.j 2#trades
`0:"j.k parse       = ",`k j.k"{\"a\":1,\"b\":[2,3]}"

hr"5. in-process tickerplant (ipc.k)"
x:u.def[`trade;([]sym:0#`;px:0#0.0)]
CNT::0
x:u.sub[`trade;{[nm;d]CNT::CNT+#d}]
x:u.pub[`trade;([]sym:`a`b;px:10 20.0)]
x:u.pub[`trade;([]sym:`c`d`e;px:30 40 50.0)]
`0:"published 2 batches -> accumulated ",($#u.get`trade)," rows, ",($CNT)," via callback"

`0:"";`0:"extended tour complete."
