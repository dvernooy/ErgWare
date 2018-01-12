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


#define PROD
//#undef PROD


#include "hal.h"
#include "nil.h"
#include <avr/pgmspace.h>
#include <stdio.h>
#include "lcd.h"
#include <string.h>
#include <avr/io.h>
#include "avr_heap.h"
#include "usart.h"
#include "time.h"
#include <math.h>

#define DEBOUNCE_TIME 275        /* time to wait while "de-bouncing" button */
#define DELTA 0.0001
#define SIXTY 60.0
#define ZERO 0.0

/********************************************************************************
Defines - erg
********************************************************************************/
#define LEFT		bit_is_clear(PIND, PD5)
#define	MIDDLE		bit_is_clear(PIND, PD6)
#define	RIGHT		bit_is_clear(PIND, PD7)

/*number of constants for averaging*/
#define MAX_N	5

/********************************************************************************
    Constants - erg
********************************************************************************/
static double J_moment = 0.16; //kg*m^2 - set this to the moment of inertia of your flywheel. 
						//look at documentation for ways to measure it. Its easy to do.
static double d_omega_div_omega2 = 0.0031;//erg_constant - to get this for your erg:
// Uncomment the code below that writes out the constant K_damp_esimator_vector[0]. Then run the erg, 
//run that version of the software, get the number and add it to this line, then re-compile.
//It should be close to value above.  I decided just to hard code it instead of dynamically updating 
//it during the running of the software, though that would be easy and is presumably what other ergs do.
static double K_damp = ZERO; //related by J_moment*omega_dot = K_damp * omega^2 during non-power part of stroke
//I calcualate this below.

static double cal_factor = ZERO; //distance per rev ... calculated later
static double magic_factor = 2.8; //a heuristic constant people use to relate revs to distance-rowed
static double pi = 3.1415926;
static double seconds_per_tick = 0.000008; //8us per tick with 64 divider in timer1 & 8MHz clock ... thread7

/********************************************************************************
Global Variables - NIL
********************************************************************************/
static FILE lcd_out = FDEV_SETUP_STREAM(lcd_chr_printf, NULL, _FDEV_SETUP_WRITE);
static FILE usart_out = FDEV_SETUP_STREAM(usart_putchar_printf, NULL, _FDEV_SETUP_WRITE);

volatile uint8_t lcd_context = 1; //which thread has LCD_context
volatile uint8_t lcd_context_count[6] = {1,1,1,1,1,1}; //start screen within thread
volatile uint8_t lcd_context_count_MAX[6] = {1,1,1,3,3,2}; //max screens within thread
static thread_reference_t trp = NULL; //thread reference object ... thread_reference_t = thread_t*
static thread_reference_t trp_chopper = NULL; //thread reference object ... thread_reference_t = thread_t*
static thread_reference_t trp_menu = NULL; //thread reference object ... thread_reference_t = thread_t*
static thread_t *tp[4] = {NULL, NULL, NULL, NULL}; //array of pointers to threads
double minute_counter;

/* Declare a semaphore */
semaphore_t lcdUSE; /*semaphore to protect the LCD*/

// Global menu data
#define maxMenus 3
#define maxItems 4

/********************************************************************************
Global Variables
********************************************************************************/ 
static uint8_t curMenu = 0; // menu currently shown
static uint8_t curMenu_old = 0; // menu currently shown
static uint8_t curItem = 0; // item currently marked (line 0 for menutitle, so starts on one). NB! Item marked may not correspond to line marked (see cursorCount)
static uint8_t menuCount = 0;  // keeps track of what line of menu to print
volatile uint8_t switchtype = 0;

static char menutitle[maxMenus][5];
static char menuitem[maxMenus][maxItems][5];
static uint8_t menulink[maxMenus][maxItems]; // a link to a submenu OR
static uint8_t menuactn[maxMenus][maxItems]; // menu actions - turn on LED or whatever
static uint8_t menuitemcount[maxMenus]; // holds number of menu items in each menu

/********************************************************************************
Global Variables - erg
********************************************************************************/
static uint8_t elapsed_hours = 0;
static uint8_t elapsed_mins = 0;
static uint8_t elapsed_secs = 0;

static uint8_t split_hours = 0;
static uint8_t split_mins = 0;
static uint8_t split_secs = 0;
static double split_time = 0.0;

static uint8_t chop_ticks = 0;

static double average_SPM = ZERO;
static double volatile distance_rowed = ZERO;

static uint16_t volatile chop_counter[2];
static double stroke_vector[MAX_N];
static double stroke_vector_avg;
static double power_vector[MAX_N];
static double power_vector_avg;
static double calorie_tot = ZERO;
static double K_power = ZERO;
static double J_power = ZERO;
static double power_ratio_vector[MAX_N];
static double K_damp_estimator_vector[MAX_N];
static double K_damp_estimator_vector_avg;

static double K_damp_estimator = ZERO;
static double speed_vector[MAX_N];
static double speed_vector_avg = ZERO;
static double power_ratio_vector_avg = ZERO;

static double stroke_elapsed = ZERO;
static double power_elapsed = ZERO;

static double stroke_distance = ZERO;
static double stroke_distance_old = ZERO;

static double omega_vector[2];
static double omega_dot_vector[2];
static double omega_dot_dot = ZERO;
static double current_dt = ZERO;
static double omega_vector_avg = ZERO;
static double omega_vector_avg_curr = 0.0;

static uint8_t omega_dot_screen = 0;
static uint8_t omega_dot_dot_screen = 0;
static uint8_t power_stroke_screen[2];

static uint16_t stroke = 0;
static uint8_t j = 0;

static TIME t_stroke, t_power;


/*********************************************************************************
time parser
********************************************************************************/ 

static void parse_time(double time_in_min, uint8_t *hours, uint8_t *mins, uint8_t *secs) {
	double temp_min;
	double temp_secs;
	*hours = (uint8_t) (time_in_min/60);
	temp_min = time_in_min - (double) SIXTY * (*hours);
		*mins = (uint8_t) temp_min;
	temp_secs = SIXTY*temp_min -  SIXTY *(double) (*mins);
		*secs = (uint8_t) temp_secs;
}

