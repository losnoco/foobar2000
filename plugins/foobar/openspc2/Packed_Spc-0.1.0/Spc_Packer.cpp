
// Packed_Spc 0.1.0. Shay Green <hotpop.com@blargg>

#include "Spc_Packer.h"

#include "unpack_spc.h" // get packed spc format

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

#include "zlib/zlib.h"

/* Library Copyright (C) 2004 Shay Green. This library is free software;
you can redistribute it and/or modify it under the terms of the GNU Lesser
General Public License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version. This
library is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details. You should have received a copy of the GNU Lesser General
Public License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA */

struct spc_file_t {
	char          header [0x100];
	unsigned char ram [0x10000];
	unsigned char dsp [0x80];
};

Spc_Packer::Spc_Packer()
{
	shared = NULL;
	files = NULL;
	preserve_echo_buffer_ = false;
	
	// add entries for 0x00 and 0xFF filled blocks
	shared_map.set( 0, 0 );
	shared_map.set( max_checksum, 0xFF );
	checksum_map.set( 0, 0 );
	checksum_map.set( max_checksum, 0xFF );
}

Spc_Packer::~Spc_Packer()
{
	if ( shared ) {
		free( shared->checksums ); // only shared file allocates this separately
		free( shared );
	}
	
	while ( files ) {
		file_t* f = files;
		files = f->next;
		free( f );
	}
}

const char* Spc_Packer::init( const char* shared_filename, const char* output_dir_ )
{
	assert( !shared );
	
	strcpy( this->output_dir, output_dir_ );
	
	// create entry for shared file
	shared = (file_t*) malloc( offsetof (file_t,data) );
	if ( !shared )
		return "Out of memory";
	shared->file_size = 0;
	shared->size = 0;
	shared->next = NULL;
	strcpy( shared->out_path, shared_filename );
	shared->checksums = (checksum_t*) malloc( 1 );
	if ( !shared->checksums )
		return "Out of memory";
	
	return NULL; // success
}

static unsigned calc_checksum( const unsigned char* p, size_t s ) {
	unsigned checksum = 0;
	while ( s-- )
		checksum += *p++;
	return checksum;
}

const char* Spc_Packer::add_spc( const void* data, long size, const char* out_filename )
{
	// create entry for file
	int block_count = ram_size / block_size;
	file_t* file = (file_t*) malloc( offsetof (file_t,data [size]) );
	if ( !file )
		return "Out of memory";
	file->next = files;
	files = file;
	
	strcpy( file->out_path, out_filename );
	file->size = ram_size;
	file->file_size = size;
	memcpy( file->file_data, data, size );
	
	// optionally clear echo
	if ( !preserve_echo_buffer_ ) {
		spc_file_t* spc = (spc_file_t*) file->file_data;
		if ( !(spc->dsp [0x6C] & 0x20) ) {
			unsigned start = spc->dsp [0x6D] * 0x100;
			unsigned len = spc->dsp [0x7D] * 0x800;
			if ( start + len > ram_size )
				len = ram_size - start;
			memset( spc->ram + start, 0, len );
		}
	}
	
	// calculate checksums
	file->checksums = file->checksums_;
	for ( int i = 0; i < block_count; i++ ) {
		const byte* begin = &file->data [i * (long) block_size];
		checksum_t checksum = calc_checksum( begin, block_size );
		assert( checksum <= max_checksum );
		file->checksums [i] = checksum;
		checksum_map.set( checksum, *begin );
	}
	
	return NULL; // success
}

bool Spc_Packer::match_data( unsigned sum, byte* begin, byte* pos, byte* end,
		file_t* file, result_t* result )
{
	for ( ; file; file = file->next )
	{
		const checksum_t* checksums = file->checksums;
		byte* src_beg = file->data;
		for ( int n = file->size / (unsigned) block_size; n; n-- )
		{
			src_beg += block_size; // overlap with compare
			if ( *checksums++ != sum )
				continue;
			
			src_beg -= block_size;
			
			// find end of match
			byte* src_end = src_beg;
			byte* dest_end = pos;
			while ( src_end - file->data < file->size && dest_end < end &&
					*src_end == *dest_end )
			{
				++src_end;
				++dest_end;
			}
			
			if ( dest_end - pos < block_size )
				continue; // not a big enough match
			
			// find beginning of match
			while ( src_beg > file->data && pos > begin && src_beg [-1] == pos [-1] ) {
				--src_beg;
				--pos;
			}
			
			// fill out result
			result->file = file;
			result->skip = pos - begin;
			result->offset = src_beg - file->data;
			result->size = dest_end - pos;
			
			assert( result->skip + result->size <= end - begin );
			
			return true;
		}
	}
	
	return false;
}

long Spc_Packer::add_shared( const byte* data, unsigned size )
{
	long old_size = shared->size;
	
	// resize shared data block
	long new_size = old_size + size;
	file_t* shared = (file_t*) realloc( this->shared, offsetof (file_t,data [new_size]) );
	assert( shared ); // to do: report out of memory
	this->shared = shared;
	shared->size = new_size;
	
	// copy new data
	memcpy( &shared->data [old_size], data, size );
	
	// resize checksums
	int checksum_count = new_size / block_size;
	checksum_t* checksums =
			(checksum_t*) realloc( shared->checksums, checksum_count * sizeof (checksum_t) );
	assert( checksums ); // to do: report out of memory
	shared->checksums = checksums;
	
	// calculate any added checksums 
	for ( int i = old_size / block_size; i < checksum_count; i++ ) {
		const byte* begin = &shared->data [i * (long) block_size];
		checksum_t s = calc_checksum( begin, block_size );
		checksums [i] = s;
		shared_map.set( s, *begin );
	}
	
	return old_size;
}

