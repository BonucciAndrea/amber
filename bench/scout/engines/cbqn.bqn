# bench/scout/engines/cbqn.bqn - CBQN side of the scout matrix (core tier).
# Run:  cbqn bench/scout/engines/cbqn.bqn <op> <N> <runs> <warmup>
# Protocol and data model: bench/scout/SCOUT_SPEC.md
#
# BQN numbers are float64 throughout; the generator's largest intermediate is
# 262147*(N-1) = 2.62e12 < 2^53, so this engine sees bit-identical input to the
# int64 engines (SCOUT_SPEC section 1).  Core tier: no table layer, so the table
# ops report SKIP.  mmax_64 has no O(n) BQN formulation and is left out rather
# than timed against native monotonic-deque kernels at a different complexity.
#
# Naming follows BQN roles, not the other engines': an Uppercase identifier has
# FUNCTION role, so the data lives in lowercase names (n, runs, warm, x, y) and
# only the kernel F and the helpers Sfa/Grp/Msum/Mcnt are capitalised.  `have`
# rather than a null F flags "op not implemented", because @ is a subject and
# cannot be assigned to a function-role name.

op   ← ⊑•args
n    ← •ParseFloat 1⊑•args
runs ← •ParseFloat 2⊑•args
warm ← •ParseFloat 3⊑•args

i ← ↕n
h ← 1048573 | 262147 × i
a ← 1000 | h
b ← 997 | h
x ← a
y ← b
chk ← (+´a) + 3 × +´b

Sfa ← { s←𝕩 ⋄ m←≠s ⋄ ((⊑s) + (s⊑˜⌊m÷4) + (s⊑˜⌊m÷2) + (s⊑˜⌊3×m÷4) + ¯1⊑s) + 1e9 × +´(1↓s) < ¯1↓s }
Grp ← { k←⍷𝕨 ⋄ ix←k⊐𝕨 ⋄ +´(1 + 251|k) × +´¨ ix⊔𝕩 }
Msum ← { s←+`𝕩 ⋄ s - (𝕨⥊0) ∾ (-𝕨)↓s }
Mcnt ← { 𝕨⌊1+↕≠𝕩 }

kr ← 1048573 | 7919 × ↕1000
vr ← 2 × ↕1000
mj ← 1000000

have ← 0
F ← {𝕤⋄0}
{ op≡"sum_f"          ? have↩1 ⋄ F↩{𝕤⋄+´x} ; @ }
{ op≡"max_f"          ? have↩1 ⋄ F↩{𝕤⋄⌈´x} ; @ }
{ op≡"dot"            ? have↩1 ⋄ F↩{𝕤⋄+´x×y} ; @ }
{ op≡"sum_i"          ? have↩1 ⋄ F↩{𝕤⋄+´a} ; @ }
{ op≡"arith_mask"     ? have↩1 ⋄ F↩{𝕤⋄+´(y+2.5×x)/˜x>50} ; @ }
{ op≡"sort_f"         ? have↩1 ⋄ F↩{𝕤⋄Sfa ∧x} ; @ }
{ op≡"sort_presorted" ? have↩1 ⋄ p←∧x ⋄ F↩{𝕤⋄Sfa ∧p} ; @ }
{ op≡"grade_i"        ? have↩1 ⋄ gk←1000⌊n ⋄ F↩{𝕤⋄+´gk↑⍋a} ; @ }
{ op≡"find"           ? have↩1 ⋄ pr←kr⊏˜1000|h ⋄ F↩{𝕤⋄+´kr⊐pr} ; @ }
{ op≡"member"         ? have↩1 ⋄ F↩{𝕤⋄+´h∊kr} ; @ }
{ op≡"distinct"       ? have↩1 ⋄ F↩{𝕤⋄d←⍷a ⋄ (1e6×≠d)++´d} ; @ }
{ op≡"distinct_100k"  ? have↩1 ⋄ g5←100000|h ⋄ F↩{𝕤⋄d←⍷g5 ⋄ (1e6×≠d)++´d} ; @ }
{ op≡"group_10"       ? have↩1 ⋄ g←10|h ⋄ F↩{𝕤⋄g Grp x} ; @ }
{ op≡"group_100"      ? have↩1 ⋄ g←100|h ⋄ F↩{𝕤⋄g Grp x} ; @ }
{ op≡"group_10k"      ? have↩1 ⋄ g←10000|h ⋄ F↩{𝕤⋄g Grp x} ; @ }
{ op≡"group_100k"     ? have↩1 ⋄ g←100000|h ⋄ F↩{𝕤⋄g Grp x} ; @ }
{ op≡"join_inner"     ? have↩1 ⋄ hj←1048573|262147×↕mj ⋄ kl←kr⊏˜1000|hj ⋄ vl←1000|hj ⋄ F↩{𝕤⋄+´vl×(kr⊐kl)⊏vr} ; @ }
{ op≡"msum_16"        ? have↩1 ⋄ F↩{𝕤⋄+´16 Msum x} ; @ }
{ op≡"mavg_256"       ? have↩1 ⋄ F↩{𝕤⋄+´(256 Msum x)÷256 Mcnt x} ; @ }

ans ← 0
{ have=0 ? •Out "SKIP "∾op ;
   {𝕤⋄ans↩F@}¨↕warm
   ts ← {𝕤⋄t0←•MonoTime@ ⋄ ans↩F@ ⋄ 1000×(•MonoTime@)-t0}¨↕runs
   •Out "BENCH   "∾op
   •Out "CHECK   "∾•Repr chk
   •Out "ANSWER  "∾•Repr ans
   •Out "TIME_MS "∾•Repr (∧ts)⊑˜⌊runs÷2
}
