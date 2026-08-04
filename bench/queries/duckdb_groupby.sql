-- Columnar group-by aggregation on a 1,000,000-row table: sum(px) by
-- sym (10 groups).
CREATE OR REPLACE TABLE g AS
  SELECT i, (i % 10) AS sym, 0.001 * ((2654435761 * i) % 100000) AS px
  FROM range(1000000) t(i);
SELECT SUM(s) FROM (SELECT sym, SUM(px) AS s FROM g GROUP BY sym);
