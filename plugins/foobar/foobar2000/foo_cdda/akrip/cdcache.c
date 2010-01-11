/*
 * cdcache.c - Copyright (C) 1999 Jay A. Key
 *
 * Interface to CD title/artist/track names cache, and cddb.  Can optionally
 * use cdplayer.ini when internet access is not available.
 *
 * The code to access CDPLAYER.INI was adapted from code submitted by
 * Blair Thompson
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
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <winsock.h>
#include "aspilib.h"
#include "cdcache.h"


DWORD CDDBPostCmd( char *szCGI, char *cmd, char *szExtraHeaders, char *retBuf, int retLen );
DWORD CDDBPostCmdProxy( char *szCGI, char *cmd, char *szExtraHeaders, char *retBuf, int retLen );
void urlEncodeString( char *s );
void GetLineFromBuf( char **src, char *tgt, int len );
void processCDDBQuery( char *buf, LPCDDBQUERY lpq );
void SkipHTTPHeaders( char **buf );
int extractCDDBQueryInfo( LPCDDBQUERYITEM lpq, char *linebuf );
void processSites( char *buf, LPCDDBSITELIST lps );
int extractCDDBSiteInfo( LPCDDBSITE s, char *buf );
void getWord( char **inBuf, char *outBuf, int len );
DWORD genCDPlayerIniIndex( HCDROM hCD );
void MSB2DWORD( DWORD *d, BYTE *b );
DWORD getDiskInfoCDPlayerIni( LPCDDBQUERYITEM lpq, char *szCDDBEntry, int maxLen );
BOOL isCDinCDPlayerIni( char *s );
void addCDPlayerCDDBIndex( DWORD cdpIdx, DWORD cddbId, DWORD numTracks, DWORD offsets[100] );
void writeCDPlayerIniEntry( LPCDDBQUERYITEM lpq, char *szCDDBEntry );
char *base64Encode( char *tgt, BYTE *src );


typedef struct {
  DWORD cdPlayerIniId;
  DWORD cddbId;
  DWORD numTracks;
  DWORD trackOfs[100];
} CDDB2CDPLAYER, *PCDDB2CDPLAYER;



extern CDHANDLEREC *cdHandles;
extern HANDLE *cdMutexes;
extern HANDLE hCacheMutex;
extern CRITICAL_SECTION csCache;

extern CRITICAL_SECTION getHandle;
extern int alErrCode;
extern BYTE alAspiErr;

extern DWORD (*pfnSendASPI32Command)(LPSRB);

//static BOOL bCacheInitMutex = FALSE;
static BOOL bCacheInit = FALSE;

static char szCacheDir[MAX_PATH+1];
//static HANDLE hCacheMutex = NULL;
//static CRITICAL_SECTION csCache;

static char szCDPlayerIni[] = "cdplayer.ini";
static char szProxyAddr[256] = "";
static char szUserAuth[129] = "";
static BOOL bProxyAuth = FALSE;
static int  iProxyPort = 0;
static char szCDDBServer[256] = "freedb.freedb.org";
static BOOL bUseProxy = FALSE;
static char szAgent[61] = "akrip32dll 0.91";
static char szUser[65] = "user@akrip.sourceforge.net";
static char szCddbCGI[81] = "/~cddb/cddb.cgi";
static char szSubmitCGI[81] = "/~cddb/submit.cgi";
static int  iHTTPPort = 80;
static BOOL bUseCDPlayerIni = TRUE;
//static DWORD dwCDDB2CDPlayer[20][3];
static CDDB2CDPLAYER cddb2cdplayer[20];
static int iNextIndex = -1;
static BOOL bUseHTTP1_0 = TRUE;
static int protoLevel = 5;

DWORD CDDBSum( DWORD n )
{
  DWORD retVal = 0;

  while( n > 0 )
    {
      retVal += ( n % 10 );
      n /= 10;
    }

  return retVal;
}


/*
 * Computes the CDDBID for the CD in the drive represented by hCD.
 * Data which can be used to construct a CDDB query is stored in the 
 * array pID.
 *   pID[0] = CDDBID
 *   pID[1] = number of tracks
 *   pID[2..(2+n)] = starting LBA offset of tracks on CD
 * pID will need to have (at least) n+3 entries, where n is the number
 * of tracks on the CD.  102 should always be enough.  Note that the lead-out
 * track is included.
 */
DWORD GetCDDBDiskID( HCDROM hCD, DWORD *pID, int numEntries )
{
  TOC toc;
  TOCTRACK *t1, *t2;
  int idx = (int)hCD - 1;
  DWORD t;
  DWORD n;
  int i;
  int j;

  *pID = 0;

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


  memset( &toc, 0, sizeof(toc) );
  ReadTOC( hCD, &toc, TRUE);

  n = t = 0;
  for( j = 2, i = toc.firstTrack - 1; i < toc.lastTrack; i++, j++ )
    {
      t1 = &(toc.tracks[i]);
      pID[j] = (((t1->addr[1]*60)+t1->addr[2])*75)+t1->addr[3];
      n += CDDBSum( 60 * toc.tracks[i].addr[1] + toc.tracks[i].addr[2] );
    }

  t2 = &(toc.tracks[toc.lastTrack-toc.firstTrack+1]);
  t = 60 * t2->addr[1] + t2->addr[2];
  // save the disc length
  pID[toc.lastTrack+2] = t;

  t2 = &(toc.tracks[0]);
  t -= ( 60 * t2->addr[1] + t2->addr[2] );

#if 0
  // fudge the total time to generate inexact matches for testing
  t += 2;
#endif

  pID[1] = toc.lastTrack - toc.firstTrack + 1;

  *pID = ((n%0xFF) << 24) | (t << 8) | (toc.lastTrack - toc.firstTrack + 1);


  n = genCDPlayerIniIndex( hCD );

  addCDPlayerCDDBIndex( n, pID[0], pID[1], pID+2 );

  ReleaseMutex( cdMutexes[idx] );
  return SS_COMP;
}


