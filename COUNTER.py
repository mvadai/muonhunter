# The COUNTER.py class for the the Muon Hunter
# v0.3
#
# (c) 2016 Mihaly Vadai 

import sqlite3
from time import time, strftime, gmtime

class COUNTER:
	def __init__(self, pin, run, name, db, start_time, image_flag):
		self.PIN = pin
		self.RUN = run
		self.NAME = name
		self.DB = db
		self.START_TIME = start_time
		self.IMAGE_FLAG = image_flag
		self.CNT = 0
		self.TIMES = []
		self.TIME = start_time
		self.IMAGENAME = ''
			
	def update(self):
		t = time() - self.START_TIME
		self.TIMES.append(t)
		self.TIME = t
		self.CNT += 1
		if self.IMAGE_FLAG == 1:
			self.IMAGENAME = '%d-%s.jpg' % (self.PIN, strftime("%d-%b-%Y-%H-%M-%S", gmtime()))
	
	def dump_memory(self):
		conn = sqlite3.connect('GM1')
		c = conn.cursor()
		
	def clear_times(self):
		self.TIMES = []
		
		

	
