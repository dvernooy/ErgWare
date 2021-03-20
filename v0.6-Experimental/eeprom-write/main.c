/*
** main.c
**
*/
#include <avr/eeprom.h>
#include <inttypes.h>
#include <avr/io.h>


/* Alphabet lookup */
unsigned char font5x7 [][5] = {
	{ 0x00, 0x00, 0x00, 0x00, 0x00 },   // sp - 0
    { 0x00, 0x00, 0x2f, 0x00, 0x00 },   // !
    { 0x00, 0x07, 0x00, 0x07, 0x00 },   // "
    { 0x14, 0x7f, 0x14, 0x7f, 0x14 },   // #
    { 0x24, 0x2a, 0x7f, 0x2a, 0x12 },   // $
	{ 0x32, 0x34, 0x08, 0x16, 0x26 },   // %
    { 0x36, 0x49, 0x55, 0x22, 0x50 },   // &
    { 0x00, 0x05, 0x03, 0x00, 0x00 },   // '
    { 0x00, 0x1c, 0x22, 0x41, 0x00 },   // (
    { 0x00, 0x41, 0x22, 0x1c, 0x00 },   // )
    { 0x14, 0x08, 0x3E, 0x08, 0x14 },   // *
    { 0x08, 0x08, 0x3E, 0x08, 0x08 },   // +
    { 0x00, 0x00, 0x50, 0x30, 0x00 },   // ,
    { 0x10, 0x10, 0x10, 0x10, 0x10 },   // -
    { 0x00, 0x60, 0x60, 0x00, 0x00 },   // .
    { 0x20, 0x10, 0x08, 0x04, 0x02 },   // /-15
    { 0x3E, 0x51, 0x49, 0x45, 0x3E },   // 0
    { 0x00, 0x42, 0x7F, 0x40, 0x00 },   // 1
    { 0x42, 0x61, 0x51, 0x49, 0x46 },   // 2
    { 0x21, 0x41, 0x45, 0x4B, 0x31 },   // 3
    { 0x18, 0x14, 0x12, 0x7F, 0x10 },   // 4
    { 0x27, 0x45, 0x45, 0x45, 0x39 },   // 5
    { 0x3C, 0x4A, 0x49, 0x49, 0x30 },   // 6
    { 0x01, 0x71, 0x09, 0x05, 0x03 },   // 7
    { 0x36, 0x49, 0x49, 0x49, 0x36 },   // 8
    { 0x06, 0x49, 0x49, 0x29, 0x1E },   // 9
    { 0x00, 0x36, 0x36, 0x00, 0x00 },   // :
    { 0x00, 0x56, 0x36, 0x00, 0x00 },   // ;
    { 0x08, 0x14, 0x22, 0x41, 0x00 },   // <
    { 0x14, 0x14, 0x14, 0x14, 0x14 },   // =
    { 0x00, 0x41, 0x22, 0x14, 0x08 },   // >
    { 0x02, 0x01, 0x51, 0x09, 0x06 },   // ?
    { 0x32, 0x49, 0x59, 0x51, 0x3E },   // @-32
    { 0x7E, 0x11, 0x11, 0x11, 0x7E },   // A
    { 0x7F, 0x49, 0x49, 0x49, 0x36 },   // B
    { 0x3E, 0x41, 0x41, 0x41, 0x22 },   // C
    { 0x7F, 0x41, 0x41, 0x22, 0x1C },   // D
    { 0x7F, 0x49, 0x49, 0x49, 0x41 },   // E
    { 0x7F, 0x09, 0x09, 0x09, 0x01 },   // F
    { 0x3E, 0x41, 0x49, 0x49, 0x7A },   // G
    { 0x7F, 0x08, 0x08, 0x08, 0x7F },   // H
    { 0x00, 0x41, 0x7F, 0x41, 0x00 },   // I
    { 0x20, 0x40, 0x41, 0x3F, 0x01 },   // J
    { 0x7F, 0x08, 0x14, 0x22, 0x41 },   // K
    { 0x7F, 0x40, 0x40, 0x40, 0x40 },   // L
    { 0x7F, 0x02, 0x0C, 0x02, 0x7F },   // M
    { 0x7F, 0x04, 0x08, 0x10, 0x7F },   // N
    { 0x3E, 0x41, 0x41, 0x41, 0x3E },   // O
    { 0x7F, 0x09, 0x09, 0x09, 0x06 },   // P
    { 0x3E, 0x41, 0x51, 0x21, 0x5E },   // Q
    { 0x7F, 0x09, 0x19, 0x29, 0x46 },   // R
    { 0x46, 0x49, 0x49, 0x49, 0x31 },   // S
    { 0x01, 0x01, 0x7F, 0x01, 0x01 },   // T
    { 0x3F, 0x40, 0x40, 0x40, 0x3F },   // U
    { 0x1F, 0x20, 0x40, 0x20, 0x1F },   // V
    { 0x3F, 0x40, 0x38, 0x40, 0x3F },   // W
    { 0x63, 0x14, 0x08, 0x14, 0x63 },   // X
    { 0x07, 0x08, 0x70, 0x08, 0x07 },   // Y
    { 0x61, 0x51, 0x49, 0x45, 0x43 },   // Z-58
    { 0x00, 0x7F, 0x41, 0x41, 0x00 },   // [
    { 0x55, 0x2A, 0x55, 0x2A, 0x55 },   // 55
    { 0x00, 0x41, 0x41, 0x7F, 0x00 },   // ]
    { 0x04, 0x02, 0x01, 0x02, 0x04 },   // ^
    { 0x40, 0x40, 0x40, 0x40, 0x40 },   // _
    { 0x00, 0x01, 0x02, 0x04, 0x00 },   // '
    { 0x20, 0x54, 0x54, 0x54, 0x78 },   // a
    { 0x7F, 0x48, 0x44, 0x44, 0x38 },   // b
    { 0x38, 0x44, 0x44, 0x44, 0x20 },   // c
    { 0x38, 0x44, 0x44, 0x48, 0x7F },   // d
    { 0x38, 0x54, 0x54, 0x54, 0x18 },   // e
    { 0x08, 0x7E, 0x09, 0x01, 0x02 },   // f
    { 0x0C, 0x52, 0x52, 0x52, 0x3E },   // g
    { 0x7F, 0x08, 0x04, 0x04, 0x78 },   // h
    { 0x00, 0x44, 0x7D, 0x40, 0x00 },   // i
    { 0x20, 0x40, 0x44, 0x3D, 0x00 },   // j
    { 0x7F, 0x10, 0x28, 0x44, 0x00 },   // k
    { 0x00, 0x41, 0x7F, 0x40, 0x00 },   // l
    { 0x7C, 0x04, 0x18, 0x04, 0x78 },   // m
    { 0x7C, 0x08, 0x04, 0x04, 0x78 },   // n
    { 0x38, 0x44, 0x44, 0x44, 0x38 },   // o
    { 0x7C, 0x14, 0x14, 0x14, 0x08 },   // p
    { 0x08, 0x14, 0x14, 0x18, 0x7C },   // q
    { 0x7C, 0x08, 0x04, 0x04, 0x08 },   // r
    { 0x48, 0x54, 0x54, 0x54, 0x20 },   // s
    { 0x04, 0x3F, 0x44, 0x40, 0x20 },   // t
    { 0x3C, 0x40, 0x40, 0x20, 0x7C },   // u
    { 0x1C, 0x20, 0x40, 0x20, 0x1C },   // v
    { 0x3C, 0x40, 0x30, 0x40, 0x3C },   // w
    { 0x44, 0x28, 0x10, 0x28, 0x44 },   // x
    { 0x0C, 0x50, 0x50, 0x50, 0x3C },   // y
    { 0x44, 0x64, 0x54, 0x4C, 0x44 },   // z ... chr = 123 (90)
};


