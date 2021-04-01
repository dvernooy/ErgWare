/*
Copyright (C) Trampas Stern  name of author

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

/*************************************
Datatypes.h 

11/10/2004 Trampas Stern
*************************************/

#ifndef __DATATYPES_H
#define __DATATYPES_H

#include <inttypes.h>

typedef unsigned char UINT;
typedef signed char INT;
typedef char CHAR;
typedef unsigned int UWORD;
typedef signed char BYTE;
typedef unsigned char UBYTE;
typedef signed int WORD;
typedef unsigned long UDWORD;
typedef signed long DWORD; 
typedef float  FLOAT; 


typedef unsigned char UINT8;
typedef signed char INT8;
typedef unsigned int UINT16;
typedef signed int INT16;
typedef unsigned long UINT32;
typedef signed long INT32; 



typedef union {
	UWORD data ;
	struct {
		BYTE low;
		BYTE high;
	};
} UWORDbytes;

#define MAX_UDWORD 0xFFFFFFFF

#endif //__DATATYPES_H
