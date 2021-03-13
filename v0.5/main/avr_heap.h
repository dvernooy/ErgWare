/* Arduino NilRTOS Library
 * Copyright (C) 2013 by William Greiman
 * Modified by Dave Vernooy
 *
 * This file is part of the Arduino NilRTOS Library
 *
 * This Library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the Arduino NilRTOS Library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */
 
#ifndef avr_heap_h
#define avr_heap_h
#include <stdlib.h>
#include "nil.h"
/* End of heap. */

extern FILE lcd_out;
extern FILE usart_out;

extern char* __brkval;

/*

#if defined(CORE_TEENSY) || ARDUINO == 104 || ARDUINO == 152
*/ 
/*
extern char __bss_end;

// @return Address of first byte after the heap.
static inline char* heapEnd(void) {
  return __brkval ? __brkval : &__bss_end;
}
// @return Size of heap and idle thread stack in bytes.
static inline size_t nilHeapIdleSize(void) {
  return (char*)RAMEND - &__bss_end + 1;
}
*/


void chPrintStackSizes(void);
void chPrintUnusedStack(void);
void chPrintStackLCD(uint8_t page);
void nilFillStacks(void);
void nilThdDelay(systime_t time);
void nilThdDelayUntil(systime_t time);
size_t nilUnusedStack(uint8_t nt);
size_t nilUnusedHeapIdle(void);
/*
bool nilSysBeginNoFill(void);
bool nilSysBegin(void);
*/

// @return Address of first byte after the heap. 
static inline char* heapEnd(void) {
 return __brkval ? __brkval : __malloc_heap_start;
}
// @return Size of heap and idle thread stack in bytes. 
static inline size_t nilHeapIdleSize(void) {
  return (char*)RAMEND - __malloc_heap_start + 1;
}

#endif // avr_heap_h