BOOL InitCache( LPCTSTR lpszDir )
{
  if ( bCacheInit )
    return FALSE;

  if ( !lpszDir )
    lpszDir = "";
  strncpy( szCacheDir, lpszDir, MAX_PATH+1 );

  bCacheInit = TRUE;
  return TRUE;
}


/*
 * Sends an HTTP POST command and waits for a response.  The response is
 * copied to retBuf.  Uses a proxy.  Returns the number of bytes copied to
 * retBuf.  Before returning, it should strip the HTTP header from the info
 * if present.
 */
DWORD CDDBPostCmdProxy( char *szCGI, char *cmd, char *szExtraHeaders,
			char *retBuf, int retLen )
{
  WSADATA wsaData;
  SOCKET s;
  struct hostent *h;
  struct sockaddr_in sin;
  int i;
  char *postcmd, *p;
  DWORD retVal = 0;

  p = retBuf;
  ZeroMemory( p, retLen );
  retLen--;                 // reserve room for the terminating NULL

  WSAStartup( MAKEWORD(1,1), &wsaData );

  s = socket( AF_INET, SOCK_STREAM, 0 );
  h = gethostbyname( szProxyAddr );

  if ( !h )
    {
#ifdef AKRIP_DEBUG
      dbprintf( "akrip32: CDDBPostCmdProxy: unable to resolve hostname" );
#endif
      return 0;
    }

  sin.sin_family = AF_INET;
  sin.sin_port = htons( (unsigned short)iProxyPort );
  memcpy( &sin.sin_addr, h->h_addr, h->h_length );

  i = connect( s, (struct sockaddr *)&sin, sizeof(sin) );

  p = postcmd = (char *)GlobalAlloc( GPTR, lstrlen(cmd) + 512 );

  wsprintf( postcmd, "POST http://%s%s HTTP/1.0\r\nContent-Type: application/x-www-form-urlencoded\r\nUser-Agent: AKRip090\r\n", szCDDBServer, szCGI );
  p += lstrlen( postcmd );
  if ( szExtraHeaders )
    lstrcat( p, szExtraHeaders );
  p += lstrlen( p );
  if ( bProxyAuth )
  {
    wsprintf( p, "Proxy-Authorization: Basic %s\r\n", szUserAuth );
    p += lstrlen( p );
  }
  wsprintf( p, "Content-Length: %d\r\n\r\n", lstrlen( cmd ) + 2 );
  strcat( p, cmd );
  strcat( p, "\r\n" );

#if 0
  i = 120;
  i = setsockopt( s, IPPROTO_TCP, SO_SNDTIMEO, &i, sizeof(int) );
  if ( i == SOCKET_ERROR )
    dbprintf( "Error setting rcv timeout on socket" );
  else
    dbprintf( "Receive socket timeout set" );
#endif

  send( s, postcmd, lstrlen( postcmd ), 0 );

  p = retBuf;
  ZeroMemory( p, retLen );
  i = -1;
  while( (i != 0) && (retLen >= 128) )
    {
      i = recv( s, p, 128, 0 );
      p += i;
      retLen -= i;
      retVal += (DWORD)i;
      if ( i < 0 )
	i = 0;
    }

  closesocket( s );
  
  GlobalFree( (HGLOBAL)postcmd );
  WSACleanup();

  return retVal;
}


/*
 * Sends an HTTP POST command and waits for a response.  The response is
 * copied to retBuf.  Returns the number of bytes copied to retBuf.
 */
