-- 7. Find the number of categories that include at least one item with a bid of more than $100.
SELECT COUNT(DISTINCT c.Name)
FROM Categories c
    JOIN Bids b ON c.ItemID = b.ItemID
WHERE b.Amount > 100;