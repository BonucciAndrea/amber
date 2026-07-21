\l amber.k
\l fin.k
r:()
t:{r,:,($x;$[y~z;"ok";"FAIL got<",($y),"> exp<",($z),">"])}

/ ---- attributes u / p / g (C-level) ----
t[`attrS;`at(`sa 1 2 3);`s]
t[`attrU;`at(`ua 3 1 2);`u]
t[`attrP;`at(`pa 1 1 2 2);`p]
t[`attrG;`at(`ga 3 1 2);`g]
t[`attrNone;`at 1 2 3;`]
t[`partedFind;(`pa 10 10 20 20 30)?20;2]
t[`partedCorrect;(`pa 10 20 30)?10 20 30 40;0 1 2 0N]

/ ---- deterministic analytics ----
q:([]sym:`a`a;time:1 2;bid:10.0 20.0;ask:12.0 24.0;bsz:100 300;asz:100 100)
t[`mid;mid q;11 22.0]
t[`spread;spread q;2 4.0]
t[`imbal;imbal q;0 0.5]
t[`micro;micro q;11 23.0]
tt:([]px:10.0 20.0;sz:1 3)
t[`vwap;vwap tt;17.5]
t[`twap;twap tt;15.0]
mq:([]px:11.0 13.0 9.0;bid:10.0 10.0 10.0;ask:12.0 12.0 12.0)
t[`tsign;tsign mq;0 1 -1]
t[`effspread;effspread mq;0 4 4.0]
t[`signedvol;signedvol([]px:11.0 13.0 9.0;bid:10 10 10.0;ask:12 12 12.0;sz:100 100 100);0 100 -100]
t[`movavg;movavg[2;1 2 3 4];0n 1.5 2.5 3.5]
t[`movsum;movsum[2;1 2 3 4];0n 3 5 7]
t[`ema1;ema[1.0;1 2 3];1 2 3.0]
t[`ema;ema[0.5;2.0 4.0];2 3.0]
t[`logret;1_logret 100.0 100.0 100.0;0 0.0]

/ ---- structural: generator, attributes, joins on proper tables ----
gentq 200
t[`nTrades;#trades;200]
t[`nQuotes;#quotes;400]
t[`tradesTable;@trades;`M]
t[`quotesTable;@quotes;`M]
t[`symAttr;`at trades`sym;`p]
t[`timeAttr;`at trades`time;`s]
t[`quoteSymAttr;`at quotes`sym;`p]
t[`quoteTimeAttr;`at quotes`time;`s]
t[`timeNumeric;@trades`time;`I]
aj1:aj[`sym`time;trades;quotes]
t[`ajTable;@aj1;`M]
t[`ajHasQuote;in[`bid;cols aj1];1]
t[`ajRows;#aj1;200]
t[`barsKeyed;iskeyed bars[5;trades];1]
t[`symstats;#symstats trades;10]

/ ---- O(1) group index correctness ----
g:bysym trades
t[`idxCorrect;(symrows[trades;g;`AAPL])~qwhere[trades;trades[`sym]=`AAPL]]

fails:r@&{"FAIL"~4#x}'r[;1]
`0:"";`0:"============ FIN / HFT TEST SUITE ============"
`0:(($#r)," tests run, ",($#fails)," failures")
$[#fails;{`0:"  ",(x 0),": ",x 1}'fails;`0:"ALL FIN TESTS PASSED"];
`0:"============================================="
