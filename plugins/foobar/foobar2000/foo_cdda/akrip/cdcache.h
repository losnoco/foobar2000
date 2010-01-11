/*
 * cdcache.h - Copyright (C) 1999 Jay A. Key
 *
 * Access routines to local cd cache and cddb
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

#ifndef _CDCACHE_H_INC
#define _CDCACHE_H_INC

#include <windows.h>

/*
 * Cache file is a tagged format.  Each file consists of a header, followed by
 * tagged 
 *
 */

typedef struct {
  DWORD tag;         // information identifier
  DWORD size;        // size (including header)
  DWORD id;          // cddb id
  DWORD flags;
} CDCACHEHEADER, FAR *LPCDCACHEHEADER;


/*
 * tag constants
 */
#define CDCH_TAG_UNKNOWN    0L      // undefined
#define CDCH_TAG_QUERY      1L      // array of DWORD for generating query
#define CDCH_TAG_LOCALCDDB  2L      // locally entered CDDB info
#define CDCH_TAG_CDDBINFO   3L      // CDDB info
#define CDCH_TAG_PARTIAL    4L      // CDDB info for partial match

/*********************************************************
 * Internal interface
 *********************************************************/

#endif
