/ bench/qbench/q.q - kdb+/q side of the comparison (docs/BENCHMARKS.md 2.9).
/ Run:  q bench/qbench/q.q -q < /dev/null      (identical closed-form data to amber.k)
N:1000000;                                    / set to 10000000 for the 10M row column
i:til N;
S:`$string til 100;
sym:S i mod 100;
px:100.0+0.01*i mod 1000;
sz:1+i mod 500;
t:([]sym:sym; px:px; sz:sz);
Q:N div 5;
qi:til Q;
qt:`sym`time xasc ([]sym:S qi mod 100; time:2*qi; bid:100.0+0.01*qi mod 900);
tr:([]sym:sym; time:i; px:px);
tmin:{[e;n] min {[e;j] t0:.z.p; value e; `long$(.z.p-t0)%1000}[e] each til n};  / min microseconds
bench:{[nm;e;n] -1 "Q|",nm,"|",string[0.001*tmin[e;n]]," ms";};
-1 "Q rows=",string N;
bench["groupby_sum";    "select sum px by sym from t"; 5];
bench["groupby_vwap";   "select vwap:sz wavg px by sym from t"; 5];
bench["filter";         "select from t where px>105"; 5];
bench["filter_groupby"; "select sum sz by sym from t where px>105"; 5];
bench["sum_reduction";  "sum px"; 10];
bench["sort_xasc";      "`px xasc t"; 5];
exit 0;
