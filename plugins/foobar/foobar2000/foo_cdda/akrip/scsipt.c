#ifdef AKRIP_DEBUG
#define _DEBUG_SCSIPT
#endif
/*
 * scsipt.c - Copyright (C) 1999 Jay A. Key
 *
 * Native NT support functions via the SCSI Pass Through interface instead
 * of ASPI.  Although based on information from the NT 4.0 DDK from 
 * Microsoft, the information has been sufficiently distilled to allow
 * compilation w/o having the DDK installed.
 *
 * Implements the following functions:
 *   DWORD SPTIGetASPI32SupportInfo(void);
 *   DWORD SPTISendASPI32Command(LPSRB);
 * which are equivalents to their ASPI counterparts.  Additionally implements
 *   int InitSCSIPT( void );
 *   int DeinitSCSIPT( void );
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

#include <windows.h>
#include <stdio.h>
#include <stddef.h>
#include "myaspi32.h"
#include "scsidefs.h"
#include "aspilib.h"
#include "akrip32.h"
#include "scsipt.h"


typedef struct {
  BYTE ha;
  BYTE tgt;
  BYTE lun;
  BYTE driveLetter;
  BOOL bUsed;
  HANDLE hDevice;
  BYTE inqData[36];
} DRIVE;

typedef struct {
  BYTE numAdapters;
  DRIVE drive[26];
} SPTIGLOBAL;

void GetDriveInformation( BYTE i, DRIVE *pDrive );
BYTE SPTIGetNumAdapters( void );
BYTE SPTIGetDeviceIndex( BYTE ha, BYTE tgt, BYTE lun );
DWORD SPTIHandleHaInquiry( LPSRB_HAInquiry lpsrb );
DWORD SPTIGetDeviceType( LPSRB_GDEVBlock lpsrb );
DWORD SPTIExecSCSICommand( LPSRB_ExecSCSICmd lpsrb, BOOL bBeenHereBefore );
HANDLE GetFileHandle( BYTE i );

static BOOL bSCSIPTInit = FALSE;
static SPTIGLOBAL sptiglobal;
static BOOL bUsingSCSIPT = FALSE;

/*
 * Initialization of SCSI Pass Through Interface code.  Responsible for
 * setting up the array of SCSI devices.  This code will be a little
 * different from the normal code -- it will query each drive letter from
 * C: through Z: to see if it is  a CD.  When we identify a CD, we then 
 * send CDB with the INQUIRY command to it -- NT will automagically fill in
 * the PathId, TargetId, and Lun for us.
 */
int InitSCSIPT( void )
{
  BYTE i;
  char buf[4];
  UINT uDriveType;
  int retVal = 0;

  if ( bSCSIPTInit )
    return 0;

  memset( &sptiglobal, 0, sizeof(SPTIGLOBAL) );
  for( i = 0; i < 26; i++ )
    sptiglobal.drive[i].hDevice = INVALID_HANDLE_VALUE;
  for( i = 2; i < 26; i++ )
    {
      sprintf( buf, "%c:\\", (char)('A'+i) );
      uDriveType = GetDriveType( buf );
      if ( uDriveType == DRIVE_CDROM )
	{
	  GetDriveInformation( i, &sptiglobal.drive[i] );
	  if ( sptiglobal.drive[i].bUsed )
	    retVal++;
	}
    }

  sptiglobal.numAdapters = SPTIGetNumAdapters( );

  bSCSIPTInit = TRUE;
  if ( retVal > 0 )
    bUsingSCSIPT = TRUE;
  return retVal;
}


int DeinitSCSIPT( void )
{
  BYTE i;

  if ( !bSCSIPTInit )
    return 0;

  for( i = 2; i < 26; i++ )
    {
      if ( sptiglobal.drive[i].bUsed )
	{
	  CloseHandle( sptiglobal.drive[i].hDevice );
	}
    }

  sptiglobal.numAdapters = SPTIGetNumAdapters( );

  ZeroMemory( &sptiglobal, sizeof(SPTIGLOBAL) );
  bSCSIPTInit = FALSE;
  return -1;
}


