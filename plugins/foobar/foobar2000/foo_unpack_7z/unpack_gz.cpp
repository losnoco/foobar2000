#include <foobar2000.h>

#include <fex/Gzip_Reader.h>

#include "file_interface.h"

static void handle_error( const char * str )
{
	if ( str ) throw exception_io_data( str );
}

class unpacker_gz : public unpacker
{
public:
	virtual void open( service_ptr_t< file > & p_out, const service_ptr_t< file > & p_source, abort_callback & p_abort )
	{
		if ( p_source.is_empty() ) throw exception_io_data();

		foobar_File_Reader in( p_source, p_abort );
		Gzip_Reader ingz;
		handle_error( ingz.open( &in ) );

		filesystem::g_open_tempmem( p_out, p_abort );

		while ( ingz.remain() )
		{
			unsigned char buffer[1024];

			uint64_t to_read = ingz.remain();
			if ( to_read > 1024 ) to_read = 1024;
			handle_error( ingz.read( buffer, (long)to_read ) );
			p_out->write( buffer, (t_size)to_read, p_abort );
		}

		p_out->reopen( p_abort );
	}
};

static unpacker_factory_t< unpacker_gz > g_unpacker_gz_factory;

DECLARE_COMPONENT_VERSION( "GZIP reader", "1.1", "" );