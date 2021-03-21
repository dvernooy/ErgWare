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

#define ST2MS(n) ((n) * 1000UL/NIL_CFG_ST_FREQUENCY)

static uint32_t volatile Seconds=0;
static uint16_t volatile MilliSeconds=0;
static uint32_t volatile cnt_timer=0;
static systime_t volatile System_time, System_delta;

void TimeInit(void)
{
	cnt_timer = 0;
	System_time = chVTGetSystemTimeX();	 
}


/*
 * Support thread.
 */
THD_WORKING_AREA(waTimer, 20);
THD_FUNCTION(timer, arg) {
  (void) arg;
  /* Initializing global resources.*/
  TimeInit();

  /* Waiting for button push and activation of the test suite.*/
  while (true) {
  chSysLock(); 
  System_delta = chVTTimeElapsedSinceX(System_time);	
  System_time = chVTGetSystemTimeX();	 
  chSysUnlock(); 

  MilliSeconds+=ST2MS(System_delta);
	if (MilliSeconds>=999)
	{
		Seconds++;
		cnt_timer++;
		MilliSeconds=0;
		minute_counter = (double) Seconds/60.0;
	}
	chThdSleepMilliseconds(20);
  }
}

void GetTime(TIME *ptr)
{
	//disable the Timer ISR 	
	//TIMSK0=TIMSK0 & ~(1<<OCIE0A); //Timer0 compare A interrupt OFF
	chSysLock(); 
	//set the time
	ptr->milliseconds_time=MilliSeconds;
	ptr->seconds_time=Seconds;
	
	//enable ISR
	//TIMSK0=TIMSK0 | (1<<OCIE0A); //Timer0 compare A interrupt ON
	chSysUnlock(); 

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
	//TIMSK0=TIMSK0 & ~(1<<OCIE0A); //Timer0 compare A interrupt OFF
	chSysLock(); 
	//set the time variables
	temp=Seconds;
	//turn ISR back on
	//TIMSK0=TIMSK0 | (1<<OCIE0A); //Timer0 compare A interrupt ON
	chSysUnlock();
	return temp;
}

void Reset_timer(void)
{

	//TIMSK0=TIMSK0 & ~(1<<OCIE0A); //Timer0 compare A interrupt OFF
	chSysLock(); 
	//set the time variables
	Seconds = 0;
	//turn ISR back on
	//TIMSK0=TIMSK0 | (1<<OCIE0A); //Timer0 compare A interrupt ON
	chSysUnlock(); 

}




