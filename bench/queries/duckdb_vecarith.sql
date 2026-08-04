-- Vector arithmetic (1,000,000 elements) + a tacit EMA over the first
-- 50,000 of them. EMA is capped well below the full 1M because it's
-- expressed as a recursive CTE (SQL has no native scan/fold primitive),
-- which is structurally disadvantaged vs a real array language's scan
-- adverb -- that's an intentional, honest part of what this benchmark
-- measures, not a limitation the harness is trying to hide.
CREATE OR REPLACE TABLE t AS
  SELECT i, 0.001 * ((2654435761 * i) % 100000) AS px
  FROM range(1000000) t(i);

CREATE OR REPLACE TABLE arith AS
  SELECT SUM(px * 2.0 - px / 3.0) AS s FROM t;

WITH RECURSIVE ema(i, e) AS (
  SELECT 0, px FROM t WHERE i = 0
  UNION ALL
  SELECT t.i, 0.1 * t.px + 0.9 * ema.e
  FROM t JOIN ema ON t.i = ema.i + 1
  WHERE t.i < 50000
)
SELECT (SELECT s FROM arith) + (SELECT e FROM ema ORDER BY i DESC LIMIT 1) AS result;
