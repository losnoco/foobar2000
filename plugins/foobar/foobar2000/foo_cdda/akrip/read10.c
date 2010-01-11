/*
 * read10.c - Copyright (C) 1999 Jay A. Key
 *
 * Functions for the CDR_READ10 class of read/init/deinit functions
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

DWORD deinitREAD10( HCDROM hCD );


/***************************************************************************
 * initREAD10
 ***************************************************************************/
DWORD initREAD10_2( HCDROM hCD )
{
  DWORD dwStatus;
  HANDLE heventSRB;
  SRB_ExecSCSICmd s;
  int idx = (int)hCD - 1;
  int i;
  BYTE init1[] = { 0, 0, 0, 0x08, 0, 0, 0, 0, 0, 0, 0x09, 0x30, 0x23, 6, 0, 0, 0, 0, 0, 0x80 };
  BYTE init2[] =
    { 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 9, 48, 1, 6, 32, 7, 0, 0, 0, 0 };

  if ( (idx<0) || (idx>=MAXCDHAND) || !cdHandles[idx].used )
    {
      alErrCode = ALERR_INVHANDLE;
      return SS_ERR;
    }

#if 0
  dwStatus = pauseResumeCD( hCD, TRUE );
  if ( dwStatus != SS_COMP )
    {
      return dwStatus;
    }


  dwStatus = startStopUnit( hCD, TRUE, TRUE );
  if ( dwStatus != SS_COMP )
    {
      return dwStatus;
    }
#endif

  for( i = 0; i < 2; i++ )
    {
      heventSRB = CreateEvent( NULL, TRUE, FALSE, NULL );

      memset( &s, 0, sizeof( s ) );
      s.SRB_Cmd        = SC_EXEC_SCSI_CMD;
      s.SRB_HaID       = cdHandles[idx].ha;
      s.SRB_Target     = cdHandles[idx].tgt;
      s.SRB_Lun        = cdHandles[idx].lun;
      //      s.SRB_Flags      = SRB_EVENT_NOTIFY | SRB_ENABLE_RESIDUAL_COUNT;
      s.SRB_Flags      = SRB_EVENT_NOTIFY;
      s.SRB_BufLen     = 0x14;
      s.SRB_BufPointer = (i==0)?init1:init2;
      s.SRB_SenseLen   = SENSE_LEN;
      s.SRB_CDBLen     = 6;
      s.SRB_PostProc   = (LPVOID)heventSRB;
      s.CDBByte[0]     = 0x15;         /* mode select (6) */
      s.CDBByte[1]     = 0x10;         /* no save */
      s.CDBByte[4]     = 0x14;         /* buffer length */

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
	  dbprintf( "akrip32: init10: #%d failed -> 0x%04X", i, s.SRB_Status );
#endif
	  alErrCode = ALERR_ASPI;
	  alAspiErr = s.SRB_Status;
	  return SS_ERR;
	}
    }

  cdHandles[idx].pfnDeinit = deinitREAD10;

  return s.SRB_Status;
}


/***************************************************************************
 * initREAD10
 * -- try a shorter version of the mode select ...
 ***************************************************************************/
DWORD initREAD10( HCDROM hCD )
{
  DWORD dwStatus;
  HANDLE heventSRB;
  SRB_ExecSCSICmd s;
  int idx = (int)hCD - 1;
  //  int i;
  BYTE init1[] = { 0, 0, 0, 0x08, 0, 0, 0, 0, 0, 0, 0x09, 0x30 };

  if ( (idx<0) || (idx>=MAXCDHAND) || !cdHandles[idx].used )
    {
      alErrCode = ALERR_INVHANDLE;
      return SS_ERR;
    }

#if 0
  dwStatus = pauseResumeCD( hCD, TRUE );
  if ( dwStatus != SS_COMP )
    {
      return dwStatus;
    }


  dwStatus = startStopUnit( hCD, TRUE, TRUE );
  if ( dwStatus != SS_COMP )
    {
      return dwStatus;
    }
#endif

  heventSRB = CreateEvent( NULL, TRUE, FALSE, NULL );

  memset( &s, 0, sizeof( s ) );
  s.SRB_Cmd        = SC_EXEC_SCSI_CMD;
  s.SRB_HaID       = cdHandles[idx].ha;
  s.SRB_Target     = cdHandles[idx].tgt;
  s.SRB_Lun        = cdHandles[idx].lun;
  s.SRB_Flags      = SRB_EVENT_NOTIFY;
  s.SRB_BufLen     = 0x0C;
  s.SRB_BufPointer = init1;
  s.SRB_SenseLen   = SENSE_LEN;
  s.SRB_CDBLen     = 6;
  s.SRB_PostProc   = (LPVOID)heventSRB;
  s.CDBByte[0]     = 0x15;         /* mode select (6) */
  s.CDBByte[4]     = 0x0C;         /* buffer length */

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
      dbprintf( "akrip32: init10 failed -> 0x%04X", s.SRB_Status );
#endif
      alErrCode = ALERR_ASPI;
      alAspiErr = s.SRB_Status;
      return SS_ERR;
    }

  cdHandles[idx].pfnDeinit = deinitREAD10;

  return s.SRB_Status;
}