/*
 * Returns the number of "adapters" present. 
 */
BYTE SPTIGetNumAdapters( void )
{
	int i;
	BYTE numAdapters = 0;

	for( i = 0; i < 26; i++ )
    {
		if ( sptiglobal.drive[i].bUsed )
		{
			BYTE num = sptiglobal.drive[i].ha + 1;
			if (num>numAdapters) numAdapters = num;
		}
	}
	return numAdapters;
}


/*
 * Replacement for GetASPI32SupportInfo from wnaspi32.dll
 */
DWORD SPTIGetASPI32SupportInfo( void )
{
  DWORD retVal;

#ifdef _DEBUG_SCSIPT
  dbprintf( "AKRip32: SPTIGetASPI32SupportInfo" );
#endif
  if ( !sptiglobal.numAdapters )
    retVal = (DWORD)(MAKEWORD(0,SS_NO_ADAPTERS));
  else
    retVal = (DWORD)(MAKEWORD(sptiglobal.numAdapters,SS_COMP));

  return retVal;
}

/*
 * Needs to call the appropriate function for the lpsrb->SRB_Cmd specified.
 * Valid types are SC_HA_INQUIRY, SC_GET_DEV_TYPE, SC_EXEC_SCSI_CMD,
 * and SC_RESET_DEV.
 */
DWORD SPTISendASPI32Command( LPSRB lpsrb )
{
  if ( !lpsrb )
    return SS_ERR;

  switch( lpsrb->SRB_Cmd )
    {
    case SC_HA_INQUIRY:
      return SPTIHandleHaInquiry( (LPSRB_HAInquiry)lpsrb );

    case SC_GET_DEV_TYPE:
      return SPTIGetDeviceType( (LPSRB_GDEVBlock)lpsrb );

    case SC_EXEC_SCSI_CMD:
      return SPTIExecSCSICommand( (LPSRB_ExecSCSICmd)lpsrb, FALSE );

    case SC_RESET_DEV:
    default:
      lpsrb->SRB_Status = SS_ERR;
      return SS_ERR;
    }

  return SS_ERR;  // should never get to here...
}


/*
 * Universal function to get a file handle to the CD device.  Since
 * NT 4.0 wants just the GENERIC_READ flag, and Win2K wants both
 * GENERIC_READ and GENERIC_WRITE (why a read-only CD device needs
 * GENERIC_WRITE access is beyond me...), the easist workaround is to just
 * try them both.
 */
HANDLE GetFileHandle( BYTE i )
{
  TCHAR buf[12];
  HANDLE fh;
  OSVERSIONINFO osver;
  DWORD dwFlags;

  ZeroMemory( &osver, sizeof(osver) );
  osver.dwOSVersionInfoSize = sizeof(osver);
  GetVersionEx( &osver );

  // if Win2K or greater, add GENERIC_WRITE
  dwFlags = GENERIC_READ;
  if ( (osver.dwPlatformId == VER_PLATFORM_WIN32_NT) &&
       (osver.dwMajorVersion > 4) )
    {
      dwFlags |= GENERIC_WRITE;
#ifdef _DEBUG_SCSIPT
      dbprintf( "AKRip32: SCSIPT: GetFileHandle(): Setting for Win2K" );
#endif
    }

  wsprintf( buf, TEXT("\\\\.\\%c:"), (char)('A'+i) );
  fh = CreateFile( buf, dwFlags, FILE_SHARE_READ, NULL,
		   OPEN_EXISTING, 0, NULL );

  if ( fh == INVALID_HANDLE_VALUE )
    {
      // it went foobar somewhere, so try it with the GENERIC_WRITE 
      // bit flipped
      dwFlags ^= GENERIC_WRITE;
      fh = CreateFile( buf, dwFlags, FILE_SHARE_READ, NULL,
		       OPEN_EXISTING, 0, NULL );
    }

#ifdef _DEBUG_SCSIPT
  if ( fh == INVALID_HANDLE_VALUE )
    dbprintf( "akrip32: scsipt: CreateFile() failed! -> %d", GetLastError() );
  else
    dbprintf( "akrip32: scsipt: CreateFile() returned %d", GetLastError() );
#endif

  return fh;
}



