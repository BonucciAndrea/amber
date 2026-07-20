\l amber.k
/ ============================================================================
/ examples/tick.k  -  realistic intraday trades & quotes, kdb+/tick style.
/ run from the amber folder:   ./amber examples/tick.k
/ or from the REPL (./a):      \l examples/tick.k
/ ============================================================================
hr:{`0:"";`0:x;`0:(#x)#"="}

/ ---- a quote stream: top-of-book bid/ask for 3 names, 09:30:00 .. 09:34 ----
qsym:`AAPL`MSFT`GOOG`AAPL`MSFT`AAPL`GOOG`MSFT`AAPL`GOOG
qt  :hms[9;30;0]+1000*0 5 8 60 62 150 155 200 250 258
bid :187.18 411.00 142.77 187.22 411.02 187.25 142.80 411.15 187.31 142.83
ask :187.22 411.08 142.82 187.27 411.09 187.29 142.86 411.24 187.36 142.90
quote:xasc[`sym`time; ([]sym:qsym; time:qt; bid:bid; ask:ask)]

/ ---- a trade stream ----
tsy:`AAPL`AAPL`MSFT`AAPL`MSFT`GOOG`AAPL`MSFT`GOOG`AAPL
tt :hms[9;30;0]+1000*10 55 65 95 130 160 190 205 260 250
tpx:187.20 187.25 411.10 187.30 411.05 142.80 187.28 411.20 142.85 187.35
tsz:100 250 80 150 60 40 300 90 120 200
trade:xasc[`sym`time; ([]sym:tsy; time:tt; px:tpx; sz:tsz)]

hr"TRADES"
show tsym[trade;`time]

hr"QUOTES"
show tsym[quote;`time]

hr"AS-OF JOIN: each trade tagged with the prevailing quote, + spread"
aj1:aj[`sym`time; trade; quote]
aj1:+(+aj1),`spread`mid!(round[4] aj1[`ask]-aj1`bid ; round[4] 0.5*aj1[`ask]+aj1`bid)
show tsym[aj1;`time]

hr"WINDOW JOIN: max ask & min bid over the 120s BEFORE each trade"
w:(trade[`time]-120000; trade`time)
wj1:wj[w; `sym`time; trade; quote; ((`maxAsk;max;`ask);(`minBid;min;`bid);(`nQ;count;`bid))]
show tsym[wj1;`time]

hr"VWAP and trade count by symbol"
show qby[trade; `sym; `vwap`trades`shares!({wavg[x`sz;x`px]};{#x};{sum x`sz})]

hr"1-MINUTE OHLCV BARS by symbol  (classic tick.minute bucketing)"
tb:+@[+trade; ,`time; minbar[1]@]
ohlc:qby[tb; `sym`time; `open`high`low`close`vol!({first x`px};{max x`px};{min x`px};{last x`px};{sum x`sz})]
show tsym[unkey ohlc; `time]

hr"done"
