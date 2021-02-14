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

#ifndef __LCD_H_
#define __LCD_H_


/* Lcd screen size */
#define LCD_X_DIMENSION 84
#define LCD_Y_DIMENSION 48
#define LCD_TOTAL ((LCD_X_DIMENSION * LCD_Y_DIMENSION) / 8)


/* LCD pins */
#define LCD_CLK_PIN 	(1<<PC4)
#define LCD_DATA_PIN 	(1<<PC3)
#define LCD_DC_PIN 		(1<<PC2)
#define LCD_CE_PIN 		(1<<PC1)
#define LCD_RST_PIN 	(1<<PC0)
#define LCD_PORT		PORTC
#define LCD_DDR			DDRC


void lcd_init(void);
void lcd_contrast(uint8_t contrast);
void lcd_clear(void);
void lcd_goto_xy(uint8_t x, uint8_t y);
void lcd_go_up_one(void);
void lcd_chr(char chr);
int lcd_chr_printf(char chr, FILE *stream);
extern uint8_t invert;
extern uint8_t bigfont;
#endif



