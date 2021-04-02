/*
** main.c
**
**working code to demonstrate following:
** deployed 12-29-16
** 1. more accurate stroke counter - DONE
** 2. better averaging of current stroke rate - DONE
** 3. save a counter for buttons or other - DONE
** 4. button-based menu of options/special buttons - DONE

** next steps
** 1. add second sensor for stroke parsing or estimate from first one
** 2. make a few numbers on display bigger & easier to read
** 3. figure out how to calculate power & implement it - IN PROCESS

*/
#include <avr/io.h>
#include <stdbool.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <stdio.h>
#include "lcd.h"
#include "datatypes.h"
#include "time.h"



/********************************************************************************
Global Variables
********************************************************************************/

static FILE lcd_out = FDEV_SETUP_STREAM(lcd_chr_printf, NULL, _FDEV_SETUP_WRITE);

#include <avr/io.h> 
#include <avr/interrupt.h> 


#define LEFT		bit_is_clear(PIND, PD5)
#define	MIDDLE		bit_is_clear(PIND, PD6)
#define	RIGHT		bit_is_clear(PIND, PD7)

/* Playback control code generated by G-sensor control task */
#define K_LEFT		1
#define	K_MIDDLE	2
#define	K_RIGHT		3

/********************************************************************************
Global Variables
********************************************************************************/
int elapsed_hours = 0;
int elapsed_mins = 0;
int elapsed_secs = 0;
uint8_t volatile CmdButton = 0;
uint8_t revs = 0;
uint8_t uptick, downtick, upfound;

double elapsed = 0.0;
double average_SPM = 0.0;
double temp_minutes = 0.0;
double cal_factor = 0.5; //distance in meters per rev
double volatile distance_rowed = 0.0;
uint8_t volatile locked = 0;

uint16_t volatile chop_counter[3];
double stroke_vector[5];
double stroke_vector_avg;
uint16_t stroke;
uint8_t updatecount;
uint16_t volatile j = 0;
uint16_t j_old = 0;
uint16_t eep_pos = 0;

uint16_t squelch_count = 0;
uint8_t trapped = 0;
uint8_t squelched = 0;

TIME t;



/********************************************************************************
                                Main
********************************************************************************/


void parse_time(double time_in_min, int *hours, int *mins, int *secs) {
double temp_min;
double temp_secs;

*hours = (int) (time_in_min/60);

temp_min = time_in_min - (double) 60.0 * (*hours);
*mins = (int) temp_min;

temp_secs = 60.0*temp_min -  60.0 *(double) (*mins);
*secs = (int) temp_secs;

}


//Initialize timer 
void InitTimer1(void) { 
	/************************************************
	Set switch inputs
	*************************************************/
	DDRD &= ~((1 << PD7) |(1 << PD6) |(1 << PD5)); //set PD7/6/5 as input
	PORTD |= ((1 << PD7) |(1 << PD6) |(1 << PD5)); //turn on pull-ups
		
	TCCR2A=(1<<WGM21);//0b00000010, Normal port operation, CTC mode
	TCCR2B= (1<<CS22)|(1<<CS21)|(1<<CS20); // c/ clock/1024 = 0.128 ms Tick ... * 78 counts = 10ms	
	OCR2A=0x9D;//157 counts, then reset (8 bit means 255 max), since 0 is included, count to 157
	TIMSK2 = (1 << OCIE2A);//0b00000010; Output compare match A interrupt enable mask bit
	TCNT2 = 0;
	
	/************************************************
	Set chopper input
	*************************************************/
	DDRD &= ~(1 << PD2); //set PD2 as input
	PORTD |= (1 << PD2); //turn on pull-ups	

	EICRA = 0b00000011; //rising edge INT0 (PD2)
	EIMSK = 0b00000001;
	
	
	TCCR1A = 0b00000000;
	TCCR1B = 0b00000011;//clk/64 which is 4us per tick
	TCNT1 = 0;
	
} 

static
void parse_cmd (void)
{
	if (CmdButton == K_LEFT) {
		CmdButton = 0;
		cli();
		Reset_timer();
		sei();
		stroke = 0;
		GetTime(&t);
	}
		
	if (CmdButton == K_MIDDLE) {
		CmdButton = 0;
		distance_rowed = 0;
		cli();
		Reset_timer();
		sei();
		stroke = 0;
		GetTime(&t);
	}
	
	if (CmdButton == K_RIGHT) {
		CmdButton = 0;
		distance_rowed = 0;
	}		
}

ISR(INT0_vect)
{
  cli();
  revs++;  
 
  if ((revs % 4 ) == 0) {
	revs = 0;
	chop_counter[2] = chop_counter[1];
	chop_counter[1] = chop_counter[0];
	chop_counter[0] = TCNT1;
	TCNT1 = 0;
	distance_rowed += 0.5*cal_factor;
	if (locked ==1) locked = 0;
	j++;
  }

  sei();
}



ISR(TIMER2_COMPA_vect)
/*---------------------------------------------------------*/
/* 100Hz (10ms) timer interrupt generated by OCR2A (128 us per tick, 80 ticks) */
/*---------------------------------------------------------*/

