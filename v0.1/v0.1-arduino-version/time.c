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


#include "time.h"

uint32_t volatile Seconds=0;
uint16_t volatile MilliSeconds=0;
uint32_t volatile cnt_timer=0;


void TimeInit(void)
{
	uint8_t count;

	//calculate the count rate
	count=(uint8_t)((FCLK/256)/(1.0/(.001*TIMER_MS)));
	TCCR0B=0x04;//  PRESCALE==256
	TCCR0A=TCCR0A |= 0x80; //enable Clear Timer on Compare
	OCR0A=count; //set count number 
	TIMSK0=0x02; //enable ISR on compare match. 
	TCNT0 =0x00;

}

ISR (TIMER0_COMPA_vect)
{
	TCNT0 = 0x00;
	MilliSeconds+=TIMER_MS;
	if (MilliSeconds>=1000)
	{
		Seconds++;
		cnt_timer++;
		MilliSeconds=0;
	}
}

void GetTime(TIME *ptr)
{
	//disable the Timer ISR 	
	TIMSK0=TIMSK0 & ~(1<<OCIE0A); //Timer0 compare A interrupt OFF

	//set the time
	ptr->milliseconds_time=MilliSeconds;
	ptr->seconds_time=Seconds;

	//enable ISR
	TIMSK0=TIMSK0 | (1<<OCIE0A); //Timer0 compare A interrupt ON

}

uint32_t Get_elapsed_ms(TIME *pStart)
{
	uint32_t temp;
	TIME t;
	GetTime(&t);

	temp=(t.seconds_time-pStart->seconds_time)*1000;
  	temp=temp + t.milliseconds_time;
  	temp=temp-pStart->milliseconds_time;
	return temp;
}

uint32_t Get_elapsed_s(TIME *pStart)
{
	uint32_t temp;
	TIME t;
	GetTime(&t);
	temp=(t.seconds_time-pStart->seconds_time);
 	return temp;
}

uint32_t getSeconds(void)
{
	uint32_t temp;
	TIMSK0=TIMSK0 & ~(1<<OCIE0A); //Timer0 compare A interrupt OFF
	//set the time variables
	temp=Seconds;
	//turn ISR back on
	TIMSK0=TIMSK0 | (1<<OCIE0A); //Timer0 compare A interrupt ON
	return temp;
}

void Reset_timer(void)
{

	TIMSK0=TIMSK0 & ~(1<<OCIE0A); //Timer0 compare A interrupt OFF
	//set the time variables
	Seconds = 0;
	//turn ISR back on
	TIMSK0=TIMSK0 | (1<<OCIE0A); //Timer0 compare A interrupt ON

}




