/ ============================================================================
/ temporal.k  -  a temporal-types layer for Amber.  Loaded after qsql.k.
/ Values are held numerically (q-style, epoch 2000.01.01):
/   date       = integer days since 2000.01.01   (2000.01.01 -> 0)
/   timestamp  = long nanoseconds since 2000.01.01
/ with constructors, formatting, parsing, arithmetic and day-of-week.
/ (Civil<->days uses Howard Hinnant's algorithm, epoch-shifted to 2000.)
/ ============================================================================

/ integer floor-division, exact: (-y)!x (a float x%y rounds above 2^53, and a timestamp is ns
/ since 2000, so 2026.07.30D23:59:59.999999999 printed as the next day). -8! and -9! are the
/ serialise verbs, so 8 and 9 go in two steps: floor(floor(x/a)/b) is floor(x/(a*b)).
fdiv:{$[~(`i~@y)&y>0;_(x%y);^`i`I?@x;_(x%y);[r:$[y=8;(-4)!(-2)!x;y=9;(-3)!(-3)!x;(-y)!x];$[`i~@x;$[^x;0N;r];@[r;&^x;:;0N]]]]}  / exact for ints over a positive int (-8! and -9! are other verbs); else as before

/ ---- date : days since 2000.01.01 ----------------------------------------
ymd2d:{[y;m;d]yy:y-m<3;era:fdiv[yy;400];yoe:yy-era*400;mp:(m-3)+12*m<3;doy:(fdiv[2+153*mp;5])+d-1;+/((146097*era);365*yoe;fdiv[yoe;4];doy;neg fdiv[yoe;100];-730425)}
d2ymd:{[d2]z:d2+730425;era:fdiv[z;146097];doe:z-146097*era;yoe:fdiv[+/(doe;neg fdiv[doe;1460];fdiv[doe;36524];neg fdiv[doe;146096]);365];y:yoe+400*era;doy:doe-+/((365*yoe);fdiv[yoe;4];neg fdiv[yoe;100]);mp:fdiv[2+5*doy;153];dd:1+doy-fdiv[2+153*mp;5];m:(mp+3)-12*mp>9;(y+m<3;m;dd)}
dstr:{[d]$[^d;:"0N";];v:d2ymd d;($v 0),".",(z2 v 1),".",z2 v 2}         / date  -> "YYYY.MM.DD"
pdate:{ymd2d . `I$"."\x}                                    / "YYYY.MM.DD" -> date
tdays:{$[`n~@x;tsdate x;`A~@x;$[`n~@*x;tsdate x;`i$x];`i$x]} / days since 2000.01.01 of a date, a timestamp (its day) or raw days
dparts:{d2ymd tdays x}                                       / date -> (year;month;day)  (native date atom or raw days)
year:{(d2ymd tdays x)0}                                      / .year
month:{(d2ymd tdays x)1}                                     / .month
dayof:{(d2ymd tdays x)2}                                     / .day
day:{(d2ymd tdays x)2}                                       / .day (alias)
dow:{`Mon`Tue`Wed`Thu`Fri`Sat`Sun 7!(tdays x)+5}             / day of week (2000.01.01 = Sat)
dadd:{[d;n]d+n}                                             / add n days
ddiff:{x-y}                                                 / days between two dates

/ ---- timestamp : nanoseconds since 2000.01.01 ----------------------------
tstamp:{[d;ms]1000000*ms+86400000*d}                        / (date;ms-of-day) -> timestamp
tsdate:{fdiv[`i$x;86400000000000]}                           / timestamp -> date
tsms:{(fdiv[x;1000000])-86400000*fdiv[x;86400000000000]}    / timestamp -> ms-of-day
tsus:{x-1000000*fdiv[x;1000000]}                            / timestamp -> its nanoseconds below the millisecond
pstr:{$[^`i$x;"0N";(dstr tsdate `i$x),"D",(stime tsms `i$x),-6$"00000",$tsus `i$x]}   / timestamp -> "YYYY.MM.DDDHH:MM:SS.nnnnnnnnn", every digit (issue #18)
ptstamp:{d:pdate(x?"D")#x;p:"."\1_(x?"D")_x;n:$[1<#p;`I$9#(p 1),"000000000";0];n+tstamp[d;ptime[*p]]} / "YYYY.MM.DDDHH:MM:SS.nnnnnnnnn" -> ns, every digit (for "P"$)

/ ---- native-type accessors on time atoms (extract raw ms, then decompose) ----
thh:{_(`i$x)%3600000}                                       / .hh  hour of day
tmm:{60!_(`i$x)%60000}                                      / .mm  minute of hour
tss:{60!_(`i$x)%1000}                                       / .ss  second of minute
