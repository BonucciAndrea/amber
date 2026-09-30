# bench/scout/engines/uiua.ua - Uiua side of the scout matrix (core tier).
# Run:  uiua run --no-format bench/scout/engines/uiua.ua <op> <N> <runs> <warmup>
# (--no-format matters: a bare `uiua run` rewrites the file in place.)
# Protocol and data model: bench/scout/SCOUT_SPEC.md
#
# Uiua numbers are float64 throughout; the generator's largest intermediate is
# 262147*(N-1) = 2.62e12 < 2^53, so the input is bit-identical to the int64
# engines (SCOUT_SPEC section 1), and sum_i is a float64 sum (exact here).
# Core tier: no table layer, so the table ops report SKIP.
#
# Uiua has no local variables, so every kernel is a function of ONE setup value
# built before the timer (a dummy 0 when it needs none) and Bench! runs it:
# warm-up passes, then the timed passes via under-now, then the median.
# Uiua 0.19 argument order: indexin (find) and memberof take the table first.
# group-by: classify then group-reduce (arbitrary keys).  msum/mavg: prefix-sum
# differences.  mmax_64: 63 lagged max passes folded over the vector, O(n*w)
# time and O(n) memory -- the stencil form ⧈/↥64 materialises all N x 64
# windows (575 MB at N = 1M, ~5 GB at 10M), so it is not used.

Op   ← °□⊡1 &args
N    ← ⋕°□⊡2 &args
Runs ← ⋕°□⊡3 &args
Warm ← ⋕°□⊡4 &args

I   ← ⇡N
H   ← ◿1048573 ×262147 I
A   ← ◿1000 H
B   ← ◿997 H
X   ← A
Y   ← B
Chk ← +/+A ×3/+B

KR ← ◿1048573 ×7919 ⇡1000
VR ← ×2 ⇡1000

# sort answer: order statistics + 1e9 * inversions
Qi  ← [⊃(⋅0|⌊÷4|⌊÷2|⌊×0.75|-1)]
Ord ← /+⊏⊸(Qi⧻)
Inv ← /+<⊃(↘¯1|↘1)
Sfa ← +⊃(Ord|×1e9 Inv)

# group-sum by arbitrary keys: classify (first-appearance codes), then group-reduce
Grp ← /+×⊃(+1◿251◴⊙◌|⊕/+⊛)

# k Shift x: x delayed by k positions, the gap filled with the identity of max
Shift ← ⊂⊃(↯⊙(¯∞◌)|↘¯)

# join setup: {KL VL}; the kernel unboxes (O(1)) and does VR[KR indexin KL]
JoinSetup ← |0.1 {⊃(˜⊏KR◿1000|◿1000) ◿1048573 ×262147 ⇡1000000}
JoinK     ← /+×⊃(°□⊣|˜⊏VR ⊗KR °□⊢)

Bench! ← (
  ⍥(◌⊸^0)Warm
  ⍥(⊂⊙⍜now(⊸^0)⊙◌)Runs [] 0
  &p $"BENCH   _" Op
  &p $"CHECK   _" Chk
  ⊙(&p $"ANSWER  _")
  &p $"TIME\_MS _" ×1000 ⊡⌊÷2 Runs ⍆
  ◌
)

Ops ← {"sum_f" "max_f" "scan_f" "dot" "sum_i" "arith_mask"
       "sort_f" "sort_presorted" "grade_i" "find" "member"
       "distinct" "distinct_100k" "group_10" "group_100" "group_10k" "group_100k"
       "join_inner" "msum_16" "mavg_256" "mmax_64"}

⨬(Bench!(/+X◌) 0
| Bench!(/↥X◌) 0
| Bench!(/+⊏[0 ⌊÷2N -1N]\+X◌) 0
| Bench!(/+×X Y◌) 0
| Bench!(/+A◌) 0
| Bench!(/+▽>50X +Y×2.5X◌) 0
| Bench!(Sfa⍆X◌) 0
| Bench!(Sfa⍆) ⍆X
| Bench!(/+↙↧1000 N⍏A◌) 0
| Bench!(/+⊗KR) ⊏◿1000 H KR
| Bench!(/+∊KR H◌) 0
| Bench!(+⊃(×1e6⧻|/+)◴A◌) 0
| Bench!(+⊃(×1e6⧻|/+)◴) ◿100000 H
| Bench!(Grp⊙X) ◿10 H
| Bench!(Grp⊙X) ◿100 H
| Bench!(Grp⊙X) ◿10000 H
| Bench!(Grp⊙X) ◿100000 H
| Bench!(JoinK) JoinSetup
| Bench!(/+-⊂↯16 0⊸(↘¯16)\+X◌) 0
| Bench!(/+÷↧256+1⇡N -⊂↯256 0⊸(↘¯256)\+X◌) 0
| Bench!(/+∧(↥Shift⊙X) +1⇡63 X◌) 0
| &p $"SKIP _" Op
) ⊗Ops □Op
