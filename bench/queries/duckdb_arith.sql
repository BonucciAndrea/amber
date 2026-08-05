-- DuckDB implementation of bench/SPEC.md. GNU AGPLv3.
-- Table creation is intentionally OUTSIDE the timed statement: every array
-- engine likewise gets its vectors materialised before the clock starts.
-- .timer on makes DuckDB report per-statement wall time; run_comparative.py
-- reads the LAST 'Run Time (s): real' line, i.e. the timed query after the
-- warm-up execution above it.
.timer off
CREATE OR REPLACE TABLE d AS
  SELECT i,
         ((262147::BIGINT * i) % 1048573) % 1000 AS a,
         ((262147::BIGINT * i) % 1048573) % 997  AS b,
         (((262147::BIGINT * i) % 1048573) % 1000) % 100 AS g
  FROM range(10000000) t(i);
SELECT 'CHECK ' || (sum(a) + 3 * sum(b))::BIGINT::VARCHAR FROM d;
SELECT sum(a::DOUBLE * 2.5 + b::DOUBLE) FROM d WHERE a > 50;
.timer on
SELECT 'ANSWER ' || printf('%.17g', sum(a::DOUBLE * 2.5 + b::DOUBLE)) FROM d WHERE a > 50;
