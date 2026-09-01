/ ============================================================================
/ graphs.k  -  Amber's terminal charting, end to end.
/   run:  ./amber examples/graphs.k      (or  \l examples/graphs.k  in the REPL)
/ 
/ Part 1  the chart verbs            plot plots xyplot scatter step area
/ Part 2  categorical & glyph charts hist barh heat spark candle
/ Part 3  options                    size, zoom, grid, labels, colour, style
/ Part 4  charting real series       prices, moving averages, volatility
/ Part 5  building a picture by hand a braille canvas and a Mandelbrot set
/ 
/ Every chart in parts 1-4 goes through `chart`, which takes a dictionary of
/ options; the verbs are presets over it.  `\help chart` lists the keys.
/ ============================================================================
\l amber.k
\l fin.k
\l std.k
\l qsql.k
\l temporal.k
\l sys.k

hd:{`0:"\n",(74#"-"),"\n  ",x,"\n",(74#"-"),"\n"}      / section header

/ ============================ Part 1: the chart verbs =======================

hd"1. plot -- a numeric vector, framed and labelled"
plot (10*{sin x%6}@!120)

hd"2. plot (v;W;H) -- the same, at a chosen size"
plot (10*{sin x%6}@!120;56;10)

hd"3. plots -- several series at once, legend from the dict keys"
plots `sin`cos`ramp!({sin x%9}@!150;{cos x%9}@!150;0.01*!150)

hd"4. plots on a plain list -- series are numbered 1,2,3"
plots ({sin x%7}@!150;{sin x%11}@!150)

hd"5. xyplot -- y against a REAL x axis, so the curve may double back"
tt:0.01*!640
xyplot ({sin x}@tt;{cos x}@tt)

hd"6. xyplot -- a Lissajous figure (3:2), impossible to draw against an index"
tt:0.0004*!15800
xyplot ({sin 3*x}@tt;{sin 2*x}@tt)

hd"7. scatter -- points with no joining line"
`prng 3
scatter (100?1.0;(100?1.0))

hd"8. step -- piecewise constant, for anything that only changes on an event"
step 12?10

hd"9. area -- filled down to the zero line"
area {sin x%9}@!90

hd"10. 200,000 points -- more points than pixel columns, so each column is drawn"
hd"    as its own min/max ENVELOPE: no smear, and every spike survives"
`prng 7
plot sums -0.5+200000?1.0

/ ==================== Part 2: categorical and glyph charts ==================

hd"11. hist -- a distribution; the bin edges are round because the axis snaps them"
`prng 5
hist ((20000?1.0)+(20000?1.0)+20000?1.0;24)

hd"12. hist v -- 24 bins by default (sum of 12 uniforms -> gaussian)"
`prng 11
hist `f${+/12?1.0}@'!8000

hd"13. barh -- labelled horizontal bars, for categories rather than a series"
barh `AAPL`MSFT`NVDA`AMZN`GOOG`META!128.4 302.1 447.8 92.2 141.7 66.3

hd"14. heat -- a matrix as truecolour cells; warm = high, cold = low"
`prng 3
hf:{-0.5+200?1.0}@'!5
hm:{[i](0.65*hf 0)+0.35*hf i}@'!5
heat `AAPL`MSFT`NVDA`AMZN`GOOG!{[i]{[a;b]cor[hm a;hm b]}[i;]@'!5}@'!5

hd"15. spark -- returns a STRING, so it drops into a row of text"
`prng 11
spk1:{[sy]pv:100+sums -0.5+60?1.0;,/(6$$sy;" ";spark pv;"  last ";$0.01*_0.5+100**|pv)}
`0:("\n"/spk1@'`AAPL`MSFT`NVDA`AMZN`GOOG`META`TSLA),"\n"

/ ============================== Part 3: options =============================

hd"16. title, axis labels and a gridline on every y tick"
chart `y`title`xlabel`ylabel`grid!({(exp neg x%40)*sin x%5}@!160;"damped oscillation";"sample";"amplitude";1)

hd"17. grid:2 adds the vertical gridlines too"
chart `y`title`grid!({sin x%9}@!150;"grid:2";2)

hd"18. ylim -- pin the axis to zoom in; the data is the same as 17"
chart `y`title`ylim`h!({sin x%9}@!150;"ylim -0.2 0.2";-0.2 0.2;10)

hd"19. xlim -- window the x axis instead"
chart `y`title`xlim!({sin x%9}@!150;"xlim 40 80";40 80)

hd"20. w and h -- any size; the default fits the terminal"
chart `y`w`h`title!({sin x%5}@!200;100;8;"wide and short")

hd"21. col and style -- per-series colour (256-colour codes) and shape"
chart `y`names`col`style`title!(({sin x%9}@!150;{cos x%9}@!150);("sin";"cos");201 51;0 2;"custom colour and a step series")

hd"22. axis:0 -- the bare braille canvas, for embedding somewhere else"
chart `y`axis`h!({sin x%6}@!120;0;8)

/ ======================= Part 4: charting real series =======================

gentq 6000
px:trades`px

