/ ============================================================================
/ wj.k  -  window join demo.  For every trade, summarise the prevailing quotes
/ over a trailing time window (last N seconds), grouped per symbol.  This is the
/ classic TAQ "quote pressure around each trade" question.  Times are ms since
/ midnight but print as HH:MM:SS.mmm automatically (they live in a `time column).
/   run:  ./amber examples/wj.k
/ ============================================================================
\l amber.k
\l fin.k

gentq 5000                                       / sets globals: trades, quotes

/ trailing window: from WIN ms before each trade, up to the trade time itself.
WIN:60000                                        / 60-second look-back
tt:trades`time
w:(tt-WIN;tt)                                    / w=(begins;ends), one pair per trade row

/ per-window quote aggregates.  each agg is (name; fn; quote-column):
aggs:((`bid;last;`bid);(`ask;last;`ask);(`hi;max;`ask);(`lo;min;`bid);(`nq;{#x};`bid))
r:wj[w;`sym`time;trades;quotes;aggs]             / trade cols + the 5 window aggs

`0:"Window join: each trade + quote stats over the trailing ",($WIN%1000)," s\n"
`0:"trades=",($#trades),"  quotes=",($#quotes),"\n"
`0:"----------------------------------------------------------------\n"

/ keep the trades that had at least one quote in-window, and tidy the floats
m:r@&r[`nq]>0
m:+@[+m;`bid`ask`hi`lo;round[2]]
m:+@[+m;`mid;:;round[2]0.5*m[`bid]+m`ask]        / window mid-price
m:+@[+m;`edge;:;round[2]m[`px]-m`mid]            / trade px minus window mid (signed)
show 12#`sym`time`px`sz`bid`ask`hi`lo`nq`mid`edge#m

`0:"\nper-symbol: how many trades printed inside vs outside the window mid:\n"
show qby[m;`sym;`n`above`below!({#x};{+/x[`edge]>0};{+/x[`edge]<0})]
