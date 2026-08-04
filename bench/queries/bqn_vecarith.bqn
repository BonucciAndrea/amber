n ← 1000000
i ← ↕n
px ← 0.001×100000|2654435761×i
y ← (px×2.0)-px÷3.0
s ← +´y
a ← 0.1
Ema ← {(a×𝕩)+(1-a)×𝕨}
folded ← Ema˝ 50000↑px
•Show s+¯1⊑folded
