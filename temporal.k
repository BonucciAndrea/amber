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
pdate:{`i$"D"$x}                                            / "YYYY.MM.DD" -> date, as "D"$ reads it
/ tdays: a date or a list of them takes `i$, a timestamp its day, an int or int list is raw days (0N the null day); a general
/ list that mixes dates and timestamps, or holds lists or dicts of them, goes item by item, a dict by its values, and in it an
/ int 0N or :: (as D,0N, prev D and D@5 make) is the null day. Anything else, a time, a float, a symbol or text, or a list that
/ mixes dates and ints, is 'type, as q's `year$ (which takes no ints at all). dkind: the kinds in a list, `d for a date or a
/ timestamp, `i for an int, none for 0N or ::, `x for anything else, from the items' types, going into lists and dicts only
/ (x~\:(::) on the whole list: indexing a general list does not keep a :: or a verb in it, (::;::)@0 1 is 0 1).
/ tdnul: a list of dates (or of timestamps) with nulls in it takes one `i$ (tsdate) with 0N for them; a mixed one goes item by item.
dkind:{$[`m~@x;dkind@.x;~`A~@x;$[(@x)in`i`I;,`i;,`x];[t:@'x;u:?t;k:(`d`d`i`x)@3^`d`n`I?u@&~u in`i`u`A`m;i:&t=`i;w:&t=`u;k,:$[#i;$[|/~^x i;,`i;0#`];0#`];k,:$[#w;$[&/(x~\:(::))w;0#`;,`x];0#`];?k,,/dkind'x@&t in`A`m]]}
tdnul:{[x;t]n:~t in`d`n`A`m;y:@[x;&n;:;0N];u:t@&~n;$[&/`d=u;`i$y;&/`n=u;tsdate y;{$[(::)~x;0N;tdays x]}'x]}
tdays:{$[`d~@x;`i$x;`n~@x;tsdate x;(@x)in`i`I;x;`m~@x;(!x)!tdays@.x;~`A~@x;`err"type";~#x;!0;&/`d=t:@'x;`i$x;&/`n=t;tsdate x;[k:dkind x;$[&/k=`d;tdnul[x;t];~&/k=`i;`err"type";|/n:x~\:(::);@[x;&n;:;0N];x]]]}
dtab:{$[`M~@x;1;`m~@x;`M~@!x;0]}                             / a table or a keyed table: the accessors go column by column, a keyed table on its value (keeping its key), as q's
dnul:{[d;v](0N*n)+(~n:^d)*v}                                 / v where the day d is not null, else 0N
dpart:{[i;x]$[dtab x;$[`M~@x;+dpart[i]@+x;(!x)!dpart[i;.x]];[d:tdays x;$[#d;dnul[d;(d2ymd d*~^d)i];d]]]}   / the year (0), month (1) or day (2) of a date; a null day gives 0N
dparts:{dpart[;x]'0 1 2}                                     / date -> (year;month;day)  (native date atom or raw days)
year:{dpart[0;x]}                                            / .year
month:{dpart[1;x]}                                           / .month
dayof:{dpart[2;x]}                                           / .day
day:{dpart[2;x]}                                             / .day (alias)
dow:{$[dtab x;$[`M~@x;+dow@+x;(!x)!dow@.x];[d:tdays x;`Mon`Tue`Wed`Thu`Fri`Sat`Sun dnul[d;7!(d*~^d)+5]]]}   / day of week (2000.01.01 = Sat); a null day gives `
dadd:{[d;n]d+n}                                             / add n days
ddiff:{x-y}                                                 / days between two dates

/ ---- timestamp : nanoseconds since 2000.01.01 ----------------------------
tstamp:{[d;ms]1000000*ms+86400000*d}                        / (date;ms-of-day) -> timestamp
tsdate:{fdiv[`i$x;86400000000000]}                           / timestamp -> date
tsms:{(fdiv[x;1000000])-86400000*fdiv[x;86400000000000]}    / timestamp -> ms-of-day
tsus:{x-1000000*fdiv[x;1000000]}                            / timestamp -> its nanoseconds below the millisecond
pstr:{$[^`i$x;"0N";(dstr tsdate `i$x),"D",(stime tsms `i$x),-6$"00000",$tsus `i$x]}   / timestamp -> "YYYY.MM.DDDHH:MM:SS.nnnnnnnnn", every digit (issue #18)
ptstamp:{`i$"P"$x}                                          / "YYYY.MM.DDDHH:MM:SS.nnnnnnnnn" -> ns, every digit, as "P"$ reads it

/ ---- native-type accessors on time atoms (extract raw ms, then decompose) ----
thh:{hh `i$x}                                               / .hh  hour of day (0 to 23; amber.k's hh)
tmm:{mm `i$x}                                               / .mm  minute of hour (amber.k's mm)
tss:{sec `i$x}                                              / .ss  second of minute (amber.k's sec)
