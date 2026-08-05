\l amber.k
\l fin.k
\l std.k
\l qsql.k
\l temporal.k
\l sys.k
\l hdb.k
/ ============================================================================
/ examples/hdb_demo.k  -  on-disk data (hdb.k): splay / dload / partsave.
/ Native-only: writes real files under /tmp/amber_db via mkd ("/bin/mkdir")
/ and 1: (file write), neither of which exist in a browser/wasm sandbox --
/ this was originally part of examples/extended.k's tour, split out into its
/ own file so extended.k stays safe to run in-browser (see extended.k's own
/ header note). Run natively:  ./amber examples/hdb_demo.k
/ ============================================================================
zz:gentq 2000
hr:{`0:"";`0:"== ",x," ",(56-#x)#"=";}

hr"on-disk data (hdb.k)"
/ text serialization suits modest tables; use a small slice here
st:25#trades
x:splay["/tmp/amber_db/trades";st]
`0:"splayed & reloaded, rows match: ",$st~dload"/tmp/amber_db/trades"
x:partsave["/tmp/amber_db/pdb";st;`sym]
`0:"partitioned by sym, partitions: ",($#parts"/tmp/amber_db/pdb")

`0:"";`0:"hdb demo complete."
