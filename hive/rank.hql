set hive.auto.convert.join=false;
CREATE TABLE visits (name STRING, url STRING, time STRING);
LOAD DATA LOCAL INPATH './visit.log'
	OVERWRITE INTO TABLE visits;
CREATE TABLE pages (url STRING, pagerank DECIMAL);
LOAD DATA LOCAL INPATH './pages.log'
	OVERWRITE INTO TABLE pages;
CREATE TABLE rank_result AS
SELECT pr.name FROM
	(SELECT V.name, AVG(P.pagerank) AS prank
	FROM visits V JOIN pages P ON (V.url = P.url)
	GROUP BY name) pr
WHERE pr.prank>0.5;