/*********************************************************************************
weighted_avg_function
********************************************************************************/ 
static double  weighted_avg(double *vector) {
double temp_weight, temp_sum;
uint8_t j;
temp_sum = ZERO;
temp_weight = ZERO;
	for (j =0;j<MAX_N; j++) {
		temp_weight += pow(2,MAX_N-j-1);
		temp_sum += pow(2,MAX_N-j-1) * vector[j];
	}
	temp_sum = temp_sum/temp_weight;
return temp_sum;
}

/*********************************************************************************
reshuffle_function
********************************************************************************/ 
/*
static void reshuffle(double *vector) {
uint8_t j;
	for (j = MAX_N;j>0; j--) {
		vector[j] = vector[j-1];
	}
}
*/
/*********************************************************************************
Menu Setup
********************************************************************************/ 

static void initMenu(void) {
    sprintf_P(menutitle[0], PSTR("Ergo"));
    sprintf_P(menuitem[0][0],  PSTR("Row!"));
    menulink[0][0] = 0; // no sub menu, just action
    menuactn[0][0] = 1; 
    sprintf_P(menuitem[0][1],  PSTR("Prog"));
    menulink[0][1] = 1;
    menuactn[0][1] = 0;
    sprintf_P(menuitem[0][2],  PSTR("Info"));
    menulink[0][2] = 2;
    menuactn[0][2] = 0;

    sprintf_P(menutitle[1],  PSTR("Prog"));
    sprintf_P(menuitem[1][0],  PSTR("D/T"));
    menulink[1][0] = 0; // no sub menu, just action
    menuactn[1][0] = 2;
    sprintf_P(menuitem[1][1],  PSTR("Pace"));
    menulink[1][1] = 0;
    menuactn[1][1] = 3;
	sprintf_P(menuitem[1][2],  PSTR("Burn"));
    menulink[1][2] = 0; 
    menuactn[1][2] = 4;
	sprintf_P(menuitem[1][3],  PSTR("//"));
    menulink[1][3] = 0; // code to trigger return to main menu.  Can't use 0 as that is considered no action here
    menuactn[1][3] = 99;
 
	sprintf_P(menutitle[2],  PSTR("Info"));
    sprintf_P(menuitem[2][0],  PSTR("Data"));
    menulink[2][0] = 0;
    menuactn[2][0] = 5;
	sprintf_P(menuitem[2][1],  PSTR("Heap"));
    menulink[2][1] = 0;
    menuactn[2][1] = 6;
    sprintf_P(menuitem[2][2],  PSTR("//"));
    menulink[2][2] = 0; // code to trigger return to main menu.  Can't use 0 as that is considered no action here
    menuactn[2][2] = 99;

	menuitemcount[0] = 3;
	menuitemcount[1] = 4;
	menuitemcount[2] = 3;
	
    // END of menu data
}

/*********************************************************************************
Menu show routine
********************************************************************************/ 

static void showMenu(void) {
    lcd_clear();
	
	lcd_goto_xy(1,1);
	
	invert = 1;
    fprintf_P(&lcd_out, PSTR("%s"), menutitle[curMenu]);
	invert = 0;

    //print menu content
	while (menuCount < menuitemcount[curMenu]) {
        lcd_goto_xy(1, menuCount+2); // +1 to leave first line for menu title
        if (curItem == menuCount) {
            // item currently indicated by cursor
            fprintf_P(&lcd_out, PSTR(">%s"), menuitem[curMenu][menuCount]);
        } else {
            fprintf_P(&lcd_out, PSTR(" %s"), menuitem[curMenu][menuCount]);
        }
        menuCount++;
    }
}

/*********************************************************************************
Interrupt handler for the CD chopper disk
********************************************************************************/ 

CH_IRQ_HANDLER(INT0_vect)
{           
CH_IRQ_PROLOGUE();
chSysLockFromISR();
	
  chop_ticks++;
 
  if ((chop_ticks % 8 ) == 0) {
	chop_ticks = 0;
	chop_counter[1] = chop_counter[0];
	chop_counter[0] = TCNT1;
	TCNT1 = 0;
	distance_rowed += cal_factor;
	chThdResumeI(&trp_chopper, (msg_t)0x55);  
  }
chSysUnlockFromISR();
CH_IRQ_EPILOGUE();

}


/*********************************************************************************
Interrupt handler for button press, signals Thread1, captures which button was pressed
********************************************************************************/ 

CH_IRQ_HANDLER(PCINT2_vect)
{           
	CH_IRQ_PROLOGUE();
	chSysLockFromISR();
	if (LEFT) {
	/*it is button 1 ... generally scrolling or escaping */
		switchtype = 1;
	}	
	else if (MIDDLE) {
		/*it is button 2 ... selecting*/
		switchtype = 2;	
	}
	else if (RIGHT) {
	/*it is button 1 ... generally scrolling or escaping */
		switchtype = 3;
	}	
	
	/* Wakes up a thread waiting on a thread reference object*/
	/* Resuming the thread with message, synchronous*/
	/* Synchronizes with thread that is suspended with &trp*/
	/* can also pass the message to that thread	*/
	chThdResumeI(&trp, (msg_t)switchtype);  
	chSysUnlockFromISR();
	CH_IRQ_EPILOGUE();
}


/*********************************************************************************
Thread1 - handles all the button context, gets woken up from Interrupt handler
********************************************************************************/ 

