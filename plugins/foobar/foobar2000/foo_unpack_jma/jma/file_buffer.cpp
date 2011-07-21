#include <foobar2000.h>
#include <string.h>

#include "file_buffer.h"

file_buffer::file_buffer( t_filetimestamp p_timestamp )
	: m_timestamp( p_timestamp ), m_position( 0 )
{
}

t_size file_buffer::read( void * p_buffer, t_size p_bytes, abort_callback & )
{
	t_size bytes_to_read = m_buffer.get_count() - m_position;
	if ( m_position > m_buffer.get_count() ) bytes_to_read = 0;
	if ( bytes_to_read > p_bytes ) bytes_to_read = p_bytes;
	memcpy( p_buffer, m_buffer.get_ptr() + m_position, bytes_to_read );
	m_position += bytes_to_read;
	return bytes_to_read;
}

void file_buffer::write( const void * p_buffer, t_size p_bytes, abort_callback & )
{
	t_size buffer_size = m_position + p_bytes;
	m_buffer.grow_size( buffer_size );
	memcpy( m_buffer.get_ptr() + m_position, p_buffer, p_bytes );
	m_position += p_bytes;
}

t_filesize file_buffer::get_size( abort_callback & )
{
	return m_buffer.get_count();
}

t_filesize file_buffer::get_position( abort_callback & )
{
	return m_position;
}

void file_buffer::resize( t_filesize p_size, abort_callback & )
{
	m_buffer.set_count( p_size );
	if ( m_position > p_size ) m_position = p_size;
}

void file_buffer::seek( t_filesize p_position, abort_callback & )
{
	m_position = p_position;
	if ( m_position > m_buffer.get_count() ) m_position = m_buffer.get_count();
}

bool file_buffer::can_seek()
{
	return true;
}

bool file_buffer::get_content_type( pfc::string_base & )
{
	return false;
}

bool file_buffer::is_in_memory()
{
	return true;
}

void file_buffer::on_idle( abort_callback & )
{
}

t_filetimestamp file_buffer::get_timestamp( abort_callback & )
{
	return m_timestamp;
}

void file_buffer::reopen( abort_callback & )
{
	m_position = 0;
}

bool file_buffer::is_remote()
{
	return false;
}
