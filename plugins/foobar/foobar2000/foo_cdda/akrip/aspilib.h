/*
 * aspilib.h - Copyright (C) 1999 Jay A. Key
 *
 * Generic routines to access wnaspi32.dll
 *
 **********************************************************************
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 **********************************************************************
 *
 */

#ifndef _ASPILIB_H_
#define _ASPILIB_H_

#include "myaspi32.h"
#include "akrip32.h"

#define MAJVER   0
#define MINVER   99

#define DEFWAITLEN    4000

#define READERRREC    0x01
#define CDRPARMS      0x0D
#define CDRAUDIOCTL   0x0E
#define CDRCAPS       0x2A

typedef BYTE ReadErrorRecoveryParmsMask[8];
typedef BYTE CDRomParmsMask[8];
typedef BYTE CDRomCapabilitiesStatusMask[20];
typedef BYTE CDRomAudioControlMask[16];

typedef struct
{
  ReadErrorRecoveryParmsMask  rer;
  CDRomParmsMask              cpm;
  CDRomCapabilitiesStatusMask ccsm;
  CDRomAudioControlMask       cacm;
  BOOL                        rerAvail;
  BOOL                        cpmAvail;
  BOOL                        ccsmAvail;
  BOOL                        cacmAvail;
} SENSEMASK, *PSENSEMASK, FAR *LPSENSEMASK;

int getSCSIDevType( BYTE bHostAdapter, BYTE bTarget, BYTE bLUN,
		    LPBYTE pDevType, LPSTR lpDevType, int iDevTypeLen );


typedef DWORD (*CDREADFN)( HCDROM hCD, LPTRACKBUF t );
typedef DWORD (*CDDEINIT)( HCDROM hCD );

typedef struct {
  BOOL bInit;
} R6INIT;

typedef struct {
  BOOL bInit;
} R10INIT;

typedef union {
  R6INIT r6;
  R10INIT r10;
} CDINIT;

typedef struct {
  BYTE      ha;
  BYTE      tgt;
  BYTE      lun;
  BYTE      readType;
  BOOL      used;
  BOOL      bInit;
  SENSEMASK sm;
  BOOL      smRead;
  CDREADFN  pfnRead;
  CDDEINIT  pfnDeinit;
  int       numCheck;     // used for jitter correction
  int       numOverlap;   // ...
  int       readMode;
} CDHANDLEREC;


#define MAXCDHAND 16
#define TIMEOUT    (10*1000)

DWORD readCDAudioLBA_ANY( HCDROM hCD, LPTRACKBUF t );
DWORD readCDAudioLBA_ATAPI( HCDROM hCD, LPTRACKBUF t );
DWORD readCDAudioLBA_READ10( HCDROM hCD, LPTRACKBUF t );
DWORD readCDAudioLBA_D8( HCDROM hCD, LPTRACKBUF t );
DWORD readCDAudioLBA_D4( HCDROM hCD, LPTRACKBUF t );

DWORD pauseResumeCD( HCDROM hCD, BOOL bPause );

void dbprintf( char *fmt, ... );
#endif
