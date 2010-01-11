#include <foobar2000.h>

#include <inttypes.h>
#include <a52.h>

#include "crc16.h"

#include "ac3_parser.h"

parser_ac3::parser_ac3()
{
	buffer.set_size( 4096 );
	reset();
}

void parser_ac3::reset()
{
	packet_size = 0;
	buffer_filled = 0;
}

void parser_ac3::buffer_fill( service_ptr_t<file> & p_file, abort_callback & p_abort )
{
	buffer_filled += p_file->read( buffer.get_ptr() + buffer_filled, 4096 - buffer_filled, p_abort );
}

void parser_ac3::buffer_remove( size_t amount )
{
	buffer_filled -= amount;
	if ( buffer_filled )
	{
		t_uint8 * ptr = buffer.get_ptr();
		memcpy( ptr, ptr + amount, buffer_filled );
	}
}

unsigned parser_ac3::sync( service_ptr_t<file> & p_file, int & flags, int & sample_rate, int & bit_rate, abort_callback & p_abort )
{
	unsigned i, j;
	unsigned fault_limit = 65536;
	t_uint8 * ptr = buffer.get_ptr();
	while ( fault_limit )
	{
		for ( i = 0, j = buffer_filled; j >= 2 && fault_limit; i++, j--, fault_limit-- )
		{
			if ( ptr [i] == 0x0B && ptr [i + 1] == 0x77 ) break;
		}
		if ( fault_limit )
		{
			if ( i ) buffer_remove( i );
			buffer_fill( p_file, p_abort );
			if ( j >= 2 )
			{
				if ( buffer_filled < 7 ) throw exception_io_data_truncation();
				unsigned block_size = a52_syncinfo( ptr, &flags, &sample_rate, &bit_rate);
				if ( ! block_size || buffer_filled < block_size )
				{
					buffer_remove( 1 );
					fault_limit--;
					continue;
				}
				hasher_crc16 crc;
				crc.process( ptr + 2, block_size - 2 );
				if ( crc.get_result() )
				{
					buffer_remove( 1 );
					fault_limit--;
					continue;
				}
				packet_size = block_size;
				break;
			}
		}
	}
	if ( ! fault_limit ) throw exception_io_data();
	return packet_size;
}

void parser_ac3::packet_flush()
{
	buffer_remove( packet_size );
	packet_size = 0;
}