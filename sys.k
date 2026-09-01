/ ============================================================================
/ sys.k  -  Amber system namespaces (.z .Q .j .h).  Loaded after temporal.k.
/ Clocks, utility functions, JSON, number formatting, a little HTML.
/ Times are computed from the microsecond wall clock `t[] (epoch 1970),
/ re-based to kdb's 2000.01.01 epoch via the temporal.k layer.
/ ============================================================================

/ ---- .z : clocks & handlers ----------------------------------------------
z.p:{1000*`t[]-946684800000000}          / now  -> timestamp (nanos since 2000.01.01)
z.P:{z.p[]}                              / local timestamp (no tz here)
z.n:{tsms z.p[]}                         / nanoseconds since midnight
z.d:{tsdate z.p[]}                       / today  -> date (days since 2000.01.01)
z.D:{z.d[]}
z.t:{fdiv[z.n[];1000000]}                / time now -> ms since midnight
z.T:{z.t[]}
z.z:{z.d[]+(z.t[]%86400000)}            / datetime (days.fraction)
z.w:{0}                                   / handle of current connection (0 = local)
z.ts:{}                                   / timer callback (set by user)
z.pg:{. x}                                / sync  message handler (evaluate)
z.ps:{. x}                                / async message handler (evaluate)
z.po:{}                                   / port-open handler
z.pc:{}                                   / port-close handler
z.exit:{}                                 / exit handler

