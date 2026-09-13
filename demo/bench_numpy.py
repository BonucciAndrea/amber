"""Faithful NumPy port of bt.k, timed the same way (min of repeated runs)."""
import numpy as np, time, sys

rng = np.random.default_rng(7)

def genpx(U, T):
    mkt = 0.0085 * rng.standard_normal(T)
    bet = 0.55 + 0.9 * rng.random(U)
    ivl = 0.008 + 0.016 * rng.random(U)
    dft = -0.00005 + 0.0004 * rng.random(U)
    inc = dft[:, None] + bet[:, None] * mkt[None, :] + ivl[:, None] * rng.standard_normal((U, T))
    return 100.0 * np.exp(np.cumsum(inc, axis=1))

def tensor(px, I, S):
    return px[:, I] / px[:, S][:, :, None]

def run(P, b, acL, cpL, cpA, barL, ppy):
    perf = P[b].min(axis=0)
    n = perf.shape[1]
    ac = perf >= acL
    red = np.maximum.accumulate(ac, axis=1)
    nrd = red.sum(axis=1)
    first = n - nrd
    called = nrd > 0
    sp = np.minimum(first, n - 1)
    hit = perf >= cpL
    cum = np.maximum.accumulate(hit * np.arange(1, n + 1), axis=1)
    paid = cum[np.arange(perf.shape[0]), sp]
    fp = perf[:, -1]
    pr = called | (fp >= barL)
    val = paid * cpA + pr + (~pr) * fp
    life = (1 + sp) / ppy
    return np.exp(np.log(np.maximum(val, 1e-7)) / life) - 1

def best(fn, reps=3):
    fn()
    return min((lambda t0=time.perf_counter(): (fn(), time.perf_counter() - t0)[1])() for _ in range(reps))

U, T = 400, 4600
px4 = genpx(U, T)
print("=== A. tensor build, scaling in universe size (S=1800, n=12) =========")
S = np.arange(1800); off = 63 * np.arange(1, 13); I = S[:, None] + off[None, :]
for u in (25, 50, 100, 200, 400):
    pxu = np.ascontiguousarray(px4[:u])
    t = best(lambda: tensor(pxu, I, S))
    print(f"  U={u:<4} {t*1000:7.1f} ms   {u*1800*12/1e6:.2f} M cells")

print()
print("=== B. tensor build, scaling in start dates (U=100, n=12) ===========")
px1 = np.ascontiguousarray(px4[:100])
for s in (450, 900, 1800, 3600):
    Sb = np.arange(s); Ib = Sb[:, None] + off[None, :]
    t = best(lambda: tensor(px1, Ib, Sb))
    print(f"  S={s:<5} {t*1000:7.1f} ms")

print()
print("=== C. one basket, full lifecycle ===================================")
for s, n, step in ((1800, 12, 63), (1800, 4, 189), (1800, 20, 63), (3600, 12, 63), (900, 12, 63)):
    Sc = np.arange(s); offc = step * np.arange(1, n + 1); Ic = Sc[:, None] + offc[None, :]
    P = tensor(px1, Ic, Sc)
    acL = np.full(n, 1.0); cpL = np.full(n, 0.70)
    b = np.array([0, 7, 23])
    t = best(lambda: run(P, b, acL, cpL, 0.025, 0.60, 4), reps=5)
    tb = best(lambda: P[np.array([1, 8, 24])].min(axis=0), reps=5)
    print(f"  S={s:<5} n={n:<3} lifecycle {t*1e6:7.0f} us   basket-min {tb*1e6:6.0f} us"
          f"   = {(t+tb)*1e6:7.0f} us total")

print()
print("=== D. sweep, cost per basket =======================================")
Sd = np.arange(1800); Id = Sd[:, None] + off[None, :]
P = tensor(px1, Id, Sd)
acL = np.full(12, 1.0); cpL = np.full(12, 0.70)
for nb in (100, 500, 2000):
    bs = rng.integers(0, 100, size=(nb, 3))
    t0 = time.perf_counter()
    for b in bs:
        ir = run(P, b, acL, cpL, 0.025, 0.60, 4)
        np.median(ir)
    t = time.perf_counter() - t0
    print(f"  {nb:<6} baskets {t*1000:8.0f} ms   {t/nb*1e6:6.0f} us/basket"
          f"   {t/nb/1800*1e9:5.0f} ns/lifecycle")

print()
print("=== E. memory ========================================================")
print(f"  tensor U=100 S=1800 n=12  {P.nbytes/1e6:.1f} MB")
print(f"  price history U=400 T=4600 {px4.nbytes/1e6:.1f} MB")
