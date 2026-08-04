/ kdb+/q dialect fallback for columnar group-by aggregation.
n:1000000; i:til n;
px:0.001*(2654435761*i) mod 100000;
sym:i mod 10;
t:([]sym;px);
r:select sum px by sym from t;
-1 string sum r`px;
\\
