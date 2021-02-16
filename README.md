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
- drastically simplified RTOS code
- updated thread table
- added source to update font table in eeprom

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
