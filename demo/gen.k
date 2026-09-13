/ ===========================================================================
/ gen.k  -  synthetic but structurally realistic price history
/   correlated GBM: one market factor + per-name beta + idiosyncratic noise
/   SYNTHETIC. Shapes and dynamics are right; these are not real prices.
/ ===========================================================================

nrm:{[n] u:n?1.0; v:n?1.0; (sqrt -2*log 0.0000001+u)*cos 6.283185307179586*v}

/ U underlyings x T daily closes
genpx:{[U;T]
  mkt: 0.0085 * nrm T;                                  / market factor, ~13.5% ann vol
  bet: 0.55 + 0.9 * U?1.0;                              / beta in [0.55,1.45]
  ivl: 0.008 + 0.016 * U?1.0;                           / idio vol 12%-38% ann
  dft: -0.00005 + 0.0004 * U?1.0;                       / small drift dispersion
  {[mkt;T;b;s;d] 100.0 * exp sums d + (b*mkt) + s*nrm T}[mkt;T]'[bet;ivl;dft] }
