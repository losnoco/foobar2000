#include "stdafx.h"

enum {backread_on_seek = 1024};

bool seekabilizer_backbuffer::initialize(unsigned p_size)
{
	m_depth = m_cursor = 0;

	return m_buffer.set_size(p_size) != 0;
}

void seekabilizer_backbuffer::write(const void * p_buffer,unsigned p_bytes)
{
	if (p_bytes >= m_buffer.get_size())
	{
		memcpy(m_buffer.get_ptr(),(const t_uint8*)p_buffer + p_bytes - m_buffer.get_size(),m_buffer.get_size());
		m_cursor = 0;
		m_depth = m_buffer.get_size();
	}
	else
	{
		const t_uint8* sourceptr = (const t_uint8*) p_buffer;
		unsigned remaining = p_bytes;
		while(remaining > 0)
		{
			unsigned delta = m_buffer.get_size() - m_cursor;
			if (delta > remaining) delta = remaining;

			memcpy(m_buffer.get_ptr() + m_cursor,sourceptr,delta);

			sourceptr += delta;
			remaining -= delta;
			m_cursor = (m_cursor + delta) % m_buffer.get_size();

			m_depth = pfc::min_t<unsigned>(m_buffer.get_size(),m_depth + delta);
			
		}
	}
}

void seekabilizer_backbuffer::read(unsigned p_backlogdepth,void * p_buffer,unsigned p_bytes) const
{
	assert(p_backlogdepth <= m_depth);
	assert(p_backlogdepth >= p_bytes);

		
	t_uint8* targetptr = (t_uint8*) p_buffer;
	unsigned remaining = p_bytes;
	unsigned cursor = (m_cursor + m_buffer.get_size() - p_backlogdepth) % m_buffer.get_size();
	
	while(remaining > 0)
	{
		unsigned delta = m_buffer.get_size() - cursor;
		if (delta > remaining) delta = remaining;
		
		memcpy(targetptr,m_buffer.get_ptr() + cursor,delta);

		targetptr += delta;
		remaining -= delta;
		cursor = (cursor + delta) % m_buffer.get_size();
	}
}

unsigned seekabilizer_backbuffer::get_depth() const
{
	return m_depth;
}

unsigned seekabilizer_backbuffer::get_max_depth() const
{
	return m_buffer.get_size();
}

void seekabilizer_backbuffer::reset()
{
	m_depth = m_cursor = 0;
}


t_io_result seekabilizer::initialize(service_ptr_t<file> p_base,unsigned p_buffer_size,abort_callback & p_abort)
{
	if (!m_buffer.initialize(p_buffer_size)) return io_result_error_out_of_memory;

	m_file = p_base;

	m_position = m_position_base = 0;
	
	t_io_result status;
	status = m_file->get_size(m_size,p_abort);
	if (io_result_failed(status)) return status;

	return io_result_success;
}

void seekabilizer::g_seekabilize_e(service_ptr_t<file> & p_reader,unsigned p_buffer_size,abort_callback & p_abort)
{
	t_io_result status;
	status = g_seekabilize(p_reader,p_buffer_size,p_abort);
	if (io_result_failed(status)) throw status;
}

t_io_result seekabilizer::g_seekabilize(service_ptr_t<file> & p_reader,unsigned p_buffer_size,abort_callback & p_abort)
{
	if (p_reader.is_valid() && p_reader->is_remote() && p_buffer_size > 0)
	{
		service_ptr_t<seekabilizer> instance = new service_impl_t<seekabilizer>();
		if (instance.is_empty()) return io_result_error_out_of_memory;
		
		t_io_result status;
		status = instance->initialize(p_reader,p_buffer_size,p_abort);
		if (io_result_failed(status)) return status;
		
		p_reader = instance.get_ptr();
	}

	return io_result_success;
}

