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

#ifndef __SYSTEM_H
#define __SYSTEM_H


#include <stdarg.h>
#include  <string.h>
#include <stdio.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#define MAX_STRING 255



#define SYSCLK 16000000UL//4Mhz
#define FCLK_IO SYSCLK 	//Fclk_I/O which is use for UART and timers

#define BIT_SET(x,n) (x=x | (0x01<<n))
#define BIT_TEST(x,n) ((x & (0x01<<n))!=0)
#define BIT_CLEAR(x,n) (x=x & ~(0x01<<n))

#define MAKE8(x,n) ((UINT8)((x>>(8*n)) & 0xFF))
#define MAKEWORD(h,l) ( (((UINT16)h)<<8) | l)


#define log_printf printf


#define ABS(x) (x?x:-x)
#define DIV(x,y) (x/y)
#define HALT() 

#define DISABLE_INTERRUPTS() SREG=SREG & ~0x80
#define ENABLE_INTERRUPTS() SREG=SREG | 0x80

#define ClrWdt() 	//Restart watch dog timer

//Error codes for Functions
#define NO_ERROR 0

#endif //__SYSTEM_H
