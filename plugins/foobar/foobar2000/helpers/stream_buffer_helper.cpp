#include "stdafx.h"

stream_reader_buffered::stream_reader_buffered(stream_reader * p_base,unsigned p_buffer) : m_base(p_base)
{
	m_buffer.set_size(p_buffer);
	m_buffer_ptr = 0;
	m_buffer_max = 0;
}

t_io_result stream_reader_buffered::read(void * p_buffer,unsigned p_bytes,unsigned & p_bytes_read,abort_callback & p_abort)
{
	char * output = (char*) p_buffer;
	unsigned output_ptr = 0;
	
	while(output_ptr < p_bytes)
	{
		{
			unsigned delta = pfc::min_t(p_bytes - output_ptr, m_buffer_max - m_buffer_ptr);
			if (delta > 0)
			{
				memcpy(output + output_ptr, m_buffer.get_ptr() + m_buffer_ptr, delta);
				output_ptr += delta;
				m_buffer_ptr += delta;
			}
		}

		if (m_buffer_ptr == m_buffer_max)
		{
			unsigned bytes_read;
			t_io_result status;
			status = m_base->read(m_buffer.get_ptr(), m_buffer.get_size(), bytes_read, p_abort);
			if (io_result_failed(status)) return status;
			m_buffer_ptr = 0;
			m_buffer_max = bytes_read;

			if (m_buffer_max == 0) break;
		}
	}		

	p_bytes_read = output_ptr;

	return io_result_success;
}

stream_writer_buffered::stream_writer_buffered(stream_writer * p_base,unsigned p_buffer)
	: m_base(p_base)
{
	m_buffer.set_size(p_buffer);
	m_buffer_ptr = 0;
}
	
t_io_result stream_writer_buffered::write(const void * p_buffer,unsigned p_bytes,unsigned & p_bytes_written,abort_callback & p_abort)
{
	const char * source = (const char*)p_buffer;
	unsigned source_remaining = p_bytes;
	t_io_result status;
	const unsigned buffer_size = m_buffer.get_size();
	if (source_remaining >= buffer_size)
	{
		status = flush(p_abort);
		if (io_result_failed(status)) return status;
		status = m_base->write_object(source,source_remaining,p_abort);
		if (io_result_failed(status)) return status;
		p_bytes_written = p_bytes;
		return io_result_success;
	}

	if (m_buffer_ptr + source_remaining >= buffer_size)
	{
		unsigned delta = buffer_size - m_buffer_ptr;
		memcpy(m_buffer.get_ptr() + m_buffer_ptr, source,delta);
		source += delta;
		source_remaining -= delta;
		m_buffer_ptr += delta;
		status = flush(p_abort);
		if (io_result_failed(status)) return status;
	}

	memcpy(m_buffer.get_ptr() + m_buffer_ptr, source,source_remaining);
	m_buffer_ptr += source_remaining;

	p_bytes_written = p_bytes;
	return io_result_success;
}


void stream_writer_buffered::flush_e(abort_callback & p_abort)
{
	t_io_result status;
	status = flush(p_abort);
	if (io_result_failed(status)) throw status;
}

t_io_result stream_writer_buffered::flush(abort_callback & p_abort)
{
	if (m_buffer_ptr > 0)
	{
		t_io_result status;
		status = m_base->write_object(m_buffer.get_ptr(),m_buffer_ptr,p_abort);
		if (io_result_failed(status)) return status;
		m_buffer_ptr = 0;
	}
	return io_result_success;
}