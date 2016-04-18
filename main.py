# The main program for the Muon Hunter
# v0.2
# Set the GPIOs and the defaults in settings.py 
# 
# (c) 2016 Mihaly Vadai 
# The Muon Hunter datalogging software is licensed under a 
# Creative Commons Attribution-ShareAlike 4.0 International License.
# Please use this link as a reference when using any parts of this code:
# http://muonhunter.com
# Thank you.
# This is free software and comes with absolutely no warranty to the extent permitted by applicable law.

import sqlite3
import RPi.GPIO as GPIO
from time import time, gmtime, sleep, strftime
from COUNTER import COUNTER
from PAC import PAC
import sys
from settings import *

# reading settings
GM1 = int(GM1)
GM2 = int(GM2)
GM3 = int(GM3)
COSMIC = int(COSMIC)
logging_time = int(logging_time)
refresh_rate = int(refresh)
database = db
runid = 0
start_time = 0

# checking settings
if trigger_type_gm1 != 'FALLING' and trigger_type_gm1 != 'RISING':
	raise ValueError('Invalid trigger setting for GM1 in settings.py. Must be either FALLING or RISING. Exiting.')

if database == '':
	print('Warning: no or incorrect database name provided.')
	print('Using the default value: muonhunter.db')
	database = 'muonhunter.db'

if GM1 < 1 or GM1 > 28:
	raise ValueError('Invalid GM1 GPIO. Check settings.py. Must be >= 1 and <= 27. Exiting.')

if GM2 < 0 or GM2 > 28:
	raise ValueError('Invalid GM2 GPIO. Check settings.py. Must be >= 0 and <= 27. Exiting.')

if GM3 < 0 or GM3 > 28:
	raise ValueError('Invalid GM2 GPIO. Check settings.py. Must be >= 0 and <= 27. Exiting.')

if COSMIC < 0 or COSMIC > 28:
	raise ValueError('Invalid COSMIC GPIO. Check settings.py. Must be >= 0 and <= 27. Exiting.')

if trigger_type_gm2 != 'FALLING' and trigger_type_gm2 != 'RISING' and GM2 != 0:
	raise ValueError('Invalid trigger setting for GM2 in settings.py. Must be either FALLING or RISING. Exiting.')

if trigger_type_gm3 != 'FALLING' and trigger_type_gm3 != 'RISING' and GM3 != 0:
	raise ValueError('Invalid trigger setting for GM2 in settings.py. Must be either FALLING or RISING. Exiting.')

if trigger_type_cosmic != 'FALLING' and trigger_type_cosmic != 'RISING' and COSMIC != 0:
	raise ValueError('Invalid trigger setting for COSMIC in settings.py. Must be either FALLING or RISING. Exiting.')

if len(sys.argv) == 2:
	logging_time = int(sys.argv[1])

if logging_time < 1:
	print('Warning: logging time not valid. Check your terminal input.')
	print('Continuing for 60 seconds.')
	logging_time = 60

# The GM Event detections run as a separate thread hence this helper function.
def add_detection(channel):
	if channel == GM1:
		GM1_COUNTER.update()
	if channel == GM2 and GM2 != 0:
		GM2_COUNTER.update()
	if channel == COSMIC and COSMIC != 0:
		COSMIC_COUNTER.update()
	
def write_to_database():
	GPIO.remove_event_detect(GM1)
	GPIO.remove_event_detect(GM2)
	GPIO.remove_event_detect(COSMIC)
	pass
	
