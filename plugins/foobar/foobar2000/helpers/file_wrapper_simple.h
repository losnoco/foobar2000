class file_wrapper_simple
{
public:
	explicit file_wrapper_simple(const service_ptr_t<file> & p_file,abort_callback & p_abort) : m_file(p_file), m_abort(p_abort), m_status(io_result_success) {}

	inline t_io_result get_status() const {return m_status;}
	inline void reset_status() {m_status = io_result_success;}


	unsigned read(void * p_buffer,unsigned p_bytes);
	unsigned write(const void * p_buffer,unsigned p_bytes);
	bool seek(t_uint64 p_offset);
	t_uint64 get_position();
	t_uint64 get_size();
	bool can_seek();
	bool truncate();
private:
	service_ptr_t<file> m_file;
	abort_callback & m_abort;
	t_io_result m_status;
};
