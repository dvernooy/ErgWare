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
 *  DATE : 5/12/2005  9:20:56 AM
 *
 *  FILENAME: time.h
 *	
 *         Copyright 2004 (c) by Trampas Stern
 *******************************************************************/
#ifndef __TIME_H
#define __TIME_H

#include "system.h"
#include "datatypes.h"   

#define TIMER_MILLISEC 2	//this is resoultion of time function
#define TIMER_PRESCALE 256	//prescaller to use (see data sheet)
#define FCLK_IO 16000000	//prescaller to use (see data sheet)


//this structure holds our time
typedef struct {
	UINT32 seconds;		//seconds since power on
	UINT16 milliSec;   //milliseconds (0-999)
} TIME; 


// ******* Prototypes *********
UINT8 TimeInit(void); //initlizes time
UINT8 GetTime(TIME *ptr); //gets current "time"
UINT32 GetElaspMs(TIME *pStart); //calculates and returns elsaped time
UINT32 GetElaspSec(TIME *pStart); //calculates and returns elsaped time
UINT32 getSeconds(void); //returns number of seconds unit has been powered on 
UINT16 connect_timer(UBYTE reset); //timer to measure how long since OBDII connected
void  Reset_timer(void); //timer to measure how long since OBDII connected

#endif //__TIME_H

