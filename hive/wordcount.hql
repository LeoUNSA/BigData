-- Drop existing tables if any
DROP TABLE IF EXISTS docs;
DROP TABLE IF EXISTS word_counts;

-- Create a table where each line is a word
CREATE TABLE docs (word STRING)
ROW FORMAT DELIMITED
FIELDS TERMINATED BY '\n'
STORED AS TEXTFILE;

-- Load your local file (absolute path preferred)
LOAD DATA LOCAL INPATH '/home/hadoop/words.txt' OVERWRITE INTO TABLE docs;

-- Create the word count table
CREATE TABLE word_counts AS
SELECT word, COUNT(*) AS count
FROM docs
GROUP BY word
ORDER BY word;

