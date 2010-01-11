class stream_reader_buffered : public stream_reader
{
public:
	stream_reader_buffered(stream_reader * p_base,unsigned p_buffer);
	bool check_buffer() const {return m_buffer.get_size() > 0;}
	t_io_result read(void * p_buffer,unsigned p_bytes,unsigned & p_bytes_read,abort_callback & p_abort);
private:
	stream_reader * m_base;
	mem_block_t<char> m_buffer;
	unsigned m_buffer_ptr, m_buffer_max;
};

class stream_writer_buffered : public stream_writer
{
public:
	stream_writer_buffered(stream_writer * p_base,unsigned p_buffer);
	
	bool check_buffer() const {return m_buffer.get_size() > 0;}

	t_io_result write(const void * p_buffer,unsigned p_bytes,unsigned & p_bytes_written,abort_callback & p_abort);

	void flush_e(abort_callback & p_abort);

	t_io_result flush(abort_callback & p_abort);

private:
	stream_writer * m_base;
	mem_block_t<char> m_buffer;
	unsigned m_buffer_ptr;
};

