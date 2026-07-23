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

/ ---- .h : minimal HTML ---------------------------------------------------
htag:{[t;s]("<",t,">"),s,"</",t,">"}
h.hc:{[c;s]htag[c;s]}                       / wrap s in tag c
h.hrow:{[c;xs]htag["tr";,/htag[c]'$'xs]}    / a list -> <tr> of <td>/<th>
h.ht:{[t]htag["table";(h.hrow["th";cols t]),,/h.hrow["td"]'{[t;i]. t i}[t]'!#t]}  / table -> HTML
