/* FluidSynth - A Software Synthesizer
 *
 * Copyright (C) 2003  Peter Hanappe and others.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307, USA
 */


#ifndef _FLUIDSYNTH_UNICODE_H
#define _FLUIDSYNTH_UNICODE_H

#ifdef WIN32
#include <tchar.h>
#define FLUID_TCHAR TCHAR
#define FLUID_TEXT(s) _T(s)
#define FLUID_TFOPEN(_f,_m) _tfopen(_f,FLUID_TEXT(_m))
#define FLUID_TCSLEN(_s) _tcslen(_s)
#define FLUID_TCSCMP(_s,_t) _tcscmp(_s,_t)
#define FLUID_TCSICMP(_s,_t) _tcsicmp(_s,_t)
#define FLUID_TCSNCMP(_s,_t,_n) _tcsncmp(_s,_t,_n)
#define FLUID_TCSCPY(_dst,_src) _tcscpy(_dst,_src)
#define FLUID_TCSNCPY(_dst,_src,_n) _tcsncpy(_dst,_src,_n)
#define FLUID_TCSDUP(s) _tcsdup(s)
#ifdef UNICODE
#define FLUID_TCSCONV(_dst,_src,_n) mbstowcs(_dst,_src,_n)
#else
#define FLUID_TCSCONV(_dst,_src,_n) strncpy(_dst,_src,_n)
#endif
#else
#define FLUID_TCHAR char
#define FLUID_TEXT(s) s
#define FLUID_TFOPEN(_f,_m) fopen(_f,_m)
#define FLUID_TCSLEN(_s) strlen(_s)
#define FLUID_TCSCMP(_s,_t) strcmp(_s,_t)
#define FLUID_TCSICMP(_s,_t) strcasecmp(_s,_t)
#define FLUID_TCSNCMP(_s,_t,_n) strncmp(_s,_t,_n)
#define FLUID_TCSCPY(_dst,_src) strcpy(_dst,_src)
#define FLUID_TCSNCPY(_dst,_src,_n) strncpy(_dst,_src,_n)
#define FLUID_TCSDUP(s) strdup(s)
#endif


#endif
