# export.py
# Created: 18/08/2015
# Last modified: 03/05/2016
# 
# Author: Mihaly Vadai
# Website:	http://muonhunter.com
# 
# For credits see README.md
#
# License: GPL v.3
# Version: 0.3

from optparse import OptionParser
import sqlite3
import csv
from time import time, gmtime, strftime
import sys

database = 'muonhunter.db'
conn = sqlite3.connect(database)
c = conn.cursor()

parser = OptionParser()
parser.add_option("-f", "--file", dest="filename",
	help="write report to this file", metavar="FILE")
parser.add_option("-r", "--run", dest="run", help="run id")
parser.add_option("-c", "--channel", dest="channel", help="GM1 or GM2 or muon")

args = parser.parse_args()[0]
filename = args.filename
run = args.run

runs = []

for row in c.execute("SELECT * from runs"):
	runs.append(row[0])
	
if run != None and int(run) not in runs:
	print "Run doesn't exist, try one of these:" 
	for row in runs:
		print row
	sys.exit("Exiting: 1")

channel = args.channel

if channel == None or channel == 'GM1' or channel == 'GM2' or channel == 'muon':
	pass
else:
	print "Not valid channel ID. Try: GM1, GM2 or muon" 
	sys.exit("Exiting: 1")

	
if filename == None:
	filename = 'muonhunter-%s.csv' % (strftime("%d-%b-%Y-%H-%M-%S", gmtime()))

with open(filename, 'wb') as csvfile:
	gmwriter = csv.writer(csvfile, delimiter=',')
	gmwriter.writerow(("RUN ID", "Channel", "Log time", "Pressure (Pa)", 
		"Temperature *C", "Total hits", "Counts per period", "Extrapolation flag", "Image flag", 
		"Image name", "D day", "D hour", "D min", "D sec"))
	if run == None and channel == None:
		for row in c.execute("SELECT * from run"):
			gmwriter.writerow(row)
	elif run == None:
		for row in c.execute("SELECT * from run WHERE channel = ?", (channel,)):
        	        gmwriter.writerow(row)
	elif channel == None:
                for row in c.execute("SELECT * from run WHERE run = ?", (run,)):
                	gmwriter.writerow(row)
	else:
                for row in c.execute("SELECT * from run WHERE run = ? AND channel = ?", (run,channel)):
                	gmwriter.writerow(row)
