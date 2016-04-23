/* 
 *
 * MuonHunter.c
 * Created: 18/08/2015 10:28:44
 * Last modified: 23/04/2016 15:01:43
 * 
 * Author: Mihaly Vadai
 * Website:	http://muonhunter.com
 *
 * License: GPL v.3
 * Version: 0.2
 */
#define F_CPU 8192000UL

#include <avr/io.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdio.h>
#include "TWI_Master.c"
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
void Initialize()
{
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
	LcdFStr(FONT_1X,(unsigned char*)PSTR("Firmware: v0.2"));
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
	if ( modes == 0x3F || modes == 0x15 )
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
		
		// GM1
		LcdGotoXYFont(10,4);
		gm1_cnt_per_min = 0;
		for ( uint8_t i = 0 ; i < MEMORY_LIMIT ; i++ )
		{
			gm1_cnt_per_min += gm1_rolling[i];
		}
		
		if ( timer_min < 1 && timer_hour < 1 && timer_day < 1 )
		{
			LcdGotoXYFont(9,4);
			LcdFStr(FONT_1X,(unsigned char*)PSTR("*"));
			LcdUpdate();
			if ( timer_sec < 1 )
			{
				dtostrf((double)0,5,0,str);
				}else{
				dtostrf((double)gm1_cnt_per_min*60/timer_sec,5,0,str);
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
		
		if ( timer_min < 1 && timer_hour < 1 && timer_day < 1 )
		{
			LcdGotoXYFont(9,5);
			LcdFStr(FONT_1X,(unsigned char*)PSTR("*"));
			LcdUpdate();
			if ( timer_sec < 1 )
			{
				dtostrf((double)0,5,0,str);
				}else{
				dtostrf((double)gm2_cnt_per_min*60/timer_sec,5,0,str);
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
	PORTD |= (1 << GM1_LED);
	_delay_ms(64);
	PORTD &= ~(1 << GM1_LED);
	gm_LED_comp++;
	}
}
void flash_gm2(){
	if ((state & 0xC) >> 2 == 0x1 || (state & 0xC) >> 2 == 0x2 ){
	PORTD &= ~(1 << MUON_LED);
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
	DDRD = 0x1F;
	DDRC = 0x0;
	PORTC = 0x3F;
	PORTD = 0xE0;
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
	DDRD = 0x1F;
	DDRC = 0x0;
	PORTC = 0x3F;
	PORTD = 0xE0;
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
