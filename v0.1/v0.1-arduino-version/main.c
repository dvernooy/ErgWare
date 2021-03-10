/*

Copyright (C) David W. Vernooy

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <avr/io.h>
#include <stdbool.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <stdio.h>
#include "lcd.h"
#include "time.h"
#include "math.h"



/********************************************************************************
Defines
********************************************************************************/

static FILE lcd_out = FDEV_SETUP_STREAM(lcd_chr_printf, NULL, _FDEV_SETUP_WRITE);

#define LEFT		bit_is_clear(PIND, PD5)
#define	MIDDLE		bit_is_clear(PIND, PD6)
#define	RIGHT		bit_is_clear(PIND, PD7)

/* Button definitions*/
#define K_LEFT		1
#define	K_MIDDLE	2
#define	K_RIGHT		3

/*number of constants for averaging*/
#define MAX_N	5

/********************************************************************************
    Constants
********************************************************************************/
double J_moment = 0.16; //kg*m^2 - set this to the moment of inertia of your flywheel. 
						//look at documentation for ways to measure it. Its easy to do.
double d_omega_div_omega2 = 0.0031;//erg_constant - to get this for your erg:
// Uncomment the code below that writes out the constant K_damp_esimator_vector[0]. Then run the erg, 
//run that version of the software, get the number and add it to this line, then re-compile.
//It should be close to value above.  I decided just to hard code it instead of dynamically updating 
//it during the running of the software, though that would be easy and is presumably what other ergs do.
double K_damp = 0.0; //related by J_moment*omega_dot = K_damp * omega^2 during non-power part of stroke
//I calcualate this below.
double magic_factor = 2.8; //a heuristic constant people use to relate revs to distance-rowed
double cal_factor = 0.0; //distance per rev ... calculated later
double pi = 3.1415926;
double seconds_per_tick = 0.000016; //8us per tick with 64 divider in timer1 & 8MHz clock

/********************************************************************************
Global Variables
********************************************************************************/
int elapsed_hours = 0;
int elapsed_mins = 0;
int elapsed_secs = 0;

int split_hours = 0;
int split_mins = 0;
int split_secs = 0;
double split_time = 0.0;

uint8_t volatile CmdButton = 0;
uint8_t chop_ticks = 0;
uint8_t uptick, downtick, upfound;

double elapsed = 0.0;
double average_SPM = 0.0;
double temp_minutes = 0.0;
double volatile distance_rowed = 0.0;
uint8_t volatile locked = 1;

uint16_t volatile chop_counter[2];
double stroke_vector[MAX_N];
double stroke_vector_avg;
double power_vector[MAX_N];
double power_vector_avg;
double K_power = 0.0;
double J_power = 0.0;
double power_ratio_vector[MAX_N];
double K_damp_estimator_vector[MAX_N];
double K_damp_estimator_vector_avg;

double K_damp_estimator = 0.0;
double speed_vector[MAX_N];
double speed_vector_avg = 0.0;
double power_ratio_vector_avg = 0.0;

double stroke_elapsed = 0.0;
double power_elapsed = 0.0;

double stroke_distance = 0.0;
double stroke_distance_old = 0.0;


double omega_vector[2];
double omega_dot_vector[2];
double omega_dot_dot = 0.0;
double current_dt = 0.0;
double omega_vector_avg = 0.0;
double omega_vector_avg_curr = 0.0;


uint8_t omega_dot_screen = 0;
uint8_t omega_dot_dot_screen = 0;
uint8_t power_stroke_screen[2];


uint16_t stroke = 0;
uint8_t updatecount;

uint16_t squelch_count = 0;
uint8_t trapped = 0;
uint8_t squelched = 0;

uint8_t j = 0;

TIME t_stroke, t_power;


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

double  weighted_avg(double *vector) {
double temp_weight, temp_sum;
uint8_t j;
temp_sum = 0.0;
temp_weight = 0.0;
	for (j =0;j<MAX_N; j++) {
		temp_weight += pow(2,MAX_N-j-1);
		temp_sum += pow(2,MAX_N-j-1) * vector[j];
	}
	temp_sum = temp_sum/temp_weight;
return temp_sum;
}


