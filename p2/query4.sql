-- 4. Find the ID(s) of auction(s) with the highest current price.
WITH MaxCurrently AS (
    SELECT MAX(Currently) AS MaxValue
    FROM Items
)
SELECT ItemID
FROM Items
WHERE Currently = (
        SELECT MaxValue
        FROM MaxCurrently
    );