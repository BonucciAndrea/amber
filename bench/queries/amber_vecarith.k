/ amber_vecarith.k -- Amber-specific vector arithmetic + tacit EMA (bench id:
/ vecarith). Companion to k_vecarith.k (the "K" row's script); this file is
/ what runs for the "Amber" row, per bench/run_comparative.py.

/ Same workload and same deterministic (RNG-free) data as k_vecarith.k, so
/ results are directly comparable:
/   - 1,000,000-element vector arithmetic + reduction: y=(px*2)-px%3, s=+/y
/   - a tacit exponential moving average, scanned over the first 50,000
/     elements of px, seeded with px[0]
/   - final answer: s + last(ema scan)

/ Optimizations applied (both verified bit-exact against k_vecarith.k's
/ 83332549.92064743 -- see bench/run_comparative.py's parity check):

/  1. Tacit EMA recurrence, algebraically simplified from a lerp expressed
/     as "a*y + (1-a)*x" (2 muls, 1 sub, 1 add, 3 temporaries per scan step)
/     to the equivalent "x + a*(y-x)" (1 sub, 1 mul, 1 add, 2 temporaries).
/     This is the standard "lerp(x,y,a) = x + a*(y-x)" identity -- same
/     value, less arithmetic and one fewer scratch allocation per step of
/     the 50,000-step scan. Because it's an *exact* algebraic reassociation
/     of the same operations in the same left-to-right order (not a
/     reordering of floating-point summation), it reproduces the identical
/     IEEE-754 rounding at every step -- confirmed bit-for-bit against the
/     original formula before adopting it.
/  2. `y` and `s` are no longer bound to top-level names. In this engine,
/     named globals are held live in the session's symbol table for the
/     rest of the script, so the 1,000,000-element `y` vector and the
/     scalar `s` would otherwise outlive their single use. Inlining them
/     into the final expression lets the arena reclaim that memory as soon
/     as the expression finishes evaluating, instead of pinning an 8MB
/     vector for no reason. Pure evaluation-order change -- doesn't affect
/     the arithmetic or the result.

/ (The scan itself is an inherently sequential recurrence -- each step
/ depends on the previous one's output -- so it isn't a parallel-each or
/ SIMD-reduction candidate the way vecsum's plain fold is; see
/ amber_vecsum.k for why peach didn't pay off there either.)
n:1000000
i:!n
px:0.001*100000!2654435761*i
a:0.1
ema:{x+a*y-x}
e:ema\[px[0];50000#px]
(+/((px*2.0)-px%3.0))+e[-1+#e]
