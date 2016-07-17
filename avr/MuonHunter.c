/* 
 *
 * MuonHunter.c
 * Created: 18/08/2015
 * Last modified: 17/07/2016
 * 
 * Author: Mihaly Vadai
 * Website:	http://muonhunter.com
 *
 * License: GPL v.3
 * Version: 0.3-eero
 */
//external crystal
#define F_CPU 8192000UL

// Test w/o crystal at 3.3V
// also see MuonHunter.h for timer settings
//#define F_CPU 8000000UL

#include <avr/io.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <util/twi.h>
#include <avr/pgmspace.h>
#include <stdio.h>
#include "pcd8544.c"
#include "image.h"
#include "MuonHunter.h"

int main(void){
	Initialize();
	while(1){};
}
ISR(PCINT1_vect, ISR_BLOCK){
	uint8_t GM_PINS = (PINC & 0xF);
	
	switch ( GM_PINS )
	{
		case 0xD :
		AC_BD_hits++;
		A_B_C_D_hits+=2;
		break;
			
		case 0xB :
		AD_hits++;
		A_B_C_D_hits+=2;
		break;
			
		case 0xE:
		AB_CD_hits++;
		A_B_C_D_hits+=2;
		break;
		
		case 0x7:
		A_B_C_D_hits++;
		break;
	}
}
ISR(PCINT2_vect, ISR_BLOCK)
{
	uint8_t GM_PINS = PIND;
	if (PIND == 0x60) 
	{ BC_hits++;
	  A_B_C_D_hits+=2;}
	  
}
ISR(TIMER0_OVF_vect)
{
	timer_time++;
	
	if ( modes == 0x3F || modes == 0x15 )
	{
		if ( timer_time == TIMER_OVF_S )
		{
			timer_update();
			update_counter();	
		}
	}else{
		if ( modes == 0x0 && timer_time == TIMER_OVF_M )
		{
			timer_update();
			update_counter();
		}
		if ( modes == 0x40 && timer_time == TIMER_OVF_M )
		{
			timer_update();
			update_counter();
		}
	}
	
}
ISR(TWI_vect){
	// this code is modified from the g4lvanix git repo 
	// buffers resized: ran out of memory on an ATmega168
	// see README.md
	// temporary stores the received data
	uint8_t data;
	
	// own address has been acknowledged
	if( (TWSR & 0xF8) == TW_SR_SLA_ACK ){  
		buffer_address = 0x1F;
		// clear TWI interrupt flag, prepare to receive next byte and acknowledge
		TWCR |= (1<<TWIE) | (1<<TWINT) | (1<<TWEA) | (1<<TWEN); 
	}
	else if( (TWSR & 0xF8) == TW_SR_DATA_ACK ){ // data has been received in slave receiver mode
		
		// save the received byte inside data 
		data = TWDR;
		
		// check wether an address has already been transmitted or not
		if(buffer_address == 0x1F){
			
			buffer_address = data; 
			
			// clear TWI interrupt flag, prepare to receive next byte and acknowledge
			TWCR |= (1<<TWIE) | (1<<TWINT) | (1<<TWEA) | (1<<TWEN); 
		}
		else{ // if a databyte has already been received
			
			// store the data at the current address
			rxbuffer[buffer_address] = data;
			
			// increment the buffer address
			buffer_address++;
			
			// if there is still enough space inside the buffer
			if(buffer_address < 0x1F){
				// clear TWI interrupt flag, prepare to receive next byte and acknowledge
				TWCR |= (1<<TWIE) | (1<<TWINT) | (1<<TWEA) | (1<<TWEN); 
			}
			else{
				// clear TWI interrupt flag, prepare to receive last byte and don't acknowledge
				TWCR |= (1<<TWIE) | (1<<TWINT) | (0<<TWEA) | (1<<TWEN); 
			}
		}
	}
	else if( (TWSR & 0xF8) == TW_ST_DATA_ACK ){ // device has been addressed to be a transmitter
		
		// copy data from TWDR to the temporary memory
		data = TWDR;
		
		// if no buffer read address has been sent yet
		if( buffer_address == 0x1F ){
			buffer_address = data;
		}
		
		// copy the specified buffer address into the TWDR register for transmission
		TWDR = txbuffer[buffer_address];
		// increment buffer read address
		buffer_address++;
		
		// if there is another buffer address that can be sent
		if(buffer_address < 0x1F){
			// clear TWI interrupt flag, prepare to send next byte and receive acknowledge
			TWCR |= (1<<TWIE) | (1<<TWINT) | (1<<TWEA) | (1<<TWEN); 
		}
		else{
			// clear TWI interrupt flag, prepare to send last byte and receive not acknowledge
			TWCR |= (1<<TWIE) | (1<<TWINT) | (0<<TWEA) | (1<<TWEN); 
		}
		
	}
	else{
		// if none of the above apply prepare TWI to be addressed again
		TWCR |= (1<<TWIE) | (1<<TWEA) | (1<<TWEN);
	} 
}

