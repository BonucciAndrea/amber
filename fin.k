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
/ ---- realistic market-data generators (dev) ----
bmn:{[a;b](sqrt -2*log 1e-9|a)*cos 6.28318530718*b}
rn:{bmn[?x;?x]}
ushp:{0.5*1-cos 3.14159265359*x}
uvol:{1+1.5*(1-2*x)*1-2*x}
ncdf:{t:1%1+0.2316419*abs x;d:0.39894228*exp -0.5*x*x;p:d*t*(0.319381530+t*(-0.356563782+t*(1.781477937+t*(-1.821255978+t*1.330274429))));((x>0)*1-p)+(~x>0)*p}
npdf:{0.39894228*exp -0.5*x*x}
OUNI:`AAPL`MSFT`GOOG`AMZN`META`TSLA`NVDA`JPM`XOM`WMT
OBP:OUNI!187.0 411.0 139.0 178.0 495.0 250.0 118.0 195.0 104.0 68.0
OSIG:OUNI!0.018 0.016 0.017 0.020 0.022 0.038 0.034 0.012 0.015 0.010
OQBPS:OUNI!1.2 1.4 1.8 2.0 2.2 3.0 2.6 1.6 2.4 2.2
OPB:OUNI!0.51 0.50 0.50 0.51 0.52 0.53 0.54 0.49 0.48 0.50
OLIQW:22 16 10 11 9 20 24 8 6 5
OPOOL:,/OLIQW#'OUNI
OIV:OUNI!0.25 0.22 0.24 0.30 0.33 0.55 0.50 0.20 0.28 0.18
OOPEN:hms[9;30;0]
OSES:hms[16;0;0]-hms[9;30;0]
cnts:{[n]ts:$[n>=#OUNI;(OUNI),((n-#OUNI)?OPOOL);n?OPOOL];{sum y=x}[;ts]'OUNI}
qgen1:{[sym;k]rel:@[_OSES*asc ushp(?k);0;:;0];tm:OOPEN+rel;dt:(deltas rel)%OSES;vf:uvol rel%OSES;st:(OSIG sym)*(sqrt 0.0|dt)*vf*rn k;md:(OBP sym)*exp sums st;hs:0.01|round[2]0.5*md*(OQBPS sym)*(0.5+?k)%10000;bd:round[2]md-hs;ak:round[2]md+hs;ak:ak+0.01*~ak>bd;([]sym:k#sym;time:tm;bid:bd;ask:ak;bsz:100*1+k?9;asz:100*1+k?9)}
tgen1:{[sym;k]rel:_OSES*asc ushp(?k);tm:OOPEN+rel;sd:-1+2*(?k)<OPB sym;sz:100*1|(_ 0.5+exp 0.9*abs rn k)&40;bl:(?k)<0.03;sz:sz+bl*100*10*1+k?5;([]sym:k#sym;time:tm;sz:sz;side:sd)}
gentq:{[n]m:2*n;QB:raze qgen1'[OUNI;cnts m];QB:sortcol[`time;partcol[`sym;xasc[`sym`time;QB]]];TB:xasc[`sym`time;raze tgen1'[OUNI;cnts n]];MB:aj[`sym`time;TB;QB];bq:MB[`side]>0;md:0.5*MB[`bid]+MB`ask;imp:(?#bq)<0.12;px:round[2](imp*md)+(~imp)*(bq*MB`ask)+(~bq)*MB`bid;px:@[px;fi;:;(OBP MB`sym)fi:&^px];RB:([]sym:MB`sym;time:MB`time;px:px;sz:MB`sz);trades::sortcol[`time;partcol[`sym;xasc[`sym`time;RB]]];quotes::QB;`trades`quotes}
OEXP:7 14 21 30 45 60 90 120 180 365
bs:{[S;K;T;iv;ic]sqT:sqrt T;d1:(log[S%K]+(0.045+0.5*iv*iv)*T)%iv*sqT;d2:d1-iv*sqT;n1:ncdf d1;n2:ncdf d2;p1:npdf d1;dc:exp neg 0.045*T;cl:(S*n1)-K*dc*n2;pu:(cl-S)+K*dc;(0.0|round[2](ic*cl)+(~ic)*pu;round[4](ic*n1)+(~ic)*n1-1;round[6]p1%S*iv*sqT;round[4]S*p1*sqT%100;round[4]((neg[S*p1*iv]%2*sqT)+0.045*K*dc*(ic*neg n2)+(~ic)*1-n2)%365)}
genopt:{[n]u:n?OUNI;S:OBP u;dte:OEXP n?#OEXP;T:dte%365.0;K:2.5*_0.5+(S*0.70+0.60*?n)%2.5;rt:`C`P n?2;lm:log K%S;iv:0.05|3.0&(OIV u)*(1+2.0*lm*lm)*(1+0.03*sqrt T)*(0.9+0.2*?n);b:bs[S;K;T;iv;rt=`C];hs:0.05|round[2]0.03+0.02*b 0;vol:_ 1+(5000*exp neg 3*lm*lm)*?n;oi:_ vol*3+n?30;options::sortcol[`strike;partcol[`sym;xasc[`sym`strike;([]sym:u;expiry:dte;strike:K;right:rt;spot:S;iv:round[4]iv;price:b 0;bid:0.0|round[2](b 0)-hs;ask:round[2](b 0)+hs;delta:b 1;gamma:b 2;vega:b 3;theta:b 4;volume:vol;oi:oi)]]];`options}

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
ema:{[a;x]`ema(a;x)}                                           / exponential moving average, factor a in (0,1]  (C kernel, O(n) sweep)
rollstd:{[w;x]sqrt(movavg[w;x*x])-m*m:movavg[w;x]}     / rolling standard deviation

/ ---- per-symbol summary ----
symstats:{[t]qby[t;`sym;`n`vwap`hi`lo`vol!({#x};{round[4]wavg[x`sz;x`px]};{max x`px};{min x`px};{sum x`sz})]}

/ ---- pretty print: format any `time` column as HH:MM:SS.mmm ----
pt:{show $[in[`time;cols x];tsym[x;`time];x]}

