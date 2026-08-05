NB. J implementation of bench/SPEC.md. GNU AGPLv3 - see LICENSE and NOTICE.
NB. One file per workload, no argument parsing, no in-language timer: this file
NB. could not be executed in the environment it was written in, so it avoids
NB. every construct that is not strictly needed. run_comparative.py measures J
NB. as (total process time - startup baseline) and labels the cell accordingly.
NB. J prints floats at 6 significant digits by default, which would mangle
NB. a 13-digit exact answer; 9!:11 raises print precision to the 17 the
NB. protocol needs.
9!:11 ] 17
N =: 10000000
M =: 1000000
K =: 1000
G =: 100
H =: 1048573 | 262147 * i. N
A =: 1000 | H
B =: 997 | H
X =: 0.0 + A
Y =: 0.0 + B
Gk =: G | A
Kr =: 1048573 | 7919 * i. K
Vr =: 2.0 * i. K
Kl =: Kr {~ K | M {. H
Vl =: M {. X
Chk =: (+/ A) + 3 * +/ B
echo 'BENCH arith'
echo 'CHECK ' , ": Chk
echo 'ANSWER ' , ": +/ (X > 50) # (X * 2.5) + Y
exit 0
