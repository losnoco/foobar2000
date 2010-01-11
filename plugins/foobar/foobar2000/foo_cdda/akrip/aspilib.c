/*
 * aspilib.c - Copyright (C) 1999 Jay A. Key
 *
 * Generic routines to access wnaspi32.dll
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
#include "aspilib.h"
#include "scsidefs.h"

#define SHAREDMEMVER 2
typedef struct {
  int iSize;
  int iVer;
  CDHANDLEREC cdHandles[MAXCDHAND];
  HANDLE cdMutex[MAXCDHAND];
  int nextHandle;
} AKSHAREDMEM, FAR *LPAKSHAREDMEM;

DWORD deinitREAD10( HCDROM hCD );
DWORD initREAD10( HCDROM hCD );
DWORD readCDParameters( HCDROM hCD, BOOL bChangeMask );
DWORD setCDSpeed( HCDROM hCD, DWORD speed );
DWORD pauseResumeCD( HCDROM hCD, BOOL bPause );
DWORD startStopUnit( HCDROM hCD, BOOL bLoEj, BOOL bStart );
DWORD CDDBSum( DWORD n );
int compBuf( BYTE *b1, BYTE *b2, int n );
DWORD testReadCDAudio( HCDROM hCD, LPTRACKBUF t );
DWORD dummyGetASPI32SupportInfo( void );
DWORD dummySendASPI32Command( LPSRB lpsrb );
BOOL dummyGetASPI32Buffer( PASPI32BUFF pbuf );
BOOL dummyFreeASPI32Buffer( PASPI32BUFF pbuf );
BOOL dummyTranslateASPI32Address( PDWORD p1, PDWORD p2 );
DWORD dummyGetASPI32DLLVersion( void );
int InitSCSIPT( void );
int DeinitSCSIPT( void );
DWORD SPTIGetASPI32SupportInfo( void );
DWORD SPTISendASPI32Command( LPSRB lpsrb );

/*
 * static variables
 */
static char *copyrightInfo = "Released under the GNU Lesser General Public License (http://www.fsf.org/).";
static char *aSCSIDevTypes[] = {
    "Direct-access device",
    "Sequential-access device",
    "Printer device",
    "Processor device",
    "Write-once device",
    "CD-ROM device",
    "Scanner device",
    "Optical memory device",
    "Medium changer device",
    "Communication device",
    "ASC IT8 device",
    "ASC IT8 device"
};

#define MAXCDTYPE   8
static CDREADFN aReadFn[] = {
  NULL, readCDAudioLBA_ATAPI, readCDAudioLBA_ATAPI, readCDAudioLBA_READ10,
  readCDAudioLBA_READ10, readCDAudioLBA_D8, readCDAudioLBA_D4,
  readCDAudioLBA_D4, readCDAudioLBA_READ10
};
static HINSTANCE hinstWNASPI32 = NULL;
int alErrCode = 0;
BYTE alAspiErr = 0;
CDHANDLEREC *cdHandles;
HANDLE *cdMutexes;
int *nextHandle = NULL;
HANDLE hCacheMutex = NULL;
static DWORD dwAPI = APIC_NONE;

//CRITICAL_SECTION useDbbuf;
//char dbbuf[512];

DWORD g_testvar;
DWORD (*pfnGetASPI32SupportInfo)(void);
DWORD (*pfnSendASPI32Command)(LPSRB);
BOOL  (*pfnGetASPI32Buffer)(PASPI32BUFF);
BOOL  (*pfnFreeASPI32Buffer)(PASPI32BUFF);
BOOL  (*pfnTranslateASPI32Address)(PDWORD,PDWORD);
DWORD (*pfnGetASPI32DLLVersion)(void);
BOOL  aspiLoaded = FALSE;



BOOL AkripTest() {return g_testvar == 0xDEADBEEF;}

/*
 * local prototypes
 */
static char *devType( int i );
static BOOL loadAspi( void );
static void unloadAspi( void );
static BOOL initMutexes( void );
static void deinitMutexes( void );


BOOL AkripInit()
{
	static AKSHAREDMEM cdShared;

	if (!loadAspi()) return FALSE;

	cdHandles = cdShared.cdHandles;
	cdMutexes = cdShared.cdMutex;

	nextHandle = &(cdShared.nextHandle);
	
	if (!initMutexes()) return FALSE;

	g_testvar = 0xDEADBEEF;

	return TRUE;
}

void AkripDeinit()
{
	deinitMutexes();
	unloadAspi();
	g_testvar = 0;
}



static BOOL initMutexes( void )
{
	unsigned i;
	for( i = 0; i < MAXCDHAND; i++ )
    {
		cdMutexes[i] = CreateMutex( NULL, FALSE, 0 );
		if (!cdMutexes[i]) return FALSE;
    }
	hCacheMutex = CreateMutex( NULL, FALSE, 0 );
	return TRUE;
}


static void deinitMutexes( void )
{
	if (cdMutexes)
	{
		int i;
		for( i = 0; i < MAXCDHAND; i++ )
		{
			if ( cdMutexes[i] )
			{
				CloseHandle( cdMutexes[i] );
				cdMutexes[i] = 0;
			}
		}
	}

	if ( hCacheMutex )
	{
		CloseHandle( hCacheMutex );
		hCacheMutex = 0;
	}
}


static char *devType( int i )
{ 
  if ( i >= 0x20 )
    return "";

  if ( i == 0x1F )
    return "Unknown or no device";

  if ( i >= 0x0C )
    return "";

  return aSCSIDevTypes[i];
}


/*
 * Assumes that loadAspi has already been called
 */
