DROP TABLE IF EXISTS logs;
DROP TABLE IF EXISTS result;
CREATE TABLE logs (user_a STRING, time STRING, query STRING);
LOAD DATA LOCAL INPATH './excite-small.log' OVERWRITE INTO TABLE logs;
CREATE TABLE result AS
SELECT user_a, count(1) AS log_entries
FROM logs
GROUP BY user_a
ORDER BY user_a;
