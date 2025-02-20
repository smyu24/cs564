#! /usr/bin/env zsh
rm *.dat
python parser.py ebay_data/items-*.json
sort -u Items.dat -o Items.dat
sort -u Users.dat -o Users.dat
sort -u Categories.dat -o Categories.dat
sort -u Bids.dat -o Bids.dat
 
 