#include "file_interface.h"

long foobar_File_Reader::read_avail( void* p, long n )
{
	try
	{
		return m_file->read( p, n, m_abort );
	}
	catch (...)
	{
		return -1;
	}
}
	
uint64_t foobar_File_Reader::remain() const
{
	t_filesize remain = 0;
	try
	{
		remain = m_file->get_size_ex( m_abort ) - m_file->get_position( m_abort );
	}
	catch (...) {}
	return remain;
}

uint64_t foobar_File_Reader::size() const
{
	t_filesize size = 0;
	try
	{
		size = m_file->get_size_ex( m_abort );
	}
	catch (...) {}
	return size;
}
	
uint64_t foobar_File_Reader::tell() const
{
	t_filesize position = 0;
	try
	{
		position = m_file->get_position( m_abort );
	}
	catch (...) {}
	return position;
}
	
File_Reader::error_t foobar_File_Reader::seek( uint64_t offset )
{
	try
	{
		m_file->seek( offset, m_abort );
		return 0;
	}
	catch (...)
	{
		return "Seek error";
	}
}

Data_Writer::error_t foobar_Data_Writer::write( const void* p, long n )
{
	try
	{
		m_file->write( p, n, m_abort );
		return 0;
	}
	catch (...)
	{
		return "Write error";
	}
}