/*
 * fills in a pDrive structure with information from a SCSI_INQUIRY
 * and obtains the ha:tgt:lun values via IOCTL_SCSI_GET_ADDRESS
 */
void GetDriveInformation( BYTE i, DRIVE *pDrive )
{
  HANDLE fh;
  char buf[1024];
  BOOLEAN status;
  PSCSI_PASS_THROUGH_DIRECT_WITH_BUFFER pswb;
  PSCSI_ADDRESS pscsiAddr;
  ULONG length, returned;
  BYTE inqData[100];

#ifdef _DEBUG_SCSIPT
  dbprintf( "AKRip32: SCSIPT: Checking drive %c:", 'A'+i );
#endif

  fh = GetFileHandle( i );

  if ( fh == INVALID_HANDLE_VALUE )
    {
#ifdef _DEBUG_SCSIPT
      dbprintf( "       : fh == INVALID_HANDLE_VALUE" );
#endif
      return;
    }

#ifdef _DEBUG_SCSIPT
  dbprintf( "       : Index %d: fh == %08X", i, fh );
#endif


  /*
   * Get the drive inquiry data
   */
  ZeroMemory( &buf, 1024 );
  ZeroMemory( inqData, 100 );
  pswb                      = (PSCSI_PASS_THROUGH_DIRECT_WITH_BUFFER)buf;
  pswb->spt.Length          = sizeof(SCSI_PASS_THROUGH);
  pswb->spt.CdbLength       = 6;
  pswb->spt.SenseInfoLength = 24;
  pswb->spt.DataIn          = SCSI_IOCTL_DATA_IN;
  pswb->spt.DataTransferLength = 100;
  pswb->spt.TimeOutValue    = 5;
  pswb->spt.DataBuffer      = inqData;
  pswb->spt.SenseInfoOffset = offsetof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER,ucSenseBuf );
  pswb->spt.Cdb[0]          = 0x12;
  pswb->spt.Cdb[4]          = 100;

  length = sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER);
  status = DeviceIoControl( fh,
			    IOCTL_SCSI_PASS_THROUGH_DIRECT,
			    pswb,
			    length,
			    pswb,
			    length,
			    &returned,
			    NULL );

  if ( !status )
    {
      CloseHandle( fh );
#ifdef _DEBUG_SCSIPT
      dbprintf( "AKRip32: SCSIPT: Error DeviceIoControl() -> %d",
		GetLastError() );
#endif
      return;
    }

  memcpy( pDrive->inqData, inqData, 36 );

  /*
   * get the address (path/tgt/lun) of the drive via IOCTL_SCSI_GET_ADDRESS
   */
  ZeroMemory( &buf, 1024 );
  pscsiAddr = (PSCSI_ADDRESS)buf;
  pscsiAddr->Length = sizeof(SCSI_ADDRESS);
  if ( DeviceIoControl( fh, IOCTL_SCSI_GET_ADDRESS, NULL, 0,
			pscsiAddr, sizeof(SCSI_ADDRESS), &returned,
			NULL ) )
    {
#ifdef _DEBUG_SCSIPT
      dbprintf( "Device %c: Port=%d, PathId=%d, TargetId=%d, Lun=%d",
		(char)i+'A', pscsiAddr->PortNumber, pscsiAddr->PathId,
		pscsiAddr->TargetId, pscsiAddr->Lun );
#endif
      pDrive->bUsed     = TRUE;
      pDrive->ha        = pscsiAddr->PortNumber;
      pDrive->tgt       = pscsiAddr->TargetId;
      pDrive->lun       = pscsiAddr->Lun;
      pDrive->driveLetter = i;
      pDrive->hDevice   = INVALID_HANDLE_VALUE;
    }
  else
    {
      pDrive->bUsed     = FALSE;
#ifdef _DEBUG_SCSIPT
      dbprintf( "AKRip32: SPTI: Device %s: Error DeviceIoControl(): %d", (char)i+'A', GetLastError() );
#endif
      return;
    }

