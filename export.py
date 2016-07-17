# export.py
# Created: 03/05/2016
# Last modified: 17/07/2016
# 
# Author: Mihaly Vadai
# Website:	http://muonhunter.com
# 
# For credits ans usage see README.md
#
# Licence: GPL v.3
# Version: 0.3-eero

from optparse import OptionParser
import sqlite3
import csv
from time import time, gmtime, strftime
import sys
from settings import db

database = db

if database == '':
	print('Warning: no or incorrect database name provided.')
	print('Using the default value: muonhunter.db')
	database = 'muonhunter.db'
	
conn = sqlite3.connect(database)
c = conn.cursor()

parser = OptionParser()
parser.add_option("-f", "--file", dest="filename",
	help="write report to this file", metavar="FILE")
parser.add_option("-r", "--run", dest="run", help="Enter the run id you are interested in after -r.")
parser.add_option("-c", "--channel", dest="channel", help="a_b_c_d or ab_cd or bc or ad or ac_bd after -c")
parser.add_option("-l", "--list-runs", action="store_const", const=1, dest="runs", 
	help="The program lists the run IDs and start times stored in the database.")

args = parser.parse_args()[0]
filename = args.filename
run = args.run
l = args.runs

runsdata = c.execute("SELECT * from runs")

def list_runs():
	print "\nRUNS table"
	print "----------\n"
	print "{0:8}{1:22}{2:8}".format("RUN ID","Start date and time","Serial no")
	print "{0:8}{1:22}{2:8}".format("------","-------------------","---------")
	for row in runsdata:
		print "{0:4d}    {1:16}{2:8d}".format(int(row[0]),row[1],int(row[2]))

if l == 1:
	list_runs()
	sys.exit("\nNo data is exported. Exiting.")

runs = []
for row in runsdata:
	runs.append(row[0])

if run != None and int(run) not in runs:
	print "Run doesn't exist, try one of these:" 
	list_runs()
	sys.exit("No data is exported. Exiting.")

channel = args.channel

if channel == None or channel == 'a_b_c_d' or channel == 'ab_cd' or channel == 'bc' or channel == 'ad' or channel == 'ac_bd':
	pass
else:
	print "Invalid channel ID. Try: a_b_c_d or ab_cd or bc or ad or ac_bd" 
	sys.exit("No data is exported. Exiting.")

	
if filename == None:
	filename = 'muonhunter-%s.csv' % (strftime("%d-%b-%Y-%H-%M-%S", gmtime()))

with open(filename, 'wb') as csvfile:
	gmwriter = csv.writer(csvfile, delimiter=',')
	gmwriter.writerow(("RUN ID", "Channel", "Log time", "Pressure (Pa)", 
		"Temperature *C", "Total hits", "Counts per period", "Extrapolation flag", "Image flag", 
		"Image name", "D day", "D hour", "D min", "D sec"))
	if run == None and channel == None:
		print "Dumping all data in the database into a file. This may take a while."
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
