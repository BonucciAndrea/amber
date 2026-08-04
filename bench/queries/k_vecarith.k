/ Vector arithmetic (1,000,000 elements, elementwise ops + reduction) followed
/ by a tacit exponential moving average via the scan adverb (\). Deterministic
/ dataset (closed formula, no RNG) so timing is stable and reproducible,
/ matching the convention used by the rest of bench/ in this repo.
n:1000000
i:!n
px:0.001*100000!2654435761*i
y:(px*2.0)-px%3.0
s:+/y
a:0.1
ema:{(a*y)+(1-a)*x}
e:ema\[px[0];50000#px]
s+e[-1+#e]
