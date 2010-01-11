
// Zip_Extractor 0.3.0. http://www.slack.net/~ant/

#include "Zip_Extractor.h"

#include "timefn.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "zlib.h"

/* Copyright (C) 2005-2006 Shay Green. This module is free software; you
can redistribute it and/or modify it under the terms of the GNU Lesser
General Public License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version. This
library is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for
more details. You should have received a copy of the GNU Lesser General
Public License along with this module; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA */

#define RETURN_ERR( expr ) {\
	error_t err_ = (expr);\
	if ( err_ ) return err_;\
}

typedef unsigned char byte;

struct header_t {
	char type [4];
	byte vers [2];
	byte flags [2];
	byte method [2];
	byte time [2];
	byte date [2];
	byte crc [4];
	byte raw_size [4];
	byte size [4];
	byte filename_len [2];
	byte extra_len [2];
	//char filename [filename_len];
	//char extra [extra_len];
};

struct entry_t {
	char type [4];
	byte made_by [2];
	byte vers [2];
	byte flags [2];
	byte method [2];
	byte time [2];
	byte date [2];
	byte crc [4];
	byte raw_size [4];
	byte size [4];
	byte filename_len [2];
	byte extra_len [2];
	byte comment_len [2];
	byte disk [2];
	byte int_attrib [2];
	byte ext_attrib [4];
	byte file_offset [4];
	//char filename [filename_len];
	//char extra [extra_len];
	//char comment [comment_len];
};

struct end_entry_t {
	char type [4];
	byte disk [2];
	byte first_disk [2];
	byte disk_entry_count [2];
	byte entry_count [2];
	byte dir_size [4];
	byte dir_offset [4];
	byte comment_len [2];
	//char comment [comment_len];
};

Zip_Extractor::Zip_Extractor()
{
	catalog = 0;
	
	assert( sizeof (header_t) == 30 );
	assert( sizeof (entry_t) == 46 );
	assert( sizeof (end_entry_t) == 22 );
}

File_Extractor* extractor_open_zip( const char* path, const char** err )
{
	return extractor_open_( new Zip_Extractor, path, err );
}

void Zip_Extractor::close_()
{
	free( catalog );
	catalog = 0;
}

Zip_Extractor::~Zip_Extractor() { close(); }

inline unsigned get_le32( byte const* p )
{
	return p [3] * 0x1000000u + p [2] * 0x10000u + p [1] * 0x100u + p [0];
}

inline unsigned get_le16( byte const* p )
{
	return p [1] * 0x100u + p [0];
}

Zip_Extractor::error_t Zip_Extractor::open_()
{
	// try reading end of catalog entry with no comment
	long file_size = reader->size();
	end_entry_t entry;
	if ( file_size < (int) sizeof entry )
		return not_archive_err;
	
	long end_offset = file_size - sizeof entry;
	RETURN_ERR( reader->seek( end_offset ) );
	RETURN_ERR( reader->read( &entry, sizeof entry ) );
	if ( 0 != memcmp( entry.type, "PK\5\6", 4 ) )
	{
		// scan end of file for end entry
		end_offset = file_size - 8 * 1024L;
		if ( end_offset < 0 )
			end_offset = 0;
		int buf_size = file_size - end_offset;
		byte* buf = (byte*) malloc( buf_size );
		if ( !buf )
			return "Out of memory";
		
		error_t err = reader->seek( end_offset );
		if ( !err )
			err = reader->read( buf, buf_size );
		int i = buf_size - sizeof entry;
		if ( !err )
		{
			while ( 0 != memcmp( buf + i, "PK\5\6", 4 ) )
			{
				if ( --i < 0 )
				{
					#ifndef NDEBUG
						reader->seek( 0 );
						char buf [2];
						reader->read( buf, sizeof buf );
						// didn't find header; be sure this isn't a zip file
						assert( buf [0] != 'P' || buf [1] != 'K' );
					#endif
					return not_archive_err;
				}
			}
		}
		free( buf );
		if ( err )
			return err;
		
		end_offset += i;
		
		RETURN_ERR( reader->seek( end_offset ) );
		RETURN_ERR( reader->read( &entry, sizeof entry ) );
	}
	
	#ifdef check
		long zip_end = end_offset + sizeof entry + get_le16( entry.comment_len );
		check( zip_end == file_size );
	#endif
	
	// read catalog into memory
	long dir_offset = get_le32( entry.dir_offset );
	assert( dir_offset < file_size );
	RETURN_ERR( reader->seek( dir_offset ) );
	long catalog_size = end_offset + sizeof entry - dir_offset;
	catalog = (char*) malloc( catalog_size );
	if ( !catalog )
		return "Out of memory";
	RETURN_ERR( reader->read( catalog, catalog_size ) );
	
	// first entry in catalog should be a file or end of archive
	if ( memcmp( catalog, "PK\1\2", 4 ) && memcmp( catalog, "PK\5\6", 4 ) )
	{
		#ifdef check
			check( 0 );
		#endif
		return not_archive_err;
	}
	
	return rewind();
}

