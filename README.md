# muonhunter
Software for the Muon Hunter project

Licence: GPL v.3

AVR firmware version history:
- 0.1
- GM & Muon rolling averages implented.
- LCD display using the pcd8544 code by Sylvain Bissonette,
Louis Frigon & Fandi Gunawan
- Buzzer and LED features.

- 0.2 
- GM totals / GM plateauing capability added.
- Lightbug fixed.

- 0.3a - unstable: no python datalogging code for this yet
- I2C communcation in slave mode by reusing code by g4lvanix
the slave code IS MODIFIED from the original
- To get the total GM and muon data using RPi python:
- sudo python
- import smbus
- bus = smbus.SMBus(1)
- bus.read_i2c_block_data(0x19, 0)

- example dataset:
- [51, 35, 0, 48, 2, 0, 0, 52, 2, 0, 0, 0, 0, 35, 0, 48, 2, 0, 0, 52, 2, 0, 0, 0, 0, 35, 0, 48, 2, 0, 0, 52]

- Calculating the number of hits from this ie. meaning of numbers
- from left to right:

- device 51
- 35 + 0*2^8 muons
- 48 + 2*2^8 + 0*2^16 + 0*2^24 on GM2
- 52 + 2*2^8 + 0*2^16 + 0*2^24 on GM1

- The rest is just repetition.

- TODO: LED flash problem fix.


- 0.4 (planned) 
- Pressure and temperature display.
