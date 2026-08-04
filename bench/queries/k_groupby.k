/ Columnar group-by aggregation on a 1,000,000-row table: sum(px) by sym
/ (10 groups), via the standard ngn/k idiom `{+/px x}'=sym` -- also used by
/ this repo's existing bench/bench_k.k, so amber and ngn/k share one file.
n:1000000
i:!n
px:0.001*100000!2654435761*i
sym:10!i
r:{+/px x}'=sym
+/r