/***************************************************************************
 * deinitREAD10
 ***************************************************************************/
DWORD deinitREAD10( HCDROM hCD )
{
  DWORD dwStatus;
  HANDLE heventSRB;
  SRB_ExecSCSICmd s;
  int idx = (int)hCD - 1;
  BYTE init1[] = { 0, 0, 0, 8, 83, 0, 0, 0, 0, 0, 8, 0 };

  if ( (idx<0) || (idx>=MAXCDHAND) || !cdHandles[idx].used )
    {
      alErrCode = ALERR_INVHANDLE;
      return SS_ERR;
    }

  cdHandles[idx].bInit = FALSE;

  heventSRB = CreateEvent( NULL, TRUE, FALSE, NULL );

  memset( &s, 0, sizeof( s ) );
  s.SRB_Cmd        = SC_EXEC_SCSI_CMD;
  s.SRB_HaID       = cdHandles[idx].ha;
  s.SRB_Target     = cdHandles[idx].tgt;
  s.SRB_Lun        = cdHandles[idx].lun;
  s.SRB_Flags      = SRB_EVENT_NOTIFY | SRB_ENABLE_RESIDUAL_COUNT;
  s.SRB_BufLen     = 0x0C;
  s.SRB_BufPointer = init1;
  s.SRB_SenseLen   = SENSE_LEN;
  s.SRB_CDBLen     = 6;
  s.SRB_PostProc   = (LPVOID)heventSRB;
  s.CDBByte[0]     = 0x15;
  s.CDBByte[4]     = 0x0C;

  ResetEvent( heventSRB );
  dwStatus = pfnSendASPI32Command( (LPSRB)&s );
  if ( dwStatus == SS_PENDING )
    {
      WaitForSingleObject( heventSRB, DEFWAITLEN );
    }
  CloseHandle( heventSRB );

  if ( s.SRB_Status != SS_COMP )
    {
      alErrCode = ALERR_ASPI;
      alAspiErr = s.SRB_Status;
      return SS_ERR;
    }

  return s.SRB_Status;
}

/***************************************************************************
 * readCDAudioLBA_READ10
 ***************************************************************************/
DWORD readCDAudioLBA_READ10( HCDROM hCD, LPTRACKBUF t )
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

  if ( !cdHandles[idx].bInit )
    {
      if ( cdHandles[idx].readType == CDR_READ10_2 )
	{
	  if ( initREAD10_2( hCD ) != SS_COMP )
	    return SS_ERR;
	}
      else
	{
	  if ( initREAD10( hCD ) != SS_COMP )
	    return SS_ERR;
	}
      cdHandles[idx].bInit = TRUE;
    }

  heventSRB = CreateEvent( NULL, TRUE, FALSE, NULL );

  memset( &s, 0, sizeof( s ) );
  s.SRB_Cmd        = SC_EXEC_SCSI_CMD;
  s.SRB_HaID       = cdHandles[idx].ha;
  s.SRB_Target     = cdHandles[idx].tgt;
  s.SRB_Lun        = cdHandles[idx].lun;
  s.SRB_Flags      = SRB_DIR_IN | SRB_EVENT_NOTIFY;
  s.SRB_BufLen     = t->maxLen;
  s.SRB_BufPointer = &(t->buf[0]);
  s.SRB_SenseLen   = SENSE_LEN;
  s.SRB_CDBLen     = 10;
  s.SRB_PostProc   = (LPVOID)heventSRB;
  s.CDBByte[0]     = 0x28;                 // read10
  s.CDBByte[1]     = cdHandles[idx].lun << 5;
  s.CDBByte[3]     = (BYTE)((t->startFrame >> 16) & 0xFF);
  s.CDBByte[4]     = (BYTE)((t->startFrame >> 8) & 0xFF);
  s.CDBByte[5]     = (BYTE)(t->startFrame & 0xFF);
  s.CDBByte[8]     = (BYTE)(t->numFrames & 0xFF);

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
      dbprintf( "akrip32: readCDAudioLBA_READ10: ERROR! 0x%08X\n", s.SRB_Status );
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
      return SS_ERR;
    }

  t->len = t->numFrames * 2352;
  t->startOffset = 0;
  t->status = s.SRB_Status;

  return s.SRB_Status;
}

