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

/*
    Set of routines for measuring stack usage in NilRTOS
*/	


#include <avr/pgmspace.h>
#include <stdio.h>
#include <avr/io.h>
#include "avr_heap.h"
#include "nil.h"
#include "usart.h"
#include "lcd.h"

#ifdef SERIAL_USED
static FILE usart_out = FDEV_SETUP_STREAM(usart_putchar_printf, NULL, _FDEV_SETUP_WRITE);
#endif
static FILE lcd_out = FDEV_SETUP_STREAM(lcd_chr_printf, NULL, _FDEV_SETUP_WRITE);

#ifndef __DOXYGEN__
static __attribute__((noinline))
#endif  // __DOXYGEN__
size_t fillSize(uint8_t* bgn, uint8_t* end) {
  uint8_t* p = bgn;
  while (p < end && *p == 0X55) p++;
  return p - bgn;
}


//------------------------------------------------------------------------------
/** Print size of all all stacks.
 * @param[in] pr Print stream for output.
 */

/* 1. thread_config_t is a structure holding thread configuration */
/*    wbase, wend, namep, funcp, arg */
/* 2. nil_thd_configs is an array of type thread_config_t holding all */
/*    elements of the thread table defined in nil.h */


void chPrintStackSizes(void) {
  const thread_config_t *tcp = nil_thd_configs;
  #ifdef SERIAL_USED
  fprintf_P(&usart_out,PSTR("Stack Sizes: "));
  #endif
  while (tcp->wend) {
    #ifdef SERIAL_USED
	fprintf_P(&usart_out,PSTR("%d "), tcp->wend - tcp->wbase);
	#endif
    tcp++;
  }
  #ifdef SERIAL_USED
  fprintf_P(&usart_out,PSTR("%d\r\n"), nilHeapIdleSize());
  #endif
}



//------------------------------------------------------------------------------
/** Print unused byte count for all stacks.
 * @param[in] pr Print stream for output.
 */
void chPrintUnusedStack(void) {
  const thread_config_t *tcp = nil_thd_configs;
  #ifdef SERIAL_USED
  fprintf_P(&usart_out,PSTR("Unused Stack: "));	
  #endif
  while (tcp->wend) {
    #ifdef SERIAL_USED
	fprintf_P(&usart_out,PSTR("%d "), fillSize((uint8_t*)tcp->wbase, (uint8_t*)tcp->wend));
	#endif
    tcp++;
  }
  #ifdef SERIAL_USED
  fprintf_P(&usart_out,PSTR("%d\r\n"), fillSize((uint8_t*)heapEnd(), (uint8_t*)RAMEND));
  #endif
}
 /** @} */

void chPrintStackLCD(uint8_t page) {
  const thread_config_t *tcp = nil_thd_configs;
  uint8_t j = 1;
  while (tcp->wend) {
	if ((page == 1) && (j<7)) {
		lcd_goto_xy(1,j);
		fprintf_P(&lcd_out,PSTR("Thd%d: %d/%d "),j, fillSize((uint8_t*)tcp->wbase, (uint8_t*)tcp->wend), tcp->wend - tcp->wbase);
    }
	else {
		if ((page == 2) && (j>6)) {
		lcd_goto_xy(1,j-6);
		fprintf_P(&lcd_out,PSTR("Thd%d: %d/%d "), j, fillSize((uint8_t*)tcp->wbase, (uint8_t*)tcp->wend), tcp->wend - tcp->wbase);
		}
	}	
	tcp++;
	j++;
  }

	if ((page == 1) && (j<7)) {
		lcd_goto_xy(1,j);
		fprintf_P(&lcd_out,PSTR("Heap: %d/%d "), fillSize((uint8_t*)heapEnd(), (uint8_t*)RAMEND), nilHeapIdleSize());

	}
	else {
		if ((page == 2) && (j>6)) {
		lcd_goto_xy(1,j-6);
		fprintf_P(&lcd_out,PSTR("Heap: %d/%d "), fillSize((uint8_t*)heapEnd(), (uint8_t*)RAMEND), nilHeapIdleSize());
		}
	}	
}



//------------------------------------------------------------------------------
static __attribute__((noinline)) void fill8(uint8_t* bgn, uint8_t* end) {
  while (bgn < end) *bgn++ = 0X55;
}

//------------------------------------------------------------------------------
/**
 * @brief   Returns the semaphore counter current value.
 * @param[in] sp        pointer to a @p Semaphore structure.
 * @return the value of the semaphore counter.
 * @api
 */
cnt_t nilSemGetCounter(semaphore_t *sp) {
  chSysLock();
  cnt_t cnt = sp->cnt;
  chSysUnlock();
  return cnt;
}

//------------------------------------------------------------------------------
/**
 * Determine unused stack for a thread.
 *
 * @param[in] nt Task index.
 *
 * @return Number of unused stack bytes.
 */
size_t nilUnusedStack(uint8_t nt) {
  const thread_config_t *tcp = &nil_thd_configs[nt];
  return fillSize((uint8_t*)tcp->wbase, (uint8_t*)tcp->wend);
}

//------------------------------------------------------------------------------
/**
 * Determine unused bytes in the heap and idle thread stack area.
 *
 * @return Number of unused bytes.
 */
size_t nilUnusedHeapIdle(void) {
  return fillSize((uint8_t*)heapEnd(), (uint8_t*)RAMEND);
}

//------------------------------------------------------------------------------
/** Fill stacks with 0X55.*/
void nilFillStacks(void) {
  const thread_config_t *tcp = nil_thd_configs;
  
  while (tcp->wend) {
    fill8((uint8_t*)tcp->wbase, (uint8_t*)tcp->wend);
    tcp++;
  }
  
  
  // fill heap/idle stack
  fill8((uint8_t*)heapEnd(), (uint8_t*)&tcp - 16);

}

//------------------------------------------------------------------------------
/** Start Nil RTOS with raw uninitialized stack memory.
 *  This call saves a little flash compared to nilSysBegin().
 *
 * @return TRUE for success else FALSE.
 */

/*
bool nilSysBeginNoFill(void) {
  if (!nil_thd_count) return FALSE;
  chSysLock();
  boardInit();
  chSysInit();
  return TRUE;
}
*/

//------------------------------------------------------------------------------
/** Start Nil RTOS with all stack memory initialized to a known value.
 *
 * @return TRUE for success else FALSE.
 */
/*
bool nilSysBegin(void) {
  nilFillStacks();
  return nilSysBeginNoFill();
}
*/

void nilThdDelay(systime_t time) {
  //systime_t t0 = port_timer_get_time();
  systime_t t0 = chVTGetSystemTimeX();
  while ((chVTGetSystemTimeX() - t0) < time) {}
}

void nilThdDelayUntil(systime_t time) {
  nilThdDelay(time - chVTGetSystemTimeX());
}

