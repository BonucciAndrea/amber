/ kdb+/q benchmark — run:  q bench/bench_q.q -q
N:1000000; i:til N;
px:0.001*(2654435761*i) mod 100000; sym:i mod 10; v:(2654435761*i) mod 100000;
-1 "SANITY sum ",string sum px;
-1 "SANITY count_gt50 ",string sum px>50;
-1 "SANITY median ",string (asc px)@N div 2;
-1 "SANITY rollchk ",string sum 100 mavg px;
-1 "SANITY distinct ",string count distinct v;
t:{[n;e] r:value("\\t:";n;e); r%n}; / not portable; use \t: below
-1 "TIME sum ",string (\t:30 sum px)%30;
-1 "TIME filter ",string (\t:30 sum px>50)%30;
tbl:([]sym;px);
-1 "TIME groupby ",string (\t:10 select sum px by sym from tbl)%10;
-1 "TIME rolling ",string (\t:10 sum 100 mavg px)%10;
-1 "TIME sort ",string (\t:10 asc px)%10;
-1 "TIME distinct ",string (\t:10 distinct v)%10;
M:50000; ti:til M;
tr:`time xasc ([]sym:ti mod 10; time:asc (2654435761*ti) mod 1000000; px:0.01*(40503*ti) mod 10000);
qu:`time xasc ([]sym:ti mod 10; time:asc (6700417*ti) mod 1000000; bid:0.01*(2246822519*ti) mod 10000);
-1 "TIME asof ",string (\t:10 aj[`sym`time; tr; qu])%10;
-1 "DONE";
\\
