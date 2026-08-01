/ growler/k benchmark (k6 / ngn-style). Amber derives from ngn/k, so ops match amber's.
/ TIMER: this uses  `t[]  microsecond wall-clock (as in ngn/k / amber). If your k exposes
/ a different timer (e.g.  \t expr ), replace `tm` below accordingly.
N:1000000; i:!N
px:0.001*100000!2654435761*i; sym:10!i; v:100000!2654435761*i
tm:{[n;e]t0:`t[];{. x}'n#,e;0.001*(`t[]-t0)%n}
`0:"SANITY sum ",($+/px),"\n"
`0:"SANITY count_gt50 ",($+/px>50),"\n"
`0:"SANITY median ",($(px@<px)@N div 2),"\n"
`0:"SANITY distinct ",($#?v),"\n"
`0:"TIME sum ",($tm[30;"+/px"]),"\n"
`0:"TIME filter ",($tm[30;"+/px>50"]),"\n"
`0:"TIME groupby ",($tm[10;"{+/px x}'. =sym"]),"\n"
`0:"TIME sort ",($tm[10;"px@<px"]),"\n"
`0:"TIME distinct ",($tm[10;"#?v"]),"\n"
`0:"DONE\n"