DWORD CDDBPostCmd( char *szCGI, char *cmd, char *szExtraHeaders,
		   char *retBuf, int retLen )
{
  WSADATA wsaData;
  SOCKET s;
  struct hostent *h;
  struct sockaddr_in sin;
  int i;
  char *postcmd, *p;
  DWORD retVal = 0;

  p = retBuf;
  ZeroMemory( p, retLen );
  retLen--;                 // reserve room for the terminating NULL

  WSAStartup( MAKEWORD(1,1), &wsaData );

  s = socket( AF_INET, SOCK_STREAM, 0 );
  h = gethostbyname( szCDDBServer );

  if ( !h )
    return 0;

  sin.sin_family = AF_INET;
  sin.sin_port = htons( (unsigned short)iHTTPPort );
  memcpy( &sin.sin_addr, h->h_addr, h->h_length );

  i = connect( s, (struct sockaddr *)&sin, sizeof(sin) );

  p = postcmd = (char *)GlobalAlloc( GPTR, lstrlen(cmd) + 512 );
  
  if ( !bUseHTTP1_0 )
    {
      wsprintf( p, "GET %s?", szCddbCGI );
      strcat( p, cmd );
      strcat( p, "\r\n" );
    }
  else
    {
      //wsprintf( postcmd, "POST http://%s%s HTTP/1.0\r\nContent-Type: application/x-www-form-urlencoded\r\nUser-Agent: AKRip090\r\n", szCDDBServer, szCGI );
      wsprintf( postcmd, "POST %s HTTP/1.0\r\nContent-Type: application/x-www-form-urlencoded\r\nUser-Agent: AKRip090\r\n", szCGI );
      p += lstrlen( postcmd );
      if ( szExtraHeaders )
	lstrcat( p, szExtraHeaders );
      p += lstrlen( p );
      wsprintf( p, "Content-Length: %d\r\n\r\n", lstrlen( cmd ) + 2 );
      strcat( p, cmd );
      strcat( p, "\r\n" );
    }

  send( s, postcmd, lstrlen( postcmd ), 0 );

  p = retBuf;
  ZeroMemory( p, retLen );
  i = -1;
  while( (i != 0) && (retLen >= 128) )
    {
      i = recv( s, p, 128, 0 );
      p += i;
      retLen -= i;
      retVal += (DWORD)i;
      if ( i < 0 )
	i = 0;
    }

  closesocket( s );
  
  GlobalFree( (HGLOBAL)postcmd );
  WSACleanup();

  return retVal;
}


/*
 * Queries the CDDB for a given disk.
 * Returns SS_COMP on success, SS_ERR on error.  numEntries on entry contains
 * the number of elements in the lpq array, and on return is set to the 
 * number of entries returned.
 *
 * If the no items are returned, or if no network connection is available,
 * returns one item with category "cdplayerini" and the index stored in
 * cddbId;
 */
DWORD CDDBQuery( HCDROM hCD, LPCDDBQUERY lpq )
{
  //int numRead = 0;
  LPDWORD pdwId;
  char *cmd, *p, *retBuf;
  int i;

  if ( !lpq )
    {
      return SS_ERR;
    }

  pdwId = GlobalAlloc( GPTR, 103 * sizeof(DWORD) );
  cmd = GlobalAlloc( GPTR, 1024 );
  retBuf = GlobalAlloc( GPTR, 2048 );
  if ( !cmd || !pdwId || !retBuf )
    {
      if ( cmd ) GlobalFree( (HGLOBAL)cmd );
      if ( pdwId ) GlobalFree( (HGLOBAL)pdwId );
      if ( retBuf ) GlobalFree( (HGLOBAL)retBuf );
      lpq->num = 0;
      return SS_ERR;
    }

  // Generate the cddb ID for the disc
  if ( GetCDDBDiskID( hCD, pdwId, 102 ) == SS_COMP )
    {
      // Generate a query string
      p = cmd;
      wsprintf( p, "cmd=cddb+query+%08x+%d+", pdwId[0], pdwId[1] );
      p += lstrlen( p );
      for( i = 0; i < (int)pdwId[1]; i++ )
	{
	  wsprintf( p, "%d+", pdwId[i+2] );
	  p += lstrlen(p);
	}
      wsprintf( p, "%d&hello=%s+%s&proto=%d",
		pdwId[pdwId[1]+2], szUser, szAgent, protoLevel );
      urlEncodeString( p );

      // send the query string
      if ( bUseProxy )
	CDDBPostCmdProxy( szCddbCGI, cmd, NULL, retBuf, 2048 );
      else
	CDDBPostCmd( szCddbCGI, cmd, NULL, retBuf, 2048 );

      // process the returned data (if any)
      processCDDBQuery( retBuf, lpq );

      // fall back to cdplayer.ini if no items matched
      if ( (lpq->num == 0) && bUseCDPlayerIni )
	{
	  pdwId[0] = genCDPlayerIniIndex( hCD );
	  wsprintf( retBuf, "%X", pdwId[0] );
	  if ( isCDinCDPlayerIni( retBuf ) )
	    {
	      wsprintf( lpq->q[0].cddbId, "%X", pdwId[0] );
	      lstrcpy( lpq->q[0].categ, "cdplayerini" );
	      lpq->q[0].bExact = TRUE;
	      lpq->num = 1;
	    }
	}
      // else if we have multiple/partial matches from CDDB, we need to 
      // associate all of the cddbId values with the cdPlayer.ini index
      else if ( (lpq->num >= 1) && bUseCDPlayerIni )
	{
          DWORD pTmp = genCDPlayerIniIndex( hCD );
	  for( i = 0; i < lpq->num; i++ )
	    {
	      pdwId[0] = (DWORD)strtoul( lpq->q[i].cddbId, NULL, 16 );
	      addCDPlayerCDDBIndex( pTmp, pdwId[0], pdwId[1], pdwId+2 );
	    }
	}
    }

  GlobalFree( (HGLOBAL)pdwId );
  GlobalFree( (HGLOBAL)cmd );
  GlobalFree( (HGLOBAL)retBuf );

  return SS_COMP;
}