//Initialize timer 
void InitTimer1(void) { 
	TCCR1A = 0b00000000;
	TCCR1B = 0b00000100;//clk/256 which is 16us per tick
	TCCR2A = 0b00000010;//CTC mode - reset TCNT2 at OCR2A
	TCCR2B = 0b00000111;//clk/1024 which is 64us per tick
	TIMSK2 = 0b00000010;//output compare A interrupt 
	OCR2A = 0x9D; //sets time of interrupt to 10 ms

	DDRD = 0b00000000;//enable portD as input
	PORTD|=0b11100100; //turn on pullups
	EICRA = 0b00000011; //rising edge INT0 (PD2)
	EIMSK = 0b00000001;
	TCNT1 = 0;
	TCNT2 = 0;
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
		GetTime(&t_stroke);
		GetTime(&t_power);
	}
		
	if (CmdButton == K_MIDDLE) {
		CmdButton = 0;
		distance_rowed = 0;
		cli();
		Reset_timer();
		sei();
		stroke = 0;
		GetTime(&t_stroke);
		GetTime(&t_power);
	}
	
	if (CmdButton == K_RIGHT) {
		CmdButton = 0;
		distance_rowed = 0;
	}		
}

ISR(INT0_vect)
{
  cli();
  chop_ticks++;
 
  if ((chop_ticks % 8 ) == 0) {
	chop_ticks = 0;
	chop_counter[1] = chop_counter[0];
	chop_counter[0] = TCNT1;
	TCNT1 = 0;
	distance_rowed += cal_factor;
	if (locked ==1) locked = 0; //used for stroke count
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
	/**************************
	calculate other constants
	*************************/
	K_damp = J_moment*d_omega_div_omega2; //= 0.0005 Nms^2 
	cal_factor = 2.0*pi*pow((K_damp/magic_factor), 1.0/3.0); //distance per rev = 0.3532
	
	_delay_ms(1000);
	_delay_ms(1000);
	_delay_ms(1000);


	
	lcd_init();
	lcd_contrast(0x40);
	
	locked = 1;
	
	TimeInit();	//start timer routine
	InitTimer1(); 
	sei();
	

	chop_counter[0]= 0;
	chop_counter[1]= 0;

	omega_vector[0]= 0.0;
	omega_vector[1]= 0.0;
	
	omega_dot_vector[0]= 0.0;
	omega_dot_vector[1]= 0.0;
	
	power_stroke_screen[0]= 1;
	power_stroke_screen[1]= 0;
	
	for (j = 0; j<MAX_N; j++) {	
		power_vector[j]= 0.0;
		speed_vector[j]= 0.0;
		power_ratio_vector[j]= 0.0;
		K_damp_estimator_vector[j]= 0.0;
		stroke_vector[j]= 0.0;
	}
	
	omega_dot_screen= 0;
	omega_dot_dot_screen= 0;
	
	K_power = 0.0;
	J_power = 0.0;
	
	stroke_elapsed = 0.0;
	power_elapsed = 0.0;
	
	uint32_t temp;
	
	// Print on first line
	lcd_clear();
	lcd_goto_xy(1,1);
	invert = 1;
    fprintf_P(&lcd_out,PSTR(" Luca's Ergo  "));
	invert = 0;
	
	GetTime(&t_stroke);
	GetTime(&t_power);

	uptick = 0;
	upfound = 0;
	downtick = 0;
	updatecount = 0;
	stroke = 0;
	
while(1) 
{ 
updatecount++;

if (updatecount % 100 ==0) {
	parse_cmd();
}	

if (locked ==0) {
	//calculate omegas
	current_dt = (double) chop_counter[0]*seconds_per_tick;
	omega_vector[1] = omega_vector[0];
	omega_vector[0] = (2.0*pi)/((1/2.0)*seconds_per_tick*((double) chop_counter[0]+(double) chop_counter[1]));
	omega_dot_vector[1] = omega_dot_vector[0];
	omega_dot_vector[0] = (omega_vector[0] - omega_vector[1])/(current_dt+0.0001);
    omega_dot_dot = (omega_dot_vector[0] - omega_dot_vector[1])/(current_dt+0.0001);
	
	//calculate screeners to find power portion of stroke - see spreadsheet if you want to understand this
	
	if ((omega_dot_dot > -40.0) && (omega_dot_dot < 40.0)) {
		omega_dot_dot_screen = 0;
	}
	else {
		omega_dot_dot_screen = 1;
	}
	if (omega_dot_vector[0] > 15) {
		omega_dot_screen = 1;
	}
	else {
		omega_dot_screen = 0;
	}
	if ( (omega_dot_dot_screen ==1) || ((omega_dot_screen ==1) && (power_stroke_screen[0] ==1))) {
		power_stroke_screen[1] = power_stroke_screen[0];
		power_stroke_screen[0] = 1;
	}	
	else {
		power_stroke_screen[1] = power_stroke_screen[0];
		power_stroke_screen[0] = 0;
	}
	//if just ended decay stroke, just started power stroke
	if ((power_stroke_screen[0] ==1) && (power_stroke_screen[1] ==0)) {
		//start clock1 = power timer
		GetTime(&t_power);
		J_power = 0.0;
		K_power = 0.0;
		K_damp_estimator_vector[4]= K_damp_estimator_vector[3];
		K_damp_estimator_vector[3]= K_damp_estimator_vector[2];
		K_damp_estimator_vector[2]= K_damp_estimator_vector[1];
		K_damp_estimator_vector[1]= K_damp_estimator_vector[0];
		K_damp_estimator_vector[0] = K_damp_estimator/(stroke_elapsed-power_elapsed+.000001);
		K_damp_estimator_vector_avg = weighted_avg(K_damp_estimator_vector);

		omega_vector_avg = omega_vector_avg + omega_vector[0]*current_dt;
	}
	
	//if just ended power stroke, starting decay stroke
	if ((power_stroke_screen[0] ==0) && (power_stroke_screen[1] ==1)) {
		stroke++;
		K_damp_estimator = 0.0;
		//end clock1 = power_timer
		power_elapsed = (double)Get_elapsed_ms(&t_power)/1000.0;
		//end clock2 = stroke_timer
		stroke_elapsed = (double)Get_elapsed_ms(&t_stroke)/1000.0;
		//start clock2 = stroke timer
		GetTime(&t_stroke);
		omega_vector_avg_curr = omega_vector_avg/(stroke_elapsed+0.0001);
		omega_vector_avg = 0.0;
		stroke_distance = distance_rowed-stroke_distance_old;
		stroke_distance_old = distance_rowed;
		
		speed_vector[4]= speed_vector[3];
		speed_vector[3]= speed_vector[2];
		speed_vector[2]= speed_vector[1];
		speed_vector[1]= speed_vector[0];
		speed_vector[0] = stroke_distance/(stroke_elapsed +0.0001);
		speed_vector_avg = weighted_avg(speed_vector);

		power_ratio_vector[4]= power_ratio_vector[3];
		power_ratio_vector[3]= power_ratio_vector[2];
		power_ratio_vector[2]= power_ratio_vector[1];
		power_ratio_vector[1]= power_ratio_vector[0];
		power_ratio_vector[0] = power_elapsed/(stroke_elapsed + 0.0001);
		power_ratio_vector_avg = weighted_avg(power_ratio_vector);

		power_vector[4]= power_vector[3];
		power_vector[3]= power_vector[2];
		power_vector[2]= power_vector[1];
		power_vector[1]= power_vector[0];
		power_vector[0]= (J_power + K_power)/(stroke_elapsed + 0.0001);
		power_vector_avg = weighted_avg(power_vector);
		
		stroke_vector[4]= stroke_vector[3];
		stroke_vector[3]= stroke_vector[2];
		stroke_vector[2]= stroke_vector[1];
		stroke_vector[1]= stroke_vector[0];
		stroke_vector[0]= stroke_elapsed;
		stroke_vector_avg = weighted_avg(stroke_vector);
	}

	//if inside power stroke
	if ((power_stroke_screen[0] ==1) && (power_stroke_screen[1] ==1)) {
		J_power = J_power + J_moment * omega_vector[0]*omega_dot_vector[0]*current_dt;
		K_power = K_power + K_damp*(omega_vector[0]*omega_vector[0]*omega_vector[0])*current_dt;
		omega_vector_avg = omega_vector_avg + omega_vector[0]*current_dt;

	}
	
	//if in decay stroke 
	if ((power_stroke_screen[0] ==0) && (power_stroke_screen[1] ==0)) {
		K_damp_estimator = K_damp_estimator+(omega_dot_vector[0]/((omega_vector[0]+0.0001)*(omega_vector[0]+0.0001)))*current_dt;
		omega_vector_avg = omega_vector_avg + omega_vector[0]*current_dt;
	}
	locked = 1;
		
}

if (updatecount % 500 ==0) {
	/************* update time ****************/
		temp = (uint32_t)getSeconds();
		temp_minutes = (double) temp;
		temp_minutes = temp_minutes/60.0;
		lcd_goto_xy(3,6);
		parse_time((double) temp_minutes, &elapsed_hours, &elapsed_mins, &elapsed_secs);
		fprintf_P(&lcd_out,PSTR("%02d:%02d"),elapsed_mins, elapsed_secs);
		
	/************ update distance ************/	
		lcd_goto_xy(3,5);
		fprintf_P(&lcd_out,PSTR("%1.0f meters    "), distance_rowed);


	/************ update stroke rate ************/	
				
		lcd_goto_xy(1,2);
		if (stroke < 5) {
			fprintf_P(&lcd_out,PSTR("SPM --  "));
		}
		else {
			fprintf_P(&lcd_out,PSTR("SPM %2.0f  "), 60.0/(stroke_vector_avg+0.0001));
		}	
		lcd_goto_xy(9,2);
		if ((stroke < 2) || ((elapsed_hours ==0) && (elapsed_mins ==0) && (elapsed_secs < 30))) {
			fprintf_P(&lcd_out,PSTR("avg --"));
		}
		else {
			average_SPM = (double) stroke/((double)elapsed_hours*60.0+(double)elapsed_mins+(double)elapsed_secs/60.0+0.01);
			fprintf_P(&lcd_out,PSTR("avg %2.0f"), average_SPM);
		}	
	
	/************ update stroke count & ratio ************/	
	
	lcd_goto_xy(1,3);
		fprintf_P(&lcd_out,PSTR("S# %d    "), stroke);
	
	
		lcd_goto_xy(11,3);
		fprintf_P(&lcd_out,PSTR("%1.2f  "), power_ratio_vector_avg);
		
				
		//uncomment this code if you want to display the d_omega_div_omega2 constant for your erg
		
		//(and comment out the 2 lines directly above that display
		// power_ratio_wector_avg display above so it doesn't over-write)
		
		//to get K_damp, you multiply this number times J_moment.
		//i.e. K_damp = J_moment*(-1*K_damp_estimator_vector[0]) = J_moment*d_omega_div_omega2
		//Note, even though (d(omega)/dt)/omega^2 is negative, I made the constant d_omega_div_omega2 positive
		//hence the need for the minus signs to make it all work out. 
		
		//do a few strokes, the number will bounce around a bit ... mine read 0.0031 +/- 0.0002
		//you can add some averaging if you want.

		
		
		/*
		lcd_goto_xy(9,3);		
			fprintf_P(&lcd_out,PSTR("%1.4f"), -1*K_damp_estimator_vector[0]);

		*/


/*
		//uncomment this code if you want to display distance per stroke & time per stroke
		//and comment out the average power display below
		lcd_goto_xy(1,4);
		fprintf_P(&lcd_out,PSTR("%1.2f    "), stroke_distance);
		
		lcd_goto_xy(11,4);
		fprintf_P(&lcd_out,PSTR("%1.2f  "), stroke_vector_avg);
*/		
		
	/************ update average power ************/	
		//lcd_goto_xy(1,4);		
		lcd_goto_xy(4,4);		
		if (stroke < 5) {
			fprintf_P(&lcd_out,PSTR("--- W"));
		}
		else {
			fprintf_P(&lcd_out,PSTR("%1.0f W   "), power_vector_avg);

		}
		
		//uncomment this code if you want to display another estimator of rower power ... k*omega_ave^3
		//it was generally reasonably close to the more accurate calculation .. just a nice verification
		/*
		lcd_goto_xy(10,4);		
		if (stroke < 5) {
			fprintf_P(&lcd_out,PSTR("--- W"));
		}
		else {
			fprintf_P(&lcd_out,PSTR("%1.0f W   "), K_damp*pow(omega_vector_avg_curr,3.0));

		}
		*/
		


	/************ update 500m split ************/	
		lcd_goto_xy(10,6);
		
		split_time = 500.0/(speed_vector_avg+.001);
		temp_minutes = split_time/60.0;
		parse_time((double) temp_minutes, &split_hours, &split_mins, &split_secs);

		if (stroke < 10) {
			fprintf_P(&lcd_out,PSTR("--:--"));
		}
		else {
			fprintf_P(&lcd_out,PSTR("%02d:%02d"), split_mins, split_secs);;
		}

		updatecount = 0;
	}	
	
	
}//while(1)
}//main


























