## Ergware ##
ErgWare is an open source hardware/software platform for DIY ergometers. All of the source code is hosted here and the project itself is fully documented at [my project page.](https://dvernooy.github.io/projects/ergware)

#### v0.1 ####
- initial release
- source code for ATMEGA328
- pdf w/ circuit diagram and chopper details
- pdf w/ details of the screenshots
- excel spreadsheet with analytical model & data from actual erg

#### v0.2 ####
- rewritten using Chibios/Nil RTOS
- added calorie counter, timers and pacers
- updated fonts

#### v0.3 ####
- updated button-press detection algorithm to be more robust
- drastically simplified RTOS code
- updated thread table
- added a splash screen
- first flash the binary (main.hex) to add the font table in eeprom
- then flash program binary (nil.hex)

#### v0.4 ####
- code to work w/ Arduino UNO rev3 *without a bootloader installed*
- "*.hex" files ... cannot be programmed/flashed from usual Arduino environment
- A pdf with detailed setup & build instructions are in the v0.4 folder
- same as v0.3 except assumes 16MHz external clock (& Atmega328)
- use 3.3V from arduino on LCD_Vcc
- first flash the eeprom save binary (main.hex) to add the font table in eeprom
- then flash the main program binary (nil.hex)

#### v0.5 ####
- code to work with a "regular" Arduino UNO rev3
- "*.ino" files ... can be flashed from usual "Arduino" environment
- A pdf with detailed setup & build instructions are in the v0.5 folder
- NOTE: you still MUST access the board over ISP to set the EESAVE fuse (see instructions),
  so you will need an external programmer (like stk500, usbasp, ponyprog, another UNO board, etc...) 
- same as v0.3 except assumes 16MHz external clock (& Atmega328)
- use 3.3V from arduino on LCD_Vcc
- first set the EESAVE fuse
- next program the EEPROM (eeprom_burn.ino) to add the font table in eeprom
- then program the main program program binary (main.ino)

#### TO USE THIS: ####
1. Build your erg ... openergo.webs.com
2. Build the chopper wheel & attach the sensor ... see pdf
3. Customize your sensor interface circuit if needed ... get a logic level signal into PD2 of ATMEGA328 ... see pdf
4. Build the circuit ... see pdf
5. Measure or estimate flywheel moment of inertia & add to source code ... will make your numbers more accurate, see main.c
6. Compile code & flash ... see makefile
7. [Optional] compile a version of code to measure d_omega_div_omega2, get the number for your erg, put in source code as a constant & recompile. This will make your numbers more accurate ... see main.c & the excel sheet if you really want to understand
8. Have fun!

  Dave Vernooy  
  Niskayuna, NY    
  January 2017
