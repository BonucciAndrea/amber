# CBQN implementation of bench/SPEC.md. GNU AGPLv3 - see LICENSE and NOTICE.
# One file per workload, so run_comparative.py can invoke it with no arguments.
#
# TWO THINGS THIS FILE GETS RIGHT THAT THE PREVIOUS VERSION DID NOT:
#
# 1. IDENTIFIER ROLES. BQN decides a name's role from its FIRST LETTER: an
#    initial capital is a FUNCTION, lowercase is a SUBJECT (a value). The old
#    file opened with `N <- 10000000`, binding a number to a function-role name,
#    and CBQN rejected the whole file at compile time with "Role of the two
#    sides in assignment must match" -- so every CBQN cell in the table was an
#    error, not a measurement. Every data name here is lowercase (n, m, kn, gn,
#    chk); only real functions (Kern, Time, Arg) are capitalised.
#
# 2. ARGUMENTS ARE OPTIONAL AND SAFE. Not every BQN environment binds •args --
#    the online REPL and •Import-ed scopes have no argv at all, and naming
#    •args directly there is a hard error ("Unknown system values: •args")
#    before a single line runs. Reaching it through •BQN keeps the reference out
#    of THIS file's compilation unit, and ⎊ (Catch) turns "no arguments
#    present" into the documented default instead of a crash. The file
#    therefore runs identically under `cbqn file.bqn`, under the harness (which
#    passes no arguments), and pasted into a browser REPL.
#
# Timing is done in-language with •MonoTime, so the reported figure is the
# KERNEL only: the 10M-element vectors below are materialised before the clock
# starts, exactly as the C, Amber and NumPy rows do it (SPEC.md 4.3). The old
# file had no timer, so the harness had to charge CBQN for data generation as
# well as for the query. No range-sum shortcuts: every vector is real.
Args ← {𝕤 ⋄ •BQN "•args"} ⎊ {𝕩 ⋄ ⟨⟩}
argv ← Args @
Arg ← {𝕩 < ≠argv ? ⌊•ParseFloat 𝕩⊑argv ; 𝕨}
runs ← 5 Arg 0
warm ← 2 Arg 1

n  ← 10000000
m  ← 1000000
kn ← 1000
gn ← 100
h  ← 1048573 | 262147 × ↕n
a  ← 1000 | h
b  ←  997 | h
x  ← 0 + a
y  ← 0 + b
gk ← gn | a
kr ← 1048573 | 7919 × ↕kn
vr ← 2 × ↕kn
kl ← kr ⊏˜ kn | m ↑ h
vl ← m ↑ x
chk ← (+´a) + 3 × +´b

# mask then weighted sum; / is Replicate, not a filter-by-predicate
Kern ← {𝕤 ⋄ +´ (x > 50) / (x × 2.5) + y}

Time ← {𝕤 ⋄ t ← •MonoTime @ ⋄ Kern @ ⋄ 1000 × (•MonoTime @) - t}
{𝕤 ⋄ Kern @ ⋄ @}¨ ↕warm
ts ← Time¨ ↕runs
med ← (⌊(≠ts)÷2) ⊑ ∧ts
•Out "BENCH arith"
•Out "CHECK " ∾ •Repr chk
•Out "ANSWER " ∾ •Repr Kern @
•Out "TIME_MS " ∾ •Repr med
