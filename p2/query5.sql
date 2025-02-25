-- 5. Find the number of sellers whose rating is higher than 1000.
WITH HighRatedUsers AS (
    SELECT UserID
    FROM Users
    WHERE Rating > 1000
)
SELECT COUNT(DISTINCT SellerID)
FROM Items
WHERE SellerID IN (
        SELECT UserID
        FROM HighRatedUsers
    );