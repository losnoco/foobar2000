#include "file_interface.h"

#include <fex/blargg_errors.h>

foobar_File_Reader::foobar_File_Reader( const service_ptr_t<file> & p_file, abort_callback & p_abort ) : m_file( p_file ), m_abort( p_abort )
{
	set_size( p_file->get_size_ex( p_abort ) );
	set_tell( p_file->get_position( p_abort ) );
}

blargg_err_t foobar_File_Reader::read_v( void* p, int n )
{
	try
	{
		m_file->read_object( p, n, m_abort );
		return 0;
	}
	catch (...)
	{
		return blargg_err_file_read;
	}
}
	
blargg_err_t foobar_File_Reader::skip_v( int n )
{
	try
	{
		m_file->seek_ex( n, foobar2000_io::file::seek_from_current, m_abort );
		return 0;
	}
	catch (...)
	{
		return blargg_err_file_io;
	}
}

blargg_err_t foobar_File_Reader::seek_v( BOOST::uint64_t n )
{
	try
	{
		m_file->seek( n, m_abort );
		return 0;
	}
	catch (...)
	{
		return blargg_err_file_io;
	}
}
