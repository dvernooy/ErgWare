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

#ifndef __TIME_H
#define __TIME_H

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>   
#include "nil.h"
#include "nilconf.h"

//time
typedef struct {
	uint32_t seconds_time;		//seconds since power on
	uint16_t milliseconds_time;   //milliseconds counter
} TIME; 

extern THD_WORKING_AREA(waTimer, 15);
extern double minute_counter;
THD_FUNCTION(timer, arg);

// ******* Prototypes *********
void TimeInit(void); //initlizes time
void GetTime(TIME *ptr); //gets time
uint32_t Get_elapsed_ms(TIME *pStart); //elapsed ms
uint32_t Get_elapsed_s(TIME *pStart); //elapsed s
uint32_t getSeconds(void); //# seconds powered

#endif //__TIME_H