int  getSCSIDevType( BYTE bHostAdapter, BYTE bTarget, BYTE bLUN,
		     LPBYTE pDevType, LPSTR lpDevTypeStr, int iDevTypeLen )
{
  SRB_GDEVBlock s;
  DWORD dwStatus;

  memset( &s, 0, sizeof(SRB_GDEVBlock) );
  s.SRB_Cmd = SC_GET_DEV_TYPE;
  s.SRB_HaID = bHostAdapter;
  s.SRB_Target = bTarget;
  s.SRB_Lun = bLUN;

  //DebugBreak();
  dwStatus = pfnSendASPI32Command( (LPSRB)&s );
  switch( dwStatus )
    {
    case SS_COMP:
#ifdef AKRIP_DEBUG
      dbprintf( "akrip32: getSCSIDevType() -> SS_COMP: %04X",
		s.SRB_DeviceType );
      dbprintf( "akrip32: getSCSIDevType(): pDevType == 0x%08X", pDevType );
#endif
      if ( pDevType )
	*pDevType = s.SRB_DeviceType;
      if ( lpDevTypeStr )
	strncpy( lpDevTypeStr, devType( s.SRB_DeviceType ), iDevTypeLen );
      break;

    case SS_NO_DEVICE:
#ifdef AKRIP_DEBUG
      dbprintf( "akrip32: getSCSIDevType() -> SS_NO_DEVICE" );
#endif
      if ( pDevType )
	*pDevType = 0x1F;
      if ( lpDevTypeStr )
	strncpy( lpDevTypeStr, "SS_NO_DEVICE", iDevTypeLen );
      return FALSE;

    default:
#ifdef AKRIP_DEBUG
      dbprintf( "akrip32: getSCSIDevType(): unexpected return from ASPI (%04X)", dwStatus );
#endif
      if ( pDevType )
	*pDevType = 0x1F;
      if ( lpDevTypeStr )
	strncpy( lpDevTypeStr, "Unknown return", iDevTypeLen );
      return FALSE;
    }

  return TRUE;
}


DWORD dummyGetASPI32SupportInfo( void )
{
  return SS_NO_ASPI;
}

DWORD dummySendASPI32Command( LPSRB lpsrb )
{
  lpsrb->SRB_Status = SS_NO_ASPI;
  return SS_NO_ASPI;
}


BOOL dummyGetASPI32Buffer( PASPI32BUFF pbuf )
{
  return FALSE;
}


BOOL dummyFreeASPI32Buffer( PASPI32BUFF pbuf )
{
  return FALSE;
}


BOOL dummyTranslateASPI32Address( PDWORD p1, PDWORD p2 )
{
  return FALSE;
}


DWORD dummyGetASPI32DLLVersion( void )
{
  return 0;
}


/***************************************************************************
 * int loadAspi( void )
 ***************************************************************************/
static BOOL loadAspi( void )
{
  DWORD dwErr;
#if 1
  OSVERSIONINFO ver;
  memset(&ver,0,sizeof(ver));
  ver.dwOSVersionInfoSize = sizeof(ver);
  GetVersionEx(&ver);

  if (ver.dwPlatformId == VER_PLATFORM_WIN32_NT)
	  hinstWNASPI32 = 0;
	else
#endif
	  hinstWNASPI32 = LoadLibrary( "WNASPI32.DLL" );

  if ( hinstWNASPI32 )
    {
#ifdef AKRIP_DEBUG
	  dbprintf("wnaspi32.dll loaded");
#endif
      pfnGetASPI32SupportInfo =
	(DWORD(*)(void))GetProcAddress( hinstWNASPI32, "GetASPI32SupportInfo" );
      pfnSendASPI32Command =
	(DWORD(*)(LPSRB))GetProcAddress( hinstWNASPI32, "SendASPI32Command" );
      pfnGetASPI32Buffer =
	(BOOL(*)(PASPI32BUFF))GetProcAddress( hinstWNASPI32, "GetASPI32Buffer" );
      pfnFreeASPI32Buffer =
	(BOOL(*)(PASPI32BUFF))GetProcAddress( hinstWNASPI32, "FreeASPI32Buffer" );
      pfnTranslateASPI32Address =
	(BOOL(*)(PDWORD,PDWORD))GetProcAddress( hinstWNASPI32, "TranslateASPI32Address" );
      pfnGetASPI32DLLVersion =
	(DWORD(*)(void))GetProcAddress( hinstWNASPI32, "GetASPI32DLLVersion" );

      if ( !pfnGetASPI32SupportInfo || !pfnSendASPI32Command )
	{
	  if ( !pfnGetASPI32SupportInfo )
	    alErrCode = ALERR_NOGETASPI32SUPP;
	  else
	    alErrCode = ALERR_NOSENDASPICMD;

#ifdef AKRIP_DEBUG
	  dbprintf( "akrip32: GetASPI32SupportInfo == 0x%08X", 
		   pfnGetASPI32SupportInfo );
	  dbprintf( "akrip32: SendASPI32Command == 0x%08X", pfnSendASPI32Command );
#endif
	  return FALSE;
	}
      dwAPI = APIC_ASPI;
    }
  else if ( InitSCSIPT() )
    {
#ifdef AKRIP_DEBUG
	  dbprintf("using nt scsi");
#endif
      pfnGetASPI32SupportInfo = SPTIGetASPI32SupportInfo;
      pfnSendASPI32Command    = SPTISendASPI32Command;
      dwAPI = APIC_SCSIPT;
    }
  else
    {
      dwErr = GetLastError();

      alErrCode = ALERR_NOWNASPI;
      pfnGetASPI32SupportInfo = dummyGetASPI32SupportInfo;
      pfnSendASPI32Command = dummySendASPI32Command;
      pfnGetASPI32Buffer = dummyGetASPI32Buffer;
      pfnFreeASPI32Buffer = dummyFreeASPI32Buffer;
      pfnTranslateASPI32Address = dummyTranslateASPI32Address;
      pfnGetASPI32DLLVersion = dummyGetASPI32DLLVersion;

#ifdef AKRIP_DEBUG
      dwErr = GetLastError();
      dbprintf( "Unable to load WNASPI32.DLL" );
      dbprintf( "akrip32: GetLastError() -> %d (%04X)", dwErr, dwErr );
#endif
      dwAPI = APIC_NONE;

      return FALSE;
    }

   aspiLoaded = TRUE;
   return TRUE;
}


static void unloadAspi( void )
{
  DeinitSCSIPT();

  if ( hinstWNASPI32 )
    {
      aspiLoaded = FALSE;
      pfnGetASPI32SupportInfo = NULL;
      pfnSendASPI32Command = NULL;
      pfnGetASPI32Buffer = NULL;
      pfnFreeASPI32Buffer = NULL;
      pfnTranslateASPI32Address = NULL;
      pfnGetASPI32DLLVersion = NULL;
      FreeLibrary( hinstWNASPI32 );
      hinstWNASPI32 = NULL;
    }
}

/***************************************************************************
 * GetNumAdapters
 ***************************************************************************/
int GetNumAdapters( void )
{

  DWORD d;
  BYTE bHACount;
  BYTE bASPIStatus;

  d = pfnGetASPI32SupportInfo();
  bASPIStatus = HIBYTE(LOWORD(d));
  bHACount    = LOBYTE(LOWORD(d));

  if ( bASPIStatus != SS_COMP && bASPIStatus != SS_NO_ADAPTERS )
    {
      alErrCode = ALERR_ASPI;
      alAspiErr = bASPIStatus;

#ifdef AKRIP_DEBUG
      dbprintf( "akrip32: GetNumAdapters: bASPIStatus == 0x%02X", bASPIStatus );
#endif
      return -1;
    }
  return (int)bHACount;
}

