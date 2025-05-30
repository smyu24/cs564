DROP TABLE IF EXISTS Users;
DROP TABLE IF EXISTS Items;
DROP TABLE IF EXISTS Categories;
DROP TABLE IF EXISTS Bids;
CREATE TABLE Users (
    UserID TEXT PRIMARY KEY NOT NULL,
    Rating INTEGER NOT NULL,
    Location TEXT,
    Country TEXT
);
CREATE TABLE Items (
    ItemID TEXT PRIMARY KEY,
    Name TEXT,
    Currently DECIMAL(10, 2),
    Buy_Price DECIMAL(10, 2),
    First_Bid DECIMAL(10, 2),
    Number_of_Bids INTEGER,
    Description TEXT,
    Ends REAL,
    Started REAL,
    SellerID TEXT,
    FOREIGN KEY (SellerID) REFERENCES Users(UserID)
);
CREATE TABLE Categories (
    ItemID TEXT NOT NULL,
    Name TEXT NOT NULL,
    PRIMARY KEY (ItemID, Name),
    FOREIGN KEY (ItemID) REFERENCES Items(ItemID)
);
CREATE TABLE Bids (
    BidderID TEXT NOT NULL,
    ItemID TEXT NOT NULL,
    Time TEXT NOT NULL,
    Amount DECIMAL(10, 2) NOT NULL,
    PRIMARY KEY (BidderID, ItemID, Time),
    FOREIGN KEY (BidderID) REFERENCES Users(UserID),
    FOREIGN KEY (ItemID) REFERENCES Items(ItemID)
);