THD_FUNCTION(Thread1, arg) {

  (void)arg;
  PCMSK2 |= (1<<PCINT21)|(1<<PCINT22)|(1<<PCINT23);
  PCICR |= (1<<PCIE2);
  msg_t msg;

  /* using a variable called lcd_context allows buttons to take */
  /* on different context depending on which screens			*/
  /* were alive when they were used 								*/
  /* this thread does the interpretation after the IRQ traps the button press */

  while (true) {
	chSysLock();

	/* Waiting for button push & IRQ to happen */
	/* Current thread put to sleep & sets up the reference variable trp for the trigger to reference */
	/* Will resume with a chThdResumeI referencing trp from interrupt*/
	
    msg = chThdSuspendTimeoutS(&trp, TIME_INFINITE);
    chSysUnlock(); 
	chThdSleepMilliseconds(DEBOUNCE_TIME);		
	switch (lcd_context) {
		case 1:	
			/* we were in the main menu & a button was pushed & handled by interrupt*/
			/* the button context was set in interrupt, need to signal thread 2 */
			//next 3 should be atomic	
			chSysLock();
			PCIFR |= (1<<PCIF2);//clear Pin change interrupt flag 0
			
			/*Send a resume signal to menu to continue - right now it is suspended */				
			chThdResumeI(&trp_menu, (msg_t)switchtype);  		
			
			chSysUnlock();

		break;
					
		case 3: //we were in thread3
			//next 3 should be atomic
			chSysLock();
			PCIFR |= (1<<PCIF2);//clear Pin change interrupt flag 0
			lcd_context =1; //going back to thread2
			
			/*returning from thread4 ... any button did it ... so we will go back to the */
			/* main menu thread 2 reset the switchtype to 0, we are neither going up or */
			/* down, nor selecting*/
			switchtype = 0;
			
			/*Send an event signal to thread 4 to go back up to the main entry point*/
			/*Since the button was pressed to get out of that thread */
			/* Note - event masks should be thought of as or'd binary places. So you should use */
			/*Send a resume signal to menu to continue - right now it is suspended */				
			chThdResumeI(&trp_menu, (msg_t)switchtype);  	
			
			/* 1, 2, 4, 8, 16 etc.. as distinct masks when signaling a specific thread*/
			/* the same mask # can be used when signaling other distinct threads, if you are */
			/* directly signaling that thread anyways */
			chEvtSignalI(tp[0], (eventmask_t)2);  
			chSysUnlock();
		break;

		case 4:
			//we were in thread4
			//next 3 should be atomic
			chSysLock();
			PCIFR |= (1<<PCIF2);//clear Pin change interrupt flag 0
			
			if (switchtype == 1) { //returning from thread9 back to this thread .. thread 1
				lcd_context =1; //need to returning to thread2
				
				/*returning from thread5 ... button 1 did it ... so we will go back to the main menu */
			    /* reset the switchtype to 0*/
				switchtype = 0; 
				
				/*Send a resume signal to menu to continue - right now it is suspended */				
				chThdResumeI(&trp_menu, (msg_t)switchtype); 
				
				/*Send an event signal to thread 5 to go back up to the main entry point*/
				/*Since the button was pressed to get out of that thread */
				
				chEvtSignalI(tp[1], (eventmask_t)2); 				
				chSysUnlock();
				break;
             } 
			if (switchtype ==2) {
				/*reset timer*/
				chEvtSignalI(tp[1], (eventmask_t)4); 				
				chSysUnlock();
				break;
			}	
			if (switchtype ==3) {
				/*start timer*/
				chEvtSignalI(tp[1], (eventmask_t)8); 				
				chSysUnlock();
				break;
			}	
		
		case 5:
		//we were in thread5
		//next 3 should be atomic
			chSysLock(); 
			PCIFR |= (1<<PCIF2);//clear Pin change interrupt flag 0
			
			if (switchtype == 1) { //returning from thread5 back to this thread .. thread 1
				lcd_context =1; //need to returning to thread2
				
				/*returning from thread5 ... button 1 did it ... so we will go back to the main menu */
			    /* reset the switchtype to 0*/
				switchtype = 0; 
				
				/*Send a resume signal to menu to continue - right now it is suspended */				
				chThdResumeI(&trp_menu, (msg_t)switchtype); 
				
				/*Send an event signal to thread 5 to go back up to the main entry point*/
				/*Since the button was pressed to get out of that thread */
				
				chEvtSignalI(tp[2], (eventmask_t)2); 				
				chSysUnlock();		
             } 
             else { 
				/*switchtype ==2, we are staying in thread5*/
				/*don't change lcd_context, we are staying in thread5*/
				
				/*button pressed to cycle through subscreens ... update the subscreen count*/
				lcd_context_count[4] = lcd_context_count[4]+1;
				if (lcd_context_count[4] > lcd_context_count_MAX[4]) {lcd_context_count[4] = 1;}
				
				/*staying in thread 5, signal thread5 to re-enter submenu with the new lcd context*/
				/* note, skip eventmask 3 because that is binary 11, which would signal 2 events*/
				chEvtSignalI(tp[2], (eventmask_t)4);  //stay in thread
				chSysUnlock();
			}	
		
		break;
		
		case 6: //we were in thread 6 ... any button returns
			chSysLock(); 
			PCIFR |= (1<<PCIF2);//clear Pin change interrupt flag 0

			if (switchtype == 1) { //returning from thread5 back to this thread .. thread 1
				lcd_context =1; //need to returning to thread2
				
				/*returning from thread5 ... button 1 did it ... so we will go back to the main menu */
			    /* reset the switchtype to 0*/
				switchtype = 0; 
				
				/*Send a resume signal to menu to continue - right now it is suspended */				
				chThdResumeI(&trp_menu, (msg_t)switchtype); 
				
				/*Send an event signal to thread 5 to go back up to the main entry point*/
				/*Since the button was pressed to get out of that thread */
				
				chEvtSignalI(tp[3], (eventmask_t)2); 				
				chSysUnlock();		
             } 
			else {
				
				/*button pressed to cycle through subscreens ... update the subscreen count*/
				lcd_context_count[5] = lcd_context_count[5]+1;
				if (lcd_context_count[5] > lcd_context_count_MAX[5]) {lcd_context_count[5] = 1;}
				
				chEvtSignalI(tp[3], (eventmask_t)4); 				
				chSysUnlock();
			}	
		break;		
	}				
  }/*while (true) */
}//THREAD1


/*********************************************************************************
Thread 2 - Main thread for LCD menu + signalling of other actions, signaled from
           Thread1 for LCD use and for re-update when button pressed
********************************************************************************/ 