def print_output():
	sys.stderr.write("\x1b[2J\x1b[H")
	print 'Muon Hunter datalogging\n'.center(72)
	print '{0:8}{1:12}{2:7}{3:14}{4:12}{5}'.format('Setup', 'Signal', 'GPIO', 'CPS to mR/h', 'mR to uGy', 'Trigger')
	print '{0:8}{1:12}{2:4d}'.format('', 'GM1', GM1)
	if GM2 != 0:
		print '{0:8}{1:12}{2:4d}'.format('', 'GM2', GM2)
	if COSMIC != 0:
		print '{0:8}{1:12}{2:4d}'.format('', 'COSMIC', COSMIC)
	print '\nDatalogging started: {0}\n'.format(start_time)
	print '{0:9}{1:15}{2:8}{3:10}{4:15}{5:15}'.format('Signal','Total counts','CPM','Av. CPM', 'Dose uGy/h', 'Av. Dose uGy/h')
	print '{0:9}{1:12d} {2:.12f}\n'.format('GM1',GM1_COUNTER.CNT,GM1_COUNTER.TIME)
	if GM2 != 0:
		print '{0:9}{1:12d} {2:.12f}\n'.format('GM2',GM2_COUNTER.CNT,GM2_COUNTER.TIME)
	if COSMIC != 0:
		print '{0:9}{1:15}{2:8}{3:10}{4:15}{5:15}'.format('Signal','Total counts','CPH','Av. CPH', 'Dose uGy/h', 'Av. Dose uGy/h')
		print '{0:9}{1:12d} {2:.12f}\n'.format('COSMIC',COSMIC_COUNTER.CNT,COSMIC_COUNTER.TIME)
	print 'Time remaining: {0:d} s\n'.format(int(round(logging_time - (time() - start_time))))
	print '(c) 2014 Mihaly Vadai'
	print 'For more information visit:\nhttp://mihalysprojects.weebly.com/blog/category/cosmic-ray'

conn = sqlite3.connect(database)
c = conn.cursor()
c.execute('''CREATE TABLE IF NOT EXISTS runs (runid INTEGER PRIMARY KEY AUTOINCREMENT, 
	starttime TIMESTAMP DEFAULT CURRENT_TIMESTAMP) ''')
c.execute('''CREATE TABLE IF NOT EXISTS run (run INTEGER, 
	channel TEXT, hitid INTEGER, time REAL, pressure REAL, temperature REAL,
	coincidence_time INTEGER, confirmed_flag INTEGER, image_flag INTEGER,
	image_name TEXT, FOREIGN KEY(run) REFERENCES runs(runid))''')
c.execute('''INSERT INTO runs DEFAULT VALUES''')
conn.commit()
runid = c.execute('''SELECT * FROM runs ORDER BY runid DESC LIMIT 1''').fetchone()[0]
	
GPIO.setmode(GPIO.BCM)
GPIO.setup(GM1, GPIO.IN)
start_time = time()
BMP180 = PAC(runid, start_time)
GM1_COUNTER = COUNTER(GM1, runid, "GM1", database, start_time, 0)

if GM2 != 0:
	GPIO.setup(GM2, GPIO.IN)
	GM2_COUNTER = COUNTER(GM2, runid, "GM2", database, start_time, 0)
if COSMIC != 0:
	GPIO.setup(COSMIC, GPIO.IN)
	COSMIC_COUNTER = COUNTER(COSMIC, runid, "MUON", database, start_time, enable_camera)
	if trigger_type_gm1 == 'FALLING':
		GPIO.add_event_detect(GM1, GPIO.FALLING, callback=add_detection, bouncetime=1)
	else:
		GPIO.add_event_detect(GM1, GPIO.RISING, callback=add_detection, bouncetime=1)

	if GM2 != 0:
		if trigger_type_gm2 == 'FALLING':
			GPIO.add_event_detect(GM2, GPIO.FALLING, callback=add_detection, bouncetime=1)
		else:
			GPIO.add_event_detect(GM2, GPIO.RISING, callback=add_detection, bouncetime=1)
		
	if COSMIC != 0:
		if trigger_type_cosmic == 'FALLING':
			GPIO.add_event_detect(COSMIC, GPIO.FALLING, callback=add_detection, bouncetime=1)
		else:
			GPIO.add_event_detect(COSMIC, GPIO.RISING, callback=add_detection, bouncetime=1)

#Main loop
while time() < start_time + logging_time:
	print_output()
	sleep(refresh_rate)
	BMP180.update()

GPIO.cleanup()
