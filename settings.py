# settings.py for the Cosmic Ray detection / GM logging application
# v0.1
#
# (c) 2016 Mihaly Vadai 
# GNU GPL v3
# Thank you.

#Databese name
db = 'muonhunter.db'

#Set the default logging time below
logging_time = 60

#set this to 1 if you connect the camera
enable_camera = 0

#Set the GPIOs you connect to the 3V pulse from the GM tubes and the coincidence detector below.
#To disable a pin set it to 0.
#This is BCM numbering. In other words this is the Broadcom SOC channel number. see: http://pi.gadgetoid.com/pinout
GM1 = 17
GM2 = 27
GM3 = 0

#Coincidence input GPIO
COSMIC = 23

#Set the trigger type here it's either FALLING or RISING. Default is FALLING.
#For some GM counters this might be rising.
trigger_type_gm1 = 'FALLING'
#trigger_type_gm1 = 'RISING'
trigger_type_gm2 = 'FALLING'
#trigger_type_gm2 = 'RISING'
trigger_type_gm3 = 'FALLING'
#trigger_type_gm3 = 'RISING'
trigger_type_cosmic = 'FALLING'
#trigger_type_cosmic = 'RISING'

# Set the screen refresh rate in seconds.
refresh = 1

# Set the frequency of the writing cycle to the database in seconds
# the more frequent the less accurate the data
# the less frequent the more memory needed
data_writeout = 60

