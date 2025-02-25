-- 3. Find the number of auctions belonging to exactly four categories.
-- SELECT COUNT(ItemID)
-- FROM Categories
-- GROUP BY ItemID
-- HAVING COUNT(Name) = 4;
SELECT COUNT(*)
FROM (
        SELECT ItemID
        FROM Categories
        GROUP BY ItemID
        HAVING COUNT(Name) = 4
    ) AS FourCategoryItems;