void CDDBSetOption( int what, char *szVal, int iVal )
{
  switch( what )
    {
    case CDDB_OPT_PROXY:
      if ( szVal )
	lstrcpyn( szProxyAddr, szVal, 255 );
      szProxyAddr[255] = 0;
      break;

    case CDDB_OPT_SERVER:
      if ( szVal )
	lstrcpyn( szCDDBServer, szVal, 255 );
      szCDDBServer[255] = 0;
      break;

    case CDDB_OPT_CGI:
      if ( szVal )
	lstrcpyn( szCddbCGI, szVal, 80 );
      szCddbCGI[80] = 0;
      break;

    case CDDB_OPT_SUBMITCGI:
      if ( szVal )
	lstrcpyn( szSubmitCGI, szVal, 80 );
      szSubmitCGI[80] = 0;
      break;

    case CDDB_OPT_PROXYPORT:
      iProxyPort = iVal;
      break;

    case CDDB_OPT_AGENT:
      if ( szVal )
	lstrcpyn( szAgent, szVal, 60 );
      szAgent[60] = 0;
      urlEncodeString( szAgent );
      break;

    case CDDB_OPT_USER:
      if ( szVal )
	lstrcpyn( szUser, szVal, 64 );
      szUser[64] = 0;
      urlEncodeString( szUser );
      break;

    case CDDB_OPT_USEPROXY:
      bUseProxy = (BOOL)iVal;
      break;

    case CDDB_OPT_HTTPPORT:
      iHTTPPort = iVal;
      break;

    case CDDB_OPT_USECDPLAYERINI:
      bUseCDPlayerIni = (BOOL)iVal;
      break;

    case CDDB_OPT_USEHTTP1_0:
      bUseHTTP1_0 = (BOOL)iVal;
      break;
    
    case CDDB_OPT_USERAUTH:
      if ( szVal != NULL )
      {
        char tmp[121];
        base64Encode( tmp, szVal );
        lstrcpyn( szUserAuth, tmp, 129 );
        bProxyAuth = TRUE;
      }
      else
        bProxyAuth = FALSE;
      break;
      
    case CDDB_OPT_PROTOLEVEL:
      if ( iVal > 0 )
        protoLevel = iVal;
      break;
    }
}


void urlEncodeString( char *s )
{
  if ( !s || !*s )
    return;

  while( *++s )
    {
      if ( (*s == '@') || (*s == ' ') )
	*s = '+';
    }
}


/*
 * Process the return buffer from a cddb query.  Verify that the return code
 * is one that we expect (200, 211, 202, 403, 409).
 */
void processCDDBQuery( char *buf, LPCDDBQUERY lpq )
{
  int total = 0;
  int iRetCode, i;
  int maxLines = 100;
  char retCode[4] = "100";
  char linebuf[81];
  char *p = buf;
#if 0
  FILE *fp;

  fp = fopen( "retbuf.txt", "wb" );
  fwrite( p, 1, lstrlen( p ), fp );
  fclose( fp );
#endif

  SkipHTTPHeaders( &p );

  GetLineFromBuf( &p, linebuf, 81 );

  strncpy( retCode, linebuf, 3 );
  iRetCode = atoi( retCode );

  switch( iRetCode )
    {
    case 200:
      // one exact match
      if ( extractCDDBQueryInfo( &lpq->q[0], linebuf+4 ) )
	{
	  lpq->q[0].bExact = TRUE;
	  total++;
	}
      break;
    
    case 210:     // multiple exact matches
    case 211:     // inexact match(es)
      i = 0;
      while ( p && lpq->num && (maxLines-- > 0) )
	{
	  GetLineFromBuf( &p, linebuf, 81 );
	  if ( !strcmp( linebuf, "." ) )
	    break;
	  if ( extractCDDBQueryInfo( &lpq->q[i], linebuf ) )
	    {
	      lpq->q[i].bExact = (iRetCode==210);
	      total++;
	      i++;
	      lpq->num--;
	    }
	}
      break;

    case 202:
      // no matches;
      break;

    case 403:
      // database entry corrupt
      break;

    case 409:
      // no handshake (probably won't happen with HTTP)
      break;
    }

  lpq->num = total;
}


void GetLineFromBuf( char **src, char *tgt, int len )
{
  char *s, *t;

  if ( !src || !*src || !tgt )
    {
      return;
    }

  ZeroMemory( tgt, len );

  s = strstr( *src, "\r\n" );
  if ( !s )
    {
      *src += lstrlen( *src );
      return;
    }

  lstrcpyn( tgt, *src, len-1 );
  t = strstr( tgt, "\r\n" );
  if ( t && ((t - tgt) < len) )
    tgt[t-tgt] = '\0';
  
  *src = s + 2;
}


/*
 * extracts the category, cddbid and title(artist/album) from linebuf and 
 * stores it in lpq.  The line is expected to be in the format
 * "category cddbid dtitle"
 */
