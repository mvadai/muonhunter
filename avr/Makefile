# Delete the other Makefile and rename this file to Makefile 
# when using the USBASP programmer on Ubuntu 
# see http://www.muonhunter.com/blog/using-usbasp-to-program-the-muon-hunter
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
	sudo $(AVRDUDE) -c avrisp2 -p $(AVRDUDEMCU) -U flash:w:$(TARGET).hex
noreset : all
	sudo $(AVRDUDE) -c avrisp2 -p $(AVRDUDEMCU) -U flash:w:$(TARGET).hex

fuse :
	sudo $(AVRDUDE) -c avrisp2 -p $(AVRDUDEMCU) -U lfuse:w:0xde:m -U hfuse:w:0xde:m -U efuse:w:0x1:m

clean :
	rm -f *.hex *.obj *.o
