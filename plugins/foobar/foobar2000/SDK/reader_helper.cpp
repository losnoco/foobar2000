#include "foobar2000.h"

t_io_result stream_writer_chunk::write(const void * p_buffer,unsigned p_bytes,unsigned & p_bytes_written,abort_callback & p_abort)
{
	unsigned remaining = p_bytes, written = 0;
	while(remaining > 0)
	{
		unsigned delta = sizeof(m_buffer) - m_buffer_state;
		if (delta > remaining) delta = remaining;
		memcpy(m_buffer,(const unsigned char*)p_buffer + written,delta);
		written += delta;
		remaining -= delta;

		if (m_buffer_state == sizeof(m_buffer))
		{
			t_io_result status;
			status = m_writer->write_lendian_t((unsigned char)m_buffer_state,p_abort);
			if (io_result_failed(status)) return status;
			status = m_writer->write_object(m_buffer,m_buffer_state,p_abort);
			m_buffer_state = 0;
			if (io_result_failed(status)) return status;
		}
	}
	p_bytes_written = written;
	return io_result_success;	
}

t_io_result stream_writer_chunk::flush(abort_callback & p_abort)
{
	t_io_result status;
	status = m_writer->write_lendian_t((unsigned char)m_buffer_state,p_abort);
	if (io_result_failed(status)) return status;
	if (m_buffer_state > 0)
	{
		status = m_writer->write_object(m_buffer,m_buffer_state,p_abort);
		m_buffer_state = 0;
		if (io_result_failed(status)) return status;
	}
	return io_result_success;
}
	
/*
	stream_writer * m_writer;
	unsigned m_buffer_state;
	unsigned char m_buffer[255];
*/

t_io_result stream_reader_chunk::read(void * p_buffer,unsigned p_bytes,unsigned & p_bytes_read,abort_callback & p_abort)
{
	unsigned todo = p_bytes, done = 0;
	while(todo > 0)
	{
		if (m_buffer_size == m_buffer_state)
		{
			if (m_eof) break;
			unsigned char temp;
			t_io_result status;
			status = m_reader->read_lendian_t(temp,p_abort);
			if (io_result_failed(status)) return status;
			m_buffer_size = temp;
			if (temp != sizeof(m_buffer)) m_eof = true;
			m_buffer_state = 0;
			if (m_buffer_size>0)
			{
				status = m_reader->read_object(m_buffer,m_buffer_size,p_abort);
				if (io_result_failed(status)) return status;
			}
		}


		unsigned delta = m_buffer_size - m_buffer_state;
		if (delta > todo) delta = todo;
		if (delta > 0)
		{
			memcpy((unsigned char*)p_buffer + done,m_buffer + m_buffer_state,delta);
			todo -= delta;
			done += delta;
			m_buffer_state += delta;
		}

	}
	p_bytes_read = done;
	return p_bytes == done ? io_result_success : io_result_eof;
}

t_io_result stream_reader_chunk::flush(abort_callback & p_abort)
{
	while(!m_eof)
	{
		unsigned char temp;
		t_io_result status;
		status = m_reader->read_lendian_t(temp,p_abort);
		if (io_result_failed(status)) return status;
		m_buffer_size = temp;
		if (temp != sizeof(m_buffer)) m_eof = true;
		m_buffer_state = 0;
		if (m_buffer_size>0)
		{
			status = m_reader->skip_object(m_buffer_size,p_abort);
			if (io_result_failed(status)) return status;
		}
	}
	return io_result_success;
}

/*
	stream_reader * m_reader;
	unsigned m_buffer_state, m_buffer_size;
	bool m_eof;
	unsigned char m_buffer[255];
*/

t_io_result stream_reader_chunk::g_skip(stream_reader * p_stream,abort_callback & p_abort)
{
	return stream_reader_chunk(p_stream).flush(p_abort);
}

t_io_result stream_writer_wrapper_write_config_callback::write(const void * p_buffer,unsigned p_bytes,unsigned & p_bytes_done,abort_callback & p_abort)
{
	if (p_abort.is_aborting()) return io_result_aborted;
	m_out->write(p_buffer,p_bytes);
	p_bytes_done = p_bytes;
	return io_result_success;
}


t_io_result reader_membuffer_base::read(void * p_buffer,unsigned p_bytes,unsigned & p_bytes_read,abort_callback & p_abort)
{
	t_io_result status = io_result_success;
	unsigned max = get_buffer_size();
	if (max < m_offset) return io_result_error_generic;
	max -= m_offset;
	unsigned delta = p_bytes;
	if (delta > max) {delta = max;status = io_result_eof;}
	memcpy(p_buffer,(char*)get_buffer() + m_offset,delta);
	m_offset += delta;
	p_bytes_read = delta;
	return status;
}

t_io_result reader_membuffer_base::seek(t_filesize position,abort_callback & p_abort)
{
	if (p_abort.is_aborting()) return io_result_aborted;
	t_filesize max = get_buffer_size();
	if (position == filesize_invalid || position > max) return io_result_error_generic;
	m_offset = (unsigned)position;
	return io_result_success;
}
