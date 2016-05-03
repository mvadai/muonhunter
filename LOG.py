# The main program for the Muon Hunter python datalogging
# v0.3
# 
# (c) 2016 Mihaly Vadai http://muonhunter.com
#
# Licence: GPL v.3
# see LICENCE

avr_address = 0x19

import sqlite3
import smbus
from PAC import PAC
from time import time, gmtime, sleep, strftime
import sys
from settings import *

logging_time = int(logging_time)
refresh_rate = int(refresh)
database = db
runid = 0
start_time = 0

bus = smbus.SMBus(1)

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

conn = sqlite3.connect(database)
c = conn.cursor()
c.execute('''CREATE TABLE IF NOT EXISTS runs (runid INTEGER PRIMARY KEY AUTOINCREMENT, 
	starttime TIMESTAMP DEFAULT CURRENT_TIMESTAMP) ''')
c.execute('''CREATE TABLE IF NOT EXISTS run (run INTEGER, 
	channel TEXT, time TIMESTAMP DEFAULT CURRENT_TIMESTAMP, pressure REAL, temperature REAL,
	total_counts INTEGER, counts_per_period INTEGER, exrap_flag INTEGER, image_flag INTEGER,
	image_name TEXT, FOREIGN KEY(run) REFERENCES runs(runid))''')
	
c.execute('''INSERT INTO runs DEFAULT VALUES''')
conn.commit()
runid = c.execute('''SELECT * FROM runs ORDER BY runid DESC LIMIT 1''').fetchone()[0]

def print_output(*args):
	sys.stderr.write("\x1b[2J\x1b[H")
	print 'Muon Hunter datalogging\n'.center(72)
	print '\nDatalogging started: {0}       Run ID = {1:d} \n'.format(start_time, int(runid))
	print '{0:9}{1:15}{2:16}{3:12}'.format('Signal','Total counts', 'Hits per min', 'Extrapolated')
	print '{0:9}{1:12d}{2:15d} {3:12d}\n'.format('GM1',gm1_total,gm1_per_min,gm1_ex_flag)
	print '{0:9}{1:12d}{2:15d} {3:12d}\n'.format('GM2',gm2_total,gm2_per_min,gm2_ex_flag)
	print '{0:9}{1:15}{2:17}{3:12}'.format('Signal','Total counts', 'Hits per hour', 'Extrapolated')
	print '{0:9}{1:12d}{2:15d} {3:12d}\n'.format('COSMIC',muon_total,muon_per_hour,muon_ex_flag)
	print 'Temperature = {0:.1f} *C'.format(BMP180.TEMP)
	print 'Pressure = {0:d} Pa'.format(int(BMP180.PRESSURE))
	print 'Time remaining: {0:d} s\n'.format(int(round(logging_time - (time() - start_time))))
	print 'To abort press CTRL + C. Data is saved every {} second.\n'.format(refresh_rate)
	print 'For more information visit:\nhttp://muonhunter.com'





start_time = time()
BMP180 = PAC(runid, start_time)

while time() < start_time + logging_time:
	try:
		data = bus.read_i2c_block_data(avr_address, 0)
	except IOError:
		print 'I2C Bus busy, retrying:'
		try:
			sleep(1)
			data = bus.read_i2c_block_data(avr_address, 0)
		except IOError:
			print 'I2C Bus still busy, exiting.'
			raise
	muon_total = data[1] + (data[2] << 8)
	gm2_total = data[3] + (data[4] << 8) + (data[5] << 16) + (data[6] << 24)
	gm1_total = data[7] + (data[8] << 8) + (data[9] << 16) + (data[10] << 24)
	muon_per_hour = data[11] + (data[12] << 8)
	muon_ex_flag = data[13]
	gm2_per_min = data[14] + (data[15] << 8)
	gm2_ex_flag = data[16]
	gm1_per_min = data[17] + (data[18] << 8)
	gm1_ex_flag = data[19]
	
	BMP180.update()
	
	print_output(muon_total, gm2_total, gm1_total, muon_per_hour,
		muon_ex_flag, gm2_per_min, gm2_ex_flag, gm1_per_min, gm1_ex_flag)
		
	gm1_data = (runid, 'GM1', BMP180.PRESSURE, BMP180.TEMP, gm1_total, gm1_per_min, gm1_ex_flag, 0, '')
	gm2_data = (runid, 'GM2', BMP180.PRESSURE, BMP180.TEMP, gm2_total, gm2_per_min, gm2_ex_flag, 0, '')
	muon_data = (runid, 'Muon', BMP180.PRESSURE, BMP180.TEMP, muon_total, muon_per_hour, muon_ex_flag, 0, '')
	c.execute('INSERT INTO run VALUES (?, ?, DateTime("now"), ?, ?, ?, ?, ?, ?, ?)', gm1_data)
	c.execute('INSERT INTO run VALUES (?, ?, DateTime("now"), ?, ?, ?, ?, ?, ?, ?)', gm2_data)
	c.execute('INSERT INTO run VALUES (?, ?, DateTime("now"), ?, ?, ?, ?, ?, ?, ?)', muon_data)
	conn.commit()
	
	sleep(refresh_rate)
	
