# LOG.py
# Created: 03/05/2016
# Last modified: 17/07/2016
# Author: Mihaly Vadai
# Website:	http://muonhunter.com
# 
# For credits and usage see README.md
#
# Licence: GPL v.3
# Version: 0.3-eero

import sqlite3
import RPi.GPIO as GPIO
import smbus
from PAC import PAC
from time import time, gmtime, sleep, strftime
import sys
from settings import *
import os
if enable_camera:
	from picamera import *
	camera = PiCamera()
	GPIO.setmode(GPIO.BCM)
	GPIO.setup(camera_trigger, GPIO.IN)

bus = smbus.SMBus(1)

def read_i2c():
	error = 1
	while ( error > 0 and error < 10 ):
		try:
			data = bus.read_i2c_block_data(avr_address, 0)
			error = 0
		except IOError:
			sleep(1)
			error =+ 1
	
	if error > 10:
		GPIO.cleanup()
		sys.exit("\nPermanent I2C error, check the device address.")
		
	return data
	
avr_address = int(avr)
logging_time = int(logging_time)
refresh_rate = int(refresh)
cam = camera_trigger
database = db
runid = 0
start_time = 0
data = read_i2c()
total_muons = data[1] + (data[2] << 8)

conn = sqlite3.connect(database)
c = conn.cursor()
c.execute('''CREATE TABLE IF NOT EXISTS runs (runid INTEGER PRIMARY KEY AUTOINCREMENT, 
	starttime TIMESTAMP DEFAULT CURRENT_TIMESTAMP, detector INTEGER) ''')

c.execute('''CREATE TABLE IF NOT EXISTS run (run INTEGER, 
	channel TEXT, time TIMESTAMP DEFAULT CURRENT_TIMESTAMP, pressure REAL, temperature REAL,
	total_counts INTEGER, counts_per_period INTEGER, exrap_flag INTEGER, image_flag INTEGER,
	image_name TEXT, t_d INTEGER, t_h INTEGER, t_m INTEGER, t_s INTEGER, FOREIGN KEY(run) REFERENCES runs(runid))''')
	
c.execute('''INSERT INTO runs ('starttime', detector) VALUES (DateTime("now"), ?)''', (data[20],))

conn.commit()
runid = c.execute('''SELECT * FROM runs ORDER BY runid DESC LIMIT 1''').fetchone()[0]

start_time = time()
BMP180 = PAC(runid, start_time)

def take_picture(channel):
	tm = total_muons
	pic = "images/run-{0:d}-muon-{1:d}.jpg".format(int(runid), int(tm)+1)
	camera.capture(pic)
	if image_filter == 1:
		data = read_i2c()
		if tm == (data[1] + (data[2] << 8)):
			path = pic
			try:
				os.remove(path)
			except OSError:
				print "Can't delete image, try to run the script as root.\n"
				print "Check that the images/ path exists.\n"
				raise
				
if enable_camera == 1:
	GPIO.add_event_detect(cam, GPIO.FALLING, callback=take_picture)

