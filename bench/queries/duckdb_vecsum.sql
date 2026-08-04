-- 10,000,000 element vector sum, DuckDB's native range() generator.
SELECT SUM(i) FROM range(10000000) t(i);