/***************************************************************************
 * GetDriveInfo
 ***************************************************************************/
DWORD GetDriveInfo( BYTE ha, BYTE tgt, BYTE lun, LPCDREC cdrec )
{
  DWORD dwStatus;
  HANDLE heventSRB;
  SRB_ExecSCSICmd s;
  BYTE buf[100];
  char outBuf[101];
  CDREC cdrecTmp;

  heventSRB = CreateEvent( NULL, TRUE, FALSE, NULL );

  if ( cdrec )
    cdrecTmp = *cdrec;

  memset( &cdrecTmp.info, 0, sizeof(CDINFO) );
  memset( &s, 0, sizeof( s ) );
  memset( buf, 0, 100 );
  s.SRB_Cmd        = SC_EXEC_SCSI_CMD;
  s.SRB_HaID       = ha;
  s.SRB_Target     = tgt;
  s.SRB_Lun        = lun;
  s.SRB_Flags      = SRB_DIR_IN | SRB_EVENT_NOTIFY;
  s.SRB_BufLen     = 100;
  s.SRB_BufPointer = buf;
  s.SRB_SenseLen   = SENSE_LEN;
  s.SRB_CDBLen     = 6;
  s.SRB_PostProc   = (LPVOID)heventSRB;
  s.CDBByte[0]     = SCSI_INQUIRY;
  s.CDBByte[4]     = 100;

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

#ifdef AKRIP_DEBUG
      dbprintf( "akrip32: GetDriveInfo: Error status: 0x%04X\n", s.SRB_Status );
#endif
      return SS_ERR;
    }

  memcpy( cdrecTmp.info.vendor, &buf[8], 8 );
  memcpy( cdrecTmp.info.prodId, &buf[16], 16 );
  memcpy( cdrecTmp.info.rev, &buf[32], 4 );
  memcpy( cdrecTmp.info.vendSpec, &buf[36], 20 );
  wsprintf( outBuf, "%s, %s v%s (%d:%d:%d)", 
	    cdrecTmp.info.vendor, cdrecTmp.info.prodId, cdrecTmp.info.rev,
	    ha, tgt, lun );
#ifdef AKRIP_DEBUG
  dbprintf( "akrip32: GetDriveInfo: %s", outBuf );
#endif

  strncpy( cdrecTmp.id, outBuf, MAXIDLEN );
  cdrecTmp.id[MAXIDLEN] = 0;

  if ( cdrec )
    *cdrec = cdrecTmp;

  return SS_COMP;
}


/***************************************************************************
 * Function Name    ReadTOC
 *
 * 
 ***************************************************************************/
DWORD ReadTOC( HCDROM hCD, LPTOC toc,BOOL bMSF)
{
  DWORD dwStatus;
  DWORD retVal = SS_COMP;
  HANDLE heventSRB;
  SRB_ExecSCSICmd s;
  int idx = (int)hCD - 1;

  if ( (idx<0) || (idx>=MAXCDHAND) )
    {
      alErrCode = ALERR_INVHANDLE;
      return SS_ERR;
    }

  if ( WaitForSingleObject( cdMutexes[idx], TIMEOUT ) != WAIT_OBJECT_0 )
    {
      alErrCode = ALERR_LOCK;
      return SS_ERR;
    }

  if ( !cdHandles[idx].used )
    {
      alErrCode = ALERR_INVHANDLE;
      ReleaseMutex( cdMutexes[idx] );
      return SS_ERR;
    }

  heventSRB = CreateEvent( NULL, TRUE, FALSE, NULL );

  memset( &s, 0, sizeof( s ) );
  s.SRB_Cmd        = SC_EXEC_SCSI_CMD;
  s.SRB_HaID       = cdHandles[idx].ha;
  s.SRB_Target     = cdHandles[idx].tgt;
  s.SRB_Lun        = cdHandles[idx].lun;
  s.SRB_Flags      = SRB_DIR_IN | SRB_EVENT_NOTIFY;
  s.SRB_BufLen     = 0x324;
  s.SRB_BufPointer = (BYTE FAR *)toc;
  s.SRB_SenseLen   = 0x0E;
  s.SRB_CDBLen     = 0x0A;
  s.SRB_PostProc   = (LPVOID)heventSRB;
  s.CDBByte[0]     = 0x43;
  s.CDBByte[1]     = bMSF?0x02:0x00;
  //  s.CDBByte[1] = 0x02;               /* 0x02 == MSF format, 0x00 == LBA */
  s.CDBByte[7]     = 0x03;               /* length of buffer to hold TOC    */
  s.CDBByte[8]     = 0x24;               /*      == 0x324                   */

  ResetEvent( heventSRB );
  dwStatus = pfnSendASPI32Command( (LPSRB)&s );

  if ( dwStatus == SS_PENDING )
    {
      WaitForSingleObject( heventSRB, 4000 );
    }
  CloseHandle( heventSRB );

  if ( s.SRB_Status != SS_COMP )
    {
      alErrCode = ALERR_ASPI;
      alAspiErr = s.SRB_Status;
      retVal = SS_ERR;
    }

#ifdef AKRIP_DEBUG
  {
    FILE *fp = fopen( "toc.dat", "wb" );
    fwrite( toc, 1, sizeof(TOC), fp );
    fclose( fp );
  }
#endif

  ReleaseMutex( cdMutexes[idx] );
  return retVal;
}


/***************************************************************************
 * resetSCSIBus
 * 
 * According to Adaptec, this command can cause problems in Win95
 *
 ***************************************************************************/
void resetSCSIBus( void )
{
  DWORD dwStatus;
  HANDLE heventSRB;
  SRB_BusDeviceReset s;

#ifdef AKRIP_DEBUG
  dbprintf( "akrip32: reset bus!" );
#endif
  heventSRB = CreateEvent( NULL, TRUE, FALSE, NULL );

  memset( &s, 0, sizeof( s ) );
  s.SRB_Cmd = SC_RESET_DEV;
  s.SRB_PostProc = (LPVOID)heventSRB;

  ResetEvent( heventSRB );
  dwStatus = pfnSendASPI32Command( (LPSRB)&s );
  if ( dwStatus == SS_PENDING )
    {
      WaitForSingleObject( heventSRB, DEFWAITLEN );
    }
  CloseHandle( heventSRB );

#ifdef AKRIP_DEBUG
  if ( s.SRB_Status != SS_COMP )
    {
      dbprintf( "akrip32: RESET BUS aspi status: 0x%08X\n", s.SRB_Status );
    }
#endif
}



