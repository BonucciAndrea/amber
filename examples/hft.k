\l amber.k
\l fin.k
/ ============================================================================
/ examples/hft.k  -  the most common HFT / market-microstructure workflows.
/ run:  ./amber examples/hft.k          (also a smoke test)
/ ============================================================================
hr:{`0:"";`0:"== ",x," ",(56-#x)#"=";}

hr"1. build a TAQ session (gentq sets globals: trades, quotes)"
gentq 2000
`0:"trades: ",($#trades),"   quotes: ",$#quotes
`0:"trades meta (note `s on time and `p on sym):"
show meta trades

hr"2. the order book: mid, spread, microprice, imbalance (first 5 quotes)"
pt 5#+(+quotes),`mid`spr`micro`imb!(mid quotes;spread quotes;round[3]micro quotes;imbal quotes)

hr"3. TAQ: attach the prevailing quote to every trade (as-of join)"
m:taq[trades;quotes]
m:qwhere[m;~^m`bid]          / drop trades that arrived before their first quote (null bid)
pt 5#m

hr"4. trade classification: tick rule sign, signed volume"
m:+(+m),`sign`sv!(tsign m;signedvol m)
pt 5#m
`0:"net signed volume (order-flow imbalance) by symbol:"
show qby[m;`sym;`buys`sells`net!({sum 0|x`sv};{sum 0&x`sv};{sum x`sv})]

hr"5. transaction-cost analysis: effective spread & spread in bps"
`0:"avg effective spread by symbol (in cents):"
show qby[+(+m),`es!,round[4]effspread m;`sym;(,`avgEff)!,{round[4]avg x`es}]
`0:"avg quoted spread (bps):"
show qby[+(+quotes),`sb!,spreadbps quotes;`sym;(,`bps)!,{round[3]avg x`sb}]

hr"6. VWAP / TWAP by symbol"
show qby[trades;`sym;`vwap`twap!({round[4]wavg[x`sz;x`px]};{round[4]avg x`px})]

hr"7. OHLCV bars: 1-minute and 5-minute"
`0:"1-minute bars (first 4):"
pt 4#unkey bars[1;trades]
`0:"5-minute bars (first 4):"
pt 4#unkey bars[5;trades]

hr"8. rolling signals on one symbol (AAPL) price series"
g:bysym trades
p:(symrows[trades;g;`AAPL])`px

`0:a:"px (first 8):"
8#round[2]p

`0:"ema[0.2] (first 8):"
8#round[2]ema[0.2;p]

`0:"movavg[10] (6..13):"
6_13#round[2]movavg[10;p]

`0:"rolling std[10] (last 5):"
-5#round[4]rollstd[10;p]

hr"9. O(1) per-symbol slice (group index)"
`0:"rows for AAPL via index: ",($#symrows[trades;g;`AAPL])," (same as a full scan)"

hr"10. minute order-flow imbalance for AAPL (signed volume per minute)"
a:+@[+symrows[trades;g;`AAPL];,`time;minbar[1]@]
a:+(+a),`sv!,signedvol taq[symrows[trades;g;`AAPL];quotes]
pt 6#unkey qby[a;`sym`time;(,`netflow)!,{sum x`sv}]

`0:"";`0:"HFT walkthrough complete."
