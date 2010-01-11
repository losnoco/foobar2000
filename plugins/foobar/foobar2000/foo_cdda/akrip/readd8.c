/*
 * readd8.c - Copyright (C) 1999 Jay A. Key
 *
 * Functions for the CDR_READ_D8 class of read/init/deinit functions
 *
 **********************************************************************
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 **********************************************************************
 *
 */

#define _AKRIP32_

#include <windows.h>
#include <stdio.h>
#include "myaspi32.h"
#include "scsidefs.h"
#include "aspilib.h"
#include "akrip32.h"

extern CDHANDLEREC *cdHandles;
extern HANDLE *cdMutexes;
extern CRITICAL_SECTION getHandle;
extern int alErrCode;
extern BYTE alAspiErr;
extern int *nextHandle;

extern DWORD (*pfnSendASPI32Command)(LPSRB);



/***************************************************************************
 * readCDAudioLBA_D8
 ***************************************************************************/
DWORD readCDAudioLBA_D8( HCDROM hCD, LPTRACKBUF t )
{
  DWORD dwStatus;
  HANDLE heventSRB;
  SRB_ExecSCSICmd s;
  int idx = (int)hCD - 1;

  if ( (idx<0) || (idx>=MAXCDHAND) || !cdHandles[idx].used )
    {
      alErrCode = ALERR_INVHANDLE;
      return SS_ERR;
    }

  if ( t->numFrames * 2352 > t->maxLen )
    {
      alErrCode = ALERR_BUFTOOSMALL;
      return SS_ERR;
    }

  dwStatus = WaitForSingleObject( cdMutexes[idx], TIMEOUT );
  if ( dwStatus != WAIT_OBJECT_0 )
    {
      alErrCode = ALERR_LOCK;
      return SS_ERR;
    }

  if ( !cdHandles[idx].bInit )
    {
      pauseResumeCD( hCD, TRUE );  // stop the unit if happens to be playing
      cdHandles[idx].bInit = TRUE;
    }

#ifdef AKRIP_DEBUG
  dbprintf( "akrip32: readCDAudioLBA_D8: (%d:%d:%d) %08X:%02X", 
	    cdHandles[idx].ha, cdHandles[idx].tgt, cdHandles[idx].lun,
	    t->startFrame, t->numFrames );
#endif

  heventSRB = CreateEvent( NULL, TRUE, FALSE, NULL );

  memset( &s, 0, sizeof( s ) );
  s.SRB_Cmd        = SC_EXEC_SCSI_CMD;
  s.SRB_HaID       = cdHandles[idx].ha;
  s.SRB_Target     = cdHandles[idx].tgt;
  s.SRB_Lun        = cdHandles[idx].lun;
  s.SRB_Flags      = SRB_DIR_IN | SRB_EVENT_NOTIFY;
  s.SRB_BufLen     = t->numFrames * 2352;
  s.SRB_BufPointer = &(t->buf[0]);
  s.SRB_SenseLen   = SENSE_LEN;
  s.SRB_CDBLen     = 12;
  s.SRB_PostProc   = (LPVOID)heventSRB;
  s.CDBByte[0]     = 0xD8;
  //s.CDBByte[1]     = cdHandles[idx].lun << 5;
  s.CDBByte[3]     = (BYTE)((t->startFrame >> 16) & 0xFF);
  s.CDBByte[4]     = (BYTE)((t->startFrame >> 8) & 0xFF);
  s.CDBByte[5]     = (BYTE)(t->startFrame & 0xFF);
  s.CDBByte[9]     = (BYTE)(t->numFrames & 0xFF);

  ResetEvent( heventSRB );
  dwStatus = pfnSendASPI32Command( (LPSRB)&s );
  if ( dwStatus == SS_PENDING )
    {
      WaitForSingleObject( heventSRB, DEFWAITLEN );
    }
  CloseHandle( heventSRB );

  if ( s.SRB_Status != SS_COMP )
    {
#ifdef AKRIP_DEBUG
      BYTE *p;
      dbprintf( "akrip32: readCDAudioLBA_D8: ERROR! 0x%08X\n", s.SRB_Status );
      dbprintf( "akrip32:  haStat == %d (0x%04X), tgtStat == %d (0x%04X)",
	       s.SRB_HaStat, s.SRB_HaStat, s.SRB_TargStat, s.SRB_TargStat );
      p = s.SenseArea;
      dbprintf( "  %02X %02X %02X %02X %02X %02X %02X %02X",
	       p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7] );
      p = &(s.SenseArea[8]);
      dbprintf( "  %02X %02X %02X %02X %02X %02X %02X %02X",
	       p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7] );
#endif
      alErrCode = ALERR_ASPI;
      alAspiErr = s.SRB_Status;
      ReleaseMutex( cdMutexes[idx] );
      return SS_ERR;
    }

  t->len = t->numFrames * 2352;
  t->startOffset = 0;
  t->status = s.SRB_Status;

  ReleaseMutex( cdMutexes[idx] );
  return s.SRB_Status;
}