/******************************************************************
 * GetCDList
 *
 * Scans all host adapters for CD-ROM units, and stores information
 * for all units located
 ******************************************************************/


int GetCDList( LPCDLIST cd )
{
  SRB_HAInquiry sh;
  SRB_GDEVBlock sd;
  int numAdapters, i, j, k;
  int maxTgt;

  /* initialize cd list */
  maxTgt = cd->max;
  memset( cd, 0, sizeof(*cd) );
  cd->max = maxTgt;

  numAdapters = GetNumAdapters();
#ifdef AKRIP_DEBUG
  dbprintf( "AKRip32: GetCDList(): numAdapters == %d", numAdapters );
#endif
  if ( numAdapters == 0 )
    {
      alErrCode = ALERR_NOADAPTERS;
      return 1;
    }

  for( i = 0; i < numAdapters; i++ )
    {
      memset( &sh, 0, sizeof( sh ) );
      sh.SRB_Cmd   = SC_HA_INQUIRY;
      sh.SRB_HaID  = i;
      pfnSendASPI32Command( (LPSRB)&sh );

      /* on error skip to next adapter */
      if ( sh.SRB_Status != SS_COMP )
	continue;

      maxTgt = (int)sh.HA_Unique[3];

      if ( maxTgt == 0 )
	maxTgt = 8;

      for( j = 0; j < maxTgt; j++ )
	{
	  for( k = 0; k < 8; k++ )
	    {
	      memset( &sd, 0, sizeof( sd ) );
	      sd.SRB_Cmd   = SC_GET_DEV_TYPE;
	      sd.SRB_HaID  = i;
	      sd.SRB_Target = j;
	      sd.SRB_Lun   = k;
	      pfnSendASPI32Command( (LPSRB)&sd );
	      if ( sd.SRB_Status == SS_COMP )
		{
		  if ( sd.SRB_DeviceType == DTYPE_CDROM )
		    {
		      cd->cd[cd->num].ha = i;
		      cd->cd[cd->num].tgt = j;
		      cd->cd[cd->num].lun = k;
		      memset( cd->cd[cd->num].id, 0, MAXIDLEN+1 );
		      GetDriveInfo( (BYTE)i, (BYTE)j, (BYTE)k, &(cd->cd[cd->num]) );
		      cd->num++;
		    }
#ifdef AKRIP_DEBUG
		  else
		    {
		      dbprintf( "       : sd.SRB_DeviceType == %d", sd.SRB_DeviceType );
		      GetDriveInfo( (BYTE)i, (BYTE)j, (BYTE)k, NULL );
		    }
#endif
		}
	    }
	}
    }

  if ( cd->num == 0 )
    alErrCode = ALERR_NOCDFOUND;

  return 1;
}


DWORD ReadCDAudioLBA( HCDROM hCD, LPTRACKBUF t )
{
  int idx = (int)hCD - 1;
  DWORD retVal;

  if ( (idx<0) || (idx>=MAXCDHAND) )
    {
      alErrCode = ALERR_INVHANDLE;
      return SS_ERR;
    }

  retVal = WaitForSingleObject( cdMutexes[idx], TIMEOUT );
  if ( retVal != WAIT_OBJECT_0 )
    {
      alErrCode = ALERR_LOCK;
      return SS_ERR;
    }

  if ( !cdHandles[idx].used )
    {
      alErrCode = ALERR_INVHANDLE;
      ReleaseMutex( cdMutexes[idx] );
      return SS_ERR;
    }

  retVal = cdHandles[idx].pfnRead( hCD, t );
  ReleaseMutex( cdMutexes[idx] );

  return retVal;
}


/*
 * Test the currently set read function.  Fills the buffer with 0xAA
 * prior to reading, and then checks the read area for a series of 0xAA
 * bytes.  This is because some read commands do not return an error, but
 * also don't read correct data or a full buffer.
 */
DWORD testReadCDAudio( HCDROM hCD, LPTRACKBUF t )
{
  DWORD i;
  DWORD dwStatus;
  BYTE *p;
  int idx = (int)hCD - 1;
  int count;

  // fill buffer with dummy data
  memset( t->buf, 0xAA, t->len );

  dwStatus = cdHandles[idx].pfnRead( hCD, t );
  if ( dwStatus != SS_COMP )
    {
      return dwStatus;
    }

  p = &(t->buf[t->startOffset]);
  for( i = 0, count = 0; i < t->len; i += 4 )
    {
      if ( *((DWORD *)p) == 0xAAAAAAAA )
	count += 1;
      else
	count = 0;
      if ( count == 8 )
	{
	  return SS_ERR;
	}

      p += 4;
    }

  return dwStatus;
}


/***************************************************************************
 * readCDAudioLBA_ANY
 *
 * Attempts to autodetect the proper read function for the CD.  It tries
 * ATAPI2, ATAPI1, and then finally READ10
 *
 ***************************************************************************/
DWORD readCDAudioLBA_ANY( HCDROM hCD, LPTRACKBUF t )
{
  DWORD dwStatus;
  int idx = (int)hCD - 1;
  int i, j;
  int ord[7] = { 2, 1, 8, 4, 5, 6, 7 };

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

  for( i = 0; i < 7; i++ )
    {
#ifdef AKRIP_DEBUG
      dbprintf( "akrip32: testing read fn %d", ord[i] );
#endif

      cdHandles[idx].readType = ord[i];
      cdHandles[idx].pfnRead = aReadFn[ord[i]];
      cdHandles[idx].pfnDeinit = NULL;
      cdHandles[idx].bInit = FALSE;

      for( j = 0; j < 3; j++ )
	{
	  dwStatus = testReadCDAudio( hCD, t );
	  if ( dwStatus == SS_COMP )
	    {
	      return dwStatus;
	    }
	}
    }


  // Failed to find compatible read mode, so we reset to the default
  // and return the error
  cdHandles[idx].readType = CDR_ANY;
  cdHandles[idx].pfnRead  = readCDAudioLBA_ANY;
  return dwStatus;
}


DWORD GetASPI32SupportInfo( void )
{
  return pfnGetASPI32SupportInfo();
}


DWORD SendASPI32Command( LPSRB s )
{
  return pfnSendASPI32Command( s );
}


