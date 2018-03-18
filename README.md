# Muon Hunter simple python CSV RPI datalogger

This script is for datalogging with the one board Muon Hunter
using a Raspberry Pi.

Set up your Muon Hunter as explained here:
http://www.muonhunter.com/blog/simple-muon-logging-with-the-raspberry-pi

## Usage
Enter the following commands

sudo pigpiod

python datalogging.py

## Data
The detections are stored in muonhunter.csv by default in 
the same directory as the script. New data is added to the old, so
if you wish to clear the data, just remove the file.

Copyright Mihaly Vadai 2018

Licensed under the Academic Free License version 3.0

https://opensource.org/licenses/AFL-3.0
