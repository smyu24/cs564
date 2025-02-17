-- 1. Find the number of users in the database.
SELECT COUNT(userID) FROM Users;


-- 2. Find the number of users from New York (i.e., users whose location is the string "New York").
SELECT COUNT(UserID)
FROM Users
WHERE Location = 'New York';


-- 3. Find the number of auctions belonging to exactly four categories.
SELECT COUNT(ItemID)
FROM Item_Categories
GROUP BY ItemID
HAVING COUNT(CategoryName) = 4;


-- 4. Find the ID(s) of auction(s) with the highest current price.
WITH MaxCurrently AS (
   SELECT MAX(Currently) AS MaxValue
   FROM Items
)
SELECT ItemID
FROM Items
WHERE Currently = (SELECT MaxValue FROM MaxCurrently);


-- 5. Find the number of sellers whose rating is higher than 1000.
WITH HighRatedUsers AS (
   SELECT UserID
   FROM Users
   WHERE Rating > 1000
)
SELECT COUNT(SellerID)
FROM Items
WHERE SellerID IN (SELECT UserID FROM HighRatedUsers);


-- 6. Find the number of users who are both sellers and bidders.
SELECT COUNT(u.UserID)
FROM Users u
JOIN Items i ON i.SellerID = u.UserID
JOIN Bids b ON b.BidderID = u.UserID;


-- 7. Find the number of categories that include at least one item with a bid of more than $100.
SELECT COUNT(DISTINCT ic.CategoryName)
FROM Item_Categories ic
JOIN Bids b ON ic.ItemID = b.ItemID
AND b.Amount > 100;