#define _GEN_CDPARMS 0
DWORD readCDParameters( HCDROM hCD, BOOL bChangeMask )
{
  HANDLE h;
  SRB_ExecSCSICmd s;
  DWORD d;
  BYTE b[256];
  int lenData;
  BYTE *p;
  BYTE *pMax = b + 256;
  LPSENSEMASK psm;
  int idx = (int)hCD - 1;

#if _GEN_CDPARMS
  FILE *fp;
#endif

  if ( (idx<0) || (idx>=MAXCDHAND) || !cdHandles[idx].used )
    {
      alErrCode = ALERR_INVHANDLE;
      return SS_ERR;
    }

  cdHandles[idx].smRead = FALSE;
  psm = &cdHandles[idx].sm;
  memset( psm, 0, sizeof( SENSEMASK ) );


  h = CreateEvent( NULL, TRUE, FALSE, NULL );
  memset( &s, 0, sizeof( s ) );
  memset( b, 0xFF, 256 );
  s.SRB_Cmd      = SC_EXEC_SCSI_CMD;
  s.SRB_HaID     = cdHandles[idx].ha;
  s.SRB_Target   = cdHandles[idx].tgt;
  s.SRB_Lun      = cdHandles[idx].lun;
  s.SRB_Flags    = SRB_DIR_IN | SRB_EVENT_NOTIFY;
  s.SRB_BufLen   = 256;
  s.SRB_BufPointer = b;
  s.SRB_CDBLen   = 12;
  s.SRB_PostProc = (LPVOID)h;
  s.CDBByte[0]   = 0x5A;
  s.CDBByte[2]   = 0x3F;
  s.CDBByte[7]   = 0x01;
  s.CDBByte[8]   = 0x00;

  /* do we want just a mask of changable items? */
  if ( bChangeMask )
    s.CDBByte[2] |= 0x40;

  ResetEvent( h );
  d = pfnSendASPI32Command( (LPSRB)&s );
  if ( d == SS_PENDING )
    {
      WaitForSingleObject( h, 500 );
    }
  CloseHandle( h );

  if ( s.SRB_Status != SS_COMP )
    {
      alErrCode = ALERR_ASPI;
      alAspiErr = s.SRB_Status;
      ReleaseMutex( cdMutexes[idx] );
      return SS_ERR;
    }

#if _GEN_CDPARMS
  fp = fopen( "cdparms.dat", "wb" );
  if ( fp )
    {
      fwrite( b, 1, 256, fp );
      fclose( fp );
    }
#endif

  lenData = ((unsigned int)b[0] << 8) + b[1];

  /* set to first sense mask, and then walk through the masks */
  p = b + 8;
  while( (p < &(b[2+lenData])) && (p < pMax) )
    {
      BYTE which;

      which = p[0] & 0x3F;
      switch( which )
	{
	case READERRREC:
	  psm->rerAvail = TRUE;
	  memcpy( psm->rer, p, p[1]+2 );
	  break;
	case CDRPARMS:
	  psm->cpmAvail = TRUE;
	  memcpy( psm->cpm, p, p[1]+2 );
	  break;
	case CDRAUDIOCTL:
	  psm->cacmAvail = TRUE;
	  memcpy( psm->cacm, p, p[1]+2 );
	  break;
	case CDRCAPS:
	  psm->ccsmAvail = TRUE;
	  memcpy( psm->ccsm, p, p[1]+2 );
	  break;
	}
      p += (p[1] + 2);
    }

  cdHandles[idx].smRead = TRUE;
  return s.SRB_Status;
}


/****************************************************************
 * Generic function to query CD unit capabilities and parameters
 *
 * which:   specifies the parm we wish to query
 * pNum:    pointer to DWORD to return data
 *
 * returns: If the parm is not available, returns FALSE.  Otherwise
 *          returns TRUE.
 *
 * The data requested will either be returned as a BOOL, or copied
 * to pNum, depending on the parameter requested.
 ****************************************************************/