THD_FUNCTION(Thread2, arg) {

/*this thread will update the menu on the lcd */
/*it will take a semaphore for use of the lcd */
/*it will also take a semaphore for button pressed*/


  (void)arg;
   msg_t msg;

    initMenu(); // initialize menu by adding menu data to menu globals
	
	/*Wait for the lcdUSe semaphore to be free & take it */
	chSemWait(&lcdUSE);
	lcd_init();
	#ifdef PROD 
	lcd_contrast(0x28);//0x28 for production application
	#else //PROD
	lcd_contrast(0x3E);//0x3E for STK500 board
	#endif //PROD
	lcd_clear();
	
	/*give back lcd semaphore */
	chSemSignal(&lcdUSE);
	chThdSleepMilliseconds(100); 
 
    while(true) { 
	
		/*********************************************************************************
			Update LCD when triggered, then wait
		********************************************************************************/ 
		/*Wait for the lcdUSe semaphore to be free & take it again */
		chSemWait(&lcdUSE);
		
		/*show the menu*/
		showMenu();
		
		/*give back the lcd semaphore */
		chSemSignal(&lcdUSE);
		
		chSysLock();
		/* Waiting for button push & IRQ to happen */
		/* Current thread put to sleep & sets up the reference variable trp for the trigger to reference */
		/* Will resume with a chThdResumeI referencing trp from interrupt*/
	
		msg = chThdSuspendTimeoutS(&trp_menu, TIME_INFINITE);
		chSysUnlock(); 

		
		
        /*********************************************************************************
			No action
		********************************************************************************/ 
		if (switchtype == 0) { //we are returning from a sub-menu action
                if (curItem == menuitemcount[curMenu]) { 
				   curItem = 0;
				}
            menuCount = 0;
        } //switchtype 0	
		
		/*********************************************************************************
			Scrolling
		********************************************************************************/ 
		if (switchtype == 1) { //we are scrolling
			curItem++; // add one to curr item
				if (curItem == menuitemcount[curMenu]) {   
				   curItem = 0; 
				 }
            menuCount = 0;
        } //switchtype 1				
		
		/*********************************************************************************
			Selecting and triggering
		********************************************************************************/ 

		if (switchtype == 2) { //we are selecting 
			// handle user input
			if (menuactn[curMenu][curItem]) {
				// has an action
				switch (menuactn[curMenu][curItem]) {
					case 1:	// action 0.1
						chEvtSignalI(tp[0], (eventmask_t)1);  // signal thread3
						break;
					case 2: 
					    /*signals thread4, which is sitting there waiting */
						/*with chEvtWaitAnyTimeout. Can also send flags */
						lcd_context_count[3] = 1;
						chEvtSignalI(tp[1], (eventmask_t)1);  // signal thread9
						break;   
					case 3: 
					    /*signals thread4, which is sitting there waiting */
						/*with chEvtWaitAnyTimeout. Can also send flags */
						lcd_context_count[3] = 2;
						chEvtSignalI(tp[1], (eventmask_t)1);  // signal thread9
						break;   	
					case 4: 
					    /*signals thread4, which is sitting there waiting */
						/*with chEvtWaitAnyTimeout. Can also send flags */
						lcd_context_count[3] = 3;
						chEvtSignalI(tp[1], (eventmask_t)1);  // signal thread4
						break;             
					case 5: //
						chEvtSignalI(tp[2], (eventmask_t)1);  // signal thread5
						break;
					case 6: //
						chEvtSignalI(tp[3], (eventmask_t)1);  // signal thread5
						break;
					case 99:
						curMenu_old = curMenu;
						curMenu = menulink[curMenu][curItem]; // return to main menu
						/* enable this if want to return to top of menu*/
						curItem=curMenu_old; // reset menu item to which cursor point
						break;
				}//switch	
				menuCount = 0;
			} //menuaction
			else {
				curMenu = menulink[curMenu][curItem];  // set to menu selected by cursor
				curItem = 0; // reset menu item to which cursor point
				menuCount = 0; // reprint from first line of this page
			}//end if menuaction
		} // switchtype = 2
		
	} // end while (true)	
  
}//THREAD2


/*********************************************************************************
Thread 3 - LCD write out all the erg data
********************************************************************************/ 
THD_FUNCTION(Thread3, arg) {
	(void)arg;
	tp[0] = chThdGetSelfX(); //returns a pointer to current thread
	
	while (true) {
		/* it starts in off state, or gets here when turned off. Eventmask1 turns it on*/
		chEvtWaitAnyTimeout((eventmask_t)1, TIME_INFINITE);
		/*take control of LCD semaphore*/
		chSemWait(&lcdUSE);
		/*switch lcd_context to thread 4*/
		lcd_context = 3;
		// Print on first line
		lcd_clear();
		lcd_goto_xy(1,1);
		invert = 1;
		fprintf_P(&lcd_out,PSTR(" Sarah's Ergo "));
		invert = 0;

		while(true) {
		/* it is inside this loop and toggling. If we get an */
		/* Eventmask2, it turns off */
             if(chEvtWaitAnyTimeout((eventmask_t)2, TIME_IMMEDIATE) == 2){
			  /*thread3 gives back the lcd */
			  chSemSignal(&lcdUSE);
			  break;
            }
		
			/***********************************************
			update time
			************************************************/	
			lcd_goto_xy(3,6);
			parse_time((double) minute_counter, &elapsed_hours, &elapsed_mins, &elapsed_secs);
			fprintf_P(&lcd_out,PSTR("%2d:%02d"),elapsed_mins, elapsed_secs);
		
			/***********************************************
			update distance
			************************************************/
			lcd_goto_xy(3,5);
			fprintf_P(&lcd_out,PSTR("%1.0f meters    "), distance_rowed);

			/***********************************************
			update stroke rate
			************************************************/				
			lcd_goto_xy(1,2);
			if (stroke < 5) {
				fprintf_P(&lcd_out,PSTR("SPM --  "));
			}
			else {
				fprintf_P(&lcd_out,PSTR("SPM %2.0f  "), SIXTY/stroke_vector_avg);
			}	
			lcd_goto_xy(9,2);
			if ((stroke < 2) || ((elapsed_hours ==0) && (elapsed_mins ==0) && (elapsed_secs < 30))) {
				fprintf_P(&lcd_out,PSTR("avg --"));
			}
			else {
				average_SPM = (double) stroke/((double)elapsed_hours*SIXTY+(double)elapsed_mins+(double)elapsed_secs/SIXTY);
				fprintf_P(&lcd_out,PSTR("avg %2.0f"), average_SPM);
			}	
	
			/***********************************************
			update stroke count & ratio 
			************************************************/		
			lcd_goto_xy(1,3);
			fprintf_P(&lcd_out,PSTR("S# %d    "), stroke);
	
			lcd_goto_xy(11,3);
			fprintf_P(&lcd_out,PSTR("%1.2f  "), power_ratio_vector_avg);
			
		
			/***********************************************
			update average power
			************************************************/		
			//lcd_goto_xy(1,4);		
			lcd_goto_xy(4,4);		
			if (stroke < 5) {
				fprintf_P(&lcd_out,PSTR("--- W"));
			}
			else {
				fprintf_P(&lcd_out,PSTR("%1.0f W   "), power_vector_avg);
			}
		
			/***********************************************
			update 500m split 
			************************************************/	
			lcd_goto_xy(10,6);
			
			if (stroke < 10) {
			fprintf_P(&lcd_out,PSTR("--:--"));
			}
			else {
				fprintf_P(&lcd_out,PSTR("%2d:%02d"), split_mins, split_secs);
			}
		chThdSleepMilliseconds(350); 	
		}//while(true) inner	
	} //while(true) outer
}//THREAD3