void Initialize()
{
	txbuffer[0x13] = SERIAL;
	PCICR = 0x6;
	PCMSK1 = 0xF;
	PCMSK2 = 0xE0;
	DDRD = 0x1F;
	DDRC = 0x0;
	PORTC = 0x3F;
	PORTD = 0xE0;
	state = DEFAULT_STATE;
	modes = DEFAULT_MODE;
	timer_time = 0;
	timer_sec = 0;
	timer_min = 0;
	timer_hour = 0;
	timer_day = 0;
	gm_LED_comp = 0;
	muon_LED_comp = 0;
	gm_buzz_comp = 0;
	muon_buzz_comp = 0;
	AC_BD_hits = 0;
	AD_hits = 0;
	AB_CD_hits = 0;
	flash_gm1();
	flash_gm2();
	flash_muon();
	TWAR = 0x32;
	TWCR = (1<<TWIE) | (1<<TWEA) | (1<<TWINT) | (1<<TWEN);
	LcdInit();
	LcdContrast(0x41);
	sei();
	LcdClear();
	LcdUpdate();
	LcdGotoXYFont(1,1);
	LcdFStr(FONT_1X,(unsigned char*)PSTR("Muon Hunter"));
	LcdUpdate();
	LcdGotoXYFont(1,3);
	LcdFStr(FONT_1X,(unsigned char*)PSTR("serial:"));
	LcdUpdate();
	LcdGotoXYFont(9,3);
	dtostrf((double)SERIAL,1,0,str);
	LcdFStr(FONT_1X,(unsigned char*)PSTR("00"));
	LcdGotoXYFont(11,3);
	LcdStr(FONT_1X, str);
	LcdUpdate();
	LcdGotoXYFont(1,4);
	LcdFStr(FONT_1X,(unsigned char*)PSTR("Firmware:v0.3-eero"));
	LcdUpdate();
	LcdGotoXYFont(1,6);
	LcdFStr(FONT_1X,(unsigned char*)PSTR("muonhunter.com"));
	LcdUpdate();
	
	_delay_ms(3000);
	
	PCICR = 0x6;
	PCMSK1 = 0xF;
	PCMSK2 = 0xE0;
	DDRC = 0x0;
	PORTC = 0x3F;
	LcdInit();
	LcdContrast(0x41);
	LcdClear();
	LcdUpdate();
	
	LcdGotoXYFont(1,1);
	LcdFStr(FONT_1X,(unsigned char*)PSTR("AC+BD:"));
	LcdUpdate();
	LcdGotoXYFont(10,1);
	dtostrf((double)AC_BD_hits,5,0,str);
	LcdStr(FONT_1X, str);
	LcdUpdate();
	
	LcdGotoXYFont(1,2);
	LcdFStr(FONT_1X,(unsigned char*)PSTR("AB+CD:"));
	LcdUpdate();
	LcdGotoXYFont(10,2);
	dtostrf((double)AB_CD_hits,5,0,str);
	LcdStr(FONT_1X, str);
	LcdUpdate();
	
	LcdGotoXYFont(1,3);
	LcdFStr(FONT_1X,(unsigned char*)PSTR("AD:"));
	LcdUpdate();
	LcdGotoXYFont(10,3);
	dtostrf((double)AD_hits,5,0,str);
	LcdStr(FONT_1X, str);
	LcdUpdate();
	
	LcdGotoXYFont(1,4);
	LcdFStr(FONT_1X,(unsigned char*)PSTR("BC:"));
	LcdUpdate();
	LcdGotoXYFont(10,4);
	dtostrf((double)BC_hits,5,0,str);
	LcdStr(FONT_1X, str);
	LcdUpdate();
	
	LcdGotoXYFont(1,5);
	LcdFStr(FONT_1X,(unsigned char*)PSTR("A+B+C+D:"));
	LcdUpdate();
	LcdGotoXYFont(10,5);
	dtostrf((double)BC_hits,5,0,str);
	LcdStr(FONT_1X, str);
	LcdUpdate();
	
	update_counter();
		change_clock(0x0);

update_counter();
}

