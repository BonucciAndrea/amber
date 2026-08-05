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
-- Right keys are sparse and unsorted by construction, so this is a real hash
-- join, not a positional lookup (SPEC.md §2).
CREATE OR REPLACE TABLE r AS
  SELECT (7919::BIGINT * j) % 1048573 AS k, 2.0 * j AS w FROM range(1000) t(j);
CREATE OR REPLACE TABLE l AS
  SELECT (SELECT k FROM r WHERE r.rowid = ((262147::BIGINT * i) % 1048573) % 1000) AS k,
         (((262147::BIGINT * i) % 1048573) % 1000)::DOUBLE AS v
  FROM range(1000000) t(i);
SELECT sum(l.v * r.w) FROM l JOIN r ON l.k = r.k;
.timer on
SELECT 'ANSWER ' || printf('%.17g', sum(l.v * r.w)) FROM l JOIN r ON l.k = r.k;
