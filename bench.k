\l amber.k
/ ============================================================================
/ bench.k  -  how much do sorted attributes actually help?   ./amber bench.k
/ Measures find (?) on a sorted vector WITH the `s attribute (binary search)
/ vs the SAME data without it (linear scan). Same inputs, same results.
/ ============================================================================
us:{[f;a]t:`t[];f . a;`t[]-t}                       / microseconds to run f . a
row:{[q;n]base:asc n?100*n;                          / sorted + `s-attributed
 plain:0+base;                                       / identical data, attribute dropped
 probe:base@q?#base;                                 / q keys that exist
 ok:(base?probe)~plain?probe;                        / results identical?
 lin:us[{y?x};(probe;plain)];                        / linear scan
 binv:us[{y?x};(probe;base)];                        / binary search (`s)
 (n;q;`at base;ok;lin;binv;_lin%1|binv)}
sizes:100000 500000 2000000 5000000
c:+row[5000]'sizes
`0:"find (?): 5000 look-ups, WITH vs WITHOUT the sorted attribute"
show([]rows:c 0;attr:c 2;same:c 3;linUs:c 4;binUs:c 5;speedup:c 6)
`0:""
`0:"membership (in): 5000 probes into 2,000,000 elements"
b:asc 2000000?100000000
p:b@5000?#b
`0:"linear us: ",$us[{in[x;y]};(p;0+b)]
`0:"binary us: ",$us[{in[x;y]};(p;b)]
