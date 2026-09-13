/ ===========================================================================
/ bt.k  -  Amber backtest kernel for structured payoffs.
/ .
/ A product compiles to length-n vectors plus three scalars. Everything here
/ is whole-array: no loop over start dates, no loop over observations, no
/ loop over rows. The only iteration is over baskets, and a basket is a
/ min-reduce over slices of a tensor that was computed once.
/ .
/ NOTE: kept as several small lambdas rather than one -- ngn/k caps locals
/ per lambda, and the natural single-function version exceeds it.
/ ===========================================================================

/ ---- observation lattice: S start rows x n offsets -> S x n index matrix --
btIdx:{[S;off] S +\: off}

/ ---- performance tensor: U x S x n, computed ONCE for the whole universe --
btTensor:{[px;I;S] {[p;I;S] (p@I) %' p@S}[;I;S]'px}

/ ---- basket rules --------------------------------------------------------
btWorst:{[P;b] (&/) P b}
btBest :{[P;b] (|/) P b}
btEqw  :{[P;b] ((+/) P b) % #b}

btMx :{maxs x}
btSm :{+/x}
btLst:{*|x}

/ ---- redemption timing: returns (stopIndex; calledFlag) ------------------
btStop:{[lvl;perf]
  rdm: btMx'perf >\: lvl;
  nrd: btSm'rdm;
  ((#lvl) - nrd; 0 < nrd) }

/ ---- coupons banked by the stop index ------------------------------------
btCpn:{[lvl;mem;perf;sp]
  hit: perf >\: lvl;
  cm:  $[mem; btMx'hit *\: 1 + !#lvl; sums'hit];
  cm @' sp }

/ ---- redemption value ----------------------------------------------------
btVal:{[Q;perf;cp;cl]
  fp: btLst'perf;
  pr: cl | fp > Q`barLvl;
  (cp * Q`cpnAmt) + pr + (1 - pr) * fp }

/ ---- one product, one basket ---------------------------------------------
btRun:{[Q;perf]
  sc: btStop[Q`acLvl; perf];
  sp: (sc 0) & (#Q`acLvl) - 1;
  cp: btCpn[Q`cpnLvl; Q`memory; perf; sp];
  pv: btVal[Q; perf; cp; sc 1];
  `val`life`called`at`paid!(pv; (1 + sp) % Q`ppy; sc 1; sp; cp) }

btIrr:{[r] (exp (log 0.0000001 | r`val) % r`life) - 1}

/ ---- summary across start dates ------------------------------------------
btPct:{[v;p] (asc v) @ _ p * -1 + #v}

btStats:{[r;ir] `n`irrMed`irrP05`pCall`pLoss`lifeMean!((#ir); btPct[ir;0.5]; btPct[ir;0.05]; avg 1.0*r`called; avg 1.0*(r`val)<1.0; avg r`life)}
