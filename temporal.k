/ ============================================================================
/ temporal.k  -  a temporal-types layer for Amber.  Loaded after qsql.k.
/ Values are held numerically (kdb-style, epoch 2000.01.01):
/   date       = integer days since 2000.01.01   (2000.01.01 -> 0)
/   timestamp  = long nanoseconds since 2000.01.01
/ with constructors, formatting, parsing, arithmetic and day-of-week.
/ (Civil<->days uses Howard Hinnant's algorithm, epoch-shifted to 2000.)
/ ============================================================================

fdiv:{_(x%y)}                                               / integer floor-division

/ ---- date : days since 2000.01.01 ----------------------------------------
ymd2d:{[y;m;d]yy:y-m<3;era:fdiv[yy;400];yoe:yy-era*400;mp:m+$[m>2;-3;9];doy:(fdiv[2+153*mp;5])+d-1;+/((146097*era);365*yoe;fdiv[yoe;4];doy;neg fdiv[yoe;100];-730425)}
d2ymd:{[d2]z:d2+730425;era:fdiv[z;146097];doe:z-146097*era;yoe:fdiv[+/(doe;neg fdiv[doe;1460];fdiv[doe;36524];neg fdiv[doe;146096]);365];y:yoe+400*era;doy:doe-+/((365*yoe);fdiv[yoe;4];neg fdiv[yoe;100]);mp:fdiv[2+5*doy;153];dd:1+doy-fdiv[2+153*mp;5];m:mp+$[mp<10;3;-9];(y+m<3;m;dd)}
dstr:{[d]v:d2ymd d;($v 0),".",(z2 v 1),".",z2 v 2}         / date  -> "YYYY.MM.DD"
pdate:{ymd2d . `I$"."\x}                                    / "YYYY.MM.DD" -> date
dparts:{d2ymd x}                                            / date -> (year;month;day)
year:{(d2ymd x)0}
month:{(d2ymd x)1}
dayof:{(d2ymd x)2}
dow:{`Mon`Tue`Wed`Thu`Fri`Sat`Sun 7!x+5}                    / day of week (2000.01.01 = Sat)
dadd:{[d;n]d+n}                                             / add n days
ddiff:{x-y}                                                 / days between two dates

/ ---- timestamp : nanoseconds since 2000.01.01 ----------------------------
tstamp:{[d;ms]1000000*ms+86400000*d}                        / (date;ms-of-day) -> timestamp
tsdate:{fdiv[x;86400000000000]}                             / timestamp -> date
tsms:{(fdiv[x;1000000])-86400000*fdiv[x;86400000000000]}    / timestamp -> ms-of-day
pstr:{(dstr tsdate x),"D",stime tsms x}                     / timestamp -> "YYYY.MM.DDDHH:MM:SS.mmm"
