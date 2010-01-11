class seekabilizer_backbuffer
{
public:
	bool initialize(unsigned p_size);
	void write(const void * p_buffer,unsigned p_bytes);
	void read(unsigned p_backlogdepth,void * p_buffer,unsigned p_bytes) const;
	unsigned get_depth() const;
	void reset();
	unsigned get_max_depth() const;
private:
	mem_block_t<t_uint8> m_buffer;
	unsigned m_depth,m_cursor;
};

class seekabilizer : public file
{
public:
	t_io_result initialize(service_ptr_t<file> p_base,unsigned p_buffer_size,abort_callback & p_abort);
	
	static t_io_result g_seekabilize(service_ptr_t<file> & p_reader,unsigned p_buffer_size,abort_callback & p_abort);
	static void g_seekabilize_e(service_ptr_t<file> & p_reader,unsigned p_buffer_size,abort_callback & p_abort);

	t_io_result read(void * p_buffer,unsigned p_bytes,unsigned & p_bytes_read,abort_callback & p_abort);
	t_io_result write(const void * p_buffer,unsigned p_bytes,unsigned & p_bytes_written,abort_callback & p_abort);
	t_io_result get_size(t_filesize & p_length,abort_callback & p_abort);
	t_io_result get_position(t_uint64 & p_position,abort_callback & p_abort);
	t_io_result set_eof(abort_callback & p_abort);
	t_io_result seek(t_filesize position,abort_callback & p_abort);
	bool can_seek();
	bool get_content_type(string_base & p_out);
	bool is_in_memory();
	void on_idle(abort_callback & p_abort);
	t_io_result get_timestamp(t_filetimestamp & p_timestamp,abort_callback & p_abort);
	t_io_result reopen(abort_callback & p_abort);
	bool is_remote();

private:
	service_ptr_t<file> m_file;
	seekabilizer_backbuffer m_buffer;
	t_filesize m_size,m_position,m_position_base;
};