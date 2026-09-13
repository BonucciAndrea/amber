\l amber.k
\l demo/gen.k
\l demo/bt.k
px1:100#genpx[100;4600]
S:til 1800
`0:"=== observation-count scaling (U=100, S=1800, monthly step=21) ======"
IN:btIdx[S; 21*1+til 6]
PN:btTensor[px1;IN;S]
QN:`acLvl`cpnLvl`cpnAmt`barLvl`memory`ppy!((6#1.0);(6#0.70);0.025;0.60;1b;12)
pf:btWorst[PN;0 7 23]
rr:btRun[QN;pf]
t0:`t[]
rr:btRun[QN;pf]
u1:`t[]-t0
t0:`t[]
rr:btRun[QN;pf]
u2:`t[]-t0
t0:`t[]
pf:btWorst[PN;1 8 24]
u3:`t[]-t0
`0:"  n=6    lifecycle ",($ _ (u1&u2))," us   basket-min ",($ _ u3)," us   total ",($ _ (u3+u1&u2))," us"
IN:btIdx[S; 21*1+til 12]
PN:btTensor[px1;IN;S]
QN:`acLvl`cpnLvl`cpnAmt`barLvl`memory`ppy!((12#1.0);(12#0.70);0.025;0.60;1b;12)
pf:btWorst[PN;0 7 23]
rr:btRun[QN;pf]
t0:`t[]
rr:btRun[QN;pf]
u1:`t[]-t0
t0:`t[]
rr:btRun[QN;pf]
u2:`t[]-t0
t0:`t[]
pf:btWorst[PN;1 8 24]
u3:`t[]-t0
`0:"  n=12   lifecycle ",($ _ (u1&u2))," us   basket-min ",($ _ u3)," us   total ",($ _ (u3+u1&u2))," us"
IN:btIdx[S; 21*1+til 24]
PN:btTensor[px1;IN;S]
QN:`acLvl`cpnLvl`cpnAmt`barLvl`memory`ppy!((24#1.0);(24#0.70);0.025;0.60;1b;12)
pf:btWorst[PN;0 7 23]
rr:btRun[QN;pf]
t0:`t[]
rr:btRun[QN;pf]
u1:`t[]-t0
t0:`t[]
rr:btRun[QN;pf]
u2:`t[]-t0
t0:`t[]
pf:btWorst[PN;1 8 24]
u3:`t[]-t0
`0:"  n=24   lifecycle ",($ _ (u1&u2))," us   basket-min ",($ _ u3)," us   total ",($ _ (u3+u1&u2))," us"
IN:btIdx[S; 21*1+til 48]
PN:btTensor[px1;IN;S]
QN:`acLvl`cpnLvl`cpnAmt`barLvl`memory`ppy!((48#1.0);(48#0.70);0.025;0.60;1b;12)
pf:btWorst[PN;0 7 23]
rr:btRun[QN;pf]
t0:`t[]
rr:btRun[QN;pf]
u1:`t[]-t0
t0:`t[]
rr:btRun[QN;pf]
u2:`t[]-t0
t0:`t[]
pf:btWorst[PN;1 8 24]
u3:`t[]-t0
`0:"  n=48   lifecycle ",($ _ (u1&u2))," us   basket-min ",($ _ u3)," us   total ",($ _ (u3+u1&u2))," us"
IN:btIdx[S; 21*1+til 96]
PN:btTensor[px1;IN;S]
QN:`acLvl`cpnLvl`cpnAmt`barLvl`memory`ppy!((96#1.0);(96#0.70);0.025;0.60;1b;12)
pf:btWorst[PN;0 7 23]
rr:btRun[QN;pf]
t0:`t[]
rr:btRun[QN;pf]
u1:`t[]-t0
t0:`t[]
rr:btRun[QN;pf]
u2:`t[]-t0
t0:`t[]
pf:btWorst[PN;1 8 24]
u3:`t[]-t0
`0:"  n=96   lifecycle ",($ _ (u1&u2))," us   basket-min ",($ _ u3)," us   total ",($ _ (u3+u1&u2))," us"
IN:btIdx[S; 21*1+til 120]
PN:btTensor[px1;IN;S]
QN:`acLvl`cpnLvl`cpnAmt`barLvl`memory`ppy!((120#1.0);(120#0.70);0.025;0.60;1b;12)
pf:btWorst[PN;0 7 23]
rr:btRun[QN;pf]
t0:`t[]
rr:btRun[QN;pf]
u1:`t[]-t0
t0:`t[]
rr:btRun[QN;pf]
u2:`t[]-t0
t0:`t[]
pf:btWorst[PN;1 8 24]
u3:`t[]-t0
`0:"  n=120  lifecycle ",($ _ (u1&u2))," us   basket-min ",($ _ u3)," us   total ",($ _ (u3+u1&u2))," us"