/*********************************************************************************
Thread 4 - self contained action task, escapable, takes LCD semaphore
********************************************************************************/ 
THD_FUNCTION(Thread4, arg) {
	(void)arg;
	TIME internal_timer;
	double internal_timer_elapsed;
	double internal_distance_rowed = ZERO;
	double internal_distance_temp_old = ZERO;
	double internal_distance_temp =ZERO;
	uint8_t internal_timer_started;
	//uint8_t entry_flag;
	tp[1] = chThdGetSelfX();//returns a pointer to current thread
	
	
	while (true) {
        /* wait for signal coming from main menu thread 2*/	
		chEvtWaitAnyTimeout((eventmask_t)1, TIME_INFINITE);
		/*take control of LCD semaphore*/
		chSemWait(&lcdUSE);
		/*switch lcd_context to thread 4*/
		lcd_context = 4; 
		lcd_clear();
		internal_timer_started = 0;
		while(true) {
			/*now inside loop, wait for signal from thread1 button handler to get out*/
		    if(chEvtWaitAnyTimeout((eventmask_t)2, TIME_IMMEDIATE) == 2){
			  /*thread4 gives back the lcd */
			  chSemSignal(&lcdUSE);
			  break;
            }
			
			switch (lcd_context_count[3]) {
				case 1:
					if (internal_timer_started == 0) {
						lcd_goto_xy(3,2);
						bigfont = 1;
						fprintf_P(&lcd_out,PSTR("--:--"));
						lcd_goto_xy(7,6);
						fprintf_P(&lcd_out,PSTR("-"));
						bigfont = 0;
						lcd_go_up_one();
						fprintf_P(&lcd_out,PSTR(" m "));
						internal_distance_rowed = 0;
					}
					/*now inside loop, wait for signal from thread1 button handler start internal timer*/
					if(chEvtWaitAnyTimeout((eventmask_t)4, TIME_IMMEDIATE) == 4){
						/*starts internal timer */
						GetTime (&internal_timer);
						internal_timer_started = 1;
						internal_distance_temp = distance_rowed;
						internal_distance_rowed = 0;
					}
					/*now inside loop, wait for signal from thread1 button handler to get out*/
					if(chEvtWaitAnyTimeout((eventmask_t)8, TIME_IMMEDIATE) == 8){
						/*thread4 gives back the lcd */
						internal_timer_started = 0;
						break;
					}
					if (internal_timer_started == 1) {
							lcd_goto_xy(3,2);
							internal_timer_elapsed = (double)Get_elapsed_s(&internal_timer);
							parse_time((double) internal_timer_elapsed/60.0, &elapsed_hours, &elapsed_mins, &elapsed_secs);
							bigfont = 1;
							fprintf_P(&lcd_out,PSTR("%2d:%02d"),elapsed_mins, elapsed_secs);
							bigfont = 0;
							internal_distance_temp_old = internal_distance_temp;
							internal_distance_temp = distance_rowed;
							internal_distance_rowed += (internal_distance_temp - internal_distance_temp_old);
							lcd_goto_xy(1,6);
							bigfont = 1;
							fprintf_P(&lcd_out,PSTR("%4.0f"), internal_distance_rowed);							
							bigfont = 0;
							lcd_go_up_one();
							fprintf_P(&lcd_out,PSTR(" m "));

						}
					break;
				
				case 2:	
				/* this case is the distance counter */
					lcd_goto_xy(3,2);
					bigfont = 1;	
					if (stroke < 5) {
						fprintf_P(&lcd_out,PSTR("---"));
					}
					else {
						fprintf_P(&lcd_out,PSTR("%3.0f"), SIXTY/stroke_vector_avg);
					}	
					bigfont = 0;
					lcd_go_up_one();
					fprintf_P(&lcd_out,PSTR(" SPM  "));

					lcd_goto_xy(3,4);
					bigfont = 1;
					if (stroke < 5) {
						fprintf_P(&lcd_out,PSTR("---"));
					}
					else {
						fprintf_P(&lcd_out,PSTR("%3.0f"), power_vector_avg);
					}	
					bigfont = 0;
					lcd_go_up_one();
					fprintf_P(&lcd_out,PSTR(" W   "));
					
					lcd_goto_xy(1,6);
					bigfont = 1;
					if (stroke < 10) {
						fprintf_P(&lcd_out,PSTR("--:--"));
					}
					else {
						fprintf_P(&lcd_out,PSTR("%2d:%02d"), split_mins, split_secs);
					}
					bigfont = 0;

				break;
				
				case 3:
				/* this case is the calorie counter */
					bigfont = 1;
					lcd_goto_xy(1,2);
					fprintf_P(&lcd_out,PSTR("%3.0f"),calorie_tot);
					bigfont = 0;
					lcd_go_up_one();
					fprintf_P(&lcd_out, PSTR(" kcals "));
				break;
			}//switch
			
			chThdSleepMilliseconds(250);
    		/* could be doing other stuff here */
			
		} //while(true) inner
	} //while(true) outer
}//THREAD4


