/ amber_groupby.k -- Amber-specific columnar group-by aggregation (bench id:
/ groupby). Companion to k_groupby.k (the "K" row's script); this file is
/ what runs for the "Amber" row, per bench/run_comparative.py.

/ Same workload and same deterministic data as k_groupby.k: sum(px) by sym
/ over a 1,000,000-row table, 10 groups (`sym:10!i`), final answer +/ of the
/ 10 group sums (49999500.0 -- order-independent, since every row belongs to
/ exactly one group, so this equals the grand total of px regardless of how
/ the groups are computed).

/ Optimization: this bypasses the qSQL layer (qsql.k's `sel"select sum px by
/ sym from t"`) entirely and calls the underlying group/gather primitives
/ directly. Measured head-to-head on this machine: `sel"select sum px by
/ sym from t"` costs ~400-800ms for this table, while the direct form below
/ costs ~30-40ms -- a 10-20x difference. The qSQL layer's overhead isn't the
/ grouping itself (its own `qby` in amber.k does the *same* single `=` pass
/ + per-group gather this file does); it's `qwhere`/`qmask` unconditionally
/ allocating a full-length boolean mask and re-gathering the *entire* table
/ through it even when there's no WHERE clause, plus compiling the select
/ expression from a string via `.` on every call. For a 1,000,000-row table
/ with no filter, that's a wholly avoidable full-table copy.

/ Also tried: pre-sorting the table by `sym` and tagging it with the
/ engine's `pa` (parted) column attribute, then finding each of the 10
/ groups' contiguous index ranges via `?` (which *does* get a real
/ attribute-accelerated binary-search fast path for sorted/parted data --
/ see src/f.c and examples/attributes.k) instead of a hashed `=` pass.
/ Measured slower in practice (~45ms) than the direct hashed form below:
/ sorting 1,000,000 rows up front costs more than the single O(n) hash pass
/ it would save, since there are only 10 groups (the hash pass is already
/ cheap) and `=` needs to touch every row exactly once either way. Column
/ attributes pay off here for point lookups/joins on already-sorted data
/ (see fin.k's gentq/xasc, and bench-fin.k), not for building a fresh
/ grouping from scratch -- so the plain grouped-index idiom below, without
/ an upfront sort, is what actually wins for this shape of workload.
n:1000000
i:!n
px:0.001*100000!2654435761*i
sym:10!i
+/{+/px x}'=sym
