/*

Ergware

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

#ifndef __USART_H
#define __USART_H
/********************************************************************************
                                Macros and Defines
********************************************************************************/


//#define F_CPU 16000000
#define BAUD 19200
#define MYUBRR F_CPU/16/BAUD-1



/********************************************************************************
                                Function Prototypes
********************************************************************************/
void usart_init(uint16_t ubrr);
char usart_getchar(void);
uint8_t usart_kbhit(void);
int usart_putchar_printf(char var, FILE *stream);

#endif