/*********************************************************************************
Thread 5 -  - self contained action task with sub-screens, escapable, takes LCD semaphore
********************************************************************************/ 
THD_FUNCTION(Thread5, arg) {
	(void)arg;
	uint8_t entry_flag;
	tp[2] = chThdGetSelfX();//returns a pointer to current thread
	while (true) {
		/* wait for signal coming from main menu thread 2*/	
		chEvtWaitAnyTimeout((eventmask_t)1, TIME_INFINITE);
		/*take control of LCD semaphore*/
		chSemWait(&lcdUSE);
		/*switch lcd_context to thread 5*/
		lcd_context = 5;
		lcd_clear();
		entry_flag = 1;
		while(true) {
		    /*now inside loop, wait for signal from thread1 button handler to get out*/
			if(chEvtWaitAnyTimeout((eventmask_t)2, TIME_IMMEDIATE) == 2){
				/*thread5 gives back the lcd */
				chSemSignal(&lcdUSE);
				break;
			}
			/*now inside loop, respond to signal from thread1 button handler to switch sub-menu*/
            if((entry_flag ==1) ||(chEvtWaitAnyTimeout((eventmask_t)4, TIME_IMMEDIATE) == 4)){           
				entry_flag = 0;
				lcd_clear();
			}	
				
			switch (lcd_context_count[4]) {
				case 1:	// action 5.2a
					lcd_goto_xy(1,1);
					fprintf_P(&lcd_out,PSTR("Dist/Rev"));
					lcd_goto_xy(1,2);	
					fprintf_P(&lcd_out,PSTR("%1.3f m"), cal_factor);
					
					lcd_goto_xy(1,3);
					fprintf_P(&lcd_out,PSTR("Dist/Stroke"));
					lcd_goto_xy(1,4);	
					//uncomment this code if you want to display distance per stroke & time per stroke
					//and comment out the average power display below
					if (stroke < 5) {
						fprintf_P(&lcd_out,PSTR("---- m"));
					}
					else {
						fprintf_P(&lcd_out,PSTR("%1.2f m    "), stroke_distance);
					}						
					lcd_goto_xy(1,5);
					fprintf_P(&lcd_out,PSTR("Time/Stroke"));
					lcd_goto_xy(1,6);
					if (stroke < 5) {
						fprintf_P(&lcd_out,PSTR("---- s"));
					}
					else {
						fprintf_P(&lcd_out,PSTR("%1.2f s    "), stroke_vector_avg);
					}
					break;
				case 2:	// action 5.2b
					lcd_goto_xy(1,1);
					fprintf_P(&lcd_out,PSTR("K_Damp"));
					lcd_goto_xy(1,2);
					fprintf_P(&lcd_out,PSTR("%1.4f"), d_omega_div_omega2);
					//uncomment this code if you want to display the d_omega_div_omega2 constant for your erg
				
					//(and comment out the 2 lines directly above that display
					// power_ratio_wector_avg display above so it doesn't over-write)
				
					//to get K_damp, you multiply this number times J_moment.
					//i.e. K_damp = J_moment*(-1*K_damp_estimator_vector[0]) = J_moment*d_omega_div_omega2
					//Note, even though (d(omega)/dt)/omega^2 is negative, I made the constant d_omega_div_omega2 positive
					//hence the need for the minus signs to make it all work out. 
				
					//do a few strokes, the number will bounce around a bit ... mine read 0.0031 +/- 0.0002						
					//you can add some averaging if you want.
				    lcd_goto_xy(1,3);
					fprintf_P(&lcd_out,PSTR("K_Damp Est."));
					lcd_goto_xy(1,4);
					if (stroke < 5) {
						fprintf_P(&lcd_out,PSTR("------"));
					}
					else {
						fprintf_P(&lcd_out,PSTR("%1.4f"), -1*K_damp_estimator_vector[0]);
					}
					lcd_goto_xy(1,5);
					fprintf_P(&lcd_out,PSTR("M-Inertia (J)"));
					lcd_goto_xy(1,6);		
					fprintf_P(&lcd_out,PSTR("%1.3f kg*m^2"), J_moment);
					break;
				case 3: // action 5.2c
					lcd_goto_xy(1,1);
					fprintf_P(&lcd_out,PSTR("Power"));
					//uncomment this code if you want to display another estimator of rower power ... k*omega_ave^3
					//it was generally reasonably close to the more accurate calculation .. just a nice verification
					lcd_goto_xy(1,2);		
					if (stroke < 5) {
						fprintf_P(&lcd_out,PSTR("--- W"));
					}
					else {
						fprintf_P(&lcd_out,PSTR("%1.0f W   "), power_vector_avg);
					}
					
					lcd_goto_xy(1,3);
					fprintf_P(&lcd_out,PSTR("Power Estimate"));
					//uncomment this code if you want to display another estimator of rower power ... k*omega_ave^3
					//it was generally reasonably close to the more accurate calculation .. just a nice verification
					lcd_goto_xy(1,4);		
					if (stroke < 5) {
						fprintf_P(&lcd_out,PSTR("--- W"));
					}
					else {
						fprintf_P(&lcd_out,PSTR("%1.0f W   "), K_damp*pow(omega_vector_avg_curr,3.0));
					}
					break;
			}//switch
		chThdSleepMilliseconds(250);			
		} //while(true) inner
	} //while(true) outer
}//THREAD5

/*********************************************************************************
Thread 6 -  self contained action task with sub-screens, escapable, takes LCD semaphore
********************************************************************************/ 
THD_FUNCTION(Thread6, arg) {

/* this thread will print stack usage to lcd */
/* note, the order printed out is in order listed in the Thread table, not by the */
/* thread name or number */

  (void)arg;
  uint8_t entry_flag = 0;  
  tp[3] = chThdGetSelfX();//returns a pointer to current thread
    
  while (true) {
	if(chEvtWaitAnyTimeout((eventmask_t)1, TIME_IMMEDIATE) == 1){           
		entry_flag = 1;
		chSemWait(&lcdUSE);
		/*switch lcd_context to thread 5*/
		lcd_context = 6;
		lcd_clear();
	}
	
	if(chEvtWaitAnyTimeout((eventmask_t)2, TIME_IMMEDIATE) == 2){           
		entry_flag = 0;
		/*thread6 gives back the lcd */
		chSemSignal(&lcdUSE);
	}
				
	if(chEvtWaitAnyTimeout((eventmask_t)4, TIME_IMMEDIATE) == 4){           
		lcd_clear();
	}

	if (entry_flag ==1) {
		chPrintStackLCD(lcd_context_count[5]);
	}
	
    chThdSleepMilliseconds(250);		
      
    }
	
  
}//THREAD6


