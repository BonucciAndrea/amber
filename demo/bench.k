\l amber.k
\l demo/gen.k
\l demo/bt.k

Q12:`acLvl`cpnLvl`cpnAmt`barLvl`memory`ppy!((12#1.0);(12#0.70);0.025;0.60;1b;4)
`0:"=== A. tensor build, scaling in universe size (S=1800, n=12) ========="
px4:genpx[400;4600]
S:til 1800
I:btIdx[S; 63*1+til 12]
pxA:25#px4
t0:`t[]
PA:btTensor[pxA;I;S]
t1:`t[]-t0
t0:`t[]
PA:btTensor[pxA;I;S]
t2:`t[]-t0
`0:"  U=25   ",($ _ (t1&t2)%1000)," ms   ",($ _ (25*1800*12)%1000000.0)," M cells"
pxA:50#px4
t0:`t[]
PA:btTensor[pxA;I;S]
t1:`t[]-t0
t0:`t[]
PA:btTensor[pxA;I;S]
t2:`t[]-t0
`0:"  U=50   ",($ _ (t1&t2)%1000)," ms   ",($ _ (50*1800*12)%1000000.0)," M cells"
pxA:100#px4
t0:`t[]
PA:btTensor[pxA;I;S]
t1:`t[]-t0
t0:`t[]
PA:btTensor[pxA;I;S]
t2:`t[]-t0
`0:"  U=100  ",($ _ (t1&t2)%1000)," ms   ",($ _ (100*1800*12)%1000000.0)," M cells"
pxA:200#px4
t0:`t[]
PA:btTensor[pxA;I;S]
t1:`t[]-t0
t0:`t[]
PA:btTensor[pxA;I;S]
t2:`t[]-t0
`0:"  U=200  ",($ _ (t1&t2)%1000)," ms   ",($ _ (200*1800*12)%1000000.0)," M cells"
pxA:400#px4
t0:`t[]
PA:btTensor[pxA;I;S]
t1:`t[]-t0
t0:`t[]
PA:btTensor[pxA;I;S]
t2:`t[]-t0
`0:"  U=400  ",($ _ (t1&t2)%1000)," ms   ",($ _ (400*1800*12)%1000000.0)," M cells"
`0:""
`0:"=== B. tensor build, scaling in start dates (U=100, n=12) ==========="
px1:100#px4
SB:til 450
IB:btIdx[SB; 63*1+til 12]
t0:`t[]
PB:btTensor[px1;IB;SB]
t1:`t[]-t0
t0:`t[]
PB:btTensor[px1;IB;SB]
t2:`t[]-t0
`0:"  S=450   ",($ _ (t1&t2)%1000)," ms"
SB:til 900
IB:btIdx[SB; 63*1+til 12]
t0:`t[]
PB:btTensor[px1;IB;SB]
t1:`t[]-t0
t0:`t[]
PB:btTensor[px1;IB;SB]
t2:`t[]-t0
`0:"  S=900   ",($ _ (t1&t2)%1000)," ms"
SB:til 1800
IB:btIdx[SB; 63*1+til 12]
t0:`t[]
PB:btTensor[px1;IB;SB]
t1:`t[]-t0
t0:`t[]
PB:btTensor[px1;IB;SB]
t2:`t[]-t0
`0:"  S=1800  ",($ _ (t1&t2)%1000)," ms"
SB:til 3600
IB:btIdx[SB; 63*1+til 12]
t0:`t[]
PB:btTensor[px1;IB;SB]
t1:`t[]-t0
t0:`t[]
PB:btTensor[px1;IB;SB]
t2:`t[]-t0
`0:"  S=3600  ",($ _ (t1&t2)%1000)," ms"
`0:""
`0:"=== C. one basket, full lifecycle ==================================="
SC:til 1800
IC:btIdx[SC; 63*1+til 12]
PC:btTensor[px1;IC;SC]
QC:`acLvl`cpnLvl`cpnAmt`barLvl`memory`ppy!((12#1.0);(12#0.70);0.025;0.60;1b;4)
pfC:btWorst[PC;0 7 23]
rr:btRun[QC;pfC]
t0:`t[]
rr:btRun[QC;pfC]
t1:`t[]-t0
t0:`t[]
rr:btRun[QC;pfC]
t2:`t[]-t0
t0:`t[]
pfC:btWorst[PC;1 8 24]
t3:`t[]-t0
`0:"  S=1800  n=12  lifecycle ",($ _ (t1&t2))," us   basket-min ",($ _ t3)," us   = ",($ _ (t3+t1&t2))," us total"
SC:til 1800
IC:btIdx[SC; 189*1+til 4]
PC:btTensor[px1;IC;SC]
QC:`acLvl`cpnLvl`cpnAmt`barLvl`memory`ppy!((4#1.0);(4#0.70);0.025;0.60;1b;4)
pfC:btWorst[PC;0 7 23]
rr:btRun[QC;pfC]
t0:`t[]
rr:btRun[QC;pfC]
t1:`t[]-t0
t0:`t[]
rr:btRun[QC;pfC]
t2:`t[]-t0
t0:`t[]
pfC:btWorst[PC;1 8 24]
t3:`t[]-t0
`0:"  S=1800  n=4   lifecycle ",($ _ (t1&t2))," us   basket-min ",($ _ t3)," us   = ",($ _ (t3+t1&t2))," us total"
SC:til 1800
IC:btIdx[SC; 63*1+til 20]
PC:btTensor[px1;IC;SC]
QC:`acLvl`cpnLvl`cpnAmt`barLvl`memory`ppy!((20#1.0);(20#0.70);0.025;0.60;1b;4)
pfC:btWorst[PC;0 7 23]
rr:btRun[QC;pfC]
t0:`t[]
rr:btRun[QC;pfC]
t1:`t[]-t0
t0:`t[]
rr:btRun[QC;pfC]
t2:`t[]-t0
t0:`t[]
pfC:btWorst[PC;1 8 24]
t3:`t[]-t0
`0:"  S=1800  n=20  lifecycle ",($ _ (t1&t2))," us   basket-min ",($ _ t3)," us   = ",($ _ (t3+t1&t2))," us total"
SC:til 3600
IC:btIdx[SC; 63*1+til 12]
PC:btTensor[px1;IC;SC]
QC:`acLvl`cpnLvl`cpnAmt`barLvl`memory`ppy!((12#1.0);(12#0.70);0.025;0.60;1b;4)
pfC:btWorst[PC;0 7 23]
rr:btRun[QC;pfC]
t0:`t[]
rr:btRun[QC;pfC]
t1:`t[]-t0
t0:`t[]
rr:btRun[QC;pfC]
t2:`t[]-t0
t0:`t[]
pfC:btWorst[PC;1 8 24]
t3:`t[]-t0
`0:"  S=3600  n=12  lifecycle ",($ _ (t1&t2))," us   basket-min ",($ _ t3)," us   = ",($ _ (t3+t1&t2))," us total"
SC:til 900
IC:btIdx[SC; 63*1+til 12]
PC:btTensor[px1;IC;SC]
QC:`acLvl`cpnLvl`cpnAmt`barLvl`memory`ppy!((12#1.0);(12#0.70);0.025;0.60;1b;4)
pfC:btWorst[PC;0 7 23]
rr:btRun[QC;pfC]
t0:`t[]
rr:btRun[QC;pfC]
t1:`t[]-t0
t0:`t[]
rr:btRun[QC;pfC]
t2:`t[]-t0
t0:`t[]
pfC:btWorst[PC;1 8 24]
t3:`t[]-t0
`0:"  S=900   n=12  lifecycle ",($ _ (t1&t2))," us   basket-min ",($ _ t3)," us   = ",($ _ (t3+t1&t2))," us total"
`0:""
`0:"=== D. sweep, cost per basket ======================================="
SD:til 1800
ID:btIdx[SD; 63*1+til 12]
PD:btTensor[px1;ID;SD]
bk:{[i;U] 3#U?U}[;100]'til 100
t0:`t[]
rs:{[b;P;Q] r:btRun[Q;btWorst[P;b]]; (btPct[btIrr r;0.5]; avg 1.0*(r`val)<1.0)}[;PD;Q12]'bk
t1:`t[]-t0
`0:"  100    baskets ",($ _ t1%1000)," ms   ",($ _ t1%100)," us/basket   ",($ _ 1000*t1%100*1800)," ns/lifecycle"
bk:{[i;U] 3#U?U}[;100]'til 500
t0:`t[]
rs:{[b;P;Q] r:btRun[Q;btWorst[P;b]]; (btPct[btIrr r;0.5]; avg 1.0*(r`val)<1.0)}[;PD;Q12]'bk
t1:`t[]-t0
`0:"  500    baskets ",($ _ t1%1000)," ms   ",($ _ t1%500)," us/basket   ",($ _ 1000*t1%500*1800)," ns/lifecycle"
bk:{[i;U] 3#U?U}[;100]'til 2000
t0:`t[]
rs:{[b;P;Q] r:btRun[Q;btWorst[P;b]]; (btPct[btIrr r;0.5]; avg 1.0*(r`val)<1.0)}[;PD;Q12]'bk
t1:`t[]-t0
`0:"  2000   baskets ",($ _ t1%1000)," ms   ",($ _ t1%2000)," us/basket   ",($ _ 1000*t1%2000*1800)," ns/lifecycle"
bk:{[i;U] 3#U?U}[;100]'til 8000
t0:`t[]
rs:{[b;P;Q] r:btRun[Q;btWorst[P;b]]; (btPct[btIrr r;0.5]; avg 1.0*(r`val)<1.0)}[;PD;Q12]'bk
t1:`t[]-t0
`0:"  8000   baskets ",($ _ t1%1000)," ms   ",($ _ t1%8000)," us/basket   ",($ _ 1000*t1%8000*1800)," ns/lifecycle"
