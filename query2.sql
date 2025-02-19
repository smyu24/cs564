-- 2. Find the number of users from New York (i.e., users whose location is the string "New York").
SELECT COUNT(UserID)
FROM Users
WHERE Location = 'New York';