/ ngn/k implementation of bench/SPEC.md -- see that file for the data model,
/ the exactness argument and the fairness rules. GNU AGPLv3.
/ ngn/k has no portable in-language monotonic clock, so this file prints no
/ TIME_MS line; bench/run_comparative.py falls back to
/   total process time - measured startup baseline
/ for this engine and labels the cell accordingly.
/ Data is fully materialised before the kernel runs -- in particular this file
/ never uses `+/!n`, which some K engines (Amber included) constant-fold to a
/ closed form.
N:10000000
M:1000000
KK:1000
G:100
ii:!N
hh:1048573!262147*ii
pa:1000!hh
pb:997!hh
px:0.0+pa
py:0.0+pb
gk:G!pa
kr:1048573!7919*!KK
vr:2.0*0.0+!KK
kl:kr KK!M#hh
vl:M#px
CHK:(+/pa)+3*+/pb
`0:"BENCH reduce"
`0:"CHECK ",$CHK
`0:"ANSWER ",$(+/px)+(|/px)++/px*py