Zip_Extractor::error_t Zip_Extractor::rewind()
{
	offset = 0;
	done_ = 0;
	update_info();
	return 0;
}

void Zip_Extractor::next_item()
{
	const entry_t& e = *(entry_t*) (catalog + offset);
	offset += sizeof e + get_le16( e.filename_len ) + get_le16( e.extra_len ) +
			get_le16( e.comment_len );
}

void Zip_Extractor::update_info()
{
	while ( 1 )
	{
		const entry_t& e = *(entry_t*) (catalog + offset);
		if ( !memcmp( e.type, "PK\1\2", 4 ) )
		{
			int len = get_le16( e.filename_len );
			const char* name_in = catalog + offset + sizeof e;
			long size = get_le32( e.size );
			if ( size == 0 && name_in [len - 1] == '/' )
			{
				next_item();
				continue; // skip dirs
			}
			
			size_ = size;
			timestamp_ = dostime_to_timestamp( get_le16( e.time ) | ( get_le16( e.date ) << 16 ) );
			if ( len >= (int) sizeof name_copy )
				len = sizeof name_copy - 1;
			name_ = name_copy;
			name_copy [len] = 0;
			memcpy( name_copy, catalog + offset + sizeof e, len );
		}
		else
		{
			assert( !memcmp( e.type, "PK\5\6", 4 ) );
			done_ = 1;
		}
		break;
	}
}

Zip_Extractor::error_t Zip_Extractor::next()
{
	assert( !done() );
	next_item();
	update_info();
	return 0;
}

Zip_Extractor::error_t Zip_Extractor::extract( void* out, long out_size )
{
	assert( out_size <= size_ );
	
	const entry_t& e = *(entry_t*) (catalog + offset);
	if ( get_le16( e.vers ) > 20 )
		return "Zip file uses unsupported compression";
	
	header_t h;
	RETURN_ERR( reader->seek( get_le32( e.file_offset ) ) );
	RETURN_ERR( reader->read( &h, sizeof h ) );
	assert( !memcmp( h.type, "PK\3\4", 4 ) );
	RETURN_ERR( reader->skip( get_le16( h.filename_len ) + get_le16( h.extra_len ) ) );
	long raw_size = get_le32( e.raw_size );
	
	int method = get_le16( e.method );
	if ( method == Z_DEFLATED )
	{
		void* data = malloc( raw_size );
		if ( !data )
			return "Out of memory";
		
		error_t err = reader->read( data, raw_size );
		if ( !err )
		{
			static z_stream default_stream = { };
			z_stream z = default_stream;
			int zerr = inflateInit2( &z, -MAX_WBITS );
			if ( !zerr )
			{
				z.next_in = (byte*) data;
				z.avail_in = raw_size;
				z.next_out = (byte*) out;
				z.avail_out = out_size;
				zerr = inflate( &z, (out_size == size_) ? Z_FINISH : Z_SYNC_FLUSH );
				if ( zerr == Z_STREAM_END )
					zerr = 0;
				inflateEnd( &z );
				assert( z.avail_out == 0 );
			}
			
			if ( zerr )
				err = zError( zerr );
		}
		
		free( data );
		if ( err )
			return err;
		
		if ( out_size == size_ )
		{
			// crc checked only if all data was read
			unsigned long crc = crc32( 0, 0, 0 );
			crc = crc32( crc, (Bytef*) out, out_size );
			if ( crc != get_le32( e.crc ) )
				return "CRC error in zip file";
		}
	}
	else if ( method == 0 )
	{
		assert( raw_size == size_ );
		RETURN_ERR( reader->read( out, out_size ) );
	}
	else
	{
		return "Unsupported compression method in zip file";
	}
	
	return 0;
}

