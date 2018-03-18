# Muon Hunter simple python CSV RPI datalogger

This script is for datalogging with the one board Muon Hunter
using a Raspberry Pi.

Set up your Muon Hunter as explained here:
http://www.muonhunter.com/blog/simple-muon-logging-with-the-raspberry-pi

## Usage

### Basic, short term logging
Enter the following commands

sudo pigpiod

python datalogging.py

### Running the script in the background
If you would like to set up a longer experiment, just start the
script in the background and you can log off from your Raspberry Pi
after issuing the following commands.

sudo pigpiod

nohup python datalogging.py&

The data rate can be estimated with the following formula:

$R \approx 2 \cdot CPM  \cdot 40 (bytes/minute)$

The expected data rate using normal background radiation (CPM = 30)
is about 140 kB/hour. This estimate includes the optional temperature 
and pressure data, too. If the detection rate is higher or lower this 
has a direct effect on the data rates being higher or lower, too.

## Data storage
The detections are stored in muonhunter.csv by default in 
the same directory as the script. New data is added to the old, so
if you wish to clear the data, just remove the file.

Copyright Mihaly Vadai 2018

Licensed under the Academic Free License version 3.0

https://opensource.org/licenses/AFL-3.0