{
	if (squelched == 1) {
		if (squelch_count < 40) {
			squelch_count++;
		}	
		else {
			squelch_count = 0;
			squelched = 0;	
		}
	}	
		
	if ((LEFT || MIDDLE || RIGHT) && (squelched == 0)) trapped++;
		
	if (trapped > 5){
		if (LEFT) CmdButton = K_LEFT; 
		if (MIDDLE) CmdButton = K_MIDDLE; 
		if (RIGHT) CmdButton = K_RIGHT; 
		trapped = 0;
		squelched = 1;
	}		

}




/********************************************************************************
                                Main
********************************************************************************/


int main(void) 
{ 
	lcd_init();
	lcd_contrast(0x36);
	
	TimeInit();	//start timer routine
	InitTimer1(); 
	sei();
	
	chop_counter[0]= 0;
	chop_counter[1]= 0;
	chop_counter[2]= 0;
	
	stroke_vector[0]= 0.0;
	stroke_vector[1]= 0.0;
	stroke_vector[2]= 0.0;
	stroke_vector[3]= 0.0;
	stroke_vector[4]= 0.0;
 
	uint32_t temp;
	
	// Print on first line
	lcd_clear();
	lcd_goto_xy(1,1);
	invert = 1;
    fprintf_P(&lcd_out,PSTR(" Sarah's Ergo "));
	invert = 0;
	
	GetTime(&t);
	uptick = 0;
	upfound = 0;
	downtick = 0;
	updatecount = 0;
	j = 0;
	j_old = 0;
	eep_pos = 0;
	
while(1) 
{ 
updatecount++;
if (updatecount % 100 ==0) {
	parse_cmd();
	lcd_goto_xy(10,4);
	fprintf_P(&lcd_out,PSTR("%d"), j);
}	


if ((j < 1000) && (j > j_old)) {
		eeprom_write_byte((uint8_t *)eep_pos,(uint8_t)(chop_counter[0] >> 8)); 			
				eep_pos = eep_pos + 1;
		eeprom_write_byte((uint8_t *)eep_pos,(uint8_t)(chop_counter[0] & 0xFF)); 			
				eep_pos = eep_pos + 1;		
				j_old = j;
	}



if (updatecount % 500 ==0) {
	//update time & distance rowed
		if (j > 1000) {
			lcd_goto_xy(14,4);
			invert = 1;
			fprintf_P(&lcd_out,PSTR("*"));
			invert = 0;
		}	
		
		temp = (UINT32)getSeconds();
		temp_minutes = (double) temp;
		temp_minutes = temp_minutes/60.0;
		lcd_goto_xy(1,6);
		parse_time((double) temp_minutes, &elapsed_hours, &elapsed_mins, &elapsed_secs);
		fprintf_P(&lcd_out,PSTR("%02d:%02d:%02d"),elapsed_hours, elapsed_mins, elapsed_secs);
		lcd_goto_xy(1,5);
		fprintf_P(&lcd_out,PSTR("%1.0f meters   "), distance_rowed);
		stroke_vector_avg = (1.0*stroke_vector[4]+2.0*stroke_vector[3]+4.0*stroke_vector[2]+8.0*stroke_vector[1]+16.0*stroke_vector[0])/31.0;			
		lcd_goto_xy(1,2);
		if (stroke < 5) {
			fprintf_P(&lcd_out,PSTR("SPM --  "));
		}
		else {
			fprintf_P(&lcd_out,PSTR("SPM %2.0f  "), 60.0/stroke_vector_avg);

		}	
		lcd_goto_xy(9,2);
		if ((stroke < 2) || ((elapsed_hours ==0) && (elapsed_mins ==0) && (elapsed_secs < 30))) {
			fprintf_P(&lcd_out,PSTR("avg --"));
		}
		else {
			average_SPM = (double) stroke/((double)elapsed_hours*60.0+(double)elapsed_mins+(double)elapsed_secs/60.0+0.01);
			fprintf_P(&lcd_out,PSTR("avg %2.0f"), average_SPM);
		}	
		updatecount = 0;
		lcd_goto_xy(1,3);
		fprintf_P(&lcd_out,PSTR("S# %d   "), stroke);
		lcd_goto_xy(11,3);
		fprintf_P(&lcd_out,PSTR("%1.2f  "), elapsed);
		updatecount = 0;
	}	

	
//calculate if stroke rate needs updating
	if ((locked ==0) && (upfound ==0) && (chop_counter[0] < chop_counter[2])) {
		locked = 1;
		uptick++;
		if (uptick > 2) {
			upfound = 1;
		}	
	}
	
	if ((locked ==0) && (upfound ==0) && (chop_counter[0] > chop_counter[2])) {
		locked = 1;
		uptick = 0;	
	}
	
	if ((locked ==0) && (upfound ==1) && (chop_counter[0] < chop_counter[2])) {
		locked = 1;
		downtick = 0;
	} 
	
	if ((locked ==0) && (upfound ==1) && (chop_counter[0] > chop_counter[2])) {
		locked = 1;
		downtick++;
		if (downtick > 4) {			
			downtick = 0;
			uptick = 0;
			upfound = 0;
			stroke++;
			elapsed = (double)GetElaspMs(&t)/1000.0;
			GetTime(&t);
			stroke_vector[4]= stroke_vector[3];
			stroke_vector[3]= stroke_vector[2];
			stroke_vector[2]= stroke_vector[1];
			stroke_vector[1]= stroke_vector[0];
			stroke_vector[0]= elapsed;			
		}
	}
	
}//while(1)
}//main

























