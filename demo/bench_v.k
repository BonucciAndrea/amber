\l amber.k
\l demo/gen.k
\l demo/bt.k
px1:100#genpx[100;4600]
S:til 1800
`0:"=== tensor build, U=100 S=1800 n=12, ten consecutive runs =========="
IT:btIdx[S; 21*1+til 12]
t0:`t[]
PT:btTensor[px1;IT;S]
`0:"  run 1   ",($ _ (`t[]-t0)%1000)," ms"
t0:`t[]
PT:btTensor[px1;IT;S]
`0:"  run 2   ",($ _ (`t[]-t0)%1000)," ms"
t0:`t[]
PT:btTensor[px1;IT;S]
`0:"  run 3   ",($ _ (`t[]-t0)%1000)," ms"
t0:`t[]
PT:btTensor[px1;IT;S]
`0:"  run 4   ",($ _ (`t[]-t0)%1000)," ms"
t0:`t[]
PT:btTensor[px1;IT;S]
`0:"  run 5   ",($ _ (`t[]-t0)%1000)," ms"
t0:`t[]
PT:btTensor[px1;IT;S]
`0:"  run 6   ",($ _ (`t[]-t0)%1000)," ms"
t0:`t[]
PT:btTensor[px1;IT;S]
`0:"  run 7   ",($ _ (`t[]-t0)%1000)," ms"
t0:`t[]
PT:btTensor[px1;IT;S]
`0:"  run 8   ",($ _ (`t[]-t0)%1000)," ms"
t0:`t[]
PT:btTensor[px1;IT;S]
`0:"  run 9   ",($ _ (`t[]-t0)%1000)," ms"
t0:`t[]
PT:btTensor[px1;IT;S]
`0:"  run 10  ",($ _ (`t[]-t0)%1000)," ms"
`0:""
`0:"=== lifecycle kernel only, 7 reps each ============================="
IN:btIdx[S; 21*1+til 12]
PN:btTensor[px1;IN;S]
QN:`acLvl`cpnLvl`cpnAmt`barLvl`memory`ppy!((12#1.0);(12#0.70);0.025;0.60;1b;12)
pf:btWorst[PN;0 7 23]
rr:btRun[QN;pf]
bs:()
t0:`t[]
rr:btRun[QN;pf]
bs:bs,`t[]-t0
t0:`t[]
rr:btRun[QN;pf]
bs:bs,`t[]-t0
t0:`t[]
rr:btRun[QN;pf]
bs:bs,`t[]-t0
t0:`t[]
rr:btRun[QN;pf]
bs:bs,`t[]-t0
t0:`t[]
rr:btRun[QN;pf]
bs:bs,`t[]-t0
t0:`t[]
rr:btRun[QN;pf]
bs:bs,`t[]-t0
t0:`t[]
rr:btRun[QN;pf]
bs:bs,`t[]-t0
`0:"  n=12   min ",($ _ min bs)," us   median ",($ _ (asc bs)@3)," us   max ",($ _ max bs)," us"
IN:btIdx[S; 21*1+til 48]
PN:btTensor[px1;IN;S]
QN:`acLvl`cpnLvl`cpnAmt`barLvl`memory`ppy!((48#1.0);(48#0.70);0.025;0.60;1b;12)
pf:btWorst[PN;0 7 23]
rr:btRun[QN;pf]
bs:()
t0:`t[]
rr:btRun[QN;pf]
bs:bs,`t[]-t0
t0:`t[]
rr:btRun[QN;pf]
bs:bs,`t[]-t0
t0:`t[]
rr:btRun[QN;pf]
bs:bs,`t[]-t0
t0:`t[]
rr:btRun[QN;pf]
bs:bs,`t[]-t0
t0:`t[]
rr:btRun[QN;pf]
bs:bs,`t[]-t0
t0:`t[]
rr:btRun[QN;pf]
bs:bs,`t[]-t0
t0:`t[]
rr:btRun[QN;pf]
bs:bs,`t[]-t0
`0:"  n=48   min ",($ _ min bs)," us   median ",($ _ (asc bs)@3)," us   max ",($ _ max bs)," us"
IN:btIdx[S; 21*1+til 96]
PN:btTensor[px1;IN;S]
QN:`acLvl`cpnLvl`cpnAmt`barLvl`memory`ppy!((96#1.0);(96#0.70);0.025;0.60;1b;12)
pf:btWorst[PN;0 7 23]
rr:btRun[QN;pf]
bs:()
t0:`t[]
rr:btRun[QN;pf]
bs:bs,`t[]-t0
t0:`t[]
rr:btRun[QN;pf]
bs:bs,`t[]-t0
t0:`t[]
rr:btRun[QN;pf]
bs:bs,`t[]-t0
t0:`t[]
rr:btRun[QN;pf]
bs:bs,`t[]-t0
t0:`t[]
rr:btRun[QN;pf]
bs:bs,`t[]-t0
t0:`t[]
rr:btRun[QN;pf]
bs:bs,`t[]-t0
t0:`t[]
rr:btRun[QN;pf]
bs:bs,`t[]-t0
`0:"  n=96   min ",($ _ min bs)," us   median ",($ _ (asc bs)@3)," us   max ",($ _ max bs)," us"
IN:btIdx[S; 21*1+til 120]
PN:btTensor[px1;IN;S]
QN:`acLvl`cpnLvl`cpnAmt`barLvl`memory`ppy!((120#1.0);(120#0.70);0.025;0.60;1b;12)
pf:btWorst[PN;0 7 23]
rr:btRun[QN;pf]
bs:()
t0:`t[]
rr:btRun[QN;pf]
bs:bs,`t[]-t0
t0:`t[]
rr:btRun[QN;pf]
bs:bs,`t[]-t0
t0:`t[]
rr:btRun[QN;pf]
bs:bs,`t[]-t0
t0:`t[]
rr:btRun[QN;pf]
bs:bs,`t[]-t0
t0:`t[]
rr:btRun[QN;pf]
bs:bs,`t[]-t0
t0:`t[]
rr:btRun[QN;pf]
bs:bs,`t[]-t0
t0:`t[]
rr:btRun[QN;pf]
bs:bs,`t[]-t0
`0:"  n=120  min ",($ _ min bs)," us   median ",($ _ (asc bs)@3)," us   max ",($ _ max bs)," us"