BOOL QueryCDParms( HCDROM hCD, int which, DWORD *pNum )
{
  BOOL retVal = FALSE;
  DWORD dwTmp;
  int idx = (int)hCD - 1;

  if ( (idx<0) || (idx>=MAXCDHAND) || !cdHandles[idx].used )
    {
      alErrCode = ALERR_INVHANDLE;
      return FALSE;
    }

  if ( WaitForSingleObject( cdMutexes[idx], TIMEOUT ) != WAIT_OBJECT_0 )
    {
      alErrCode = ALERR_LOCK;
      return SS_ERR;
    }

  if ( pNum )
    *pNum = 0;
  else
    pNum = &dwTmp;

  if ( !cdHandles[idx].smRead )
    {
      if ( readCDParameters( hCD, FALSE ) != SS_COMP )
	{
	  ReleaseMutex( cdMutexes[idx] );
	  return FALSE;
	}
    }

  switch( which )
    {
    case CDP_READCDR:
      retVal = cdHandles[idx].sm.ccsmAvail && (cdHandles[idx].sm.ccsm[2] & 0x01);
      break;

    case CDP_READCDE:
      retVal = cdHandles[idx].sm.ccsmAvail && (cdHandles[idx].sm.ccsm[2] & 0x02);
      break;

    case CDP_METHOD2:
      retVal = cdHandles[idx].sm.ccsmAvail && (cdHandles[idx].sm.ccsm[2] & 0x04);
      break;

    case CDP_WRITECDR:
      retVal = cdHandles[idx].sm.ccsmAvail && (cdHandles[idx].sm.ccsm[3] & 0x01);
      break;

    case CDP_WRITECDE:
      retVal = cdHandles[idx].sm.ccsmAvail && (cdHandles[idx].sm.ccsm[3] & 0x02);
      break;

    case CDP_AUDIOPLAY:
      retVal = cdHandles[idx].sm.ccsmAvail && (cdHandles[idx].sm.ccsm[4] & 0x01);
      break;

    case CDP_COMPOSITE:
      retVal = cdHandles[idx].sm.ccsmAvail && (cdHandles[idx].sm.ccsm[4] & 0x02);
      break;

    case CDP_DIGITAL1:
      retVal = cdHandles[idx].sm.ccsmAvail && (cdHandles[idx].sm.ccsm[4] & 0x04);
      break;

    case CDP_DIGITAL2:
      retVal = cdHandles[idx].sm.ccsmAvail && (cdHandles[idx].sm.ccsm[4] & 0x08);
      break;

    case CDP_M2FORM1:
      retVal = cdHandles[idx].sm.ccsmAvail && (cdHandles[idx].sm.ccsm[4] & 0x10);
      break;

    case CDP_M2FORM2:
      retVal = cdHandles[idx].sm.ccsmAvail && (cdHandles[idx].sm.ccsm[4] & 0x20);
      break;

    case CDP_MULTISES:
      retVal = cdHandles[idx].sm.ccsmAvail && (cdHandles[idx].sm.ccsm[4] & 0x40);
      break;

    case CDP_CDDA:
      retVal = cdHandles[idx].sm.ccsmAvail && (cdHandles[idx].sm.ccsm[5] & 0x01);
      break;

    case CDP_RW:
      retVal = cdHandles[idx].sm.ccsmAvail && (cdHandles[idx].sm.ccsm[5] & 0x04);
      break;

    case CDP_RWCORR:
      retVal = cdHandles[idx].sm.ccsmAvail && (cdHandles[idx].sm.ccsm[5] & 0x08);
      break;

    case CDP_C2SUPP:
      retVal = cdHandles[idx].sm.ccsmAvail && (cdHandles[idx].sm.ccsm[5] & 0x10);
      break;

    case CDP_ISRC:
      retVal = cdHandles[idx].sm.ccsmAvail && (cdHandles[idx].sm.ccsm[5] & 0x20);
      break;

    case CDP_UPC:
      retVal = cdHandles[idx].sm.ccsmAvail && (cdHandles[idx].sm.ccsm[5] & 0x40);
      break;

    case CDP_CANLOCK:
      retVal = cdHandles[idx].sm.ccsmAvail && (cdHandles[idx].sm.ccsm[6] & 0x01);
      break;

    case CDP_LOCKED:
      retVal = cdHandles[idx].sm.ccsmAvail && (cdHandles[idx].sm.ccsm[6] & 0x02);
      break;

    case CDP_PREVJUMP:
      retVal = cdHandles[idx].sm.ccsmAvail && (cdHandles[idx].sm.ccsm[6] & 0x04);
      break;

    case CDP_CANEJECT:
      retVal = cdHandles[idx].sm.ccsmAvail && (cdHandles[idx].sm.ccsm[6] & 0x08);
      break;

    case CDP_SEPVOL:
      retVal = cdHandles[idx].sm.ccsmAvail && (cdHandles[idx].sm.ccsm[7] & 0x01);
      break;

    case CDP_SEPMUTE:
      retVal = cdHandles[idx].sm.ccsmAvail && (cdHandles[idx].sm.ccsm[7] & 0x02);
      break;

    case CDP_SDP:
      retVal = cdHandles[idx].sm.ccsmAvail && (cdHandles[idx].sm.ccsm[7] & 0x04);
      break;

    case CDP_SSS:
      retVal = cdHandles[idx].sm.ccsmAvail && (cdHandles[idx].sm.ccsm[7] & 0x08);
      break;

    case CDP_MECHTYPE:
      if ( cdHandles[idx].sm.ccsmAvail )
	{
	  retVal = TRUE;
	  *pNum = (DWORD)(cdHandles[idx].sm.ccsm[6] >> 5);
	}
      break;

    case CDP_STREAMACC:
      return cdHandles[idx].sm.ccsmAvail && (cdHandles[idx].sm.ccsm[5] & 0x02);
      break;

    case CDP_MAXSPEED:
      if ( cdHandles[idx].sm.ccsmAvail )
	{
	  retVal = TRUE;
	  *pNum = ((DWORD)cdHandles[idx].sm.ccsm[8] << 8) + (DWORD)cdHandles[idx].sm.ccsm[9];
	}
      break;

    case CDP_NUMVOL:
      if ( cdHandles[idx].sm.ccsmAvail )
	{
	  retVal = TRUE;
	  *pNum = ((DWORD)cdHandles[idx].sm.ccsm[10] << 8) + (DWORD)cdHandles[idx].sm.ccsm[11];
	}
      break;

    case CDP_BUFSIZE:
      if ( cdHandles[idx].sm.ccsmAvail )
	{
	  retVal = TRUE;
	  *pNum = ((DWORD)cdHandles[idx].sm.ccsm[12] << 8) + (DWORD)cdHandles[idx].sm.ccsm[13];
	}
      break;

    case CDP_CURRSPEED:
      if ( cdHandles[idx].sm.ccsmAvail )
	{
	  retVal = TRUE;
	  *pNum = ((DWORD)cdHandles[idx].sm.ccsm[14] << 8) + (DWORD)cdHandles[idx].sm.ccsm[15];
	}
      break;

    case CDP_SPM:
      if ( cdHandles[idx].sm.cpmAvail )
	{
	  retVal = TRUE;
	  *pNum = ((DWORD)cdHandles[idx].sm.cpm[4] << 8) + (DWORD)cdHandles[idx].sm.cpm[5];
	}
      break;

    case CDP_FPS:
      if ( cdHandles[idx].sm.cpmAvail )
	{
	  retVal = TRUE;
	  *pNum = ((DWORD)cdHandles[idx].sm.cpm[6] << 8) + (DWORD)cdHandles[idx].sm.cpm[7];
	}
      break;

    case CDP_INACTMULT:
      if ( cdHandles[idx].sm.cpmAvail )
	{
	  retVal = TRUE;
	  *pNum = (DWORD)(cdHandles[idx].sm.cpm[3] & 0x0F);
	}
      break;

    case CDP_JITTER:
      retVal = TRUE;
      *pNum = (DWORD)(cdHandles[idx].numCheck);
      break;

    case CDP_OVERLAP:
      retVal = TRUE;
      *pNum = (DWORD)(cdHandles[idx].numOverlap);
      break;

    case CDP_READMODE:
      retVal = TRUE;
      *pNum = (DWORD)(cdHandles[idx].readMode);
      break;

    default:
      break;
    }

  ReleaseMutex( cdMutexes[idx] );
  return retVal;
}


/*
 * Complement to queryCDParms -- used to set values in the various control
 * pages on the CD drive.
 */
