#include <limits.h>

#include "reader.h"

foobar_File_Reader::foobar_File_Reader( const service_ptr_t<file> & p_file, abort_callback & p_abort ) : m_file( p_file ), m_abort( p_abort )
{
	t_filesize remain = m_file->get_size_ex( m_abort ) - m_file->get_position( m_abort );
	if ( remain > INT_MAX ) remain = INT_MAX;
	set_remain( remain );
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