#include <limits.h>

#include "reader.h"

foobar_File_Reader::foobar_File_Reader( const service_ptr_t<file> & p_file, abort_callback & p_abort ) : m_file( p_file ), m_abort( p_abort )
{
	remain_ = m_file->get_size_ex( m_abort ) - m_file->get_position( m_abort );
}

long foobar_File_Reader::read_avail( void * p, long s )
{
	if (t_filesize(s) > remain_) s = long(remain_);
	remain_ -= t_filesize(s);
	return m_file->read( p, s , m_abort );
}

Data_Reader::error_t foobar_File_Reader::read( void * p, long s )
{
	remain_ -= t_filesize(s);
	if ( m_file->read( p, s , m_abort ) == s )
		return NULL;
	return "Read returned fewer bytes than requested";
}

long foobar_File_Reader::remain() const
{
	if (remain_ > INT_MAX) return INT_MAX;
	return long(remain_);
}

Data_Reader::error_t foobar_File_Reader::skip( long n )
{
	remain_ -= t_filesize(n);
	m_file->seek_ex(n, file::seek_from_current, m_abort);
	if (remain_ >= 0) return NULL;
	return "Skipped past end of file";
}