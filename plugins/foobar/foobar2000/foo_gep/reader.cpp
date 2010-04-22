#include <limits.h>

#include "reader.h"

foobar_Data_Reader::foobar_Data_Reader( const service_ptr_t<file> & p_file, abort_callback & p_abort ) : m_file( p_file ), m_abort( p_abort )
{
	t_filesize remain = m_file->get_size_ex( m_abort ) - m_file->get_position( m_abort );
	if ( remain > INT_MAX ) remain = INT_MAX;
	set_remain( remain );
}

blargg_err_t foobar_Data_Reader::read_v( void * p, int s )
{
	if ( m_file->read( p, s , m_abort ) == s )
		return blargg_ok;
	return "Read returned fewer bytes than requested";
}

blargg_err_t foobar_Data_Reader::skip_v( int n )
{
	m_file->seek_ex(n, file::seek_from_current, m_abort);
	return blargg_ok;
}

foobar_File_Reader::foobar_File_Reader( const service_ptr_t<file> & p_file, abort_callback & p_abort ) : m_file( p_file ), m_abort( p_abort )
{
	set_size( p_file->get_size_ex( p_abort ) );
	set_tell( p_file->get_position( p_abort ) );
}

blargg_err_t foobar_File_Reader::read_v( void * p, int s )
{
	if ( m_file->read( p, s , m_abort ) == s )
		return blargg_ok;
	return "Read returned fewer bytes than requested";
}

blargg_err_t foobar_File_Reader::skip_v( int n )
{
	m_file->seek_ex(n, file::seek_from_current, m_abort);
	return blargg_ok;
}

blargg_err_t foobar_File_Reader::seek_v( BOOST::uint64_t n )
{
	m_file->seek( n, m_abort );
	return 0;
}
