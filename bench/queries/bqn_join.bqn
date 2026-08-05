# CBQN implementation of bench/SPEC.md. GNU AGPLv3 - see LICENSE and NOTICE.
# One file per workload and no argument parsing, deliberately: this file could
# not be executed in the environment it was written in, so it avoids every
# construct that is not strictly needed. CI is its first real run; the harness
# reports a clear "error" cell if it does not work rather than failing the job.
# No in-language timer is used either -- run_comparative.py measures this engine
# as (total process time - startup baseline) and labels the cell accordingly.
# Data is fully materialised before the kernel; no range-sum shortcuts.
N ← 10000000
M ← 1000000
K ← 1000
G ← 100
h  ← 1048573 | 262147 × ↕N
a  ← 1000 | h
b  ←  997 | h
x  ← 0 + a
y  ← 0 + b
gk ← G | a
kr ← 1048573 | 7919 × ↕K
vr ← 2 × ↕K
kl ← kr ⊏˜ K | M ↑ h
vl ← M ↑ x
CHECK ← (+´a) + 3 × +´b
# kr is sparse and unsorted, so ⍋ (bins) performs a real key search.
ord ← ⍋kr
krS ← ord ⊏ kr
vrS ← ord ⊏ vr
•Out "BENCH join"
•Out "CHECK " ∾ •Repr CHECK
•Out "ANSWER " ∾ •Repr +´ vl × vrS ⊏˜ krS ⍋ kl
