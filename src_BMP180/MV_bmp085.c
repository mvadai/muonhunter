// MV_bmp085.c
// 
// Bosch BMP085 experimental driver over I2C 
// using the wiringPi I2C library - issued under GNU LGPLv3 see COPYING.LESSER
// for Raspberry Pi rev2
//
// Step 1 install wiringPi see: https://projects.drogon.net/raspberry-pi/wiringpi/i2c-library/
// Step 2 make sure your user is added to the i2c group (eg. sudo usermod -a -G i2c pi )
// Step 3 set the OSS value (optional) - see below
// Step 4 compile this file:  gcc -Wall -std=gnu99 MV_bmp085.c -o MV_bmp085 -l wiringPi 
// Step 5 execute the script ./MV_bmp085
// 
// first created: 30-05-2014
// last modified: 18-04-2016

#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <wiringPiI2C.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#define DEBUG 0

//set the oversampling value (OSS 0..3) - this affects the precision of pressure measurement and
//the current required to get one reading
// OSS value | +/- pressure (hPa) | +/- altitude (m) | av. current at 1 sample/sec (uA) | no. of samples
//     0     |       0.06         |       0.5        |               3                  |         1
//     1     |       0.05         |       0.4        |               5                  |         2
//     2     |       0.04         |       0.3        |               7                  |         4
//     3     |       0.03         |       0.25       |               12                 |         8
#define OSS 3

// see BMP085 data sheet
short AC1, AC2, AC3;
unsigned short AC4, AC5, AC6;
short B1, B2, MB, MC, MD;
long B5, UP, UT, temp, pressure;
int fd;

// each OSS value requires a different max timing (in us)
// this usually could be done quicker - see data sheet
int get_timing(){
  if ( OSS == 0 ) return 4500;  
  if ( OSS == 1 ) return 7500;
  if ( OSS == 2 ) return 13500;
  if ( OSS == 3 ) return 25500;
  return 25500;
}

// reading factory calibration data - call this first
// 11 16 bit words from 2 x 8 bit registers each, MSB comes first
void read_calibration_values(int fd){
  AC1 = (wiringPiI2CReadReg8 (fd, 0xAA) << 8) + wiringPiI2CReadReg8 (fd, 0xAB);
  AC2 = (wiringPiI2CReadReg8 (fd, 0xAC) << 8) + wiringPiI2CReadReg8 (fd, 0xAD);
  AC3 = (wiringPiI2CReadReg8 (fd, 0xAE) << 8) + wiringPiI2CReadReg8 (fd, 0xAF);
  AC4 = (wiringPiI2CReadReg8 (fd, 0xB0) << 8) + wiringPiI2CReadReg8 (fd, 0xB1);
  AC5 = (wiringPiI2CReadReg8 (fd, 0xB2) << 8) + wiringPiI2CReadReg8 (fd, 0xB3);
  AC6 = (wiringPiI2CReadReg8 (fd, 0xB4) << 8) + wiringPiI2CReadReg8 (fd, 0xB5);
  B1  = (wiringPiI2CReadReg8 (fd, 0xB6) << 8) + wiringPiI2CReadReg8 (fd, 0xB7);
  B2  = (wiringPiI2CReadReg8 (fd, 0xB8) << 8) + wiringPiI2CReadReg8 (fd, 0xB9);
  MB  = (wiringPiI2CReadReg8 (fd, 0xBA) << 8) + wiringPiI2CReadReg8 (fd, 0xBB);
  MC  = (wiringPiI2CReadReg8 (fd, 0xBC) << 8) + wiringPiI2CReadReg8 (fd, 0xBD);
  MD  = (wiringPiI2CReadReg8 (fd, 0xBE) << 8) + wiringPiI2CReadReg8 (fd, 0xBF);
  if ( DEBUG == 1 ) printf("AC1 = %d \nAC2 = %d \nAC3 = %d \nAC4 = %d \nAC5 = %d \nAC6 = %d \nB1 = %d \nB2 = %d \nMB = %d \nMC = %d \nMD = %d \n", 
     AC1, AC2, AC3, AC4, AC5, AC6, B1, B2, MB, MC, MD);
}


