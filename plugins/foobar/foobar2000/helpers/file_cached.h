template<unsigned blocksize>
class file_cached : public file
{
public:

	static void g_create_e(service_ptr_t<file> & p_out,service_ptr_t<file> p_base,abort_callback & p_abort)
	{
		t_io_result status = g_create(p_out,p_base,p_abort);
		if (io_result_failed(status)) throw status;
	}
	static t_io_result g_create(service_ptr_t<file> & p_out,service_ptr_t<file> p_base,abort_callback & p_abort)
	{
		service_ptr_t<file_cached<blocksize> > temp;
		temp = new service_impl_t<file_cached<blocksize> >();
		if (temp.is_empty()) return io_result_error_out_of_memory;
		t_io_result status;
		status = temp->initialize(p_base,p_abort);
		if (io_result_failed(status)) return status;
		p_out = temp.get_ptr();
		return io_result_success;
	}
private:
	t_io_result initialize(service_ptr_t<file> p_base,abort_callback & p_abort)
	{
		t_io_result status;
		m_base = p_base;
		m_position = 0;
		m_can_seek = m_base->can_seek();
		if (m_can_seek)
		{
			status = m_base->get_position(m_position_base,p_abort);
			if (io_result_failed(status)) return status;
		}
		else
		{
			m_position_base = 0;
		}

		status = m_base->get_size(m_size,p_abort);
		if (io_result_failed(status)) return status;

		flush_buffer();
		
		return io_result_success;
	}
public:

	t_io_result read(void * p_buffer,unsigned p_bytes,unsigned & p_bytes_read,abort_callback & p_abort)
	{
		t_uint8 * outptr = (t_uint8*)p_buffer;
		unsigned done = 0;
		while(done < p_bytes && m_position < m_size)
		{
			if (p_abort.is_aborting()) return io_result_aborted;


			if (m_position >= m_buffer_position && m_position < m_buffer_position + m_buffer_status)
			{
				
				unsigned delta = pfc::min_t<unsigned>((unsigned)(m_buffer_position + m_buffer_status - m_position),p_bytes - done);
				unsigned bufptr = (unsigned)(m_position - m_buffer_position);
				memcpy(outptr+done,m_buffer+bufptr,delta);
				done += delta;
				m_position += delta;

				if (m_buffer_status != sizeof(m_buffer) && done < p_bytes) break;//EOF before m_size is hit
			}
			else
			{
				t_io_result status;
				
				m_buffer_position = m_position - m_position % blocksize;
				status = adjust_position(m_buffer_position,p_abort);
				if (io_result_failed(status)) return status;

				status = m_base->read(m_buffer,sizeof(m_buffer),m_buffer_status,p_abort);
				if (io_result_failed(status)) return status;
				m_position_base += m_buffer_status;

				if (m_buffer_status <= (unsigned)(m_position - m_buffer_position)) break;
			}
		}

		p_bytes_read = done;

		return done == p_bytes ? io_result_success : io_result_eof;
	}

	t_io_result write(const void * p_buffer,unsigned p_bytes,unsigned & p_bytes_written,abort_callback & p_abort)
	{
		if (p_abort.is_aborting()) return io_result_aborted;
		t_io_result status;
		status = adjust_position(m_position,p_abort);
		if (io_result_failed(status)) return status;
		status = m_base->write(p_buffer,p_bytes,p_bytes_written,p_abort);
		if (io_result_failed(status)) return status;
		m_position_base = m_position = m_position + p_bytes_written;
		if (m_size < m_position) m_size = m_position;
		flush_buffer();
		return io_result_success;
	}
	t_io_result get_size(t_filesize & p_size,abort_callback & p_abort)
	{
		if (p_abort.is_aborting()) return io_result_aborted;
		p_size = m_size;
		return io_result_success;
	}
	t_io_result get_position(t_uint64 & p_position,abort_callback & p_abort)
	{
		if (p_abort.is_aborting()) return io_result_aborted;
		p_position = m_position;
		return io_result_success;
	}
	t_io_result set_eof(abort_callback & p_abort)
	{
		if (p_abort.is_aborting()) return io_result_aborted;
		t_io_result status;
		status = adjust_position(m_position,p_abort);
		if (io_result_failed(status)) return status;
		status = m_base->set_eof(p_abort);
		if (io_result_failed(status)) return status;
		flush_buffer();
		return io_result_success;
	}
	t_io_result seek(t_filesize p_position,abort_callback & p_abort)
	{
		if (p_abort.is_aborting()) return io_result_aborted;
		if (!can_seek() || p_position > m_size) return io_result_error_generic;
		m_position = p_position;
		return io_result_success;
	}
	bool can_seek() {return m_can_seek;}
	bool get_content_type(string_base & out) {return m_base->get_content_type(out);}
	void on_idle(abort_callback & p_abort) {m_base->on_idle(p_abort);}
	t_io_result get_timestamp(t_filetimestamp & p_timestamp,abort_callback & p_abort) {return m_base->get_timestamp(p_timestamp,p_abort);}
private:
	t_io_result adjust_position(t_filesize p_target,abort_callback & p_abort)
	{
		t_io_result status;
		if (p_target != m_position_base)
		{
			status = m_base->seek(p_target,p_abort);
			if (io_result_failed(status)) return status;
			m_position_base = p_target;
		}
		return io_result_success;
	}

	void flush_buffer()
	{
		m_buffer_status = 0;
		m_buffer_position = 0;
	}

	service_ptr_t<file> m_base;
	t_filesize m_position,m_position_base,m_size;
	bool m_can_seek;
	t_filesize m_buffer_position;
	unsigned m_buffer_status;
	t_uint8 m_buffer[blocksize];
	
};