void update_state()
{
	LcdGotoXYFont(3,1);
	LcdGotoXYFont(10,1);
}
void update_counter()
{
	LcdGotoXYFont(10,1);
	dtostrf((double)AC_BD_hits,5,0,str);
	LcdStr(FONT_1X, str);
	LcdUpdate();
	
	LcdGotoXYFont(10,2);
	dtostrf((double)AB_CD_hits,5,0,str);
	LcdStr(FONT_1X, str);
	LcdUpdate();
	
	LcdGotoXYFont(10,3);
	dtostrf((double)AD_hits,5,0,str);
	LcdStr(FONT_1X, str);
	LcdUpdate();
	
	LcdGotoXYFont(10,4);
	dtostrf((double)BC_hits,5,0,str);
	LcdStr(FONT_1X, str);
	LcdUpdate();
	
	LcdGotoXYFont(10,5);
	dtostrf((double)BC_hits,5,0,str);
	LcdStr(FONT_1X, str);
	LcdUpdate();
	display_time();
}
void flash_gm1(){
	if ((state & 0xC) >> 2 == 0x1 || (state & 0xC) >> 2 == 0x2 ){
	PORTD &= ~(1 << MUON_LED);
	PORTD &= ~(1 << GM2_LED);
	PORTD |= (1 << GM1_LED);
	_delay_ms(64);
	PORTD &= ~(1 << GM1_LED);
	gm_LED_comp++;
	}
}
void flash_gm2(){
	if ((state & 0xC) >> 2 == 0x1 || (state & 0xC) >> 2 == 0x2 ){
	PORTD &= ~(1 << MUON_LED);
	PORTD &= ~(1 << GM1_LED);
	PORTD |= (1 << GM2_LED);
	_delay_ms(64);
	PORTD &= ~(1 << GM2_LED);
	gm_LED_comp++;
	}
}
void flash_muon(){
	if ((state & 0xC) >> 2 == 0x1 || (state & 0xC) >> 2 == 0x2 ){
		PORTD &= ~(1 << GM1_LED);
		PORTD &= ~(1 << GM2_LED);
		PORTD |= (1 << MUON_LED);
		_delay_ms(256);
		PORTD &= ~(1 << MUON_LED);
		PORTD &= ~(1 << GM1_LED);
		PORTD &= ~(1 << GM2_LED);
		muon_LED_comp++;
	}
}
void gm_buzz()
{
	if ( (state & 0x3) == 0x2 || (state & 0x3) == 0x3 )
	{
		for (uint8_t i = 0; i < 8; i++)
		{
			PORTD |= (1 << MUON_BUZZER);
			_delay_ms(1);
			PORTD &= ~(1 << MUON_BUZZER);
		}
		gm_buzz_comp++;
	}
}
void muon_buzz()
{
	if ( (state & 0x3) == 0x1 || (state & 0x3) == 0x3 )
	{
		for (uint8_t i = 0; i < 16; i++)
		{
			PORTD |= (1 << MUON_BUZZER);
			_delay_ms(1);
			PORTD &= ~(1 << MUON_BUZZER);
		}
		muon_buzz_comp++;
	}
}
void change_clock(uint8_t mode)
{
	switch ( mode )
	{
		case 0x0: // change to /min mode
		// timer clock set to clk / 256
		timer_time = timer_time*TIMER_OVF_M / TIMER_OVF_S;
		TIMSK0 = 0x0;
		TCCR0B = 0x4;
		// counter overflow interrupt enabled
		TIMSK0 = 0x1;
		update_state();
		break;
		case 0x1: // change to /sec mode
		// timer clock set to clk / 8
		// TODO: disable LEDs and buzzer in second mode
		timer_time = timer_time*TIMER_OVF_S / TIMER_OVF_M;
		TIMSK0 = 0x0;
		TCCR0B = 0x2;
		// counter overflow interrupt enabled
		TIMSK0 = 0x1;
				update_state();

		break;
	}
}
void timer_update()
{
	timer_time = 0;
	txbuffer[0x17] = timer_day;
	txbuffer[0x16] = timer_hour;
	txbuffer[0x15] = timer_min;
	txbuffer[0x14] = timer_sec;
	txbuffer[0x9] = ( AC_BD_hits & 0xFF00 ) >> 8;
	txbuffer[0x8] = ( AC_BD_hits & 0xFF ); 
	txbuffer[0x7] = ( AD_hits & 0xFF00 ) >> 8;
	txbuffer[0x6] = ( AD_hits & 0xFF ); 
	txbuffer[0x7] = ( BC_hits & 0xFF00 ) >> 8;
	txbuffer[0x6] = ( BC_hits & 0xFF ); 
	txbuffer[0x3] = ( AB_CD_hits & 0xFF00 ) >> 8;
	txbuffer[0x2] = ( AB_CD_hits & 0xFF );
	txbuffer[0x1] = ( A_B_C_D_hits & 0xFF00 ) >> 8;
	txbuffer[0x0] = ( A_B_C_D_hits & 0xFF ); 
	timer_sec++;
	timer_compensate();
	if ( timer_sec == MEMORY_LIMIT * (60 / MEMORY_LIMIT) )
	{
		timer_min++;
		timer_sec = 0;
		if ( timer_min == MEMORY_LIMIT * (60 / MEMORY_LIMIT) )
		{
			timer_min = 0;
			timer_hour++;
			if ( timer_hour == 24 )
			{
				timer_hour = 0;
				timer_day++;
			}
		}
	}
}
void timer_compensate()
{
	// for minute mode
	// TODO disable LEDs in second mode
	// calculating compensation values in overflows
	uint8_t ovf = 0;
	ovf = ( gm_LED_comp * 8 );
	ovf += gm_buzz_comp;
	ovf += ( muon_LED_comp * 32 );
	ovf += ( muon_buzz_comp * 2 );
	uint8_t ovfs = 0;
	
	ovfs = ovf / TIMER_OVF_M;
	
	timer_sec += ovfs;
	timer_time += ovf % TIMER_OVF_M;
	
	gm_LED_comp = 0;
	gm_buzz_comp = 0;
	muon_LED_comp = 0;
	muon_buzz_comp = 0;
}