/*********************************************************************************
Thread 7 -  chopper data calculator
********************************************************************************/ 
THD_FUNCTION(Thread7, arg) {

  (void)arg; 
	TCCR1A = 0b00000000;
	TCCR1B = 0b00000011;//clk/64 which is 8us per tick
	
	EICRA = 0b00000111; //all edges INT1 (PD3), rising edge INT0 (PD2)
	EIMSK = 0b00000001;
	TCNT1 = 0;
	msg_t msg;
	
/***********************************************
	calculate other constants & setup
************************************************/
	K_damp = J_moment*d_omega_div_omega2; //= 0.0005 Nms^2 
	cal_factor = 2.0*pi*pow((K_damp/magic_factor), 1.0/3.0); //distance per rev = 0.3532
	
	chThdSleepMilliseconds(3000); 

	chop_counter[0]= 0;
	chop_counter[1]= 0;

	omega_vector[0]= ZERO;
	omega_vector[1]= ZERO;
	
	omega_dot_vector[0]= ZERO;
	omega_dot_vector[1]= ZERO;
	
	power_stroke_screen[0]= 1;
	power_stroke_screen[1]= 0;
	
	calorie_tot = ZERO;
	
	for (j = 0; j<MAX_N; j++) {	
		power_vector[j]= ZERO;
		speed_vector[j]= ZERO;
		power_ratio_vector[j]= ZERO;
		K_damp_estimator_vector[j]= ZERO;
		stroke_vector[j]= ZERO;
	}
	
	omega_dot_screen= 0;
	omega_dot_dot_screen= 0;
	
	K_power = ZERO;
	J_power = ZERO;
	
	stroke_elapsed = ZERO;
	power_elapsed = ZERO;

	GetTime(&t_stroke);
	GetTime(&t_power);

	stroke = 0;

  /* doing all of the calculations when the chopper triggers */

  while (true) {
	chSysLock();

	/* Waiting for chopper & IRQ to happen */
	/* Current thread put to sleep & sets up the reference variable trp_chopper for the trigger to reference */
	/* Will resume with a chThdResumeI referencing trp from interrupt*/
    msg = chThdSuspendTimeoutS(&trp_chopper, TIME_INFINITE);
    chSysUnlock(); 
	
	/***********************************************
	calculate omegas
	************************************************/
	current_dt = (double) chop_counter[0]*seconds_per_tick;
	omega_vector[1] = omega_vector[0];
	omega_vector[0] = (2.0*pi)/((1/2.0)*seconds_per_tick*((double) chop_counter[0]+(double) chop_counter[1]));
	omega_dot_vector[1] = omega_dot_vector[0];
	omega_dot_vector[0] = (omega_vector[0] - omega_vector[1])/(current_dt+DELTA);
    omega_dot_dot = (omega_dot_vector[0] - omega_dot_vector[1])/(current_dt+DELTA);
	
	/***********************************************
	calculate screeners to find power portion of stroke - see spreadsheet if you want to understand this
	************************************************/
	
	if ((omega_dot_dot > -40.0) && (omega_dot_dot < 40.0)) {
		omega_dot_dot_screen = 0;
	}
	else {
		omega_dot_dot_screen = 1;
	}
	if (omega_dot_vector[0] > 15) {
		omega_dot_screen = 1;
	}
	else {
		omega_dot_screen = 0;
	}
	if ( (omega_dot_dot_screen ==1) || ((omega_dot_screen ==1) && (power_stroke_screen[0] ==1))) {
		power_stroke_screen[1] = power_stroke_screen[0];
		power_stroke_screen[0] = 1;
	}	
	else {
		power_stroke_screen[1] = power_stroke_screen[0];
		power_stroke_screen[0] = 0;
	}
	
	
	/****************************************************************************************************
	calculate all items depending on where we are in stroke
	****************************************************************************************************/
	
	/*********************************************************
	if just ended decay stroke, just started power stroke
	**********************************************************/
	if ((power_stroke_screen[0] ==1) && (power_stroke_screen[1] ==0)) {
		//start clock1 = power timer
		GetTime(&t_power);
		J_power = 0.0;
		K_power = 0.0;
		K_damp_estimator_vector[4]= K_damp_estimator_vector[3];
		K_damp_estimator_vector[3]= K_damp_estimator_vector[2];
		K_damp_estimator_vector[2]= K_damp_estimator_vector[1];
		K_damp_estimator_vector[1]= K_damp_estimator_vector[0];
		K_damp_estimator_vector[0] = K_damp_estimator/(stroke_elapsed-power_elapsed+.000001);
		K_damp_estimator_vector_avg = weighted_avg(K_damp_estimator_vector);

		omega_vector_avg = omega_vector_avg + omega_vector[0]*current_dt;
	}
	
	/*********************************************************
	if just ended power stroke, starting decay stroke
	**********************************************************/
	if ((power_stroke_screen[0] ==0) && (power_stroke_screen[1] ==1)) {
		stroke++;
		K_damp_estimator = 0.0;
		//end clock1 = power_timer
		power_elapsed = (double)Get_elapsed_ms(&t_power)/1000.0;
		//end clock2 = stroke_timer
		stroke_elapsed = (double)Get_elapsed_ms(&t_stroke)/1000.0;
		//start clock2 = stroke timer
		GetTime(&t_stroke);
		omega_vector_avg_curr = omega_vector_avg/(stroke_elapsed+DELTA);
		omega_vector_avg = 0.0;
		stroke_distance = distance_rowed-stroke_distance_old;
		stroke_distance_old = distance_rowed;
		
//		reshuffle(speed_vector);
		speed_vector[4]= speed_vector[3];
		speed_vector[3]= speed_vector[2];
		speed_vector[2]= speed_vector[1];
		speed_vector[1]= speed_vector[0];
		speed_vector[0] = stroke_distance/(stroke_elapsed +DELTA);
		speed_vector_avg = weighted_avg(speed_vector);
		split_time = 500.0/(speed_vector_avg+DELTA);
		parse_time((split_time/SIXTY), &split_hours, &split_mins, &split_secs);

//		reshuffle(power_ratio_vector);
		power_ratio_vector[4]= power_ratio_vector[3];
		power_ratio_vector[3]= power_ratio_vector[2];
		power_ratio_vector[2]= power_ratio_vector[1];
		power_ratio_vector[1]= power_ratio_vector[0];
		power_ratio_vector[0] = power_elapsed/(stroke_elapsed + DELTA);
		power_ratio_vector_avg = weighted_avg(power_ratio_vector);

//		reshuffle(power_vector);
		power_vector[4]= power_vector[3];
		power_vector[3]= power_vector[2];
		power_vector[2]= power_vector[1];
		power_vector[1]= power_vector[0];
		power_vector[0]= (J_power + K_power)/(stroke_elapsed + DELTA);
		power_vector_avg = weighted_avg(power_vector);
		
		if (power_vector_avg > 10.0) {
		calorie_tot = calorie_tot+(4*power_vector_avg+350)*(stroke_elapsed)/(4.1868 * 1000);
		}
		
//		reshuffle(stroke_vector);		
		stroke_vector[4]= stroke_vector[3];
		stroke_vector[3]= stroke_vector[2];
		stroke_vector[2]= stroke_vector[1];
		stroke_vector[1]= stroke_vector[0];
		stroke_vector[0]= stroke_elapsed;
		stroke_vector_avg = weighted_avg(stroke_vector);
	}

	/*********************************************************
	if inside power stroke
	**********************************************************/
	if ((power_stroke_screen[0] ==1) && (power_stroke_screen[1] ==1)) {
		J_power = J_power + J_moment * omega_vector[0]*omega_dot_vector[0]*current_dt;
		K_power = K_power + K_damp*(omega_vector[0]*omega_vector[0]*omega_vector[0])*current_dt;
		omega_vector_avg = omega_vector_avg + omega_vector[0]*current_dt;

	}
	
	/*********************************************************
	if in decay stroke
	**********************************************************/
	if ((power_stroke_screen[0] ==0) && (power_stroke_screen[1] ==0)) {
		K_damp_estimator = K_damp_estimator+(omega_dot_vector[0]/((omega_vector[0]+DELTA)*(omega_vector[0]+DELTA)))*current_dt;
		omega_vector_avg = omega_vector_avg + omega_vector[0]*current_dt;
	}

	}//while(true)
}//THREAD7




