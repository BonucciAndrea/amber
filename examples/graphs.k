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

/ ============================================================================
/ Beyond plot and candle: a 2-D braille canvas, a truecolour raster, and
/ chart types built out of block glyphs.  All of it is Amber -- no library.
/ ============================================================================

E:`c$27
FG:{E,"[38;2;",(";"/$x),"m"}                            / truecolour foreground
BG:{E,"[48;2;",(";"/$x),"m"}                            / truecolour background
RS:E,"[0m"
BR:{`c$226,(160+_x%64),128+64!x}                        / braille glyph, 8-dot mask
PW:1 2 4 8 16 32 64 128
NM:{$[(|/x)=&/x;0*x;(x-&/x)%(|/x)-&/x]}                 / normalise to 0..1
SB:{3#(3*x)_"▁▂▃▄▅▆▇█"}                                 / one of 8 block glyphs

/ ---- a general 2-D braille canvas -----------------------------------------
/ Each cell is 2x4 dots, so a WxH box draws at 2W x 4H.  Points are scattered
/ into a per-cell dot mask with OR, then one pass turns masks into glyphs.
/ This is what plot cannot do: plot draws y against its INDEX, so it can never
/ show a curve that doubles back on itself.
CV:{[xs;ys;W;H]
  dc:_(NM xs)*-1+2*W; dr:_(1-NM ys)*-1+4*H;
  ci:(W*_dr%4)+_dc%2; bx:2!dc; by:4!dr; w:by=3;
  bp:(w*6+bx)+(~w)*by+3*bx;
  / @[x;i;|;y] is LAST-WINS on repeated i, which would throw most dots away.
  / @[x;i;+;y] does accumulate, so dedupe the (cell,bit) pairs first and add:
  / each bit then contributes exactly once and the sum IS the OR.
  u:distinct (8*ci)+bp;
  "\n"/{,/BR'x}'(H;W)#@[(W*H)#0;_u%8;+;PW 8!u]}

hd"14. Lissajous 3:2 -- a closed curve, which plot cannot draw"
tt:0.0004*!15800
`0:CV[{sin 3*x}'tt;{sin 2*x}'tt;44;22],"\n"   / W=2H keeps the dots square

hd"15. rose curve    r = cos 5t"
tt:0.0002*!31500
`0:CV[{(cos 5*x)*cos x}'tt;{(cos 5*x)*sin x}'tt;44;22],"\n"

hd"16. logarithmic spiral    r = e^(0.15t)"
tt:0.0006*!42000
`0:CV[{(exp 0.15*x)*cos x}'tt;{(exp 0.15*x)*sin x}'tt;44;22],"\n"

/ ---- 17. Mandelbrot, half-block truecolour --------------------------------
/ Two tricks make this look like a picture rather than ASCII art.
/   (a) The escape loop is VECTORISED: zr, zi and the counter are three flat
/       vectors over the whole raster, and one iteration is four multiplies
/       across all of it.  The mask m freezes a pixel the instant it escapes --
/       note every (m*a) is parenthesised, because k reads right to left and
/       m*a+b would mean m*(a+b).
/   (b) The glyph is ▀ (upper half block): the FOREGROUND paints the top pixel
/       and the BACKGROUND the bottom one, so a character cell carries TWO
/       pixels and the image gets twice the vertical resolution.
/ Smooth colouring uses the fractional escape count k+1-log2(log|z|), which
/ turns the iteration bands into a continuous ramp, fed through a cosine
/ palette so there is no banding anywhere.
MB:{[v;W;H;N]
  xs:(v 0)+((v 1)-v 0)*(!W)%W-1;
  cr::,/H#,xs;
  ci::,/{y#x}[;W]'(v 2)+((v 3)-v 2)*(!H)%H-1;
  s:{zr:x 0;zi:x 1;m:4>(zr*zr)+zi*zi;
     (((m*cr+(zr*zr)-zi*zi)+(~m)*zr);((m*ci+2*zr*zi)+(~m)*zi);(x 2)+m)}/[N;(0*cr;0*cr;0*cr)];
  nu:(s 2)+1-(log log 1.0000001|((s 0)*s 0)+(s 1)*s 1)%log 2;
  (N&0|nu)%N}
CP:{[t;i]_255*(t<0.999)*0.5+0.5*cos 6.28318*(0.33*i)+3*t}   / cosine palette, black in the set
CEL:{FG[(R0 x;G0 x;B0 x)],BG[(R0 y;G0 y;B0 y)],"▀"}
ROW:{a:((2*x)*W0)+!W0;b:(((2*x)+1)*W0)+!W0;,/CEL'[a;b]}
FRAC:{[v;W;H;N]W0::W;tv:MB[v;W;H;N];R0::CP[tv;0];G0::CP[tv;1];B0::CP[tv;2];
  ("\n"/{RS,ROW x}'!_H%2),RS}

hd"17. Mandelbrot set -- smooth escape colouring, two pixels per character"
`0:FRAC[-2.15 0.75 -1.22 1.22;100;44;90],"\n"

hd"17b. zoom on the seahorse valley -- same code, a different window"
`0:FRAC[-0.78 -0.72 -0.14 -0.10;100;44;220],"\n"

/ ---- 18. sparklines --------------------------------------------------------
/ NOTE: these build ONE string and print it with a single `0:.  A `0: inside a
/ lambda that is being applied with ' does not reach the terminal here.
hd"18. per-symbol sparklines -- one row each, eight block glyphs"
`prng 11
/ NOTE: the parameter is `sy` and the local `pv`, not `s` and `v`.  Short names
/ collide with globals the loaded libraries already define, and the collision is
/ SILENT: the row still prints, but the pieces that referenced them come out empty.
SPK:{[sy]pv:100+sums -0.5+60?1.0;,/(FG[150 142 130];6$$sy;RS;FG[93 219 160];,/SB'_7.999*NM pv;RS;FG[110 110 120];"   last ";$0.01*_0.5+100**|pv;RS)}
`0:("\n"/SPK'`AAPL`MSFT`NVDA`AMZN`GOOG`META`TSLA),"\n"

/ ---- 19. correlation heatmap ----------------------------------------------
hd"19. correlation heatmap -- truecolour cells, warm = positive"
`prng 3
hsym:`AAPL`MSFT`NVDA`AMZN`GOOG
hf:{-0.5+200?1.0}'!5
hm:{[i](0.65*hf 0)+0.35*hf i}'!5
`0:"        ",(,/{7$$x}'hsym),"\n"
HCEL:{,/(BG[$[x>0;(_60+195*x;_50+90*1-x;55);(55;_70+110*1+x;_60+195*-x)]];FG[16 14 10];7$" ",$0.01*_0.5+100*x;RS)}
HROW:{,/(FG[150 142 130];8$$hsym x;RS;,/HCEL'{[i;j]cor[hm i;hm j]}[x;]'!5)}
`0:("\n"/HROW'!5),"\n"

/ ---- 20. order-book depth --------------------------------------------------
hd"20. order-book depth -- cumulative size either side of the touch"
`prng 5
bpx:100.0-0.01*1+!14; apx:100.01+0.01*!14
bqt:sums 200+14?800; aqt:sums 150+14?800; wq:|/bqt|aqt
/ NOTE the bid bar is right-aligned by PADDING, not by reversing: | reverses
/ BYTES, and "█" is three of them, so |"███" is mojibake rather than "███".
/ n#"█" takes n BYTES of a three-byte glyph, so it cycles and produces mojibake.
/ ,/n#,"█" takes n copies of the whole glyph, which is what a bar actually wants.
BLK:{,/x#,"█"}
BAR:{((28-x)#" "),BLK x}
DROW:{,/(FG[110 110 120];8$$0.01*_0.5+100*bpx x;RS;FG[93 219 160];BAR[_28*(bqt x)%wq];RS;FG[110 110 120];9$$0.01*_0.5+100*apx x;RS;FG[255 107 107];BLK[_28*(aqt x)%wq];RS)}
`0:("\n"/DROW'!14),"\n"

/ ---- 21. histogram ---------------------------------------------------------
hd"21. histogram -- 20000 draws, sum of three uniforms (Irwin-Hall)"
`prng 13
hv:(20000?1.0)+(20000?1.0)+20000?1.0
hh:{[k]+/k=_19.999*NM hv}'!20
HIROW:{,/(FG[110 110 120];7$$0.01*_0.5+100*0.15*x;RS;FG[255 176 32];BLK[_56*(hh x)%|/hh];RS)}
`0:("\n"/HIROW'!20),"\n"

`0:"\ndone.  1-13 use plot and candle; 14-21 build the picture in Amber itself.\n"
