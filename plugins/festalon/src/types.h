/*  Festalon - NSF Player
 *  Copyright (C) 2001 Aaron Oneal
 *  Copyright (C) 2004 Xodnizel
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __FESTAH_TYPESH
#define __FESTAH_TYPESH

#ifdef _MSC_VER
typedef signed __int8 int8;
typedef signed __int16 int16;
typedef signed __int32 int32;

typedef unsigned __int8 uint8;
typedef unsigned __int16 uint16;
typedef unsigned __int32 uint32;
#else
#include <inttypes.h>

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32; 

typedef uint8_t uint8;  
typedef uint16_t uint16;
typedef uint32_t uint32;
#endif

#ifdef __GNUC__
 typedef unsigned long long uint64;
 typedef long long int64;
 #define INLINE inline
 #define GINLINE inline
#elif MSVC
 typedef __int64 int64;
 typedef unsigned __int64 uint64;
 #define INLINE __inline
 #define GINLINE			/* Can't declare a function INLINE
					   and global in MSVC.  Bummer.
					*/
 #define PSS_STYLE 2			/* Does MSVC compile for anything
					   other than Windows/DOS targets?
					*/
#endif

#if PSS_STYLE==2

#define PSS "\\"
#define PS '\\'

#elif PSS_STYLE==1

#define PSS "/"
#define PS '/'

#elif PSS_STYLE==3

#define PSS "\\"
#define PS '\\'

#elif PSS_STYLE==4

#define PSS ":" 
#define PS ':'

#endif


#ifdef __GNUC__
 #ifdef ARCH_X86
  #define FASTAPASS(x) __attribute__((regparm(x)))
  #define FP_FASTAPASS FASTAPASS
 #else
  #define FASTAPASS(x)	
  #define FP_FASTAPASS(x)	
 #endif
#elif MSVC
 #define FP_FASTAPASS(x)
 #define FASTAPASS(x) __fastcall
#else
 #define FP_FASTAPASS(x)
 #define FASTAPASS(x)
#endif

#endif
