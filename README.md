# Muon Hunter simple python CSV RPI datalogger

This script is for datalogging with the one board Muon Hunter
using a Raspberry Pi.

Set up your Muon Hunter as explained here:
http://www.muonhunter.com/blog/simple-muon-logging-with-the-raspberry-pi

## Usage

### Basic, short term logging
Enter the following commands

```
sudo pigpiod
python datalogging.py
```

```CTRL+C``` cancels the datalogging.

### Running the script in the background
If you would like to set up a longer experiment, just start the
script in the background and you can log off from your Raspberry Pi
after issuing the following commands.
```
sudo pigpiod
nohup python datalogging.py&
logout
```
### Data rates

The data rate can be estimated with the following formula:

![equation](http://latex.codecogs.com/gif.latex?%24%24R%20%5Capprox%202%20%5Ccdot%20CPM%20%5Ccdot%2045%20%5C%20%5C%20%5Cfrac%7Bbytes%7D%7Bminute%7D%24%24)

Where R is the data rate in bytes/minute, CPM is the counts per minute per
GM tube. 

The expected data rate using normal background radiation (CPM = 30)
is about 160 kB/hour. This estimate includes the optional temperature 
and pressure data, too. If the detection rate is higher or lower this 
has a direct effect on the data rates being higher or lower, too.

## Data storage
The detections are stored in muonhunter.csv by default in 
the same directory as the script. The data is added to the file every
5s by default you can change this in the script. New data is added to 
the old, so if you wish to clear the data, just remove the file.

Copyright Mihaly Vadai 2018

Licensed under the Academic Free License version 3.0

https://opensource.org/licenses/AFL-3.0