//starts at 456
const uint8_t NumLookupLow[16][11]= {

{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // Code for char  
//upshifted
{0x00,0x00,0x00,0xB8,0xF8,0x78,0x00,0x00,0x00,0x00,0x00}, // Code for char ,
{0x00,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x00,0x00}, // Code for char -
{0x00,0x00,0x00,0x70,0x70,0x70,0x00,0x00,0x00,0x00,0x00}, // Code for char .
{0x30,0x38,0x1C,0x0E,0x07,0x03,0x01,0x00,0x00,0x00,0x00}, // Code for char /
{0x0F,0x3F,0x3C,0x66,0x63,0x61,0x60,0x60,0x30,0x3F,0x0F}, // Code for char 0
{0x00,0x00,0x60,0x60,0x60,0x7F,0x7F,0x60,0x60,0x60,0x00}, // Code for char 1
{0x60,0x70,0x78,0x7C,0x6E,0x67,0x63,0x61,0x60,0x60,0x60}, // Code for char 2
{0x18,0x38,0x70,0x61,0x61,0x61,0x61,0x61,0x73,0x3E,0x1C}, // Code for char 3
{0x07,0x07,0x06,0x06,0x06,0x06,0x06,0x7F,0x7F,0x06,0x06}, // Code for char 4
{0x18,0x38,0x70,0x60,0x60,0x60,0x60,0x60,0x71,0x3F,0x1F}, // Code for char 5
{0x1F,0x3F,0x73,0x61,0x61,0x61,0x61,0x61,0x73,0x3F,0x1E}, // Code for char 6
{0x00,0x00,0x00,0x60,0x78,0x1E,0x07,0x01,0x00,0x00,0x00}, // Code for char 7
{0x1E,0x3F,0x73,0x61,0x61,0x61,0x61,0x61,0x73,0x3F,0x1E}, // Code for char 8
{0x00,0x00,0x61,0x61,0x61,0x71,0x39,0x1D,0x0F,0x07,0x01}, // Code for char 9
{0x00,0x00,0x00,0x38,0x38,0x38,0x00,0x00,0x00,0x00,0x00}, // Code for char :

};


//starts at 632
const uint8_t NumLookupHigh[16][11]= {
{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // Code for char  
//upshifted
{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // Code for char ,
{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // Code for char -						
{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // Code for char .						
{0x00,0x00,0x00,0x00,0x00,0x80,0xC0,0xE0,0x70,0x38,0x1C}, // Code for char /						
{0xF0,0xFC,0x0C,0x06,0x06,0x86,0xC6,0x66,0x3C,0xFC,0xF0}, // Code for char 0						
{0x00,0x00,0x18,0x18,0x1C,0xFE,0xFE,0x00,0x00,0x00,0x00}, // Code for char 1						
{0x38,0x3C,0x0E,0x06,0x06,0x06,0x86,0xC6,0xEE,0x7C,0x38}, // Code for char 2						
{0x18,0x1C,0x0E,0x86,0x86,0x86,0x86,0x86,0xCE,0xFC,0x78}, // Code for char 3						
{0x80,0xC0,0xE0,0x70,0x38,0x1C,0x0E,0xFE,0xFE,0x00,0x00}, // Code for char 4						
{0x7E,0xFE,0xC6,0xC6,0xC6,0xC6,0xC6,0xC6,0xC6,0x86,0x06}, // Code for char 5						
{0x80,0xE0,0xF0,0xB8,0x9C,0x8E,0x86,0x86,0x86,0x00,0x00}, // Code for char 6						
{0x06,0x06,0x06,0x06,0x06,0x06,0x86,0xE6,0x7E,0x1E,0x06}, // Code for char 7						
{0x00,0x78,0xFC,0xCE,0x86,0x86,0x86,0xCE,0xFC,0x78,0x00}, // Code for char 8						
{0x78,0xFC,0xCE,0x86,0x86,0x86,0x86,0x86,0xCE,0xFC,0xF8}, // Code for char 9						
{0x00,0x00,0x00,0xE0,0xE0,0xE0,0x00,0x00,0x00,0x00,0x00}, // Code for char :						
};

//starts at 632
const uint8_t Paddle[3][60]= {
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xC0, 0xE0, 0xE6, 0xEF,
 0xEF, 0xEF, 0xEF, 0x82, 0x82, 0x80, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, //0x00
 
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x7C, 0xFE, 0xFF, 0xFF, 0xE7, 0xE3, 0xE1, 0x70,
 0x30, 0x30, 0x31, 0x19, 0x79, 0xB9, 0x5B, 0x23, 0x13, 0x0A, 
 0x06, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, //0x00
 
{0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x81, 0xC1, 0xC1, 0x21,
 0x11, 0x09, 0x05, 0x02, 0x01, 0x00, 0x01, 0x01, 0x01, 0x01, 
 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01}, //0x01


};


uint16_t EEMEM counter;

int main (void)
{
uint16_t addr;
uint8_t ByteOfData;
//double temp;
uint16_t i, j;


    for(i=0;i<91;i++) {
		for(j=0;j<5;j++) {
			ByteOfData = font5x7[i][j];
			addr = (i*5+j+1) % 512;
			eeprom_update_byte((uint8_t *) addr , ByteOfData);
        }
	}
 

    for(i=0;i<16;i++) {
		for(j=0;j<11;j++) {
			ByteOfData = NumLookupLow[i][j];
			addr = 455 + (i*11+j+1);
			eeprom_update_byte((uint8_t *) addr , ByteOfData);
        }
	}
	
	    for(i=0;i<16;i++) {
		for(j=0;j<11;j++) {
			ByteOfData = NumLookupHigh[i][j];
			addr = 631 + (i*11+j+1);
			eeprom_update_byte((uint8_t *) addr , ByteOfData);
        }
	}
	
		    for(i=0;i<3;i++) {
		    for(j=0;j<60;j++) {
			ByteOfData = Paddle[i][j];
			addr = 807 + (i*60+j+1);
			eeprom_update_byte((uint8_t *) addr , ByteOfData);
        }
	}

}