#ifdef _DEBUG_SCSIPT
  dbprintf( "AKRip32: SPTI: Adding drive %c: (%d:%d:%d)", 'A'+i,
	    pDrive->ha, pDrive->tgt, pDrive->lun );
#endif

  CloseHandle( fh );
}



DWORD SPTIHandleHaInquiry( LPSRB_HAInquiry lpsrb )
{
  DWORD *pMTL;

  lpsrb->HA_Count    = sptiglobal.numAdapters;
  if ( lpsrb->SRB_HaID >= sptiglobal.numAdapters )
    {
      lpsrb->SRB_Status = SS_INVALID_HA;
      return SS_INVALID_HA;
    }
  lpsrb->HA_SCSI_ID  = 7;  // who cares... we're not really an ASPI manager
  memcpy( lpsrb->HA_ManagerId,  "AKASPI v0.000001", 16 );
  memcpy( lpsrb->HA_Identifier, "SCSI Adapter    ", 16 );
  lpsrb->HA_Identifier[13] = (char)('0'+lpsrb->SRB_HaID);
  ZeroMemory( lpsrb->HA_Unique, 16 );
  lpsrb->HA_Unique[3] = 8;
  pMTL = (LPDWORD)&lpsrb->HA_Unique[4];
  *pMTL = 64 * 1024;

  lpsrb->SRB_Status = SS_COMP;
  return SS_COMP;
}


/*
 * Scans through the drive array and returns DTYPE_CDROM type for all items
 * found, and DTYPE_UNKNOWN for all others.
 */
DWORD SPTIGetDeviceType( LPSRB_GDEVBlock lpsrb )
{
#ifdef _DEBUG_SCSIPT
  dbprintf( "AKRip32: SPTIGetDeviceType( %d:%d:%d )",lpsrb->SRB_HaID, lpsrb->SRB_Target, lpsrb->SRB_Lun );
#endif

  lpsrb->SRB_Status = SS_NO_DEVICE;
  if ( SPTIGetDeviceIndex( lpsrb->SRB_HaID, lpsrb->SRB_Target, lpsrb->SRB_Lun ) )
    lpsrb->SRB_Status = SS_COMP;

  if ( lpsrb->SRB_Status == SS_COMP )
    lpsrb->SRB_DeviceType = DTYPE_CDROM;
  else
    lpsrb->SRB_DeviceType = DTYPE_UNKNOWN;

  return lpsrb->SRB_Status;
}


/*
 * Looks up the index in the drive array for a given ha:tgt:lun triple
 */
BYTE SPTIGetDeviceIndex( BYTE ha, BYTE tgt, BYTE lun )
{
  BYTE i;

#ifdef _DEBUG_SCSIPT
  dbprintf( "AKRip32: SPTIGetDeviceIndex" );
#endif

  for( i = 2; i < 26; i++ )
    {
      if ( sptiglobal.drive[i].bUsed )
	{
	  DRIVE *lpd;
	  lpd = &sptiglobal.drive[i];
	  if ( (lpd->ha == ha) && (lpd->tgt == tgt) && (lpd->lun == lun) )
	    return i;
	}
    }

  return 0;
}

/*
 * Converts ASPI-style SRB to SCSI Pass Through IOCTL
 */
