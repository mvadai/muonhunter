# Creates a test instance for the coincidence circuitry in the Muon Hunter kit
# through the Raspberry Pi 2,3 B+
# 
# Licence: GPL v3
# (c) Mihaly Vadai - muonhunter.com, 2016

import RPi.GPIO as GPIO
from time import time, gmtime, strftime

class CTEST:
	def __init__(self, test_pins, coincidence_pin):
		self.TEST = test_pins
		self.COINCIDENCE = coincidence_pin
		GPIO.setmode(GPIO.BCM)
		GPIO.setwarnings(False)
		for p in self.TEST:			
			GPIO.setup(p, GPIO.OUT)
		GPIO.setup(self.COINCIDENCE, GPIO.IN)
		GPIO.setup(17, GPIO.IN)
		GPIO.setup(27, GPIO.IN)
		self.START_TIME = time()
		self.RUN = 0

		# generating test cases; all possible combinations based on
		# number of coincidence inputs: 2^N x N matrix,
		# consider direct evaluation in case of lack of memory
		self.CASES = []
		for i in range(0, 2**len(self.TEST)):
			row = []
			for j, pin in enumerate(self.TEST):
				row.append([pin, (i & (1<<j))>>j])
			self.CASES.append(row)
		self.status = -1
		self.log = []

	def run(self):
		self.status = 2
		self.START_TIME = time()
		self.RUN += 1
		self.print_status()
		self.log.append(['MSG', "Run", self.RUN])
		for row in self.CASES:
			coincidence = 0
			for pin_data in row:
				if pin_data[1] == 1:
					GPIO.output(pin_data[0], GPIO.HIGH)
					if pin_data[0] == 18 and GPIO.input(17) != 1:
						print "Failed on pin 17 high."
					if pin_data[0] == 22 and GPIO.input(27) != 1:
						print "Failed on pin 27 high."
				elif pin_data[1] == 0:
					GPIO.output(pin_data[0], GPIO.LOW)
					coincidence+=1
                                        if pin_data[0] == 18 and GPIO.input(17) != 0:
                                                print "Failed on pin 17 low."
                                        if pin_data[0] == 22 and GPIO.input(27) != 0:
                                                print "Failed on pin 27 low."

				else:
					print "INVALID STATE."
			c = GPIO.input(self.COINCIDENCE)
			if c == 0:
				if coincidence == len(self.TEST):
					self.log.append([row, [self.COINCIDENCE, c], "PASS"])
				else:
					self.log.append([row, [self.COINCIDENCE, c],"FAIL"])
					self.status = 1
			elif c == 1:
				if ( coincidence < len(self.TEST) ):
					self.log.append([row, [self.COINCIDENCE, c], "PASS"])
				else:
					self.log.append([row, [self.COINCIDENCE, c], "FAIL"])
					self.status = 1
		if self.status == 2:
			self.status = 0
		else:
			self.status = 1
		self.print_status()
		
	def print_status(self):
		if self.status == -1:
			print "Test initialised."
		elif self.status == 0:
			print "Test PASS."
		elif self.status == 2:
			print "Test RUNNING."
		elif self.status == 1:
			print "Test FAIL."
		else:
			print "Unkown status."

	def print_cases(self):
		for row in self.CASES:
			print row

	def print_log(self):
		for row in self.log:
			if row[0] == 'MSG':
				print row[1], row[2]
			else:
				print row
