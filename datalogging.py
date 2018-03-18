'''
*************************************************************
* Muon Hunter basic datalogging script for the Raspberry Pi *
*************************************************************

Set up your Muon Hunter as explained here:
http://www.muonhunter.com/blog/simple-muon-logging-with-the-raspberry-pi

Make sure you run sudo pigpiod before running this script.
Calling this script: python datalogging.py

The detections are stored in muonhunter.csv by default in 
the same directory as the script. New data is added to the old, so
if you wish to clear the data, just remove the file.

Copyright Mihaly Vadai 2018
Licensed under the Academic Free License version 3.0
https://opensource.org/licenses/AFL-3.0
'''

import pigpio, Queue, os, csv
from time import sleep
from datetime import datetime as dt
mhpi = pigpio.pi()

'''
The detections are stored in memory in a Queue
until they're written out to a file, this
ensures faster timestamp logging. Increase the
queue size if you need a bigger buffer size.
Rename it lower case q for Python 3.
'''
muonDetections = Queue.Queue(1024)
filename = 'muonhunter.csv'

'''
File write out frequency in seconds.
This the same as the environmental data
logging frequency. Empties the queue when called.
'''
timeout = 5

class Environment:
    def __init__(self):
        self.p = 0
        self.t = 0
    
    def getCPUtemperature(self):
        '''
        Get the BMP temperature data here.
        '''
        res = os.popen('vcgencmd measure_temp').readline()
        self.t = (float(res.replace("temp=","").replace("'C\n","")))
    
    def getPressure(self):
        '''
        Get the BMP pressure data here.
        '''
        self.p = 0.0


def loghits():
    '''
    Logs the hist to a file by emptying the queue.
    The hit rate of the detector is lower than the queue
    emptying rate. The program is not protected against
    higher data rates.
    '''
    while(1):
        with open(filename, 'a+') as csvfile:
            datfile = csv.writer(csvfile, delimiter=',')
            while not muonDetections.empty():
                datfile.writerow(muonDetections.get_nowait())
        env.getPressure()
        env.getCPUtemperature()
        sleep(timeout)


def addToQueue(gpio,level,tick):
    '''
    At every detection, the detection with a timestamp 
    is added to a queue.
    '''
    now = dt.now()
    detect = [gpio, now.year, now.month, now.day, now.hour, now.minute, 
        now.second, now.microsecond, env.p, env.t]
    muonDetections.put(detect)

env = Environment()
gm1hit = mhpi.callback(18,pigpio.FALLING_EDGE,addToQueue)
gm2hit = mhpi.callback(17,pigpio.FALLING_EDGE,addToQueue)
loghits()
