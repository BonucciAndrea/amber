\l amber.k
/ ============================================================================
/ examples/basics.k  -  a 2-minute tour of Amber.   ./amber examples/basics.k
/ ============================================================================
hr:{`0:"";`0:x;`0:(#x)#"-"}
p:{`0:x,`k y}

hr"aggregations & uniform functions"
p["sum 1..10       = ";sum 1+!10]
p["avg 1..10       = ";avg 1+!10]
p["med 3 1 4 1 5   = ";med 3 1 4 1 5]
p["deltas 1 3 6 10 = ";deltas 1 3 6 10]
p["sums   1 2 3 4  = ";sums 1 2 3 4]

hr"a table, a grid, and meta"
t:+`sym`px`qty!(`a`b`c`a; 10 20 30 40; 5 6 7 8)
show t
`0:""
show meta t

hr"sorted attribute makes search O(log n)"
v:asc 1000000?100000000
p["attribute of v  = ";`at v]
p["v ? 42 999      = ";v?42 999]

hr"group-by aggregate -> keyed table"
show qby[t; `sym; `tot`n!({sum x`qty};{#x})]

hr"a keyed table"
show xkey[`sym; t]

hr"strings"
`0:"upper           = ",upper"amber"
`0:"ssr             = ",ssr["a.b.c";".";"/"]
p["like a*c        = ";like["abc";"a*c"]]

hr"see also:  examples/tick.k   (trades, quotes, as-of/window joins, OHLC bars)"
