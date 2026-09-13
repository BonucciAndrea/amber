\l amber.k
\l demo/gen.k
\l demo/bt.k

U:100
T:2600
NS:1800
NOBS:12
STEP:63
NB:4000

L:"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
syms:`$'{[L;i] (L@div[i;26]),(L@mod[i;26]),"X"}[L]'til U

`0:"== data ============================================================"
t0:`t[]
px:genpx[U;T]
`0:"generate        ",($ _ (`t[]-t0)%1000)," ms    ",($U)," underlyings x ",($T)," days (synthetic GBM)"

S:til NS
t0:`t[]
I:btIdx[S; STEP*1+til NOBS]
P:btTensor[px;I;S]
tTen:`t[]-t0
`0:"tensor          ",($ _ tTen%1000)," ms    ",($U),"x",($NS),"x",($NOBS)," = ",($U*NS*NOBS)," floats, built ONCE"

Q:`acLvl`cpnLvl`cpnAmt`barLvl`memory`ppy!((NOBS#1.0);(NOBS#0.70);0.025;0.60;1b;4)

`0:""
`0:"== one product, one basket ========================================="
t0:`t[]
perf:btWorst[P;0 7 23]
r:btRun[Q;perf]
ir:btIrr r
t1:`t[]-t0
`0:"evaluate        ",($ _ t1)," us    ",($NS)," start dates"
show btStats[r;ir]

`0:""
`0:"autocall by observation (cumulative %):"
cum:{[r;j] _ 0.5 + 1000 * avg 1.0 * (r`called) & (r`at) < j}[r]'1+til NOBS
show ([]obs:1+til NOBS; cumPct:cum % 10.0)

`0:""
`0:"IRR distribution (deciles):"
show ([]decile:1+til 9; irr:{[ir;q] btPct[ir;q%10.0]}[ir]'1+til 9)

`0:""
`0:"== the sweep ======================================================="
bsk:{[i;U] 3#U?U}[;U]'til NB
t0:`t[]
res:{[P;Q;b] ir:btIrr r:btRun[Q;btWorst[P;b]]; (btPct[ir;0.5]; avg 1.0*r`called; avg 1.0*(r`val)<1.0)}[P;Q]'bsk
tSw:`t[]-t0
`0:($NB)," baskets x ",($NS)," start dates = ",($NB*NS)," full lifecycles"
`0:"sweep           ",($ _ tSw%1000)," ms    ",($ _ 1000*tSw%NB*NS)," ns per lifecycle"

m:+res
med:m 0
ord:idesc med
nm:`$'{[b;syms] sv["/"; $syms b]}[;syms]'bsk
`0:""
`0:"best 6 baskets by median IRR:"
show 6#([]basket:nm ord; medIRR:med ord; pCall:(m 1) ord; pLoss:(m 2) ord)
`0:""
`0:"worst 4:"
rev:|ord
show 4#([]basket:nm rev; medIRR:med rev; pCall:(m 1) rev; pLoss:(m 2) rev)

`0:""
`0:"sample baskets:"
show 4#nm
