# This is the Raspberry file Makefile
MCU=atmega168
AVRDUDEMCU=m168
CC=/usr/bin/avr-gcc
CFLAGS=-g2 -std=gnu99 -Os -w -mcall-prologues -mmcu=$(MCU)
OBJ2HEX=/usr/bin/avr-objcopy
AVRDUDE=/usr/local/bin/avrdude
TARGET=MuonHunter
all :
	$(CC) $(CFLAGS) $(TARGET).c -o $(TARGET)
	$(OBJ2HEX) -R .eeprom -O ihex $(TARGET) $(TARGET).hex
	rm -f $(TARGET)

install : all
	sudo gpio -g mode 22 out
	sudo gpio -g write 22 0
	sudo $(AVRDUDE) -p $(AVRDUDEMCU) -P /dev/spidev0.0 -c linuxspi -b 1000000 -U flash:w:$(TARGET).hex
	sudo gpio -g write 22 1
noreset : all
	sudo $(AVRDUDE) -p $(AVRDUDEMCU) -P /dev/spidev0.0 -c linuxspi -b 100000 -U flash:w:$(TARGET).hex

fuse :
	sudo gpio -g mode 22 out
	sudo gpio -g write 22 0
	sudo $(AVRDUDE) -c usbasp -p $(AVRDUDEMCU) -U lfuse:w:0xde:m -U hfuse:w:0xde:m -U efuse:w:0x1:m
	sudo gpio -g write 22 1

clean :
	rm -f *.hex *.obj *.o
