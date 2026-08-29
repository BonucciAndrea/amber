/ bench/qbench/amber.k - Amber side of the kdb+/q comparison (docs/BENCHMARKS.md 2.9).
/ Run:  ./amber bench/qbench/amber.k     (or with AMBER_NATIVE=1 for -march=native)
/ Identical closed-form data to bench/qbench/q.q (no RNG), so the two are like-for-like.
\l amber.k
\l std.k
\l qsql.k
`diag 0
N:1000000                                   / set to 10000000 for the 10M row column
i:!N
S:`$'$'!100                                  / 100 symbols; `!` dyadic = mod, so 100!i == i mod 100
sym:S 100!i
px:100.0+0.01*1000!i
sz:1+500!i
t:([]sym:sym; px:px; sz:sz)
Q:_N%5                                       / floor(N/5); `%` is float divide, `_` floors
qi:!Q
qt:xasc[`sym`time; ([]sym:S 100!qi; time:2*qi; bid:100.0+0.01*900!qi)]
tr:([]sym:sym; time:i; px:px)
tmin:{[e;n]&/{[e;j]t0:`t[];.e;0.001*`t[]-t0}[e]'!n}   / min ms over n reps (`t[] is microseconds)
bench:{[nm;e;n]`0:"AMBER|",nm,"|",($tmin[qrw e;n])," ms"}   / qrw so bare qSQL evaluates
`0:"AMBER rows=",$N
bench["groupby_sum";    "select sum px by sym from t"; 5]
bench["groupby_vwap";   "select vwap:wavg[sz;px] by sym from t"; 5]
bench["filter";         "select from t where px>105"; 5]
bench["filter_groupby"; "select sum sz by sym from t where px>105"; 5]
bench["sum_reduction";  "sum px"; 10]
bench["sort_xasc";      "`px xasc t"; 5]
