#include <foobar2000.h>

#include "dts_parser.h"

parser_dts::parser_dts()
{
	buffer.set_size( 4096 );
	dca_temp = dca_init(0);
	reset();
}

parser_dts::~parser_dts()
{
	dca_free(dca_temp);
}

void parser_dts::reset()
{
	packet_size = 0;
	buffer_filled = 0;
}

void parser_dts::buffer_fill( service_ptr_t<file> & p_file, abort_callback & p_abort )
{
	if ( buffer_filled == 4096 ) return;
	t_size read = p_file->read( buffer.get_ptr() + buffer_filled, 4096 - buffer_filled, p_abort );
	buffer_filled += read;
	//if ( !read ) throw exception_io_data_truncation();
}

void parser_dts::buffer_remove( size_t amount )
{
	buffer_filled -= amount;
	if ( buffer_filled )
	{
		t_uint8 * ptr = buffer.get_ptr();
		memcpy( ptr, ptr + amount, buffer_filled );
	}
}

unsigned parser_dts::sync( service_ptr_t<file> & p_file, int & flags, int & sample_rate, int & bit_rate, int & frame_length, abort_callback & p_abort )
{
	unsigned i, j;
	unsigned fault_limit = 65536;
	t_uint8 * ptr = buffer.get_ptr();
	while ( fault_limit )
	{
		for ( i = 0, j = buffer_filled; j >= 6 && fault_limit; i++, j--, fault_limit-- )
		{
			if (ptr[0] == 0xff && ptr[1] == 0x1f &&
				ptr[2] == 0x00 && ptr[3] == 0xe8 &&
				(ptr[4] & 0xf0) == 0xf0 && ptr[5] == 0x07)
			{
				break;
			}

			if (ptr[0] == 0x1f && ptr[1] == 0xff &&
				ptr[2] == 0xe8 && ptr[3] == 0x00 &&
				ptr[4] == 0x07 && (ptr[5] & 0xf0) == 0xf0)
			{
				break;
			}

			if (ptr[0] == 0xfe && ptr[1] == 0x7f &&
				ptr[2] == 0x01 && ptr[3] == 0x80)
			{
				break;
			}

			/* 16 bits and big endian bitstream */
			if (ptr[0] == 0x7f && ptr[1] == 0xfe &&
				ptr[2] == 0x80 && ptr[3] == 0x01)
			{
				break;
			}
		}
		if ( fault_limit )
		{
			if ( i ) buffer_remove( i );
			buffer_fill( p_file, p_abort );
			if ( j >= 2 )
			{
				if ( buffer_filled < 14 ) throw exception_io_data_truncation();
				unsigned block_size = dca_syncinfo( dca_temp, ptr, &flags, &sample_rate, &bit_rate, &frame_length );
				if ( ! block_size || buffer_filled < block_size )
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

void parser_dts::packet_flush()
{
	buffer_remove( packet_size );
	packet_size = 0;
}