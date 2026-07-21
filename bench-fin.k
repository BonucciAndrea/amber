\l amber.k
\l fin.k
/ ============================================================================
/ bench-fin.k  -  O(1) grouped-index lookup vs O(n) linear scan.
/ ============================================================================
us:{[f;a]t:`t[];f . a;`t[]-t}
gentq 1000000000
`0:"trades: ",($#trades),"   distinct symbols: ",$#distinct trades`sym
g:bysym trades                                   / build grouped index once, O(n)
c:trades`sym
syms:distinct c
q:2000?syms                                      / 2,000 random symbol look-ups
`0:""
`0:"2,000 position look-ups (get the row indices for a symbol):"
`0:"  O(n) linear scan  &col=s :  ",($us[{[q;c]{[c;s]&c=s}[c]'q}[q];,c])," us"
`0:"  O(1) grouped index  g s  :  ",($us[{[q;g]{[g;s]g s}[g]'q}[q];,g])," us"
`0:""
`0:"(index built once in O(n); each look-up is then a single dict hit -> O(1).)"
