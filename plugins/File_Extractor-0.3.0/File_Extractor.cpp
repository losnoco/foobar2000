
// File_Extractor 0.3.0. http://www.slack.net/~ant/

#include "File_Extractor.h"

#include <assert.h>
#include <stdlib.h>

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

const char not_archive_err [] = "Not a valid archive";

void File_Extractor::init_state()
{
	name_ = 0;
	size_ = 0;
	timestamp_ = 0;
	done_ = 1;
	reader = 0;
	own_file_ = 0;
	scan_only_ = 0;
}

File_Extractor::error_t File_Extractor::extract( Data_Writer& out )
{
	if ( !size_ )
		return 0;
	
	void* p = malloc( size_ );
	if ( !p )
		return "Out of memory";
	error_t err = extract( p, size_ );
	if ( !err )
		err = out.write( p, size_ );
	free( p );
	return err;
}

File_Extractor::error_t File_Extractor::extract( void* out, long count )
{
	Mem_Writer mem( out, count, 1 );
	return extract( mem );
}

File_Extractor::error_t File_Extractor::open( File_Reader* input )
{
	close();
	reader = input;
	error_t err = open_();
	if ( err )
		init_state();
	return err;
}

void File_Extractor::close()
{
	close_();
	if ( own_file_ && reader )
		delete reader;
	init_state();
}

File_Extractor::~File_Extractor() { assert( !own_file_ && !reader ); }

// C interface

File_Extractor* extractor_open_( File_Extractor* fe, const char* path, const char** err_out )
{
	const char* err = "Out of memory";
	Std_File_Reader* in = 0;
	if ( fe )
		in = new Std_File_Reader;
	
	if ( fe && in )
		err = in->open( path );
	
	if ( !err )
		err = fe->open( in );
	
	if ( err_out )
		*err_out = err;
	
	if ( err )
	{
		delete fe;
		delete in;
		return 0;
	}
	
	fe->own_file();
	return fe;
}

int         extractor_done      ( File_Extractor const* fe ) { return fe->done(); }
const char* extractor_name      ( File_Extractor const* fe ) { return fe->name(); }
long        extractor_size      ( File_Extractor const* fe ) { return fe->size(); }
uint64_t    extractor_timestamp ( File_Extractor const* fe ) { return fe->timestamp(); }
const char* extractor_extract   ( File_Extractor* fe, void* out, long count ) { return fe->extract( out, count ); }
const char* extractor_next      ( File_Extractor* fe ) { return fe->next(); }
const char* extractor_rewind    ( File_Extractor* fe ) { return fe->rewind(); }
void        extractor_scan_only ( File_Extractor* fe ) { fe->scan_only(); }
void        extractor_close     ( File_Extractor* fe ) { delete fe; }

