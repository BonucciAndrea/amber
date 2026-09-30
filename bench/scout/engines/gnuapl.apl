⍝ bench/scout/engines/gnuapl.apl - GNU APL side of the scout matrix (core tier).
⍝ Run:  apl --script -f bench/scout/engines/gnuapl.apl -- <op> <N> <runs> <warmup>
⍝ Protocol and data model: bench/scout/SCOUT_SPEC.md
⍝
⍝ ISO/APL2-style interpreter: 64-bit integers, float64, ⎕IO←0 so indices are
⍝ 0-based like every other engine.  Core tier only (no table layer).
⍝ group_*: GNU APL has no key operator, so the group-by is the classic APL2
⍝ sort-and-partition (grade the keys, cut at key changes, differences of a
⍝ running sum); it works for arbitrary keys.  find/join use dyadic ⍳, which GNU
⍝ APL implements as sort + binary search; member uses ∊, which it evaluates by
⍝ a linear scan of the right argument (O(n*k), ~10 s at N = 1M).  msum/mavg use the prefix-sum
⍝ formulation (as the J and BQN adapters do); mmax_64 is the n-wise reduction
⍝ 64⌈/ over a vector padded with the max identity, O(n*w).
⍝ Timing: ⎕FIO[50] is gettimeofday() in microseconds.
⍝ Printing: ⍕ at ⎕PP←17 is not exact (2497 prints as 2497.0000000000005 and the
⍝ half-integer 1718647862.5 as 1718647862.5000002), so FMT goes through
⍝ ⎕FIO[58], which is libc snprintf: '%.17g' is exact for every float64.

⎕IO←0
⎕PP←17

ARGS←(1+⎕ARG⍳⊂,'--')↓⎕ARG
OP←⊃ARGS[0]
N←⍎⊃ARGS[1]
RUNS←⍎⊃ARGS[2]
WARM←⍎⊃ARGS[3]

I←⍳N
H←1048573|262147×I
A←1000|H
B←997|H
X←A×1.0
Y←B×1.0
CHK←(+/A)+3×+/B

KR←1048573|7919×⍳1000
VR←2.0×⍳1000
MJ←1000000

∇Z←SFA S;M
 M←↑⍴S
 Z←(S[0]+S[⌊M÷4]+S[⌊M÷2]+S[⌊3×M÷4]+S[M-1])+1E9×+/(1↓S)<¯1↓S
∇

∇Z←SCANK V;S
 S←+\V
 Z←S[0]+S[⌊N÷2]+S[N-1]
∇

∇Z←DSUM D
 Z←(1E6×↑⍴D)++/D
∇

∇Z←FMT V
 Z←'%.17g' ⎕FIO[58] V
∇

∇Z←G GRP V;O;GS;VS;E;C;CE
 O←⍋G
 GS←G[O]
 VS←V[O]
 E←(((1↓GS)≠¯1↓GS),1)/⍳↑⍴GS
 C←+\VS
 CE←C[E]
 Z←+/(1+251|GS[E])×CE-0,¯1↓CE
∇

∇Z←W MSUM V;S
 S←+\V
 Z←S-(W⍴0),(-W)↓S
∇

∇Z←W MAVG V
 Z←(W MSUM V)÷W⌊1+⍳↑⍴V
∇

OPS←'sum_f' 'max_f' 'scan_f' 'dot' 'sum_i' 'arith_mask'
OPS←OPS,'sort_f' 'sort_presorted' 'grade_i' 'find' 'member'
OPS←OPS,'distinct' 'distinct_100k' 'group_10' 'group_100' 'group_10k' 'group_100k'
OPS←OPS,'join_inner' 'msum_16' 'mavg_256' 'mmax_64'

SETUPS←'NOP←0' 'NOP←0' 'NOP←0' 'NOP←0' 'NOP←0' 'NOP←0'
SETUPS←SETUPS,'NOP←0' 'P←X[⍋X]' 'GK←1000⌊N' 'PR←KR[1000|H]' 'NOP←0'
SETUPS←SETUPS,'NOP←0' 'G5←100000|H' 'G←10|H' 'G←100|H' 'G←10000|H' 'G←100000|H'
SETUPS←SETUPS,⊂'HJ←1048573|262147×⍳MJ ◊ KL←KR[1000|HJ] ◊ VL←1.0×1000|HJ'
SETUPS←SETUPS,'NOP←0' 'NOP←0' 'NOP←0'

KERNS←'+/X' '⌈/X' 'SCANK X' '+/X×Y' '+/A' '+/(X>50)/Y+2.5×X'
KERNS←KERNS,'SFA X[⍋X]' 'SFA P[⍋P]' '+/GK↑⍋A' '+/KR⍳PR' '+/H∊KR'
KERNS←KERNS,'DSUM ∪A' 'DSUM ∪G5' 'G GRP X' 'G GRP X' 'G GRP X' 'G GRP X'
KERNS←KERNS,'+/VL×VR[KR⍳KL]' '+/16 MSUM X' '+/256 MAVG X' '+/64⌈/(63⍴⌈/⍳0),X'

∇MAIN;J;K;ANS;TS;T0;R
 J←OPS⍳⊂OP
 →(J<↑⍴OPS)/HAVE
 ⎕←'SKIP ',OP
 →0
HAVE:⍎⊃SETUPS[J]
 K←⊃KERNS[J]
 ANS←0
 R←0
WL:→(R≥WARM)/TIMED
 ANS←⍎K
 R←R+1
 →WL
TIMED:TS←⍳0
 R←0
TL:→(R≥RUNS)/DONE
 T0←⎕FIO[50] 1000000
 ANS←⍎K
 TS←TS,((⎕FIO[50] 1000000)-T0)÷1000
 R←R+1
 →TL
DONE:⎕←'BENCH   ',OP
 ⎕←'CHECK   ',FMT CHK
 ⎕←'ANSWER  ',FMT ANS
 ⎕←'TIME_MS ','%.6f' ⎕FIO[58] (TS[⍋TS])[⌊RUNS÷2]
∇

MAIN
)OFF
