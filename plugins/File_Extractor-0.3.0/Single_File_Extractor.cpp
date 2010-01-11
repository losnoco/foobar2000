
// File_Extractor 0.3.0. http://www.slack.net/~ant/

#include "Single_File_Extractor.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>

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

typedef File_Extractor::error_t error_t;

Single_File_Extractor::Single_File_Extractor() { }

Single_File_Extractor::~Single_File_Extractor() { close(); }
	
void Single_File_Extractor::set_name( const char* n )
{
	strncpy( name, n, sizeof name );
}

File_Extractor* extractor_open_file( const char* path, const char** err, const char* name )
{
	Single_File_Extractor* sfe = new Single_File_Extractor;
	if ( sfe )
		sfe->set_name( name ? name : path );
	return extractor_open_( sfe, path, err );
}

error_t Single_File_Extractor::open_()
{
	name_ = name;
	return rewind();
}

error_t Single_File_Extractor::next()
{
	done_ = 1;
	return 0;
}

error_t Single_File_Extractor::rewind()
{
	done_ = 0;
	error_t err = reader->seek( 0 );
	if ( err )
		return err;
	size_ = reader->remain();
	return 0;
}

error_t Single_File_Extractor::extract( Data_Writer& out )
{
	int const buf_size = 32 * 1024L;
	void* buf = malloc( buf_size );
	if ( !buf )
		return "Out of memory";
	
	long count = reader->remain();
	error_t err = 0;
	while ( count && !err )
	{
		long n = count;
		if ( n > buf_size )
			n = buf_size;
		count -= n;
		
		err = reader->read( buf, n );
		if ( !err )
			err = out.write( buf, n );
	}
	
	free( buf );
	
	return err;
}

void Single_File_Extractor::close_() { }