int extractCDDBQueryInfo( LPCDDBQUERYITEM lpq, char *linebuf )
{
  int i;
  char *p = linebuf;
  char *t;

  if ( !lpq || !linebuf || !*linebuf )
    return 0;

  ZeroMemory( lpq, sizeof(CDDBQUERY) );
  
  t = strstr( p, " " );
  if ( (t == NULL) || (t == p) )
    return 0;
  
  i = (t - p + 1);
  if ( i > 12 )
    i = 12;
  lstrcpyn( lpq->categ, p, i );
  p = t;

  // extract the cddbid
  while ( *p && (*++p == ' ') );    // skip space
  i = 8;
  t = lpq->cddbId;
  while ( *p && ( isxdigit( *p ) ) && i )
    {
      *t++ = *p++;
      i--;
    }

  if ( *p != ' ' )
    {
      return 0;
    }

  // get artist and title
  while ( *p && (*++p == ' ') );    // skip space
    
  
  t = strstr( p, " / " );
  if ( t == NULL )
  {
    // " / " not found
    lstrcpyn( lpq->artist, p, 81 );
    lstrcpyn( lpq->title, p, 81 );
  }
  else
  {
    i = (t - p + 1);
    if ( i > 81 )
      i = 81;
    lstrcpyn( lpq->artist, p, i );
    lstrcpyn( lpq->title, p+3, 81 );
  }
  
  i = lstrlen( lpq->title );
  if ( i > 0 )
    {
      if ( lpq->title[i-1] == '\r' || lpq->title[i-1] == '\n' )
	lpq->title[i-1] = '\0';
    }
  i = lstrlen( lpq->artist );
  if ( i > 0 )
    {
      if ( lpq->artist[i-1] == '\r' || lpq->artist[i-1] == '\n' )
	lpq->artist[i-1] = '\0';
    }


  return 1;
}


void SkipHTTPHeaders( char **buf )
{
  char *p;

  if ( !buf || !*buf || !**buf )
    return;

  p = *buf;

  if ( strncmp( p, "HTTP", 4 ) )
    {
      return;
    }

  p = strstr( p, "\r\n\r\n" );
  if ( p )
    {
      p += 4;
      *buf = p;
    }
}


/*
 * Returns the CDDB entry verbatim from the CDDB database.  If not large
 * enough, no data is copied.  Verifies that the return code from CDDB is
 * 210 -- CDDB entry follows...
 *
 * If the use of CDPLAYER.INI is enabled and the category is for the query
 * is "cdplayerini", then an attempt is made to read the information from
 * CDPLAYER.INI.
 */
DWORD CDDBGetDiskInfo( LPCDDBQUERYITEM lpq, char *szCDDBEntry, int maxLen )
{
  char *cmd, *p;
  char *retBuf;
  DWORD retVal = SS_ERR;

  if ( !lpq || !szCDDBEntry )
    return retVal;

  if ( !lstrcmp( lpq->categ, "cdplayerini" ) )
    return getDiskInfoCDPlayerIni( lpq, szCDDBEntry, maxLen );

  cmd = (char *)GlobalAlloc( GPTR, 512 );
  retBuf = (char *)GlobalAlloc( GPTR, maxLen );

  p = cmd;
  wsprintf( p, "cmd=cddb+read+%s+%s", lpq->categ, lpq->cddbId );
  p += lstrlen( p );
  wsprintf( p, "&hello=%s+%s&proto=%d", szUser, szAgent, protoLevel );
  urlEncodeString( p );

      // send the query string
  if ( bUseProxy )
    CDDBPostCmdProxy( szCddbCGI, cmd, NULL, retBuf, maxLen );
  else
    CDDBPostCmd( szCddbCGI, cmd, NULL, retBuf, maxLen );

  // strip any HTTP headers
  p = retBuf;
  SkipHTTPHeaders( &p );

  if ( !strncmp( p, "210", 3 ) )
    {
      p = strstr( p, "\n" );
      if ( p )
	{
	  p += 1;  // skip the '\n'
	  if ( lstrlen(p) < maxLen )
	    {
              char *szEnd;
              szEnd = p + lstrlen( p ) - 3;
              szEnd = strstr( szEnd, "." );
              if ( szEnd )
                *szEnd = '\0';
	      strcpy( szCDDBEntry, p );
	      retVal = SS_COMP;
	      // should we add it to cdplayer.ini?
	      if ( bUseCDPlayerIni && *p )
		{
		  writeCDPlayerIniEntry( lpq, p );
		}
	    }
	}
    }

  GlobalFree( (HGLOBAL)cmd );
  GlobalFree( (HGLOBAL)retBuf );

  return retVal;
}



DWORD CDDBGetServerList( LPCDDBSITELIST lps )
{
  char *cmd, *p;
  char *retBuf;
  DWORD retVal = SS_ERR;

  if ( !lps || !lps->s )
    return retVal;

  cmd = (char *)GlobalAlloc( GPTR, 512 );
  retBuf = (char *)GlobalAlloc( GPTR, 4096 );

  p = cmd;
  wsprintf( p, "cmd=sites&hello=%s+%s&proto=%d", szUser, szAgent, protoLevel );
  urlEncodeString( p );

  // send the query string
  if ( bUseProxy )
    CDDBPostCmdProxy( szCddbCGI, cmd, NULL, retBuf, 4096 );
  else
    CDDBPostCmd( szCddbCGI, cmd, NULL, retBuf, 4096 );

  if ( retBuf[0] )
    {
      // look for the "210 OK, site information..." message
      processSites( retBuf, lps );
      retVal = SS_COMP;
    }

  GlobalFree( (HGLOBAL)cmd );
  GlobalFree( (HGLOBAL)retBuf );

  return retVal;
}



