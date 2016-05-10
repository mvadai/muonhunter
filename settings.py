# settings.py
# Created: 03/05/2016
# Last modified: 04/05/2016
# Author: Mihaly Vadai
# Website:	http://muonhunter.com
# 
# For credits see README.md
#
# Licence: GPL v.3
# Version: 0.3

#AVR address - address of the Muon Hunter kit in hex format
#if unsure run: sudo i2cdetect -y 1 and copy the number here
avr = 0x19

#Database name
db = 'muonhunter.db'

#Set the default logging time below
logging_time = 60

#set this to 1 if you connect the camera
enable_camera = 0

#trigger channel for the camera GPIO.BCM
camera_trigger = 23

#image filter
#if this is set to 0 the camera will
#be triggered more often, resulting in a lot more images
#you'll have to sort images by hand
#setting 1 the program filters images for you
image_filter = 1

# Set the datalogging frequency and screen refresh rate in seconds.
refresh = 1
