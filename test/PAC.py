# The PAC.py class for the the Muon Hunter
# v0.2
#
# (c) 2016 Mihaly Vadai 
# Please use this link as a reference when using any parts of this code: 
# http://muonhunter.com
# Thank you.

import sqlite3
import subprocess
import re
import sys
from time import time, sleep

class PAC:
	def __init__(self, run, start_time):
		self.RUN = run
		self.START_TIME = start_time
		self.DATA = []
		self.TEMP = -400.0
		self.PRESSURE = -1.0
		
	def update(self):
		t = time() - self.START_TIME
		cnt = 0
		while cnt < 5:
			output = subprocess.check_output(["./MV_bmp085"])
			data = re.search("Temperature:\s+([0-9.]+)", output)
			if not data:
				sleep(1)
				cnt += 1
				continue
			else:
				self.TEMP = float(data.group(1))
				data = re.search("Pressure:\s+([0-9]+)", output)
				self.PRESSURE = int(data.group(1))
				self.DATA.append([t, self.TEMP, self.PRESSURE])
				break

	def clear_data(self):
		self.DATA = []
