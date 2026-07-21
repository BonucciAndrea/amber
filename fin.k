/ ============================================================================
/ fin.k  -  Amber financial / HFT module.  Loaded after amber.k.
/ Tables carry attributes on key columns; times are numeric (ms since midnight).
/ ============================================================================

/ ---- attribute helpers on table columns ----
sortcol:{[c;t]d:+t;+d,(1#c,())!`sa'd@1#c,()}          / mark column c sorted (`s)
partcol:{[c;t]d:+t;+d,(1#c,())!`pa'd@1#c,()}          / mark column c parted (`p)
groupcol:{[c;t]d:+t;+d,(1#c,())!`ga'd@1#c,()}         / mark column c grouped (`g)

/ ---- O(1) group index (value -> row positions) ----
gidx:{=x}                                              / index of a vector:  gidx[v]  ->  val!positions
bysym:{[t]=t`sym}                                      / per-symbol row positions of a table
symrows:{[t;g;s]atr[t;g s]}                            / O(1): rows of table t for symbol s (g=bysym t)

/ ---- data generator: sets globals trades & quotes (numeric times, `s on time, `p on sym) ----
gentq:{[n]u:`AAPL`MSFT`GOOG`AMZN`META`TSLA`NVDA`JPM`XOM`WMT;o:hms[9;30;0];L:hms[16;0;0]-o;bp:u!100.0+10*!#u;s:n?u;m:2*n;qs:m?u;mid:round[2](bp qs)+0.01*(m?201)-100;h:0.01*1+m?10;trades::sortcol[`time;partcol[`sym;xasc[`sym`time;([]sym:s;time:o+n?L;px:round[2](bp s)+0.01*(n?201)-100;sz:100*1+n?10)]]];quotes::sortcol[`time;partcol[`sym;xasc[`sym`time;([]sym:qs;time:o+m?L;bid:mid-h;ask:mid+h;bsz:100*1+m?5;asz:100*1+m?5)]]];`trades`quotes}

/ ---- quote analytics (take a quote table) ----
mid:{0.5*x[`bid]+x`ask}
spread:{x[`ask]-x`bid}
spreadbps:{round[2]10000*spread[x]%mid x}
micro:{((x[`bid]*x`asz)+x[`ask]*x`bsz)%x[`bsz]+x`asz}   / size-weighted microprice
imbal:{round[4](x[`bsz]-x`asz)%x[`bsz]+x`asz}           / order-book imbalance in [-1,1]

/ ---- price series analytics ----
ret:{deltas[x]%prev x}                                 / simple returns
logret:{deltas log x}                                  / log returns
rvol:{sqrt sum r*r:1_logret x}                         / realized volatility
vwap:{[t]wavg[t`sz;t`px]}                               / volume-weighted avg price
twap:{[t]avg t`px}                                      / time-weighted (simple) avg price

/ ---- trade-quote (TAQ) matching ----
taq:{[tr;qu]aj[`sym`time;tr;qu]}                       / attach prevailing quote to each trade
tsign:{signum x[`px]-mid x}                            / Lee-Ready trade sign (needs bid/ask): +1 buy -1 sell
signedvol:{(tsign x)*x`sz}                             / signed volume
effspread:{2*abs x[`px]-mid x}                         / effective spread = 2|px-mid|
notional:{x[`px]*x`sz}

/ ---- OHLCV bars (w-minute) ----
bars:{[w;t]qby[+@[+t;,`time;minbar[w]@];`sym`time;`o`h`l`c`v!({first x`px};{max x`px};{min x`px};{last x`px};{sum x`sz})]}

/ ---- rolling / moving ----
movavg:{[w;x]x:0.0+x;((w-1)#0n),avg'w':x}                       / w-window moving average (front-padded)
movsum:{[w;x]x:0.0+x;((w-1)#0n),sum'w':x}
movmax:{[w;x]x:0.0+x;((w-1)#0n),max'w':x}
movmin:{[w;x]x:0.0+x;((w-1)#0n),min'w':x}
ema:{[a;x]x:0.0+x;(1#x),(*x){[a;p;v](a*v)+p*1-a}[a]\1_x}       / exponential moving average, factor a in (0,1]
rollstd:{[w;x]sqrt(movavg[w;x*x])-m*m:movavg[w;x]}     / rolling standard deviation

/ ---- per-symbol summary ----
symstats:{[t]qby[t;`sym;`n`vwap`hi`lo`vol!({#x};{round[4]wavg[x`sz;x`px]};{max x`px};{min x`px};{sum x`sz})]}

/ ---- pretty print: format any `time` column as HH:MM:SS.mmm ----
pt:{show $[in[`time;cols x];tsym[x;`time];x]}