// the pressure measurement depends on the temperature measurement
// hence it's done in the same method
void get_readings(long UT, long UP){
  long X1, X2, X3, B3, B6, B7, p;
  unsigned long B4;
if ( DEBUG == 1 ) printf("\nUT = %ld", UT);
  X1 = ((UT - AC6) * AC5) >> 15;
if ( DEBUG == 1 ) printf("\nX1 = %ld", X1);
  X2 = (MC << 11) / (X1 + MD);
if ( DEBUG == 1 ) printf("\nX2 = %ld", X2);
  B5 = X1 + X2;
if ( DEBUG == 1 ) printf("\nB5 = %ld", B5);
  temp = (B5 + 8) >> 4;
if ( DEBUG == 1 ) printf("\ntemp = %ld", temp);
  
  B6 = B5 - 4000;
if ( DEBUG == 1 ) printf("\nB6 = %ld", B6);
  X1 = (B2 * ((B6 * B6) >> 12)) >> 11;
if ( DEBUG == 1 ) printf("\nX1 = %ld", X1);
  X2 = (AC2 * B6) >> 11;
if ( DEBUG == 1 ) printf("\nX2 = %ld", X2);
  X3 = X1 + X2;
if ( DEBUG == 1 ) printf("\nX3 = %ld", X3);
  B3 = (((AC1*4+X3) << OSS) + 2) >> 2;
if ( DEBUG == 1 ) printf("\nB3 = %ld", B3);
  X1 = (AC3 * B6) >> 13;
if ( DEBUG == 1 ) printf("\nX1 = %ld", X1);
  X2 = (B1 * (B6 * B6 >> 12)) >> 16;
if ( DEBUG == 1 ) printf("\nX2 = %ld", X2);
  X3 = ((X1 + X2) + 2) >> 2;
if ( DEBUG == 1 ) printf("\nX3 = %ld", X3);
  B4 = (AC4 * (unsigned long)(X3 + 32768)) >> 15;
if ( DEBUG == 1 ) printf("\nB4 = %ld", B4);
  B7 = ((unsigned long)UP - B3) * (50000 >> OSS);
if ( DEBUG == 1 ) printf("\nB7 = %ld", B7);
  p = B7 < 0x80000000 ? (B7 * 2) / B4 : (B7 / B4) * 2;
if ( DEBUG == 1 ) printf("\np = %ld", p);

  X1 = (p >> 8) * (p >> 8);
if ( DEBUG == 1 ) printf("\nX1 = %ld", X1);
  X1 = (X1 * 3038) >> 16;
if ( DEBUG == 1 ) printf("\nX1 = %ld", X1);
  X2 = (-7357 * p) >> 16;
if ( DEBUG == 1 ) printf("\nX2 = %ld \n", X2);
  pressure = p + ((X1 + X2 + 3791) >> 4);
}

int main(int argc, char **argv){
  int MSB, LSB, XLSB, timing, DEVICE_ADDRESS;
  double temperature;
  
  DEVICE_ADDRESS = 0x77;

  fd = wiringPiI2CSetup (DEVICE_ADDRESS);
  read_calibration_values(fd);
  
  //get raw temperature
  wiringPiI2CWriteReg8 (fd, 0xF4, 0x2E);  
  usleep(4500);
  MSB = wiringPiI2CReadReg8 (fd, 0xF6);
  LSB = wiringPiI2CReadReg8 (fd, 0xF7);
if ( DEBUG == 1 ) printf("\nMSB = %d\nLSB = %d ",MSB,LSB);
  UT = (MSB << 8) + LSB;

  //get raw pressure
  timing = get_timing();
  wiringPiI2CWriteReg8 (fd, 0xF4, 0x34 + (OSS << 6));
  usleep(timing);
  MSB = wiringPiI2CReadReg8 (fd, 0xF6);
  LSB = wiringPiI2CReadReg8 (fd, 0xF7);
  XLSB = wiringPiI2CReadReg8 (fd, 0xF8);
  UP = ((MSB << 16) + (LSB << 8) + XLSB) >> (8 - OSS);
if ( DEBUG == 1 ) printf("\nMSB = %d,\nLSB = %d,\nXLSB = %d", MSB, LSB, XLSB);
  get_readings(UT, UP);
  temperature = temp/10;
  printf("Temperature: %.1f *C, Pressure: %d Pa\n", temperature, (int) pressure);
  return 0;
}
