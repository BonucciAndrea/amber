# Uiua implementation of bench/SPEC.md. GNU AGPLv3 - see LICENSE and NOTICE.
# One file per workload, no argument parsing, no in-language timer: this file
# could not be executed in the environment it was written in, so it avoids every
# construct that is not strictly needed. run_comparative.py measures Uiua as
# (total process time - startup baseline) and labels the cell accordingly.
# NOTE ON PRECISION: Uiua computes in f64 throughout, which is exactly why
# SPEC.md picks a generator whose largest intermediate (262147 * (N-1) = 2.6e12)
# stays under 2^53. The old suite constant 2654435761 would have overflowed 53
# bits here and silently produced different data from the int64 engines.
N ← 10000000
M ← 1000000
K ← 1000
G ← 100
H  ← ◿1048573 ×262147 ⇡N
A  ← ◿1000 H
B  ← ◿997 H
X  ← A
Y  ← B
Gk ← ◿G A
Kr ← ◿1048573 ×7919 ⇡K
Vr ← ×2 ⇡K
Kl ← ⊏ ◿K ↙M H Kr
Vl ← ↙M X
Chk ← + /+ A × 3 /+ B
# Kr is sparse and unsorted, so this is a real key search, not indexing.
Idx ← ⊗ Kl Kr
&p "BENCH join"
&p $"CHECK _" Chk
&p $"ANSWER _" /+ × Vl ⊏ Idx Vr