void processSites( char *buf, LPCDDBSITELIST lps )
{
  char linebuf[81];
  char retCode[4] = "";
  char *p;
  int total, iRetCode;
  int maxLines = 100;

  // strip any HTTP headers
  p = buf;
  SkipHTTPHeaders( &p );

  GetLineFromBuf( &p, linebuf, 81 );

  strncpy( retCode, linebuf, 3 );
  iRetCode = atoi( retCode );

  total = 0;

  switch( iRetCode )
    {
    case 210:   // normal return code, site list follows
      while ( p && lps->num && (maxLines-- > 0) )
	{
	  GetLineFromBuf( &p, linebuf, 81 );
	  if ( !strcmp( linebuf, "." ) )
	    break;
	  if ( extractCDDBSiteInfo( &lps->s[total], linebuf ) )
	    {
	      total++;
	      lps->num--;
	    }
	}
      break;

    case 401: // no site info available
      break;
    }

  lps->num = total;
}



int extractCDDBSiteInfo( LPCDDBSITE lps, char *linebuf )
{
  char *p;
  char buf[6];

  if ( !lps || !linebuf || !*linebuf )
    return 0;

  ZeroMemory( lps, sizeof(CDDBSITE) );

  p = linebuf;

  // extract the server
  getWord( &p, lps->szServer, 81 );
  if ( *p != ' ' )
    return 0;

  // extract the protocol
  getWord( &p, buf, 6 );
  if ( *p != ' ' )
    return 0;
  if ( !lstrcmpi( buf, "http" ) )
    lps->bHTTP = TRUE;

  // extract the port number
  getWord( &p, buf, 6 );
  if ( *p != ' ' )
    return 0;
  lps->iPort = atoi( buf );
  if ( lps->bHTTP && !lps->iPort )
    lps->iPort = 80;

  // extract the CGI
  getWord( &p, lps->szCGI, 81 );
  if ( *p != ' ' )
    return 0;

  // extract north coordinate
  getWord( &p, lps->szNorth, 16 );
  if ( *p != ' ' )
    return 0;

  // extract north coordinate
  getWord( &p, lps->szSouth, 16 );
  if ( *p != ' ' )
    return 0;

  // extract the location
  while( *p && (*p == ' ') ) p++; 
  lstrcpyn( lps->szLocation, p, 80 );

  return -1;
}


void getWord( char **inBuf, char *outBuf, int len )
{
  char *p = *inBuf;

  ZeroMemory( outBuf, len );
  len--;

  // skip space
  while( *p && (*p == ' ') ) p++;

#if 0
  while( TRUE )
    {
      if ( !*p )
	break;
      if ( !isalnum( *p ) && (*p != '.') )
	break;
      if ( !len )
	break;
      *outBuf++ = *p++;
      len--;
    }
#else
  while( *p &&
	 ( isalnum( *p ) || ( *p == '.') || ( *p == '/' ) || ( *p == '~' ) ) &&
	 len )
    {
      *outBuf++ = *p++;
      len--;
    }
#endif

  *inBuf = p;
}


DWORD genCDPlayerIniIndex( HCDROM hCD )
{
  DWORD retVal = 0;
  int idx = (int)hCD - 1;
  int i;
  TOC toc;
  DWORD dwAddr;

  memset( &toc, 0, sizeof(toc) );
  ReadTOC( hCD, &toc , TRUE);

  for( i = 0; i <= (toc.lastTrack - toc.firstTrack); i++ )
    {
      if ( !(toc.tracks[i].ADR & 0x04 ) )
	{
	  MSB2DWORD( &dwAddr, toc.tracks[i].addr );
	  retVal += dwAddr;
	}
    }


  return retVal;
}


void MSB2DWORD( DWORD *d, BYTE *b )
{
  DWORD retVal;

  retVal = (DWORD)b[0];
  retVal = (retVal<<8) + (DWORD)b[1];
  retVal = (retVal<<8) + (DWORD)b[2];
  retVal = (retVal<<8) + (DWORD)b[3];

  *d = retVal;
}


void addString( char **pBuf, int *maxLen, char *szInfo )
{
  int len;
  if ( !pBuf || !*pBuf || !maxLen || !szInfo )
    return;
  
  len = lstrlen( szInfo );
  
  if ( *maxLen < (len + 1) )
    return;
  
  strcpy( *pBuf, szInfo );
  *maxLen -= len;
  *pBuf += len;
}


DWORD getCDPlayerIniOffset( DWORD cdPlayerIniId, int trackNo )
{
  return 0;
}


