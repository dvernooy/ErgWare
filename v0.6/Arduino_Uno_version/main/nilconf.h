/*
    ChibiOS/NIL - Copyright (C) 2013,2014 Giovanni Di Sirio

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
 * @file    nilconf.h
 * @brief   Configuration file template.
 * @details A copy of this file must be placed in each project directory, it
 *          contains the application specific kernel settings.
 *
 * @addtogroup config
 * @details Kernel related settings and hooks.
 * @{
 */

#ifndef _NILCONF_H_
#define _NILCONF_H_

/*===========================================================================*/
/**
 * @name Kernel parameters and options
 * @{
 */
/*===========================================================================*/

/**
 * @brief   Number of user threads in the application.
 * @note    This number is not inclusive of the idle thread which is
 *          Implicitly handled.
 */
#define NIL_CFG_NUM_THREADS                 10

/** @} */

/*===========================================================================*/
/**
 * @name System timer settings
 * @{
 */
/*===========================================================================*/

/**
 * @name    Systick modes.
 * @{
 */
#define NIL_ST_MODE_NONE                   0
#define NIL_ST_MODE_PERIODIC               1
#define NIL_ST_MODE_FREERUNNING            2
/** @} */


/**
 * @brief   System time counter resolution.
 * @note    Allowed values are 16 or 32 bits.
 */
#define NIL_CFG_ST_RESOLUTION               32

/**
 * @brief   System tick frequency.
 * @note    This value together with the @p NIL_CFG_ST_RESOLUTION
 *          option defines the maximum amount of time allowed for
 *          timeouts.
 * @note    Currently the Timer is configured with a prescaler of
 *          256. Arduinos running at 16MHz will have a frequency
 *          of 8MHz/256 ~ 31250 Hz. This will provide a resolution of ~32uS.
 */
 /* periodic tick mode better because it uses the 8 bit timer/counter 0 */
 /* free running tickless uses 16 bit timer/counter 1 */
 
//#define NIL_CFG_ST_FREQUENCY                 31250 /* working well for free-running tickless mode */
#define NIL_CFG_ST_FREQUENCY                4000 /* working well for Periodic Tick mode */


/**
 * @brief   Time delta constant for the tick-less mode.
 * @note    If this value is zero then the system uses the classic
 *          periodic tick. This value represents the minimum number
 *          of ticks that is safe to specify in a timeout directive.
 *          The value one is not valid, timeouts are rounded up to
 *          this value.
 */
 
//#define NIL_CFG_ST_TIMEDELTA              2 //use for free running tickless mode
#define NIL_CFG_ST_TIMEDELTA                0 //use for periodic tick mode


/**
 * @brief   Systick mode required by the underlying OS.
 */
#if (NIL_CFG_ST_TIMEDELTA == 0) || defined(__DOXYGEN__)
#define NIL_ST_MODE                        NIL_ST_MODE_PERIODIC
#else
#define NIL_ST_MODE                        NIL_ST_MODE_FREERUNNING
#endif

/** @} */

/*===========================================================================*/
/**
 * @name Subsystem options
 * @{
 */
/*===========================================================================*/

/**
 * @brief   Events Flags APIs.
 * @details If enabled then the event flags APIs are included in the kernel.
 *
 * @note    The default is @p TRUE.
 */
#define NIL_CFG_USE_EVENTS                  TRUE

/** @} */

/*===========================================================================*/
/**
 * @name Debug options
 * @{
 */
/*===========================================================================*/

/**
 * @brief   System assertions.
 */
#define NIL_CFG_ENABLE_ASSERTS              FALSE

/**
 * @brief   Stack check.
 */
#define NIL_CFG_ENABLE_STACK_CHECK          FALSE

/** @} */

/*===========================================================================*/
/**
 * @name Kernel hooks
 * @{
 */
/*===========================================================================*/

/**
 * @brief   System initialization hook.
 */
#if !defined(NIL_CFG_SYSTEM_INIT_HOOK) || defined(__DOXYGEN__)
#define NIL_CFG_SYSTEM_INIT_HOOK() {                                        \
}
#endif

/**
 * @brief   Threads descriptor structure extension.
 * @details User fields added to the end of the @p thread_t structure.
 */
#define NIL_CFG_THREAD_EXT_FIELDS                                           \
  /* Add threads custom fields here.*/

/**
 * @brief   Threads initialization hook.
 */
#define NIL_CFG_THREAD_EXT_INIT_HOOK(tr) {                                  \
  /* Add custom threads initialization code here.*/                         \
}

/**
 * @brief   Idle thread enter hook.
 * @note    This hook is invoked within a critical zone, no OS functions
 *          should be invoked from here.
 * @note    This macro can be used to activate a power saving mode.
 */
#define NIL_CFG_IDLE_ENTER_HOOK() {                                         \
}

/**
 * @brief   Idle thread leave hook.
 * @note    This hook is invoked within a critical zone, no OS functions
 *          should be invoked from here.
 * @note    This macro can be used to deactivate a power saving mode.
 */
#define NIL_CFG_IDLE_LEAVE_HOOK() {                                         \
}

/**
 * @brief   System halt hook.
 */
#if !defined(NIL_CFG_SYSTEM_HALT_HOOK) || defined(__DOXYGEN__)
#define NIL_CFG_SYSTEM_HALT_HOOK(reason) {                                  \
}
#endif

/** @} */

/*===========================================================================*/
/* Port-specific settings (override port settings defaulted in nilcore.h).   */
/*===========================================================================*/

#endif  /* _NILCONF_H_ */

/** @} */
