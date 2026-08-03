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

/ ---- 7. big series: plot decimates a million points to fit the screen -------
/ A raw million-point line overplots every column into a solid blob.  plot folds
/ the samples onto the pixel columns (min/max/mean per column) so it stays clean
/ and fast.  mode: 0 = centre line[default], 1 = high/low band, 2 = both.
big:(1000+sums -0.5+1000000?1.0)+8*(-0.5+1000000?1.0)   / 1e6-tick noisy price
hd"14. one million points, centre line (default, mode 0)"
plot (big;100;16)
hd"15. same series, high/low envelope (mode 1)"
plot (big;100;16;1)

/ ---- 8. x-axis: every chart now shows a labelled x-axis ---------------------
/ pass (X;Y;...) to label ticks with your own x-values (numeric or strings).
hd"16. x-axis from a numeric X vector (hours of the day)"
hrs:9.5+0.02*!390                                       / 09:30 .. 16:00
plot (hrs;100+sums -0.5+390?1.0;100;12)

hd"17. x-axis with string time labels (time-aware ticks)"
tk:$'`$("09:30";"10:15";"11:00";"11:45";"12:30";"13:15";"14:00";"14:45";"15:30")
plot (tk;100+{sin x%3}'!9;90;9)

/ ---- 9. multi-series overlay + legend --------------------------------------
/ pass a list of series -> each drawn in its own colour with a legend.
hd"18. three series overlaid, colour-coded, with a legend"
px:100+sums -0.5+300?1.0
plot ((px;movavg[20;px];movavg[60;px]);100;16)

`0:"\ndone.  every chart above was drawn with  plot  or  candle.\n"