BOOL ModifyCDParms( HCDROM hCD, int which, DWORD val )
{
  //SENSEMASK smask;
  //BOOL smRead = FALSE;
  BOOL retVal = FALSE;
  int idx = (int)hCD - 1;

  if ( (idx<0) || (idx>=MAXCDHAND) || !cdHandles[idx].used )
    {
      alErrCode = ALERR_INVHANDLE;
      return FALSE;
    }

  if ( WaitForSingleObject( cdMutexes[idx], TIMEOUT ) != WAIT_OBJECT_0 )
    {
      alErrCode = ALERR_LOCK;
      return SS_ERR;
    }

#if 0
  if ( readCDParameters( hCD, TRUE ) != SS_COMP )
    {
      ReleaseMutex( cdMutexes[idx] );
      return FALSE;
    }
#endif

  switch( which )
    {
    case CDP_CURRSPEED:
      if ( setCDSpeed( hCD, val ) == SS_COMP )
	retVal = TRUE;
      break;

    case CDP_JITTER:
      cdHandles[idx].numCheck = (int)val;
      retVal = TRUE;
      break;

    case CDP_OVERLAP:
      cdHandles[idx].numOverlap = (int)val;
      retVal = TRUE;
      break;

    case CDP_READMODE:
      cdHandles[idx].readMode = (int)val;
      retVal = TRUE;
      break;
    }

  ReleaseMutex( cdMutexes[idx] );
  return retVal;
}


/*
 * Speed is specified in KB/sec: 1x == 176, 2x == 353, 4x == 706
 *
 * To set to the maximum allowed speed, specify 0xFFFF.  Attempting to set
 * a speed higher than the allowed maximum speed should not cause an error,
 * but should set the speed at the highest allowed value.
 */
DWORD setCDSpeed( HCDROM hCD, DWORD speed )
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

  heventSRB = CreateEvent( NULL, TRUE, FALSE, NULL );

  memset( &s, 0, sizeof( s ) );

  s.SRB_Cmd      = SC_EXEC_SCSI_CMD;
  s.SRB_HaID     = cdHandles[idx].ha;
  s.SRB_Target   = cdHandles[idx].tgt;
  s.SRB_Lun      = cdHandles[idx].lun;
  s.SRB_Flags    = SRB_DIR_OUT | SRB_EVENT_NOTIFY;
  s.SRB_SenseLen = SENSE_LEN;
  s.SRB_CDBLen   = 12;
  s.SRB_PostProc = (LPVOID)heventSRB;
  s.CDBByte[0]   = 0xBB;
  s.CDBByte[2]   = (BYTE)(speed >> 8);
  s.CDBByte[3]   = (BYTE)speed;

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


DWORD pauseResumeCD( HCDROM hCD, BOOL bPause )
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

  if ( WaitForSingleObject( cdMutexes[idx], TIMEOUT ) != WAIT_OBJECT_0 )
    {
      alErrCode = ALERR_LOCK;
      return SS_ERR;
    }

  heventSRB = CreateEvent( NULL, TRUE, FALSE, NULL );

  memset( &s, 0, sizeof( s ) );

  s.SRB_Cmd      = SC_EXEC_SCSI_CMD;
  s.SRB_HaID     = cdHandles[idx].ha;
  s.SRB_Target   = cdHandles[idx].tgt;
  s.SRB_Lun      = cdHandles[idx].lun;
  s.SRB_Flags    = SRB_EVENT_NOTIFY;
  s.SRB_SenseLen = SENSE_LEN;
  s.SRB_CDBLen   = 10;
  s.SRB_PostProc = (LPVOID)heventSRB;
  s.CDBByte[0]   = 0x4B;
  s.CDBByte[8]   = bPause?0:1;

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
      ReleaseMutex( cdMutexes[idx] );
      return SS_ERR;
    }

  ReleaseMutex( cdMutexes[idx] );
  return s.SRB_Status;
}


DWORD startStopUnit( HCDROM hCD, BOOL bLoEj, BOOL bStart )
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
 
  if ( WaitForSingleObject( cdMutexes[idx], TIMEOUT ) != WAIT_OBJECT_0 )
    {
      alErrCode = ALERR_LOCK;
      return SS_ERR;
    }

  heventSRB = CreateEvent( NULL, TRUE, FALSE, NULL );

  memset( &s, 0, sizeof( s ) );

  s.SRB_Cmd      = SC_EXEC_SCSI_CMD;
  s.SRB_HaID     = cdHandles[idx].ha;
  s.SRB_Target   = cdHandles[idx].tgt;
  s.SRB_Lun      = cdHandles[idx].lun;
  s.SRB_Flags    = SRB_EVENT_NOTIFY;
  s.SRB_SenseLen = SENSE_LEN;
  s.SRB_CDBLen   = 6;
  s.SRB_PostProc = (LPVOID)heventSRB;
  s.CDBByte[0]   = 0x1B;
  s.CDBByte[4]  |= bLoEj?0x02:0x00;
  s.CDBByte[4]  |= bStart?0x01:0x00;

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
      ReleaseMutex( cdMutexes[idx] );
      return SS_ERR;
    }

  ReleaseMutex( cdMutexes[idx] );
  return s.SRB_Status;
}


/****************************************************************
 * GetAspiLibError
 *
 * Used after functions return SS_ERR to get more specific error
 * information.  If it returns AL_ERRASPI, call getAspiLibAspiError
 * to get the actual status code returned by the calls to the ASPI
 * manager.  After reading, the error code is cleared.
 *
 ****************************************************************/
int GetAspiLibError( void )
{
  int retVal;

  retVal = alErrCode;
  alErrCode = ALERR_NOERROR;

  return retVal;
}


/****************************************************************
 * GetAspiLibAspiError
 *
 * Returns the last error completion code from the ASPI manager.
 * After reading, the error code is cleared.
 *
 ****************************************************************/
BYTE GetAspiLibAspiError( void )
{
  BYTE retVal;

  retVal = alAspiErr;
  alAspiErr = SS_COMP;

  return retVal;
}


/****************************************************************
 * GetCDId
 *
 * Generates an identifier string for the CD drive identified by
 * hCD
 *
 ****************************************************************/
DWORD  GetCDId( HCDROM hCD, char *buf, int maxBuf )
{
  int idx = (int)hCD - 1;
  CDREC cd;

  if ( (idx<0) || (idx>=MAXCDHAND) || !cdHandles[idx].used )
    {
      alErrCode = ALERR_INVHANDLE;
      return SS_ERR;
    }

  if ( !buf )
    {
      alErrCode = ALERR_BUFPTR;
      return SS_ERR;
    }

  if ( WaitForSingleObject( cdMutexes[idx], TIMEOUT ) != WAIT_OBJECT_0 )
    {
      alErrCode = ALERR_LOCK;
      return SS_ERR;
    }

  memset( &cd, 0, sizeof(cd) );
  GetDriveInfo( cdHandles[idx].ha, cdHandles[idx].tgt, cdHandles[idx].lun,
		&cd );
  if ( strlen( cd.id ) > (size_t)(maxBuf-1) )
    {
      alErrCode = ALERR_BUFTOOSMALL;
      strncpy( buf, cd.id, maxBuf );
      buf[maxBuf-1] = '\0';
      ReleaseMutex( cdMutexes[idx] );
      return SS_ERR;
    }
  else
    strcpy( buf, cd.id );

  ReleaseMutex( cdMutexes[idx] );
  return SS_COMP;
}


