/ ============================================================================
/ bench-std.k  -  timing the vectorised moving/window aggregates (std.k).
/ run:  ./amber bench-std.k
/ ============================================================================
\l amber.k
\l fin.k
\l std.k

N:1000000
dat:0.0+N?100
win:20
`0:"vector = ",($N)," floats   window = ",$win
`0:""
tm:{[lbl;g]t0:`t[];r:g[];ms:0.001*`t[]-t0;`0:(10$lbl)," ",(9$$round[2]ms)," ms"}
`0:"O(n) prefix-based (truly vectorised):"
tm["msum";{msum[win;dat]}]
tm["mavg";{mavg[win;dat]}]
tm["mcount";{mcount[win;dat]}]
tm["mdev";{mdev[win;dat]}]
`0:""
`0:"O(n*w) window-based:"
tm["mmin";{mmin[win;dat]}]
tm["mmax";{mmax[win;dat]}]
`0:""
`0:"the O(n) ones are ~constant in w; mmin/mmax scale with the window."
