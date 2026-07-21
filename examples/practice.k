\l amber.k
\l fin.k
/ ============================================================================
/ examples/practice.k  -  a playground. Copy lines into the REPL and tweak them.
/ Runs top-to-bottom as a smoke test too.
/ ============================================================================
ex:{r:. x;$[(`M=@r)|iskeyed r;[`0:(30$x),"->";`0:"    ",/:amfmt r];`0:(30$x),"-> ",`k r]}
`0:"--- vectors & math ---"
ex"1+2*3 4 5"
ex"sum til 100"
ex"avg 10?100"
ex"(asc 5 3 9 1 7)"
ex"in[3 5;1 2 3 4 5]"
ex"deltas sums 1 2 3 4 5"

`0:"";`0:"--- dictionaries & tables ---"
ex"`a`b`c!10 20 30"
ex"([]city:`ldn`nyc`tok;pop:9 8 14)"
ex"([id:1 2 3]name:`ann`bob`cid)"
ex"meta ([]x:1 2;y:1.5 2.5;z:`a`b)"

`0:"";`0:"--- select / by ---"
tb:([]sym:`a`b`a`b`a;px:10 20 30 40 50;sz:1 2 3 4 5)
ex"qwhere[tb;tb[`sz]>2]"
ex"qby[tb;`sym;`n`tot!({#x};{sum x`sz})]"
ex"xdesc[`px;tb]"

`0:"";`0:"--- a mini market session ---"
gentq 50
ex"count trades"
ex"symstats trades"
ex"3#tsym[aj[`sym`time;trades;quotes];`time]"
ex"vwap trades"
ex"mid 3#quotes"
ex"spread 3#quotes"

`0:"";`0:"practice file ran clean. Now open the REPL (./a) and experiment!"