BOOL allZeros( LPTRACKBUF t )
{
  DWORD i;
  unsigned char *p = &t->buf[t->startOffset]; 

  for( i = 0; i < t->len; i++ )
    if ( p[i] != 0 )
      return FALSE;

  return TRUE;
}


/*
 * Returns TRUE if the overlap buffer is located in the track buffer.  The
 * track buffer startOffset is adjusted to reflect where in the buffer the
 * aligned data begins.  numFrames and len are also adjusted to reflect the
 * number of complete frames are contained in the buffer.
 */
int jitterAdjust( LPTRACKBUF tbuf, LPTRACKBUF tover, int checkFrames )
{
  int i;
  int max;
  int bFound = 0;
  BYTE *p;
  int checkLen = checkFrames * 2352;

  max = tbuf->len - checkLen;

  if ( tover->len == 0 || allZeros( tover ) )
    {
      return 1;
    }

  p = &tover->buf[tover->startOffset];

  bFound = 0;
  for( i = 0; i < max; i++ )
    {
      if ( compBuf( p, &(tbuf->buf[i]), checkLen ) )
	{
	  tbuf->startOffset = i + checkLen;
	  tbuf->len -= (i + checkLen);
	  tbuf->startFrame = tover->startFrame + checkFrames;
	  i = max + 1;
	  bFound = -1;
	}
    }

  /* adjust frame count and crop length */
  tbuf->numFrames = tbuf->len / 2352;
  tbuf->len = 2352 * tbuf->numFrames;

  //  return (tbuf->startOffset == 0);
  return bFound;
}


/*
 * Reads CD-DA audio, implementing jitter correction.  tOver is used to align
 * the current read, if possible.  After a successful read, numOverlap frames
 * are copied to tOver.
 */
DWORD ReadCDAudioLBAEx( HCDROM hCD, LPTRACKBUF t, LPTRACKBUF tOver )
{
  DWORD retVal;
  int idx = (int)hCD - 1;
  int j, o;
  unsigned char *pOverAddr;
  BOOL bJitterCorr, bSaveJitter;

  if ( (idx<0) || (idx>=MAXCDHAND) )
    {
      alErrCode = ALERR_INVHANDLE;
      return SS_ERR;
    }

  retVal = WaitForSingleObject( cdMutexes[idx], TIMEOUT );
  if ( retVal != WAIT_OBJECT_0 )
    {
      alErrCode = ALERR_LOCK;
      return SS_ERR;
    }

  if ( !cdHandles[idx].used )
    {
      alErrCode = ALERR_INVHANDLE;
      ReleaseMutex( cdMutexes[idx] );
      return SS_ERR;
    }

  j = cdHandles[idx].numCheck;
  o = cdHandles[idx].numOverlap;

  bSaveJitter = ((tOver->maxLen / 2352) >= (DWORD)j);
  bJitterCorr = bSaveJitter && !allZeros( tOver );

  // adjust the starting address of the read if necessary
  if ( bJitterCorr && tOver->startFrame && tOver->numFrames && tOver->len )
    {
      DWORD numFrames;

      numFrames = t->numFrames;

      // is this tOver from the last sequential read?
      if ( tOver->startFrame == (t->startFrame - j) )
	{
	  t->startFrame -= o;
	  // detect situation when trying to read less than the overlap buffer
	  if ( (numFrames <= (DWORD)o) && ((numFrames + o) <= (t->maxLen/2352)))
	    {
	      t->numFrames += o;
	    }
#ifdef AKRIP_DEBUG
	  dbprintf( "rolling back start frame to %d", t->startFrame );
#endif
	}
      else
	{
	  tOver->len = tOver->startFrame = tOver->numFrames = 0;
#ifdef AKRIP_DEBUG
	  dbprintf( "zeroing out overlap buffer" );
#endif
	}
    }

  retVal = cdHandles[idx].pfnRead( hCD, t );

  if ( retVal == SS_COMP )
    {
      if ( bJitterCorr )
	{
	  j = cdHandles[idx].numCheck;
	  if ( !jitterAdjust( t, tOver, j ) )
	    {
#ifdef AKRIP_DEBUG
	      dbprintf( "akrip32: ... jitterAdjust failed!" );
#endif
	      alErrCode = ALERR_JITTER;
	      ReleaseMutex( cdMutexes[idx] );
	      return SS_ERR;
	    }
	}

      if ( bSaveJitter )
	{
	  pOverAddr = &(t->buf[2352*(t->numFrames-j)+t->startOffset]);
	  memcpy( tOver->buf, pOverAddr, 2352*j );

	  tOver->startFrame = t->startFrame + t->numFrames - j;
	  tOver->numFrames = j;
	  tOver->len = 2352 * j;
	  tOver->startOffset = 0;
	}
      else
	{
	  tOver->startFrame = tOver->numFrames = tOver->len = 0;
	}
    }
#ifdef AKRIP_DEBUG
  else
    dbprintf( "akrip32: ReadCDAudioLBAEx: read failed" );
#endif

  ReleaseMutex( cdMutexes[idx] );

#ifdef AKRIP_DEBUG
  dbprintf( "akrip32: readCDAudioLBAEx: returning %04X", retVal );
#endif

  return retVal;
}


/********************************************************************
 * compBuf
 *
 * Compares two buffers up to n bytes, returning 1 if they contain the
 * same data, or zero if they are different.
 *
 ********************************************************************/
int compBuf( BYTE *b1, BYTE *b2, int n )
{
#if 0
  int i;

  for( i = 0; i < n; i++ )
    if ( b1[i] != b2[i] )
      {
	return 0;
      }

  return 1;
#else
  return !memcmp( b1, b2, n );
#endif
}



#ifdef AKRIP_DEBUG
void dbprintf( char *fmt, ... )
{
  char buf[512];
  va_list arg;

  va_start( arg, fmt );

  vsprintf( buf, fmt, arg );
  OutputDebugString( buf );
  OutputDebugString("\n");

  va_end( arg );
}

#endif

#if 0

DWORD GetInterfaceID( void )
{
  return dwAPI;
}

DWORD GetAKRipDllVersion( void )
{
  DWORD retVal;

  retVal = (DWORD)( (((WORD)MAJVER)<<16) | (WORD)MINVER );
  return retVal;
}
#endif