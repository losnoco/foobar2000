#include "stdafx.h"

unsigned file_wrapper_simple::read(void * p_buffer,unsigned p_bytes)
{
	if (io_result_failed(m_status)) return 0;
	unsigned done = 0;
	m_status = m_file->read(p_buffer,p_bytes,done,m_abort);
	if (io_result_failed(m_status)) return 0;
	return done;
}

unsigned file_wrapper_simple::write(const void * p_buffer,unsigned p_bytes)
{
	if (io_result_failed(m_status)) return 0;
	unsigned done = 0;
	m_status = m_file->write(p_buffer,p_bytes,done,m_abort);
	if (io_result_failed(m_status)) return 0;
	return done;
}

bool file_wrapper_simple::seek(t_uint64 p_offset)
{
	if (io_result_failed(m_status)) return false;
	m_status = m_file->seek(p_offset,m_abort);
	if (io_result_failed(m_status)) return false;
	return true;
}

t_uint64 file_wrapper_simple::get_position()
{
	t_uint64 ret;
	if (io_result_failed(m_status)) return (t_uint64)(-1);
	m_status = m_file->get_position(ret,m_abort);
	if (io_result_failed(m_status)) return (t_uint64)(-1);
	return ret;
}

bool file_wrapper_simple::truncate()
{
	if (io_result_failed(m_status)) return false;
	m_status = m_file->set_eof(m_abort);
	if (io_result_failed(m_status)) return false;
	return true;
}


t_uint64 file_wrapper_simple::get_size()
{
	t_uint64 ret;
	if (io_result_failed(m_status)) return (t_uint64)(-1);
	m_status = m_file->get_size(ret,m_abort);
	if (io_result_failed(m_status)) return (t_uint64)(-1);
	return ret;

}

bool file_wrapper_simple::can_seek()
{
	if (io_result_failed(m_status)) return false;
	return m_file->can_seek();
}