t_io_result seekabilizer::read(void * p_buffer,unsigned p_bytes,unsigned & p_bytes_read,abort_callback & p_abort)
{
	if (p_abort.is_aborting()) return io_result_aborted;

	if (m_position > m_position_base + pfc::max_t<unsigned>(m_buffer.get_max_depth(),backread_on_seek) && m_file->can_seek())
	{
		t_io_result status;
		t_filesize target = m_position;
		if (target < backread_on_seek) target = 0;
		else target -= backread_on_seek;
		status = m_file->seek(target,p_abort);
		if (io_result_failed(status)) return status;
		m_position_base = target;
	}

	//seek ahead
	while(m_position > m_position_base)
	{
		enum {tempsize = 1024};
		t_uint8 temp[tempsize];
		unsigned delta = (unsigned) pfc::min_t<t_filesize>(tempsize,m_position - m_position_base);
		t_io_result status;
		unsigned bytes_read = 0;
		status = m_file->read(temp,delta,bytes_read,p_abort);
		if (io_result_failed(status)) return status;
		m_buffer.write(temp,bytes_read);
		m_position_base += bytes_read;

		if (bytes_read < delta)
		{
			p_bytes_read = 0;
			return io_result_eof;
		}
	}

	unsigned done = 0;
	t_uint8 * targetptr = (t_uint8*) p_buffer;

	//try to read backbuffer
	if (m_position < m_position_base)
	{
		if (m_position_base - m_position > (t_filesize)m_buffer.get_depth()) return io_result_error_generic;
		unsigned backread_depth = (unsigned) (m_position_base - m_position);
		unsigned delta = pfc::min_t<unsigned>(backread_depth,p_bytes-done);
		m_buffer.read(backread_depth,targetptr,delta);
		done += delta;
		m_position += delta;
	}

	//regular read
	if (done < p_bytes)
	{
		t_io_result status;
		unsigned bytes_read = 0;
		status = m_file->read(targetptr+done,p_bytes-done,bytes_read,p_abort);
		if (io_result_failed(status)) return status;

		m_buffer.write(targetptr+done,bytes_read);

		done += bytes_read;
		m_position += bytes_read;
		m_position_base += bytes_read;
	}

	p_bytes_read = done;
	return done == p_bytes ? io_result_success : io_result_eof;
}

t_io_result seekabilizer::write(const void * p_buffer,unsigned p_bytes,unsigned & p_bytes_written,abort_callback & p_abort)
{
	return io_result_error_denied;
}

t_io_result seekabilizer::get_size(t_filesize & p_size,abort_callback & p_abort)
{
	if (p_abort.is_aborting()) return io_result_aborted;
	p_size = m_size;
	return io_result_success;
}

t_io_result seekabilizer::get_position(t_uint64 & p_position,abort_callback & p_abort)
{
	if (p_abort.is_aborting()) return io_result_aborted;
	p_position = m_position;
	return io_result_success;
}

t_io_result seekabilizer::set_eof(abort_callback & p_abort)
{
	return io_result_error_denied;
}

t_io_result seekabilizer::seek(t_filesize p_position,abort_callback & p_abort)
{
	assert(m_position_base >= m_buffer.get_depth());

	if (m_size != filesize_invalid && p_position > m_size) return io_result_error_generic;

	t_filesize lowest = m_position_base - m_buffer.get_depth();

	if (p_position < lowest)
	{
		if (m_file->can_seek())
		{
			m_buffer.reset();
			t_filesize target = p_position;
			unsigned delta = m_buffer.get_max_depth();
			if (delta > backread_on_seek) delta = backread_on_seek;
			if (target > delta) target -= delta;
			else target = 0;
			t_io_result status = m_file->seek(target,p_abort);
			if (io_result_failed(status)) return status;
			m_position_base = target;
		}
		else
		{
			m_buffer.reset();
			t_io_result status = m_file->reopen(p_abort);
			if (io_result_failed(status)) return status;
			m_position_base = 0;
		}
	}

	m_position = p_position;

	return io_result_success;
}

bool seekabilizer::can_seek()
{
	return true;
}

bool seekabilizer::get_content_type(string_base & p_out) {return m_file->get_content_type(p_out);}

bool seekabilizer::is_in_memory() {return false;}

void seekabilizer::on_idle(abort_callback & p_abort) {return m_file->on_idle(p_abort);}

t_io_result seekabilizer::get_timestamp(t_filetimestamp & p_timestamp,abort_callback & p_abort)
{
	return m_file->get_timestamp(p_timestamp,p_abort);
}

t_io_result seekabilizer::reopen(abort_callback & p_abort)
{
	if (m_position_base - m_buffer.get_depth() == 0)
	{
		return seek(0,p_abort);
	}
	m_position = m_position_base = 0;
	m_buffer.reset();
	return m_file->reopen(p_abort);
}

bool seekabilizer::is_remote()
{
	return m_file->is_remote();
}
