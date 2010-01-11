
// Zip7_Extractor 0.3.0. http://www.slack.net/~ant/

#include "Zip7_Extractor.h"

#include <stdlib.h>
#include <assert.h>
 
#include <stddef.h>
#include "7z_C/7zTypes.h"
extern "C" {
	#include "7z_C/7zExtract.h"
	#include "7z_C/7zCrc.h"
}

#include "timefn.h"

/* Copyright (C) 2006 Shay Green. This module is free software; you
can redistribute it and/or modify it under the terms of the GNU Lesser
General Public License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version. This
library is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for
more details. You should have received a copy of the GNU Lesser General
Public License along with this module; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA */

static ISzAlloc alloc = { SzAlloc, SzFree };
static ISzAlloc alloc_temp = { SzAllocTemp, SzFreeTemp };

struct Zip7_Extractor_Impl
{
	ISzInStream stream; // must be first
	Zip7_Extractor* intf;
	File_Reader* in;
	CArchiveDatabaseEx db;
	UInt32 block_index;
	Byte* buf;
	size_t buf_size;
};

extern "C"
{
	static SZ_RESULT zip7_read_( void* data, void* out, size_t size, size_t* size_out )
	{
		Zip7_Extractor_Impl* impl = (Zip7_Extractor_Impl*) data;
		*size_out = impl->in->read_avail( out, size );
		// TODO: does it expect an error result if less than 'size' bytes were read?
		return SZ_OK;
	}

	static SZ_RESULT zip7_seek_( void* data, CFileSize offset )
	{
		Zip7_Extractor_Impl* impl = (Zip7_Extractor_Impl*) data;
		if ( impl->in->seek( offset ) )
			return SZE_FAIL;
		return SZ_OK;
	}
}

Zip7_Extractor::Zip7_Extractor()
{
	impl = 0;
	InitCrcTable();
}

File_Extractor* extractor_open_7zip( const char* path, const char** err )
{
	return extractor_open_( new Zip7_Extractor, path, err );
}

Zip7_Extractor::~Zip7_Extractor() { close(); }

static const char* zip7_err( int err )
{
	switch ( err )
	{
	case SZ_OK: return 0;
	case SZE_OUTOFMEMORY: return "7-zip out of memory";
	case SZE_CRC_ERROR: return "7-zip archive is corrupt (CRC error)";
	case SZE_NOTIMPL: return "Unsupported 7-zip feature";
	//case SZE_FAIL:
	//case SZE_DATA_ERROR:
	//case SZE_ARCHIVE_ERROR:
	}
	
	printf( "%d\n", err );
	
	return "7-zip error";
}

Zip7_Extractor::error_t Zip7_Extractor::open_()
{
	if ( !impl )
	{
		impl = (Zip7_Extractor_Impl*) malloc( sizeof *impl );
		if ( !impl )
			return "Out of memory";
	}
	
	impl->stream.Read = zip7_read_;
	impl->stream.Seek = zip7_seek_;
	impl->in = reader;
	impl->block_index = -1;
	impl->buf = 0;
	impl->buf_size = 0;
	
	SzArDbExInit( &impl->db );
	int code = SzArchiveOpen( &impl->stream, &impl->db, &alloc, &alloc_temp );
	error_t err = (code == SZE_ARCHIVE_ERROR ? not_archive_err : zip7_err( code ));
	if ( err )
		return err;
	
	return rewind();
}

Zip7_Extractor::error_t Zip7_Extractor::rewind()
{
	index = -1;
	done_ = 0;
	return next();
}

void Zip7_Extractor::close_()
{
	if ( impl )
	{
		if ( impl->in )
		{
			impl->in = 0;
			SzArDbExFree( &impl->db, alloc.Free );
		}
		
		alloc.Free( impl->buf );
		free( impl );
		impl = 0;
	}
}

Zip7_Extractor::error_t Zip7_Extractor::next()
{
	while ( 1 )
	{
		index++;
		int count = (int) impl->db.Database.NumFiles;
		if ( index >= count )
		{
			index = count;
			done_ = 1;
		}
		else
		{
			CFileItem const* item = &impl->db.Database.Files [index];
			name_ = item->Name;
			size_ = item->Size;
			if ( item->IsLastWriteTimeDefined )
				timestamp_ = item->LastWriteTime;
			if ( item->IsDirectory )
				continue;
		}
		break;
	}
	
	return 0; 
}

Zip7_Extractor::error_t Zip7_Extractor::extract( Data_Writer& out )
{
	size_t offset = 0;
	size_t size = 0;
	error_t err = zip7_err( SzExtract( &impl->stream, &impl->db, index,
			&impl->block_index, &impl->buf, &impl->buf_size,
			&offset, &size, &alloc, &alloc_temp ) );
	if ( err )
		return err;
	assert( size == size_ );
	return out.write( impl->buf + offset, size );
}

