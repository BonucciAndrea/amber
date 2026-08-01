/ ============================================================================
/ graphs.k  -  a tour of Amber's terminal graphing (plot + candle).
/ Braille line charts and Unicode candlesticks straight in the REPL.
/   run:  ./amber examples/graphs.k       (or  \l examples/graphs.k  in the REPL)
/ ============================================================================
\l amber.k
\l fin.k
\l std.k
\l qsql.k
\l temporal.k
\l sys.k

hd:{`0:"\n",(60#"-"),"\n  ",x,"\n",(60#"-"),"\n"}      / section header

/ ---- 1. trigonometric curves ---------------------------------------------
hd"1. sine wave    y = 10*sin(x/6)"
plot (10*{sin x%6}'!120;100;16)

hd"2. damped oscillation    y = e^(-x/40)*sin(x/5)"
plot ({(exp neg x%40)*sin x%5}'!160;100;16)

/ ---- 2. random processes --------------------------------------------------
hd"3. random walk    cumulative sum of centred noise"
plot (sums -0.5+400?1.0;100;16)

hd"4. random walk + EMA(0.05) smoothing overlaid"
rw:sums -0.5+400?1.0
plot (ema[0.05;rw];100;16)

hd"5. beat pattern    sin(x/5) + sin(x/5.4)  (two close frequencies)"
plot (({(sin x%5)+sin x%5.4}'!220);100;14)

/ ---- 3. deterministic chaos ----------------------------------------------
hd"6. logistic map    x' = 3.9*x*(1-x)   (route to chaos)"
plot (120{3.9*x*1-x}\0.4;100;16)

/ ---- 4. distributions -----------------------------------------------------
hd"7. central limit: sorted sums of 12 uniforms  ->  gaussian CDF"
plot (asc`f${+/12?1.0}'!300;100;14)

hd"8. sorted uniform sample  ->  linear CDF"
plot (asc 300?1.0;100;12)

/ ---- 5. financial series (simulated ticks) --------------------------------
gentq 6000
px:trades`px
hd"9. AAPL trade prices"
plot (100_px;110;16)

hd"10. price with 50-tick moving average (MA is the smooth line)"
plot (50_movavg[50;px];110;16)

hd"11. 50-tick rolling volatility (rolling std of price)"
plot (50_rollstd[50;px];110;14)

/ ---- 6. candlesticks ------------------------------------------------------
hd"12. OHLC candlesticks    green = up, red = down"
candle bars[10;sel"select from trades where sym=`AAPL"]

hd"13. wider candlesticks (5-minute bars, MSFT)"
candle bars[5;sel"select from trades where sym=`MSFT"]

`0:"\ndone.  every chart above was drawn with  plot  or  candle.\n"
