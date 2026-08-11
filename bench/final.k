/ end-to-end reduction timings through the interpreter.  ./amber bench/final.k
one:{[g]t0:`t[];r:g[];0.001*`t[]-t0}
tm:{[lbl;g;k]ts:{[g;i]one g}[g]'!k;`0:(20$lbl)," ",($&/ts)," ms"}
S:20000000
v:3000000000+S#!1000
w:S#!100
`0:"--- 20,000,000 elements ---"
tm["*/ long";{*/v};10]
tm["&/ long";{&/v};10]
tm["|/ long";{|/v};10]
tm["&/ int";{&/w};10]
tm["+/ long";{+/v};10]
s:100000
v2:3000000000+s#!1000
w2:s#!100
`0:"--- 100,000 elements ---"
tm["*/ long";{*/v2};400]
tm["&/ long";{&/v2};400]
tm["|/ long";{|/v2};400]
tm["&/ int";{&/w2};400]
tm["+/ long";{+/v2};400]
tm["+/ int";{+/w2};400]
