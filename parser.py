
"""
FILE: skeleton_parser.py
------------------
Author: Firas Abuzaid (fabuzaid@stanford.edu)
Author: Perth Charernwattanagul (puch@stanford.edu)
Modified: 04/21/2014

Skeleton parser for CS564 programming project 1. Has useful imports and
functions for parsing, including:

1) Directory handling -- the parser takes a list of eBay json files
and opens each file inside of a loop. You just need to fill in the rest.
2) Dollar value conversions -- the json files store dollar value amounts in
a string like $3,453.23 -- we provide a function to convert it to a string
like XXXXX.xx.
3) Date/time conversions -- the json files store dates/ times in the form
Mon-DD-YY HH:MM:SS -- we wrote a function (transformDttm) that converts to the
for YYYY-MM-DD HH:MM:SS, which will sort chronologically in SQL.

Your job is to implement the parseJson function, which is invoked on each file by
the main function. We create the initial Python dictionary object of items for
you; the rest is up to you!
Happy parsing!
"""

import sys
from json import loads
from re import sub

columnSeparator = "|"

# Dictionary of months used for date transformation
MONTHS = {'Jan': '01', 'Feb': '02', 'Mar': '03', 'Apr': '04', 'May': '05', 'Jun': '06',
          'Jul': '07', 'Aug': '08', 'Sep': '09', 'Oct': '10', 'Nov': '11', 'Dec': '12'}

"""
Returns true if a file ends in .json
"""


def isJson(f):
    return len(f) > 5 and f[-5:] == '.json'


"""
Converts month to a number, e.g. 'Dec' to '12'
"""


def transformMonth(mon):
    if mon in MONTHS:
        return MONTHS[mon]
    else:
        return mon


"""
Transforms a timestamp from Mon-DD-YY HH:MM:SS to YYYY-MM-DD HH:MM:SS
"""


def transformDttm(dttm):
    dttm = dttm.strip().split(' ')
    dt = dttm[0].split('-')
    date = '20' + dt[2] + '-'
    date += transformMonth(dt[0]) + '-' + dt[1]
    return date + ' ' + dttm[1]


"""
Transform a dollar value amount from a string like $3,453.23 to XXXXX.xx
"""


def transformDollar(money):
    if money == None or len(money) == 0:
        return money
    return sub(r'[^\d.]', '', money)


"""
Parses a single json file. Currently, there's a loop that iterates over each
item in the data set. Your job is to extend this functionality to create all
of the necessary SQL tables for your database.
"""
def parseJson(json_file):
    with open(json_file, 'r') as f:
        with open('Items.dat','a') as items_file:
            with open('Users.dat', 'a') as users_file:
                with open('Categories.dat', 'a') as categories_file:
                    with open('Bids.dat','a') as bids_file:
                        # creates a Python dictionary of Items for the supplied json file
                        items = loads(f.read())['Items']
                        users = {}
                        for item in items:
                            """
                            TODO: traverse the items dictionary to extract information from the
                            given `json_file' and generate the necessary .dat files to generate
                            the SQL tables based on your relation design
                            """
                            ItemID = item["ItemID"]
                            Name = item.get("Name", "NULL").replace('"', '""')
                            Categories = item.get("Category", "NULL")
                            Curently = transformDollar(item.get("Currently", "NULL"))
                            Buy_Price = transformDollar(item.get("First_Bid", "NULL"))
                            First_Bid = transformDollar(item.get("First_Bid", "NULL"))
                            Number_of_Bids = item.get("Number_of_Bids", "0")
                            Bids = item.get("Bids", [])
                            Location = item.get("Location", "NULL").replace('"', '""')
                            Country = item.get("Country", "NULL")
                            Started = transformDttm(item.get("Started", "NULL"))
                            Ends = transformDttm(item.get("Ends", "NULL"))
                            SellerID = item["Seller"]["UserID"]
                            SellerRating = item["Seller"]["Rating"]

                            for category in Categories:
                                categories_file.write(f"{ItemID}|{category}\n")
                            Description = "Null"
                            if item.get("Description"):
                                Description = item.get("Description", "NULL").replace("|", " ") 
                                Description = Description.replace('"', '""')
                            if Bids:
                                for bid in Bids:
                                    bid = bid["Bid"]
                                    bidder = bid["Bidder"]
                                    country = bidder.get("Country", "NULL")
                                    location = bidder.get("Location", "NULL").replace('"', '""')
                                    bids_file.write(f"{bidder["UserID"]}|{ItemID}|{bid["Time"]}|{bid["Amount"]}\n")
                                    bidderID = bidder["UserID"]
                                    users[bidderID] = f'{bidderID}|{bidder["Rating"]}|"{location}"|{country}\n'
                            users[SellerID] = f'{SellerID}|{SellerRating}|"{Location}"|{Country}\n'
                            items_file.write(f'{ItemID}|"{Name}"|{Curently}|{Buy_Price}|{First_Bid}|{Number_of_Bids}|"{Description}"|{Ends}|{Started}|{SellerID}\n')

                        for val in users.values():
                            users_file.write(val)
            
		# "required": ["ItemID", "Name", "Category", "Currently", "First_Bid", "Number_of_Bids", "Bids", "Location", "Country", "Started", "Ends", "Seller", "Description"]
		# 		"required": ["UserID", "Rating"]
        #                 "required": ["Bidder", "Time", "Amount"]
		# 					"required": ["UserID", "Rating"]


            # Item: ItemID, Name, Category, Currently, Buy_Price, First_Bid, Number_of_Bids, Bids, Location, Country, Started, Ends, Seller (becomes SellerID), Description
            # Bids: Bidder (is an ID in Users Table), Time, Amount
            # Bidder: Location, Country, UserID, Rating
            # Seller: UserID, Rating




"""
Loops through each json files provided on the command line and passes each file
to the parser
"""


def main(argv):
    if len(argv) < 2:
        print >> sys.stderr, 'Usage: python skeleton_json_parser.py <path to json files>'
        sys.exit(1)
    # loops over all .json files in the argument
    for f in argv[1:]:
        if isJson(f):
            parseJson(f)
            print("Success parsing " + f)


if __name__ == '__main__':
    main(sys.argv)