/ ---- .Q : utilities ------------------------------------------------------
Q.n:"0123456789"
Q.a:"abcdefghijklmnopqrstuvwxyz"
Q.A:"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
Q.f:{[n;x]$round[n;x]}                     / format x to n decimals -> string
Q.fmt:{[w;n;x]w$$round[n;x]}               / format to width w, n decimals
Q.s:{"\n"/amfmt x}                         / show as a string
Q.ty:{@x}                                  / type of x
Q.qt:{`M=@x}                               / is x a table?
Q.id:{`$@[x;&~{((y>64)&y<91)|((y>96)&y<123)|((y>47)&y<58)|y=95}[;0+x]!#x;:;"_"]}  / sanitise a name
Q.dd:{`$($x),".",$y}                       / dotted join  Q.dd[`a;`b] -> `a.b
Q.gc:{0}                                   / garbage collect (no-op here)
Q.w:{`used`heap`peak!0 0 0}                / workspace memory (placeholder)
Q.fc:{[f;x]f x}                            / parallel-on-cut (sequential fallback)
Q.trp:{[f;a;h].[f;a;h]}                    / protected apply with handler

/ ---- .j : JSON -----------------------------------------------------------
jenc:{$[in[@x;`i`f];$x;`s=@x;"\"",($x),"\"";`c=@x;"\"",(,x),"\"";`C=@x;"\"",x,"\"";in[@x;`I`F];"[",(","/$'x),"]";`S=@x;"[",(","/{"\"",($x),"\""}'x),"]";`m=@x;"{",(","/{[d;k]("\"",($k),"\":"),jenc d k}[x]'!x),"}";`M=@x;"[",(","/{[t;i]jenc t i}[x]'!#x),"]";`A=@x;"[",(","/jenc'x),"]";$x]}
j.j:{jenc x}                               / value -> JSON string
j.k:{`j? x}                                / JSON string -> value

/ ---- terminal charting ----------------------------------------------------
/ Every chart funnels into `chart`, which takes a dictionary of options and
/ hands the `plt` kernel one fixed 10-slot spec.  Everything else in this
/ section is a preset that fills part of that dictionary in, so the feature has
/ exactly one place where a default lives and one place to extend.
/   chart `y`title!(v;"AAPL")   plot v   plots (a;b;c)   xyplot (xs;ys)
/   scatter (xs;ys)   step v   area v   hist v   barh `a`b!3 7   heat m   spark v
chE:`c$27
chrs:chE,"[0m"
chfg:{chE,"[38;2;",(";"/$x),"m"}            / truecolour foreground
chbg:{chE,"[48;2;",(";"/$x),"m"}            / truecolour background
chflr:{_x}                                  / floor as a named verb: a bare _ after a value is drop, not floor
chblk:{,/x#,"█"}                            / n whole blocks; n#"█" would take n BYTES of a 3-byte glyph
chsb:{3#(3*x)_"▁▂▃▄▅▆▇█"}                / one of the eight partial blocks
chatm:{"a"<*$@x}                            / type symbol lowercase -> atom, uppercase -> vector
chtxt:{$[`C=@x;x;`c=@x;,x;`s=@x;$x;""]}     / anything -> a char vector label
chwin:{v:.[{`bi 0};,0;!0];$[(7<#v)&(0<v 6)&0<v 7;(v 7;v 6);24 80]}   / (rows;cols); 24 80 when not a tty
chd1:{[k;v](,k)!,v}                         / a ONE-key dict: `k!v would make one entry per element of v
chser:{$[`A=@x;x;,x]}                       / one vector, or many -> always a list of series
/ The full option set; anything omitted takes the value below.
/   y  vector or list of vectors (one per series)   x  matching x coordinates, else the index
/   w h  size in character cells, 0 = fit the terminal
/   ylim xlim  (lo;hi) to pin an axis; omitted autoscales out to round numbers
/   grid  0 none, 1 horizontal gridlines, 2 horizontal and vertical
/   colour  0 off, 1 on, 2 = on only when stdout is a terminal
/   style  per series 0 line, 1 scatter, 2 step, 3 area    col  per series 256-colour code
chopt:`y`x`w`h`ylim`xlim`grid`axis`legend`colour`title`xlabel`ylabel`names`col`style!(();();0;0;();();1;1;1;2;"";"";"";();0#0;0#0)
chart:{[d]d:chopt,$[`m=@d;d;chd1[`y;d]];ys:`f$'chser d`y;xs:$[0=#d`x;();`f$'chser d`x];t:chwin[];w:$[d`w;d`w;20|160&((t 1)-14)];h:$[d`h;d`h;6|18&((t 0)-8)];nm:d`names;nm:$[0=#nm;();$[`S=@nm;$'nm;chtxt'nm]];yl:$[2=#d`ylim;`f$d`ylim;0n 0n];xl:$[2=#d`xlim;`f$d`xlim;0n 0n];`0:`plt (ys;xs;(w;h;d`grid;d`axis;$[#nm;d`legend;0];d`colour);`f$yl,xl;chtxt d`title;chtxt d`xlabel;chtxt d`ylabel;nm;d`col;d`style);}
/ plot keeps its original shapes -- plot v and plot (v;W;H) -- and now draws them
/ framed and labelled.  (v;W;H) is only taken when the trailing elements really
/ are atoms, so plot (a;b) still means two series rather than a series and a width.
plot:{$[`m=@x;chart `y`names!(value x;$'!x);~`A=@x;chart chd1[`y;x];~1<#x;chart chd1[`y;x];~chatm x 1;chart `y`names!(x;$'1+!#x);3=#x;chart `y`w`h!x;2=#x;chart `y`w!x;chart chd1[`y;x]]}
plots:{$[`m=@x;chart `y`names!(value x;$'!x);chart `y`names!(x;$'1+!#x)]}   / multi-series; a dict labels the legend
xyplot:{chart `x`y!(x 0;x 1)}               / y against a real x axis, not the index
scatter:{chart `x`y`style!(x 0;x 1;,1)}     / points, with no joining line
step:{chart `y`style!(x;,2)}                / piecewise constant
area:{chart `y`style!(x;,3)}                / filled down to the zero line
hbins:{[v;n]lo:&/v;wd:((|/v)-lo)%n;wd:$[wd>0;wd;1.0];ix:(n-1)&chflr (v-lo)%wd;(lo+wd*(0.5+!n);{[b;k]+/k=b}[ix]'!n)}
hist:{d:$[(`A=@x)&2=#x;x;(x;24)];b:hbins[`f$d 0;d 1];chart `x`y`style`ylabel!(b 0;`f$b 1;,3;"count")}   / hist v  or  hist (v;nbins)
/ barh, heat and spark are glyph charts rather than braille ones: they are for
/ labelled categories and matrices, where a line chart has nothing to say.
barh:{d:$[`m=@x;($'!x;value x);($'x 0;x 1)];lb:d 0;bv:`f$d 1;mx:$[#bv;|/bv;1.0];mx:$[mx>0;mx;1.0];lw:|/#'lb;`0:("\n"/{[lb;bv;mx;lw;i]((lw$lb i)," ",(chblk chflr 40*(bv i)%mx)," ",$bv i)}[lb;bv;mx;lw]'!#bv),"\n";}   / bv, not vs: vs is a library global ({x\y}), and |/vs would fold over the VERB
chhot:{$[x>0;(chflr 60+195*x;chflr 50+90*(1-x);55);(55;chflr 70+110*(1+x);chflr 60+195*(-x))]}
heat:{d:$[`m=@x;($'!x;value x);($'x 0;x 1)];lb:d 0;m:`f$'d 1;lo:&/(&/'m);hi:|/(|/'m);rg:$[hi>lo;hi-lo;1.0];lw:|/#'lb;`0:("\n"/{[lb;m;lo;rg;lw;i]((lw$lb i)," ",,/{chbg[chhot x],"  ",chrs}'(-1+2*((m i)-lo)%rg))}[lb;m;lo;rg;lw]'!#m),"\n";}
spark:{v:`f$x;lo:&/v;hi:|/v;rg:$[hi>lo;hi-lo;1.0];,/chsb'chflr 7.999*(v-lo)%rg}   / returns the string: it is meant to sit inside a row
candle:{x:unkey x;p:{[t;n;a;b]t@$[in[a;n];a;b]}[x;cols x];`0:`cdl (p[`open;`o];p[`high;`h];p[`low;`l];p[`close;`c]);}  / candlestick from OHLC table

/ ---- arrow : Apache Arrow C Data Interface (zero-dependency, C ABI) -------
arrow.export:{x:unkey x;`aex (cols x;value flip x)}   / table -> (schemaAddr;arrayAddr) 64-bit C ABI pointers
arrow.import:{d:`aim x;+(d 0)!d 1}                     / (schemaAddr;arrayAddr) -> Amber table

/ ---- .h : minimal HTML ---------------------------------------------------
htag:{[t;s]("<",t,">"),s,"</",t,">"}
h.hc:{[c;s]htag[c;s]}                       / wrap s in tag c
h.hrow:{[c;xs]htag["tr";,/htag[c]'$'xs]}    / a list -> <tr> of <td>/<th>
h.ht:{[t]htag["table";(h.hrow["th";cols t]),,/h.hrow["td"]'{[t;i]. t i}[t]'!#t]}  / table -> HTML