DWORD SPTIExecSCSICommand( LPSRB_ExecSCSICmd lpsrb, BOOL bBeenHereBefore )
{
  BOOLEAN status;
  SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER swb;
  ULONG length, returned;
  //BYTE i;
  BYTE idx;

  idx = SPTIGetDeviceIndex( lpsrb->SRB_HaID, lpsrb->SRB_Target, lpsrb->SRB_Lun );

  if ( idx == 0 )
    {
      lpsrb->SRB_Status = SS_ERR;
      return SS_ERR;
    }

  if ( lpsrb->CDBByte[0] == 0x12 ) // is it an INQUIRY?
    {
      lpsrb->SRB_Status = SS_COMP;
      memcpy( lpsrb->SRB_BufPointer, sptiglobal.drive[idx].inqData, 36 );
      return SS_COMP;
    }

  if ( sptiglobal.drive[idx].hDevice == INVALID_HANDLE_VALUE )
    sptiglobal.drive[idx].hDevice = GetFileHandle( sptiglobal.drive[idx].driveLetter );

  ZeroMemory( &swb, sizeof(swb) );
  swb.spt.Length            = sizeof(SCSI_PASS_THROUGH);
  swb.spt.CdbLength         = lpsrb->SRB_CDBLen;
  if ( lpsrb->SRB_Flags & SRB_DIR_IN )
    swb.spt.DataIn          = SCSI_IOCTL_DATA_IN;
  else if ( lpsrb->SRB_Flags & SRB_DIR_OUT )
    swb.spt.DataIn          = SCSI_IOCTL_DATA_OUT;
  else
    swb.spt.DataIn          = SCSI_IOCTL_DATA_UNSPECIFIED;
  swb.spt.DataTransferLength = lpsrb->SRB_BufLen;
  swb.spt.TimeOutValue      = 5;
  swb.spt.DataBuffer        = lpsrb->SRB_BufPointer;
  swb.spt.SenseInfoOffset   =
    offsetof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER, ucSenseBuf );
  memcpy( swb.spt.Cdb, lpsrb->CDBByte, lpsrb->SRB_CDBLen );
  length = sizeof(swb);

#ifdef _DEBUG_SCSIPT
  dbprintf( "AKRip32: SPTIExecSCSICmd: calling DeviceIoControl()" );
  dbprintf( "       : cmd == 0x%02X", swb.spt.Cdb[0] );
#endif
  status = DeviceIoControl( sptiglobal.drive[idx].hDevice,
			    IOCTL_SCSI_PASS_THROUGH_DIRECT,
			    &swb,
			    length,
			    &swb,
			    length,
			    &returned,
			    NULL );

  if ( status )
    {
      lpsrb->SRB_Status = SS_COMP;
#ifdef _DEBUG_SCSIPT
      OutputDebugString( "       : SRB_Status == SS_COMP" );
#endif
    }
  else
    {
      DWORD dwErrCode;

      lpsrb->SRB_Status = SS_ERR;
      lpsrb->SRB_TargStat = 0x0004;
      dwErrCode = GetLastError();
#ifdef _DEBUG_SCSIPT
      dbprintf( "       : error == %d   handle == %08X", dwErrCode, sptiglobal.drive[idx].hDevice );
#endif
      /*
       * KLUDGE ALERT! KLUDGE ALERT! KLUDGE ALERT!
       * Whenever a disk changer switches disks, it may render the device
       * handle invalid.  We try to catch these errors here and recover
       * from them.
       */
      if ( !bBeenHereBefore &&
	   ((dwErrCode == ERROR_MEDIA_CHANGED) || (dwErrCode == ERROR_INVALID_HANDLE)) )
	{
	  if ( dwErrCode != ERROR_INVALID_HANDLE )
	    CloseHandle( sptiglobal.drive[idx].hDevice );
	  GetDriveInformation( idx, &sptiglobal.drive[idx] );
#ifdef _DEBUG_SCSIPT
	  dbprintf( "AKRip32: SPTIExecSCSICommand: Retrying after ERROR_MEDIA_CHANGED" );
#endif
	  return SPTIExecSCSICommand( lpsrb, TRUE );
	}
    }

  return lpsrb->SRB_Status;
}



BOOL UsingSCSIPT( void )
{
  return bUsingSCSIPT;
}



/*
 * Calls GetFileHandle for the CD refered to by ha:tgt:lun to open it for
 * use
 */
void SPTIOpenCDHandle( BYTE ha, BYTE tgt, BYTE lun )
{
  BYTE idx;

#ifdef _DEBUG_SCSIPT
  dbprintf( "AKRip32: SPTIOpenCDHandle( %d, %d, %d )", ha, tgt, lun );
#endif

  idx = SPTIGetDeviceIndex( ha, tgt, lun );

  if ( idx && sptiglobal.drive[idx].hDevice == INVALID_HANDLE_VALUE )
    sptiglobal.drive[idx].hDevice = GetFileHandle( sptiglobal.drive[idx].driveLetter );  
}
