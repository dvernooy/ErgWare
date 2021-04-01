/*
Copyright (C) Trampas Stern  name of author

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

/*******************************************************************
 *
 *  DESCRIPTION:
 *
 *  AUTHOR : Trampas Stern 
 *
 *  DATE : 5/12/2005  9:48:19 AM
 *
 *  FILENAME: time.c
 *	
 *         Copyright 2004 (c) by Trampas Stern
 *******************************************************************/
#include "time.h"
#include "lcd.h"

UINT32 volatile Seconds=0;
UINT16 volatile MilliSeconds=0;
UWORD volatile cnt_timer=0;



/*******************************************************************
 *  FUNCTION: TimeInit
 *  AUTHOR 		= 	TRAMPAS STERN
 *  FILE 		=	time.c	
 *  DATE		=   5/12/2005  9:48:35 AM
 *
 *  PARAMETERS: 	
 *
 *  DESCRIPTION: Sets up the ISR for the timer 
 *
 *  RETURNS: 
 *
 *               Copyright 2004 (c) by Trampas Stern
 *******************************************************************/
UINT8 TimeInit(void)
{
	UINT8 count;

	//we need to calculate the count value
	count=(UINT8)((FCLK_IO/TIMER_PRESCALE)/(1.0/(.001*TIMER_MILLISEC)));

#if  TIMER_PRESCALE==1
	TCCR0B=0x01;
#endif 
#if  TIMER_PRESCALE==8
	TCCR0B=0x02;
#endif 
#if  TIMER_PRESCALE==64
	TCCR0B=0x03;
#endif 
#if  TIMER_PRESCALE==256
	TCCR0B=0x04;
#endif 
#if  TIMER_PRESCALE==1024
	TCCR0B=0x05;
#endif
	TCCR0A=TCCR0A |= 0x80; //enable Clear Timer on Compare
	OCR0A=count; //set count 
	TIMSK0=0x02; //enable ISR on compare match. 
	TCNT0 =0x00;

    return NO_ERROR;
}
/*******************************************************************
 *  FUNCTION: TimerISR
 *  AUTHOR 		= 	TRAMPAS STERN
 *  FILE 		=	time.c	
 *  DATE		=   5/12/2005  10:14:45 AM
 *
 *  PARAMETERS: 	
 *
 *  DESCRIPTION:  This ISR is executed when timer0 overflows
 *
 *  RETURNS: 
 *
 *               Copyright 2004 (c) by Trampas Stern
 *******************************************************************/
ISR (TIMER0_COMPA_vect)
{
	TCNT0 = 0x00;
	MilliSeconds+=TIMER_MILLISEC;
	if (MilliSeconds>=1000)
	{
		Seconds++;
		cnt_timer++;
		MilliSeconds=0;
	}
}

/*******************************************************************
 *  FUNCTION: GetTime
 *  AUTHOR 		= 	TRAMPAS STERN
 *  FILE 		=	time.c	
 *  DATE		=   5/12/2005  10:16:50 AM
 *
 *  PARAMETERS: 	
 *
 *  DESCRIPTION: Gets the current time
 *
 *  RETURNS: 
 *
 *               Copyright 2004 (c) by Trampas Stern
 *******************************************************************/
UINT8 GetTime(TIME *ptr)
{
	//first we need to disable the Timer ISR 	
	TIMSK0=TIMSK0 & ~(1<<OCIE0A); //Turn Timer0 compare A interrupt off

	//set the time variables
	ptr->milliSec=MilliSeconds;
	ptr->seconds=Seconds;

	//turn ISR back on
	TIMSK0=TIMSK0 | (1<<OCIE0A); //Turn Timer0 compare A interrupt ON

	return NO_ERROR;
}

/*******************************************************************
 *  FUNCTION: GetElaspMs
 *  AUTHOR 		= 	TRAMPAS STERN
 *  FILE 		=	time.c	
 *  DATE		=   5/12/2005  10:19:48 AM
 *
 *  PARAMETERS: pointer to Time of start 	
 *
 *  DESCRIPTION: Returns number of milliseconds elasped
 *
 *  RETURNS: 
 *
 *               Copyright 2004 (c) by Trampas Stern
 *******************************************************************/
UINT32 GetElaspMs(TIME *pStart)
{
	UINT32 temp;
	TIME t;
	GetTime(&t);

	temp=(t.seconds-pStart->seconds)*1000;
  	temp=temp + t.milliSec;
  	temp=temp-pStart->milliSec;
	return temp;
}

/*******************************************************************
 *  FUNCTION: GetElaspSec
 *  AUTHOR 		= 	TRAMPAS STERN
 *  FILE 		=	time.c	
 *  DATE		=   5/12/2005  10:29:25 AM
 *
 *  PARAMETERS:   pointer to Time of start	
 *
 *  DESCRIPTION: returns elasped seconds
 *
 *  RETURNS: 
 *
 *               Copyright 2004 (c) by Trampas Stern
 *******************************************************************/
UINT32 GetElaspSec(TIME *pStart)
{
	UINT32 temp;
	TIME t;
	GetTime(&t);

	temp=(t.seconds-pStart->seconds);
 	return temp;
}

UINT32 getSeconds(void)
{
	UINT32 temp;

	TIMSK0=TIMSK0 & ~(1<<OCIE0A); //Turn Timer0 compare A interrupt off

	//set the time variables
	temp=Seconds;

	//turn ISR back on
	TIMSK0=TIMSK0 | (1<<OCIE0A); //Turn Timer0 compare A interrupt ON

	return temp;
}

void Reset_timer(void)
{

	TIMSK0=TIMSK0 & ~(1<<OCIE0A); //Turn Timer0 compare A interrupt off

	//set the time variables
	Seconds = 0;

	//turn ISR back on
	TIMSK0=TIMSK0 | (1<<OCIE0A); //Turn Timer0 compare A interrupt ON

}



UINT16 connect_timer(UBYTE reset)
{
	UINT16 temp;
	TIMSK0=TIMSK0 & ~(1<<OCIE0A); //Turn Timer0 compare A interrupt off
	if (reset)
	{
		cnt_timer=0;	
	}
	temp=cnt_timer;
	TIMSK0=TIMSK0 | (1<<OCIE0A); //Turn Timer0 compare A interrupt ON
	return temp;
}

