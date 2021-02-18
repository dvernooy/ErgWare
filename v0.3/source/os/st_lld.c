/*
    ChibiOS/HAL - Copyright (C) 2006-2014 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

/**
 * @file    AVR/st_lld.c
 * @brief   ST Driver subsystem low level driver code.
 *
 * @addtogroup ST
 * @{
 */

#include <avr/io.h>
#include "nil.h"

#if (NIL_ST_MODE != NIL_ST_MODE_NONE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/**
 * @brief  Timer maximum value
 */
#define AVR_TIMER_COUNTER_MAX 255


/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local types.                                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

#if (NIL_ST_MODE == NIL_ST_MODE_PERIODIC) || defined(__DOXYGEN__)

/* Work out what the timer interrupt is called on this MCU */
#define AVR_TIMER_VECT TIMER0_COMPA_vect

/* Find the most suitable prescaler setting for the desired OSAL_ST_FREQUENCY */
#if ((F_CPU / NIL_CFG_ST_FREQUENCY) <= AVR_TIMER_COUNTER_MAX)
  #define AVR_TIMER_PRESCALER 1
  #define AVR_TIMER_PRESCALER_BITS (0<<CS02)|(0<<CS01)|(1<<CS00); /* CLK      */
#elif ((F_CPU / NIL_CFG_ST_FREQUENCY / 8) <= AVR_TIMER_COUNTER_MAX)
  #define AVR_TIMER_PRESCALER 8
  #define AVR_TIMER_PRESCALER_BITS (0<<CS02)|(1<<CS01)|(0<<CS00); /* CLK/8    */
#elif ((F_CPU / NIL_CFG_ST_FREQUENCY / 64) <= AVR_TIMER_COUNTER_MAX)
  #define AVR_TIMER_PRESCALER 64
  #define AVR_TIMER_PRESCALER_BITS (0<<CS02)|(1<<CS01)|(1<<CS00); /* CLK/64   */
#elif ((F_CPU / NIL_CFG_ST_FREQUENCY / 256) <= AVR_TIMER_COUNTER_MAX)
  #define AVR_TIMER_PRESCALER 256
  #define AVR_TIMER_PRESCALER_BITS (1<<CS02)|(0<<CS01)|(0<<CS00); /* CLK/256  */
#elif ((F_CPU / NIL_CFG_ST_FREQUENCY / 1024) <= AVR_TIMER_COUNTER_MAX)
  #define AVR_TIMER_PRESCALER 1024
  #define AVR_TIMER_PRESCALER_BITS (1<<CS02)|(0<<CS01)|(1<<CS00); /* CLK/1024 */
#else
  #error "Frequency too low for timer, please set OSAL_ST_FREQUENCY to a higher value"
#endif

#define AVR_TIMER_COUNTER (F_CPU / NIL_CFG_ST_FREQUENCY / AVR_TIMER_PRESCALER)

/* Test if OSAL_ST_FREQUENCY can be matched exactly using this timer */
#define F_CPU_ (AVR_TIMER_COUNTER * AVR_TIMER_PRESCALER * NIL_CFG_ST_FREQUENCY)
#if (F_CPU_ != F_CPU)
  #warning "NIL_CFG_ST_FREQUENCY cannot be generated exactly using timer"
#endif
#undef F_CPU_

#endif /* NIL_ST_MODE == NIL_ST_MODE_PERIODIC */

#if (NIL_ST_MODE == NIL_ST_MODE_FREERUNNING) || defined(__DOXYGEN__)

/* FIXME: Prescaler is now fixed in 256.
 *        Should add support for calculating best value according to
 *        user requested configuration.
 */
//#define PRESCALER (_BV(CS12) | _BV(CS10))
#define PRESCALER _BV(CS12)

#endif /* NIL_ST_MODE == NIL_ST_MODE_FREERUNNING */

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/
#if (NIL_ST_MODE == NIL_ST_MODE_PERIODIC) || defined(__DOXYGEN__)

/**
 * @brief Timer handler for periodic mode.
 */
CH_IRQ_HANDLER(AVR_TIMER_VECT) {

  CH_IRQ_PROLOGUE();

  chSysLockFromISR();
  chSysTimerHandlerI();
  chSysUnlockFromISR();

  CH_IRQ_EPILOGUE();
}

#endif /* NIL_ST_MODE == NIL_ST_MODE_PERIODIC */

#if (NIL_ST_MODE == NIL_ST_MODE_FREERUNNING) || defined(__DOXYGEN__)

/**
 * @brief Timer handler for free running mode.
 */
CH_IRQ_HANDLER(TIMER1_COMPA_vect) {

  CH_IRQ_PROLOGUE();

  // TODO: reset status if required

  chSysLockFromISR();
  chSysTimerHandlerI();
  chSysUnlockFromISR();

  CH_IRQ_EPILOGUE();
}

#endif /* NIL_ST_MODE == NIL_ST_MODE_FREERUNNING */

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level ST driver initialization.
 *
 * @notapi
 */
void st_lld_init(void) {

#if (NIL_ST_MODE == NIL_ST_MODE_FREERUNNING) || defined(__DOXYGEN__)

  /*
   * Free-running mode uses Timer 1 (16 bit).
   */

  /* CTC mode, no clock source */
  TCCR1A  = 0;
  TCCR1B  = _BV(WGM12);

  /* start disabled */
  TCCR1C  = 0;
  OCR1A   = 0;
  TCNT1   = 0;
  TIFR1   = _BV(OCF1A);                                  /* Reset pending.   */
  TIMSK1  = 0;
  TCCR1B  = PRESCALER;

#endif /* NIL_ST_MODE == NIL_ST_MODE_FREERUNNING */

#if (NIL_ST_MODE == NIL_ST_MODE_PERIODIC) || defined(__DOXYGEN__)

  /*
   * Periodic mode uses Timer 0 (8 bit).
   */
  TCCR0A  = (1 << WGM01) | (0 << WGM00) |                /* CTC mode.        */
            (0 << COM0A1) | (0 << COM0A0) |              /* OC0A disabled.   */
            (0 << COM0B1) | (0 << COM0B0);               /* OC0B disabled.   */
  TCCR0B  = (0 << WGM02) | AVR_TIMER_PRESCALER_BITS;     /* CTC mode.        */
  OCR0A   = AVR_TIMER_COUNTER - 1;
  TCNT0   = 0;                                           /* Reset counter.   */
  TIFR0   = (1 << OCF0A);                                /* Reset pending.   */
  TIMSK0  = (1 << OCIE0A);                               /* IRQ on compare.  */
#endif /* NIL_ST_MODE == NIL_ST_MODE_PERIODIC */

}

#endif /* NIL_ST_MODE != NIL_ST_MODE_NONE */

/** @} */
