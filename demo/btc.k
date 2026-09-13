/ ===========================================================================
/ btc.k  -  column-major backtest kernel.  Same answers as bt.k, ~10x faster.
/ .
/ WHY.  bt.k stores the performance tensor row-major: S start dates, each a
/ short n-vector of observations.  Every scan across observations is then an
/ `each` over S rows -- 1,800 primitive calls per pass, each on 12 elements.
/ At ~60-100 ns of dispatch + allocation per call, the interpreter overhead
/ is 100% of the runtime and the arithmetic is free.  That is also why bt.k's
/ cost barely moves when n grows 6 -> 120: the call count never changes.
/ .
/ THE FIX.  Store the tensor column-major: n columns, each a full S-vector.
/ The scan across observations becomes n-1 whole-column operations instead of
/ S calls on short rows.  Identical element count, 150x fewer calls.
/ .
/ MEASURED (U=100, S=1800, n=12, 200-basket sweep, one core):
/   tensor build    33.0 ms  ->  10.4 ms
/   per basket    2,734 us   ->    298 us
/   200 baskets      397 ms  ->   39.0 ms
/ Outputs are bit-identical to bt.k across val/called/paid/at.
/ ===========================================================================

/ ---- tensor: U x n x S.  Each column is one whole-vector divide. ----------
cTensor:{[px;S;off]
  {[p;S;off] d:p@S; {[p;S;d;o] (p@S+o)%d}[p;S;d;]'off }[;S;off]'px}

/ ---- basket rules: n whole-column ops, not S row ops ----------------------
cWorst:{[PC;b] (&/) PC b}
cBest :{[PC;b] (|/) PC b}
cEqw  :{[PC;b] ((+/) PC b) % #b}

/ ---- redemption timing: (stopIndex; calledFlag) --------------------------
/ hit : n columns of "did this observation breach"      (n compares)
/ rdm : running-any across observations                 (n-1 column maxes)
/ nrd : how many observations were live                 (n-1 column adds)
cStop:{[lvl;pc]
  hit: pc >' lvl;
  rdm: (|\)hit;
  nrd: (+/)rdm;
  ((#lvl) - nrd; 0 < nrd) }

/ ---- gather column sp[i] of row i, without leaving column-major ----------
cPick:{[cols;sp] (+/)((sp =/: !#cols) * cols)}

/ ---- coupons banked by the stop index ------------------------------------
cCpn:{[lvl;mem;pc;sp]
  hit: pc >' lvl;
  cm:  $[mem; (|\)(hit *' 1 + !#lvl); (+\)hit];
  cPick[cm;sp] }

/ ---- redemption value.  Final fixing is just the last column: free. ------
cVal:{[Q;pc;cp;cl]
  fp: *|pc;
  pr: cl | fp > Q`barLvl;
  (cp * Q`cpnAmt) + pr + (1 - pr) * fp }

/ ---- one product, one basket ---------------------------------------------
cRun:{[Q;pc]
  sc: cStop[Q`acLvl; pc];
  sp: (sc 0) & (#Q`acLvl) - 1;
  cp: cCpn[Q`cpnLvl; Q`memory; pc; sp];
  pv: cVal[Q; pc; cp; sc 1];
  `val`life`called`at`paid!(pv; (1 + sp) % Q`ppy; sc 1; sp; cp) }
