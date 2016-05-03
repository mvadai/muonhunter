# muonhunter
Software for the Muon Hunter project 

### Licence: GPL v.3

The RPi Makefile and the MV_bmp085 code uses wiringPi as a library
issued under (LGPL v.3) see COPYING.LESSER in the src_BMP180 directory 
for the wiringPi licence.

This work is licenced under GPL v.3 as per this information: 
http://www.gnu.org/licenses/gpl-faq.html#AllCompatibility

## RPi python and C++ datalogging software  version history:

### 0.1
- python based event handling
- parallel threads for each signal
- objects for each sensor created
- I2C BMP180 (BMP085 codebase) driver added

### 0.2
- sqlite capability added
TODO: fix RPi bug GM2 and Muon signal

### 0.3
- I2C based datalogging for the GM signals, too
- AVR records coincidences
- python event handling removed due to lack of precision when
counting coincidences
- UI program revised
- LOG.py main datalogging script added
- export.py database to csv export script added

### Usage

#### Live mode
sudo python LOG.py <number of seconds>

#### Daemon / background task mode
- Over SSH: nohup sudo python LOG.py <number of seconds> >/dev/null 2>&1 &
- Console: sudo python LOG.py <number of seconds> >/dev/null 2>&1 &

Data is written into muonhunter.db every 1s. This is an sqlite database.

#### Export data into spreadsheet
- Check out this video: https://www.youtube.com/watch?v=lY-0Rk3oNpw

## AVR firmware version history:

### 0.1
- GM & Muon rolling averages implented.
- LCD display
- Buzzer and LED features.

### 0.2 
- GM totals / GM plateauing capability added.
- Lightbug fixed.
- GM counting precision improved

### 0.3
- I2C communcation in slave mode by reusing code by g4lvanix
the slave code IS MODIFIED from the original
- LED flash problem fixed.
- cleaned I2C data

### Usage in python (should you wish to write your own client program):

sudo python

import smbus

bus = smbus.SMBus(1)

bus.read_i2c_block_data(0x19, 0)

#### Example dataset:

[51, 5, 0, 60, 0, 0, 0, 88, 0, 0, 0, 108, 0, 1, 40, 0, 0, 24, 0, 0, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]

read as follows from left to right:
- Device [51,
- Muon total LSB, MSB values 5, 0
- GM1 total XXLSB, XLSB, LSB, MSB values 60, 0, 0, 0
- GM2 total XXLSB, XLSB, LSB, MSB values 88, 0, 0, 0,
- Muon per hour LSB, MSB, extrapolation flag values  108, 0, 1,
- GM1 per min LSB, MSB, extrapolation flag values 40, 0, 0,
- GM2 per min LSB, MSB, extrapolation flag values 24, 0, 0,
- Detector serial number: 10

Extrapolation means that the base time period hasn't elapsed yet since the beginning of datalogging so the
detector extrapolates the data for the base time period based on current observations.
This is indicated on the LCD by displaying a '*' next to the number.
Otherwise it displays a rolling average based on the last base time period.

EG. 108, 0, 1
means in the muon data that 1 hour hasn't elapsed since datalogging, so given the current
rate of muon detections the code's best guess is 108 for the hourly number of muons
provided the rate doesn't change. This number is based on 5 observations as you can see from
the total number of muons that you should treat accordingly. 
So the extrapolation flag is just an indication that you should use these
numbers carefully. (The base period for GM hits is 1 min.)

#### TODOs
- TODO: timer compensation for I2C transmit times
- TODO: implement second mode including buffers

### Credits
- Sylvain Bissonette, Louis Frigon & Fandi Gunawan for the pcd8544 LCD driver code (GPL v.3)
- g4lvanix - eliaselectronics.com for the I2C slave library - this code
is modified from the original (public domain)
- the RPi Makefile and the MV_bmp085 code uses wiringPi as a library - see wiringpi.com
