# The main program for the Muon Hunter python datalogging
# v0.3
# 
# (c) 2016 Mihaly Vadai http://muonhunter.com
#
# Licence: GPL v.3

avr_address = 0x19

import sqlite3
import smbus
from PAC import PAC
from time import time, gmtime, sleep, strftime
import sys
from settings import *

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

def update_db():
	pass
	
def print_output(gm1,gm2,muon):
	sys.stderr.write("\x1b[2J\x1b[H")
	print 'Muon Hunter datalogging\n'.center(72)
	print '{0:8}{1:12}{2:7}{3:14}{4:12}{5}'.format('Setup', 'Signal', 'GPIO', 'CPS to mR/h', 'mR to uGy', 'Trigger')
	print '{0:8}{1:12}{2:4d}'.format('', 'GM1', '')
	if GM2 != 0:
		print '{0:8}{1:12}{2:4d}'.format('', 'GM2', '')
	if COSMIC != 0:
		print '{0:8}{1:12}{2:4d}'.format('', 'COSMIC', '')
	print '\nDatalogging started: {0}\n'.format(start_time)
	print '{0:9}{1:15}{2:8}{3:10}{4:15}{5:15}'.format('Signal','Total counts','CPM','Av. CPM', 'Dose uGy/h', 'Av. Dose uGy/h')
	print '{0:9}{1:12d} {2:.12f}\n'.format('GM1',gm1,BMP180.TEMP)
	if GM2 != 0:
		print '{0:9}{1:12d} {2:.12f}\n'.format('GM2',gm2,BMP180.PRESSURE)
	if COSMIC != 0:
		print '{0:9}{1:15}{2:8}{3:10}{4:15}{5:15}'.format('Signal','Total counts','CPH','Av. CPH', 'Dose uGy/h', 'Av. Dose uGy/h')
		print '{0:9}{1:12d} {2:.12f}\n'.format('COSMIC',muon,muon)
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

start_time = time()
BMP180 = PAC(runid, start_time)

while time() < start_time + logging_time:
	try:
		data = bus.read_i2c_block_data(avr_address, 0)
	except IOError:
		print 'Cannot open I2C bus. Check connections.'
		raise
	muon_total = data[1] + (data[2] << 8)
	gm2_total = data[3] + (data[4] << 8) + (data[5] << 16) + (data[6] << 24)
	gm1_total = data[7] + (data[8] << 8) + (data[9] << 16) + (data[10] << 24)
	update_db()
	print_output(gm1_total,gm2_total,muon_total)
	sleep(refresh_rate)
	BMP180.update()