DWORD getDiskInfoCDPlayerIni( LPCDDBQUERYITEM lpq, char *szCDDBEntry, int maxLen )
{
  UINT i, numRead;
  char buf[512];
  char idx[5];
  char defaultName[13];
  char *p;
  DWORD cdPlayerIniId;
  int cdPlayerIdx;

  if ( !lpq || !szCDDBEntry )
    return SS_ERR;

  cdPlayerIniId = (DWORD)strtoul( lpq->cddbId, NULL, 16 );
  cdPlayerIdx = -1;
  for( i = 0; i < 20; i++ )
  {
    if ( cddb2cdplayer[i].cdPlayerIniId == cdPlayerIniId )
    {
      cdPlayerIdx = (int)i;
      break;
    }
  }
  
  if ( idx < 0 )
    return SS_ERR;
  
  numRead = GetPrivateProfileInt( lpq->cddbId, "NUMTRACKS", 0, szCDPlayerIni );
  if ( numRead )
    {
      lstrcpy( lpq->categ, "rock" );
      lpq->bExact = TRUE;
      GetPrivateProfileString( lpq->cddbId, "ARTIST", "", buf, 256, szCDPlayerIni );
      lstrcpyn( lpq->artist, buf, 80 ); lpq->artist[80] = '\0';
      GetPrivateProfileString( lpq->cddbId, "TITLE", "", buf, 256, szCDPlayerIni );
      lstrcpyn( lpq->title, buf, 80 ); lpq->title[80] = '\0';
      p = szCDDBEntry;
      
      // write the entry header
      addString( &p, &maxLen, "# xcmd CD database file\r\n# Copyright (c) CD-DA X-Tractor 2001\r\n#\r\n# Track frame offsets:\r\n" );
      
      // write track offsets:
      for( i = 0; i < numRead; i++ )
      {
        DWORD ofs;
        ofs = cddb2cdplayer[cdPlayerIdx].trackOfs[i];
        wsprintf( buf, "#\t%d\r\n", ofs );
        addString( &p, &maxLen, buf );
      }
      
      // the .trackOfs[numRead] entry holds the length of the CD
      // as used to calculate the CDDBID.  In the entry, however,
      // we must add 2 seconds for the lead-in.
      wsprintf( buf, "#\r\n# Disc length: %d\r\n#\r\n", cddb2cdplayer[cdPlayerIdx].trackOfs[numRead] + 2 );
      addString( &p, &maxLen, buf );
      
      // we'll just fake the revision, processed by, and submitted
      // lines
      addString( &p, &maxLen, "# Revision: 1\r\n# Processed by: Some program that we don't know\r\n# Submitted by: Some guy\r\n#\r\n" );
      
      wsprintf( buf, "DTITLE=%s / %s\r\n", lpq->artist, lpq->title );
      addString( &p, &maxLen, buf );
      for( i = 0; i < numRead; i++ )
	{
	  wsprintf( idx, "%d", i );
	  wsprintf( defaultName, "Track %d", i+1 );
	  GetPrivateProfileString( lpq->cddbId, idx, defaultName, buf, 256, szCDPlayerIni );
	  if ( maxLen > (lstrlen( buf )+12) )
	    {
	      wsprintf( p, "TTITLE%d=%s\r\n", i, buf );
	      maxLen -= lstrlen( p );
	      p += lstrlen( p );
	    }
	  else
	    break;
	}
    }

  if ( numRead )
    return SS_COMP;
  return SS_ERR;
}


BOOL isCDinCDPlayerIni( char *s )
{
  UINT uiVal;

  uiVal = GetPrivateProfileInt( s, "NUMTRACKS", 0, "cdplayer.ini" );

  return (BOOL)uiVal;
}


/*
 * This fairly cryptic function saves the cdplayer.ini and cddbId
 * values so that a CDDB entry can be constructed from cdplayer.ini
 * if remote CDDB is not available.
 */
void addCDPlayerCDDBIndex( DWORD cdpIdx, DWORD cddbId, DWORD numTracks, DWORD offsets[100] )
{
  DWORD i;
  if ( iNextIndex == -1 )
    ZeroMemory( cddb2cdplayer, sizeof(cddb2cdplayer) );

  if ( (++iNextIndex % 20) == 0 )
    iNextIndex = 0;

  cddb2cdplayer[iNextIndex].cdPlayerIniId = cdpIdx;
  cddb2cdplayer[iNextIndex].cddbId = cddbId;
  cddb2cdplayer[iNextIndex].numTracks = numTracks;
  ZeroMemory( cddb2cdplayer[iNextIndex].trackOfs, sizeof(DWORD[100]) );

  // in some cases we won't be saving the info
  if ( offsets == NULL )
    return;
  
  // we use 'i <= numTracks' in the for loop, since we need to include 
  // the leadout track.
  for( i = 0; i <= numTracks; i++ )
    cddb2cdplayer[iNextIndex].trackOfs[i] = offsets[i];
}


/*
 * This function returns the cdplayer.ini index and number of tracks
 * given a cddbid.
 */
DWORD CDDBIndex2CDPlayerIni( char *szCDDBId, DWORD *dwRetVal, DWORD *numTracks )
{
  int i;
  DWORD dwIdx = (DWORD)strtoul( szCDDBId, NULL, 16 );

  for( i = 0; i < 20; i++ )
    {
      if ( cddb2cdplayer[i].cddbId == dwIdx )
	{
	  *dwRetVal = cddb2cdplayer[i].cdPlayerIniId;
	  *numTracks = cddb2cdplayer[i].numTracks;
	  return *dwRetVal;
	}
    }

  return 0;
}


