SELECT COUNT(*)
FROM (
        SELECT UserID
        FROM Users
        WHERE UserID IN (
                SELECT SellerID
                FROM Items
            )
            AND UserID IN (
                SELECT BidderID
                FROM Bids
            )
    ) AS SellersAndBidders;