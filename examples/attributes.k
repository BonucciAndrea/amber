\l amber.k
\l fin.k
/ ============================================================================
/ examples/attributes.k  -  the four kdb-style column attributes.
/ ============================================================================
hr:{`0:"";`0:"== ",x," ",(48-#x)#"=";}

hr"setting & reading attributes"
`0:"`at `sa 1 2 3   -> ",$`at(`sa 1 2 3)      / s = sorted
`0:"`at `ua 3 1 2   -> ",$`at(`ua 3 1 2)      / u = unique
`0:"`at `pa 1 1 2 2 -> ",$`at(`pa 1 1 2 2)    / p = parted
`0:"`at `ga 3 1 2   -> ",$`at(`ga 3 1 2)      / g = grouped
`0:"`at 1 2 3       -> ",$`at 1 2 3           / none

hr"sorted / parted => binary-search find (O(log n))"
v:asc 2000000?9999999
`0:"attribute: ",$`at v
`0:"v ? 1234567 -> ",$v?1234567

hr"grouped attribute + a group index give O(1) per-key row access"
gentq 100000
`0:"trades sym attribute (set by gentq): ",$`at trades`sym
g:bysym trades
`0:"rows for `AAPL (O(1) via index): ",$#symrows[trades;g;`AAPL]
`0:"meta shows attributes in the `a` column:"
show meta trades

hr"note"
`0:"s/p accelerate the kernel's find (?/ in) to O(log n)."
`0:"g + the group index (bysym) give O(1) per-symbol slicing."
`0:"see bench.k (sorted) and bench-fin.k (grouped) for numbers."