/*
 * Stores a CDDB entry in cdplayer.ini.  NOTE: the buffer pointed to
 * by szCDDBEntry may be modified by this function.  Do not count on the
 * buffer being unmodified!!!
 */
void writeCDPlayerIniEntry( LPCDDBQUERYITEM lpq, char *szCDDBEntry )
{
  DWORD dwCDPlayerIdx, dwNumTracks;
  char section[24];
  char buf[128];
  char *p1, *p2;

  CDDBIndex2CDPlayerIni( lpq->cddbId, &dwCDPlayerIdx, &dwNumTracks );
  if ( !dwCDPlayerIdx )
    {
      return;
    }

  wsprintf( section, "%X", dwCDPlayerIdx );
  WritePrivateProfileString( section, "EntryType", "1", szCDPlayerIni );
  WritePrivateProfileString( section, "artist", lpq->artist, szCDPlayerIni );
  WritePrivateProfileString( section, "title", lpq->title, szCDPlayerIni );
  WritePrivateProfileString( section, "genre", lpq->categ, szCDPlayerIni );
  wsprintf( buf, "%d", dwNumTracks );
  WritePrivateProfileString( section, "numtracks", buf, szCDPlayerIni );

  while (*szCDDBEntry)
    {
      GetLineFromBuf( &szCDDBEntry, buf, 128 );
      if ( !szCDDBEntry )
	break;
      if ( !strncmp( "TTITLE", buf, 6 ) )
	{
	  p1 = buf + 6;
	  p2 = strstr( buf, "=" );
	  if ( *p1 && p2 )
	    {
	      *p2 = '\0';
	      p2++;
	      WritePrivateProfileString( section, p1, p2, szCDPlayerIni );
	    }
	}
    }
}


DWORD CDDBSubmit( DWORD dwDiscID, BOOL bTest, char *szEmail, char *szCategory,
		  char *szEntry )
{
  DWORD dwRetVal = SS_COMP;
  char buf[512];
  char *p = buf;
  char *retBuf;

  retBuf = (char *)GlobalAlloc( GPTR, 32*1024 );

#ifndef _DEBUG_NOSENDCAT
  wsprintf( p, "Category: %s\r\n", szCategory );
  p += lstrlen( p );
#endif
#ifndef _DEBUG_NOSENDDISCID
  wsprintf( p, "Discid: %08x\r\n", dwDiscID );
  p += lstrlen( p );
#endif
#ifndef _DEBUG_NOSENDEMAIL
  wsprintf( p, "User-Email: %s\r\n", szEmail );
  p += lstrlen( p );
#endif
#ifndef _DEBUG_NOSENDMODE
  wsprintf( p, "Submit-Mode: %s\r\n", bTest?"test":"submit" );
  p += lstrlen( p );
#endif

  // send the submit
  if ( bUseProxy )
    CDDBPostCmdProxy( szSubmitCGI, szEntry, buf, retBuf, 32*1024 );
  else
    CDDBPostCmd( szSubmitCGI, szEntry, buf, retBuf, 32*1024 );

#ifdef _DEBUG_CDDB_SUBMIT
  {
    FILE *fp = fopen( "cddbsubmit.dat", "wb" );
    if ( fp )
      {
	fwrite( retBuf, 1, lstrlen( retBuf ), fp );
	fclose( fp );
      }
  }
#endif

  // check the return code from cddb -- 200 on success
  p = retBuf;
  SkipHTTPHeaders( &p );
  if ( strncmp( p, "200", 3 ) )
    return dwRetVal = SS_ERR;

  GlobalFree( (HGLOBAL)retBuf );

  return dwRetVal;
}

/*
 * Base64 encoding for proxy authorization
 */
static char base64_table[] =
{ 
  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
  'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
  'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
  'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
  'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
  'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
  'w', 'x', 'y', 'z', '0', '1', '2', '3',
  '4', '5', '6', '7', '8', '9', '+', '/'
};
static char base64_pad = '=';

char *base64Encode( char *tgt, BYTE *src )
{
  int len = lstrlen( src );
  int i = 0;
  
  while( len > 2 ) {
    tgt[i++] = base64_table[src[0]>>2];
    tgt[i++] = base64_table[((src[0]&0x03)<<4) | (src[1]>>4)];
    tgt[i++] = base64_table[((src[1]&0x0f)<<2) | (src[2]>>6)];
    tgt[i++] = base64_table[src[2]&0x3f];
    len -= 3;
    src += 3;
  }
  
  if ( len == 2 )
  {
    tgt[i++] = base64_table[src[0]>>2];
    tgt[i++] = base64_table[((src[0]&0x03)<<4) | (src[1]>>4)];
    tgt[i++] = base64_table[(src[1]&0x0f)<<2];
    tgt[i++] = '=';
  }
  else if ( len == 1 )
  {
    tgt[i++] = base64_table[src[0]>>2];
    tgt[i++] = base64_table[(src[0]&0x03)<<4];
    tgt[i++] = '=';
    tgt[i++] = '=';
  }
  tgt[i] = '\0';
  
  return tgt;
}