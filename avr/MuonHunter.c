/* 
 *
 * MuonHunter.c
 * Created: 18/08/2015
 * Last modified: 03/05/2016
 * 
 * Author: Mihaly Vadai
 * Website:	http://muonhunter.com
 *
 * License: GPL v.3
 * Version: 0.3
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
	uint8_t GM_PINS = (PINC & 0x07);
	uint8_t CONTROL_PINS = (PINC & 0x8);
	
	switch ( GM_PINS )
	{
		case 0x5 :
		gm1_hits++;
		flash_gm1();
		gm_buzz();
		break;
			
		case 0x3 :
		gm2_hits++;
		flash_gm2();
		gm_buzz();
		break;
			
		case ( GM_PINS & 0x0 ) :
		muon_hits++;
		gm1_hits++;
		gm2_hits++;
		muon_total++;
		flash_muon();
		muon_buzz();
		
		break;
	}
	
	//reset
	switch( CONTROL_PINS )
	{
		case 0x0 :
		Initialize();
		break;
	}
}
ISR(PCINT2_vect, ISR_BLOCK)
{
	_delay_ms(8);
	gm_buzz_comp++;
	uint8_t CONTROL_PINS = (PIND & 0xE0) >> 5;
	update_counter();
	uint8_t lightstate = ((state & 0xC) >> 2);
	uint8_t buzzerstate = (state & 0x3);
	switch ( CONTROL_PINS ){
		case 0x3:
		switch ( modes )
		{
			case 0x0:
			modes = 0x40;
			plateau_display_init();
			update_state();
			break;
			
			case 0x40:
			modes = 0x0;
			rolling_display_init();
			update_state();
			break;
			modes = DEFAULT_MODE;
			update_counter();
		}
		break;
		
		case 0x5:
		switch ( buzzerstate )
		{
			case 0x3:
			state &= ~0x2;
			update_state();
			break;
			
			case 0x2:
			state &= ~0x2;
			update_state();
			break;
			
			case 0x1:
			state &= ~0x1;
			state |= 0x2;
			update_state();
			break;
			
			case 0x0:
			state |= 0x3;
			update_state();
			break;
			
			state = DEFAULT_STATE;
			update_state();
		}
		break;
		
		case 0x6:
		switch ( lightstate )
		{
			case 0x3 :
			state &= ~0x8;
			PORTD |= (1 << LIGHTSWITCH);
			update_state();
			break;
			
			case 0x2 :
			state &= ~0x8;
			PORTD &= ~(1 << LIGHTSWITCH);
			update_state();
			break;
			
			case 0x1 :
			state &= ~0x4;
			state |= 0x8;
			PORTD &= ~(1 << LIGHTSWITCH);
			update_state();
			break;
			
			case 0x0 :
			state |= 0x4;
			PORTD |= (1 << LIGHTSWITCH);
			update_state();
			break;
			
			state = DEFAULT_STATE;
			update_counter();
		}
		break;
		
	}
}
ISR(TIMER0_OVF_vect)
{
	timer_time++;
	
	gm1_sum += gm1_hits;
	gm2_sum += gm2_hits;
	muon_sum += muon_hits;
	
	gm1_hits = 0;
	gm2_hits = 0;
	muon_hits = 0;
	
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
			plateau_counter_update();
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
	gm1_sum = 0;
	gm2_sum = 0;
	muon_sum = 0;
	gm_LED_comp = 0;
	muon_LED_comp = 0;
	gm_buzz_comp = 0;
	muon_buzz_comp = 0;
	gm1_hits = 0;
	gm2_hits = 0;
	muon_hits = 0;
	gm1_total = 0;
	gm2_total = 0;
	for ( uint8_t i = 0; i < MEMORY_LIMIT; i++){
		gm1_rolling[i] = 0;
		gm2_rolling[i] = 0;
		muon_rolling[i] = 0;
	}
	gm1_cnt_per_min = 0;
	gm2_cnt_per_min = 0;
	muon_cnt_per_min = 0;
	muon_cnt_per_hour = 0;
	muon_total = 0;
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
	LcdFStr(FONT_1X,(unsigned char*)PSTR("Firmware:v0.3a"));
	LcdUpdate();
	LcdGotoXYFont(1,6);
	LcdFStr(FONT_1X,(unsigned char*)PSTR("muonhunter.com"));
	LcdUpdate();
	
	_delay_ms(3000);
	rolling_display_init();
		change_clock(0x0);

update_counter();
}

void update_state()
{
	LcdGotoXYFont(3,1);
	switch ( ((state & 0xC) >> 2) )
	{
		case 0x3 :
		LcdFStr(FONT_1X,(unsigned char*)PSTR("OFF"));
		LcdUpdate();
		break;
		
		case 0x2 :
		LcdFStr(FONT_1X,(unsigned char*)PSTR("LED"));
		LcdUpdate();
		break;
		
		case 0x1 :
		LcdFStr(FONT_1X,(unsigned char*)PSTR("ON "));
		LcdUpdate();
		break;
		
		case 0x0 :
		LcdFStr(FONT_1X,(unsigned char*)PSTR("OFF"));
		LcdUpdate();
		break;
	}
	LcdGotoXYFont(10,1);
	switch ( (state & 0x3) )
	{
		case 0x3 :
		LcdFStr(FONT_1X,(unsigned char*)PSTR("MU GM"));
		LcdUpdate();
		break;
		
		case 0x2 :
		LcdFStr(FONT_1X,(unsigned char*)PSTR("GM   "));
		LcdUpdate();
		break;
		
		case 0x1 :
		LcdFStr(FONT_1X,(unsigned char*)PSTR("MU   "));
		LcdUpdate();
		break;
		
		case 0x0 :
		LcdFStr(FONT_1X,(unsigned char*)PSTR("OFF  "));
		LcdUpdate();
		break;
	}
}
void update_counter()
{
	double extrap = 0;
	uint8_t muon_extrap = 0;
	// see register descriptions 
	if ( modes == 0x3F || modes == 0x35 )
	// second modes
	{
		// muon total
		LcdGotoXYFont(10,2);
		dtostrf((double)muon_total,5,0,str);
		LcdStr(FONT_1X, str);
		LcdUpdate();
		// Muons
		LcdGotoXYFont(10,3);
		if ( timer_min < 1 && timer_hour < 1 && timer_day < 1)
		{
			LcdGotoXYFont(9,3);
			LcdFStr(FONT_1X,(unsigned char*)PSTR("*"));
			LcdUpdate();
			if ( timer_sec != 0 )
			{
				extrap = ((double)muon_extrap*60 / (timer_sec));
				}else{
				extrap = (double)0;
			}
			
			}else{
			LcdGotoXYFont(9,3);
			LcdFStr(FONT_1X,(unsigned char*)PSTR(" "));
			LcdUpdate();
			dtostrf((double)muon_rolling[timer_min],5,0,str);
		}
		
		LcdStr(FONT_1X, str);
		LcdUpdate();
		
		// GM1
		LcdGotoXYFont(9,4);
		LcdFStr(FONT_1X,(unsigned char*)PSTR(" "));
		LcdUpdate();
		dtostrf((double)gm1_rolling[timer_sec],5,0,str);
		LcdStr(FONT_1X, str);
		LcdUpdate();
		
		// GM2
		LcdGotoXYFont(9,5);
		LcdFStr(FONT_1X,(unsigned char*)PSTR(" "));
		LcdUpdate();
		dtostrf((double)gm2_rolling[timer_sec],5,0,str);
		LcdStr(FONT_1X, str);
		LcdUpdate();
		
		} else {
		// minute modes
		
		// muon total
		LcdGotoXYFont(10,2);
		dtostrf((double)muon_total,5,0,str);
		LcdStr(FONT_1X, str);
		LcdUpdate();
		// Muons
		LcdGotoXYFont(10,3);
		if ( timer_hour < 1 && timer_min > 0)
		{
			LcdGotoXYFont(9,3);
			LcdFStr(FONT_1X,(unsigned char*)PSTR("*"));
			LcdUpdate();
			for ( uint8_t i = 0 ; i < MEMORY_LIMIT ; i++ )
			{
				muon_extrap += muon_rolling[i];
			}
			extrap = ((double)muon_extrap*3600 / (timer_min*60 + timer_sec));
			set_muon_roll_buffer(1, ((uint16_t)extrap & 0xFF00) >> 8, (uint16_t)extrap & 0xFF);
			dtostrf(extrap,5,0,str);
		}
		if ( timer_min < 1 && timer_hour < 1 && timer_day < 1)
		{
			LcdGotoXYFont(9,3);
			LcdFStr(FONT_1X,(unsigned char*)PSTR("*"));
			LcdUpdate();
			if ( timer_sec > 0)
			{
				extrap = (double)muon_cnt_per_min*3600/timer_sec;
				set_muon_roll_buffer(1, ((uint16_t)extrap & 0xFF00) >> 8, (uint16_t)extrap & 0xFF);
				dtostrf(extrap,5,0,str);
				}else{
				set_muon_roll_buffer(1, 0, 0);
				dtostrf((double)0,5,0,str);
			}
			
			
		}
		if ( timer_hour > 0 || timer_day > 0 )
		{
			LcdGotoXYFont(9,3);
			LcdFStr(FONT_1X,(unsigned char*)PSTR(" "));
			LcdUpdate();
			for ( uint8_t i = 0 ; i < MEMORY_LIMIT ; i++ )
			{
				muon_cnt_per_hour += muon_rolling[i];
			}
			set_muon_roll_buffer(0, ((uint16_t)muon_cnt_per_hour & 0xFF00) >> 8, (uint16_t)muon_cnt_per_hour & 0xFF);
			dtostrf((double)muon_cnt_per_hour,5,0,str);
			muon_cnt_per_hour = 0;
		}
		
		LcdStr(FONT_1X, str);
		LcdUpdate();
		
		// GM1
		LcdGotoXYFont(10,4);
		gm1_cnt_per_min = 0;
		for ( uint8_t i = 0 ; i < MEMORY_LIMIT ; i++ )
		{
			gm1_cnt_per_min += gm1_rolling[i];
		}
		set_GM1_roll_buffer(0, ((uint16_t)gm1_cnt_per_min & 0xFF00) >> 8, (uint16_t)gm1_cnt_per_min & 0xFF);
		
		if ( timer_min < 1 && timer_hour < 1 && timer_day < 1 )
		{
			LcdGotoXYFont(9,4);
			LcdFStr(FONT_1X,(unsigned char*)PSTR("*"));
			LcdUpdate();
			if ( timer_sec < 1 )
			{
				set_GM1_roll_buffer(1, 0, 0);
				dtostrf((double)0,5,0,str);
				}else{
				extrap = (double)gm1_cnt_per_min*60/timer_sec ;
				set_GM1_roll_buffer(1, ((uint16_t)extrap & 0xFF00) >> 8, (uint16_t)extrap & 0xFF);
				dtostrf(extrap,5,0,str);
			}
			}else{
			LcdGotoXYFont(9,4);
			LcdFStr(FONT_1X,(unsigned char*)PSTR(" "));
			LcdUpdate();
			dtostrf((double)gm1_cnt_per_min,5,0,str);
		}
		LcdStr(FONT_1X, str);
		LcdUpdate();
		
		// GM2
		LcdGotoXYFont(10,5);
		gm2_cnt_per_min = 0;
		for ( uint8_t i = 0 ; i < MEMORY_LIMIT ; i++ )
		{
			gm2_cnt_per_min += gm2_rolling[i];
		}
		set_GM2_roll_buffer(0, ((uint16_t)gm2_cnt_per_min & 0xFF00) >> 8, (uint16_t)gm2_cnt_per_min & 0xFF);
		if ( timer_min < 1 && timer_hour < 1 && timer_day < 1 )
		{
			LcdGotoXYFont(9,5);
			LcdFStr(FONT_1X,(unsigned char*)PSTR("*"));
			LcdUpdate();
			if ( timer_sec < 1 )
			{
				set_GM2_roll_buffer(1, 0, 0);
				dtostrf((double)0,5,0,str);
				}else{
				extrap = (double)gm1_cnt_per_min*60/timer_sec ;
				set_GM2_roll_buffer(1, ((uint16_t)extrap & 0xFF00) >> 8, (uint16_t)extrap & 0xFF);
				dtostrf(extrap,5,0,str);
			}
			}else{
			LcdGotoXYFont(9,5);
			LcdFStr(FONT_1X,(unsigned char*)PSTR(" "));
			LcdUpdate();
			dtostrf((double)gm2_cnt_per_min,5,0,str);
		}
		LcdStr(FONT_1X, str);
		LcdUpdate();
	}
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
		LcdClear();
		LcdImage(st);
		LcdUpdate();
		update_state();
		LcdGotoXYFont(2,2);
		LcdFStr(FONT_1X,(unsigned char*)PSTR(" total:"));
		LcdUpdate();
		LcdGotoXYFont(10,2);
		dtostrf((double)muon_total,5,0,str);
		LcdStr(FONT_1X, str);
		LcdUpdate();
		LcdGotoXYFont(2,3);
		LcdFStr(FONT_1X,(unsigned char*)PSTR(" /hour:"));
		LcdUpdate();
		LcdGotoXYFont(1,4);
		LcdFStr(FONT_1X,(unsigned char*)PSTR("GM1/min:"));
		LcdUpdate();
		LcdGotoXYFont(1,5);
		LcdFStr(FONT_1X,(unsigned char*)PSTR("GM2/min:"));
		LcdUpdate();
		break;
		case 0x1: // change to /sec mode
		// timer clock set to clk / 8
		// TODO: disable LEDs and buzzer in second mode
		timer_time = timer_time*TIMER_OVF_S / TIMER_OVF_M;
		TIMSK0 = 0x0;
		TCCR0B = 0x2;
		// counter overflow interrupt enabled
		TIMSK0 = 0x1;
		LcdClear();
		LcdImage(st);
		LcdUpdate();
		update_state();
		LcdGotoXYFont(2,2);
		LcdFStr(FONT_1X,(unsigned char*)PSTR(" total:"));
		LcdUpdate();
		LcdGotoXYFont(10,2);
		dtostrf((double)muon_total,5,0,str);
		LcdStr(FONT_1X, str);
		LcdUpdate();
		LcdGotoXYFont(2,3);
		LcdFStr(FONT_1X,(unsigned char*)PSTR(" /min:"));
		LcdUpdate();
		LcdGotoXYFont(1,4);
		LcdFStr(FONT_1X,(unsigned char*)PSTR("GM1/sec:"));
		LcdUpdate();
		LcdGotoXYFont(1,5);
		LcdFStr(FONT_1X,(unsigned char*)PSTR("GM2/sec:"));
		LcdUpdate();
		break;
	}
}
void timer_update()
{
	timer_time = 0;
	gm1_rolling[timer_sec] = gm1_sum;
	gm2_rolling[timer_sec] = gm2_sum;
	gm1_total += gm1_sum;
	gm2_total += gm2_sum;
	txbuffer[0x9] = ( gm1_total & 0xFF000000 ) >> 24;
	txbuffer[0x8] = ( gm1_total & 0xFF0000 ) >> 16;
	txbuffer[0x7] = ( gm1_total & 0xFF00 ) >> 8;
	txbuffer[0x6] = ( gm1_total & 0xFF );
	txbuffer[0x5] = ( gm2_total & 0xFF000000 ) >> 24;
	txbuffer[0x4] = ( gm2_total & 0xFF0000 ) >> 16;
	txbuffer[0x3] = ( gm2_total & 0xFF00 ) >> 8;
	txbuffer[0x2] = ( gm2_total & 0xFF );
	txbuffer[0x1] = ( muon_total & 0xFF00 ) >> 8;
	txbuffer[0x0] = ( muon_total & 0xFF ); 
	muon_cnt_per_min += muon_sum;
	timer_sec++;
	timer_compensate();
	gm1_sum = 0;
	gm2_sum = 0;
	muon_sum = 0;
	if ( timer_sec == MEMORY_LIMIT * (60 / MEMORY_LIMIT) )
	{
		muon_rolling[timer_min] = muon_cnt_per_min;
		muon_cnt_per_min = 0;
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

void plateau_counter_update(){
		double extrap = 0;
		uint8_t muon_extrap = 0;
		// muon total
		LcdGotoXYFont(10,2);
		dtostrf((double)muon_total,5,0,str);
		LcdStr(FONT_1X, str);
		LcdUpdate();
		// Muons
		LcdGotoXYFont(10,3);
		if ( timer_hour < 1 && timer_min > 0)
		{
			LcdGotoXYFont(9,3);
			LcdFStr(FONT_1X,(unsigned char*)PSTR("*"));
			LcdUpdate();
			for ( uint8_t i = 0 ; i < MEMORY_LIMIT ; i++ )
			{
				muon_extrap += muon_rolling[i];
			}
			extrap = ((double)muon_extrap*3600 / (timer_min*60 + timer_sec));
			dtostrf(extrap,5,0,str);
		}
		if ( timer_min < 1 && timer_hour < 1 && timer_day < 1)
		{
			LcdGotoXYFont(9,3);
			LcdFStr(FONT_1X,(unsigned char*)PSTR("*"));
			LcdUpdate();
			if ( timer_sec > 0)
			{
				dtostrf((double)muon_cnt_per_min*3600/timer_sec,5,0,str);
				}else{
				dtostrf((double)0,5,0,str);
			}
			
			
		}
		if ( timer_hour > 0 || timer_day > 0 )
		{
			LcdGotoXYFont(9,3);
			LcdFStr(FONT_1X,(unsigned char*)PSTR(" "));
			LcdUpdate();
			for ( uint8_t i = 0 ; i < MEMORY_LIMIT ; i++ )
			{
				muon_cnt_per_hour += muon_rolling[i];
			}
			dtostrf((double)muon_cnt_per_hour,5,0,str);
			muon_cnt_per_hour = 0;
		}
		
		LcdStr(FONT_1X, str);
		LcdUpdate();
		
		//GM1 totals display
		LcdGotoXYFont(10,4);
		LcdGotoXYFont(9,4);
		LcdFStr(FONT_1X,(unsigned char*)PSTR(" "));
		LcdUpdate();
		dtostrf((double)gm1_total,5,0,str);
		LcdStr(FONT_1X, str);
		LcdUpdate();
		
		//GM2 totals display
		LcdGotoXYFont(10,5);
		LcdGotoXYFont(9,5);
		LcdFStr(FONT_1X,(unsigned char*)PSTR(" "));
		LcdUpdate();
		dtostrf((double)gm2_total,5,0,str);
		LcdStr(FONT_1X, str);
		LcdUpdate();
		
		display_time();
}
void plateau_display_init(){
	
	PCICR = 0x6;
	PCMSK1 = 0xF;
	PCMSK2 = 0xE0;
	DDRC = 0x0;
	PORTC = 0x3F;
	LcdInit();
	LcdContrast(0x41);
	LcdClear();
	LcdUpdate();
	LcdImage(st);
	LcdUpdate();
	LcdGotoXYFont(2,2);
	LcdFStr(FONT_1X,(unsigned char*)PSTR(" total:"));
	LcdUpdate();
	LcdGotoXYFont(10,2);
	dtostrf((double)muon_total,5,0,str);
	LcdStr(FONT_1X, str);
	LcdUpdate();
	LcdGotoXYFont(2,3);
	LcdFStr(FONT_1X,(unsigned char*)PSTR(" /hour:"));
	LcdUpdate();
	LcdGotoXYFont(1,4);
	LcdFStr(FONT_1X,(unsigned char*)PSTR("GM1 tot:"));
	LcdUpdate();
	LcdGotoXYFont(1,5);
	LcdFStr(FONT_1X,(unsigned char*)PSTR("GM2 tot:"));
	LcdUpdate();
		
	plateau_counter_update();
	}

void rolling_display_init(){
	PCICR = 0x6;
	PCMSK1 = 0xF;
	PCMSK2 = 0xE0;
	DDRC = 0x0;
	PORTC = 0x3F;
	LcdInit();
	LcdContrast(0x41);
	LcdClear();
	LcdUpdate();
	LcdImage(st);
	LcdUpdate();
	LcdGotoXYFont(2,2);
	LcdFStr(FONT_1X,(unsigned char*)PSTR(" total:"));
	LcdUpdate();
	LcdGotoXYFont(10,2);
	dtostrf((double)muon_total,5,0,str);
	LcdStr(FONT_1X, str);
	LcdUpdate();
	LcdGotoXYFont(2,3);
	LcdFStr(FONT_1X,(unsigned char*)PSTR(" /hour:"));
	LcdUpdate();
	LcdGotoXYFont(1,4);
	LcdFStr(FONT_1X,(unsigned char*)PSTR("GM1/min:"));
	LcdUpdate();
	LcdGotoXYFont(1,5);
	LcdFStr(FONT_1X,(unsigned char*)PSTR("GM2/min:"));
	LcdUpdate();
	
	update_counter();
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
void set_muon_roll_buffer(uint8_t extrapolation_flag, uint8_t MSB, uint8_t LSB)
{
			txbuffer[0xC] = extrapolation_flag;
			txbuffer[0xB] = MSB;
			txbuffer[0xA] = LSB;
	}
void set_GM1_roll_buffer(uint8_t extrapolation_flag, uint8_t MSB, uint8_t LSB)
{
			txbuffer[0xF] = extrapolation_flag;
			txbuffer[0xE] = MSB;
			txbuffer[0xD] = LSB;
	}
void set_GM2_roll_buffer(uint8_t extrapolation_flag, uint8_t MSB, uint8_t LSB)
{
			txbuffer[0x12] = extrapolation_flag;
			txbuffer[0x11] = MSB;
			txbuffer[0x10] = LSB;
	}
