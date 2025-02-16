-- 1. Find the number of users in the database.
SELECT COUNT(user_id) FROM User;

-- 2. Find the number of users from New York (i.e., users whose location is the string "New York").
SELECT COUNT(user_id) 
FROM User
WHERE User.location = "New York";

-- 3. Find the number of auctions belonging to exactly four categories. 
SELECT COUNT(item_id)
FROM Item
 

-- 4. Find the ID(s) of auction(s) with the highest current price.


-- 5. Find the number of sellers whose rating is higher than 1000.


-- 6. Find the number of users who are both sellers and bidders.


-- 7. Find the number of categories that include at least one item with a bid of more than $100.

