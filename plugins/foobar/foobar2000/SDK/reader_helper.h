#ifndef _READER_HELPER_H_
#define _READER_HELPER_H_

//helper
class file_path_canonical : public string8
{
public:
	file_path_canonical(const char * src)
	{
		filesystem::g_get_canonical_path(src,*this);
	}
	operator const char * () {return get_ptr();}
};

class file_path_display : public string8
{
public:
	file_path_display(const char * src)
	{
		filesystem::g_get_display_path(src,*this);
	}
	operator const char * () {return get_ptr();}
};



class NOVTABLE reader_membuffer_base : public file
{
public:
	reader_membuffer_base() : m_offset(0) {}

	t_io_result read(void * p_buffer,unsigned p_bytes,unsigned & p_bytes_read,abort_callback & p_abort);

	t_io_result write(const void * p_buffer,unsigned p_bytes,unsigned & p_bytes_written,abort_callback & p_abort) {return io_result_error_generic;}

	t_io_result get_size(t_filesize & p_out,abort_callback & p_abort) {p_out = get_buffer_size();return io_result_success;}
	t_io_result get_position(t_filesize & p_out,abort_callback & p_abort) {p_out = m_offset;return io_result_success;}
	t_io_result set_eof(abort_callback & p_abort) {return io_result_error_generic;}
	t_io_result seek(t_filesize position,abort_callback & p_abort);
	
	bool can_seek() {return true;}
	bool is_in_memory() {return true;}
		
protected:
	virtual const void * get_buffer() = 0;
	virtual unsigned get_buffer_size() = 0;
	virtual t_io_result get_timestamp(t_filetimestamp & p_out,abort_callback & p_abort) = 0;
	inline void seek_internal(unsigned param) {m_offset = param;}
private:
	unsigned m_offset;
};

class reader_membuffer_mirror : public reader_membuffer_base
{
public:
	t_io_result get_timestamp(t_filetimestamp & p_timestamp,abort_callback & p_abort) {p_timestamp = m_timestamp; return io_result_success;}
	bool is_remote() {return m_remote;}

	static bool g_create_e(service_ptr_t<file> & p_out,const service_ptr_t<file> & p_src,abort_callback & p_abort)
	{
		service_ptr_t<reader_membuffer_mirror> ptr = new service_impl_t<reader_membuffer_mirror>();
		if (ptr.is_empty()) return false;
		if (!ptr->init_e(p_src,p_abort)) return false;
		p_out = ptr.get_ptr();
		return true;
	}

private:
	const void * get_buffer() {return m_buffer.get_ptr();}
	unsigned get_buffer_size() {return m_buffer.get_size();}

	bool init_e(const service_ptr_t<file> & p_src,abort_callback & p_abort)
	{
		if (p_src->is_in_memory()) return false;//already buffered
		m_remote = p_src->is_remote();
		t_int64 size64 = p_src->get_size_e(p_abort);
		if (size64<=0 || size64>0x7FFFFFFF) return false;
		unsigned size = (unsigned)size64;

		if (!m_buffer.set_size(size)) throw io_result_error_out_of_memory;

		if (p_src->can_seek()) p_src->seek_e(0,p_abort);
		
		p_src->read_object_e(m_buffer.get_ptr(),size,p_abort);

		m_timestamp = p_src->get_timestamp_e(p_abort);

		return true;
	}


	t_filetimestamp m_timestamp;
	mem_block_t<char> m_buffer;
	bool m_remote;

};

class reader_limited : public file
{
	service_ptr_t<file> r;
	t_filesize begin;
	t_filesize end;
	t_io_result write(const void * p_buffer,unsigned p_bytes,unsigned & p_bytes_written,abort_callback & p_abort) {return io_result_error_denied;}
	
public:
	reader_limited() {begin=0;end=0;}
	reader_limited(const service_ptr_t<file> & p_r,t_int64 p_begin,t_int64 p_end,abort_callback & p_abort)
	{
		r = p_r;
		begin = p_begin;
		end = p_end;
		r->seek(begin,p_abort);
	}

	t_io_result init(const service_ptr_t<file> & p_r,t_int64 p_begin,t_int64 p_end,abort_callback & p_abort)
	{
		r = p_r;
		begin = p_begin;
		end = p_end;
		return r->seek(begin,p_abort);
	}

	t_io_result get_timestamp(t_filetimestamp & p_out,abort_callback & p_abort) {return r->get_timestamp(p_out,p_abort);}

	t_io_result read(void *buffer, unsigned length,unsigned & p_bytes_read,abort_callback & p_abort)
	{
		t_filesize pos;
		t_io_result status;
		status = r->get_position(pos,p_abort);
		if (io_result_failed(status)) return status;
		bool eof = false;
		if (length > end - pos) {length = (unsigned)(end - pos);eof=true;}
		status = r->read(buffer,length,p_bytes_read,p_abort);
		if (eof && status == io_result_success) status = io_result_eof;
		return status;
	}

	t_io_result get_size(t_filesize & p_out,abort_callback & p_abort)
	{
		p_out = end-begin;
		return io_result_success;
	}

	t_io_result get_position(t_filesize & p_out,abort_callback & p_abort)
	{
		t_filesize temp;
		t_io_result status;
		status = r->get_position(temp,p_abort);
		if (io_result_succeeded(status)) p_out = temp - begin;
		return status;
	}