void display_time(){
	
// time
	LcdGotoXYFont(1,6);
	dtostrf((double)timer_day,2,0,str);
	LcdStr(FONT_1X, str);
	LcdUpdate();
	
	LcdGotoXYFont(3,6);
	LcdFStr(FONT_1X,(unsigned char*)PSTR("day"));
	LcdUpdate();
	
	LcdGotoXYFont(7,6);
	dtostrf((double)timer_hour,2,0,str);
	LcdStr(FONT_1X, str);
	LcdUpdate();
	
	LcdGotoXYFont(9,6);
	LcdFStr(FONT_1X,(unsigned char*)PSTR(":0"));
	LcdUpdate();
	if ( timer_min < 10 ){
		LcdGotoXYFont(11,6);
		dtostrf((double)timer_min,1,0,str);
		LcdStr(FONT_1X, str);
		LcdUpdate();
		}else{
		LcdGotoXYFont(10,6);
		dtostrf((double)timer_min,2,0,str);
		LcdStr(FONT_1X, str);
		LcdUpdate();
	}
	
	
	LcdGotoXYFont(12,6);
	LcdFStr(FONT_1X,(unsigned char*)PSTR(":0"));
	LcdUpdate();
	
	if ( timer_sec < 10 ){
		LcdGotoXYFont(14,6);
		dtostrf((double)timer_sec,1,0,str);
		LcdStr(FONT_1X, str);
		LcdUpdate();
		}else{
		LcdGotoXYFont(13,6);
		dtostrf((double)timer_sec,2,0,str);
		LcdStr(FONT_1X, str);
		LcdUpdate();
	}	
	}
