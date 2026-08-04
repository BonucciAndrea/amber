/ kdb+/q dialect fallback for vector arithmetic + tacit EMA.
n:1000000; i:til n;
px:0.001*(2654435761*i) mod 100000;
y:(px*2.0)-px%3.0;
s:sum y;
ema:{[a;x;y] a*y+(1-a)*x};
folded:(ema[0.1]/)\[px[0];50000#px];
-1 string s+last folded;
\\
