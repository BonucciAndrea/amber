\l amber.k
\l fin.k
/ ============================================================================
/ demo/hft_demo.k  -  Mega Demo, part 1: HFT financial tick demo.
/ --
/ Generates a realistic multi-symbol tick session, computes VWAP per symbol,
/ a 50-period EMA via the tacit `ema` call, and an as-of join matching every
/ trade to its prevailing quote -- then prints a formatted benchmark table
/ of how long each stage took, timed with the same `t[] clock and `ts (see
/ std.k) build on internally.
/ --
/ Row count is the `N` global just below -- edit it directly for a quicker
/ local run (e.g. N:200000 finishes in well under a second on a laptop).
/ --
/ run:  ./amber demo/hft_demo.k
/ ============================================================================

N:5000000                              / 5,000,000 trades (10,000,000 quotes); edit down for a quick local check

hr:{`0:"";`0:"== ",x," ",(60-#x)#"=";}

/ ---- benchmark table: collects stage tags + ms, printed at the end --------
/ (the table's `stage` column has to be a symbol vector, not a list of
/  variable-length strings -- so bench takes a short `tag symbol for the
/  table alongside the long descriptive `nm string used in the live line)
BSTG:()                                / stage tags (becomes a symbol vector on first append)
BMS:()                                 / matching elapsed-ms values
bench:{[tag;nm;f] t0:`t[]; r:f[]; ms:0.001*`t[]-t0;
  BSTG,:tag; BMS,:ms;
  pad:(54-#nm)#".";
  msg:nm,pad," ",(-8$$round[1]ms),"ms\n";
  `0:msg;
  r}

`0:"Amber HFT Mega Demo — ",($N)," trades / ",($2*N)," quotes\n"
`0:"------------------------------------------------------------\n"

/ ---- 1. generate a realistic multi-symbol tick session --------------------
/ gentq[n] builds `n` trades and 2n quotes across amber.k's OUNI symbol
/ universe with a proper order book (bid/ask/mid), assigns each trade to the
/ bid or ask side, prices it near the prevailing mid with realistic
/ microstructure noise, then sorts both tables by (sym;time). Sets the
/ globals `trades` and `quotes`.
qq:bench[`gentq;"1. generate tick session (gentq)";{gentq N}]

`0:"   trades: ",($#trades),"   quotes: ",($#quotes),"   symbols: ",($#distinct trades`sym),"\n"

/ ---- 2. VWAP per symbol ----------------------------------------------------
/ wavg (amber.k: {(+/x*y)%+/x}) is the volume-weighted average -- size-weight
/ every trade's price. qby groups trades by symbol and applies it per group.
vwapT:bench[`vwap;"2. VWAP per symbol (qby + wavg)";{qby[trades;`sym;(,`vwap)!,{round[4]wavg[x`sz;x`px]}]}]

`0:"   sample VWAPs:\n"
qq:pt 5#unkey vwapT

/ ---- 3. 50-period EMA via the tacit `ema` call -----------------------------
/ ema:{[a;x]`ema(a;x)} in fin.k is a tacit wrapper around the native C EMA
/ kernel (O(n) sweep) -- `a` is the smoothing factor; the standard mapping
/ from an "N-period EMA" to a smoothing factor is a = 2/(N+1).
g:bysym trades                                    / per-symbol row positions (group index)
firstSym:first distinct trades`sym
pxSeries:(symrows[trades;g;firstSym])`px          / one symbol's price series, in time order
emaA:2%51                                          / 50-period EMA smoothing factor
emaSeries:bench[`ema;"3. 50-period EMA (tacit `ema call)";{ema[emaA;pxSeries]}]

`0:"   ",($firstSym)," price (last 8): ",sv[" ";$'-8#round[2]pxSeries],"\n"
`0:"   ",($firstSym)," ema[50] (last 8): ",sv[" ";$'-8#round[2]emaSeries],"\n"

/ ---- 4. as-of join: match every trade with its nearest prevailing quote ---
/ aj[`sym`time;trades;quotes] is the native C as-of-join kernel (branch-free
/ lower_bound over each symbol's sorted quote timestamps) -- for every trade,
/ find the most recent quote at-or-before it, per symbol.
matched:bench[`asof;"4. as-of join: trades -> nearest quote (aj)";{taq[trades;quotes]}]

matched:qwhere[matched;~^matched`bid]              / drop any trade before its symbol's first quote
`0:"   matched rows: ",($#matched)," (",($#trades),"-",($(#trades)-#matched)," dropped: no quote yet)\n"
`0:"   sample matched rows:\n"
qq:pt 5#matched

/ ---- summary ---------------------------------------------------------------
qq:hr"benchmark summary"
summaryT:+`stage`ms!(BSTG;round[1]BMS)
qq:show summaryT
`0:"total: ",($round[1]sum BMS)," ms for ",($N)," trades end-to-end\n"

/ ---- literal `\t` micro-timer, per the spec: one line, self-timing --------
`0:"\n`\\t:3 sum trades\\`sz` (3 runs, native backslash-timer, ms):\n"
\t:3 sum trades`sz

`0:"\nHFT demo complete.\n"