	t_io_result seek(t_filesize position,abort_callback & p_abort)
	{
		return r->seek(position+begin,p_abort);
	}
	bool can_seek() {return r->can_seek();}
	bool is_remote() {return r->is_remote();}
	
	t_io_result set_eof(abort_callback & p_abort) {return io_result_error_denied;}
};

class stream_reader_memblock_ref : public stream_reader
{
public:
	stream_reader_memblock_ref(const void * p_data,unsigned p_data_size) : m_data((const unsigned char*)p_data), m_data_size(p_data_size), m_pointer(0) {}
	t_io_result read(void * p_buffer,unsigned p_bytes,unsigned & p_bytes_read,abort_callback & p_abort)
	{
		unsigned remaining = m_data_size - m_pointer;
		unsigned toread = p_bytes;
		if (toread > remaining) toread = remaining;
		if (toread > 0)
		{
			memcpy(p_buffer,m_data+m_pointer,toread);
			m_pointer += toread;
		}

		p_bytes_read = toread;
		return p_bytes == toread ? io_result_success : io_result_eof;
	}
private:
	const unsigned char * m_data;
	unsigned m_data_size,m_pointer;
};

class stream_reader_limited_ref : public stream_reader
{
public:
	stream_reader_limited_ref(stream_reader * p_reader,unsigned p_limit) : m_reader(p_reader), m_remaining(p_limit) {}
	
	t_io_result read(void * p_buffer,unsigned p_bytes,unsigned & p_bytes_read,abort_callback & p_abort)
	{
		bool eof = false;
		if (p_bytes > m_remaining)
		{
			eof = true;
			p_bytes = m_remaining;
		}

		t_io_result status;
		status = m_reader->read(p_buffer,p_bytes,p_bytes_read,p_abort);
		if (io_result_failed(status)) return status;
		m_remaining -= p_bytes_read;
		if (eof) return io_result_eof;
		return status;
	}

private:
	stream_reader * m_reader;
	unsigned m_remaining;
};

class stream_writer_chunk_dwordheader : public stream_writer
{
public:
	stream_writer_chunk_dwordheader(const service_ptr_t<file> & p_writer) : m_writer(p_writer)
	{
	}

	t_io_result initialize(abort_callback & p_abort)
	{
		t_io_result status;
		status = m_writer->get_position(m_headerposition,p_abort);
		if (io_result_failed(status)) return status;
		m_written = 0;
		unsigned long dummy = 0;
		return m_writer->write_lendian_t(dummy,p_abort);
	}

	t_io_result finalize(abort_callback & p_abort)
	{
		t_io_result status;
		t_filesize end_offset;
		status = m_writer->get_position(end_offset,p_abort);
		if (io_result_failed(status)) return status;
		status = m_writer->seek(m_headerposition,p_abort);
		if (io_result_failed(status)) return status;
		status = m_writer->write_lendian_t(m_written,p_abort);
		if (io_result_failed(status)) return status;
		status = m_writer->seek(end_offset,p_abort);
		if (io_result_failed(status)) return status;
		return io_result_success;
	}

	t_io_result write(const void * p_buffer,unsigned p_bytes,unsigned & p_bytes_written,abort_callback & p_abort)
	{
		t_io_result status = m_writer->write(p_buffer,p_bytes,p_bytes_written,p_abort);
		if (io_result_succeeded(status)) m_written += p_bytes_written;
		return status;
	}

private:
	service_ptr_t<file> m_writer;
	t_filesize m_headerposition;
	unsigned long m_written;
};

class stream_writer_chunk : public stream_writer
{
public:
	stream_writer_chunk(stream_writer * p_writer) : m_writer(p_writer), m_buffer_state(0) {}

	t_io_result write(const void * p_buffer,unsigned p_bytes,unsigned & p_bytes_written,abort_callback & p_abort);

	t_io_result flush(abort_callback & p_abort);//must be called after writing before object is destroyed
	
private:
	stream_writer * m_writer;
	unsigned m_buffer_state;
	unsigned char m_buffer[255];
};

class stream_reader_chunk : public stream_reader
{
public:
	stream_reader_chunk(stream_reader * p_reader) : m_reader(p_reader), m_buffer_state(0), m_buffer_size(0), m_eof(false) {}

	t_io_result read(void * p_buffer,unsigned p_bytes,unsigned & p_bytes_read,abort_callback & p_abort);

	t_io_result flush(abort_callback & p_abort);//must be called after reading before object is destroyed

	static t_io_result g_skip(stream_reader * p_stream,abort_callback & p_abort);

private:
	stream_reader * m_reader;
	unsigned m_buffer_state, m_buffer_size;
	bool m_eof;
	unsigned char m_buffer[255];
};

class stream_writer_wrapper_write_config_callback : public stream_writer
{
public:
	stream_writer_wrapper_write_config_callback(cfg_var::write_config_callback * p_out) : m_out(p_out) {}
	
	t_io_result write(const void * p_buffer,unsigned p_bytes,unsigned & p_bytes_written,abort_callback & p_abort);

private:
	cfg_var::write_config_callback * m_out;
};

#endif//_READER_HELPER_H_