/*********************************************************************************
Thread 8 -  autonomous task, triggered at start, runs UART
********************************************************************************/ 
THD_FUNCTION(Thread8, arg) {

/* this thread will print stack usage to uart once per second */
/* note, the order printed out is in order listed in the Thread table, not by the */
/* thread name or number */

  (void)arg;
  /* Activates the serial driver using the driver default configuration.*/
  
  sdStart(&SD1, NULL);
  
  /*duplicative, but I needed it in order to us PSTR for strings*/
  usart_init(MYUBRR);
    
  // systime_t wakeTime = chTimeNow();
  // systime_t wakeTime = port_timer_get_time();

  /*systime_t is a type reprenting number of system ticks */
  /*wakeTime will be the number of system ticks since the start of the system */

  systime_t wakeTime = chVTGetSystemTimeX();
  /* Welcome message.*/
  fprintf_P(&usart_out,PSTR("Hello World!\r\n"));
  
  while (true) {
	// Print unused stack for thread 1, thread 2, and idle thread.
    chPrintStackSizes();
	
    // Print unused stack for thread 1, thread 2, and idle thread.
    chPrintUnusedStack();
	
	
    // Add ticks for one second.  The MS2ST macro converts ms to system ticks.
    wakeTime += MS2ST(1000);
	
	// Sleep until next second.
    chThdSleepUntil(wakeTime);
      
    }
	
  
}//THREAD8


/*********************************************************************************
Thread Table -   Remember to change threadcount in nilconf.h
Notes:
1. All of these macros are definied in nil.h
2. Use the stack tracer to optimize stack sizes 
3. Priorities - look at order in Thread table
********************************************************************************/ 

THD_WORKING_AREA(waThread1, 16);
THD_WORKING_AREA(waThread2, 90);
THD_WORKING_AREA(waThread3, 90);
THD_WORKING_AREA(waThread4, 100);
THD_WORKING_AREA(waThread5, 96);
THD_WORKING_AREA(waThread6, 110);
THD_WORKING_AREA(waThread7, 80);
THD_WORKING_AREA(waThread8, 80);
//THD_WORKING_AREA(waTimer, 20); /*just for completeness, commented out here because defined in time.c


THD_TABLE_BEGIN  
  THD_TABLE_ENTRY(waThread1, NULL, Thread1, NULL) //Button Press Handler
  THD_TABLE_ENTRY(waThread7, NULL, Thread7, NULL) //Chopper Calculations
  THD_TABLE_ENTRY(waThread2, NULL, Thread2, NULL) //Menu Navigation Handler
  THD_TABLE_ENTRY(waThread6, NULL, Thread6, NULL) //Stack Output via LCD
  THD_TABLE_ENTRY(waThread8, NULL, Thread8, NULL) //Stack Output via USART
  THD_TABLE_ENTRY(waThread4, NULL, Thread4, NULL) //Time, Dist & Calories
  THD_TABLE_ENTRY(waThread5, NULL, Thread5, NULL) //LCD navigation
  THD_TABLE_ENTRY(waThread3, NULL, Thread3, NULL) //LCD ergo
  THD_TABLE_ENTRY(waTimer, NULL, timer, NULL) //declared in time.c, run at lowest priority
THD_TABLE_END


/*********************************************************************************
Application entry point
********************************************************************************/ 
 
int main(void) {
  /* Fill stacks with 0x55 to enable stack trace */
  nilFillStacks();
 
  /* Initialize LCD semaphore so thread2 can take it */
 
  chSemObjectInit(&lcdUSE,1);

  /*
   * System initializations.
   * - HAL initialization, this also initializes the configured device drivers
   *   and performs the board-specific initializations.
   */
  halInit();
  /*
   * System initializations.
   * - Kernel initialization, the main() function becomes a thread and the
   *   RTOS is active.
   */
  chSysInit();
  
   while (true) {
   /* This is now the idle thread loop, you may perform here a low priority
     task but you must never try to sleep or wait in this loop. Note that
     this tasks runs at the lowest priority level so any instruction added
     here will be executed after all other tasks have been started.*/ 
  }
}