hd"23. AAPL trade prices"
chart `y`title`ylabel!(100_px;"AAPL trades";"price")

hd"24. price with its 50-tick moving average, aligned by giving each series its"
hd"    own x -- the average is shorter, so plotting both against the index lies"
ma:50_movavg[50;px]
chart `y`x`names`title!((px;ma);(!#px;50+!#ma);("price";"MA(50)");"AAPL: price and moving average")

hd"25. 50-tick rolling volatility"
chart `y`title`ylabel`style!(50_rollstd[50;px];"AAPL rolling volatility";"stdev";,3)

hd"26. OHLC candlesticks -- green up, red down"
candle bars[10;sel"select from trades where sym=`AAPL"]

hd"27. wider candlesticks (5-minute bars, MSFT)"
candle bars[5;sel"select from trades where sym=`MSFT"]

/ ==================== Part 5: building a picture by hand =====================
/ Everything above is a verb.  Everything below is ordinary Amber, and shows
/ what the same braille and block glyphs can do when you drive them yourself.

E:`c$27
FG:{E,"[38;2;",(";"/$x),"m"}                            / truecolour foreground
BG:{E,"[48;2;",(";"/$x),"m"}                            / truecolour background
RS:E,"[0m"
BR:{`c$226,(160+_x%64),128+64!x}                        / braille glyph, 8-dot mask
PW:1 2 4 8 16 32 64 128
NM:{$[(|/x)=&/x;0*x;(x-&/x)%(|/x)-&/x]}                 / normalise to 0..1

/ ---- a general 2-D braille canvas -----------------------------------------
/ Each cell is 2x4 dots, so a WxH box draws at 2W x 4H.  Points are scattered
/ into a per-cell dot mask, then one pass turns masks into glyphs.  This is
/ what xyplot does natively; it is written out here because the same scatter
/ is the basis of any raster you want to build.
CV:{[xs;ys;W;H]
  dc:_(NM xs)*-1+2*W; dr:_(1-NM ys)*-1+4*H;
  ci:(W*_dr%4)+_dc%2; bx:2!dc; by:4!dr; w:by=3;
  bp:(w*6+bx)+(~w)*by+3*bx;
  / @[x;i;|;y] is LAST-WINS on repeated i, which would throw most dots away.
  / @[x;i;+;y] does accumulate, so dedupe the (cell,bit) pairs first and add:
  / each bit then contributes exactly once and the sum IS the OR.
  u:distinct (8*ci)+bp;
  "\n"/{,/BR@'x}@'(H;W)#@[(W*H)#0;_u%8;+;PW 8!u]}

hd"28. rose curve  r = cos 5t  -- hand-built canvas"
tt:0.0002*!31500
`0:CV[{(cos 5*x)*cos x}@'tt;{(cos 5*x)*sin x}@'tt;44;22],"\n"   / W=2H keeps the dots square

hd"29. logarithmic spiral  r = e^(0.15t)"
tt:0.0006*!42000
`0:CV[{(exp 0.15*x)*cos x}@'tt;{(exp 0.15*x)*sin x}@'tt;44;22],"\n"

/ ---- Mandelbrot, half-block truecolour ------------------------------------
/ Two tricks make this look like a picture rather than ASCII art.
/   (a) The escape loop is VECTORISED: zr, zi and the counter are three flat
/       vectors over the whole raster, and one iteration is four multiplies
/       across all of it.  The mask m freezes a pixel the instant it escapes --
/       note every (m*a) is parenthesised, because k reads right to left and
/       m*a+b would mean m*(a+b).
/   (b) The glyph is the upper half block: the FOREGROUND paints the top pixel
/       and the BACKGROUND the bottom one, so a character cell carries TWO
/       pixels and the image gets twice the vertical resolution.
/ Smooth colouring uses the fractional escape count k+1-log2(log|z|), which
/ turns the iteration bands into a continuous ramp, fed through a cosine
/ palette so there is no banding anywhere.
MB:{[v;W;H;N]
  xs:(v 0)+((v 1)-v 0)*(!W)%W-1;
  cr::,/H#,xs;
  ci::,/{y#x}[;W]@'(v 2)+((v 3)-v 2)*(!H)%H-1;
  s:{zr:x 0;zi:x 1;m:4>(zr*zr)+zi*zi;
     (((m*cr+(zr*zr)-zi*zi)+(~m)*zr);((m*ci+2*zr*zi)+(~m)*zi);(x 2)+m)}/[N;(0*cr;0*cr;0*cr)];
  nu:(s 2)+1-(log log 1.0000001|((s 0)*s 0)+(s 1)*s 1)%log 2;
  (N&0|nu)%N}
CP:{[t;i]_255*(t<0.999)*0.5+0.5*cos 6.28318*(0.33*i)+3*t}   / cosine palette, black in the set
CEL:{FG[(R0 x;G0 x;B0 x)],BG[(R0 y;G0 y;B0 y)],"▀"}
ROW:{a:((2*x)*W0)+!W0;b:(((2*x)+1)*W0)+!W0;,/CEL'[a;b]}
FRAC:{[v;W;H;N]W0::W;tv:MB[v;W;H;N];R0::CP[tv;0];G0::CP[tv;1];B0::CP[tv;2];
  ("\n"/{RS,ROW x}@'!_H%2),RS}

hd"30. Mandelbrot set -- smooth escape colouring, two pixels per character"
`0:FRAC[-2.15 0.75 -1.22 1.22;100;44;90],"\n"

hd"31. zoom on the seahorse valley -- same code, a different window"
`0:FRAC[-0.78 -0.72 -0.14 -0.10;100;44;220],"\n"

`0:"\ndone.  1-27 are chart verbs; 28-31 build the picture in Amber itself.\n"