def print_output(*args):
	sys.stderr.write("\x1b[2J\x1b[H")
	print 'Muon Hunter datalogging\n'.center(72)
	print '\nDatalogging started: {0}       Run ID = {1:d}  Detector serial: {2:d}\n'.format(start_time, int(runid), int(serial))
	print '{0:9}{1:15}'.format('Signal','Total counts')
	print '{0:9}{1:12d}\n'.format('a_b_c_d',a_b_c_d_hits)
	print '{0:9}{1:12d}\n'.format('ab_cd',ab_cd_hits)
	print '{0:9}{1:12d}\n'.format('bc',bc_hits)
	print '{0:9}{1:12d}\n'.format('ad',ad_hits)
	print '{0:9}{1:12d}\n'.format('ad',ad_hits)
	if t_h < 10:
		if t_m < 10:
			if t_s < 10:
				print 'Detector time: {0:d}day 0{1:d}:0{2:d}:0{3:d}'.format(int(t_d), int(t_h), int(t_m), int(t_s))
			else:
				print 'Detector time: {0:d}day 0{1:d}:0{2:d}:{3:d}'.format(int(t_d), int(t_h), int(t_m), int(t_s))
		else:
			if t_s < 10:
				print 'Detector time: {0:d}day 0{1:d}:{2:d}:0{3:d}'.format(int(t_d), int(t_h), int(t_m), int(t_s))
			else:
				print 'Detector time: {0:d}day 0{1:d}:{2:d}:{3:d}'.format(int(t_d), int(t_h), int(t_m), int(t_s))
	else:
		if t_m < 10:
			if t_s < 10:
				print 'Detector time: {0:d}day {1:d}:0{2:d}:0{3:d}'.format(int(t_d), int(t_h), int(t_m), int(t_s))
			else:
				print 'Detector time: {0:d}day {1:d}:0{2:d}:{3:d}'.format(int(t_d), int(t_h), int(t_m), int(t_s))
		else:
			if t_s < 10:
				print 'Detector time: {0:d}day {1:d}:{2:d}:0{3:d}'.format(int(t_d), int(t_h), int(t_m), int(t_s))
			else:
				print 'Detector time: {0:d}day {1:d}:{2:d}:{3:d}'.format(int(t_d), int(t_h), int(t_m), int(t_s))
					
	print '\nTemperature = {0:.1f} *C'.format(BMP180.TEMP)
	print 'Pressure = {0:d} Pa'.format(int(BMP180.PRESSURE))
	print 'Time remaining: {0:d} s\n'.format(int(round(logging_time - (time() - start_time))))
	print 'To abort press CTRL + C. Data is saved every {} second.\n'.format(refresh_rate)
	print 'For more information visit:\nhttp://muonhunter.com and http://github.com/mvadai/muonhunter'
	
if database == '':
	print('Warning: no or incorrect database name provided.')
	print('Using the default value: muonhunter.db')
	database = 'muonhunter.db'

if len(sys.argv) == 2:
	logging_time = int(sys.argv[1])

if logging_time < 1:
	print('Warning: logging time not valid. Check your terminal input.')
	print('Continuing for 60 seconds.')
	logging_time = 60

try:
	while time() < start_time + logging_time:
		data = read_i2c()
		a_b_c_d_hits = data[1] + (data[2] << 8)
		ab_cd_hits = data[3] + (data[4] << 8)
		bc_hits = data[5] + (data[6] << 8)
		ad_hits = data[7] + (data[8] << 8)
		ac_bd_hits = data[9] + (data[10] << 8)
		serial = data[20]
		t_s = data[21]
		t_m = data[22]
		t_h = data[23]
		t_d = data[24]
	
		BMP180.update()
		
		a_b_c_d = (runid, 'a_b_c_d', BMP180.PRESSURE, BMP180.TEMP, a_b_c_d_hits, 0, 0, 0, '', t_d, t_h, t_m, t_s)
		ab_cd = (runid, 'ab_cd', BMP180.PRESSURE, BMP180.TEMP, ab_cd_hits, 0, 0, 0, '', t_d, t_h, t_m, t_s)
		bc = (runid, 'bc', BMP180.PRESSURE, BMP180.TEMP, bc_hits, 0, 0, 0, '', t_d, t_h, t_m, t_s)
		ad = (runid, 'ad', BMP180.PRESSURE, BMP180.TEMP, ad_hits, 0, 0, 0, '', t_d, t_h, t_m, t_s)
		ac_bd = (runid, 'bc', BMP180.PRESSURE, BMP180.TEMP, ac_bd_hits, 0, 0, 0, '', t_d, t_h, t_m, t_s)
		c.execute('INSERT INTO run VALUES (?, ?, DateTime("now"), ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)', a_b_c_d)
		c.execute('INSERT INTO run VALUES (?, ?, DateTime("now"), ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)', ab_cd)
		c.execute('INSERT INTO run VALUES (?, ?, DateTime("now"), ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)', bc)
		c.execute('INSERT INTO run VALUES (?, ?, DateTime("now"), ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)', ad)
		c.execute('INSERT INTO run VALUES (?, ?, DateTime("now"), ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)', ac_bd)
		conn.commit()
		
		print_output(a_b_c_d_hits, ab_cd_hits, bc_hits, ad_hits,
			ac_bd_hits, serial,
			t_d, t_h, t_m, t_s)
	
		sleep(refresh_rate)

except KeyboardInterrupt:
	if enable_camera:
		GPIO.cleanup()
	sys.exit("\nExiting.")

if enable_camera:		
	GPIO.cleanup()
