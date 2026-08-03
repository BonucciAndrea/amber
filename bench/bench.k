/ Amber benchmark — deterministic data, prints "SANITY name value" and "TIME name ms".
\l amber.k
\l fin.k
\l std.k
\l qsql.k
\l temporal.k
\l sys.k
\l hdb.k
\l ipc.k
N:1000000
i:!N
px:0.001*100000!2654435761*i
sym:10!i
v:100000!2654435761*i
tm:{[n;e]t0:`t[];{. x}'n#,e;0.001*(`t[]-t0)%n}
S:{[nm;val]`0:"SANITY ",nm," ",($val),"\n"}
T:{[nm;n;e]`0:"TIME ",nm," ",($tm[n;e]),"\n"}
S["sum";+/px]
S["count_gt50";+/px>50]
S["grpsum";{+/px x}'. =sym]
S["median";(asc px)500000]
S["rollchk";+/mavg[100;px]]
S["distinct";#?v]
T["sum";30;"+/px"]
T["filter";30;"+/px>50"]
T["groupby";10;"{+/px x}'. =sym"]
T["rolling";10;"+/mavg[100;px]"]
T["sort";10;"asc px"]
T["distinct";10;"#?v"]
gentq 50000
T["asof";10;"aj[`sym`time;trades;quotes]"]
`0:"DONE\n"