inline unsigned char* append_block( unsigned char* out, packed_spc_block_t type,
		unsigned size )
{
	*out++ = type;
	*out++ = size;
	*out++ = size >> 8;
	return out;
}

unsigned char* Spc_Packer::match_blocks( byte* out, byte* scan_begin, byte* end, int phase )
{
	// unique data
	if ( phase > 1 || end - scan_begin < block_size )
	{
		unsigned size = end - scan_begin;
		if ( !size )
			return out;
		
		out = append_block( out, packed_spc_unique, size );
		memcpy( out, scan_begin, size );
		return out + size;
	}
	
	byte* const scan_end = end - block_size;
	while ( scan_begin <= scan_end )
	{
		unsigned sum = (checksum_t) calc_checksum( scan_begin, block_size );
		
		// scan until match or end
		const Checksum_Map& map = phase ? checksum_map : shared_map;
		byte* pos = scan_begin;
		while ( true )
		{
			if ( map.check( sum, *pos ) )
			{
				// first byte matches or multiple blocks with this checksum
				if ( sum == 0 || sum == 0xFF * block_size )
				{
					// run of repeated 0xFF or 0x00
					
					// handle data before the run
					out = match_blocks( out, scan_begin, pos, phase + 1 );
					scan_begin = pos;
					
					// find end of run
					int b = *pos;
					assert( b == 0x00 || b == 0xFF );
					while ( pos < end && *pos == b )
						++pos;
					
					// write record
					out = append_block( out,
							b ? packed_spc_0xFF : packed_spc_0x00,
							pos - scan_begin );
					
					scan_begin = pos;
					break;
				}
				
				// compare with all blocks
				result_t result;
				if ( match_data( sum, scan_begin, pos, end, phase ? files : shared, &result ) )
				{
					// Add matched data to shared file if necessary. For some reason
					// if this is done after handling the skip data, less compression results.
					if ( result.file != shared )
						result.offset = add_shared( scan_begin + result.skip, result.size );
					
					// write any unmatched data
					if ( result.skip )
						out = match_blocks( out, scan_begin, scan_begin + result.skip,
								phase + 1 );
					
					// matched block offset and size in shared file
					out = append_block( out, packed_spc_shared, result.size );
					*out++ = result.offset;
					*out++ = result.offset >> 8;
					*out++ = result.offset >> 16;
					
					scan_begin += result.skip + result.size;
					break;
				}
			}
			
			// advance checksum (running sum) one byte forward
			sum -= pos [0];
			sum += pos [block_size];
			
			if ( pos >= scan_end )
				goto end;
			
			++pos;
		}
	}
end:
	assert( scan_begin <= end );
	
	if ( scan_begin < end )
		out = match_blocks( out, scan_begin, end, phase + 1 );
	
	return out;
}

static const char* write_file( const char* dir, const char* name, void* data, size_t size )
{
	char path [256];
	strcpy( path, dir );
	strcat( path, name );
	
	gzFile out = gzopen( path, "wb" );
	if ( !out )
		return "Couldn't create and open output file";
	if ( !gzwrite( out, data, size ) )
		return "Couldn't write to file";
	if ( gzclose( out ) )
		return "Couldn't closing file";
	return NULL; // success
}

const char* Spc_Packer::next_file() const {
	return files ? files->out_path : NULL;
}

const char* Spc_Packer::pack_file()
{
	const char * ret;
	file_t* file = files;
	if ( !file )
		return "No more shared to pack";
	files = file->next; // remove *before* encoding (so it won't get searched)
	
	spc_file_t* spc = (spc_file_t*) file->file_data;
	packed_spc_t* cmp = (packed_spc_t*) file->file_data;
	
	// encode
	unsigned cmp_size = match_blocks( temp_buf, spc->ram, spc->ram + ram_size, 0 ) - temp_buf;
	
	if ( cmp_size >= sizeof cmp->cmp ) // packed version was larger
	{
		ret = write_file( output_dir, file->out_path, file->file_data, file->file_size );
		free( file );
		return ret;
	}
	
	// generate spc data
	unsigned long checksum = crc32( 0, spc->ram, ram_size );
	memset( spc->ram, 0, ram_size );
	memcpy( cmp->cmp, temp_buf, cmp_size );
	assert( spc->header [0xD8] == 0 );
	spc->header [0xD8] |= 1;
	cmp->format = 'P';
	strcpy( cmp->shared_filename, shared->out_path );
	cmp->checksum [0] = checksum;
	cmp->checksum [1] = checksum >> 8;
	cmp->checksum [2] = checksum >> 16;
	cmp->checksum [3] = checksum >> 24;
	
	ret = write_file( output_dir, file->out_path, spc, file->file_size );

	free( file );

	return ret;
}

const char* Spc_Packer::write_shared()
{
	assert( shared );
	return write_file( output_dir, shared->out_path, shared->data, shared->size );
}

