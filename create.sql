DROP TABLE IF EXISTS Users;
DROP TABLE IF EXISTS Items;
DROP TABLE IF EXISTS Categories;
DROP TABLE IF EXISTS Bids;

CREATE TABLE Users (
    id TEXT PRIMARY KEY,
    rating INTEGER NOT NULL,
    location TEXT,
    country TEXT,
);

CREATE TABLE Items (
    id TEXT PRIMARY KEY,
    name TEXT,
    currently DECIMAL(10,2),
    buy_price DECIMAL(10,2),
    first_bid DECIMAL(10,2),
    number_of_bids INTEGER,
    description TEXT,
    ends REAL,
    started REAL,
    seller_id TEXT,
);

CREATE TABLE Categories (
    name TEXT PRIMARY KEY
);

CREATE TABLE Bids (
    bidder_id TEXT PRIMARY KEY,
    
);